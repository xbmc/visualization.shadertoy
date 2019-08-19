#version 100

precision mediump float;
precision mediump int;

attribute vec4 vertex;

varying vec2 vTextureCoord;

void main(void)
{
  gl_Position = vertex;
  vTextureCoord = vertex.xy*0.5+0.5;
}
