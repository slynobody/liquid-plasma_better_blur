#version 140

uniform sampler2D texUnit;

in vec2 uv;

out vec4 fragColor;

void main(void)
{
    fragColor = texture2D(texUnit, uv);
}
