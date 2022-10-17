
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define PI 3.1415926535f
#define SQRT2 1.4142135624f

#define CAM_FOV_DEG 45.0f


#define _saturatef(x) clamp(x, 0.0f, 1.0f)

__DEVICE__ float lerp(float a, float b, float t) {
    t = _saturatef(t);
    return (1.0f-t)*a + t*b;
}

__DEVICE__ float3 lerp_f3(float3 a, float3 b, float t) {
    t = _saturatef(t);
    return (1.0f-t)*a + t*b;
}

__DEVICE__ bool intersectXYPlane(float3 org, float3 dir, inout float *dist) {
    const float epsilon = 1e-6;
    float3 normal = to_float3(0.0f, 0.0f, 1.0f);

    float denom = dot(-normal, dir); 
    if (denom > epsilon) { 
        *dist = dot(-org, -normal) / denom; 
        return (*dist >= 0.0f); 
    } 
 
    return false; 
}

__DEVICE__ float hash(float2 p) {
  return 2.0f*fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f)-1.0f;   
}

__DEVICE__ float2 hash2(float2 P) {
   return fract_f2(cos_f2(mul_f2_mat2(P , to_mat2(-64.2f,71.3f,81.4f,-29.8f)))*8321.3f); 
}

__DEVICE__ float noise(float2 p) {
    float2 id = _floor(p);
    float2 u = fract_f2(p);
    
    float a = hash(id+to_float2(0,0));
    float b = hash(id+to_float2(1,0));
    float c = hash(id+to_float2(0,1));
    float d = hash(id+to_float2(1,1));
    
    u = u*u*u*(u*(6.0f*u-15.0f)+10.0f);
    
    float k0 = a;
    float k1 = b-a;
    float k2 = c-a;
    float k3 = a-b-c+d;
    
    return k0 + k1*u.x + k2*u.y + k3*u.x*u.y;       
}

__DEVICE__ float fbm(in float2 p, int octaves) {
    const float scale_y = 2.0f;
    const float scale_xz = 0.125f;
    const mat2 rot = to_mat2(0.8f, 0.6f, -0.6f, 0.8f);
    
    p *= scale_xz;

    float res = 0.0f;
    
    mat2 M = to_mat2_s(1.0f);
    
    float A = 1.0f, a = 1.0f;
    
    for (int i=1; i<=octaves; i++) {
        res += A*noise(a*mul_mat2_f2(M,p));
        
        a *= 2.0f;
        A *= 0.5f;
        M = mul_mat2_mat2(M , rot);
    }
    
    return scale_y*res;
}

__DEVICE__ float edge_noise(float2 p, float grid_size) {
    float2 q = p - mod_f2f2(p, to_float2_s(grid_size));
    
    float2 res = to_float2_s(SQRT2*grid_size);
    
    float2 first, second;
    
    for (int i=-1; i<=1; i++) {
        for (int j=-1; j<=1; j++) {
            float2 v = q + grid_size * to_float2(i,j);
            float2 rand = v + grid_size*hash2(v);
            
            float d = length(p-rand);
            
            if (d < res.y) {
                res.x = res.y;
                res.y = d;
                second = first;
                first = rand;
            }
            
            else if (d < res.x) {
                res.x = d;
                second = rand;
            }
        }
    }

    float dist = dot(0.5f*(first+second) - p, normalize(second-first));
    
    return dist/(SQRT2 * grid_size);
}

__DEVICE__ float stains(float2 p) {    
    const float scale = 33.0f;
    float wy = _saturatef(0.7f+0.1f*fbm(scale*p, 8));
    
    
    return smoothstep(0.7f, 0.75f, wy - 0.07f);
}

__DEVICE__ float cracks(float2 p) {
    const float scale = 50.0f;
    const float2 offset = to_float2(5.2f, 1.3f);
    
    p += 0.015f * to_float2(fbm(scale*p, 5), fbm(scale*(p+offset), 5));

    const float grid_size = 0.04f;
    float we = edge_noise(p, grid_size);
    we = 1.0f - smoothstep(0.0f, 0.06f, we);
    
    return _saturatef( (0.44f + fbm(66.0f*p, 3)) * we);
}

