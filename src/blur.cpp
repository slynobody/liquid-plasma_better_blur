/*
    SPDX-FileCopyrightText: 2010 Fredrik Höglund <fredrik@kde.org>
    SPDX-FileCopyrightText: 2011 Philipp Knechtges <philipp-dev@knechtges.com>
    SPDX-FileCopyrightText: 2018 Alex Nemeth <alex.nemeth329@gmail.com>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "blur.h"
// KConfigSkeleton
#include "blurconfig.h"

#include "blur_cache.hpp"
#include "better_blur_dx_api.hpp"
#include "kwin_compat.hpp"
#include "refraction_pass.hpp"
#include "rounded_corners_pass.hpp"
#include "settings.hpp"
#include "utils.h"
#include "window_manager.hpp"

#if KWIN_VERSION >= KWIN_VERSION_CODE(6, 5, 80)
#  include <core/rect.h>
#  include <core/region.h>
#endif

#include <core/output.h>
#include <core/pixelgrid.h>
#include <core/rendertarget.h>
#include <core/renderviewport.h>
#include <effect/effect.h>
#include <effect/effecthandler.h>
#include <opengl/glplatform.h>
#include <opengl/glshadermanager.h>
#include <opengl/glutils.h>
#if KWIN_VERSION >= KWIN_VERSION_CODE(6, 6, 4)
#include <scene/backgroundeffectitem.h>
#endif
#include <scene/decorationitem.h>
#include <scene/borderradius.h>
#include <scene/scene.h>
#include <scene/surfaceitem.h>
#include <scene/windowitem.h>
#include <utils/xcbutils.h>
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
#include <wayland/blur.h>
#include <wayland/contrast.h>
#endif
#include <wayland/display.h>
#include <wayland/surface.h>
#include <x11window.h>
#if KWIN_VERSION >= KWIN_VERSION_CODE(6, 6, 91)
// not shipped in KWin 6.6.90
// https://invent.kde.org/plasma/kwin/-/merge_requests/9214
#include <wayland/backgroundeffect_v1.h>
#include <wayland_server.h>
#endif

#include <QGuiApplication>
#include <QMatrix4x4>
#include <QScreen>
#include <QTime>
#include <QTimer>
#include <QVariant>
#include <QWindow>
#include <cmath> // for ceil()
#include <cstdlib>

#include <KConfigGroup>
#include <KSharedConfig>

#include <KDecoration3/Decoration>

#include <algorithm>
#include <memory>
#include <optional>
#include <utility>

Q_LOGGING_CATEGORY(KWIN_BLUR, "kwin_effect_better_blur_dx_wobbly_api", QtInfoMsg)

static void ensureResources()
{
    // Must initialize resources manually because the effect is a static lib.
    Q_INIT_RESOURCE(blur_wobbly_api);
}

namespace BBDX {
using namespace KWin;

static const QByteArray s_blurAtomName = QByteArrayLiteral("_KDE_NET_WM_BLUR_BEHIND_REGION");
static void clearBetterWobblyProviderResult(EffectWindow *w)
{
    w->setData(BetterBlurDxApi::ResultRole, QVariant());
}


#if !defined(BBDX_X11) && KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
BlurManagerInterface *BlurEffect::s_blurManager = nullptr;
QTimer *BlurEffect::s_blurManagerRemoveTimer = nullptr;

ContrastManagerInterface *BlurEffect::s_contrastManager = nullptr;
QTimer *BlurEffect::s_contrastManagerRemoveTimer = nullptr;
#endif

static QMatrix4x4 colorTransformMatrix(qreal saturation, qreal contrast, qreal brightness)
{
    QMatrix4x4 saturationMatrix;
    QMatrix4x4 contrastMatrix;
    QMatrix4x4 brightnessMatrix;

    if (!qFuzzyCompare(saturation, 1.0)) {
        const qreal rval = (1.0 - saturation) * 0.2126;
        const qreal gval = (1.0 - saturation) * 0.7152;
        const qreal bval = (1.0 - saturation) * 0.0722;

        saturationMatrix = QMatrix4x4(rval + saturation, rval, rval, 0.0,
                                      gval, gval + saturation, gval, 0.0,
                                      bval, bval, bval + saturation, 0.0,
                                      0.0, 0.0, 0.0, 1.0);
    }

    if (!qFuzzyCompare(contrast, 1.0)) {
        const float transl = (1.0 - contrast) / 2.0;

        contrastMatrix = QMatrix4x4(contrast, 0.0, 0.0, 0.0,
                                    0.0, contrast, 0.0, 0.0,
                                    0.0, 0.0, contrast, 0.0,
                                    transl, transl, transl, 1.0);
    }

    if (!qFuzzyCompare(brightness, 1.0)) {
        brightnessMatrix.scale(brightness, brightness, brightness);
    }

    return contrastMatrix * saturationMatrix * brightnessMatrix;
}

BlurEffect::BlurEffect()
{
    BlurConfig::instance(effects->config());
    ensureResources();

    // Initialize mouse tracking for border/mouse highlight
    m_lastMousePosition = effects->cursorPos();

    m_onscreenPass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                              BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/vertex.vert"),
                                                                              BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/onscreen.frag"));
    if (!m_onscreenPass.shader) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to load onscreen pass shader";
        return;
    } else {
        m_onscreenPass.mvpMatrixLocation = m_onscreenPass.shader->uniformLocation("modelViewProjectionMatrix");
        m_onscreenPass.colorMatrixLocation = m_onscreenPass.shader->uniformLocation("colorMatrix");
        m_onscreenPass.offsetLocation = m_onscreenPass.shader->uniformLocation("offset");
        m_onscreenPass.halfpixelLocation = m_onscreenPass.shader->uniformLocation("halfpixel");
    }

    m_wobblyCompositePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                                      BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/vertex.vert"),
                                                                                      BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/wobbly_composite.frag"));
    if (!m_wobblyCompositePass.shader) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to load Better Wobbly provider composite shader";
        return;
    }
    m_wobblyCompositePass.mvpMatrixLocation = m_wobblyCompositePass.shader->uniformLocation("modelViewProjectionMatrix");
    m_wobblyCompositePass.textureSizeLocation = m_wobblyCompositePass.shader->uniformLocation("textureSize");
    m_wobblyCompositePass.opacityLocation = m_wobblyCompositePass.shader->uniformLocation("opacity");
    m_wobblyCompositePass.boxLocation = m_wobblyCompositePass.shader->uniformLocation("box");
    m_wobblyCompositePass.cornerRadiusLocation = m_wobblyCompositePass.shader->uniformLocation("cornerRadius");
    m_wobblyCompositePass.roundedMaskEnabledLocation =
        m_wobblyCompositePass.shader->uniformLocation("roundedMaskEnabled");

#if BBDX_NOT_NEEDED
    m_roundedOnscreenPass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                                     QStringLiteral(":/effects/better_blur_dx_wobbly_api/shaders/onscreen_rounded.vert"),
                                                                                     QStringLiteral(":/effects/better_blur_dx_wobbly_api/shaders/onscreen_rounded.frag"));
    if (!m_roundedOnscreenPass.shader) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to load onscreen pass shader";
        return;
    } else {
        m_roundedOnscreenPass.mvpMatrixLocation = m_roundedOnscreenPass.shader->uniformLocation("modelViewProjectionMatrix");
        m_roundedOnscreenPass.colorMatrixLocation = m_roundedOnscreenPass.shader->uniformLocation("colorMatrix");
        m_roundedOnscreenPass.offsetLocation = m_roundedOnscreenPass.shader->uniformLocation("offset");
        m_roundedOnscreenPass.halfpixelLocation = m_roundedOnscreenPass.shader->uniformLocation("halfpixel");
        m_roundedOnscreenPass.boxLocation = m_roundedOnscreenPass.shader->uniformLocation("box");
        m_roundedOnscreenPass.cornerRadiusLocation = m_roundedOnscreenPass.shader->uniformLocation("cornerRadius");
        m_roundedOnscreenPass.opacityLocation = m_roundedOnscreenPass.shader->uniformLocation("opacity");
    }
#endif

    m_downsamplePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                                BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/vertex.vert"),
                                                                                BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/downsample.frag"));
    if (!m_downsamplePass.shader) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to load downsampling pass shader";
        return;
    } else {
        m_downsamplePass.mvpMatrixLocation = m_downsamplePass.shader->uniformLocation("modelViewProjectionMatrix");
        m_downsamplePass.offsetLocation = m_downsamplePass.shader->uniformLocation("offset");
        m_downsamplePass.halfpixelLocation = m_downsamplePass.shader->uniformLocation("halfpixel");
    }

    m_upsamplePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                              BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/vertex.vert"),
                                                                              BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/upsample.frag"));
    if (!m_upsamplePass.shader) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to load upsampling pass shader";
        return;
    } else {
        m_upsamplePass.mvpMatrixLocation = m_upsamplePass.shader->uniformLocation("modelViewProjectionMatrix");
        m_upsamplePass.offsetLocation = m_upsamplePass.shader->uniformLocation("offset");
        m_upsamplePass.halfpixelLocation = m_upsamplePass.shader->uniformLocation("halfpixel");
    }

    m_noisePass.shader = ShaderManager::instance()->generateShaderFromFile(ShaderTrait::MapTexture,
                                                                           BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/vertex.vert"),
                                                                           BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/noise.frag"));
    if (!m_noisePass.shader) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to load noise pass shader";
        return;
    } else {
        m_noisePass.mvpMatrixLocation = m_noisePass.shader->uniformLocation("modelViewProjectionMatrix");
        m_noisePass.noiseTextureSizeLocation = m_noisePass.shader->uniformLocation("noiseTextureSize");
    }

    // BBDX: managed extension objects
    m_windowManager = std::make_unique<BBDX::WindowManager>(this);

    m_blurCache = BBDX::BlurCache::create(this);
    if (!m_blurCache) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to create BlurCache";
        return;
    }

    m_refractionPass = BBDX::RefractionPass::create();
    if (!m_refractionPass) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to create RefractionPass";
        return;
    }

    m_roundedCornersPass = BBDX::RoundedCornersPass::create();
    if (!m_roundedCornersPass) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to create RoundedCornersPass";
        return;
    }

    initBlurStrengthValues();
    reconfigure(ReconfigureAll);

    if (effects->xcbConnection()) {
        net_wm_blur_region = effects->announceSupportProperty(s_blurAtomName, this);
    }

#if !defined(BBDX_X11) && KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    if (!s_blurManagerRemoveTimer) {
        s_blurManagerRemoveTimer = new QTimer(QCoreApplication::instance());
        s_blurManagerRemoveTimer->setSingleShot(true);
        s_blurManagerRemoveTimer->callOnTimeout([]() {
            s_blurManager->remove();
            s_blurManager = nullptr;
        });
    }
    s_blurManagerRemoveTimer->stop();
    if (!s_blurManager) {
        s_blurManager = new BlurManagerInterface(effects->waylandDisplay(), s_blurManagerRemoveTimer);
    }

    if (!s_contrastManagerRemoveTimer) {
        s_contrastManagerRemoveTimer = new QTimer(QCoreApplication::instance());
        s_contrastManagerRemoveTimer->setSingleShot(true);
        s_contrastManagerRemoveTimer->callOnTimeout([]() {
            s_contrastManager->remove();
            s_contrastManager = nullptr;
        });
    }
    s_contrastManagerRemoveTimer->stop();
    if (!s_contrastManager) {
        s_contrastManager = new ContrastManagerInterface(effects->waylandDisplay(), s_contrastManagerRemoveTimer);
    }
#endif

#if KWIN_VERSION >= KWIN_VERSION_CODE(6, 6, 91)
    waylandServer()->backgroundEffectManager()->addBlurCapability();
#endif

    connect(effects, &EffectsHandler::windowAdded, this, &BlurEffect::slotWindowAdded);
    connect(effects, &EffectsHandler::windowDeleted, this, &BlurEffect::slotWindowDeleted);
#if defined(BBDX_X11)
    connect(effects, &EffectsHandler::screenRemoved, this, &BlurEffect::slotViewRemoved);
#else
    connect(effects, &EffectsHandler::viewRemoved, this, &BlurEffect::slotViewRemoved);
#endif
    connect(effects, &EffectsHandler::propertyNotify, this, &BlurEffect::slotPropertyNotify);
    connect(effects, &EffectsHandler::xcbConnectionChanged, this, [this]() {
        net_wm_blur_region = effects->announceSupportProperty(s_blurAtomName, this);
    });

    // Fetch the blur regions for all windows
    const auto stackingOrder = effects->stackingOrder();
    for (EffectWindow *window : stackingOrder) {
        slotWindowAdded(window);
    }

    m_valid = true;
}

BlurEffect::~BlurEffect()
{
#if !defined(BBDX_X11) && KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    // When compositing is restarted, avoid removing the manager immediately.
    if (s_blurManager) {
        s_blurManagerRemoveTimer->start(1000);
    }

    if (s_contrastManager) {
        s_contrastManagerRemoveTimer->start(1000);
    }
#elif KWIN_VERSION >= KWIN_VERSION_CODE(6, 6, 91)
    waylandServer()->backgroundEffectManager()->removeBlurCapability();
#endif
}

void BlurEffect::initBlurStrengthValues()
{
    // This function creates an array of blur strength values that are evenly distributed

    // The range of the slider on the blur settings UI
    int numOfBlurSteps = 15;
    int remainingSteps = numOfBlurSteps;

    /*
     * Explanation for these numbers:
     *
     * The texture blur amount depends on the downsampling iterations and the offset value.
     * By changing the offset we can alter the blur amount without relying on further downsampling.
     * But there is a minimum and maximum value of offset per downsample iteration before we
     * get artifacts.
     *
     * The minOffset variable is the minimum offset value for an iteration before we
     * get blocky artifacts because of the downsampling.
     *
     * The maxOffset value is the maximum offset value for an iteration before we
     * get diagonal line artifacts because of the nature of the dual kawase blur algorithm.
     *
     * The expandSize value is the minimum value for an iteration before we reach the end
     * of a texture in the shader and sample outside of the area that was copied into the
     * texture from the screen.
     */

    // {minOffset, maxOffset, expandSize}
    blurOffsets.append({1.0, 2.0, 10}); // Down sample size / 2
    blurOffsets.append({2.0, 3.0, 20}); // Down sample size / 4
    blurOffsets.append({2.0, 5.0, 50}); // Down sample size / 8
    blurOffsets.append({3.0, 8.0, 150}); // Down sample size / 16
    // blurOffsets.append({5.0, 10.0, 400}); // Down sample size / 32
    // blurOffsets.append({7.0, ?.0});       // Down sample size / 64

    float offsetSum = 0;

    for (int i = 0; i < blurOffsets.size(); i++) {
        offsetSum += blurOffsets[i].maxOffset - blurOffsets[i].minOffset;
    }

    for (int i = 0; i < blurOffsets.size(); i++) {
        int iterationNumber = std::ceil((blurOffsets[i].maxOffset - blurOffsets[i].minOffset) / offsetSum * numOfBlurSteps);
        remainingSteps -= iterationNumber;

        if (remainingSteps < 0) {
            iterationNumber += remainingSteps;
        }

        float offsetDifference = blurOffsets[i].maxOffset - blurOffsets[i].minOffset;

        for (int j = 1; j <= iterationNumber; j++) {
            // {iteration, offset}
            blurStrengthValues.append({i + 1, blurOffsets[i].minOffset + (offsetDifference / iterationNumber) * j});
        }
    }
}

