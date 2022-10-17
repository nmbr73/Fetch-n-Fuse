

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// CC0: Ice sheet height field experimentation
//  Was tinkering with using recursive voronoi patterns to
//  generate something that could pass for ice breaking up
//  into smaller blocks
#define RESOLUTION  iResolution
#define TIME        iTime

// License: Unknown, author: Unknown, found: don't remember
vec2 hash2(vec2 p) {
  p = vec2(dot (p, vec2 (127.1, 311.7)), dot (p, vec2 (269.5, 183.3)));
  return fract (sin (p)*43758.5453123);
}

// From: https://www.shadertoy.com/view/MsScWz
// Originally from: https://www.shadertoy.com/view/ldl3W8
vec3 voronoi(vec2 x) {
  vec2 n = floor(x);
  vec2 f = fract(x);

  vec2 mr;
  vec2 mp;

  float md = 8.0;
  for(int j=-1; j<=1; ++j)
  for(int i=-1; i<=1; ++i) {
    vec2 g = vec2(float(i),float(j));
    vec2 o = hash2(n + g);
    vec2 r = g + o - f;
    float d = dot(r,r);

    if(d<md) {
      md = d;
      mr = r;
      mp = x+r;
    }
  }

  md = 8.0;
  for(int j=-1; j<=1; ++j)
  for(int i=-1; i<=1; ++i) {
    vec2 g = vec2(float(i),float(j));
    vec2 o = hash2(n + g);
    vec2 r = g + o - f;

    if(dot(mr-r,mr-r)>0.0001) // skip the same cell
      md = min(md, dot(0.5*(mr+r), normalize(r-mr)));
  }

  return vec3(md, mp);
}

float height(vec2 p) {
  vec2 vp = p;
  float vz = 1.0;
  
  const float aa = 0.025;

  float gh = 0.0;
  float hh = 0.0;

  const float hf = 0.025;

  // Recursive voronois
  {
    vec3 c = voronoi(vp);
    gh = tanh(max(abs(0.35*(c.y-2.0*sin(0.25*c.z)*cos(sqrt(0.1)*c.z)))-0.4, 0.));
    hh = smoothstep(-aa, aa, c.x-2.0*aa*smoothstep(1.0, 0.75, gh));
    if (gh > 0.75) {    
      return hf*tanh(hh+1.0*(gh-0.75));
    }

    vz *= 0.5;
    vp = vp * 2.0;
  }

  {
    vec3 c = voronoi(vp);
    hh = hh*smoothstep(-aa, aa, vz*c.x-3.0*aa*smoothstep(1.0, 0.5, gh));
    if (gh > 0.5) {
      return 0.75*hf*hh;
    }

    vz *= 0.5;
    vp = vp * 2.0;
  }

  {
    vec3 c = voronoi(vp);
    hh = hh*smoothstep(-aa, aa, vz*c.x-2.0*aa*smoothstep(0.9, 0.25, gh));
    if (gh > 0.25) {
      return 0.5*hf*hh;
    }

    vz *= 0.5;
    vp = vp * 2.0;
  }
  
  return 0.0;
}

vec3 normal(vec2 p) {
  vec2 v;
  vec2 w;
  vec2 e = vec2(4.0/RESOLUTION.y, 0);
  
  vec3 n;
  n.x = height(p + e.xy) - height(p - e.xy);
  n.y = 2.0*e.x;
  n.z = height(p + e.yx) - height(p - e.yx);
  
  return normalize(n);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
  vec2 q = fragCoord/RESOLUTION.xy;
  vec2 p = -1. + 2. * q;
  p.x *= RESOLUTION.x/RESOLUTION.y;
  float aa = 2.0/RESOLUTION.y;
  
  float z = mix(0.2, 0.5, smoothstep(-0.5, 0.5, sin(0.5*TIME)));

  vec2 ip = p;
  ip /= z;
  ip.y += 0.5*TIME;
  float h = height(ip);
  vec3 n  = normal(ip);
 
  vec3 ro = vec3(0.0, -1.0, 0.0);
  vec3 lp = vec3(1.0, -0.95, 1.5);
  vec3 pp = vec3(p.x, h, p.y);;
  vec3 rd = normalize(ro-pp);
  vec3 ld = normalize(pp-lp);
  vec3 ref= reflect(rd, n);
  
  float dif = max(dot(n, ld), 0.0)*tanh(200.0*h);
  float spe = pow(max(dot(ref, ld), 0.0), 10.0);
 
  vec3 col = vec3(0.);
  col += dif;
  col += spe;
  
  
  fragColor = vec4(col, 1.0);
}

