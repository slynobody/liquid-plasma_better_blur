#pragma once

#include "blur_cache.hpp"
#include "kwin_compat.hpp"
#include "window_manager.hpp"

#include <opengl/glshader.h>

#include <QMatrix4x4>
#include <QVector2D>
#include <QVector4D>
#include <QtNumeric>

#include <opengl/glvertexbuffer.h>

#include <memory>

namespace KWin {
    class BorderRadius;
    class EffectWindow;
    class GLVertexBuffer;
    class RenderViewport;
    class WindowPaintData;
}

namespace BBDX {
    class BlurCache;
    class BlurCacheEntry;
}

namespace BBDX {
struct BlurRenderData;

class RoundedCornersPass {
private:
    std::unique_ptr<KWin::GLShader> m_shader{nullptr};
    int m_mvpMatrixLocation;
    int m_modulationLocation;
    int m_boxLocation;
    int m_cornerRadiusLocation;

    RoundedCornersPass() = default;

public:
    /**
     * Loads required shaders and sets up shader uniformLocations
     * nullptr on error
     */
    static std::unique_ptr<RoundedCornersPass> create();

    /**
     * Draw the cached texture with rounded corners to the screen
     *
     * returns true if drawn, else false (meaning it should be drawn normally)
     */
    bool drawRounded(const BBDX::WindowManager *windowManager,
                     const BBDX::BlurCache *blurCache,
                     BBDX::BlurCacheEntry *cacheEntry,
                     KWin::GLVertexBuffer *vbo,
                     const int vertexCount,
                     const float modulation) const;
};

} // namespace BBDX

