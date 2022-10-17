

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Settings
// (smoothness, extinction and scale are changed in function of heart beats)
      float smoothness;           // 1.0: smooth shading  0.0: faceted shading 
      float refract_index;        // 1.0: no refraction   1.3: water   2.4: diamond
      float extinction;           // the higher, the darker (in function of thicness) 
const float reflect_coeff = 0.75; // 0.0: no reflection   1.0: shiny reflections
const vec3  core_color = vec3(1.0,0.0,0.0); 
      float scale;
      bool  dispersion = true;    // set to true to decompose light like a prism
bool  fixed_cam = false;          // Set to true for fixed cam and rotating lighting env.

// Mesh data borrowed from https://www.thingiverse.com/thing:4041510

const vec3 bbox_min = vec3(-0.429853,-0.406522,-0.147051);
const vec3 bbox_max = vec3(0.429853,0.406522,0.147051);

// Yes, I could have compressed that in a single int per triangle, on the other hand there are
// only 64 of them, so I think it will not make a big difference...
const ivec3 triangles[64] = ivec3[](
ivec3(0x00000004,0x00000015,0x00000003),ivec3(0x00000015,0x00000017,0x00000003),ivec3(0x00000004,0x00000016,0x00000015),ivec3(0x00000003,0x00000005,0x00000004),ivec3(0x00000016,0x00000017,0x00000015),ivec3(0x00000001,0x00000016,0x00000004),ivec3(0x00000003,0x00000006,0x00000005),ivec3(0x00000005,0x00000019,0x00000004),ivec3(0x00000002,0x00000016,0x00000001),ivec3(0x00000004,0x0000001a,0x00000001),ivec3(0x00000000,0x00000017,0x00000016),ivec3(0x00000017,0x00000020,0x00000003),ivec3(0x00000019,0x0000001a,0x00000004),ivec3(0x00000000,0x00000016,0x00000002),ivec3(0x00000001,0x0000001a,0x00000002),
ivec3(0x00000003,0x00000020,0x00000006),ivec3(0x00000006,0x00000007,0x00000005),ivec3(0x00000008,0x00000019,0x00000005),ivec3(0x00000007,0x00000008,0x00000005),ivec3(0x00000019,0x0000001b,0x0000001a),ivec3(0x00000002,0x0000001a,0x00000013),ivec3(0x00000000,0x00000018,0x00000017),ivec3(0x00000018,0x00000021,0x00000017),ivec3(0x00000017,0x00000021,0x00000020),ivec3(0x00000002,0x00000013,0x00000000),ivec3(0x00000006,0x00000020,0x00000007),ivec3(0x00000008,0x0000001b,0x00000019),ivec3(0x0000001a,0x0000001b,0x00000013),ivec3(0x00000013,0x00000018,0x00000000),ivec3(0x00000007,0x0000001d,0x00000008),ivec3(0x00000009,0x00000021,0x00000018),
ivec3(0x00000007,0x00000020,0x0000001d),ivec3(0x00000020,0x00000021,0x00000009),ivec3(0x0000000a,0x00000018,0x00000013),ivec3(0x00000013,0x0000001b,0x00000012),ivec3(0x00000008,0x0000001c,0x0000001b),ivec3(0x0000001d,0x00000020,0x0000001f),ivec3(0x0000000a,0x00000013,0x0000000b),ivec3(0x00000009,0x00000018,0x0000000d),ivec3(0x0000000d,0x00000020,0x00000009),ivec3(0x0000001d,0x0000001e,0x00000008),ivec3(0x0000000b,0x00000013,0x00000012),ivec3(0x0000001b,0x0000001c,0x00000012),ivec3(0x0000000d,0x00000018,0x0000000a),ivec3(0x0000001f,0x00000020,0x0000000d),ivec3(0x00000008,0x0000001e,0x0000001c),ivec3(0x0000001d,0x0000001f,0x00000014),
ivec3(0x0000000b,0x00000012,0x0000000f),ivec3(0x00000014,0x0000001e,0x0000001d),ivec3(0x0000000b,0x00000010,0x0000000a),ivec3(0x00000012,0x0000001c,0x00000011),ivec3(0x0000000a,0x00000010,0x0000000d),ivec3(0x0000001c,0x0000001e,0x00000014),ivec3(0x0000000f,0x00000012,0x00000011),ivec3(0x00000011,0x0000001c,0x00000014),ivec3(0x0000000f,0x00000010,0x0000000b),ivec3(0x00000014,0x0000001f,0x0000000e),ivec3(0x0000000e,0x0000001f,0x0000000d),ivec3(0x0000000f,0x00000011,0x00000010),ivec3(0x00000011,0x00000014,0x0000000e),ivec3(0x0000000d,0x00000010,0x0000000c),ivec3(0x00000010,0x00000011,0x0000000c),ivec3(0x0000000c,0x0000000e,0x0000000d),
ivec3(0x0000000c,0x00000011,0x0000000e));

