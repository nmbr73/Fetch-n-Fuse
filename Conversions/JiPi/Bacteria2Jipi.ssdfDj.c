
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define OMEGA 1.57079632679f
#define PI 3.1415926538f
#define TAU 6.283185307179586f

#define MAX_UNIT 500
#define GLOBAL_SCALER 1.0f
#define LIFETIME 1.5f

#define ANTENNA_DISTANCE 14.0f
#define ANTENNA_ANGLE  (TAU / 8.0f)
#define MOVEMENT_SPEED 250.0f

#define PHEROMONE_DECAY 2.0f
#define PHEROMONE_POTENCY 0.06f
#define PHEROMONE_RADIUS 10.0f
#define FRAME_COUNT 3

//float2 R;

struct Unit
{
    float2 position;
    float direction;
    float life;
};

//noise see https://www.shadertoy.com/view/ltB3zD
__DEVICE__ float random (float2 st) 
{
    return fract(_sinf(dot(swi2(st,x,y), to_float2(12.9898f,78.233f)))*43758.5453123f);
}

__DEVICE__ int2 getUnitUV(int index, int frame, float2 iResolution, __TEXTURE2D__ sampler)
{
    frame = (frame + FRAME_COUNT) % FRAME_COUNT;
    
    int frameOffset = frame * MAX_UNIT;
    index += frameOffset;
    
    //int width = textureSize(sampler, 0).x;
	int width = (int)iResolution.x;
    return to_int2(index % width, (int)(index / width));
}

__DEVICE__ int getUnitIndex(int2 position, float2 iResolution, __TEXTURE2D__ sampler)
{
    //int width = textureSize(sampler, 0).x;
	int width = (int)iResolution.x;
    return (position.y * width + position.x) % MAX_UNIT;
}

__DEVICE__ struct Unit readUnit(int index, int frame, float2 iResolution, in __TEXTURE2D__ sampler)
{
    //vec4 current = texelFetch(sampler, getUnitUV(index, frame, iResolution, sampler), 0);
    float4 current = texture(sampler, (to_float2_cint(getUnitUV(index, frame, iResolution, sampler))+0.5f)/iResolution);
    
    struct Unit unit;
    unit.position.x = current.x;
    unit.position.y = current.y;
    unit.direction = current.z;
    unit.life = current.w;
    return unit;
}

__DEVICE__ float4 writeUnit(in struct Unit unit)
{
    float4 packed;
    packed.x = unit.position.x;
    packed.y = unit.position.y;
    packed.z = unit.direction;
    packed.w = unit.life;
    return packed;
}

__DEVICE__ float distanceToLine(float2 position, float2 a, float2 b)
{
    // https://iquilezles.org/articles/distfunctions
    float2 ap = position - a;
    float2 ab = b - a;

    float clamped_projection = clamp(dot(ap,ab) / dot(ab,ab), 0.0f, 1.0f);
    
    return length(ap - ab * clamped_projection);
}

__DEVICE__ float2 calculatePosition(float2 position, float direction, float distance, in __TEXTURE2D__ sampler)
{
    float2 result = position + to_float2(
											_cosf(direction) * distance, 
											_sinf(direction) * distance
										);
    return result;
}

