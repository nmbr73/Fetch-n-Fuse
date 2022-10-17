
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0


struct Ray {
    float3 origin;
    float3 direction;
};



__DEVICE__ float smoothMin(float a, float b, float k) {
    float h = _fmaxf(k - _fabs(a - b), 0.0f) / k;
    return _fminf(a, b) - h * h * h * 1.0f / 6.0f;
}

__DEVICE__ float sdSphere(float3 pos, float3 spherePos, float sphereRadius) {
    return distance_f3(pos, spherePos) - sphereRadius;
}

__DEVICE__ float sdRoundBox( float3 p, float3 b, float r )
{
    float3 q = abs_f3(p) - b;
    return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(_fmaxf(q.x,_fmaxf(q.y,q.z)),0.0f) - r;
}

__DEVICE__ float sdBox2( float3 p, float3 b )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(_fmaxf(q.x,_fmaxf(q.y,q.z)),0.0f);
}

__DEVICE__ float sdBox(float3 pos, float3 boxPos, float3 dimensions) {
    float x = _fabs(pos.x - boxPos.x) - dimensions.x * 0.5f;
    float y = _fabs(pos.y - boxPos.y) - dimensions.y * 0.5f;
    float z = _fabs(pos.z - boxPos.z) - dimensions.z * 0.5f;
    float d = _fmaxf(x, _fmaxf(y, z));
    return d;
}

__DEVICE__ float sdRoundedCylinder( float3 p, float ra, float rb, float h )
{
  float2 d = to_float2( length(swi2(p,x,z))-2.0f*ra+rb, _fabs(p.y) - h );
  return _fminf(_fmaxf(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f))) - rb;
}

__DEVICE__ float sdCylinder(float3 pos, float3 cylinderPos, float height, float radius) {
    float y = _fabs(pos.y - cylinderPos.y) - height * 0.5f;
    float xz = distance_f2(swi2(pos,x,z), swi2(cylinderPos,x,z)) - radius;
    float d = _fmaxf(xz, y);
    return d;
}

__DEVICE__ float sdBrick(float3 pos, float3 brickPos, float2 brickSize) {
    return sdRoundBox(pos - brickPos, to_float3(0.4f * brickSize.x - 0.03f, 0.47f, 0.4f * brickSize.y - 0.03f), 0.03f);
}

__DEVICE__ float sdSmallBrick(float3 pos, float3 brickPos, float2 brickSize) {
    return sdRoundBox(pos - brickPos, to_float3(0.4f * brickSize.x - 0.03f, 0.13666f, 0.4f * brickSize.y - 0.03f), 0.03f);
}

__DEVICE__ float sdKnob(float3 pos, float3 knobPos) { 
    return sdRoundedCylinder(pos - knobPos, 0.13f, 0.01f, 0.16f);
}

__DEVICE__ float sdSmallBrick2x2(float3 pos, float3 brickPos) {
    const float offset = 0.29f * 1.5f;
    const float k = 0.0f;

    return _fminf(
        sdRoundBox(pos - brickPos, to_float3(0.77f, 0.13666f, 0.77f), 0.03f),
        _fminf(
            _fminf(
                sdRoundedCylinder(pos - (brickPos + to_float3(+0.4f, 0.1666f, +0.4f)), 0.13f, 0.01f, 0.16f),
                sdRoundedCylinder(pos - (brickPos + to_float3(-0.4f, 0.1666f, +0.4f)), 0.13f, 0.01f, 0.16f)
            ),
            _fminf(
                sdRoundedCylinder(pos - (brickPos + to_float3(-0.4f, 0.1666f, -0.4f)), 0.13f, 0.01f, 0.16f),
                sdRoundedCylinder(pos - (brickPos + to_float3(+0.4f, 0.1666f, -0.4f)), 0.13f, 0.01f, 0.16f)
            )
        )
    );
}

__DEVICE__ float orangeMap(float3 pos, float iTime) {
    float d = pos.y + 0.1666f;
    float t = (-_cosf(iTime * 0.3f) * 0.5f + 0.5f) * 1.2f + 0.05f;
    
    const float3 brickSize = to_float3(0.8f, 1.0f, 0.8f);
    
    if(t > 0.1f) {
        float y = 1.0f - (_fminf(t - 0.1f, 0.1f) / 0.1f);
    
        d = _fminf(d, sdSmallBrick(pos, to_float3(0.0f, 0.0f + y * 10.0f, 0.0f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 0.1666f + y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 0.1666f + y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 0.1666f + y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 0.1666f + y * 10.0f, -0.4f)));
    }
    
    if(t > 0.8f) {
        float y = 1.0f - (_fminf(t - 0.8f, 0.1f) / 0.1f);
        d = _fminf(d, sdSmallBrick(pos, to_float3(0.0f, 3.7333f+ y * 10.0f, -0.8f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.9f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.9f+ y * 10.0f, -0.4f)));  
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.9f+ y * 10.0f, -1.2f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.9f+ y * 10.0f, -1.2f)));
    }
    
    return d;
}

