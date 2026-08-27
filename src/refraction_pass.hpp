#pragma once

#include <opengl/glshader.h>

#include <QColor>
#include <QMatrix4x4>
#include <QVector2D>
#include <QVector4D>
#include <QtNumeric>

#include <memory>

namespace BBDX {

class RefractionPass {
private:
    std::unique_ptr<KWin::GLShader> m_shader;
    // contrast parameters
    int m_mvpMatrixLocation;
    int m_colorMatrixLocation;
    int m_offsetLocation;
    int m_halfpixelLocation;
    // refraction parameters
    int m_refractionRectSizeLocation;
    int m_refractionEdgeSizePixelsLocation;
    int m_refractionCornerRadiusPixelsLocation;
    int m_refractionStrengthLocation;
    int m_refractionNormalPowLocation;
    int m_refractionRGBFringingLocation;
    int m_refractionTextureRepeatModeLocation;
    int m_refractionModeLocation;
    // border highlight parameters
    int m_borderHighlightColorLocation;
    int m_borderHighlightWidthLocation;
    int m_borderHighlightCornerRadiusLocation;
    int m_borderHighlightMouseLocation;
    int m_borderHighlightMouseStrengthLocation;

    bool m_enabled{false};

    // user settings
    qreal m_normalPow{};
    qreal m_strength{};
    qreal m_edgeSizePixels{};
    qreal m_cornerRadiusPixels{};
    qreal m_RGBFringing{};
    int m_textureRepeatMode{};
    int m_mode{};

    // border highlight settings
    QColor m_borderHighlightColor{Qt::white};
    int m_borderHighlightWidth{2};
    int m_borderHighlightStrength{50};
    bool m_borderHighlightEnabled{false};
    bool m_borderHighlightMouseEnabled{false};
    int m_borderHighlightMouseStrength{50};
    int m_borderHighlightCornerRadius{};
    QVector2D m_mousePosition{0.5, 0.5};

    RefractionPass() = default;

public:
    /**
     * Loads required shaders and sets up shader uniformLocations
     */
    static std::unique_ptr<RefractionPass> create();

    /**
     * reconfigure from BlurConfig
     */
    void reconfigure();

    /**
     * Check if refraction pass is enabled
     */
    bool enabled() const { return m_enabled; }

    /**
     * Check if border highlight is enabled within refraction
     */
    bool borderHighlightEnabled() const { return m_borderHighlightEnabled; }

    /**
     * Check if mouse highlight is enabled within border highlight
     */
    bool borderHighlightMouseEnabled() const { return m_borderHighlightMouseEnabled; }

    /**
     * Get border highlight settings
     */
    QColor borderHighlightColor() const { return m_borderHighlightColor; }
    int borderHighlightWidth() const { return m_borderHighlightWidth; }
    int borderHighlightStrength() const { return m_borderHighlightStrength; }
    int borderHighlightMouseStrength() const { return m_borderHighlightMouseStrength; }
    int borderHighlightCornerRadius() const { return m_borderHighlightCornerRadius; }

    /**
     * Set mouse position (normalized screen coordinates 0-1)
     */
    void setMousePosition(const QVector2D &pos) { m_mousePosition = pos; }

    /**
     * Push respective shader to the ShaderManager
     *
     * returns false if refraction is disabled
     */
    bool pushShader() const;

    /**
     * Set GLSL parameters
     *
     * returns false if refraction is disabled
     */
    bool setParameters(const QMatrix4x4 &projectionMatrix,
                       const QMatrix4x4 &colorMatrix,
                       const QVector2D &halfpixel,
                       const float offset,
                       const QRect &scaledBackgroundRect) const;
};

} // namespace BBDX