__DEVICE__ float readPheromone(float2 position, float2 iResolution, in __TEXTURE2D__ sampler)
{
    //return texelFetch(sampler, to_int2(swi2(position,x,y)), 0).r;
    return texture(sampler, (to_float2((int)position.x,(int)position.y)+0.5f)/iResolution).x;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1


// Unit data buffer
__KERNEL__ void Bacteria2JipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
	CONNECT_SLIDER0(StartX, -20.0f, 20.0f, 0.0f);
	CONNECT_SLIDER1(StartY, -20.0f, 20.0f, 0.0f);

    fragCoord+=0.5f;

    int2 cellIndex = to_int2_cfloat(fragCoord);
    int index = getUnitIndex(cellIndex, iResolution, iChannel0);
    
    struct Unit unit = readUnit(index, iFrame - 1, iResolution, iChannel0);
    
    // Make sure that we _have_ to update the unit
    //if(getUnitUV(index, iFrame, iResolution, iChannel0) != cellIndex)
	if(getUnitUV(index, iFrame, iResolution, iChannel0).x != cellIndex.x || getUnitUV(index, iFrame, iResolution, iChannel0).y != cellIndex.y)
    {
        //discard;
//        fragColor = texture(iChannel0, fragCoord/iResolution);
//		SetFragmentShaderComputedColor(fragColor);
//		return;
    }
    
    
    // generate ants
    // -0.1f is a hack, unit needs to be dead for the FRAME_COUNT duration, or we will have star wars effects
    if(unit.life < -0.1f || iFrame < FRAME_COUNT)
    {
        int2 size = to_int2_cfloat(iResolution);//textureSize(iChannel0, 0);
        unit.position.x = random(fragCoord + 5.0f + StartX) * (float)(size.x);
        unit.position.y = random(fragCoord + 2.0f + StartY) * (float)(size.y);
        unit.direction = random(fragCoord) * TAU;
        unit.life = mod_f(random(fragCoord * iTime), LIFETIME);

        unit.position.x = 0.0f + StartX;
        unit.position.y = 0.0f + StartY;

    }

    float dist = ANTENNA_DISTANCE;
    float dirOff = ANTENNA_ANGLE;
    
    float2 antennaLeft = calculatePosition(unit.position, unit.direction - ANTENNA_ANGLE, ANTENNA_DISTANCE, iChannel1);
    float2 antennaRight = calculatePosition(unit.position, unit.direction + ANTENNA_ANGLE, ANTENNA_DISTANCE, iChannel1);

    float a = readPheromone(antennaLeft, iResolution, iChannel1);
    float b = readPheromone(antennaRight, iResolution, iChannel1);

    float2 leftRight = normalize(to_float2(a , b));

    float dir = ((-a + b) + 1.0f) / 2.0f;
    
    // add life, according to detected pheromone
    // this should make denser populations
    unit.life += readPheromone(unit.position, iResolution, iChannel1) * iTimeDelta;

    unit.direction += _mix(-dirOff, dirOff, dir);
    unit.direction = mod_f(unit.direction + TAU, TAU);

    unit.position = calculatePosition(unit.position, unit.direction, MOVEMENT_SPEED * iTimeDelta, iChannel1);
    unit.life -= iTimeDelta;
    fragColor = writeUnit(unit);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// Pheromone buffer
__KERNEL__ void Bacteria2JipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    // Data is not warm yet, theres buggy data in frames, and that we do not want to draw
    if(iFrame < FRAME_COUNT * 2)
    {
        //discard;
		fragColor = texture(iChannel0, fragCoord/iResolution);
		SetFragmentShaderComputedColor(fragColor);
		return;
    }
    // get the current pixel
    //float current = texelFetch(iChannel0, to_int2(swi2(gl_FragCoord,x,y)), 0).r;
	float current = texture(iChannel0, (to_float2_cint(to_int2_cfloat(fragCoord))+0.5f)/iResolution).x;

    current -= PHEROMONE_DECAY * iTimeDelta; // decay

    current = _fmaxf(current, 0.0f);
    float2 imageSize = iResolution; //to_float2(textureSize(iChannel0, 0));
    
    for(int i=0 ; i < MAX_UNIT ; ++i)
    {
        struct Unit now = readUnit(i, iFrame, iResolution, iChannel1);
        struct Unit prev = readUnit(i, iFrame-1, iResolution, iChannel1);
        
        if(now.life < 0.0f || prev.life < 0.0f)
        {
            continue;
        }

        float dist = distanceToLine(fragCoord, now.position, prev.position);
        current += _fmaxf(((PHEROMONE_RADIUS - dist) / PHEROMONE_RADIUS) * PHEROMONE_POTENCY,  0.0f);
    }
    
    fragColor = to_float4(current,0,0,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel1
// Connect Image 'Previsualization: Buffer B' to iChannel0


// See https://iquilezles.org/articles/palettes for more information
__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

__KERNEL__ void Bacteria2JipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float intensity = (smoothstep(0.0f, 2.0f, texture(iChannel0,fragCoord/iResolution).x) - 0.5f);

    
    float3 col = pal(intensity, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,0.7f,0.4f),to_float3(0.0f,0.15f,0.20f));
    //vec3 col = pal(intensity, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,1.0f,1.0f),to_float3(0.00f, 0.10f, 0.20f));
    
    fragColor = to_float4_aw(
        col,
        1.0f
                            );

  SetFragmentShaderComputedColor(fragColor);
}