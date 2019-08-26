void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  float y = ( fragCoord.y / iResolution.y ) * 26.0;
  float x = 1.0 - ( fragCoord.x / iResolution.x );
  float b = fract( pow( 2.0, floor(y) ) + x );
  if (fract(y) >= 0.9)
  b = 0.0;
  fragColor = vec4(b, b, b, 1.0 );
}