void BlurEffect::reconfigure(ReconfigureFlags flags)
{
    Q_UNUSED(flags);
    BlurConfig::self()->read();
    m_refractionPass->reconfigure();
    m_windowManager->reconfigure();
    m_blurCache->reconfigure();
    m_forceContrastParams = BlurConfig::forceContrastParams();

    int blurStrength = BlurConfig::blurStrength() - 1;
    m_iterationCount = blurStrengthValues[blurStrength].iteration;
    m_offset = blurStrengthValues[blurStrength].offset;
    m_expandSize = blurOffsets[m_iterationCount - 1].expandSize;
    m_noiseStrength = BlurConfig::noiseStrength();
    m_colorMatrix = colorTransformMatrix(BlurConfig::saturation() / 100.0,
                                         BlurConfig::contrast() / 100.0,
                                         BlurConfig::brightness() / 100.0);
#if KWIN_VERSION >= KWIN_VERSION_CODE(6, 6, 4)
    for (auto &[window, data] : m_windows) {
        data.blurItem->setPixelsToExpandRepaintsBelowOpaqueRegions(m_expandSize);
    }
#endif

    for (EffectWindow *w : effects->stackingOrder()) {
        updateBlurRegion(w);
    }

    // Update all windows for the blur to take effect
    effects->addRepaintFull();
}

