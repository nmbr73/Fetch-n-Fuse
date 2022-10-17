

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    vec2 uv = fragCoord/iResolution.xy;
    uv -= 0.5;
    uv.x *= iResolution.x/iResolution.y;
    uv += 0.5;

    vec3 col = vec3(0.1);
    
    col = vec3(0.1);
    float touch = 0.0;;
    vec3 col2 = vec3(0.0, 0.0, 0.5);
    for(int i = 0; i < entity; i++){
        vec4 ball = getData(i, iChannel0);
        if (ballRadius*ballRadius*1.8 > dot(uv - ball.xy, uv - ball.xy)) {
            touch += 1.0;
            col2.xy = mix(col2.xy, normalize(ball.zw)*0.5 + 0.5, 1.0/touch);
        }
    }
    if(touch != 0.0){
        col = col2;
    }
    



    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define highp float

// from https://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-the-basics-and-impulse-resolution--gamedev-6331
struct Circle
{
  vec2 position;
  vec2 velocity;
};

bool intersect(Circle A, Circle B){
    return dot(A.position - B.position, A.position - B.position) <= 4.0*ballRadius*ballRadius;
}

void ResolveCollision(inout Circle A, Circle B, vec2 normal, bool wall)
{
  // Calculate relative velocity
  vec2 rv = B.velocity - A.velocity;
 
  // Calculate relative velocity in terms of the normal direction
  float velAlongNormal = dot(rv, normal);
 
  // Do not resolve if velocities are separating
  if(velAlongNormal > 0.0)
    return;
 
  // Calculate impulse scalar
  float j = - 0.5 * (1.0 +  restitution) * velAlongNormal;
  
  if(wall) j *= 2.0;
  
  // Apply impulse
  A.velocity -= j * normal;
  return;
}

void PositionalCorrection(inout Circle A, Circle B, vec2 normal, float penetrationDepth, bool wall)
{
  const float percent = 0.1;
  vec2 correction = 0.5 * penetrationDepth * percent * normal;
  if(wall) correction *= 2.0;
  
  A.position -= correction;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    int idx = int(fragCoord.x) + int(fragCoord.y)*int(texDimX);
    if( idx >= entity || fragCoord.x >= float(texDimX)){
        fragColor = vec4(0.0);
        return;
    }
    if( iFrame < 6 ){
        float y;
        float x = 0.1 + modf(float(idx)*0.03, y)*0.8;
        fragColor = vec4(x, 0.2 + y*0.025, 0.0, -0.9);
        return;    
    }
    vec4 a = getData(idx, iChannel0);
    Circle A = Circle(a.xy, a.zw);
    
    for(int i = 0; i < entity; i++){
        if(i != idx){
            vec4 raw = getData(i, iChannel0);
            Circle B = Circle(raw.xy, raw.zw);
            if(intersect(A, B)){
                vec2 normal = normalize(B.position - A.position);
                ResolveCollision(A, B, normal, false);
                
                float depth = distance(A.position, B.position) - 2.0 * ballRadius;
                PositionalCorrection(A, B, -normal, depth, false);
                
            }
        }
    }
    
    //collition with walls
    Circle Bfake =  Circle(vec2(0.0), vec2(0.0));
    
    if(A.position.x + ballRadius >= 1.0){
        ResolveCollision(A, Bfake, vec2(1.0, 0.0), true);
        float depth = A.position.x + ballRadius - 1.0;
        PositionalCorrection(A, Bfake, vec2(1.0, 0.0), depth, true);
    }
    if(A.position.x - ballRadius <= 0.0){
        ResolveCollision(A, Bfake, vec2(-1.0, 0.0), true);
        float depth = -(A.position.x - ballRadius);
        PositionalCorrection(A, Bfake, vec2(-1.0, 0.0), depth, true);
    }
        
    if(A.position.y + ballRadius >= 1.0){
        ResolveCollision(A, Bfake, vec2(0.0, 1.0), true);
        float depth = A.position.y + ballRadius - 1.0;
        PositionalCorrection(A, Bfake, vec2(0.0, 1.0), depth, true);
    }
    if(A.position.y - ballRadius <= 0.0){
        ResolveCollision(A, Bfake, vec2(0.0, -1.0), true);
        float depth = -(A.position.y - ballRadius);
        PositionalCorrection(A, Bfake, vec2(0.0, -1.0), depth, true);
    }
    
    
    
    float dt = min(iTimeDelta, maxdt);
    A.velocity += vec2(0.0, -gravity) * dt;
    A.position += A.velocity * dt;
    
    if(mod(float(iFrame), 700.0)<=0.01)A.velocity = vec2(-(A.position.x - 0.5)*2.0, 3.0);
    
    
    fragColor = vec4(A.position, A.velocity);
}








// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define entity 1632
#define maxdt 0.002
#define gravity 9.81

#define ballRadius 0.01
#define ballMass 0.8
#define restitution 0.7
const float invMass = 1.0/ballMass;

#define texDimX 420
vec4 getData(int i, sampler2D tex){
    ivec2 coord = ivec2(i % texDimX, i / texDimX);
    return texelFetch(tex, coord, 0);
}
