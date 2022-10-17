

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// See https://iquilezles.org/articles/palettes for more information
vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float intensity = (smoothstep(0.0, 2.0, texture(iChannel0,fragCoord/iResolution.xy).r) - 0.5);
    
    
    vec3 col = pal(intensity, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,0.7,0.4),vec3(0.0,0.15,0.20));
    //vec3 col = pal(intensity, vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.00, 0.10, 0.20));
    
	fragColor = vec4(
        col,
        1.0
    );
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

#define OMEGA 1.57079632679
#define PI 3.1415926538
#define TAU 6.283185307179586

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

vec2 R;

struct Unit
{
    vec2 position;
    float direction;
    float life;
};

//noise see https://www.shadertoy.com/view/ltB3zD
float random (vec2 st) 
{
    return fract(sin(dot(st.xy, vec2(12.9898,78.233)))*43758.5453123);
}

ivec2 getUnitUV(int index, int frame, sampler2D sampler)
{
    frame = (frame + FRAME_COUNT) % FRAME_COUNT;
    
    int frameOffset = frame * MAX_UNIT;
    index += frameOffset;
    
    int width = textureSize(sampler, 0).x;
    return ivec2(index % width, int(index / width));
}

int getUnitIndex(ivec2 position, sampler2D sampler)
{
    int width = textureSize(sampler, 0).x;
    return (position.y * width + position.x) % MAX_UNIT;
}

Unit readUnit(int index, int frame, in sampler2D sampler)
{
    //vec4 current = texelFetch(sampler, getUnitUV(index, frame, sampler), 0);
    vec4 current = texture(sampler, (vec2(getUnitUV(index, frame, sampler))+0.5)/R);
    
    Unit unit;
    unit.position.x = current.x;
    unit.position.y = current.y;
    unit.direction = current.z;
    unit.life = current.w;
    return unit;
}

vec4 writeUnit(in Unit unit)
{
    vec4 packed;
    packed.x = unit.position.x;
    packed.y = unit.position.y;
    packed.z = unit.direction;
    packed.w = unit.life;
    return packed;
}

float distanceToLine(vec2 position, vec2 a, vec2 b)
{
    // https://iquilezles.org/articles/distfunctions
    vec2 ap = position - a;
    vec2 ab = b - a;
    
    float clamped_projection = clamp(dot(ap,ab) / dot(ab,ab), 0.0, 1.0);
    
    return length(ap - ab * clamped_projection);
}

vec2 calculatePosition(vec2 position, float direction, float distance, in sampler2D sampler)
{
    vec2 result = position + vec2(
        cos(direction) * distance, 
        sin(direction) * distance
    );
    return result;
}

float readPheromone(vec2 position, in sampler2D sampler)
{
    //return texelFetch(sampler, ivec2(position.xy), 0).r;
    return texture(sampler, (vec2(ivec2(position.xy))+0.5)/R).r;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Unit data buffer
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    R = iResolution.xy;

    ivec2 cellIndex = ivec2(fragCoord.xy);
    int index = getUnitIndex(cellIndex, iChannel0);
    
    Unit unit = readUnit(index, iFrame - 1, iChannel0);
    
    // Make sure that we _have_ to update the unit
    if(getUnitUV(index, iFrame, iChannel0) != cellIndex)
    {
        discard;
    }
    
    
    // generate ants
    // -0.1 is a hack, unit needs to be dead for the FRAME_COUNT duration, or we will have star wars effects
    if(unit.life < -0.1f || iFrame < FRAME_COUNT)
    {
        ivec2 size = textureSize(iChannel0, 0);
        unit.position.x = random(fragCoord + 5.0) * float(size.x);
        unit.position.y = random(fragCoord + 2.0) * float(size.y);
        unit.direction = random(fragCoord) * TAU;
        unit.life = mod(random(fragCoord * iTime), LIFETIME);
    }

    float dist = ANTENNA_DISTANCE;
    float dirOff = ANTENNA_ANGLE;
    
    vec2 antennaLeft = calculatePosition(unit.position, unit.direction - ANTENNA_ANGLE, ANTENNA_DISTANCE, iChannel1);
    vec2 antennaRight = calculatePosition(unit.position, unit.direction + ANTENNA_ANGLE, ANTENNA_DISTANCE, iChannel1);

    float a = readPheromone(antennaLeft, iChannel1);
    float b = readPheromone(antennaRight, iChannel1);

    vec2 leftRight = normalize(vec2(a , b));

    float dir = ((-a + b) + 1.0) / 2.0;
    
    // add life, according to detected pheromone
    // this should make denser populations
    unit.life += readPheromone(unit.position, iChannel1) * iTimeDelta;

    unit.direction += mix(-dirOff, dirOff, dir);
    unit.direction = mod(unit.direction + TAU, TAU);

    unit.position = calculatePosition(unit.position, unit.direction, MOVEMENT_SPEED * iTimeDelta, iChannel1);
    unit.life -= iTimeDelta;
    fragColor = writeUnit(unit);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Pheromone buffer
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    R = iResolution.xy;

    // Data is not warm yet, theres buggy data in frames, and that we do not want to draw
    if(iFrame < FRAME_COUNT * 2)
    {
        discard;
    }
    // get the current pixel
    float current = texelFetch(iChannel0, ivec2(gl_FragCoord.xy), 0).r;
    current -= PHEROMONE_DECAY * iTimeDelta; // decay
    
    current = max(current, 0.0);
    vec2 imageSize = vec2(textureSize(iChannel0, 0));
    
    for(int i=0 ; i < MAX_UNIT ; ++i)
    {
        Unit now = readUnit(i, iFrame, iChannel1);
        Unit prev = readUnit(i, iFrame-1, iChannel1);
        
        if(now.life < 0.0f || prev.life < 0.0f)
        {
            continue;
        }

        float dist = distanceToLine(fragCoord, now.position, prev.position);
        current += max(((PHEROMONE_RADIUS - dist) / PHEROMONE_RADIUS) * PHEROMONE_POTENCY,  0.0);
    }
    
    fragColor = vec4(current,0,0,1.0);
}