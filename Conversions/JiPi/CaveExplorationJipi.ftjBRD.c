
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Lichen' to iChannel0
// Connect Image 'Texture: RGBA Noise Medium' to iChannel1

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Based on "Stalactite Cave" by mazander - https://www.shadertoy.com/view/ldV3W1

// =================================== General functions ===================================

// Grey scale.
__DEVICE__ float getGrey(float3 p){ return p.x*0.299f + p.y*0.587f + p.z*0.114f; }

__DEVICE__ float noise(in float3 p, __TEXTURE2D__ iChannel1){
    float3 i = _floor(p); 
    p -= i; 
    p *= p*(3.0f - 2.0f*p); //from linear to smooth - derivatives at 0 and 1 are zero.
    // The RGBA noise satisfies G(x+37,y+17) = R(x,y), though only when VFLIP is unchecked.
  swi2S(p,x,y, swi2(texture(iChannel1, (swi2(p,x,y) + swi2(i,x,y) + to_float2(37, 17)*i.z + 0.5f)/256.0f),y,x));
  return _mix(p.x, p.y, p.z);
}

// Project the 2D texture from the three main directions and mix them together to create a 3D texture.
__DEVICE__ float4 texcube(__TEXTURE2D__ sam, in float3 p, in float3 n){
float zzzzzzzzzzzzzzzzzz;
  p *= 1.5f;    
  float4 x = texture( sam, swi2(p,y,z) );
  float4 y = texture( sam, swi2(p,z,x) );
  float4 z = texture( sam, swi2(p,x,y) );
  return x*_fabs(n.x) + y*_fabs(n.y) + z*_fabs(n.z);
}

// =================================== Scene ===================================

// The path for the center of the cave
__DEVICE__ float3 path(float time) {
    return to_float3(_cosf(0.51f * time) + _sinf(0.14f * time), 0.8f * _sinf(0.27f * time), time); 
}


// Distance to the inside of the cave
__DEVICE__ float mapTerrain(float3 position, __TEXTURE2D__ iChannel1) {
    // Your friendly random orthogonal (rotation) matrix.
    const mat3 m = to_mat3( 0.00f, -0.80f, -0.60f,
                            -0.80f,  0.36f, -0.48f,
                            -0.60f, -0.48f,  0.64f );
  
    float initialCaveSize = 0.00f;

    // an approximation to the distance vector from the path.
    float3 xyDifference = position - path(position.z);
    
    // stretching the distance - the smaller a coordinate is (X or Y), the larger the cave is along that axis.
    float3 stretchFactor = 1.2f * to_float3(_sinf(position.z * 0.32f), _cosf(position.z * 0.77f), 1.0f);
    float d = initialCaveSize-length(xyDifference * stretchFactor);
    
    // Add stalactites by adding noise which has lower frequency along the y-direction (So it changes faster
    // along the XZ plane).
    float3 stalactites = to_float3(6.0f, 0.15f, 6.0f);
    d += 0.7500f * noise(stalactites * position, iChannel1); 
    
    // Add more random noise to add the smaller details
    float strength = 0.5f;
    for (int i=0; i<5; i++){
        position = mul_mat3_f3(m,position)*2.04f;
        d += strength * noise( position, iChannel1 );
        strength *= 0.5f;
    }
        
    return d;
}

// moss color for a given point and normal.
__DEVICE__ float3 moss(float3 position, float3 normal, __TEXTURE2D__ iChannel1){
    // I have no idea how moss works...
    float moss_noise = noise(position*100.0f,iChannel1);
    float3 moss_color = _mix(to_float3(0.0f, 0.1f, 0.0f), to_float3(0.3f, 0.3f, 0.0f), moss_noise*moss_noise);
    moss_noise = noise(position*210.0f+100.0f,iChannel1);
    moss_color = _mix(moss_color, to_float3(0.3f, 0.6f, 0.3f)*0.8f, _powf(moss_noise, 5.0f));
    moss_color*=2.0f*texture(iChannel1,swi2(position,x,z)).x;
    
    return moss_color;
}

// =================================== Compute position and normal ===================================

// Sets the orientation of the camera given its position and viewing target.
__DEVICE__ void camera(in float3 origin, in float3 target, out float3 *forward, out float3 *right, out float3 *up){
    *forward = normalize(target-origin);
    *right = normalize(to_float3((*forward).z, 0.0f, -(*forward).x)); // Perpendicular to the forward direction ~ approx (1,0,0).
    *up = normalize(cross(*forward, *right));
}

// Standard ray marching
__DEVICE__ float rayMarch(in float3 ro, in float3 rd, __TEXTURE2D__ iChannel1){
  float maxd = 20.0f;
    float t = 0.1f;
    for( int i = 0; i< 160; i++ )
    {
      float h = mapTerrain( ro + rd * t, iChannel1 );
        if( h < (0.001f * t) || t > maxd ) break;
        t += (step(h, 1.0f) * 0.05f + 0.1f) * h;
    }

    if( t>maxd ) t=-1.0f;
    return t;
}

__DEVICE__ float3 calcNormal( in float3 pos, in float t, __TEXTURE2D__ iChannel1 ){
  float3 eps = to_float3( _fmaxf(0.02f,0.001f*t),0.0f,0.0f);
  return normalize( to_float3(
           mapTerrain(pos+swi3(eps,x,y,y),iChannel1) - mapTerrain(pos-swi3(eps,x,y,y),iChannel1),
           mapTerrain(pos+swi3(eps,y,x,y),iChannel1) - mapTerrain(pos-swi3(eps,y,x,y),iChannel1),
           mapTerrain(pos+swi3(eps,y,y,x),iChannel1) - mapTerrain(pos-swi3(eps,y,y,x),iChannel1) ) );

}

