

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// https://www.shadertoy.com/view/ssBczR
// based on https://www.shadertoy.com/view/sdscDs insulate by jt
// wrap 2d SDF around a torus - less exact than plane version

#define EPSILON 0.001
#define DIST_MAX 50.0

// https://iquilezles.org/www/articles/distfunctions2d/distfunctions2d.htm
float halfspace(vec3 p, float d)
{
    return p.z - d;
}

float torus(vec3 p, vec2 t)
{
    vec2 q = vec2(length(p.xz) - t.x, p.y);
    return length(q) - t.y;
}

float box2d(vec2 p)
{
    vec2 d = abs(p) - 1.0;
    return min(max(d.x, d.y),0.0) + length(max(d, 0.0));
}

float circle2d(vec2 p, float r)
{
    return length(p) - r;
}

float segment2d(vec2 p, vec2 a, vec2 b)
{
    vec2 pa = p - a, ba = b - a;
    float h = clamp(dot(pa, ba) / dot(ba, ba), 0.0, 1.0);
    return length(pa - ba * h);
}

float insulate_box(vec3 p)
{
    float dp = torus(p, vec2(1,0.5)); // distance to torus
    float d = box2d(p.xy); // distance to 2d SDF
    return sqrt(dp*dp+d*d); // 3dify 2d SDF
}

float insulate_boxes(vec3 p)
{
    float dp = torus(p, vec2(1,0.5)); // distance to torus
    float d = abs(abs(box2d(p.xy)) - 0.5) - 0.25; // distance to 2d SDF
    return sqrt(dp*dp+d*d); // 3dify 2d SDF
}

float insulate_circle(vec3 p)
{
    float dp = torus(p, vec2(1,0.5)); // distance to torus
    float d = circle2d(p.xy, 1.0); // distance to 2d SDF
    return sqrt(dp*dp+d*d); // 3dify 2d SDF
}

float insulate_circles(vec3 p)
{
    float dp = torus(p, vec2(1,0.5)); // distance to torus
    float d = abs(abs(circle2d(p.xy, 1.0)) - 0.5) - 0.25; // distance to 2d SDF
    return sqrt(dp*dp+d*d); // 3dify 2d SDF
}

float insulate_segment(vec3 p)
{
    float dp = torus(p, vec2(1,0.5)); // distance to torus
    float d = segment2d(p.xy, vec2(-1.5), vec2(+1.5)); // distance to 2d SDF
    return sqrt(dp*dp+d*d); // 3dify 2d SDF
}

float map(vec3 p)
{
    float d = mix(0.01, 0.1, 0.5 + 0.5 * cos(iTime));
    return
        min
        (
            min
            (
                min
                (
                    insulate_circles(p) - d,
                    insulate_boxes(p) - d
                ),
                insulate_segment(p) - d
            ),
            halfspace(p, -1.2)
        );
}

// https://iquilezles.org/www/articles/normalsSDF/normalsSDF.htm tetrahedron normals
vec3 normal(vec3 p)
{
    const float h = EPSILON;
    const vec2 k = vec2(1,-1);
    return
        normalize
        (
            k.xyy * map(p + k.xyy*h)
            +
            k.yyx * map(p + k.yyx*h)
            +
            k.yxy * map(p + k.yxy*h)
            +
            k.xxx * map(p + k.xxx*h)
        );
}

float trace(vec3 ro, vec3 rd)
{
    for(float t = 0.0; t < DIST_MAX;)
    {
        float h = map(ro + rd * t);
        if(h < EPSILON)
            return t;
        t += h * 0.5; // NOTE: due to inexact SDF step must be reduced
    }
    return DIST_MAX;
}

// https://iquilezles.org/www/articles/rmshadows/rmshadows.htm
float shadow( in vec3 ro, in vec3 rd, float mint, float maxt )
{
    for( float t=mint; t<maxt; )
    {
        float h = map(ro + rd*t);
        if( h<EPSILON )
            return 0.0;
        t += h * 0.5; // NOTE: due to inexact SDF step must be reduced
    }
    return 1.0;
}

// https://www.shadertoy.com/view/Xds3zN raymarching primitives
float calcAO( in vec3 pos, in vec3 nor )
{
    float occ = 0.0;
    float sca = 1.0;
    for( int i=0; i<5; i++ )
    {
        float h = 0.01 + 0.12*float(i)/4.0;
        float d = map( pos + h*nor );
        occ += (h-d)*sca;
        sca *= 0.95;
        if( occ>0.35 ) break;
    }
    return clamp( 1.0 - 3.0*occ, 0.0, 1.0 ) ;
}

#define pi 3.1415926

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord.xy / iResolution.xy;
    vec2 ndc = 2.0 * uv - 1.0;
    ndc.x *= float(iResolution.x) / float(iResolution.y);
    float mx = 2.0 * pi * float(iMouse.x) / float(iResolution.x);
    float my = pi / 2.0 + pi / 2.0 * float(iMouse.y) / float(iResolution.y);
    mx = (iMouse.x != 0.0) ? mx : 2.0 * pi * fract(iTime * 0.1);
    my = (iMouse.y != 0.0) ? my : pi * 3.0 / 4.0;;

    mat2 R = mat2(vec2(cos(mx), sin(mx)), vec2(-sin(mx), cos(mx)));
    vec3 ro = vec3(0.0, 0.0, -5.0 );//vec3(0.0, -10.0 * my, 0.0);
    //mat2 S = mat2(vec2(0.0, 1.0), vec2(-1.0, 0.0));
    mat2 S = mat2(vec2(cos(my), sin(my)), vec2(-sin(my), cos(my)));
    ro.yz=S*ro.yz;
    ro.xy = R * ro.xy;

    vec3 rd = normalize(vec3(0.5 * ndc.xy, 1.0)); // NOTE: omitting normalization results in clipped edges artifact
    rd.yz=S*rd.yz;
    rd.xy = R * rd.xy;

    float dist = trace(ro, rd);
    vec3 dst = ro + rd * dist;
    vec3 n = normal(dst);

    vec3 lightdir = normalize(vec3(1.0, 1.0, 1.0));
    vec3 ambient = vec3(0.4);
    float brightness = max(dot(lightdir, n), 0.0);
    brightness *= shadow(ro+rd*dist,lightdir, 0.01, DIST_MAX); // XXX artifacts on cylinder XXX
    vec3 color = vec3(1.0);
    color *= (n * 0.5 + 0.5);
    color = (ambient * calcAO(dst, n) + brightness) * color;

    fragColor = mix(vec4(color, 1.0), vec4(0.0), step(DIST_MAX, dist));
    fragColor = sqrt(fragColor); // approximate gamma
}
