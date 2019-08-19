#version 150

in vec4 vertex;
out vec2 vTextureCoord;

void main(void)
{
  gl_Position = vertex;
  vTextureCoord = vertex.xy*0.5+0.5;
}
