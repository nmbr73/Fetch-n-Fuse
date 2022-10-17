

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
lowp float snoise(in lowp vec2 v);

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    uv /= 1.1;
    uv += .05;
    uv *= 1.;
	float t = iTime*.8;
	float s = smoothstep(.5,1.,uv.x);
    uv.y += s * sin(t+uv.x * 5.) * .05;// * snoise(uv*2.+1.+t*.3);
    uv.x += s * snoise(uv*(4.3*(s/3.7+1.2))-vec2(t*1.2,0.));
    float tt = mod(t,10.);
    if(tt<3.){
      fragColor = texture(iChannel0,uv);
    } else if(tt<6.){
  	  fragColor = texture(iChannel1,uv);
    } else {
      fragColor = texture(iChannel2,uv);    
    }
}

lowp vec3 permute(in lowp vec3 x) { return mod( x*x*34.+x, 289.); }
lowp float snoise(in lowp vec2 v) {
  lowp vec2 i = floor((v.x+v.y)*.36602540378443 + v),
      x0 = (i.x+i.y)*.211324865405187 + v - i;
  lowp float s = step(x0.x,x0.y);
  lowp vec2 j = vec2(1.0-s,s),
      x1 = x0 - j + .211324865405187, 
      x3 = x0 - .577350269189626; 
  i = mod(i,289.);
  lowp vec3 p = permute( permute( i.y + vec3(0, j.y, 1 ))+ i.x + vec3(0, j.x, 1 )   ),
       m = max( .5 - vec3(dot(x0,x0), dot(x1,x1), dot(x3,x3)), 0.),
       x = fract(p * .024390243902439) * 2. - 1.,
       h = abs(x) - .5,
      a0 = x - floor(x + .5);
  return .5 + 65. * dot( pow(m,vec3(4.))*(- 0.85373472095314*( a0*a0 + h*h )+1.79284291400159 ), a0 * vec3(x0.x,x1.x,x3.x) + h * vec3(x0.y,x1.y,x3.y));
}