void BlurEffect::updateBlurRegion(EffectWindow *w)
{
    std::optional<RegionF> content;
    std::optional<RegionF> frame;
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    std::optional<qreal> saturation;
    std::optional<qreal> contrast;
#endif

    if (net_wm_blur_region != XCB_ATOM_NONE) {
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
        const QByteArray value = w->readProperty(net_wm_blur_region, XCB_ATOM_CARDINAL, 32);
        Region region;
        if (value.size() > 0 && !(value.size() % (4 * sizeof(uint32_t)))) {
            const uint32_t *cardinals = reinterpret_cast<const uint32_t *>(value.constData());
            for (unsigned int i = 0; i < value.size() / sizeof(uint32_t);) {
                int x = cardinals[i++];
                int y = cardinals[i++];
                int w = cardinals[i++];
                int h = cardinals[i++];
                region += Xcb::fromXNative(Rect(x, y, w, h)).toRect();
            }
        }
        if (!value.isNull()) {
            content = region;
        }
#else
        if (const auto x11Window = qobject_cast<X11Window *>(w->window())) {
            Xcb::Property wmBlurRegionProperty(false, x11Window->window(), net_wm_blur_region, XCB_ATOM_CARDINAL, 0, 32768);
            if (const auto cardinals = wmBlurRegionProperty.array<uint32_t>()) {
                if (cardinals->size() == 0 || cardinals->size() == 1) {
                    // It means blur background behind whole window.
                    content = RegionF();
                } else if (cardinals->size() % 4 == 0) {
                    RegionF region;
                    for (uint i = 0; i < cardinals->size();) {
                        const int x = (*cardinals)[i++];
                        const int y = (*cardinals)[i++];
                        const int w = (*cardinals)[i++];
                        const int h = (*cardinals)[i++];
                        region += Xcb::fromXNative(Rect(x, y, w, h));
                    }
                    content = region;
                }
            }
        }
#endif
    }

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    if (SurfaceInterface *surface = w->surface()) {
        if (surface->blur()) {
            content = surface->blur()->region();
        }
        if (surface->contrast()) {
            saturation = surface->contrast()->saturation();
            contrast = surface->contrast()->contrast();
        }
    }
#else
    if (SurfaceInterface *surface = w->surface()) {
        if (!surface->blurRegion().isEmpty()) {
            content = surface->blurRegion();
        }
    }
#endif

    if (auto internal = w->internalWindow()) {
        const auto property = internal->property("kwin_blur");
        if (property.isValid()) {
            content = property.value<RegionF>();
        }
    }

    if (w->decorationHasAlpha() && decorationSupportsBlurBehind(w)) {
        frame = decorationBlurRegion(w);
    }

    // BBDX:
    m_windowManager->getFinalBlurRegion(w, content, frame);

    if (content.has_value() || frame.has_value()) {
        BlurEffectData &data = m_windows[w];
        data.content = content;
        data.frame = frame;
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
        if (m_forceContrastParams) {
            data.colorMatrix.reset();
        } else if (saturation || contrast) {
            data.colorMatrix = colorTransformMatrix(saturation.value_or(1.0), contrast.value_or(1.0), 1.0);
        } else {
            data.colorMatrix.reset();
        }
#endif
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 4)
        data.windowEffect = ItemEffect(w->windowItem());
#else
        if (!data.blurItem) {
            data.blurItem = std::make_unique<BackgroundEffectItem>(w->windowItem());
        }
        data.blurItem->setPixelsToExpandRepaintsBelowOpaqueRegions(m_expandSize);
        data.blurItem->setEffectBoundingRect(blurRegion(w).boundingRect());
#endif
    } else {
        if (auto it = m_windows.find(w); it != m_windows.end()) {
            effects->makeOpenGLContextCurrent();
            m_windows.erase(it);
        }
    }
}

void BlurEffect::slotWindowAdded(EffectWindow *w)
{
    SurfaceInterface *surf = w->surface();

    if (surf) {
        windowBlurChangedConnections[w] = connect(surf, &SurfaceInterface::blurChanged, this, [this, w]() {
            if (w) {
                updateBlurRegion(w);
            }
        });
#if !defined(BBDX_X11) && KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
        windowContrastChangedConnections[w] = connect(surf, &SurfaceInterface::contrastChanged, this, [this, w]() {
            if (w) {
                updateBlurRegion(w);
            }
        });
#endif
    }
    if (auto internal = w->internalWindow()) {
        internal->installEventFilter(this);
    }

    setupDecorationConnections(w);
    connect(w, &EffectWindow::windowDecorationChanged, this, [this, w]() {
        setupDecorationConnections(w);
        updateBlurRegion(w);
    });

    updateBlurRegion(w);
}

void BlurEffect::slotWindowDeleted(EffectWindow *w)
{
    if (auto it = m_windows.find(w); it != m_windows.end()) {
        effects->makeOpenGLContextCurrent();
        m_windows.erase(it);
    }
    if (auto it = windowBlurChangedConnections.find(w); it != windowBlurChangedConnections.end()) {
        disconnect(*it);
        windowBlurChangedConnections.erase(it);
    }
#if !defined(BBDX_X11) && KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    if (auto it = windowContrastChangedConnections.find(w); it != windowContrastChangedConnections.end()) {
        disconnect(*it);
        windowContrastChangedConnections.erase(it);
    }
#endif
}

void BlurEffect::slotViewRemoved(KWin::RenderView *view)
{
    for (auto &[window, data] : m_windows) {
        if (auto it = data.render.find(view); it != data.render.end()) {
            effects->makeOpenGLContextCurrent();
            data.render.erase(it);
        }
    }

    // BBDX: cleanup wallpaper
    m_blurCache->dropWallpaper(view);
}

void BlurEffect::slotPropertyNotify(EffectWindow *w, long atom)
{
    if (w && atom == net_wm_blur_region && net_wm_blur_region != XCB_ATOM_NONE) {
        updateBlurRegion(w);
    }
}

void BlurEffect::setupDecorationConnections(EffectWindow *w)
{
    if (!w->decoration()) {
        return;
    }

    connect(w->decoration(), &KDecoration3::Decoration::blurRegionChanged, this, [this, w]() {
        updateBlurRegion(w);
    });
}

bool BlurEffect::eventFilter(QObject *watched, QEvent *event)
{
    auto internal = qobject_cast<QWindow *>(watched);
    if (internal && event->type() == QEvent::DynamicPropertyChange) {
        QDynamicPropertyChangeEvent *pe = static_cast<QDynamicPropertyChangeEvent *>(event);
        if (pe->propertyName() == "kwin_blur") {
            if (auto w = effects->findWindow(internal)) {
                updateBlurRegion(w);
            }
        }
    }
    return false;
}

bool BlurEffect::enabledByDefault()
{
#if BBDX_NOT_NEEDED
    const auto context = effects->openglContext();
    if (!context || context->isSoftwareRenderer()) {
        return false;
    }
    GLPlatform *gl = context->glPlatform();

    if (gl->isIntel() && gl->chipClass() < SandyBridge) {
        return false;
    }
    if (gl->isPanfrost() && gl->chipClass() <= MaliT8XX) {
        return false;
    }
    // The blur effect works, but is painfully slow (FPS < 5) on Mali and VideoCore
    if (gl->isLima() || gl->isVideoCore4() || gl->isVideoCore3D()) {
        return false;
    }
    return true;
#endif
    return false;
}

bool BlurEffect::supported()
{
    const auto context = effects->openglContext();
    if (!context) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Effect unsupported: No OpenGL context";
        return false;
    }

    if (!effects->isOpenGLCompositing()) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Effect unsupported: No OpenGL compositing";
        return false;
    }

    if (!context->supportsTextureSwizzle()) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Effect unsupported: OpenGL context does not support texture swizzle (GL_ARB_texture_swizzle)";
        return false;
    }

    qCInfo(KWIN_BLUR) << BBDX::LOG_PREFIX << "Effect supported";
    return true;
}

