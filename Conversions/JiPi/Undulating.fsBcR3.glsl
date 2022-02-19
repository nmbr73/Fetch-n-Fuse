

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define AA 1

const float pi = 3.14159265358979323;

float pieceDist(vec3 p, float th, int n, float r, float rs)
{
    vec2 delta = vec2(sin(th), cos(th));
    float y = 0.;
    float d = dot(vec2(delta.y, -delta.x), vec2(length(p.xz) - r, p.y - y));
    float r0 = 0.;
    float l = length(p.xz);
    for(int i = 0; i < n; ++i)
    {
        r0 = (.07 + cos(float(i + n) + iTime / 2.) * .04) * rs;
        y += delta.y * r0;
        r += delta.x * r0;
        float td = length(vec2(l - r, p.y - y)) - r0;
        if((i & 1) == 0)
        	d = min(d, td);
        else
            d = max(d, -td);
        y += delta.y * r0;
        r += delta.x * r0;
    }
    return max(d, p.y - y);
}

vec4 piece(vec3 p, vec2 org, float th, int n, float r, float rs)
{
    return vec4(org.x, org.y, pieceDist(p - vec3(org.x, 0, org.y), th, n, r, rs), r);
}

vec4 u(vec4 a, vec4 b)
{
    return a.z < b.z ? a : b;
}

vec4 scene(vec3 p)
{
    vec4 res = vec4(0, 0, 1e4, 0);
    res = u(res, piece(p, vec2(0), -.2, 13, .5, 1.));
    res = u(res, piece(p, vec2(1.5, 0), -.0,9, .2, 1.));
    res = u(res, piece(p, vec2(-.7, -.9), -.0, 8, .3, 1.3));
    res = u(res, piece(p, vec2(-1.5, .1), -.5, 5, .8, 2.));
    res = u(res, piece(p, vec2(.5, .7), -.05, 12, .2, 1.));
    res.z = min(res.z, p.y);
    return res;
}

float map(vec3 p)
{
    return scene(p).z;
}

// Soft shadow for SDF, from IQ and Sebastian Aaltonen:
// https://www.shadertoy.com/view/lsKcDD
float calcSoftshadow( in vec3 ro, in vec3 rd, in float mint, in float tmax, int technique, float s )
{
    float res = 1.0;
    float t = mint;
    float ph = 1e10; // big, such that y = 0 on the first iteration

    for( int i=0; i<55; i++ )
    {
        float h = map( ro + rd*t );

        // traditional technique
        if( technique==0 )
        {
            res = min( res, s*h/t );
        }
        // improved technique
        else
        {
            // use this if you are getting artifact on the first iteration, or unroll the
            // first iteration out of the loop
            //float y = (i==0) ? 0.0 : h*h/(2.0*ph); 

            float y = h*h/(2.0*ph);
            float d = sqrt(h*h-y*y);
            res = min( res, s*d/max(0.0,t-y) );
            ph = h;
        }

        t += h;

        if( res<0.0001 || t>tmax ) break;

    }
    return clamp( res, 0.0, 1.0 );
}

// Forward-difference SDF gradients.
vec3 distG(vec3 p)
{
    vec2 e = vec2(1e-4, 0);
    return vec3(map(p + e.xyy), map(p + e.yxy), map(p + e.yyx)) -
        vec3(map(p - e.xyy), map(p - e.yxy), map(p - e.yyx));
}

void render( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy * 2. - 1.;
	uv.x *= iResolution.x / iResolution.y;
    
	vec3 ro = vec3(-.3, .8, 4.2), rd = normalize(vec3(uv, -3.));
    
    float t = 2.5;
    for(int i = 0; i < 110; ++i)
    {
        float d = map(ro + rd * t);
        if(abs(d) < 1e-4)
            break;
        if(t > 10.)
            break;
        t += d;
    }
    
    vec3 rp = ro + rd * t;
    
    vec3 n = normalize(distG(ro + rd * t));
    vec3 r = reflect(rd, n);
    vec3 ld = normalize(vec3(-1, 1, 1));
    float sh = calcSoftshadow(ro + rd * t, ld, 1e-2, 1e3, 0, 2.);
    float sh2 = calcSoftshadow(ro + rd * t, r, 1e-2, 1e3, 0, 10.);
    
    vec3 diff = .5 + .5 * cos(rp.y * vec3(3, 2, 5) * .5 + vec3(.6, 0, .6));
    
    vec4 sp = scene(rp);
    diff = mix(vec3(1), diff, smoothstep(.1, .12,abs(fract(.1 + atan(rp.z - sp.y, rp.x - sp.x) / pi * 5.) - .5)));
    
    if(abs(rp.y) < 1e-2 || t > 9.)
        diff = vec3(.5, .75, 1.) * smoothstep(-.1, .15, distance(rp.xz, sp.xy) - sp.a);
    
    fragColor.rgb = diff;
    
    fragColor.rgb *= mix(.5, 1., sh) * vec3(max(0., .6 + .4 * dot(n, ld)));
    
    float fr = pow(clamp(1. - dot(n, -rd), 0., 1.), 2.);
    
    fragColor.rgb += textureLod(iChannel0, r, 5.).rrr * fr * sh2;
    fragColor.rgb += smoothstep(.4, .5, dot(ld, r)) * fr * sh2 * 1.6;
    
    fragColor.rgb *= .85;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor.a = 1.;
    fragColor.rgb = vec3(0);
    
    // Anti-aliasing loop
    for(int y = 0; y < AA; ++y)
        for(int x = 0; x < AA; ++x)
        {
            vec4 rc;
            render(rc, fragCoord + vec2(x, y) / float(AA));
            fragColor.rgb += clamp(rc.rgb, 0., 1.);
        }
    
    fragColor.rgb /= float(AA * AA);
    fragColor.rgb /= (fragColor.rgb + 1.5)*.43;
    fragColor.rgb = pow(clamp(fragColor.rgb, 0., 1.), vec3(1. / 2.2)) +
        				texelFetch(iChannel1, ivec2(fragCoord) & 1023, 0).rgb / 200.;
}