
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel0


// Settings
// (smoothness, extinction and scale are changed in function of heart beats)
__DEVICE__      float smoothness;           // 1.0: smooth shading  0.0: faceted shading 
__DEVICE__      float refract_index;        // 1.0: no refraction   1.3: water   2.4: diamond
__DEVICE__      float extinction;           // the higher, the darker (in function of thicness) 
__DEVICE__      float reflect_coeff = 0.75f; // 0.0: no reflection   1.0: shiny reflections
__DEVICE__      float3  core_color;// = to_float3(1.0f,0.0f,0.0f); 
__DEVICE__       float scale;
__DEVICE__ bool  dispersion = true;    // set to true to decompose light like a prism
__DEVICE__ bool  fixed_cam = false;          // Set to true for fixed cam and rotating lighting env.

// Mesh data borrowed from https://www.thingiverse.com/thing:4041510

__DEVICE__ float3 bbox_min;// = to_float3(-0.429853f,-0.406522f,-0.147051f);
__DEVICE__ float3 bbox_max;// = to_float3(0.429853f,0.406522f,0.147051f);



__DEVICE__ const int points[34] = {
0x017d7191,0x2c5eb8dd,0x21cffd13,0x21aab001,0x354c7486,0x353860b1,0x21530152,0x35235591,0x3a4545f4,0x01c6fe52,0x017d726d,0x21cffeeb,0x21cd03ff,0x020aeb32,0x21aab3fd,0x2e3ebb21, 0x223ebba2,0x351c7f79,0x351eb2aa,0x21fe21ff,0x3598b72a,0x21cd0000,0x223eb85c,0x020ae8dd,0x000a7606,0x3e3cd911,0x351eb154,0x382b6dff,0x3ffca6ee,0x3523566d,0x3588b32b,0x2182e29b, 0x21700206,0x01d6f9ac
};

__DEVICE__ const int normals[34] = {
0x053c5a11,0x380ca561,0x184fba2a,0x13745467,0x3b99790f,0x3b95f121,0x2bd36c88,0x3b44b15d,0x3fd755fd,0x01b5ce56,0x053c59ed,0x186fb9d5,0x153aa7c2,0x0396d6de,0x1de42bbf,0x380ca69c, 0x0f0d76ff,0x3ba936f2,0x390cb9a1,0x1dcff5ff,0x36e53b34,0x13ea9c44,0x0ead60ff,0x03a6c91e,0x0038edff,0x3fe831e5,0x38fcc657,0x3f29c1f7,0x3fe83219,0x3b94f6a7,0x3f16da59,0x19535390, 0x07132da4,0x01c5cda4
};



// Gets x,y,z, coordinates packed in a single 32 bits int (10 bits per coordinate)
__DEVICE__ int get_compressed_point(in int v) {
    return  points[v];
}

// Converts a single-int 10 bits per components packed point into a standard vec3.
__DEVICE__ float3 uncompress_point(in int xyz) {
  float zzzzzzzzzzzzzzzzz;
   int3 XYZ = to_int3((xyz>>0)&1023,(xyz>>10)&1023,(xyz>>20)&1023); 
   return scale * (bbox_min + (bbox_max - bbox_min) * make_float3(XYZ) / 1023.0f);
}

// Gets a point from a vertex index
__DEVICE__ float3 get_point(int v) {
   return uncompress_point(get_compressed_point(v));
}

// Gets a normal from a vertex index
__DEVICE__ float3 get_normal(int v) {
   int xyz = normals[v];
   //int3 XYZ = (to_int3(xyz) >> to_int3(0,10,20)) & to_int3(1023); 
   int3 XYZ = to_int3((xyz>>0)&1023,(xyz>>10)&1023,(xyz>>20)&1023); 
   return to_float3_s(-1) + make_float3(XYZ) / 512.0f;    
}

//const float FARAWAY=1e30;

struct Camera {
    float3 Obs;
    float3 View;
    float3 Up;
    float3 Horiz;
    float H;
    float W;
    float z;
};

struct Ray {
    float3 Origin;
    float3 Dir;
};

__DEVICE__ Camera camera(in float3 Obs, in float3 LookAt, in float aperture, float2 iResolution) {
   Camera C;
   C.Obs = Obs;
   C.View = normalize(LookAt - Obs);
   C.Horiz = normalize(cross(to_float3(0.0f, 1.0f, 0.0f), C.View));
   C.Up = cross(C.View, C.Horiz);
   C.W = float(iResolution.x);
   C.H = float(iResolution.y);
   C.z = (C.H/2.0f) / _tanf((aperture * 3.1415f / 180.0f) / 2.0f);
   return C;
}