__DEVICE__ float2 brickCoords(float2 uv, float2 dims, float offset) {
    if ((int)(_floor(uv.y/dims.y)) % 2 == 0)
        uv += to_float2(offset*dims.x, 0.0f);

    return dims * mod_f2(uv/dims, 1.0f);
}

__DEVICE__ float brick_id(float2 uv) {
    const float2 dims = to_float2(0.3f, 0.1f);
    const float offset = 0.5f;

    if ((int)(_floor(uv.y/dims.y)) % 2 == 0)
        uv += to_float2(offset*dims.x, 0.0f);
        
    float2 ts = _floor(uv/dims);
    return hash(ts);
}

__DEVICE__ float brickHeightmap(float2 uv, float2 dims, float offset, float2 edge){
    float2 ts = brickCoords(uv, dims, offset);
    
    float x = smoothstep(0.0f, edge.x, ts.x) * smoothstep(dims.x, dims.x-edge.x, ts.x);
    float y = smoothstep(0.0f, edge.y, ts.y) * smoothstep(dims.y, dims.y-edge.y, ts.y);
    
    return x*y;
}

#define MAT_BRICK 0
#define MAT_MORTAR 1

__DEVICE__ float Heightmap(float2 uv, inout int *mat_id) {
    //brick params
    const float2 dims = to_float2(0.3f, 0.1f);
    const float offset = 0.5f, height = 1.1f;
    const float2 edge = to_float2_s(0.017f);
    //brick noise params
    const float scale = 100.0f, strength = 0.15f;
    const int octaves = 4;
    //mortar params
    const float m_scale = 100.0f, m_strength = 0.25f, m_base = 0.4f;
    const int m_octaves = 5;
    
    float mortar_height = m_base + m_strength * fbm(m_scale*uv, m_octaves);
    
    float brick_noise = strength * fbm(scale*uv, octaves);
    float brick_height = height * brickHeightmap(uv, dims, offset, edge) + brick_noise;
    brick_height -= 0.1f*cracks(uv);
    
    if (brick_height > mortar_height) {
        *mat_id = MAT_BRICK;
        return brick_height;
    }
    
    else {
        *mat_id = MAT_MORTAR;
        return mortar_height;
    }
}

__DEVICE__ float3 normal(float2 uv) {
    const float smoothness = 0.1f;
    const float2 h = to_float2(0.0f, 0.001f);
    int id;

    float dx = Heightmap(uv + swi2(h,y,x), &id) - Heightmap(uv - swi2(h,y,x), &id);
    float dy = Heightmap(uv + swi2(h,x,y), &id) - Heightmap(uv - swi2(h,x,y), &id);
             
    return normalize(to_float3(-dx, -dy, smoothness));   
}

__DEVICE__ float3 albedo(float2 uv, bool tex, float3 texpar1, float3 texpar2, float3 texpar3, float2 tuv, float4 Color1,float4 Color2,float4 Color3,float4 Color4,float4 Color5, __TEXTURE2D__ iChannel0) {
    int id = MAT_BRICK;
    float height = Heightmap(uv, &id);
    
    float3 col = to_float3_s(0.0f);
    
    if (id == MAT_BRICK) {
        float darkening = 0.25f * _fabs(brick_id(uv));
        //col = to_float3(0.6f, 0.3f, 0.1f) - to_float3_s(darkening);
        col = swi3(Color1,x,y,z) - to_float3_s(darkening);
    }
        
    else if (id == MAT_MORTAR) {
        col = swi3(Color2,x,y,z);//to_float3(0.70f, 0.66f, 0.58f);
    }
    
    float s  = stains((uv + swi2(texpar1,x,y))*texpar1.z);
    float s2 = stains((uv + to_float2(7.25f, 3.73f) + swi2(texpar2,x,y)) * texpar2.z );
    float s3 = stains((uv + to_float2(9.25f, 5.73f) + swi2(texpar3,x,y)) * texpar3.z );
    
    
    
    if ( tex )
    {
      float texin = texture(iChannel0, tuv).w;
      if (texin > 0.0f)
      {
        //col += to_float3_s(0.66f);
        col = lerp_f3(col, s  * swi3(Color3,x,y,z), s - 0.25f);
        col = lerp_f3(col, s2 * swi3(Color4,x,y,z), s2 - 0.25f);  
        col = lerp_f3(col, s3 * swi3(Color5,x,y,z), s3 - 0.25f);  
      }
    }
    else
    {
      col = lerp_f3(col,  s * swi3(Color3,x,y,z), s  - 0.25f);
      col = lerp_f3(col, s2 * swi3(Color4,x,y,z), s2 - 0.25f);
      col = lerp_f3(col, s3 * swi3(Color5,x,y,z), s3 - 0.25f);  
    }
    
    return col;
}

