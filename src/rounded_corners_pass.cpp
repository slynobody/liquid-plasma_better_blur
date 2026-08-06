#include "rounded_corners_pass.hpp"

#include "blur.h"
#include "kwin_compat.hpp"
#include "utils.h"

#if KWIN_VERSION >= KWIN_VERSION_CODE(6, 5, 80)
#  include <core/rect.h>
#  include <core/region.h>
#endif

#include <core/pixelgrid.h>
#include <core/renderviewport.h>
#include <effect/effect.h>
#include <effect/effectwindow.h>
#include <epoxy/gl_generated.h>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>
#include <scene/borderradius.h>

#include <QLoggingCategory>
#include <QRect>
#include <QImage>

#include <memory>

Q_LOGGING_CATEGORY(ROUNDED_CORNERS_PASS, "kwin_effect_better_blur_dx.rounded_corners_pass", QtInfoMsg)

std::unique_ptr<BBDX::RoundedCornersPass> BBDX::RoundedCornersPass::create() {
    std::unique_ptr<RoundedCornersPass> pass{new RoundedCornersPass};

    pass->m_shader = KWin::ShaderManager::instance()->generateShaderFromFile(
        KWin::ShaderTrait::MapTexture,
        BBDX::shaderFilePath(":/effects/better_blur_dx/shaders/rounded_corners.vert"),
        BBDX::shaderFilePath(":/effects/better_blur_dx/shaders/rounded_corners.frag")
    );

    if (!pass->m_shader) {
        qCWarning(ROUNDED_CORNERS_PASS) << BBDX::LOG_PREFIX << "Failed to load rounded corners pass shader";
        return nullptr;
    } else {
        pass->m_mvpMatrixLocation = pass->m_shader->uniformLocation("modelViewProjectionMatrix");
        pass->m_modulationLocation = pass->m_shader->uniformLocation("modulation");
        pass->m_boxLocation = pass->m_shader->uniformLocation("box");
        pass->m_cornerRadiusLocation = pass->m_shader->uniformLocation("cornerRadius");
    }

    return pass;
}

bool BBDX::RoundedCornersPass::drawRounded(const WindowManager *windowManager,
                                           const BBDX::BlurCache *blurCache,
                                           BBDX::BlurCacheEntry *cacheEntry,
                                           KWin::GLVertexBuffer *vbo,
                                           const int vertexCount,
                                           const float modulation) const {

    const auto w = blurCache->paintData().window;
    const auto cornerRadius = windowManager->getEffectiveBorderRadius(w);

    if (cornerRadius.isNull()) {
        return false;
    }

    const auto data = blurCache->paintData().windowPaintData;
    const auto scaledBackgroundRect = blurCache->paintData().scaledBackgroundRect;
    const auto viewport = blurCache->paintData().viewport;

    KWin::ShaderManager::instance()->pushShader(m_shader.get());

    // we're drawing on screen here
    QMatrix4x4 projectionMatrix = viewport->projectionMatrix();
    projectionMatrix.translate(scaledBackgroundRect->x(), scaledBackgroundRect->y());

    const KWin::RectF transformedRect = KWin::RectF{
        w->frameGeometry().x() + data->xTranslation(),
        w->frameGeometry().y() + data->yTranslation(),
        w->frameGeometry().width() * data->xScale(),
        w->frameGeometry().height() * data->yScale(),
    };

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    const KWin::RectF box{KWin::snapToPixelGridF(KWin::scaledRect(transformedRect, viewport->scale())).translated(-scaledBackgroundRect->topLeft())};
#else
    const KWin::RectF box{transformedRect.scaled(viewport->scale()).rounded().translated(-scaledBackgroundRect->topLeft())};
#endif

    m_shader->setUniform(m_mvpMatrixLocation, projectionMatrix);
    m_shader->setUniform(m_modulationLocation, modulation);
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
    m_shader->setUniform(m_boxLocation, QVector4D(box.x() + box.width() * 0.5, box.y() + box.height() * 0.5, box.width() * 0.5, box.height() * 0.5));
#else
    m_shader->setUniform(m_boxLocation, QVector4D(box.horizontalCenter(), box.verticalCenter(), box.width() * 0.5, box.height() * 0.5));
#endif
    m_shader->setUniform(m_cornerRadiusLocation, cornerRadius.scaled(viewport->scale()).rounded().toVector());

    cacheEntry->cachedTexture()->bind();

    /**
     * src.rgb is passed through
     * src.a is modulation * rounded-box-alpha
     */
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    vbo->draw(GL_TRIANGLES, blurCache->vboStartScreen(), vertexCount);

    glDisable(GL_BLEND);

    KWin::ShaderManager::instance()->popShader();

    return true;
}
