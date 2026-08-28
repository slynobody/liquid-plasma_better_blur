/*
    Provider-owned Better Wobbly mesh composite.
    The source coordinates refer to Better Blur DX's own final cache.
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#extension GL_OES_standard_derivatives : enable

#include "sdf.glsl"

uniform sampler2D texUnit;
uniform vec2 textureSize;
uniform float opacity;
uniform vec4 box;
uniform vec4 cornerRadius;
uniform float roundedMaskEnabled;

varying vec2 uv;

void main(void)
{
    vec2 cacheUv = vec2(uv.x / textureSize.x,
                        1.0 - uv.y / textureSize.y);
    vec2 halfTexel = vec2(0.5) / textureSize;
    cacheUv = clamp(cacheUv, halfTexel, vec2(1.0) - halfTexel);
    gl_FragColor = texture2D(texUnit, cacheUv) * opacity;

    if (roundedMaskEnabled > 0.5) {
        float f = sdfRoundedBox(uv, box.xy, box.zw, cornerRadius);
        float df = max(fwidth(f), 0.0001);
        float alpha = clamp(0.5 - f / df, 0.0, 1.0);
        gl_FragColor *= alpha;
    }
}