const int points[34] = int[](
0x017d7191,0x2c5eb8dd,0x21cffd13,0x21aab001,0x354c7486,0x353860b1,0x21530152,0x35235591,0x3a4545f4,0x01c6fe52,0x017d726d,0x21cffeeb,0x21cd03ff,0x020aeb32,0x21aab3fd,0x2e3ebb21, 0x223ebba2,0x351c7f79,0x351eb2aa,0x21fe21ff,0x3598b72a,0x21cd0000,0x223eb85c,0x020ae8dd,0x000a7606,0x3e3cd911,0x351eb154,0x382b6dff,0x3ffca6ee,0x3523566d,0x3588b32b,0x2182e29b, 0x21700206,0x01d6f9ac
);

const int normals[34] = int[](
0x053c5a11,0x380ca561,0x184fba2a,0x13745467,0x3b99790f,0x3b95f121,0x2bd36c88,0x3b44b15d,0x3fd755fd,0x01b5ce56,0x053c59ed,0x186fb9d5,0x153aa7c2,0x0396d6de,0x1de42bbf,0x380ca69c, 0x0f0d76ff,0x3ba936f2,0x390cb9a1,0x1dcff5ff,0x36e53b34,0x13ea9c44,0x0ead60ff,0x03a6c91e,0x0038edff,0x3fe831e5,0x38fcc657,0x3f29c1f7,0x3fe83219,0x3b94f6a7,0x3f16da59,0x19535390, 0x07132da4,0x01c5cda4
);



// Gets x,y,z, coordinates packed in a single 32 bits int (10 bits per coordinate)
int get_compressed_point(in int v) {
    return  points[v];
}

// Converts a single-int 10 bits per components packed point into a standard vec3.
vec3 uncompress_point(in int xyz) {
   ivec3 XYZ = (ivec3(xyz) >> ivec3(0,10,20)) & ivec3(1023); 
   return scale * (bbox_min + (bbox_max - bbox_min) * vec3(XYZ) / 1023.0);
}

// Gets a point from a vertex index
vec3 get_point(int v) {
   return uncompress_point(get_compressed_point(v));
}

// Gets a normal from a vertex index
vec3 get_normal(int v) {
   int xyz = normals[v];
   ivec3 XYZ = (ivec3(xyz) >> ivec3(0,10,20)) & ivec3(1023); 
   return vec3(-1) + vec3(XYZ) / 512.0;    
}

const float FARAWAY=1e30;

struct Camera {
    vec3 Obs;
    vec3 View;
    vec3 Up;
    vec3 Horiz;
    float H;
    float W;
    float z;
};

struct Ray {
    vec3 Origin;
    vec3 Dir;
};

Camera camera(in vec3 Obs, in vec3 LookAt, in float aperture) {
   Camera C;
   C.Obs = Obs;
   C.View = normalize(LookAt - Obs);
   C.Horiz = normalize(cross(vec3(0.0, 1.0, 0.0), C.View));
   C.Up = cross(C.View, C.Horiz);
   C.W = float(iResolution.x);
   C.H = float(iResolution.y);
   C.z = (C.H/2.0) / tan((aperture * 3.1415 / 180.0) / 2.0);
   return C;
}

