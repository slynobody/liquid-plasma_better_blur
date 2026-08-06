uniform sampler2D texUnit;

varying vec2 uv;

void main(void)
{
    gl_FragColor = texture2D(texUnit, uv);
}
