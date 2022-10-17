
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel0



#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float map(float3 p,float t, __TEXTURE2D__ iChannel0) {
  
   float pl = p.y +
    0.1f * texture(iChannel0, _sinf(t*0.008f)+t*0.008f+swi2(p,x,z)*0.5f).x +
    0.2f * texture(iChannel0, _cosf(-t*0.004f)-t*0.004f+swi2(p,x,z)*0.15f).x +
    0.9f * texture(iChannel0, _sinf( t*0.020f)+t*0.010f+swi2(p,x,z)*0.05f).x;

    return pl;
}


__DEVICE__ float3 raymarch(float3 ro,  float3 rd,float t, __TEXTURE2D__ iChannel0) {
    float d = 0.0f;
    float3 p = ro;
    float li=0.0f;
    for(float i=0.0f; i<2000.0f; i++) {
      float h = map(p,t, iChannel0); 
      if(_fabs(h)<0.002f*d) return to_float3(d,i,1);
      if(d>100.0f) return to_float3(d,i,0);
      d+=h;
      p+=rd*h;
      li = i;
    }
    return to_float3(d, li, 0);
}

__DEVICE__ float3 destroyed_normals(float3 p,float t, __TEXTURE2D__ iChannel0) {
    const float2 e = to_float2(0.3f,0.0f);
    return normalize(map(p,t,iChannel0)-to_float3(map(p-swi3(e,x,y,y),t, iChannel0), map(p-swi3(e,y,x,y),t, iChannel0), map(p-swi3(e,y,y,x),t, iChannel0)));
}

__DEVICE__ float3 cam(float2 uv, float3 cameraPos, float3 lookAtPoint, float z) {
    float3 cd = normalize(lookAtPoint - cameraPos); // camera direction
    float3 cr = normalize(cross(to_float3(0, 1, 0), cd)); // camera right
    float3 cu = normalize(cross(cd, cr));
    return normalize(cd*z+uv.x*cr+uv.y*cu);
}


__DEVICE__ float3 phong(float3 lightDir, float3 normal, float3 rd) {
  // ambient
  float k_a = 0.6f;
  float3 i_a = to_float3(0.2f, 0.5f, 0.8f);
  float3 ambient = k_a * i_a;

  // diffuse
  float k_d = 0.7f;
  float dotLN = clamp(dot(lightDir, normal), 0.0f, 1.0f);
  float3 i_d = to_float3(0.0f, 0.3f, 0.7f);
  float3 diffuse = k_d * dotLN * i_d;

  // specular
  float k_s = 0.6f;
  float dotRV = clamp(dot(reflect(lightDir, normal), -rd), 0.0f, 1.0f);
  float3 i_s = to_float3(0.2f, 0.8f, 1.0f);
  float alpha = 12.0f;
  float3 specular = k_s * _powf(dotRV, alpha) * i_s;

  return ambient + diffuse + specular;
}


__KERNEL__ void OceanterrainJipi706Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    float t = mod_f(iTime, 100.0f)-50.0f;
    float2 uv = ((fragCoord/iResolution)-0.5f) / to_float2(iResolution.y / iResolution.x, 1);
    
    float3 ro = to_float3(0,0.5f,-2);
    float3 rd = to_float3_s(0);
    float3 dir = cam(uv,ro,rd,0.9f);
  
    float3 lp = to_float3(1,2,2);
  
    float3 col = to_float3_s(0);
    float3 m = raymarch(ro, dir, t, iChannel0);
    if(m.z == 1.0f) {
      float3 p = ro+dir*m.x;
      float3 n = destroyed_normals(p, t, iChannel0);
      float3 ld = normalize(lp-p);
        
      float3 lightPosition1 = to_float3(8, 2, -20);
      float3 lightDirection1 = normalize(lightPosition1 - m);
      float lightIntensity1 = 0.75f;
      
      col = lightIntensity1 * phong(lightDirection1, n, dir); 
      
    } else {
      col = cos_f3(dir)*to_float3(0.8f, 0.7f, 1.1f)*smoothstep(0.0f,0.1f,dir.y);
    }
    col += _powf(m.y/70.0f, 3.0f);
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}