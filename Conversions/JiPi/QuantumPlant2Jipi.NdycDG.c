
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float cubeHeight(float3 cell, float iTime) {
    float t = iTime * 2.0f;
    return (_sinf(cell.x + t*0.125f)*0.125f+_sinf(cell.z + t)*0.125f) * 15.0f;
}

__DEVICE__ float3 cellTransform(float3 m, float CELL_SIZE, float iTime) {

   float cellSize = CELL_SIZE;

   float3 cell = round(m / cellSize);
   cell.y = _fminf(cell.y, -2.0f);

   m.y -= cubeHeight(cell, iTime);
   
   return m - cell * cellSize;
}

__DEVICE__ float udBox(float3 p, float CELL_SIZE) {
    float a = CELL_SIZE*0.45f;
    float corner = a * 0.1f;

    return length(_fmaxf(abs_f3(p) - a, to_float3_s(0.0f))) - corner;
}

__DEVICE__ float map(float3 m, float CELL_SIZE, float iTime) {

   float3 cell = round(m / CELL_SIZE);
   cell.y = -2.0f;

   float dist = 9999.0f;

   for(float i=-1.0f;i<2.0f;i+=1.0f) {
      for(float j=-1.0f;j<2.0f;j+=1.0f) {

            float3 neighbour = cell;

            neighbour.x += i;
            neighbour.z += j;
        
            float3 p = m - neighbour * CELL_SIZE;
            p.y -= cubeHeight(neighbour, iTime);        
        
            dist = _fminf(dist, udBox(p, CELL_SIZE));
        }
    }

    return dist;
}

__DEVICE__ bool rayMarch(in float3 ro, in float3 rd, out float3 *m, float max, float CELL_SIZE, float iTime) {
    
    float md = 0.0f;
   
    while(md < max) {       
    
        *m = ro + rd * md;    
        
        float dist = map(*m, CELL_SIZE, iTime);
        
        if(dist < 0.01f) {
            return true;
        }

        md += _fminf(CELL_SIZE, dist);
    }
    
  return false;    
}

__DEVICE__ float3 computeNormal(in float3 pos, float CELL_SIZE, float iTime) {
  float3 eps = to_float3( 0.005f, 0.0f, 0.0f );
  float3 nor = to_float3(
       map(pos+swi3(eps,x,y,y), CELL_SIZE, iTime) - map(pos-swi3(eps,x,y,y), CELL_SIZE, iTime),
       map(pos+swi3(eps,y,x,y), CELL_SIZE, iTime) - map(pos-swi3(eps,y,x,y), CELL_SIZE, iTime),
       map(pos+swi3(eps,y,y,x), CELL_SIZE, iTime) - map(pos-swi3(eps,y,y,x), CELL_SIZE, iTime));
  return normalize(nor);
}



__DEVICE__ float computeShadow(float3 p, float3 light, float CELL_SIZE, float iTime, float SHADOW_FADE) {

    float3 m;
    
    if(rayMarch(p - light * 0.05f, -light, &m, SHADOW_FADE, CELL_SIZE, iTime)) {
        float distFactor = clamp(length(m-p)/SHADOW_FADE, 0.0f, 1.0f);
        return 0.5f + smoothstep(0.0f, 1.0f, distFactor) * 0.5f;
    }

    return 1.0f;
}

__DEVICE__ float4 text3d(float3 p, float3 n, __TEXTURE2D__ iChannel1, float3 tex, float3 tex2) {
  p=p*0.2f*tex2.x;//IQ made it
  
  p.x+=tex.x; // TexturOffset
  p.y+=tex.y;
  p.y+=tex.z;
  
  float3 a = n*n;
  p.y*= tex2.y; //ratio
  float4 x = texture(iChannel1, swi2(p,y,z) );
  p.z*= tex2.y; //ratio
  float4 y = texture(iChannel1, swi2(p,z,x) );
  float4 z = texture(iChannel1, swi2(p,y,x) );
  return (x*a.x + y*a.y + z*a.z) / (a.x + a.y + a.z);
}