__DEVICE__ Ray launch(in Camera C, in float2 XY) {
   Ray ret = {C.Obs, C.Obs+C.z*C.View+(XY.x-C.W/2.0f)*C.Horiz+(XY.y-C.H/2.0f)*C.Up };
   return ret;
}

struct Intersection {
   float t;
   float3 P;
   float3 N;
   float3 Nsmooth;
   int id; // index of latest intersected triangle, used to avoid finding the point
           //  where you started from when computing multiple bounces.
};
 
__DEVICE__ Intersection intersection(float FARAWAY) {
   Intersection I;
   I.t = FARAWAY;
   I.id = -1; 
   return I;
}

__DEVICE__ Ray reflect_ray(in Ray R, in Intersection I) {
    Ray ret = {I.P, reflect(R.Dir, I.N)};
    return ret;
}

__DEVICE__ Ray refract_ray(in Ray R, in Intersection I, in float n1, in float n2) {
    Ray ret = {I.P, refract_f3(normalize(R.Dir), I.N, n1/n2)};
    return ret;
}

// Branchless ray-triangle intersection that gives, for free, as by-products:
//   Normal N
//   Barycentric coordinates (1-u-v, u, v)
// (slightly modified Moller-Trumbore algorithm)
// References and explanations here:
// https://stackoverflow.com/questions/42740765/intersection-between-line-and-triangle-in-3d/42752998#42752998
__DEVICE__ bool intersect_triangle(
    in Ray R, in float3 A, in float3 B, in float3 C,
    out float *t, out float *u, out float *v, out float3 *N
) { 
   float3 E1 = B-A;
   float3 E2 = C-A;
           *N = cross(E1,E2);
   float det = -dot(R.Dir, *N);
   float invdet = 1.0f/det;
   float3 AO  = R.Origin - A;
   float3 DAO = cross(AO, R.Dir);
   *u =  dot(E2,DAO) * invdet;
   *v = -dot(E1,DAO) * invdet;
   *t =  dot(AO,*N)   * invdet;
   return (_fabs(det) >= 1e-6 && *t >= 0.0f && *u >= 0.0f && *v >= 0.0f && (*u + *v) <= 1.0f);
}

// Computes ray-triangle intersection from node index (i)
// Returns true if there was an intersection.
// Note: normal is not normalized
__DEVICE__ bool triangle(in Ray R, in int i, inout Intersection *I, int3 triangles[64]) {
   int3 T = triangles[i];
   float3 A = get_point(T.x);
   float3 B = get_point(T.y);
   float3 C = get_point(T.z);    
   float t,u,v;
   float3 N;
   if(intersect_triangle(R, A,B,C, &t, &u, &v, &N) && t < (*I).t) {
      (*I).t = t;
      (*I).P = R.Origin + t*R.Dir;
      (*I).id = i;
      float3 N1 = get_normal(T.x);
      float3 N2 = get_normal(T.y);
      float3 N3 = get_normal(T.z);       
      (*I).Nsmooth = (1.0f-u-v)*N1 + u*N2 + v*N3;
      (*I).N = N;
      return true;
   }
   return false;
}
 
// Good explanations here:
// https://tavianator.com/fast-branchless-raybounding-box-intersections/
__DEVICE__ bool segment_box_intersection(
  in float3 q1,
  in float3 dirinv,
  in float3 boxmin,
  in float3 boxmax,
  in float t 
) {  
   float3 T1 = dirinv*(boxmin - q1);
   float3 T2 = dirinv*(boxmax - q1);
   float3 Tmin = _fminf(T1,T2);
   float3 Tmax = _fmaxf(T1,T2);
   float tmin = _fmaxf(max(Tmin.x, Tmin.y),Tmin.z);
   float tmax = _fminf(min(Tmax.x, Tmax.y),Tmax.z);    
   return (tmax >= 0.0f) && (tmin <= tmax) && (tmin <= t);
}

// No AABB-tree here (not worth it, we only got 64 triangles !)
// Mesh ray-tracing with AABB-tree is here: https://www.shadertoy.com/view/WlcXRS
__DEVICE__ bool raytrace_mesh(in Ray R, inout Intersection *I, int3 triangles[64]) {
    int prev_id = (*I).id; // index of latest intersected triangle
    bool result = false;  
    // ... But I still keep a single AABB around the whole mesh.
    // We could do without it, but it gains a couple of FPS.
    float3 invDir = to_float3(1.0f/R.Dir.x, 1.0f/R.Dir.y, 1.0f/R.Dir.z);
    if(!segment_box_intersection(R.Origin, invDir, scale*bbox_min, scale*bbox_max, (*I).t)) {
        return false;
    }
    //for(int i=0; i<triangles.length(); ++i) {
      for(int i=0; i<64; ++i) {
       if(i == prev_id) { continue; } // skip latest triangle
       bool t_isect = triangle(R, i, I, triangles);
       result = result || t_isect;
    }

    if(result) { // Interpolating between facetted and smooth shading, just for fun.
        (*I).N = _mix(normalize((*I).N), normalize((*I).Nsmooth), smoothness);
        (*I).N = normalize((*I).N); 
    }
    return result;
}  

