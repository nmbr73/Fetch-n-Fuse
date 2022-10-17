

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define BALL_THINNESS 900.0
#define BALL_DROPOFF_EXPONENT 2.1

#define SHADOW_DROPOFF_EXPONENT 2.0

#define MOUSE_MAX_DIST 100.0
#define MOUSE_DROPOFF_EXPONENT 2.0


//Gets the "strength" of a ball the given distance away from a point.
float getWeight(float distToBall)
{
    return pow(1.0 / distToBall, BALL_DROPOFF_EXPONENT);
}



//Be careful; the following code is duplicated in the other pass.
//Make sure it stays identical across both passes.
//---------------------------------------------------------------
#define N_BALLS 64
#define BALL_MAX_SPEED 1.0

#define ballPos(ballData) ballData.xy
#define ballVel(ballData) ballData.zw

//Gets the position in the "world" of the given pixel.
vec2 getPos(vec2 fragCoords, vec2 resolution)
{
    return fragCoords / min(resolution.x, resolution.y);
}

//Gets the given ball's position/velocity.
vec4 getBallData(float index)
{
    vec4 col = texture(iChannel0, vec2(index / iChannelResolution[0].x), 0.0);
    
    //Unpack the position/velocity.
    ballVel(col) = BALL_MAX_SPEED * (-1.0 + (2.0 * ballVel(col)));
    
    return col;
}
//---------------------------------------------------------------



vec3 getVoidColor(vec2 uv, float strength)
{
    vec3 tex = texture(iChannel2, uv).rgb;
    return tex * (1.0 - pow(strength / BALL_THINNESS, SHADOW_DROPOFF_EXPONENT));
}
vec3 getBallColor(float strength, vec3 avgNormal)
{
    return texture(iChannel3, avgNormal.xzy).rgb;
}
vec3 getMouseColorAdd(vec2 fragCoord)
{
    if (iMouse.z > 1.0)
    {
        float dist = min(MOUSE_MAX_DIST, distance(fragCoord, iMouse.xy)),
              distLerp = dist / MOUSE_MAX_DIST;
        return (1.0 - distLerp) *
            	vec3(0.5 + (0.5 * sin(distLerp * 5.0 + (iTime * 20.0))));
    }
    return vec3(0.0);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = getPos(fragCoord, iResolution.xy);
    float strength = 0.0;
    vec3 strengthDir = vec3(0.0);
    
    for (int i = 0; i < N_BALLS; ++i)
    {
        vec4 ballDat = getBallData(float(i));
        vec2 toBall = ballPos(ballDat) - pos;
        float dist = length(toBall),
              ballStrength = getWeight(dist);
        
        strength += ballStrength;
        strengthDir -= vec3(toBall, strength * .01);
    }
    strengthDir = normalize(strengthDir);
    
    vec3 outColor = mix(getVoidColor(fragCoord / iResolution.xy, strength),
                        getBallColor(strength, strengthDir),
                        step(BALL_THINNESS, strength));
    fragColor = vec4(outColor + getMouseColorAdd(fragCoord), 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
      This buffer contains the data for the balls. Each pixel is one ball.
      The data is stored along the bottom of the texture (uv.y == 0.0).
*/


//Influences the starting position/velocity of each ball.
#define SEED .6811

//The strength of the attraction towards the mouse.
#define MOUSE_STRENGTH 0.02
#define MOUSE_DROPOFF_EXPONENT 15.0

#define BALL_SLOWDOWN 0.997



//Gets the "strength" of a ball the given distance away from the mouse.
float getWeight(float distToBall)
{
    return MOUSE_STRENGTH * 1.0 / pow(distToBall, MOUSE_DROPOFF_EXPONENT);
}


//Be careful; the following code is duplicated in the other pass.
//Make sure it stays identical across both passes.
//---------------------------------------------------------------
#define N_BALLS 64
#define BALL_MAX_SPEED 1.0

#define ballPos(ballData) ballData.xy
#define ballVel(ballData) ballData.zw

//Gets the position in the "world" of the given pixel.
vec2 getPos(vec2 fragCoords, vec2 resolution)
{
    return fragCoords / min(resolution.x, resolution.y);
}

//Gets the given ball's position/velocity.
vec4 getBallData(float index)
{
    vec4 col = texture(iChannel0, vec2(index / iChannelResolution[0].x, 0.0));
    
    //Unpack the position/velocity.
    ballVel(col) = BALL_MAX_SPEED * (-1.0 + (2.0 * ballVel(col)));
    
    return col;
}
//---------------------------------------------------------------

#define N_BALLS_F float(N_BALLS)


//Packs up data for a ball into a texture value.
vec4 packBallData(vec2 pos, vec2 vel)
{
	vec4 outD;
    ballPos(outD) = pos;
    ballVel(outD) = 0.5 + (0.5 * (vel / BALL_MAX_SPEED));
    return outD;
}



void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    if (fragCoord.x > N_BALLS_F)
        discard;
    
    if (iFrame < 2)
    {
        vec2 uv = fragCoord / iChannelResolution[0].xy;
        uv += SEED;
            
        vec2 pos = texture(iChannel1, uv * 02.12312).yz,
             vel = texture(iChannel1, uv * 5.415234).zy;
        pos.x *= iChannelResolution[0].x / iChannelResolution[0].y;
        vel = BALL_MAX_SPEED * (-1.0 + (2.0 * vel));
            
        fragColor = packBallData(pos, vel);
    }
    else
    {
    	vec4 ballData = getBallData(fragCoord.x);
        vec2 pos = ballPos(ballData),
             vel = ballVel(ballData);
        	
        float aspectRatio = iChannelResolution[0].y / iChannelResolution[0].x;
        pos += vel * iTimeDelta * vec2(aspectRatio, 1.0);
        
            
        //Make sure the balls aren't outside the map.
        float invAspectRatio = 1.0 / aspectRatio;
        if (pos.x < 0.0)
        {
            pos.x = 0.0;
            vel.x = abs(vel.x);
        }
        if (pos.y < 0.0)
        {
			pos.y = 0.0;
            vel.y = abs(vel.y);
        }
        if (pos.x >= invAspectRatio)
        {
            pos.x = invAspectRatio;
            vel.x = -abs(vel.x);
        }
        if (pos.y >= 1.0)
        {
            pos.y = 1.0;
            vel.y = -abs(vel.y);
        }
            
        //Push the balls towards the mouse.
        if (iMouse.z > 1.0)
        {
            vec2 toBall = pos - getPos(iMouse.xy, iResolution.xy);
            vel += toBall * getWeight(length(toBall));
        }
            
        //Update velocity.
        vel *= BALL_SLOWDOWN;
        float speed = length(vel);
        if (speed > BALL_MAX_SPEED)
            vel = (vel / speed) * BALL_MAX_SPEED;
        
        //Jitter the position a bit.
        vec2 jitter = texture(iChannel3, 34.4234 * fragCoord / iChannelResolution[0].xy).rb;
        jitter *= iTime * 9856.1341;
        pos += vel * sin(jitter) * 0.005;
        
        fragColor = packBallData(pos, vel);
    }
}