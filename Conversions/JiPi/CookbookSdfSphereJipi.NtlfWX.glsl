

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// 09.11.2021.
// Made by Darko Supe (omegasbk)

#define MAX_STEPS 100

#define MAX_DIST 100.
#define MIN_DIST 0.0002

float sdfSphere(vec3 c, float r, vec3 p)
{
    return distance(p, c) - r + texture(iChannel0, p.xy).r / ((sin(iTime) + 1.) * 80.);
}

float getDist(vec3 p)
{
    // Setup scene
    return sdfSphere(vec3(0.), 0.8, p);
}

float rayMarch(vec3 ro, vec3 rd)
{
    float dist = 0.;
    
    for (int i = 0; i < MAX_STEPS; i++)
    {
        vec3 itPos = ro + rd * dist;
        float itDist = getDist(itPos);
        
        dist += itDist;
        
        if (dist > MAX_DIST || dist < MIN_DIST)  
            break;
    }    
    
    return dist;
}

vec3 getNormal(vec3 p)
{
    vec2 e = vec2(0.01, 0.);    
    return normalize(vec3(getDist(p + e.xyy), getDist(p + e.yxy), getDist(p + e.yyx)));    
}

float getLight(vec3 p)
{
    vec3 lightPos = vec3(sin(iTime * 3.), 3., -2.2);
    vec3 lightDir = normalize(p - lightPos);
    
    return -dot(getNormal(p), lightDir);    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{    
    vec2 uv = fragCoord/iResolution.xy - 0.5;
    uv.x *= iResolution.x / iResolution.y;
    
    float focalDist = 0.6;
    vec3 ro = vec3(0., 0., -1.6);
    vec3 rd = vec3(uv.x, uv.y, focalDist);   
    
    vec3 col = vec3(0.);
    
    float dist = rayMarch(ro, rd);
    if (dist < MAX_DIST)
    {
        vec3 pHit = ro + rd * dist;
        col = vec3(0.5, 0.6, 0.6);
        col *= vec3(getLight(pHit)) + vec3(0.1);
        
    }    

    // Output to screen
    fragColor = vec4(col,1.0);
}