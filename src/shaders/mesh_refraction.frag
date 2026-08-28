uniform sampler2D texUnit;
uniform mat4 colorMatrix;
uniform float offset;
uniform vec2 halfpixel;

// Border highlight uniforms
uniform vec4 borderHighlightColor;  // RGBA (alpha = strength)
uniform float borderHighlightWidth;  // Width in pixels
uniform vec2 borderHighlightMouse;  // Mouse position in screen coordinates (0-1)
uniform float borderHighlightMouseStrength;  // Mouse highlight strength (0-1)

// For proper texture sampling
uniform vec2 meshRectSize;

varying vec2 uv;

// Simple rectangle distance function with rounded corners
// Returns signed distance: negative inside, positive outside, zero at edge
float rectangleSDF(vec2 uv, float radius) {
    // Map UV from [0,1] to [-0.5, 0.5] centered at (0.5, 0.5)
    vec2 p = uv - vec2(0.5);
    
    // Box without rounding
    vec2 d = abs(p) - vec2(0.5);
    
    // Rounded box: distance to nearest edge accounting for rounded corners
    // radius is in UV space [0, 0.5]
    vec2 d_with_radius = d + vec2(radius);
    return min(max(d_with_radius.x, d_with_radius.y), 0.0) + length(max(d_with_radius, 0.0)) - radius;
}

void main()
{
    // Sample the blurred texture
    // uv is the mesh-relative texture coordinate [0,1]
    vec2 uvOffset = uv + vec2(offset / meshRectSize.x, offset / meshRectSize.y);
    vec4 color = texture2D(texUnit, uvOffset);
    
    // Apply color matrix
    gl_FragColor = color * colorMatrix;
    
    // === BORDER HIGHLIGHT (UV-based for mesh geometry) ===
    // For mesh rendering, the UV space [0,1] corresponds to the mesh geometry
    // Use rounded rectangle SDF to follow wobbled window shape
    
    // Convert border highlight width from pixels to UV space
    // Use average scale to maintain consistent width
    float uvScale = (meshRectSize.x + meshRectSize.y) * 0.5;
    float cornerRadiusUV = borderHighlightWidth / uvScale;
    
    float dist = rectangleSDF(uv, cornerRadiusUV);
    
    // Convert SDF distance to pixel distance
    float distPixels = abs(dist) * uvScale;
    
    // Normalize by border width and create falloff
    float normalizedDist = distPixels / max(borderHighlightWidth, 1.0);
    float edgeFalloff = 1.0 - smoothstep(0.0, 1.0, normalizedDist * 2.0);
    float intensity = edgeFalloff * borderHighlightColor.a;
    
    // === MOUSE HIGHLIGHT EFFECT ===
    // Mouse position and fragment position in mesh pixel space
    vec2 fragPos = uv * meshRectSize;
    vec2 mousePos = borderHighlightMouse * meshRectSize;
    
    float mouseDist = length(fragPos - mousePos);
    float maxDist = length(meshRectSize) * 0.5;
    float normalizedMouseDist = clamp(mouseDist / maxDist, 0.0, 1.0);
    
    // Mouse proximity: brighter near mouse cursor
    float mouseProximity = borderHighlightMouseStrength > 0.0 ?
        smoothstep(1.0, 0.0, normalizedMouseDist) : 0.0;
    
    // Combine base highlight with mouse effect
    float finalIntensity = intensity * (1.0 + mouseProximity * borderHighlightMouseStrength * 16.0);
    
    // Apply highlight
    vec4 highlight = vec4(borderHighlightColor.rgb, 1.0);
    gl_FragColor = mix(gl_FragColor, highlight, clamp(finalIntensity, 0.0, 0.95));
}
