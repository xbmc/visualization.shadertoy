#version 150

uniform sampler2D uTexture;

in vec2 vTextureCoord;

out vec4 FragColor;

void main(void)
{
  FragColor = texture(uTexture, vTextureCoord);
}