// Perturb the normal using a texture to add more details.
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tex, in float3 p, in float3 nor, float bumpfactor){
   
    const float eps = 0.001f;
    float ref = getGrey(swi3(texcube(tex,  p , nor),x,y,z));                 
    float3 grad = to_float3( getGrey(swi3(texcube(tex, to_float3(p.x - eps, p.y, p.z), nor),x,y,z)) - ref,
                             getGrey(swi3(texcube(tex, to_float3(p.x, p.y - eps, p.z), nor),x,y,z)) - ref,
                             getGrey(swi3(texcube(tex, to_float3(p.x, p.y, p.z - eps), nor),x,y,z)) - ref )/eps;
             
    // get the gradient's projection to the plane.
    grad -= nor*dot(nor, grad);          
                      
    return normalize(nor + grad*bumpfactor);  
}


// =================================== Light ===================================

/**
 * Look for the closest object along the given ray.
 * The closer this object is to the ray, and the quicker we get close to it,
 * we expect the shadow to be harder.
 * See Inigo Quilez tutorial - https://iquilezles.org/articles/rmshadows/
 */
__DEVICE__ float softshadow( in float3 rayOrigin, in float3 rayDirection, float mint, float maxt, float k, __TEXTURE2D__ iChannel1 )
{
    float res = 1.0f;
    float t = mint;
    for( int i=0; i<100; i++ )
    {
        float h = mapTerrain(rayOrigin + rayDirection*t,iChannel1);
        res = _fminf( res, k*_fmaxf(h,0.0f)/t );
        t += clamp( h, 0.01f, 0.4f );
    if( res<0.001f || t>=maxt) break;
    }
    return clamp(res,0.0f,1.0f);
}

// Compute light value at the given point.
__DEVICE__ float lightValue(float3 rayOrigin, float3 position, float3 normal, float3 lightPosition, __TEXTURE2D__ iChannel1){
    // diffuse
    float3 lightDir = lightPosition - position;
    
    float diffuseFactor = clamp(dot(lightDir, normal), 0.0f, 1.0f);
    float shadow = softshadow(position, normalize(lightDir), 0.0f, length(lightDir), 2.0f,iChannel1); 
    
    return shadow * diffuseFactor / _powf(length(lightDir),2.5f);
}

__DEVICE__ float wisp(float3 rayOrigin, float3 rayDirection, float hitDistance, float3 lightPosition){
    float3 lightDirection = lightPosition - rayOrigin;
    float lightValue = _powf(dot(normalize(lightDirection),normalize(rayDirection)),2000.0f); // maybe add flickering?
    float distanceToLight = length(lightDirection);
    // return the light value if it is before the cave wall.
    return step(distanceToLight, hitDistance) * lightValue;
}

// =================================== Main ===================================

__KERNEL__ void CaveExplorationJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(WallColor, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR1(MossColor, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER1(Water, -10.0f, 10.0f, -1.0f);
    CONNECT_SLIDER2(Bump, -10.0f, 10.0f, 0.005f);

    // normalize coordinates.
    float2 q = fragCoord / iResolution;
    float2 p = -1.0f + 2.0f*q;
    p.x *= iResolution.x / iResolution.y;
    
    float time = 3.8f + 0.6f * iTime;
    float3 rayOrigin = path(time);
    // look further a head, and move head left and right along the way to see the scenery.
    float3 target = path(time+1.6f)+to_float3((_sinf(time)+_cosf(time/2.0f))/2.0f,0,0); 
    
    target += to_float3_aw(ViewXY,ViewZ);
    
    // light moves before us
    float3 lightPosition = path( time+2.5f + _cosf(time) + _sinf(1.2f*time));
    
  // camera to world transform        
    float3 forward, right, up;
    camera(rayOrigin, target, &forward, &right, &up);
    float3 rayDirection = normalize(2.0f*forward + right*p.x + up*p.y);
    
    float3 col = to_float3_s(0.0f);
    float t = rayMarch(rayOrigin, rayDirection,iChannel1);
    if (t>0.0f){
        float3 position = rayOrigin + t * rayDirection;
        float3 normal = calcNormal(position, t,iChannel1);
        normal = doBumpMap(iChannel0 ,position, normal,  Bump);//0.005f);
        
        float3 wall_col = swi3(texcube( iChannel0, 0.5f*position, normal ),x,y,z);
        wall_col += swi3(WallColor,x,y,z)-0.5f;
        
        float3 moss_color = moss(position, normal,iChannel1);
        moss_color += swi3(MossColor,x,y,z)-0.5f;
        
        float lValue = lightValue(rayOrigin, position, normal, lightPosition, iChannel1);
        wall_col *= lValue;        
        moss_color *= clamp(lValue, 0.0f, 3.0f); // moss a bit less shiney, I think...
        
        // add more moss when normal is pointing up
        col = _mix(wall_col, moss_color, smoothstep(0.1f, 0.7f, normal.y)); 
        
        // add water
        if(position.y < Water){ //-1.0f) {
           col.z += 0.005f;   // blue tint            
           col *= _powf(0.4f, position.y * position.y);    // darken when deep
        }
        
        col += wisp(rayOrigin, rayDirection, t, lightPosition);
    }    
    
    fragColor = to_float4_aw(col , 1.0f);    

  SetFragmentShaderComputedColor(fragColor);
}