Ray launch(in Camera C, in vec2 XY) {
   return Ray(
      C.Obs,
      C.Obs+C.z*C.View+(XY.x-C.W/2.0)*C.Horiz+(XY.y-C.H/2.0)*C.Up 
   );
}

struct Intersection {
   float t;
   vec3 P;
   vec3 N;
   vec3 Nsmooth;
   int id; // index of latest intersected triangle, used to avoid finding the point
           //  where you started from when computing multiple bounces.
};
 
Intersection intersection() {
   Intersection I;
   I.t = FARAWAY;
   I.id = -1; 
   return I;
}

Ray reflect_ray(in Ray R, in Intersection I) {
    return Ray(I.P, reflect(R.Dir, I.N));
}

Ray refract_ray(in Ray R, in Intersection I, in float n1, in float n2) {
    return Ray(I.P, refract(normalize(R.Dir), I.N, n1/n2));
}

// Branchless ray-triangle intersection that gives, for free, as by-products:
//   Normal N
//   Barycentric coordinates (1-u-v, u, v)
// (slightly modified Moller-Trumbore algorithm)
// References and explanations here:
// https://stackoverflow.com/questions/42740765/intersection-between-line-and-triangle-in-3d/42752998#42752998
bool intersect_triangle(
    in Ray R, in vec3 A, in vec3 B, in vec3 C, out float t, 
    out float u, out float v, out vec3 N
) { 
   vec3 E1 = B-A;
   vec3 E2 = C-A;
         N = cross(E1,E2);
   float det = -dot(R.Dir, N);
   float invdet = 1.0/det;
   vec3 AO  = R.Origin - A;
   vec3 DAO = cross(AO, R.Dir);
   u =  dot(E2,DAO) * invdet;
   v = -dot(E1,DAO) * invdet;
   t =  dot(AO,N)   * invdet;
   return (abs(det) >= 1e-6 && t >= 0.0 && u >= 0.0 && v >= 0.0 && (u+v) <= 1.0);
}

// Computes ray-triangle intersection from node index (i)
// Returns true if there was an intersection.
// Note: normal is not normalized
bool triangle(in Ray R, in int i, inout Intersection I) {
   ivec3 T = triangles[i];
   vec3 A = get_point(T.x);
   vec3 B = get_point(T.y);
   vec3 C = get_point(T.z);    
   float t,u,v;
   vec3 N;
   if(intersect_triangle(R, A,B,C, t, u, v, N) && t < I.t) {
      I.t = t;
      I.P = R.Origin + t*R.Dir;
      I.id = i;
      vec3 N1 = get_normal(T.x);
      vec3 N2 = get_normal(T.y);
      vec3 N3 = get_normal(T.z);       
      I.Nsmooth = (1.0-u-v)*N1 + u*N2 + v*N3;
      I.N = N;
      return true;
   }
   return false;
}
 
// Good explanations here:
// https://tavianator.com/fast-branchless-raybounding-box-intersections/
bool segment_box_intersection(
  in vec3 q1,
  in vec3 dirinv,
  in vec3 boxmin,
  in vec3 boxmax,
  in float t 
) {  
   vec3 T1 = dirinv*(boxmin - q1);
   vec3 T2 = dirinv*(boxmax - q1);
   vec3 Tmin = min(T1,T2);
   vec3 Tmax = max(T1,T2);
   float tmin = max(max(Tmin.x, Tmin.y),Tmin.z);
   float tmax = min(min(Tmax.x, Tmax.y),Tmax.z);    
   return (tmax >= 0.0) && (tmin <= tmax) && (tmin <= t);
}

// No AABB-tree here (not worth it, we only got 64 triangles !)
// Mesh ray-tracing with AABB-tree is here: https://www.shadertoy.com/view/WlcXRS
bool raytrace_mesh(in Ray R, inout Intersection I) {
    int prev_id = I.id; // index of latest intersected triangle
    bool result = false;  
    // ... But I still keep a single AABB around the whole mesh.
    // We could do without it, but it gains a couple of FPS.
    vec3 invDir = vec3(1.0/R.Dir.x, 1.0/R.Dir.y, 1.0/R.Dir.z);
    if(!segment_box_intersection(R.Origin, invDir, scale*bbox_min, scale*bbox_max, I.t)) {
        return false;
    }
    for(int i=0; i<triangles.length(); ++i) {
       if(i == prev_id) { continue; } // skip latest triangle
       bool t_isect = triangle(R, i, I);
       result = result || t_isect;
    }
    if(result) { // Interpolating between facetted and smooth shading, just for fun.
        I.N = mix(normalize(I.N), normalize(I.Nsmooth), smoothness);
        I.N = normalize(I.N); 
    }
    return result;
}  

