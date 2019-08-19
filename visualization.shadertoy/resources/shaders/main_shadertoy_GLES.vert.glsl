#version 100

attribute vec4 vertex;

varying vec2 vTextureCoord;

uniform vec2 uScale;

void main(void)
{
  gl_Position = vertex;
  vTextureCoord = vertex.xy*0.5+0.5;
  vTextureCoord.x = vTextureCoord.x * uScale.x;
  vTextureCoord.y = vTextureCoord.y * uScale.y;
}
