#version 150

uniform vec2 uScale;

in vec4 vertex;
out vec2 vTextureCoord;

void main(void)
{
  gl_Position = vertex;
  vTextureCoord = vertex.xy*0.5+0.5;
  vTextureCoord.x = vTextureCoord.x * uScale.x;
  vTextureCoord.y = vTextureCoord.y * uScale.y;
}