bool BlurEffect::decorationSupportsBlurBehind(const EffectWindow *w) const
{
    return w->decoration() && !w->decoration()->blurRegion().isNull();
}

RegionF BlurEffect::decorationBlurRegion(const EffectWindow *w) const
{
    if (!decorationSupportsBlurBehind(w)) {
        return RegionF();
    }

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    Region decorationRegion = Region(Rect(w->decoration()->rect().toAlignedRect())) - w->contentsRect().toRect();
#else
    RegionF decorationRegion = RegionF(w->decoration()->rect()) - w->contentsRect();
#endif
    //! we return only blurred regions that belong to decoration region
    return decorationRegion.intersected(RegionF(w->decoration()->blurRegion()));
}

RegionF BlurEffect::blurRegion(EffectWindow *w) const
{
    RegionF region;

    if (auto it = m_windows.find(w); it != m_windows.end()) {
        const std::optional<RegionF> &content = it->second.content;
        const std::optional<RegionF> &frame = it->second.frame;
        if (content.has_value()) {
            if (content->isEmpty()) {
                // An empty region means that the blur effect should be enabled
                // for the whole window.
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
                region = Rect(w->contentsRect().toRect());
            } else {
                region = content->translated(w->contentsRect().topLeft().toPoint()) & w->contentsRect().toRect();
#else
                region = w->contentsRect();
            } else {
                region = content->translated(w->contentsRect().topLeft()) & w->contentsRect();
#endif
            }
            if (frame.has_value()) {
                region += frame.value();
            }
        } else if (frame.has_value()) {
            region = frame.value();
        }
    }

    return region;
}

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
void BlurEffect::prePaintScreen(ScreenPrePaintData &data, std::chrono::milliseconds presentTime)
#else
void BlurEffect::prePaintScreen(ScreenPrePaintData &data)
#endif
{
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 4)
    m_paintedDeviceArea = Region();
    m_currentDeviceBlur = Region();
#endif // KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 4)
#if defined(BBDX_X11)
    m_currentView = data.screen;
#else
    m_currentView = data.view;
#endif

    m_blurCache->flushAccumulatedDirtyRegions(data);

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    effects->prePaintScreen(data, presentTime);
#else
    effects->prePaintScreen(data);
#endif
}

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 5, 80)
void BlurEffect::prePaintWindow(EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
#elif KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
void BlurEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data, std::chrono::milliseconds presentTime)
#else
void BlurEffect::prePaintWindow(RenderView *view, EffectWindow *w, WindowPrePaintData &data)
#endif
{
    // this effect relies on prePaintWindow being called in the bottom to top order

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 5, 80)
    effects->prePaintWindow(w, data, presentTime);

    // When the original Better Blur DX is enabled, this fork acts as an
    // API-only companion so the two effects do not composite the same normal
    // blur twice. When this fork is the only Better Blur DX implementation,
    // retain the complete upstream pre-paint path so stationary windows are
    // blurred normally.
    if (upstreamBetterBlurDxLoaded()
        && !BetterBlurDxApi::isRequest(w->data(BetterBlurDxApi::RequestRole))) {
        return;
    }

    const QRegion oldOpaque = data.opaque;
    if (data.opaque.intersects(m_currentDeviceBlur)) {
        // to blur an area partially we have to shrink the opaque area of a window
        QRegion newOpaque;
        for (const QRect &rect : data.opaque) {
            newOpaque += rect.adjusted(m_expandSize, m_expandSize, -m_expandSize, -m_expandSize);
        }
        data.opaque = newOpaque;

        // we don't have to blur a region we don't see
        m_currentDeviceBlur -= newOpaque;
    }

    // if we have to paint a non-opaque part of this window that intersects with the
    // currently blurred region we have to redraw the whole region
    if ((data.paint - oldOpaque).intersects(m_currentDeviceBlur)) {
        data.paint += m_currentDeviceBlur;
    }

    // in case this window has regions to be blurred
    const QRegion blurArea = blurRegion(w).boundingRect().translated(w->pos().toPoint());

    // if this window or a window underneath the blurred area is painted again we have to
    // blur everything
    if (m_paintedDeviceArea.intersects(blurArea) || data.paint.intersects(blurArea)) {
        data.paint += blurArea;
        // we have to check again whether we do not damage a blurred area
        // of a window
        if (blurArea.intersects(m_currentDeviceBlur)) {
            data.paint += m_currentDeviceBlur;
        }
    }

    m_currentDeviceBlur += blurArea;

    m_paintedDeviceArea -= data.opaque;
    m_paintedDeviceArea += data.paint;
#elif KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 4)
    effects->prePaintWindow(view, w, data, presentTime);

    if (upstreamBetterBlurDxLoaded()
        && !BetterBlurDxApi::isRequest(w->data(BetterBlurDxApi::RequestRole))) {
        return;
    }

    const Region oldOpaque = data.deviceOpaque;
    if (data.deviceOpaque.intersects(m_currentDeviceBlur)) {
        // to blur an area partially we have to shrink the opaque area of a window
        Region newOpaque;
        for (const Rect &rect : data.deviceOpaque.rects()) {
            newOpaque += rect.adjusted(m_expandSize, m_expandSize, -m_expandSize, -m_expandSize);
        }
        data.deviceOpaque = newOpaque;

        // we don't have to blur a region we don't see
        m_currentDeviceBlur -= newOpaque;
    }

    // if we have to paint a non-opaque part of this window that intersects with the
    // currently blurred region we have to redraw the whole region
    if ((data.devicePaint - oldOpaque).intersects(m_currentDeviceBlur)) {
        data.devicePaint += m_currentDeviceBlur;
    }

    // in case this window has regions to be blurred
    const Region blurArea = view->mapToDeviceCoordinatesAligned(QRectF(blurRegion(w).boundingRect()).translated(w->pos()));

    // if this window or a window underneath the blurred area is painted again we have to
    // blur everything
    if (m_paintedDeviceArea.intersects(blurArea) || data.devicePaint.intersects(blurArea)) {
        data.devicePaint += blurArea;
        // we have to check again whether we do not damage a blurred area
        // of a window
        if (blurArea.intersects(m_currentDeviceBlur)) {
            data.devicePaint += m_currentDeviceBlur;
        }
    }

    m_currentDeviceBlur += blurArea;

    m_paintedDeviceArea -= data.deviceOpaque;
    m_paintedDeviceArea += data.devicePaint;
#elif KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    effects->prePaintWindow(view, w, data, presentTime);

    if (upstreamBetterBlurDxLoaded()
        && !BetterBlurDxApi::isRequest(w->data(BetterBlurDxApi::RequestRole))) {
        return;
    }
#else
    effects->prePaintWindow(view, w, data);

    if (upstreamBetterBlurDxLoaded()
        && !BetterBlurDxApi::isRequest(w->data(BetterBlurDxApi::RequestRole))) {
        return;
    }
#endif

    // BBDX change:
    // blurred windows should be painted translucent
    // to avoid issues with repainting because KWin would
    // cull some areas leading to incomplete blits.
    //
    // Wallpaper mode does not need this because it
    // doesn't rely on blitting
    if (m_windowManager->windowIsBlurred(w) && m_blurCache->blitMode() != BlitMode::WALLPAPER) {
        data.setTranslucent();
    }
}

bool BlurEffect::upstreamBetterBlurDxLoaded() const
{
    // KWin identifies the original plugin by the CMake/plugin target name.
    // Check this every frame rather than caching it so toggling either effect in
    // System Settings immediately switches ownership without restarting KWin.
    return effects && effects->loadedEffects().contains(QStringLiteral("better_blur_dx"));
}

bool BlurEffect::canHandleBetterWobblyProviderRequest(const EffectWindow *w) const
{
    if (!w) {
        return false;
    }
    const QVariant request = w->data(BetterBlurDxApi::RequestRole);
    return BetterBlurDxApi::isRequest(request)
        && BetterBlurDxApi::decodeProvider(request)
        && m_wobblyCompositePass.shader;
}

