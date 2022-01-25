

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define MAX_STEPS 100
#define MAX_DIST 50.
#define SURF_DIST .005

const int mat_support = 1;
const int mat_bar = 2;
const int mat_ball = 3;
const int mat_line = 4;


mat2 Rot(float a) {
    float s=sin(a), c=cos(a);
    return mat2(c, -s, s, c);
}

float SDFbox(vec3 pos, vec3 dim) {
    vec3 p = abs(pos) - dim;;
    return length(max(p, 0.)) + min(max(p.x, max(p.y, p.z)), 0.);;
}

float SDFsphere(vec3 pos, float r) {
    return length(pos) - r;
}

float SDF2box(vec2 pos, vec2 dim) {
    vec2 p = abs(pos) - dim;
    return length(max(p, 0.)) + min(max(p.x, p.y), 0.);;
}

float SDFring(vec2 pos, float r) {
    return length(pos) - r;
}

float SDFseg(vec3 pos, vec3 a, vec3 b) {
    vec3 segDir = b - a;
    float t = max(min(dot(pos - a, segDir)/dot(segDir, segDir), 1.), 0.);
    vec3 q = a + t * (b - a);
    return length(pos - q);
}

vec2 SDFball(vec3 pos, float a) {
    
    pos.y -= 1.8;
    pos.yx *= Rot(a);
    pos.y += 1.8;
    
    float sphereDist = SDFsphere(pos - vec3(0, .5, 0), .2);
    float ring = length(vec2(SDFring(pos.yx - vec2(.7, 0.), .03), pos.z)) - .01;
    
    vec3 sim = pos;
    sim.z = abs(sim.z);
    float line = SDFseg(sim, vec3(0, .7, 0), vec3(0., 1.8, .5))-0.008;
    
    sphereDist = min(sphereDist, ring);
    
    float dist = min(sphereDist, line); 
    
    return vec2(dist, sphereDist == dist ? mat_ball : mat_line);
}

vec2 mini(vec2 a, vec2 b) {
    return a.x < b.x ? a : b;
}

vec2 GetDist(vec3 pos) {
	vec4 sphere = vec4(0, 1, 6, 1);
    
    float a = 0.78 * cos(iTime * 1.5) * min(10. / iTime, 1.);
    float aplus = min(a, 0.);
    float amin = max(a, 0.);
    
    vec2 ball1 = SDFball(pos, 0.05 * a);
    vec2 ball2 = SDFball(pos - vec3(.41, .0, 0), aplus * 0.02 + amin * 0.07);
    vec2 ball3 = SDFball(pos - vec3(.82, .0, 0), amin + 0.01 * aplus);
    vec2 ball4 = SDFball(pos - vec3(-.41, .0, 0), aplus * 0.07 + amin * 0.02);
    vec2 ball5 = SDFball(pos - vec3(-.82, .0, 0), aplus + 0.01 * amin);
    
    float support = SDFbox(pos - vec3(0, 0, 0), vec3(1.5, .1, .7)-0.05) - 0.05;
    support = max(support, -pos.y);
    
    float bar = length(vec2(SDF2box(pos.yx, vec2(1.8, 1.3)), abs(pos.z) - 0.5)) - 0.05;
    bar = max(bar, -pos.y);
    
    vec2 ball = mini(mini(mini(mini(ball1, ball2), ball3), ball4), ball5);
    
    float dist = min(min(support, bar), ball.x);
    
    int mat = 0;
    
    if (dist == support) mat = mat_support;
    else if (dist == bar) mat = mat_bar;
    else if (dist == ball.x) mat = int(ball.y);
    
    return vec2(dist, mat);
}

vec2 RayMarch(vec3 ro, vec3 rd) {
    float dO = 0.;
    vec2 dS = vec2(0.);
    for (int i = 0; i < MAX_STEPS; i++) {
    	vec3 pos = ro + rd * dO;
        dS = GetDist(pos);
        dO += dS.x;
        if(dO > MAX_DIST || dS.x < SURF_DIST) break;
    }
    
    return vec2(dO, dS.y);
}

vec3 GetNormal(vec3 pos) {
	float dist = GetDist(pos).x;
    vec2 eps = vec2(.01, 0);
    
    vec3 norm = dist - vec3(
        GetDist(pos - eps.xyy).x,
        GetDist(pos - eps.yxy).x,
        GetDist(pos - eps.yyx).x);
    
    return normalize(norm);
}

vec3 GetRayDir(vec2 uv, vec3 p, vec3 l, float z) {
    vec3 f = normalize(l-p),
        r = normalize(cross(vec3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}

vec3 Render(inout vec3 ro, inout vec3 rd, inout vec3 ref, bool last) {

   vec3 col = texture(iChannel0, rd).rgb;

   vec2 d = RayMarch(ro, rd);

   if(d.x<MAX_DIST) {
      vec3 p = ro + rd * d.x;
      vec3 n = GetNormal(p);
      vec3 r = reflect(rd, n);
      
      float fresnel = pow(1. - abs(dot(n, -rd)), 5.);

      float dif = dot(n, normalize(vec3(1,2,3)))*.5+.5;
       
      if (int(d.y) == mat_support) {
          ref *= vec3(mix(0.01, 0.5, fresnel));
          col = vec3(dif) * 0.1;
      } else if (int(d.y) == mat_bar) {
          ref *= vec3(0.9);
          col = vec3(dif) * 0.1;
      } else if (int(d.y) == mat_ball) {
          ref *= vec3(0.9, 0.65, 0.2);
          col = vec3(dif) * 0.1;
      } else if (int(d.y) == mat_line) {
          ref = vec3(0.01);
          col = vec3(dif) * 0.1;
      }
      
      if (last) col += texture(iChannel0, r).rgb;
      
      ro = p + 3. * n * SURF_DIST;
      rd = r;
      
   } else ref = vec3(0.);
   
   return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord-.5*iResolution.xy)/iResolution.y;
    vec2 m = iMouse.xy/iResolution.xy;

    vec3 ro = vec3(0, 3, -3);
    ro.yz *= Rot(-m.y*3.14+1.);
    ro.xz *= Rot(-m.x*6.2831);
    
    vec3 rd = GetRayDir(uv, ro, vec3(0, 0.75, 0), 1.6);
    
    vec3 ref = vec3(1.);
    
    vec3 col = Render(ro, rd, ref, false);
    
    int NB_BOUNCE = 2;
    for (int i = 0; i < NB_BOUNCE; i++) {
        col += ref * Render(ro, rd, ref, i + 1 == NB_BOUNCE);
    }
    
    col = pow(col, vec3(.4545));

    fragColor = vec4(col,1.0);
}