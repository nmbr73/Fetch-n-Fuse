

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// (C) Copyright 2021 by Yury Ershov

// https://en.wikipedia.org/wiki/Smoothed-particle_hydrodynamics

#define PALETTE        0    // 0=fire, 1=blue, 2=green

vec3 col(float x) {
  return vec3(
#if PALETTE == 0
      clamp(x, 0., 1./3.),
      clamp(x-1./3., 0., 1./3.),
      clamp(x-2./3., 0., 1./3.)
#elif PALETTE == 1
      clamp(x-2./3., 0., 1./3.),
      clamp(x-1./3., 0., 1./3.),
      clamp(x, 0., 1./3.)
#elif PALETTE == 2
      clamp(x-2./3., 0., 1./3.),
      clamp(x, 0., 1./3.),
      clamp(x-1./3., 0., 1./3.)
#endif
   ) * 3.;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    fragColor.a = 1.0;

	fragColor.rgb = col(log(texture(iChannel0,uv).a*10000.)/12.);
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// (C) Copyright 2021 by Yury Ershov

#define WALLS_BOUNCE   1

#define CH iChannel0

const float dt = 1.;
const float G = 0.01;
const float m_epsilon = 0.000001;
const float speed_rnd = 0.08;
const float center_gravity = 0.0005;

vec2 vclamp(vec2 v) {
    float l = length(v);
    return l < 10. ? v : v/l;
}

// https://www.shadertoy.com/view/4djSRW
float hash12(vec2 position)
{
    vec2 p = (position + mod(iTime, 200.) * 1500. + 50.0);
    vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float overlapping_area(vec2 l1, vec2 r1, vec2 l2, vec2 r2)
{
    float x_dist = min(r1.x, r2.x) - max(l1.x, l2.x);
    float y_dist = min(r1.y, r2.y) - max(l1.y, l2.y);
    return x_dist > 0. && y_dist > 0. ? x_dist * y_dist : 0.;
}

vec2 randomizespeed(vec2 v) {
    return vec2(v.x + (hash12(fract(v)*1573.32)-0.5)*speed_rnd, v.y + (hash12(fract(v)*178362.78)-0.5)*speed_rnd);
}


// Gas dynamics layer
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    if (fragCoord.x < 1.5 || fragCoord.y < 1.5 || fragCoord.x > iResolution.x-1.5 || fragCoord.y > iResolution.y-1.5) { fragColor=vec4(0.); return; }
    vec2 uv = fragCoord / iResolution.xy;

    if(iFrame < 2 || iMouse.z > 0.) {
        float r = distance(fragCoord, iResolution.xy/2.);
        if (r < 50.) {
            fragColor = vec4(0.0,-(uv.y - 0.5)*30./(r+1.)*50.,(uv.x - 0.5)*30./(r+1.)*50.,hash12(fragCoord)*5.*(52.-r)/20.);
        } else {
//            fragColor = vec4(0.0,0.0,0.0,hash12(fragCoord)*100.*m_epsilon);
            fragColor = vec4(0.0);
        }
        return;
    }

    vec2 s1 = vec2(1., 1.) / iResolution.xy;
    vec4 pt0 = texelFetch(iChannel0, ivec2(fragCoord), 0);
    vec2 speed0 = pt0.gb;
    float mass0 = pt0.a;
    vec2 f = vec2(0.);
    vec2 p = vec2(0.);
    float mass2 = 0.;

    vec2 sh;
    for (sh.y = -10.; sh.y < 10.5; sh.y+=1.) for (sh.x = -10.; sh.x < 10.5; sh.x+=1.) {
        float l = length(sh);
        vec2 coord1 = fragCoord + sh;
        vec4 pt1 = texelFetch(iChannel0, ivec2(coord1), 0);
        vec2 speed1 = pt1.gb;
        float mass1 = pt1.a;

        if (l > 3.5) {    // must be 0.5 but bigger number prevents from collapsing into 2x2 dots.
            // Gravity, acceleration
            f += sh/l * mass0*mass1/l/l;
        }

        // speed: mass transfer, impulse change:
        vec2 coord_next = coord1 + speed1 * dt;
        float overlap = overlapping_area(fragCoord, fragCoord+vec2(1.,1.), coord_next, coord_next+vec2(1.,1.));
        float dm = mass1 * overlap;
        mass2 += dm;
        p += speed1 * dm;
    }
    
    // Slight gravity towards the center to compensate the inability to feel further than 10 pts.
    vec2 to_c = iResolution.xy/2. - fragCoord;
    to_c = length(to_c) < 5. ? vec2(0.) : normalize(to_c)*center_gravity;

    fragColor.r = 0.;
    fragColor.a = mass2 > m_epsilon ? mass2 : 0.;
    fragColor.gb =
        mass2 > m_epsilon ?
        vclamp(randomizespeed(p/mass2 + dt * G*f/(mass0 > m_epsilon ? mass0 : mass2) + dt*to_c)) : vec2(0.);
#if WALLS_BOUNCE
    if (fragCoord.x <= 11.) {
        if (fragColor.g < 0.) fragColor.g = abs(fragColor.g)/2.;
    } else if (fragCoord.x >= iResolution.x - 12.) {
        if (fragColor.g > 0.) fragColor.g = -abs(fragColor.g)/2.;
    }
    if (fragCoord.y <= 11.) {
        if (fragColor.b < 0.) fragColor.b = abs(fragColor.b)/2.;
    } else if (fragCoord.y >= iResolution.y - 12.) {
        if (fragColor.b > 0.) fragColor.b = -abs(fragColor.b)/2.;
    }
#endif
}