bool BlurEffect::betterWobblyProviderPolicyAllowsBlur(const EffectWindow *w) const
{
    if (!w || w->isDesktop()) {
        return false;
    }

    // This is the effective Better Blur DX decision after native blur regions,
    // force-blur matching, menu/dock filters and hard exclusions are applied.
    if (!m_windowManager->windowIsBlurred(w)) {
        return false;
    }

    // Keep the same full-screen-effect policy as the normal rendering path.
    // A fallback must not reintroduce blur that Better Blur DX intentionally
    // disabled for the current compositor state.
    if (effects->activeFullScreenEffect() && !w->data(WindowForceBlurRole).toBool()) {
        return false;
    }

    return true;
}

bool BlurEffect::shouldBlur(const EffectWindow *w, int mask, const WindowPaintData &data) const
{
    const QVariant request = w->data(BetterBlurDxApi::RequestRole);
    if (BetterBlurDxApi::isRequest(request)) {
        // API v1 must obey the same effective blur policy as normal Better Blur
        // DX rendering. A window with no requested/forced blur region has been
        // excluded by matching/type policy, so do not allocate provider resources
        // for it and do not let the requester substitute its fallback blur.
        if (!betterWobblyProviderPolicyAllowsBlur(w)) {
            m_windowManager->setWindowIsTransformed(w, false);
            return false;
        }

        // Decline technically before allocating or updating a blur pyramid if
        // the callback or composite shader is unavailable; the requester may
        // run its standalone fallback for this kind of failure.
        if (!canHandleBetterWobblyProviderRequest(w)) {
            m_windowManager->setWindowIsTransformed(w, false);
            return false;
        }
        m_windowManager->setWindowIsTransformed(w, true);
        return true;
    }

    if (upstreamBetterBlurDxLoaded()) {
        // The original plugin owns stationary/ordinary blur while both effects
        // are enabled. This fork remains available only for explicit API frames.
        m_windowManager->setWindowIsTransformed(w, false);
        return false;
    }

    // Standalone mode: preserve Better Blur DX's complete original behaviour.
    if (effects->activeFullScreenEffect() && !w->data(WindowForceBlurRole).toBool()) {
        return false;
    }

    if (w->isDesktop()) {
        return false;
    }

    const bool scaled = !qFuzzyCompare(data.xScale(), 1.0) && !qFuzzyCompare(data.yScale(), 1.0);
    const bool translated = data.xTranslation() || data.yTranslation();

    if ((scaled || translated || (mask & PAINT_WINDOW_TRANSFORMED))
        && !w->data(WindowForceBlurRole).toBool()) {
        m_windowManager->setWindowIsTransformed(w, true);
        return m_windowManager->windowShouldBlurWhileTransformed(w);
    }

    m_windowManager->setWindowIsTransformed(w, false);
    return true;
}

void BlurEffect::drawWindow(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data)
{
    clearBetterWobblyProviderResult(w);
    const QVariant request = w->data(BetterBlurDxApi::RequestRole);
    if (BetterBlurDxApi::isRequest(request)) {
        // Distinguish an intentional policy rejection from a technical provider
        // failure. The former must suppress Wobbly's fallback; the latter may use
        // it. A successful provider composite overwrites either value with Rendered.
        const auto initialResult = betterWobblyProviderPolicyAllowsBlur(w)
            ? BetterBlurDxApi::ProviderResult::Declined
            : BetterBlurDxApi::ProviderResult::DeclinedByPolicy;
        w->setData(BetterBlurDxApi::ResultRole,
                   BetterBlurDxApi::encodeProviderResult(initialResult));
    }
    blur(renderTarget, viewport, w, mask, deviceRegion, data);

    // Draw the window over the blurred area. ProviderResult remains visible
    // only through this nested chain, where Better Wobbly can skip its fallback.
    effects->drawWindow(renderTarget, viewport, w, mask, deviceRegion, data);
    clearBetterWobblyProviderResult(w);
}

GLTexture *BlurEffect::ensureNoiseTexture()
{
    if (m_noiseStrength == 0) {
        return nullptr;
    }

    const qreal scale = std::max(1.0, QGuiApplication::primaryScreen()->logicalDotsPerInch() / 96.0);
    if (!m_noisePass.noiseTexture || m_noisePass.noiseTextureScale != scale || m_noisePass.noiseTextureStength != m_noiseStrength) {
        // Init randomness based on time
        std::srand((uint)QTime::currentTime().msec());

        QImage noiseImage(QSize(256, 256), QImage::Format_Grayscale8);

        for (int y = 0; y < noiseImage.height(); y++) {
            uint8_t *noiseImageLine = (uint8_t *)noiseImage.scanLine(y);

            for (int x = 0; x < noiseImage.width(); x++) {
                noiseImageLine[x] = std::rand() % m_noiseStrength;
            }
        }

        noiseImage = noiseImage.scaled(noiseImage.size() * scale);

        m_noisePass.noiseTexture = GLTexture::upload(noiseImage);
        if (!m_noisePass.noiseTexture) {
            return nullptr;
        }
        m_noisePass.noiseTexture->setFilter(GL_NEAREST);
        m_noisePass.noiseTexture->setWrapMode(GL_REPEAT);
        m_noisePass.noiseTextureScale = scale;
        m_noisePass.noiseTextureStength = m_noiseStrength;
    }

    return m_noisePass.noiseTexture.get();
}

bool BlurEffect::drawBetterWobblyMesh(const RenderTarget &renderTarget,
                                            const RenderViewport &viewport,
                                            EffectWindow *w,
                                            const Region &deviceRegion,
                                            WindowPaintData &data,
                                            BlurRenderData &renderInfo,
                                            const Rect &backgroundRect,
                                            float modulation)
{
    const BetterBlurDxApi::MeshProvider *provider = BetterBlurDxApi::decodeProvider(
        w->data(BetterBlurDxApi::RequestRole));
    if (!provider || !provider->buildMesh || !renderInfo.cache
        || !renderInfo.cache->cachedTexture() || !m_wobblyCompositePass.shader) {
        return false;
    }

    BetterBlurDxApi::MeshBuildInput input;
    input.effectWindow = reinterpret_cast<quintptr>(w);
    input.windowPaintData = reinterpret_cast<quintptr>(&data);
    input.renderScale = viewport.scale();
    input.cacheOriginX = backgroundRect.x();
    input.cacheOriginY = backgroundRect.y();
    input.textureWidth = renderInfo.cache->cachedTexture()->width();
    input.textureHeight = renderInfo.cache->cachedTexture()->height();

    BetterBlurDxApi::MeshView mesh;
    if (!provider->buildMesh(provider->context, &input, &mesh)
        || !mesh.vertices || mesh.vertexCount <= 0 || mesh.opacity <= 0.0f) {
        return false;
    }

    m_wobblyCompositeVertices.clear();
    m_wobblyCompositeVertices.reserve(size_t(mesh.vertexCount));
    for (int i = 0; i < mesh.vertexCount; ++i) {
        const BetterBlurDxApi::MeshVertex &source = mesh.vertices[i];
        m_wobblyCompositeVertices.push_back(GLVertex2D{
            .position = QVector2D(source.x, source.y),
            .texcoord = QVector2D(source.cacheX, source.cacheY),
        });
    }

    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
    vbo->reset();
    vbo->setAttribLayout(std::span(GLVertexBuffer::GLVertex2DLayout), sizeof(GLVertex2D));
    vbo->setData(m_wobblyCompositeVertices.data(),
                 m_wobblyCompositeVertices.size() * sizeof(GLVertex2D));
    vbo->setVertexCount(m_wobblyCompositeVertices.size());
    vbo->bindArrays();

    ShaderManager::instance()->pushShader(m_wobblyCompositePass.shader.get());
    m_wobblyCompositePass.shader->setUniform(m_wobblyCompositePass.mvpMatrixLocation,
                                              viewport.projectionMatrix());
    m_wobblyCompositePass.shader->setUniform(m_wobblyCompositePass.textureSizeLocation,
                                              QVector2D(input.textureWidth, input.textureHeight));
    m_wobblyCompositePass.shader->setUniform(m_wobblyCompositePass.opacityLocation,
                                              modulation * std::clamp(mesh.opacity, 0.0f, 1.0f));

    // The provider composite bypasses BlurCache::drawCached(), so its rounded
    // corner pass must be applied in the mesh's undeformed cache coordinates.
    // This keeps the mask attached to the window while the output vertices wobble.
    const BorderRadius cornerRadius = m_windowManager->getEffectiveBorderRadius(w);
    const RectF transformedRect = RectF{
        w->frameGeometry().x() + data.xTranslation(),
        w->frameGeometry().y() + data.yTranslation(),
        w->frameGeometry().width() * data.xScale(),
        w->frameGeometry().height() * data.yScale(),
    };
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    const RectF box{snapToPixelGridF(transformedRect).translated(-backgroundRect.topLeft())};
#else
    const RectF box{transformedRect.rounded().translated(-backgroundRect.topLeft())};
#endif
    m_wobblyCompositePass.shader->setUniform(m_wobblyCompositePass.boxLocation,
        QVector4D(box.horizontalCenter(), box.verticalCenter(), box.width() * 0.5, box.height() * 0.5));
    m_wobblyCompositePass.shader->setUniform(m_wobblyCompositePass.cornerRadiusLocation,
        cornerRadius.rounded().toVector());
    m_wobblyCompositePass.shader->setUniform(m_wobblyCompositePass.roundedMaskEnabledLocation,
        cornerRadius.isNull() ? 0.0f : 1.0f);

    renderInfo.cache->cachedTexture()->bind();
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
    if (deviceRegion != Region::infinite()) {
        const Region clipRegion = viewport.transform().map(deviceRegion, renderTarget.transformedSize());
        glEnable(GL_SCISSOR_TEST);
        vbo->draw(clipRegion, GL_TRIANGLES, 0, mesh.vertexCount, true);
        glDisable(GL_SCISSOR_TEST);
    } else {
        vbo->draw(GL_TRIANGLES, 0, mesh.vertexCount);
    }
    glDisable(GL_BLEND);

    ShaderManager::instance()->popShader();
    vbo->unbindArrays();
    m_blurCache->finishExternalCache(renderInfo);
    return true;
}

