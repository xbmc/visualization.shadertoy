#version 100

precision mediump float;
precision mediump int;

varying vec2 vTextureCoord;

uniform sampler2D uTexture;

void main(void)
{
  gl_FragColor = texture2D(uTexture, vTextureCoord);
}
