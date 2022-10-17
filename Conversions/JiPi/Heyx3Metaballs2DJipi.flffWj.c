
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Abstract 1' to iChannel1
// Connect Buffer A 'Texture: Abstract 3' to iChannel2

// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*
      This buffer contains the data for the balls. Each pixel is one ball.
      The data is stored along the bottom of the texture (uv.y == 0.0f).
*/


//Influences the starting position/velocity of each ball.
#define SEED 0.6811f

//The strength of the attraction towards the mouse.
#define MOUSE_STRENGTH 0.02f
#define MOUSE_DROPOFF_EXPONENT 15.0f

#define BALL_SLOWDOWN 0.997f



//Gets the "strength" of a ball the given distance away from the mouse.
__DEVICE__ float getWeight(float distToBall)
{
    return MOUSE_STRENGTH * 1.0f / _powf(distToBall, MOUSE_DROPOFF_EXPONENT);
}


//Be careful; the following code is duplicated in the other pass.
//Make sure it stays identical across both passes.
//---------------------------------------------------------------
#define N_BALLS 64
#define BALL_MAX_SPEED 1.0f

#define ballPos(ballData) swi2(ballData,x,y)
#define ballVel(ballData) swi2(ballData,z,w)

//Gets the position in the "world" of the given pixel.
__DEVICE__ float2 getPos(float2 fragCoords, float2 resolution)
{
    return fragCoords / _fminf(resolution.x, resolution.y);
}

//Gets the given ball's position/velocity.
__DEVICE__ float4 getBallData(float index, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float4 col = texture(iChannel0, to_float2(index / iResolution.x, 0.0f));
    
    //Unpack the position/velocity.
    swi2S(col,x,y, BALL_MAX_SPEED * (-1.0f + (2.0f * swi2(col,x,y)))); //Velocity
    
    return col;
}
//---------------------------------------------------------------

#define N_BALLS_F float(N_BALLS)


//Packs up data for a ball into a texture value.
__DEVICE__ float4 packBallData(float2 pos, float2 vel)
{
    float4 outD;
    //ballPos(outD) = pos;
    swi2S(outD,x,y, pos);
    
    //ballVel(outD) = 0.5f + (0.5f * (vel / BALL_MAX_SPEED));
    swi2S(outD,z,w, 0.5f + (0.5f * (vel / BALL_MAX_SPEED)));
    return outD;
}