void BlurEffect::blur(const RenderTarget &renderTarget, const RenderViewport &viewport, EffectWindow *w, int mask, const Region &deviceRegion, WindowPaintData &data)
{
    auto it = m_windows.find(w);
    if (it == m_windows.end()) {
        return;
    }

    BlurEffectData &blurInfo = it->second;
    BlurRenderData &renderInfo = blurInfo.render[m_currentView];

    if (!shouldBlur(w, mask, data)) {
        return;
    }

    // BBDX: only blur top level for performance reasons
    // TODO: this currently looks really bad e.g. when maximizing
    /*
    if (!m_windowManager->windowHasTopLevelBlur(w)) {
        return;
    }
    */

    // Compute the effective blur shape. Note that if the window is transformed, so will be the blur shape.
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    Region blurShape = blurRegion(w).translated(w->pos().toPoint());
    if (data.xScale() != 1 || data.yScale() != 1) {
        QPoint pt = blurShape.boundingRect().topLeft();
        Region scaledShape;
        for (const Rect &r : blurShape.rects()) {
            const QPointF topLeft(pt.x() + (r.x() - pt.x()) * data.xScale() + data.xTranslation(),
                                  pt.y() + (r.y() - pt.y()) * data.yScale() + data.yTranslation());
            const QPoint bottomRight(std::floor(topLeft.x() + r.width() * data.xScale()) - 1,
                                     std::floor(topLeft.y() + r.height() * data.yScale()) - 1);
            scaledShape += QRect(QPoint(std::floor(topLeft.x()), std::floor(topLeft.y())), bottomRight);
        }
        blurShape = scaledShape;
    } else if (data.xTranslation() || data.yTranslation()) {
        blurShape.translate(std::round(data.xTranslation()), std::round(data.yTranslation()));
    }
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 5, 80)
    const QRect backgroundRect = blurShape.boundingRect();
    const QRect scaledBackgroundRect = snapToPixelGrid(scaledRect(backgroundRect, viewport.scale()));
    const QRect deviceBackgroundRect = scaledBackgroundRect;
#else
    const Rect backgroundRect = blurShape.boundingRect();
    const Rect scaledBackgroundRect = backgroundRect.scaled(viewport.scale()).rounded();
    const Rect deviceBackgroundRect = viewport.mapToDeviceCoordinates(backgroundRect).rounded();
#endif
#else
    RegionF blurShape = blurRegion(w);
    if (data.xScale() != 1 || data.yScale() != 1) {
        blurShape.scale(data.xScale(), data.yScale());
    }
    if (data.xTranslation() || data.yTranslation()) {
        blurShape.translate(data.xTranslation(), data.yTranslation());
    }

    blurShape.translate(w->pos());

    const Rect backgroundRect = blurShape.boundingRect().rounded();
    const Rect scaledBackgroundRect = backgroundRect.scaled(viewport.scale()).rounded();
    const Rect deviceBackgroundRect = viewport.mapToDeviceCoordinates(backgroundRect).rounded();
#endif
    const bool providerOwnedWobblyFrame = BetterBlurDxApi::isRequest(
        w->data(BetterBlurDxApi::RequestRole));
    // Better Blur DX normally fades blur to zero over 250 ms while a window is
    // transformed because the stock rectangular composite cannot follow a
    // Wobbly mesh. API v1 supplies that mesh, so applying that fade here makes
    // a successfully rendered provider frame fully transparent during a drag.
    const auto opacity = m_windowManager->getEffectiveBlurOpacity(
        w, data, providerOwnedWobblyFrame);

    // Get the effective shape that will be actually blurred. It's possible that all of it will be clipped.
    QList<RectF> effectiveShape;
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 5, 80)
    effectiveShape.reserve(blurShape.rectCount());
    if (deviceRegion != infiniteRegion()) {
        for (const QRect &clipRect : deviceRegion) {
            const QRectF deviceClipRect = snapToPixelGridF(scaledRect(clipRect, viewport.scale()))
                                              .translated(-deviceBackgroundRect.topLeft());
            for (const QRect &shapeRect : blurShape) {
                const QRectF deviceShapeRect = snapToPixelGridF(scaledRect(shapeRect.translated(-backgroundRect.topLeft()), viewport.scale()));
                if (const QRectF intersected = deviceClipRect.intersected(deviceShapeRect); !intersected.isEmpty()) {
                    effectiveShape.append(intersected);
                }
            }
        }
    } else {
        for (const QRect &rect : blurShape) {
            effectiveShape.append(snapToPixelGridF(scaledRect(rect.translated(-backgroundRect.topLeft()), viewport.scale())));
        }
    }
#else
    effectiveShape.reserve(blurShape.rects().size());
    if (deviceRegion != Region::infinite()) {
        for (const Rect &clipRect : deviceRegion.rects()) {
            const RectF deviceClipRect = clipRect.translated(-deviceBackgroundRect.topLeft());
            for (const auto &shapeRect : blurShape.rects()) {
                const RectF deviceShapeRect = shapeRect.translated(-backgroundRect.topLeft()).scaled(viewport.scale()).rounded();
                if (const RectF intersected = deviceClipRect.intersected(deviceShapeRect); !intersected.isEmpty()) {
                    effectiveShape.append(intersected);
                }
            }
        }
    } else {
        for (const auto &rect : blurShape.rects()) {
            effectiveShape.append(rect.translated(-backgroundRect.topLeft()).scaled(viewport.scale()).rounded());
        }
    }
