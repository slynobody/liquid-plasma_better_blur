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
        pass->m_boxLocation = pass->m_shader->uniformLocation("box");
        pass->m_cornerRadiusLocation = pass->m_shader->uniformLocation("cornerRadius");
    }

    return pass;
}

void BBDX::RoundedCornersPass::apply(const WindowManager *windowManager,
                                     const KWin::Rect &backgroundRect,
                                     const KWin::EffectWindow *w,
                                     const KWin::WindowPaintData &data,
                                     KWin::GLVertexBuffer *vbo,
                                     const BBDX::BlurCache *blurCache,
                                     BBDX::BlurCacheEntry *cacheEntry) const {

        const auto cornerRadius = windowManager->getEffectiveBorderRadius(w);

        if (cornerRadius.isNull()) {
            return;
        }
        
        KWin::ShaderManager::instance()->pushShader(m_shader.get());

        // cache local coordinates
        QMatrix4x4 projectionMatrix;
        projectionMatrix.ortho(QRectF(0.0, 0.0, backgroundRect.width(), backgroundRect.height()));

        // read background from the blit
        const auto &read = blurCache->paintData().blitFramebuffer;

        const KWin::RectF transformedRect = KWin::RectF{
            w->frameGeometry().x() + data.xTranslation() / data.xScale(),
            w->frameGeometry().y() + data.yTranslation() / data.yScale(),
            w->frameGeometry().width(),
            w->frameGeometry().height(),
        };

#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
        const KWin::RectF box{KWin::snapToPixelGridF(transformedRect).translated(-backgroundRect.topLeft())};
#else
        const KWin::RectF box{transformedRect.rounded().translated(-backgroundRect.topLeft())};
#endif

        m_shader->setUniform(m_mvpMatrixLocation, projectionMatrix);
#if KWIN_VERSION < KWIN_VERSION_CODE(6, 6, 90)
        m_shader->setUniform(m_boxLocation, QVector4D(box.x() + box.width() * 0.5, box.y() + box.height() * 0.5, box.width() * 0.5, box.height() * 0.5));
#else
        m_shader->setUniform(m_boxLocation, QVector4D(box.horizontalCenter(), box.verticalCenter(), box.width() * 0.5, box.height() * 0.5));
#endif
        m_shader->setUniform(m_cornerRadiusLocation, cornerRadius.toVector());

        read->colorAttachment()->bind();

        /**
         * Don't actually write alpha because we
         * might have a texture without that channel
         */
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);

        /**
         * SRC.rgb are background blit pixels
         * DST.rgb are blurred pixels
         * SRC.a is 0.0 in the corners that should be clipped, 1.0 inside the window
         */
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE_MINUS_SRC_ALPHA, GL_SRC_ALPHA);

        blurCache->drawToCache(cacheEntry, vbo);

        glDisable(GL_BLEND);
        glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

        KWin::ShaderManager::instance()->popShader();
}