// Modifies ray and intersection, returns total length of traversed matter.
__DEVICE__ float multi_refract(inout Ray R, inout Intersection *I, float FARAWAY,int3 triangles[64]) {
    float result = 0.0f;
    for(int i=0; i<10; ++i) {
        if(dot(R.Dir,(*I).N) > 0.0f) { // Exiting matter
            result += (*I).t;
            float3 old_dir = R.Dir;
            (*I).N = -(*I).N;
float rrrrrrrrrrrrrrrrrrrr;    
            R = refract_ray(R, *I, refract_index, 1.0f);
            //if(R.Dir == to_float3_s(0)) { 
            if(R.Dir.x == 0.0f && R.Dir.y == 0.0f && R.Dir.z == 0.0f) { 
               R.Dir = reflect(old_dir, (*I).N); // total reflection  
            }
        } else { // Entering matter
            R = refract_ray(R, *I, 1.0f, refract_index);
            R.Dir = normalize(R.Dir); // Needs to be unit, so that on exit, I.t is distance
        }
        (*I).t = FARAWAY;
        if(!raytrace_mesh(R, I, triangles)) { break; }  
    }
    return result;
}

__DEVICE__ float3 sky(in Ray R, float iTime, __TEXTURE2D__ iChannel0) {
    if(fixed_cam) {
        float alpha = iTime/2.0f;
        float s = _sinf(alpha);
        float c = _cosf(alpha);
        float3 V = to_float3(
            R.Dir.x * c + R.Dir.z * s,
            R.Dir.y,
            R.Dir.x * s - R.Dir.z * c
        );
        return swi3(decube_f3(iChannel0, V),x,y,z);
    }
   
    
    return swi3(decube_f3(iChannel0, R.Dir),x,y,z);
}


// Borrowed from https://www.shadertoy.com/view/XsKBWD
__DEVICE__ float beat(float iTime) {
  return 0.7f + _fabs(-_fabs(_sinf(iTime * 3.0f)) + 0.5f);
}


__KERNEL__ void D75KilobytesForNathalieFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    // Yes, I could have compressed that in a single int per triangle, on the other hand there are
    // only 64 of them, so I think it will not make a big difference...
    int3 triangles[64] = {
    to_int3(0x00000004,0x00000015,0x00000003),to_int3(0x00000015,0x00000017,0x00000003),to_int3(0x00000004,0x00000016,0x00000015),to_int3(0x00000003,0x00000005,0x00000004),to_int3(0x00000016,0x00000017,0x00000015),to_int3(0x00000001,0x00000016,0x00000004),to_int3(0x00000003,0x00000006,0x00000005),to_int3(0x00000005,0x00000019,0x00000004),to_int3(0x00000002,0x00000016,0x00000001),to_int3(0x00000004,0x0000001a,0x00000001),to_int3(0x00000000,0x00000017,0x00000016),to_int3(0x00000017,0x00000020,0x00000003),to_int3(0x00000019,0x0000001a,0x00000004),to_int3(0x00000000,0x00000016,0x00000002),to_int3(0x00000001,0x0000001a,0x00000002),
    to_int3(0x00000003,0x00000020,0x00000006),to_int3(0x00000006,0x00000007,0x00000005),to_int3(0x00000008,0x00000019,0x00000005),to_int3(0x00000007,0x00000008,0x00000005),to_int3(0x00000019,0x0000001b,0x0000001a),to_int3(0x00000002,0x0000001a,0x00000013),to_int3(0x00000000,0x00000018,0x00000017),to_int3(0x00000018,0x00000021,0x00000017),to_int3(0x00000017,0x00000021,0x00000020),to_int3(0x00000002,0x00000013,0x00000000),to_int3(0x00000006,0x00000020,0x00000007),to_int3(0x00000008,0x0000001b,0x00000019),to_int3(0x0000001a,0x0000001b,0x00000013),to_int3(0x00000013,0x00000018,0x00000000),to_int3(0x00000007,0x0000001d,0x00000008),to_int3(0x00000009,0x00000021,0x00000018),
    to_int3(0x00000007,0x00000020,0x0000001d),to_int3(0x00000020,0x00000021,0x00000009),to_int3(0x0000000a,0x00000018,0x00000013),to_int3(0x00000013,0x0000001b,0x00000012),to_int3(0x00000008,0x0000001c,0x0000001b),to_int3(0x0000001d,0x00000020,0x0000001f),to_int3(0x0000000a,0x00000013,0x0000000b),to_int3(0x00000009,0x00000018,0x0000000d),to_int3(0x0000000d,0x00000020,0x00000009),to_int3(0x0000001d,0x0000001e,0x00000008),to_int3(0x0000000b,0x00000013,0x00000012),to_int3(0x0000001b,0x0000001c,0x00000012),to_int3(0x0000000d,0x00000018,0x0000000a),to_int3(0x0000001f,0x00000020,0x0000000d),to_int3(0x00000008,0x0000001e,0x0000001c),to_int3(0x0000001d,0x0000001f,0x00000014),
    to_int3(0x0000000b,0x00000012,0x0000000f),to_int3(0x00000014,0x0000001e,0x0000001d),to_int3(0x0000000b,0x00000010,0x0000000a),to_int3(0x00000012,0x0000001c,0x00000011),to_int3(0x0000000a,0x00000010,0x0000000d),to_int3(0x0000001c,0x0000001e,0x00000014),to_int3(0x0000000f,0x00000012,0x00000011),to_int3(0x00000011,0x0000001c,0x00000014),to_int3(0x0000000f,0x00000010,0x0000000b),to_int3(0x00000014,0x0000001f,0x0000000e),to_int3(0x0000000e,0x0000001f,0x0000000d),to_int3(0x0000000f,0x00000011,0x00000010),to_int3(0x00000011,0x00000014,0x0000000e),to_int3(0x0000000d,0x00000010,0x0000000c),to_int3(0x00000010,0x00000011,0x0000000c),to_int3(0x0000000c,0x0000000e,0x0000000d),
    to_int3(0x0000000c,0x00000011,0x0000000e)};


   const float FARAWAY=1e30;
   
   core_color = to_float3(1.0f,0.0f,0.0f);
   bbox_min = to_float3(-0.429853f,-0.406522f,-0.147051f);
   bbox_max = to_float3(0.429853f,0.406522f,0.147051f);
 