#endif
    if (effectiveShape.isEmpty()) {
        return;
    }

    // Maybe reallocate offscreen render targets. Keep in mind that the first one contains
    // original background behind the window, it's not blurred.
    GLenum textureFormat = GL_RGBA8;
    if (renderTarget.texture()) {
        textureFormat = renderTarget.texture()->internalFormat();
    }

    if (renderInfo.framebuffers.size() != (m_iterationCount + 1) || renderInfo.textures[0]->size() != backgroundRect.size() || renderInfo.textures[0]->internalFormat() != textureFormat) {
        renderInfo.framebuffers.clear();
        renderInfo.textures.clear();
        // BBDX:
        if (renderInfo.cache) {
            renderInfo.cache->invalidate(static_cast<uint>(BlurCacheInvalidationFlag::FULL), "New framebuffers required");
        }

        // BBDX: alpha 1.0
        // unknown (yet-to-be-blitted) pixels should be black
        // instead of transparent to avoid artifacts when dragging
        // windows across screen borders
        glClearColor(0.0, 0.0, 0.0, 1.0);
        for (size_t i = 0; i <= m_iterationCount; ++i) {
            auto texture = GLTexture::allocate(textureFormat, BBDX::getTextureSize(backgroundRect, i));
            if (!texture) {
                qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to allocate an offscreen texture";
                return;
            }
            texture->setFilter(GL_LINEAR);
            texture->setWrapMode(GL_CLAMP_TO_EDGE);

            auto framebuffer = std::make_unique<GLFramebuffer>(texture.get());
            if (!framebuffer->valid()) {
                qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to create an offscreen framebuffer";
                return;
            }
            GLFramebuffer::pushFramebuffer(framebuffer.get());
            glClear(GL_COLOR_BUFFER_BIT);
            GLFramebuffer::popFramebuffer();
            renderInfo.textures.push_back(std::move(texture));
            renderInfo.framebuffers.push_back(std::move(framebuffer));
        }
    }

    // Fetch the pixels behind the shape that is going to be blurred.
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 5, 80)
    const QRegion dirtyRegion = deviceRegion & backgroundRect;
#else
    const Region dirtyRegion = viewport.mapFromDeviceCoordinatesContained(deviceRegion) & backgroundRect;
#endif
#if BBDX_NOT_NEEDED
    for (const Rect &dirtyRect : dirtyRegion.rects()) {
        renderInfo.framebuffers[0]->blitFromRenderTarget(renderTarget, viewport, dirtyRect, dirtyRect.translated(-backgroundRect.topLeft()));
    }
#else
    // BBDX: prepare cache, bail if there is no cache entry
    m_blurCache->preparePaintData(&renderTarget,
                                  &viewport,
                                  m_currentView,
                                  &data,
                                  w,
                                  &dirtyRegion,
                                  renderInfo.framebuffers[0].get(),
                                  &backgroundRect,
                                  &scaledBackgroundRect,
                                  renderInfo.cache);

    if (!renderInfo.cache.get()) {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Bailing due to missing cache entry";
        return;
    }
    if (BetterBlurDxApi::requireFreshCache(w->data(BetterBlurDxApi::RequestRole))) {
        renderInfo.cache->flush("inter-effect API requested a fresh frame");
    }
#endif

    // Upload the geometry: the first 6 vertices are used when downsampling and upsampling offscreen,
    // the remaining vertices are used when rendering on the screen.
    GLVertexBuffer *vbo = GLVertexBuffer::streamingBuffer();
    vbo->reset();
    vbo->setAttribLayout(std::span(GLVertexBuffer::GLVertex2DLayout), sizeof(GLVertex2D));

    const int vertexCount = effectiveShape.size() * 6;
    if (auto result = vbo->map<GLVertex2D>(6 + vertexCount)) {
        auto map = *result;

        size_t vboIndex = 0;

        // The geometry that will be blurred offscreen, in logical pixels.
        {
            const RectF localRect = RectF(0, 0, backgroundRect.width(), backgroundRect.height());

            const float x0 = localRect.left();
            const float y0 = localRect.top();
            const float x1 = localRect.right();
            const float y1 = localRect.bottom();

            const float u0 = x0 / backgroundRect.width();
            const float v0 = 1.0f - y0 / backgroundRect.height();
            const float u1 = x1 / backgroundRect.width();
            const float v1 = 1.0f - y1 / backgroundRect.height();

            // first triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y1),
                .texcoord = QVector2D(u0, v1),
            };

            // second triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y0),
                .texcoord = QVector2D(u1, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
        }

        // The geometry that will be painted on screen, in device pixels.
        for (const RectF &rect : effectiveShape) {
            const float x0 = rect.left();
            const float y0 = rect.top();
            const float x1 = rect.right();
            const float y1 = rect.bottom();

            const float u0 = x0 / scaledBackgroundRect.width();
            const float v0 = 1.0f - y0 / scaledBackgroundRect.height();
            const float u1 = x1 / scaledBackgroundRect.width();
            const float v1 = 1.0f - y1 / scaledBackgroundRect.height();

            // first triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y1),
                .texcoord = QVector2D(u0, v1),
            };

            // second triangle
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x0, y0),
                .texcoord = QVector2D(u0, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y0),
                .texcoord = QVector2D(u1, v0),
            };
            map[vboIndex++] = GLVertex2D{
                .position = QVector2D(x1, y1),
                .texcoord = QVector2D(u1, v1),
            };
        }

        vbo->unmap();
    } else {
        qCWarning(KWIN_BLUR) << BBDX::LOG_PREFIX << "Failed to map vertex buffer";
        return;
    }

    vbo->bindArrays();

    // BBDX: Check if we should bypass cache for mouse highlight responsiveness
    const QPointF currentMousePos = effects->cursorPos();
    const bool mouseMoved = (currentMousePos - m_lastMousePosition).manhattanLength() > 0;
    m_lastMousePosition = currentMousePos;
    
    auto config = BBDX::BlurConfig::self();
    const bool noCacheForMouse = config && config->borderHighlightNoCache();
    const bool mouseHighlightActive = m_refractionPass && 
                                       m_refractionPass->borderHighlightEnabled() && 
                                       m_refractionPass->borderHighlightMouseEnabled();
    
    if (noCacheForMouse && mouseHighlightActive && mouseMoved) {
        // Force cache flush by flushing the cache entry
        renderInfo.cache->flush("Mouse highlight: cache bypassed for responsiveness");
    }

    // BBDX: rate limited
    if (!renderInfo.cache->isFlushing()) {
        const float modulation = opacity * opacity;
        const QVariant request = w->data(BetterBlurDxApi::RequestRole);
        if (BetterBlurDxApi::suppressDefaultComposite(request)) {
            const bool rendered = BetterBlurDxApi::decodeProvider(request)
                && drawBetterWobblyMesh(renderTarget,
                                        viewport,
                                        w,
                                        deviceRegion,
                                        data,
                                        renderInfo,
                                        backgroundRect,
                                        modulation);
            w->setData(BetterBlurDxApi::ResultRole,
                       BetterBlurDxApi::encodeProviderResult(rendered
                           ? BetterBlurDxApi::ProviderResult::Rendered
                           : BetterBlurDxApi::ProviderResult::Declined));
        } else {
            m_blurCache->drawCached(viewport, renderInfo, vbo, vertexCount, modulation);
        }
        vbo->unbindArrays();
        return;
    }

    // The downsample pass of the dual Kawase algorithm: the background will be scaled down 50% every iteration.
    {
        ShaderManager::instance()->pushShader(m_downsamplePass.shader.get());

        QMatrix4x4 projectionMatrix;
        projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

        m_downsamplePass.shader->setUniform(m_downsamplePass.mvpMatrixLocation, projectionMatrix);
        m_downsamplePass.shader->setUniform(m_downsamplePass.offsetLocation, float(m_offset));

        for (size_t i = 1; i < renderInfo.framebuffers.size(); ++i) {
            // BBDX: read the blit from cache entry
            const GLFramebuffer *read;
            read = renderInfo.framebuffers[i - 1].get();
            const auto &draw = renderInfo.framebuffers[i];

            const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                                      0.5 / read->colorAttachment()->height());
            m_downsamplePass.shader->setUniform(m_downsamplePass.halfpixelLocation, halfpixel);

            read->colorAttachment()->bind();

            GLFramebuffer::pushFramebuffer(draw.get());
            BBDX::setGLScissor(dirtyRegion, backgroundRect);
            vbo->draw(GL_TRIANGLES, 0, 6);
        }

        ShaderManager::instance()->popShader();
    }

    // The upsample pass of the dual Kawase algorithm: the background will be scaled up 200% every iteration.
    {
        ShaderManager::instance()->pushShader(m_upsamplePass.shader.get());

        QMatrix4x4 projectionMatrix;
        projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

        m_upsamplePass.shader->setUniform(m_upsamplePass.mvpMatrixLocation, projectionMatrix);
        m_upsamplePass.shader->setUniform(m_upsamplePass.offsetLocation, float(m_offset));

        for (size_t i = renderInfo.framebuffers.size() - 1; i > 1; --i) {
            GLFramebuffer::popFramebuffer();
            const auto &read = renderInfo.framebuffers[i];

            const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                                      0.5 / read->colorAttachment()->height());
            m_upsamplePass.shader->setUniform(m_upsamplePass.halfpixelLocation, halfpixel);

            read->colorAttachment()->bind();

            BBDX::setGLScissor(dirtyRegion, backgroundRect);
            vbo->draw(GL_TRIANGLES, 0, 6);
        }

        ShaderManager::instance()->popShader();
    }

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    const QMatrix4x4 &colorMatrix = blurInfo.colorMatrix ? *blurInfo.colorMatrix : m_colorMatrix;
#else
    const QMatrix4x4 &colorMatrix = m_colorMatrix;
