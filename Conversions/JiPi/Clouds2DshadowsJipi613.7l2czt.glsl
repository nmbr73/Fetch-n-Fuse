

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_DIST 6000.0
#define MIN_DIST 0.01
#define STEPS 255

struct Object
{
    int id;
    float d;
};

Object minValue(Object a, Object b)
{
    if(a.d <= b.d)
    {
        return a;
    }
    return b;
}

vec3 rotateY(in vec3 p, in float angle, vec3 axis)
{
    vec3 result = p;
    result -= axis;

    float rnd = radians(angle);
    result = vec3(result.x * cos(rnd) - result.z * sin(rnd), result.y, result.x * sin(rnd) + result.z * cos(rnd));
    result += axis;
    return result;
}

Object sdPlane(in int id, in vec3 p, in vec3 normal, in float h)
{
    return Object(id, dot(p, normal) + h);
}

Object sdScene(in vec3 p)
{
    Object plane1 = sdPlane(0, p, vec3(0.0f, -1.0f, 0.0f), 3.0f);
    Object plane2 = sdPlane(1, p, vec3(0.0f, 1.0f, 0.0f), 1.0f);

    Object result = minValue(plane1, plane2);
    
    return result;
}

Object rayMarching(in vec3 ro, in vec3 rd)
{
    Object result;
    for(int i = 0; i < STEPS; ++i)
    {
        vec3 p = ro + rd * result.d;
        Object tmp = sdScene(p);
        result.d += tmp.d;
        result.id = tmp.id;
        
        if(tmp.d <= MIN_DIST || tmp.d >= MAX_DIST)
            break;
    }
    return result;
}

float saturate(in float a)
{
    return clamp(a, 0.0f, 1.0f);
}

float diffuseCloud(in vec3 p, in vec3 lightPos, in vec3 normal)
{
    vec3 lightDir = normalize(lightPos - p);
    
    return saturate(dot(lightDir, normal));
}

float diffuse(in vec3 p, in vec3 lightPos, in vec3 normal)
{
    vec3 lightDir = normalize(lightPos - p);
    
    vec3 p1 = p - normal * MIN_DIST;
    
    float result = saturate(dot(lightDir, normal));
    if(rayMarching(p1, lightDir).d <= length(lightPos - p))
    {
        result *= 0.2f;
    }
    
    return result;
}

vec3 calcCloudsShadow(in vec3 col, in float clouds)
{
    return mix(col, col * vec3(0.4f), clouds);
}

vec3 calcNormal(vec3 p)
{
    float e = 0.0005f;
    return normalize(
                    vec3(
                        sdScene(vec3(p.x + e, p.y, p.z)).d - sdScene(vec3(p.x - e, p.y, p.z)).d,
                        sdScene(vec3(p.x, p.y + e, p.z)).d - sdScene(vec3(p.x, p.y - e, p.z)).d,
                        sdScene(vec3(p.x, p.y, p.z + e)).d - sdScene(vec3(p.x, p.y, p.z - e)).d
                    ));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 lightPos = vec3(100.0f, 1000.0f, 0.0f);
    vec2 uv = (fragCoord - 0.5f * iResolution.xy)/iResolution.y;

    vec3 ro = vec3(0.0f, 1.0f, -3.0f);
    vec3 rd = normalize(vec3(uv, 1.0f));

    Object result = rayMarching(ro, rd);
    vec3 clouds;
    vec3 col = vec3(0.2f);
    
    if(result.d <= MAX_DIST)
    {
        vec3 p = ro + rd * result.d;
        vec4 clouds = texture(iChannel0, p.xz / 60.0f * 0.5f + 0.5f).rgba;
        if(result.id == 0)
        {        
            col = mix(vec3(1.0f), vec3(0.52734375f, 0.8046875f, 0.91796875), clouds.a) *  mix(diffuseCloud(p, lightPos, clouds.xyz), 1.0f, clouds.a);
        }
        else if(result.id == 1)
        {
            vec3 normal = calcNormal(p);
            col = texture(iChannel1, p.xz / 2.0f * 0.5f + 0.5f).rgb * mix(diffuse(p, lightPos, normal), 1.0f, clouds.a);
            
        }
    }

    col = mix(vec3(0.74296875f, 0.86953125f, 0.98234375f), col, exp(-result.d * 0.01f));
    // Output to screen
    fragColor = vec4(col, 1.0f);
    //fragColor = vec4(texture(iChannel0, uv * 0.5f + 0.5f).aaa,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define OCTAVES 5

float random(in vec3 p)
{
    return fract(sin(dot(p, vec3(12.345345f, 18.42545f, 32.454342f))) * 4321488.23432f);
}

float simplexNoise(in vec3 p)
{
    vec3 ip = floor(p);
    vec3 fp = fract(p);
    
    float a1 = random(ip);
    float a2 = random(ip + vec3(1.0f, 0.0f, 0.0f));
    float a3 = random(ip + vec3(0.0f, 1.0f, 0.0f));
    float a4 = random(ip + vec3(1.0f, 1.0f, 0.0f));
    
    float b1 = random(ip + vec3(0.0f, 0.0f, 1.0f));
    float b2 = random(ip + vec3(1.0f, 0.0f, 1.0f));
    float b3 = random(ip + vec3(0.0f, 1.0f, 1.0f));
    float b4 = random(ip + vec3(1.0f, 1.0f, 1.0f));
    
    
    vec3 u = fp * fp * (3.0f - 2.0f * fp);
    
    float x1 = mix(a1, a2, u.x);
    float x2 = mix(a3, a4, u.x);
    
    float y1 = mix(b1, b2, u.x);
    float y2 = mix(b3, b4, u.x);
    
    float r1 = mix(x1, x2, u.y);
    float r2 = mix(y1, y2, u.y);
    
    return mix(r1, r2, u.z); 
}

float fbm(in vec3 p)
{
    float result = 0.0f;
    float amp = 0.5f;
    for(int i = 0; i < OCTAVES; ++i)
    {
        float m = pow(2.0f, float(i));
        result += simplexNoise(p * m) * (1.0f / m);
    }
    return pow(result, 2.0f);
}

vec3 calcNormal(in vec3 p)
{
    vec3 p1 = p + vec3(1.0f, 0.0f, 0.0f);
    vec3 p2 = p + vec3(-1.0f, 0.0f, 0.0f);
    vec3 p3 = p + vec3(0.0f, 0.0f, 1.0f);
    vec3 p4 = p + vec3(0.0f, 0.0f, -1.0f);
    
    float a = fbm(p1);
    float b = fbm(p2);
    float c = fbm(p3);
    float d = fbm(p4);
   
    
    return normalize(vec3(a - b, 1.0f, c - d));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    uv *= 30.0f;
    
    vec3 p =vec3(uv + iTime * 0.08f, iTime * 0.1f);
    
    vec3 normal = calcNormal(p);
    
    fragColor = vec4(normal, fbm(p));
}