__DEVICE__ float4 render(float3 ro, float3 rd, float3 m, float3 normal, float4 env, float CELL_SIZE, float3 light, float SHADOW_FADE, float iTime, __TEXTURE2D__ iChannel1, float3 tex,float3 tex2) {
float zzzzzzzzzzzzzz;
    float shadow = computeShadow(m, light, CELL_SIZE, iTime, SHADOW_FADE);
    float diffuse = clamp(dot(normal, -light), 0.0f,1.0f);
    float hilight = _powf(clamp(dot(reflect(light, normal), rd), 0.0f, 1.0f), 40.0f) * 0.0f;
    
    float4 baseColor = text3d(cellTransform(m, CELL_SIZE, iTime), normal, iChannel1, tex, tex2);
    
    float4 testMask = to_float4_s(baseColor.y);
//    return testMask;
    
    float fresnel = (1.0f - clamp(dot(-rd, normal), 0.0f, 1.0f))*0.25f;    
    
    return (to_float4_s(0.25f + diffuse) * _mix(baseColor, env, fresnel) + hilight) * shadow;
}

__KERNEL__ void QuantumPlant2JipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
  
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    CONNECT_SLIDER0(CELL_SIZE, 0.01f, 10.0f, 2.0f);    
    //const float CELL_SIZE = 2.0f;
    const float3 light = normalize(to_float3(1,-1,-1));
    
    
    CONNECT_SLIDER1(SHADOW_FADE, -1.0f, 30.0f, 10.0f);
    //const float SHADOW_FADE = 10.0f;
    
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f );
    //CONNECT_SLIDER2(ViewZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_POINT1(CameraViewXY, 0.0f, 0.0f );
    CONNECT_SLIDER2(CameraViewZ, -20.0f, 20.0f, 0.0f);
    
    CONNECT_POINT2(TexXY, 0.0f, 0.0f );
    CONNECT_SLIDER3(TexZ, -20.0f, 20.0f, 0.0f);
    CONNECT_SLIDER4(TexMul, -20.0f, 20.0f, 0.0f);
    
    float3 tex = to_float3_aw(TexXY, TexZ);
    float3 tex2 = to_float3(TexMul, iResolution.y/iResolution.x, 0.0f);
    
float IIIIIIIIIIIIIIII;
    float3 m;
    float2 uv = (fragCoord - iResolution * 0.5f) / iResolution.y;

    float angleY = 3.1415f*0.25f*iTime * 0.1f;
    float cosa = _cosf(angleY+ViewXY.x);
    float sina = _sinf(angleY+ViewXY.x);    

    mat3 rotY = to_mat3_f3(to_float3(cosa, 0.0f, sina), to_float3(0.0f, 1.0f, 0.0f), to_float3(-sina, 0.0f, cosa));

    float angleX = 0.7f;//iTime * 0.0f;
    cosa = _cosf(angleX+ViewXY.y);
    sina = _sinf(angleX+ViewXY.y);    

    mat3 rotX = to_mat3_f3(to_float3(1.0f, 0.0f, 0.0f), to_float3(0.0f, cosa, sina), to_float3(0.0f, -sina, cosa));

    mat3 transfo = mul_mat3_mat3(rotY , rotX);

    float camAnim = iTime*0.0f;
    float3 camera = to_float3(0, -2.0f, -10.0f + 1.5f*_sinf(camAnim));
    
    float3 ro = camera + to_float3_aw(CameraViewXY, CameraViewZ);
    float3 rd  = normalize(to_float3_aw(swi2(uv,x,y), 1.0f));

    ro = mul_mat3_f3(transfo , ro);
    rd = mul_mat3_f3(transfo , rd);

    if(rayMarch(ro, rd, &m, 100.0f, CELL_SIZE, iTime)) {
        float3 normal = computeNormal(m, CELL_SIZE, iTime);
        
        float3 refl = reflect(rd, normal);
        float3 m2;
        
        float4 env; 
        
        if(rayMarch(m + refl*0.015f, refl, &m2, 100.0f, CELL_SIZE, iTime)) {
            float3 normal2 = computeNormal(m2, CELL_SIZE, iTime);
            float3 localRefl = reflect(refl, normal2);
            float4 localEnv = decube_f3(iChannel0,localRefl); 
            
 //render(float3 ro, float3 rd, float3 m, float3 normal, float4 env, float CELL_SIZE, float3 light, float SHADOW_FADE, float iTime, __TEXTURE2D__ iChannel1) {
            env = render(m, refl, m2, normal2, localEnv, CELL_SIZE, light, SHADOW_FADE, iTime, iChannel1, tex, tex2);
        }
        else {
            env = decube_f3(iChannel0,refl);
        }
        
        fragColor = render(ro, rd, m, normal, env, CELL_SIZE, light, SHADOW_FADE, iTime, iChannel1, tex, tex2);
    }
    else {
       fragColor = to_float4_s(0); 
    }

    fragColor += (Color-0.5f);
    fragColor.w=Color.w;

  SetFragmentShaderComputedColor(fragColor);
}