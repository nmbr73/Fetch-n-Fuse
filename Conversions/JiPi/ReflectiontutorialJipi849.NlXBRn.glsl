

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const int MAX_MARCHING_STEPS = 255;
const float MIN_DIST = 0.0;
const float MAX_DIST = 100.0;
const float PRECISION = 0.001;
const float EPSILON = 0.0005;
const float PI = 3.14159265359;

mat2 rotate2d(float theta) {
  float s = sin(theta), c = cos(theta);
  return mat2(c, -s, s, c);
}

float sdSphere(vec3 p, float r )
{
  return length(p) - r;
}

float sdScene(vec3 p) {
  return sdSphere(p, 1.);
}

float rayMarch(vec3 ro, vec3 rd) {
  float depth = MIN_DIST;

  for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
    vec3 p = ro + depth * rd;
    float d = sdScene(p);
    depth += d;
    if (d < PRECISION || depth > MAX_DIST) break;
  }

  return depth;
}

vec3 calcNormal(vec3 p) {
    vec2 e = vec2(1.0, -1.0) * EPSILON;
    float r = 1.;
    return normalize(
      e.xyy * sdScene(p + e.xyy) +
      e.yyx * sdScene(p + e.yyx) +
      e.yxy * sdScene(p + e.yxy) +
      e.xxx * sdScene(p + e.xxx));
}

mat3 camera(vec3 cameraPos, vec3 lookAtPoint) {
	vec3 cd = normalize(lookAtPoint - cameraPos);
	vec3 cr = normalize(cross(vec3(0, 1, 0), cd));
	vec3 cu = normalize(cross(cd, cr));
	
	return mat3(-cr, cu, -cd);
}

vec3 phong(vec3 lightDir, float lightIntensity, vec3 rd, vec3 normal) {
  vec3 cubemapReflectionColor = texture(iChannel0, reflect(rd, normal)).rgb;

    
 
  vec3 K_a = 1.5 * vec3(0.7,0.7,0.8) * cubemapReflectionColor; // Reflection
  vec3 K_d = vec3(1);
  vec3 K_s = vec3(1);
  float alpha = 50.;

  float diffuse = clamp(dot(lightDir, normal), 0., 1.);
  float specular = pow(clamp(dot(reflect(lightDir, normal), -rd), 0., 1.), alpha);

  return lightIntensity * (K_a + K_d * diffuse + K_s * specular);
}

float fresnel(vec3 n, vec3 rd) {
  return pow(clamp(1. - dot(n, -rd), 0., 1.), 5.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
  vec2 mouseUV = iMouse.xy/iResolution.xy;
  if (mouseUV == vec2(0.0)) mouseUV = vec2(0.5); // trick to center mouse on page load

  vec3 lp = vec3(0);
  vec3 ro = vec3(0, 0, 3);
  ro.yz *= rotate2d(mix(-PI/2., PI/2., mouseUV.y));
  ro.xz *= rotate2d(mix(-PI, PI, mouseUV.x));

  vec3 rd = camera(ro, lp) * normalize(vec3(uv, -1));
  
  vec3 col = texture(iChannel0, rd).rgb;

  float d = rayMarch(ro, rd);

  vec3 p = ro + rd * d;
  vec3 normal = calcNormal(p);

  vec3 lightPosition1 = vec3(1, 1, 1);
  vec3 lightDirection1 = normalize(lightPosition1 - p);
  vec3 lightPosition2 = vec3(-8, -6, -5);
  vec3 lightDirection2 = normalize(lightPosition2 - p);

  float lightIntensity1 = 0.6;
  float lightIntensity2 = 0.3;
    
  vec3 sphereColor = phong(lightDirection1, lightIntensity1, rd, normal);
  sphereColor += phong(lightDirection2, lightIntensity2, rd, normal);
  sphereColor += fresnel(normal, rd) * 0.4;
  
  col = mix(col, sphereColor, step(d - MAX_DIST, 0.));

  fragColor = vec4(col, 1.0);
}