__DEVICE__ float map(float3 pos, float iTime) {
    float d = pos.y + 0.1666f;
    float t = (-_cosf(iTime * 0.3f) * 0.5f + 0.5f) * 1.2f + 0.05f;
    
    const float3 brickSize = to_float3(0.8f, 1.0f, 0.8f);
float zzzzzzzzzzzzzzzzzzz;    
    //if(t > 0.1f) {
        float y = 1.0f - (_fminf(t - 0.1f, 0.1f) / 0.1f);
    
        d = _fminf(d, sdSmallBrick(pos, to_float3(0.0f, 0.0f + y * 10.0f, 0.0f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 0.1666f + y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 0.1666f + y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 0.1666f + y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 0.1666f + y * 10.0f, -0.4f)));
    //}
    
    //if(t > 0.2f) {
         y = 1.0f - (_fminf(t - 0.2f, 0.1f) / 0.1f);
        d = _fminf(d, sdBrick(pos, to_float3(0.0f, 0.6666f+ y * 10.0f, 0.4f), to_float2(2, 1)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 1.1666f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 1.1666f+ y * 10.0f, +0.4f)));
    //}
    
    //if(t > 0.3f) {
         y = 1.0f - (_fminf(t - 0.3f, 0.1f) / 0.1f);
        d = _fminf(d, sdSmallBrick(pos, to_float3(0.0f, 1.3666f+ y * 10.0f, 0.4f), to_float2(2, 3)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 1.5333f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 1.5333f+ y * 10.0f, +0.4f)));        
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 1.5333f+ y * 10.0f, +1.2f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 1.5333f+ y * 10.0f, +1.2f)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 1.5333f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 1.5333f+ y * 10.0f, -0.4f)));
    //}
    
    //if(t > 0.4f) {
         y = 1.0f - (_fminf(t - 0.4f, 0.1f) / 0.1f);
        d = _fminf(d, sdBrick(pos, to_float3(0.0f, 2.0666f+ y * 10.0f, 0.0f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 2.5666f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 2.5666f+ y * 10.0f, +0.4f)));  
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 2.5666f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 2.5666f+ y * 10.0f, -0.4f)));
    //}
    
    //if(t > 0.5f) {
         y = 1.0f - (_fminf(t - 0.5f, 0.1f) / 0.1f);
        d = _fminf(d, sdBrick(pos, to_float3(0.0f, 2.0666f+ y * 10.0f, 1.6f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 2.5666f+ y * 10.0f, +2.0f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 2.5666f+ y * 10.0f, +2.0f)));  
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 2.5666f+ y * 10.0f, +1.2f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 2.5666f+ y * 10.0f, +1.2f)));
    //}
    
    //if(t > 0.6f) {
         y = 1.0f - (_fminf(t - 0.6f, 0.1f) / 0.1f);
        d = _fminf(d, sdBrick(pos, to_float3(0.0f, 3.0666f+ y * 10.0f, 0.8f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.5666f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.5666f+ y * 10.0f, +0.4f)));  
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.5666f+ y * 10.0f, +1.2f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.5666f+ y * 10.0f, +1.2f)));
    //}
    
    //if(t > 0.7f) {
         y = 1.0f - (_fminf(t - 0.7f, 0.1f) / 0.1f);
        d = _fminf(d, sdBrick(pos, to_float3(0.0f, 3.0666f+ y * 10.0f, -0.4f), to_float2(4, 1)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.5666f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.5666f+ y * 10.0f, -0.4f)));  
        d = _fminf(d, sdKnob(pos, to_float3(+1.2f, 3.5666f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-1.2f, 3.5666f+ y * 10.0f, -0.4f)));
    //}
    
    //if(t > 0.8f) {
         y = 1.0f - (_fminf(t - 0.8f, 0.1f) / 0.1f);
        d = _fminf(d, sdSmallBrick(pos, to_float3(0.0f, 3.7333f+ y * 10.0f, -0.8f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.9f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.9f+ y * 10.0f, -0.4f)));  
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.9f+ y * 10.0f, -1.2f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.9f+ y * 10.0f, -1.2f)));
    //}
    
    //if(t > 0.9f) {
         y = 1.0f - (_fminf(t - 0.9f, 0.1f) / 0.1f);
        d = _fminf(d, sdSmallBrick(pos, to_float3(0.0f, 3.7333f+ y * 10.0f, +0.4f), to_float2(2, 1)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 3.9f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 3.9f+ y * 10.0f, +0.4f)));
    //}
    
    //if(t > 1.0f) {
         y = 1.0f - (_fminf(t - 1.0f, 0.1f) / 0.1f);
        d = _fminf(d, sdBrick(pos, to_float3(0.0f, 4.4f+ y * 10.0f, 0.0f), to_float2(2, 2)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 4.9f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 4.9f+ y * 10.0f, +0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(+0.4f, 4.9f+ y * 10.0f, -0.4f)));
        d = _fminf(d, sdKnob(pos, to_float3(-0.4f, 4.9f+ y * 10.0f, -0.4f)));
    //}
    
    return d;
}
 
