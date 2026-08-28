#include "blurconfig.h"
#include "refraction_pass.hpp"
#include "utils.h"

#include <memory>
#include <opengl/glshader.h>
#include <opengl/glshadermanager.h>

#include <QLoggingCategory>
#include <QRect>

#include <algorithm>

Q_LOGGING_CATEGORY(REFRACTION_PASS, "kwin_effect_better_blur_dx.refraction_pass", QtInfoMsg)

std::unique_ptr<BBDX::RefractionPass> BBDX::RefractionPass::create() {
    // The vertex shaders should always be the one of the
    // respective contrast pass.
    // The refraction uses a modified version of
    // the contrast fragment shader.

    std::unique_ptr<RefractionPass> pass{new RefractionPass};

    pass->m_shader = KWin::ShaderManager::instance()->generateShaderFromFile(
        KWin::ShaderTrait::MapTexture,
        BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/vertex.vert"),
        BBDX::shaderFilePath(":/effects/better_blur_dx_wobbly_api/shaders/refraction.frag")
    );

    if (!pass->m_shader) {
        qCWarning(REFRACTION_PASS) << BBDX::LOG_PREFIX << "Failed to load refraction pass shader";
        return nullptr;
    } else {
        // contrast parameters
        pass->m_mvpMatrixLocation = pass->m_shader->uniformLocation("modelViewProjectionMatrix");
        pass->m_colorMatrixLocation = pass->m_shader->uniformLocation("colorMatrix");
        pass->m_offsetLocation = pass->m_shader->uniformLocation("offset");
        pass->m_halfpixelLocation = pass->m_shader->uniformLocation("halfpixel");
        // refraction parameters
        pass->m_refractionRectSizeLocation = pass->m_shader->uniformLocation("refractionRectSize");
        pass->m_refractionEdgeSizePixelsLocation = pass->m_shader->uniformLocation("refractionEdgeSizePixels");
        pass->m_refractionCornerRadiusPixelsLocation = pass->m_shader->uniformLocation("refractionCornerRadiusPixels");
        pass->m_refractionStrengthLocation = pass->m_shader->uniformLocation("refractionStrength");
        pass->m_refractionNormalPowLocation = pass->m_shader->uniformLocation("refractionNormalPow");
        pass->m_refractionRGBFringingLocation = pass->m_shader->uniformLocation("refractionRGBFringing");
        pass->m_refractionTextureRepeatModeLocation = pass->m_shader->uniformLocation("refractionTextureRepeatMode");
        pass->m_refractionModeLocation = pass->m_shader->uniformLocation("refractionMode");
        // border highlight parameters
        pass->m_borderHighlightColorLocation = pass->m_shader->uniformLocation("borderHighlightColor");
        pass->m_borderHighlightWidthLocation = pass->m_shader->uniformLocation("borderHighlightWidth");
        pass->m_borderHighlightCornerRadiusLocation = pass->m_shader->uniformLocation("borderHighlightCornerRadius");
        pass->m_borderHighlightMouseLocation = pass->m_shader->uniformLocation("borderHighlightMouse");
        pass->m_borderHighlightMouseStrengthLocation = pass->m_shader->uniformLocation("borderHighlightMouseStrength");
    }

    return pass;
}

void BBDX::RefractionPass::reconfigure() {
    auto config = BBDX::BlurConfig::self();

    if (!config) {
        qCWarning(REFRACTION_PASS) << BBDX::LOG_PREFIX
                                   << "RefractionPass::reconfigure() called before BlurConfig::read()";
        return;
    }

    // mark enabled if strength > 0 OR if border highlight is enabled
    // (border highlight is integrated into refraction shader)
    m_enabled = (config->refractionStrength() > 0) || config->borderHighlightEnabled();

    // scaled up by 10.0
    constexpr double scaleEdgeSizePixels{10.0};
    m_edgeSizePixels = static_cast<double>(config->refractionEdgeSize()) * scaleEdgeSizePixels;

    // snapped to nearest step
    constexpr double maxCornerRadiusPixels{200.0};
    constexpr double steps{30.0};
    constexpr double stepSize{maxCornerRadiusPixels / steps}; // ~6.6667, matching step size from blur_config.ui
    const double snapped{std::round(static_cast<double>(config->refractionCornerRadius()) / stepSize) * stepSize};
    m_cornerRadiusPixels = snapped;

    // expects range 0.0-1.0
    // max value from blur_config.ui
    constexpr double maxStrength{30.0};
    m_strength = static_cast<double>(config->refractionStrength()) / maxStrength;

    // XXX: why scaled down?
    constexpr double scaleNormalPow{0.5};
    m_normalPow = static_cast<double>(config->refractionNormalPow()) * scaleNormalPow;

    // expects range 0.0-1.0
    // max value from blur_config.ui
    constexpr double maxRGBFringing{30.0};
    m_RGBFringing = static_cast<double>(config->refractionRGBFringing()) / maxRGBFringing;

    // integer mode selectors
    m_textureRepeatMode = config->refractionTextureRepeatMode();
    m_mode = config->refractionMode();

    // border highlight settings
    m_borderHighlightEnabled = config->borderHighlightEnabled();
    m_borderHighlightColor = config->borderHighlightColor();
    m_borderHighlightWidth = config->borderHighlightWidth();
    m_borderHighlightStrength = config->borderHighlightStrength();
    m_borderHighlightMouseEnabled = config->borderHighlightMouseEnabled();
    m_borderHighlightMouseStrength = config->borderHighlightMouseStrength();
    m_borderHighlightCornerRadius = config->borderHighlightCornerRadius();
}

