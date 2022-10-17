

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
	fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Hue to RGB function from Fabrice's shadertoyunofficial blog:
#define hue2rgb(hue) 0.6 + 0.6 * cos(6.3 * hue + vec3(0.0, 23.0, 21.0))

const float radius = 0.15;
const vec2 center = vec2(0.0);


vec3 irri(float hue) {
  return .5+ .5 *cos(( 9.*hue)+ vec3(0,23.,21.));
}

const float numOfBalls = 10.;
const float distanceTraveled = 1.5;
const float speed = .8;
const float rotationSpeed = .5;

float metaballs(vec2 uv, float time) {									
    float size = .9;					
    const float startIndex = numOfBalls;
	const float endIndex = numOfBalls * 2.;
    
    for(float i = startIndex; i < endIndex; i++) {					// create x number of balls											// get rads for control point
        float radius = distanceTraveled * sin(time + i * 2.);		// calculate radius
        vec2 ball = radius * vec2(sin(i), cos(i));					// ball position
		size += 1. / pow(i, distance(uv, ball));					// metaball calculation
    }
    return size;
}

float aastep(float threshold, float value) {

    float afwidth = length(vec2(dFdx(value), dFdy(value))) / max(iResolution.x,iResolution.y);
    return smoothstep(threshold-afwidth, threshold+afwidth, value);
 
}

#define R iResolution

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 screenCenter = 0.5 * iResolution.xy;
    vec2 uv = (fragCoord - screenCenter) / iResolution.y;
    
    
     float zoom = .99;
     vec2 direction = vec2(-0.0025,0.0025)*0.;
     vec4 previousColor =  texture(iChannel0, ((fragCoord - screenCenter) * zoom / iResolution.xy + 0.5)+direction);
     float edgeSmoothing = 1./max(iResolution.x,iResolution.y);
      //previousColor *= iFeedbackFadeRate;
      vec2 off = vec2(sin(iTime)*.5,0.);
      float blob = metaballs((uv+off)*8., iTime);
      float shape = smoothstep(
        1.,
        1.+ edgeSmoothing,
        blob
      );
      shape = aastep(1.-sin(iTime)*.5+.5, blob);
      //shape = smoothstep(dFdx(uv.y),1.-dFdx(uv.y),shape);
      vec3 col = irri(shape+ iTime *.25);
      vec4 newColor = vec4(shape*col,1.);
      vec4 color = max(previousColor,newColor);
      // color.xyz += shape*irri(iTime*0.1)*1.1;
      color = clamp(color, 0., 1.);
      //fragColor += (1.-fragColor.a) * vec4( color.rgb, 1 ) *color.a;
      
      fragColor = color*.95;
      
}