//#define ORG //ORG: 4.0s  else 2.7s
#ifdef ORG
__DEVICE__ float3 calcNormal(float3 pos, float iTime) {
    const float epsilon = 0.001f;
    float dist0 = map(pos.iTime);
    float dist1 = map(pos + to_float3(epsilon, 0.0f, 0.0f),iTime);
    float dist2 = map(pos + to_float3(0.0f, epsilon, 0.0f),iTime);
    float dist3 = map(pos + to_float3(0.0f, 0.0f, epsilon),iTime);
    return (to_float3(dist1, dist2, dist3) - dist0) / epsilon;
}

#else
__DEVICE__ float3 calcNormal(float3 pos, float iTime)
{
    const float epsilon = 0.001f;
    float3 n = to_float3_s(0.0f);
    for( int i=0 ; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+epsilon*e,iTime);
    }
    return normalize(n);
}

#endif

struct Hit {
    float3 position;
    float closestDistance;
    int raySteps;
    bool hit;
};

__DEVICE__ Hit castRay(Ray ray, float iTime, mat3 rotationMatrix) {
    Hit hit;
    hit.position = ray.origin;
    hit.raySteps = 0;
    hit.hit = false;
    
    float hitDist = 0.0f;
    for(; hit.raySteps < 100; ++hit.raySteps) {
        hit.position = ray.origin + ray.direction * hitDist;
        
        float sd = map(mul_f3_mat3(hit.position , rotationMatrix), iTime);
        
        if(sd < 0.01f) { 
            hit.hit = true;
            break;
        }
        
        hitDist += sd;
    }
    
    return hit;
}

__DEVICE__ bool obscured(Ray ray, float iTime, mat3 rotationMatrix) {
    float occlusion = 1.0f;
    float3 position;
    float hitDist = 0.0f;
    for(int raySteps = 0; raySteps < 100; ++raySteps) {
        position = ray.origin + ray.direction * hitDist;
        float sd = map(mul_f3_mat3(position , rotationMatrix), iTime);
        
        if(sd < 0.01f) { 
            return true;
        }
        
        hitDist += sd;
    }
    return false;
}

__DEVICE__ float softShadow(Ray ray, float k, float iTime, mat3 rotationMatrix) {
    float visibility = 1.0f;
    float3 position;
    float hitDist = 0.0f;
    for(int raySteps = 0; raySteps < 100; ++raySteps) {
        position = ray.origin + ray.direction * hitDist;
        float sd = map(mul_f3_mat3(position , rotationMatrix),iTime);
        
        if(sd < 0.01f) { 
            return 0.0f;
        }
        
        visibility = _fmaxf(smoothMin(visibility, k * sd/hitDist, 0.3f), 0.0f);
        hitDist += sd;
    }
    return visibility;
}

__DEVICE__ float3 calculateColor(float2 seed, float3 normal, float3 viewDir, float3 position, float3 albedo, float3 reflection, int raySteps, float iTime, mat3 rotationMatrix) {
    const float3 lightDir = normalize(to_float3(0.4f, 1.0f, -0.8f));
    float diffuse = _fmaxf(dot(normal, lightDir), 0.0f);
    
    const float3 orange = to_float3(_powf(1.0f,2.2f), _powf(0.5f,2.2f), _powf(0.3f,2.2f));
    const float3 yellow = to_float3(_powf(1.0f,2.2f), _powf(0.8f,2.2f), _powf(0.3f,2.2f));
    
    float r0 = 0.05f;
    float ao = 0.0f;
    if(position.y > -0.1f) {
        ao = _powf(1.0f / float(raySteps), 0.3f);
        albedo = yellow;
        if(orangeMap(mul_f3_mat3(position , rotationMatrix),iTime) < 0.01f)
            albedo = orange;
    }else{
        r0 = 0.0f;
        ao = _powf(1.0f / (float)(raySteps), 0.1f);
    }
    
    /*float shadow = 0.0f;
    for(int i = 0; i < 4; i++) {
        float3 rand = textureLod(iChannel0, seed, 0.0f).xyz;
        seed = swi2(rand,x,y);
    
        Ray shadowRay;
        shadowRay.direction = lightDir + rand - 0.5f;
        shadowRay.origin = position + shadowRay.direction * 0.15f;

        shadow += obscured(shadowRay, iTime, rotationMatrix) ? 0.0f : 1.0f;
    }
    shadow /= 4.0f*/;
    
    
    Ray shadowRay;
    shadowRay.direction = lightDir;
    shadowRay.origin = position + shadowRay.direction * 0.15f;
    float shadow = softShadow(shadowRay, 2.0f, iTime, rotationMatrix);
    
    
    diffuse = diffuse * shadow * 0.5f + 0.65f;
    
    float fresnel = r0 + (1.0f - r0) * _powf(1.0f - dot(-viewDir, normal), 5.0f);
    
    float specular = _fmaxf(dot(reflect(viewDir, normal), lightDir), 0.0f) * fresnel;
    
    return (albedo * diffuse + fresnel * reflection + specular) * ao;
}

