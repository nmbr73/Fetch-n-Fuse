

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
Audio cube II - Visualiser - 
Variation of AudioCube by: kuvkar - 19th March, 2015 https://www.shadertoy.com/view/llBGR1
Auido Cube II by: uNiversal - 28th May, 2015
Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
*/
 
//Distance field functions from
//http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
/*-------------------------------------------------------------------------*/
float sdSphere( vec3 p, float s )
{
    return length(p)-s;
}

float sdTorus( vec3 p, vec2 t )
{
    vec2 q = vec2(length(p.xz)-t.x,p.y);
    return length(q)-t.y;
}

float udBox( vec3 p, vec3 b )
{
    return length(max(abs(p)-b,0.0));
}

float sdHexPrism( vec3 p, vec2 h )
{
    vec3 q = abs(p);
    return max(q.z-h.y,max((q.x*0.866025+q.y*0.5),q.y)-h.x);
}

float udRoundBox( vec3 p, vec3 b, float r )
{
    return length(max(abs(p)-b,0.0))-r;
}

float smin( float a, float b, float k )
{
    float res = exp( -k*a ) + exp( -k*b );
    return -log( res )/k;
}

float sdBox( vec3 p, vec3 b )
{
    vec3 d = abs(p) - b;
    return min(max(d.x,max(d.y,d.z)),0.0) +
           length(max(d,0.0));
}

/*-------------------------------------------------------------------------*/

#define PI 3.14159
#define TYPE_CUBE 1
#define TYPE_SPHERE 2
#define OBJECT_COUNT 2

mat3 rotx(float a) { mat3 rot; rot[0] = vec3(1.0, 0.0, 0.0); rot[1] = vec3(0.0, cos(a), -sin(a)); rot[2] = vec3(0.0, sin(a), cos(a)); return rot; }
mat3 roty(float a) { mat3 rot; rot[0] = vec3(cos(a), 0.0, sin(a)); rot[1] = vec3(0.0, 1.0, 0.0); rot[2] = vec3(-sin(a), 0.0, cos(a)); return rot; }

struct Object
{
    mat3 rot;
    vec3 pos;
    vec3 size;
    int type;
};

Object objects[OBJECT_COUNT];

float dist(vec3 position)
{
    float m = 9999.0;
    vec4 audio = texture(iChannel0, vec2(0.0, 0.0));
    vec2 uv = vec2(position.x, position.y) * 1.0;
    vec4 col = texture(iChannel0, uv).rgba;

    Object o = objects[0];
    vec3 p = position + o.pos;

    p = p * o.rot;
    float f = 0.0;

    float a = sdBox(p, o.size);
    float b = sdSphere(p, o.size.x);
    float au = audio.x;

    f = au * a + (1.0 - au) * b;
    f -= sdSphere(p, o.size.x) * (0.3 + au * 0.4);

    m = min(f, m);
    return m;
}

float toClipSpace(float f)
{
    return f * 2.0 - 1.0;
}

vec3 lookAt(vec3 from, vec3 to, vec3 dir)
{
    mat3 m;
    
    vec3 fwd = normalize(to - from);
    vec3 _up = vec3(0.0, 1.0, 0.0);
    vec3 r = cross(fwd, _up);
    vec3 u = cross(r, fwd);
    
    m[0] = r;
    m[1] = u;
    m[2] = fwd;
    vec3 d = m * dir;    
    d.z *= -1.0;
    return d;
}

bool trace(vec3 from, vec3 dir, out vec3 hitPosition, out float m)
{
    
    const int steps = 300;
    float step = 0.01;    
    vec3 rp = from;
    m = 99999.0;
    
    for (int i = 0; i < steps; ++i)
    {
        rp += dir * step;
        float sp = dist(rp);
        step  = sp;
        m = min(m, abs(sp));
        if (abs(sp) <= 0.001)
        {
            hitPosition = rp;
            return true;
        }
        
    }
    return false;
}

Object getObject(vec3 position, int type)
{
    Object c;
    c.pos = position;
    c.rot = mat3(1.0);
    c.type = type;
    c.size = vec3(0.1, 0.1, 0.1);
    return c;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = vec2(fragCoord.x / iResolution.x, fragCoord.y / iResolution.x);
    uv.y += (iResolution.y / iResolution.x) * 0.4;

    vec4 audio = texture(iChannel0, vec2(1.0, 0.0));

    objects[0] = getObject(vec3(0.0), TYPE_CUBE);
    objects[0].size = vec3(0.1, 0.1, 0.1);

    objects[0].rot *= rotx(iTime * 1.5);
    objects[0].rot *= roty(sin(iTime * 1.5));

    vec3 camPos = vec3(0.0, 0.0, 1.0);    
    vec3 lk = vec3(-toClipSpace(uv.x), -toClipSpace(uv.y), -2.0);
    vec3 mclip = vec3(0.0);
    vec3 dir = lookAt(camPos, mclip, normalize(lk));
    vec3 hit = vec3(0.0);
    vec4 color = vec4(0.0);

    float m;
    bool h = trace(camPos, dir, hit, m);
    vec4 aColor = vec4(1.0);

    float p =  pow(length(uv - vec2(0.5, 0.5)), 3.0);
    aColor.r = 0.5 + (sin((iTime + 100.0) * 0.25) * 0.5);
    aColor.g = 0.5 + (cos((iTime + 300.0) * 0.15) * 0.5);

    if(h)
    {
        vec2 offset = vec2(0.001, 0.0);
        vec3 grad = normalize(vec3(dist(hit + offset.xyy) - dist(hit - offset.xyy), 
                                   dist(hit + offset.yxy) - dist(hit - offset.yxy),
                                   dist(hit + offset.yyx) - dist(hit - offset.yyx)));
        
        float d = clamp(dot(grad, vec3(0.0, 0.0, 1.0)), 0.0, 1.0);
        float rim = (1.0 - d) * 1.4;        
        color += aColor * pow(rim, 2.0);

    }
    else
    {
        color = aColor * pow(length(uv - vec2(0.5, 0.5)) * (1.0 + p * audio.x * 5.0), 1.4) * 1.5;
    }

    float d = (1.0 - m * 75.0) * 15.0 * p;
    color += aColor * pow(clamp(d, 0.0, 0.0), 1.2);

    fragColor = color;
}
