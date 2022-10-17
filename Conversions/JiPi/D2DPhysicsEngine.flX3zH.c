
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define entity 1632
#define maxdt 0.002
#define gravity 9.81

#define ballRadius 0.01
#define ballMass 0.8
#define restitution 0.7
//const float invMass = 1.0f/ballMass;

#define texDimX 420
__DEVICE__ float4 getData(int i, float2 R, __TEXTURE2D__ tex){
    int2 coord = to_int2(i % texDimX, i / texDimX);
    return _tex2DVecN(tex, ((float)(coord.x)+0.5f)/R.x,((float)(coord.y)+0.5f)/R.y, 15);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Textur' to iChannel1


#define highp float

// from https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331
struct Circle
{
  float2 position;
  float2 velocity;
};

__DEVICE__ bool intersect(Circle A, Circle B){
    return dot(A.position - B.position, A.position - B.position) <= 4.0f*ballRadius*ballRadius;
}

__DEVICE__ void ResolveCollision(inout Circle *A, Circle B, float2 normal, bool wall)
{
  // Calculate relative velocity
  float2 rv = B.velocity - (*A).velocity;
 
  // Calculate relative velocity in terms of the normal direction
  float velAlongNormal = dot(rv, normal);
 
  // Do not resolve if velocities are separating
  if(velAlongNormal > 0.0f)
    return;
 
  // Calculate impulse scalar
  float j = - 0.5f * (1.0f +  restitution) * velAlongNormal;
  
  if(wall) j *= 2.0f;
  
  // Apply impulse
  (*A).velocity -= j * normal;
  return;
}

__DEVICE__ void PositionalCorrection(inout Circle *A, Circle B, float2 normal, float penetrationDepth, bool wall)
{
  const float percent = 0.1f;
  float2 correction = 0.5f * penetrationDepth * percent * normal;
  if(wall) correction *= 2.0f;
  
  (*A).position -= correction;
}

__KERNEL__ void D2DPhysicsEngineFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, float iTimeDelta, int iFrame, sampler2D iChannel0)
{

    CONNECT_CHECKBOX0(Textur, 0);

    float4 textur = to_float4_s(0.0f);

    int idx = (int)(fragCoord.x) + (int)(fragCoord.y)*(int)(texDimX);

//Textur funktioniert nicht !!!!!!!!!!!!!!!!!!!!!!!!
    if(Textur)
    {
      float _y = 0.0f;
      float _x = 0.1f + _modf((float)(idx)*0.03f, _y)*0.8f;
      float tex = texture(iChannel1, fragCoord/iResolution).w;
      if (tex != 0.0f) textur = to_float4(_x, 0.2f + _y*0.025f, 0.0f, -0.9f);
      
    }


    
    if( idx >= entity || fragCoord.x >= (float)(texDimX)){
        fragColor = to_float4_s(0.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    if( iFrame < 6 ){
        float _y = 0.0f;
        float _x = 0.1f + _modf((float)(idx)*0.03f, _y)*0.8f;
        
        if(Textur)
        {
          fragColor = textur;
        }
        else
          fragColor = to_float4(_x, 0.2f + _y*0.025f, 0.0f, -0.9f);
        
        SetFragmentShaderComputedColor(fragColor);
        return;    
    }
    float4 a = getData(idx, iResolution, iChannel0);
    Circle A = {swi2(a,x,y), swi2(a,z,w)};
    
    for(int i = 0; i < entity; i++){
        if(i != idx){
            float4 raw = getData(i, iResolution, iChannel0);
            Circle B = {swi2(raw,x,y), swi2(raw,z,w)};
            if(intersect(A, B)){
                float2 normal = normalize(B.position - A.position);
                ResolveCollision(&A, B, normal, false);
                
                float depth = distance_f2(A.position, B.position) - 2.0f * ballRadius;
                PositionalCorrection(&A, B, -normal, depth, false);
                
            }
        }
    }
    
    //collition with walls
    Circle Bfake =  {to_float2_s(0.0f), to_float2_s(0.0f)};
    
    if(A.position.x + ballRadius >= 1.0f){
        ResolveCollision(&A, Bfake, to_float2(1.0f, 0.0f), true);
        float depth = A.position.x + ballRadius - 1.0f;
        PositionalCorrection(&A, Bfake, to_float2(1.0f, 0.0f), depth, true);
    }
    if(A.position.x - ballRadius <= 0.0f){
        ResolveCollision(&A, Bfake, to_float2(-1.0f, 0.0f), true);
        float depth = -(A.position.x - ballRadius);
        PositionalCorrection(&A, Bfake, to_float2(-1.0f, 0.0f), depth, true);
    }
        
    if(A.position.y + ballRadius >= 1.0f){
        ResolveCollision(&A, Bfake, to_float2(0.0f, 1.0f), true);
        float depth = A.position.y + ballRadius - 1.0f;
        PositionalCorrection(&A, Bfake, to_float2(0.0f, 1.0f), depth, true);
    }
    if(A.position.y - ballRadius <= 0.0f){
        ResolveCollision(&A, Bfake, to_float2(0.0f, -1.0f), true);
        float depth = -(A.position.y - ballRadius);
        PositionalCorrection(&A, Bfake, to_float2(0.0f, -1.0f), depth, true);
    }
    

    
    float dt = _fminf(iTimeDelta, maxdt);
    A.velocity += to_float2(0.0f, -gravity) * dt;
    A.position += A.velocity * dt;
    
    if(mod_f((float)(iFrame), 700.0f)<=0.01f) A.velocity = to_float2(-(A.position.x - 0.5f)*2.0f, 3.0f);
    
    
    fragColor = to_float4_f2f2(A.position, A.velocity);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void D2DPhysicsEngineFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    
    float2 uv = fragCoord/iResolution;
    uv -= 0.5f;
    uv.x *= iResolution.x/iResolution.y;
    uv += 0.5f;

    float3 col = to_float3_s(0.1f);
    
    col = to_float3_s(0.1f);
    float touch = 0.0f;;
    float3 col2 = to_float3(0.0f, 0.0f, 0.5f);
    for(int i = 0; i < entity; i++){
        float4 ball = getData(i, iResolution, iChannel0);
        if (ballRadius*ballRadius*1.8f > dot(uv - swi2(ball,x,y), uv - swi2(ball,x,y))) {
            touch += 1.0f;
            swi2S(col2,x,y,  _mix(swi2(col2,x,y), normalize(swi2(ball,z,w))*0.5f + 0.5f, 1.0f/touch));
        }
    }
    if(touch != 0.0f){
        col = col2;
    }
    
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}