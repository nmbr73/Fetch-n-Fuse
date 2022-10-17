
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel0
// Connect Image 'Cubemap: Forest_0' to iChannel1


#define MAX_MARCHING_STEPS 255
#define MIN_DIST    0.0f
#define MAX_DIST  100.0f
#define PRECISION   0.001f
#define EPSILON     0.0005f
#define PI          3.14159265359f

__DEVICE__ mat2 rotate2d(float theta) {
  float s = _sinf(theta), c = _cosf(theta);
  return to_mat2(c, -s, s, c);
}

__DEVICE__ float sdSphere(float3 p, float r )
{
  return length(p) - r;
}

__DEVICE__ float sdScene(float3 p) {
  return sdSphere(p, 1.0f);
}

__DEVICE__ float rayMarch(float3 ro, float3 rd) {
  float depth = MIN_DIST;

  for (int i = 0; i < MAX_MARCHING_STEPS; i++) {
    float3 p = ro + depth * rd;
    float d = sdScene(p);
    depth += d;
    if (d < PRECISION || depth > MAX_DIST) break;
  }

  return depth;
}

__DEVICE__ float3 calcNormal(float3 p) {
    float2 e = to_float2(1.0f, -1.0f) * EPSILON;
    float r = 1.0f;
    return normalize(
      swi3(e,x,y,y) * sdScene(p + swi3(e,x,y,y)) +
      swi3(e,y,y,x) * sdScene(p + swi3(e,y,y,x)) +
      swi3(e,y,x,y) * sdScene(p + swi3(e,y,x,y)) +
      swi3(e,x,x,x) * sdScene(p + swi3(e,x,x,x)));
}

__DEVICE__ mat3 camera(float3 cameraPos, float3 lookAtPoint) {
  float3 cd = normalize(lookAtPoint - cameraPos);
  float3 cr = normalize(cross(to_float3(0, 1, 0), cd));
  float3 cu = normalize(cross(cd, cr));
  
  return to_mat3_f3(-cr, cu, -cd);
}

__DEVICE__ float3 phong(float3 lightDir, float lightIntensity, float3 rd, float3 normal, __TEXTURE2D__ iChannel0) {
  float3 cubemapReflectionColor = swi3(decube_f3(iChannel0, reflect(rd, normal)),x,y,z);

  float3 K_a = 1.5f * to_float3(0.7f,0.7f,0.8f) * cubemapReflectionColor; // Reflection
  float3 K_d = to_float3_s(1);
  float3 K_s = to_float3_s(1);
  float alpha = 50.0f;

  float diffuse = clamp(dot(lightDir, normal), 0.0f, 1.0f);
  float specular = _powf(clamp(dot(reflect(lightDir, normal), -rd), 0.0f, 1.0f), alpha);

  return lightIntensity * (K_a + K_d * diffuse + K_s * specular);
}

__DEVICE__ float fresnel(float3 n, float3 rd) {
  return _powf(clamp(1.0f - dot(n, -rd), 0.0f, 1.0f), 5.0f);
}

__KERNEL__ void ReflectiontutorialJipi849Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

  float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
  float2 mouseUV = swi2(iMouse,x,y)/iResolution;
  if (mouseUV.x == 0.0f && mouseUV.y == 0.0f) mouseUV = to_float2_s(0.5f); // trick to center mouse on page load

  float3 lp = to_float3_s(0);
  float3 ro = to_float3(0, 0, 3);
  swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , rotate2d(_mix(-PI/2.0f, PI/2.0f, mouseUV.y))));
  swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , rotate2d(_mix(-PI, PI, mouseUV.x))));

  float3 rd = mul_mat3_f3(camera(ro, lp) , normalize(to_float3_aw(uv, -1)));
  
  float3 col = swi3(decube_f3(iChannel0,rd),x,y,z);

  float d = rayMarch(ro, rd);

  float3 p = ro + rd * d;
  float3 normal = calcNormal(p);

  float3 lightPosition1 = to_float3(1, 1, 1);
  float3 lightDirection1 = normalize(lightPosition1 - p);
  float3 lightPosition2 = to_float3(-8, -6, -5);
  float3 lightDirection2 = normalize(lightPosition2 - p);

  float lightIntensity1 = 0.6f;
  float lightIntensity2 = 0.3f;
    
  float3 sphereColor = phong(lightDirection1, lightIntensity1, rd, normal,iChannel0);
  sphereColor += phong(lightDirection2, lightIntensity2, rd, normal,iChannel0);
  sphereColor += fresnel(normal, rd) * 0.4f;
  
  col = _mix(col, sphereColor, step(d - MAX_DIST, 0.0f));

  fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}