#endif
    const float modulation = opacity * opacity;

#if BBDX_NOT_NEEDED
    if (const BorderRadius cornerRadius = w->window()->borderRadius(); !cornerRadius.isNull()) {
        ShaderManager::instance()->pushShader(m_roundedOnscreenPass.shader.get());

        QMatrix4x4 projectionMatrix = viewport.projectionMatrix();
        projectionMatrix.translate(scaledBackgroundRect.x(), scaledBackgroundRect.y());

        GLFramebuffer::popFramebuffer();
        const auto &read = renderInfo.framebuffers[1];

        const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                                  0.5 / read->colorAttachment()->height());

        const RectF transformedRect = RectF{
            w->frameGeometry().x() + data.xTranslation(),
            w->frameGeometry().y() + data.yTranslation(),
            w->frameGeometry().width() * data.xScale(),
            w->frameGeometry().height() * data.yScale(),
        };
        const RectF nativeBox = transformedRect
                                    .scaled(viewport.scale())
                                    .rounded()
                                    .translated(-scaledBackgroundRect.topLeft());
        const BorderRadius nativeCornerRadius = cornerRadius.scaled(viewport.scale()).rounded();

        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.mvpMatrixLocation, projectionMatrix);
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.colorMatrixLocation, colorMatrix);
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.halfpixelLocation, halfpixel);
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.offsetLocation, float(m_offset));
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.boxLocation, QVector4D(nativeBox.horizontalCenter(), nativeBox.verticalCenter(), nativeBox.width() * 0.5, nativeBox.height() * 0.5));
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.cornerRadiusLocation, nativeCornerRadius.toVector());
        m_roundedOnscreenPass.shader->setUniform(m_roundedOnscreenPass.opacityLocation, modulation);

        read->colorAttachment()->bind();

        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        vbo->draw(GL_TRIANGLES, 6, vertexCount);

        glDisable(GL_BLEND);

        ShaderManager::instance()->popShader();
    } else {
#endif
        if (!m_refractionPass->pushShader()) {
        ShaderManager::instance()->pushShader(m_onscreenPass.shader.get());
        } // indent intentional for KWin diff

        // BBDX: MVP matrix maps to backgroundRect for BlurCache
        QMatrix4x4 projectionMatrix;
        projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

        GLFramebuffer::popFramebuffer();
        const auto &read = renderInfo.framebuffers[1];

        const QVector2D halfpixel(0.5 / read->colorAttachment()->width(),
                                  0.5 / read->colorAttachment()->height());

        // Update mouse position for mouse highlight effect
        if (m_refractionPass) {
            const QPointF cursorPos = effects->cursorPos();
            // Convert cursor position to refraction rectangle space
            // backgroundRect is the area being refracted in screen coordinates
            QVector2D mousePosInRefraction(
                static_cast<float>(cursorPos.x() - backgroundRect.x()),
                static_cast<float>(cursorPos.y() - backgroundRect.y())
            );
            // Normalize to 0-1 range
            // Flip Y: screen coords have Y down, but we want mouse at top to affect top of window
            if (backgroundRect.width() > 0 && backgroundRect.height() > 0) {
                mousePosInRefraction.setX(mousePosInRefraction.x() / backgroundRect.width());
                mousePosInRefraction.setY(1.0f - mousePosInRefraction.y() / backgroundRect.height());
            }
            m_refractionPass->setMousePosition(mousePosInRefraction);
        }

        if (!m_refractionPass->setParameters(projectionMatrix,
                                             colorMatrix,
                                             halfpixel,
                                             float(m_offset),
                                             backgroundRect)) {
        m_onscreenPass.shader->setUniform(m_onscreenPass.mvpMatrixLocation, projectionMatrix);
        m_onscreenPass.shader->setUniform(m_onscreenPass.colorMatrixLocation, colorMatrix);
        m_onscreenPass.shader->setUniform(m_onscreenPass.halfpixelLocation, halfpixel);
        m_onscreenPass.shader->setUniform(m_onscreenPass.offsetLocation, float(m_offset));
        } // indent intentional for KWin diff

        read->colorAttachment()->bind();

#if BBDX_NOT_NEEDED
        if (modulation < 1.0) {
            glEnable(GL_BLEND);
            glBlendColor(0, 0, 0, modulation);
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE_MINUS_CONSTANT_ALPHA);
        }
#endif

        // BBDX:
        m_blurCache->drawToCache(renderInfo.cache.get(), vbo);

#if BBDX_NOT_NEEDED
        if (modulation < 1.0) {
            glDisable(GL_BLEND);
        }
#endif

        ShaderManager::instance()->popShader();
#if BBDX_NOT_NEEDED
    }
#endif

    if (m_noiseStrength > 0) {
        // Apply an additive noise onto the blurred image. The noise is useful to mask banding
        // artifacts, which often happens due to the smooth color transitions in the blurred image.

        glEnable(GL_BLEND);
#if BBDX_NOT_NEEDED
        if (opacity < 1.0) {
            glBlendFunc(GL_CONSTANT_ALPHA, GL_ONE);
        } else {
#endif
            glBlendFunc(GL_ONE, GL_ONE);
#if BBDX_NOT_NEEDED
        }
#endif

        if (GLTexture *noiseTexture = ensureNoiseTexture()) {
            ShaderManager::instance()->pushShader(m_noisePass.shader.get());

            // BBDX: MVP matrix maps to backgroundRect for BlurCache
            QMatrix4x4 projectionMatrix;
            projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

            m_noisePass.shader->setUniform(m_noisePass.mvpMatrixLocation, projectionMatrix);
            m_noisePass.shader->setUniform(m_noisePass.noiseTextureSizeLocation, QVector2D(noiseTexture->width(), noiseTexture->height()));

            noiseTexture->bind();

            // BBDX:
            m_blurCache->drawToCache(renderInfo.cache.get(), vbo);

            ShaderManager::instance()->popShader();
        }

        glDisable(GL_BLEND);
    }

    // BBDX:
    BBDX::clearGLScissor();
    const QVariant request = w->data(BetterBlurDxApi::RequestRole);
    if (BetterBlurDxApi::suppressDefaultComposite(request)) {
        const bool rendered = BetterBlurDxApi::decodeProvider(request)
            && drawBetterWobblyMesh(renderTarget,
                                    viewport,
                                    w,
                                    deviceRegion,
                                    data,
                                    renderInfo,
                                    backgroundRect,
                                    modulation);
        if (!rendered) {
            // The freshly generated cache is complete even when the provider
            // declines this frame. Mark it finished so fallback does not cause
            // Better Blur DX to recompute it indefinitely in parallel.
            m_blurCache->finishExternalCache(renderInfo);
        }
        w->setData(BetterBlurDxApi::ResultRole,
                   BetterBlurDxApi::encodeProviderResult(rendered
                       ? BetterBlurDxApi::ProviderResult::Rendered
                       : BetterBlurDxApi::ProviderResult::Declined));
    } else {
        m_blurCache->drawCached(viewport, renderInfo, vbo, vertexCount, modulation);
    }

    vbo->unbindArrays();
}

bool BlurEffect::isActive() const
{
    return m_valid && !effects->isScreenLocked();
}

bool BlurEffect::blocksDirectScanout() const
{
    return false;
}

} // namespace BBDX

#include "moc_blur.cpp"