bool BBDX::RefractionPass::pushShader() const {
    if (!enabled())
        return false;

    KWin::ShaderManager::instance()->pushShader(m_shader.get());

    return true;
}


bool BBDX::RefractionPass::setParameters(const QMatrix4x4 &projectionMatrix,
                                         const QMatrix4x4 &colorMatrix,
                                         const QVector2D &halfpixel,
                                         const float offset,
                                         const QRect &scaledBackgroundRect) const {
    if (!enabled())
        return false;

    // contrast parameters
    m_shader->setUniform(m_mvpMatrixLocation, projectionMatrix);
    m_shader->setUniform(m_colorMatrixLocation, colorMatrix);
    m_shader->setUniform(m_halfpixelLocation, halfpixel);
    m_shader->setUniform(m_offsetLocation, offset);
    // refraction parameters
    m_shader->setUniform(m_refractionRectSizeLocation,
                                     QVector2D(scaledBackgroundRect.width(), scaledBackgroundRect.height()));
    m_shader->setUniform(m_refractionEdgeSizePixelsLocation,
                                     std::min(static_cast<float>(m_edgeSizePixels),
                                              static_cast<float>(std::min(scaledBackgroundRect.width() / 2,
                                                                          scaledBackgroundRect.height() / 2))));
    m_shader->setUniform(m_refractionCornerRadiusPixelsLocation, static_cast<float>(m_cornerRadiusPixels));
    m_shader->setUniform(m_refractionStrengthLocation, static_cast<float>(m_strength));
    m_shader->setUniform(m_refractionNormalPowLocation, static_cast<float>(m_normalPow));
    m_shader->setUniform(m_refractionRGBFringingLocation, static_cast<float>(m_RGBFringing));
    m_shader->setUniform(m_refractionTextureRepeatModeLocation, m_textureRepeatMode);
    m_shader->setUniform(m_refractionModeLocation, m_mode);

    // border highlight parameters
    // Always set these uniforms so the shader has the correct values
    // (border highlight may be toggled on/off without restarting)
    
    // Calculate opacity from strength (0-100 -> 0.0-1.0)
    const float opacity = static_cast<float>(m_borderHighlightStrength) / 100.0f;
    const QVector4D highlightColor(
        static_cast<float>(m_borderHighlightColor.redF()),
        static_cast<float>(m_borderHighlightColor.greenF()),
        static_cast<float>(m_borderHighlightColor.blueF()),
        m_borderHighlightEnabled ? opacity : 0.0f  // Set alpha to 0 if disabled
    );
    
    if (m_borderHighlightColorLocation != -1) {
        m_shader->setUniform(m_borderHighlightColorLocation, highlightColor);
    }
    if (m_borderHighlightWidthLocation != -1) {
        m_shader->setUniform(m_borderHighlightWidthLocation, static_cast<float>(m_borderHighlightWidth));
    }
    if (m_borderHighlightCornerRadiusLocation != -1) {
        m_shader->setUniform(m_borderHighlightCornerRadiusLocation, static_cast<float>(m_borderHighlightCornerRadius));
    }
    if (m_borderHighlightMouseLocation != -1) {
        m_shader->setUniform(m_borderHighlightMouseLocation, m_mousePosition);
    }
    if (m_borderHighlightMouseStrengthLocation != -1) {
        m_shader->setUniform(m_borderHighlightMouseStrengthLocation, static_cast<float>(m_borderHighlightMouseEnabled ? m_borderHighlightMouseStrength : 0) / 100.0f);
    }

    return true;
}