__DEVICE__ float light(float3 pos, float3 norm, float3 dir) {
    float3 l_pos = to_float3(-0.2f, -0.2f, 1.0f);
    
    float3 dif = l_pos - pos;
    
    float attenuation = dot(dif, dif);
    
    dif = normalize(dif);
    
    float3 R = reflect(dif, norm);
    float spec = 0.1f * _powf(saturate(dot(dir, R)), 5.0f);
    
    return (spec + _saturatef(dot(dif, norm)))/attenuation;
}

__KERNEL__ void ProceduralBricksJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution)
{
    CONNECT_CHECKBOX0(Tex, 0);
    
    CONNECT_POINT0(TexPar1XY, 0.0f, 0.0f );
    CONNECT_SLIDER0(TexPar1Z, -100.0f, 100.0f, 1.0f);
    
    CONNECT_POINT1(TexPar2XY, 0.0f, 0.0f );
    CONNECT_SLIDER1(TexPar2Z, -100.0f, 100.0f, 1.0f);
    
    CONNECT_POINT2(TexPar3XY, 0.0f, 0.0f );
    CONNECT_SLIDER2(TexPar3Z, -100.0f, 100.0f, 1.0f);
    
    CONNECT_POINT3(tuvoff, 0.0f, 0.0f );
    CONNECT_SLIDER3(tuvsize, -10.0f, 10.0f, 1.0f);
    
    CONNECT_COLOR0(Color1, 0.6f, 0.3f, 0.1f, 1.0f);
    CONNECT_COLOR1(Color2, 0.70f, 0.66f, 0.58f, 1.0f);
    CONNECT_COLOR2(Color3, 0.66f, 0.66f, 0.66f, 1.0f);
    CONNECT_COLOR3(Color4, 0.22f, 0.22f, 0.22f, 1.0f);
    CONNECT_COLOR4(Color5, 0.11f, 0.11f, 0.11f, 1.0f);
    
  
    float3 texpar1 = to_float3_aw(TexPar1XY, TexPar1Z); 
    float3 texpar2 = to_float3_aw(TexPar2XY, TexPar2Z); 
    float3 texpar3 = to_float3_aw(TexPar3XY, TexPar3Z); 
  
  
  
    const float CAM_DIST = 1.0f/_tanf(CAM_FOV_DEG * 0.5f * PI / 180.0f);

    float2 uv = (2.0f*fragCoord - iResolution)/iResolution.y;
    
    float2 tuv = (fragCoord/iResolution + tuvoff) * tuvsize;

    float3 ray_org = to_float3(0.0f, 0.0f, 1.0f);
    float3 ray_dir = normalize(to_float3_aw(uv, -CAM_DIST));
    
    float3 col = to_float3_s(0.0f);
    
    float dist = 0.0f;
    if (intersectXYPlane(ray_org, ray_dir, &dist)) {
        float3 hit = ray_org + dist * ray_dir;
        
        float3 norm = normal(swi2(hit,x,y));
        col = albedo(swi2(hit,x,y), Tex, texpar1, texpar2, texpar3, tuv, Color1,Color2,Color3,Color4,Color5, iChannel0) * to_float3_s(light(hit, norm, ray_dir));
    }
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}