__KERNEL__ void LegoShaderJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{

    float2 screenPos = (fragCoord - iResolution * 0.5f) / iResolution.y;

    Ray ray;
    ray.origin = to_float3(0.0f, 9.0f, -8.0f);
    ray.direction = normalize(to_float3_aw(screenPos, 1.0f));
    const float cameraRotation = 40.0f;
    const float cameraRotSin = _sinf(cameraRotation * 0.0174532f);
    const float cameraRotCos = _cosf(cameraRotation * 0.0174532f);
    const mat3 cameraMatrix = to_mat3(
                                        1.0f, 0.0f, 0.0f,
                                        0.0f, cameraRotCos, -cameraRotSin,
                                        0.0f, cameraRotSin, cameraRotCos
                                     );
    ray.direction = mul_f3_mat3(ray.direction , cameraMatrix);
    
    float rot = iTime * 0.8f;
    float rotSin = _sinf(rot);
    float rotCos = _cosf(rot);
    mat3 rotationMatrix = to_mat3(
                              rotCos, 0.0f, rotSin,
                              0.0f, 1.0f, 0.0f,
                              -rotSin, 0.0f, rotCos);
    
    float rot_inv = -rot;
    float rotSin_inv = _sinf(rot_inv);
    float rotCos_inv = _cosf(rot_inv);
    mat3 rotationMatrix_inv = to_mat3(
                              rotCos_inv, 0.0f, rotSin_inv,
                              0.0f, 1.0f, 0.0f,
                              -rotSin_inv, 0.0f, rotCos_inv);
    
    
    Hit hit = castRay(ray, iTime, rotationMatrix);
        
    if(hit.hit) {
        float3 normal = normalize(calcNormal(mul_f3_mat3(hit.position , rotationMatrix), iTime));
        
        Ray reflectionRay;
        reflectionRay.direction = reflect(ray.direction, normal);
        reflectionRay.origin = hit.position + reflectionRay.direction * 0.02f;
        Hit reflection = castRay(reflectionRay, iTime, rotationMatrix);
        float3 reflectionCol = to_float3(1.0f, 1.0f, 1.0f);
        if(reflection.hit)
        
            //reflectionCol = calculateColor(screenPos.xy, normalize(calcNormal(reflection.position * rotationMatrix)) * rotationMatrix_inv, reflectionRay.direction, reflection.position, vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), hit.raySteps);      
        
            reflectionCol = calculateColor(swi2(screenPos,x,y), mul_f3_mat3(normalize(calcNormal(mul_f3_mat3(reflection.position , rotationMatrix), iTime)) , rotationMatrix_inv), reflectionRay.direction, reflection.position, to_float3(1.0f, 1.0f, 1.0f), to_float3(1.0f, 1.0f, 1.0f), hit.raySteps, iTime, rotationMatrix);
        float3 col = calculateColor(swi2(screenPos,x,y), mul_f3_mat3(normal , rotationMatrix_inv), ray.direction, hit.position, to_float3(1.0f, 1.0f, 1.0f), reflectionCol, hit.raySteps, iTime, rotationMatrix);
        fragColor = to_float4(_powf(col.x, 1.0f / 2.2f), _powf(col.y, 1.0f / 2.2f), _powf(col.z, 1.0f / 2.2f), 0.0f);//to_float4_aw(normal, 1.0f) * 0.5f + 0.5f;
    } else {
        fragColor = to_float4(1.0f, 1.0f, 1.0f, 1.0f);
    }   
float IIIIIIIIIIIIIIIIIIIIIII;
  SetFragmentShaderComputedColor(fragColor);
}