// Modifies ray and intersection, returns total length of traversed matter.
float multi_refract(inout Ray R, inout Intersection I) {
    float result = 0.0;
    for(int i=0; i<10; ++i) {
        if(dot(R.Dir,I.N) > 0.0) { // Exiting matter
            result += I.t;
            vec3 old_dir = R.Dir;
            I.N = -I.N;
            R = refract_ray(R, I, refract_index, 1.0);
            if(R.Dir == vec3(0)) { 
               R.Dir = reflect(old_dir, I.N); // total reflection  
            }
        } else { // Entering matter
            R = refract_ray(R, I, 1.0, refract_index);
            R.Dir = normalize(R.Dir); // Needs to be unit, so that on exit, I.t is distance
        }
        I.t = FARAWAY;
        if(!raytrace_mesh(R, I)) { break; }  
    }
    return result;
}

vec3 sky(in Ray R) {
    if(fixed_cam) {
        float alpha = iTime/2.0;
        float s = sin(alpha);
        float c = cos(alpha);
        vec3 V = vec3(
            R.Dir.x * c + R.Dir.z * s,
            R.Dir.y,
            R.Dir.x * s - R.Dir.z * c
        );
        return vec3(textureLod(iChannel0, V, 0.0));
    }
    return vec3(textureLod(iChannel0, R.Dir, 0.0));
}


// Borrowed from https://www.shadertoy.com/view/XsKBWD
float beat() {
  return .7 + abs(-abs(sin(iTime * 3.)) + .5);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    
   bool animate = (sin(iTime/2.0) < 0.0); 
   if(animate) {
      refract_index = 1.2;
      scale      = 5.0+5.0*beat();
      extinction = 0.5*(1.0 + sin(iTime*6.0));
      smoothness = beat(); // 0.5*(1.0 + sin(iTime*3.0));        
   } else {
      refract_index = 2.4;
      scale = 10.0;  
      extinction = 0.0;  
      smoothness = 0.0;          
   }
    
   float alpha = fixed_cam ? 0.0 : iTime;  
   float c = cos(alpha);
   float s = sin(alpha);
   Camera C = camera(
       vec3(20.0*s, 1.5, 20.0*c),
       vec3(0.0, 0.0, 0.0),
       30.0       
   );
   Ray R = launch(C, fragCoord);
   Intersection I = intersection();
   raytrace_mesh(R, I);
 
   if(I.t == FARAWAY) {
      fragColor.xyz = sky(R); 
    } else {        
      float fresnel = 1.0 + dot(R.Dir,I.N) / length(R.Dir);
      fresnel = clamp(fresnel, 0.0, 1.0); 
      fragColor.xyz = reflect_coeff * fresnel * sky(reflect_ray(R,I)); 
       
      if(dispersion) {
         // Trace three rays (red, green, blue), change refraction index
         // for each of them.
         float base_refract_index = refract_index;
         Ray R0 = R;
         Intersection I0 = I;
         refract_index = base_refract_index;
         float l = multi_refract(R,I);
         float d = exp(-l * extinction);
         float r = sky(R).r;
         I = I0; R = R0; refract_index = base_refract_index + 0.03;
         multi_refract(R,I); 
         float g = sky(R).g; 
         I = I0; R = R0; refract_index = base_refract_index + 0.06;
         multi_refract(R,I); 
         float b = sky(R).b;  
         // Assemble final color using the contributions from the
         // red, green and blue rays.
         fragColor.xyz += mix(core_color, vec3(r,g,b), d);
      } else {
         float l = multi_refract(R,I);
         float d = exp(-l * extinction);
         fragColor.xyz += mix(core_color, sky(R), d);
      }       
    }
}