float IIIIIIIIIIIIIIIIIIIIIIIIIIIIIII;
    
   bool animate = (_sinf(iTime/2.0f) < 0.0f); 
   if(animate) {
      refract_index = 1.2f;
      scale      = 5.0f+5.0f*beat(iTime);
      extinction = 0.5f*(1.0f + _sinf(iTime*6.0f));
      smoothness = beat(iTime); // 0.5f*(1.0f + _sinf(iTime*3.0f));        
   } else {
      refract_index = 2.4f;
      scale = 10.0f;  
      extinction = 0.0f;  
      smoothness = 0.0f;          
   }
    
   float alpha = fixed_cam ? 0.0f : iTime;  
   float c = _cosf(alpha);
   float s = _sinf(alpha);
   Camera C = {
              to_float3(20.0f*s, 1.5f, 20.0f*c),
              to_float3(0.0f, 0.0f, 0.0f),
              30.0f       
              };
   Ray R = launch(C, fragCoord);
   Intersection I = intersection(FARAWAY);
   raytrace_mesh(R, &I, triangles);
 
   if(I.t == FARAWAY) {
      swi3(fragColor,x,y,z) = sky(R, iTime, iChannel0); 
    } else {        
      float fresnel = 1.0f + dot(R.Dir,I.N) / length(R.Dir);
      fresnel = clamp(fresnel, 0.0f, 1.0f); 
      swi3S(fragColor,x,y,z, reflect_coeff * fresnel * sky(reflect_ray(R,I), iTime, iChannel0)); 
       
      if(dispersion) {
         // Trace three rays (red, green, blue), change refraction index
         // for each of them.
         float base_refract_index = refract_index;
         Ray R0 = R;
         Intersection I0 = I;
         refract_index = base_refract_index;
         float l = multi_refract(R,&I, FARAWAY, triangles);
         float d = _expf(-l * extinction);
         float r = sky(R, iTime, iChannel0).x;
         I = I0; R = R0; refract_index = base_refract_index + 0.03f;
         multi_refract(R,&I,FARAWAY, triangles); 
         float g = sky(R, iTime, iChannel0).y; 
         I = I0; R = R0; refract_index = base_refract_index + 0.06f;
         multi_refract(R,&I,FARAWAY, triangles); 
         float b = sky(R, iTime, iChannel0).z;  
         // Assemble final color using the contributions from the
         // red, green and blue rays.
         swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + _mix(core_color, to_float3(r,g,b), d));
      } else {
         float l = multi_refract(R,&I,FARAWAY, triangles);
         float d = _expf(-l * extinction);
         swi3S(fragColor,x,y,z, swi3(fragColor,x,y,z) + _mix(core_color, sky(R, iTime,iChannel0), d));
      }       
    }

  SetFragmentShaderComputedColor(fragColor);
}