__KERNEL__ void Heyx3Metaballs2DJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;

    if (fragCoord.x > N_BALLS_F)
    {  
      fragColor = texture(iChannel0, fragCoord/iResolution);
      SetFragmentShaderComputedColor(fragColor);
      return;
      //discard;
    }
    
    if (iFrame < 2 || Reset)
    {
        float2 uv = fragCoord / iResolution;
        uv += SEED;
            
        float2 pos = swi2(texture(iChannel1, uv * 02.12312f),y,z),
               vel = swi2(texture(iChannel1, uv * 5.415234f),z,y);
        pos.x *= iResolution.x / iResolution.y;
        vel = BALL_MAX_SPEED * (-1.0f + (2.0f * vel));
            
        fragColor = packBallData(pos, vel);
    }
    else
    {
      float4 ballData = getBallData(fragCoord.x,iResolution, iChannel0);
        float2 pos = ballPos(ballData),
               vel = ballVel(ballData);
          
        float aspectRatio = iResolution.y / iResolution.x;
        pos += vel * iTimeDelta * to_float2(aspectRatio, 1.0f);
        
            
        //Make sure the balls aren't outside the map.
        float invAspectRatio = 1.0f / aspectRatio;
        if (pos.x < 0.0f)
        {
            pos.x = 0.0f;
            vel.x = _fabs(vel.x);
        }
        if (pos.y < 0.0f)
        {
            pos.y = 0.0f;
            vel.y = _fabs(vel.y);
        }
        if (pos.x >= invAspectRatio)
        {
            pos.x = invAspectRatio;
            vel.x = -_fabs(vel.x);
        }
        if (pos.y >= 1.0f)
        {
            pos.y = 1.0f;
            vel.y = -_fabs(vel.y);
        }
            
        //Push the balls towards the mouse.
        if (iMouse.z > 1.0f)
        {
            float2 toBall = pos - getPos(swi2(iMouse,x,y), iResolution);
            vel += toBall * getWeight(length(toBall));
        }
            
        //Update velocity.
        vel *= BALL_SLOWDOWN;
        float speed = length(vel);
        if (speed > BALL_MAX_SPEED)
            vel = (vel / speed) * BALL_MAX_SPEED;
        
        //Jitter the position a bit.
        float2 jitter = swi2(texture(iChannel2, 34.4234f * fragCoord / iResolution),x,z);
        jitter *= iTime * 9856.1341f;
        pos += vel * sin_f2(jitter) * 0.005f;
        
        fragColor = packBallData(pos, vel);
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Texture: Abstract 1' to iChannel1
// Connect Image 'Texture: Abstract 3' to iChannel2
// Connect Image 'Texture: Wood' to iChannel3
// Connect Image 'Cubemap: Forest_0' to iChannel4


#define BALL_THINNESS 900.0f
#define BALL_DROPOFF_EXPONENT 2.1f

#define SHADOW_DROPOFF_EXPONENT 2.0f

#define MOUSE_MAX_DIST 100.0f
#define MOUSE_DROPOFF_EXPONENT 2.0f //!!! Anders als Buffer A


//Gets the "strength" of a ball the given distance away from a point.
__DEVICE__ float getWeightI(float distToBall)
{
    return _powf(1.0f / distToBall, BALL_DROPOFF_EXPONENT);
}



//Be careful; the following code is duplicated in the other pass.
//Make sure it stays identical across both passes.
//---------------------------------------------------------------
//#define N_BALLS 64
//#define BALL_MAX_SPEED 1.0f

//#define ballPos(ballData) swi2(ballData,x,y)
//#define ballVel(ballData) swi2(ballData,z,w)



//---------------------------------------------------------------



__DEVICE__ float3 getVoidColor(float2 uv, float strength, __TEXTURE2D__ iChannel3)
{
    float3 tex = swi3(_tex2DVecN(iChannel3,uv.x,uv.y,15),x,y,z);
    return tex * (1.0f - _powf(strength / BALL_THINNESS, SHADOW_DROPOFF_EXPONENT));
}
__DEVICE__ float3 getBallColor(float strength, float3 avgNormal, __TEXTURE2D__ iChannel4)
{
    return swi3(decube_f3(iChannel4, swi3(avgNormal,x,z,y)),x,y,z);
}
__DEVICE__ float3 getMouseColorAdd(float2 fragCoord, float4 iMouse, float iTime)
{
    if (iMouse.z > 1.0f)
    {
        float dist = _fminf(MOUSE_MAX_DIST, distance_f2(fragCoord, swi2(iMouse,x,y))),
              distLerp = dist / MOUSE_MAX_DIST;
        return (1.0f - distLerp) *
              to_float3_s(0.5f + (0.5f * _sinf(distLerp * 5.0f + (iTime * 20.0f))));
    }
    return to_float3_s(0.0f);
}


__KERNEL__ void Heyx3Metaballs2DJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float3 iChannelResolution[], sampler2D iChannel0, sampler2D iChannel2, sampler2D iChannel3)
{

    float2 pos = getPos(fragCoord, iResolution);
    float strength = 0.0f;
    float3 strengthDir = to_float3_s(0.0f);
    
    for (int i = 0; i < N_BALLS; ++i)
    {
        float4 ballDat = getBallData((float)(i),iResolution,iChannel0);
        float2 toBall = ballPos(ballDat) - pos;
        float dist = length(toBall),
              ballStrength = getWeightI(dist);
        
        strength += ballStrength;
        strengthDir -= to_float3_aw(toBall, strength * 0.01f);
    }
    strengthDir = normalize(strengthDir);
    
    float3 outColor = _mix(getVoidColor(fragCoord / iResolution, strength,iChannel3),
                           getBallColor(strength, strengthDir,iChannel4),
                           step(BALL_THINNESS, strength));
    fragColor = to_float4_aw(outColor + getMouseColorAdd(fragCoord,iMouse,iTime), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
