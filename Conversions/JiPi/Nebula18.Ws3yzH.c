
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// KeyParam(ascii) gives value of tuner #ascii
// KeyToggle(ascii) gives toggle state for key #ascii
// KeyParam(0) tells whether any key is currently pressed (e.g. to prevent mouse-move object) 
//#define KeyParam(ascii)    texelFetch( iChannel0, to_int2(ascii,0), 0 ).w 
//#define KeyToggle(ascii) ( texelFetch( iChannel0, to_int2(ascii+256,0), 0 ).w > 0.5f )

#define KeyParam(ascii)    texture( iChannel0, (make_float2(to_int2(ascii,0))+0.5f)/iResolution).w 
#define KeyToggle(ascii) ( texture( iChannel0, (make_float2(to_int2(ascii+256,0))+0.5f)/iResolution ).w > 0.5f )



#define PI           3.1415926

//most of "noise stuff" comes from iq

/* discontinuous pseudorandom uniformly distributed in [-0.5f, +0.5]^3 */
__DEVICE__ float3 random3(float3 c) {
  float j = 4096.0f*_sinf(dot(c,to_float3(17.0f, 59.4f, 15.0f)));
  float3 r;
  r.z = fract(512.0f*j);
  j *= 0.125f;
  r.x = fract(512.0f*j);
  j *= 0.125f;
  r.y = fract(512.0f*j);
  return r-0.5f;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333f;
const float G3 =  0.1666667f;

__DEVICE__ float3 hash( float3 p ) // replace this by something better
{
  p = to_float3( dot(p,to_float3(127.1f,311.7f, 74.7f)),
                 dot(p,to_float3(269.5f,183.3f,246.1f)),
                 dot(p,to_float3(113.5f,271.9f,124.6f)));

  return -1.0f + 2.0f*fract_f3(sin_f3(p)*43758.5453123f);
}

/* 3d simplex noise */
__DEVICE__ float snoise(float3 p) {
   /* 1.0f find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
   /* calculate s and x */
   float3 s = _floor(p + dot(p, to_float3_s(F3)));
   float3 _x = p - s + dot(s, to_float3_s(G3));
   
   /* calculate i1 and i2 */
   float3 e = step(to_float3_s(0.0f), _x - swi3(_x,y,z,x));
   float3 i1 = e*(1.0f - swi3(e,z,x,y));
   float3 i2 = 1.0f - swi3(e,z,x,y)*(1.0f - e);
     
   /* x1, x2, x3 */
   float3 x1 = _x - i1 + G3;
   float3 x2 = _x - i2 + 2.0f*G3;
   float3 x3 = _x - 1.0f + 3.0f*G3;
   
   /* 2.0f find four surflets and store them in d */
   float4 w, d;
   
   /* calculate surflet weights */
   w.x = dot(_x, _x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   /* w fades from 0.6f at the center of the surflet to 0.0f at the margin */
   w = _fmaxf(to_float4_s(0.6f) - w, to_float4_s(0.0f));
   
   /* calculate surflet components */
   d.x = dot(random3(s), _x);
   d.y = dot(random3(s + i1), x1);
   d.z = dot(random3(s + i2), x2);
   d.w = dot(random3(s + 1.0f), x3);
   
   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;
   
   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}

__DEVICE__ float noise( in float3 p )
{
  float3 i = _floor( p );
  float3 f = fract_f3( p );
  
  float3 u = f*f*(3.0f-2.0f*f);

    return _mix( _mix( _mix( dot( hash( i + to_float3(0.0f,0.0f,0.0f) ), f - to_float3(0.0f,0.0f,0.0f) ), 
                             dot( hash( i + to_float3(1.0f,0.0f,0.0f) ), f - to_float3(1.0f,0.0f,0.0f) ), u.x),
                       _mix( dot( hash( i + to_float3(0.0f,1.0f,0.0f) ), f - to_float3(0.0f,1.0f,0.0f) ), 
                             dot( hash( i + to_float3(1.0f,1.0f,0.0f) ), f - to_float3(1.0f,1.0f,0.0f) ), u.x), u.y),
                 _mix( _mix( dot( hash( i + to_float3(0.0f,0.0f,1.0f) ), f - to_float3(0.0f,0.0f,1.0f) ), 
                             dot( hash( i + to_float3(1.0f,0.0f,1.0f) ), f - to_float3(1.0f,0.0f,1.0f) ), u.x),
                       _mix( dot( hash( i + to_float3(0.0f,1.0f,1.0f) ), f - to_float3(0.0f,1.0f,1.0f) ), 
                             dot( hash( i + to_float3(1.0f,1.0f,1.0f) ), f - to_float3(1.0f,1.0f,1.0f) ), u.x), u.y), u.z );
}

__DEVICE__ float cloud( in float3 p )
{
  float3 q = p - to_float3(0.0f,0.1f,1.0f);
  float f;
    f  = 0.50000f*noise( q ); q = q*2.02f;
    f += 0.25000f*noise( q ); q = q*2.03f;
    f += 0.12500f*noise( q ); q = q*2.01f;
    f += 0.06250f*noise( q ); q = q*2.02f;
    f += 0.03125f*noise( q );
  return clamp( 1.5f - p.y - 2.0f + 1.75f*f, 0.0f, 1.0f );
}
/////////////////////

//Transformations

//translation

__DEVICE__ mat4 translate(float3 k)
{
  float tttttttttttttttttttttt;
    mat4 mat = to_mat4_f4(
                      to_float4(1.0f, 0.0f, 0.0f, 0.0f),
                      to_float4(0.0f, 1.0f, 0.0f, 0.0f), 
                      to_float4(0.0f, 0.0f, 1.0f, 0.0f),
                      to_float4_aw(k, 1.0f) );
    
    return mat;
}



__DEVICE__ mat2 rot2(float angle) {
    float s = _sinf(angle);
    float c = _cosf(angle);
    return to_mat2( c, -s, s, c);
}

//rotation around the x axis
__DEVICE__ mat3 rotateX(float degree)
{
   float rad = PI*degree/180.0f;
   mat3 rot = to_mat3(1.0f, 0.0f, 0.0f,
                      0.0f, _cosf(rad), -_sinf(rad),
                      0.0f, _sinf(rad), _cosf(rad));
    return rot;
}

//rotation axis-angle
__DEVICE__ mat4 rotation_matrix(float3 axis, float angle)
{
    axis = normalize(axis);
    float s = _sinf(angle);
    float c = _cosf(angle);
    float oc = 1.0f - c;
    return to_mat4(oc * axis.x * axis.x + c,           oc * axis.x * axis.y - axis.z * s,  oc * axis.z * axis.x + axis.y * s,  0.0f,
                   oc * axis.x * axis.y + axis.z * s,  oc * axis.y * axis.y + c,           oc * axis.y * axis.z - axis.x * s,  0.0f,
                   oc * axis.z * axis.x - axis.y * s,  oc * axis.y * axis.z + axis.x * s,  oc * axis.z * axis.z + c,           0.0f,
                   0.0f,                               0.0f,                               0.0f,                               1.0f);
}

/////////////////////
struct Ray
{
    float3 origin; //origin
    float3 dir; //direction of the ray
};

// Sphere intersection
__DEVICE__ float intersect_sphere( Ray r, float3 sphere, float rad )
{
  float3 oc = r.origin - sphere;
  float b = dot( oc, r.dir );
  float c = dot( oc, oc ) - rad*rad;
  float h = b*b - c;
  if( h<0.0f ) return -1.0f;
  return -b - _sqrtf( h );
}
///////////////////////////
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: RGBA Noise Medium' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void Nebula18Fuse__Buffer_A(float4 O, float2 U, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(keyToggle, 0);
    CONNECT_CHECKBOX1(keyDown, 0);
    CONNECT_CHECKBOX2(keyClick, 0);
  
float AAAAAAAAAAAAAAAAAAAAA;  
    U+=0.5f;
  
    float2 R = iResolution; 

    // --- .rgb: backbround sky. [ do we really need to recompute this at every frame ? ]
    
    O = texture(iChannel1,U/256.0f); 
  //O = to_float4_aw( _powf( O.x, 20.0f ) );             // BW stars
  //O = _powf( O, to_float4(20) );                       // color stars
    O *= _powf( _fmaxf(O.x,_fmaxf(O.y,O.z)), 100.0f);    // faint color stats
    
    // --- .w: could be used to implement parameter persistant states ( for mouse or keyboard tuning ).
    if (U.y>0.5f) 
    {
      SetFragmentShaderComputedColor(O);    
      return;
    }
 
    if (iFrame==0) {                         // init values
        O.w = 0.5f;
                    
    } else {                                 // tuning
        //O.w = texelFetch(iChannel0, to_int2(U), 0).w;
        O.w = texture(iChannel0, U/iResolution).w;

//#define keyToggle(ascii)  ( texelFetch(iChannel3,to_int2(ascii,2),0).x > 0.0f)
//#define keyDown(ascii)    ( texelFetch(iChannel3,to_int2(ascii,1),0).x > 0.0f)
//#define keyClick(ascii)   ( texelFetch(iChannel3,to_int2(ascii,0),0).x > 0.0f)
        
        if (U.x<256.0f) {
            if ( keyClick && iMouse.z>0.0f )
                O.w = iMouse.y/R.y; }        // tune value of parameter #ascii
        else if(U.x<512.0f) 
            O.w = (float)(keyToggle);        // register the toggle on/off for #ascii
       
                                             // -> use KeyParam(ascii) to get it
        if (U.x<1.0f) {                      //        KeyParam(0) tells where any key is used
            O.w = 0.0f;
            for (int i=0; i<256; i++) 
                if(keyClick) { O.w = 1.0f; break; }
        }
    }

  SetFragmentShaderComputedColor(O);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: RGBA Noise Small' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


// Fork of "Nebula 17" by Leria. https://shadertoy.com/view/3ljfDd
// 2020-09-17 15:21:56

//Free for any use, just let my name appears or a link to this shader

//animation 0 or 1
#define ANIM          1

#define RADIUS        4.0f
#define GRAV_CONST    6.674
#define THICK         2.0f
#define DISTORSION    6.67f
#define  MASS         0.1f
    
#define ALPHA         20.0f
#define COLOR         1   

#define DISPERSION_VELOCITY  0.15f

//Set High definition to 1 for more details (sort of LOD) else 0 :
#define HIGH_DEF      1
//----> in HIGH_DEF mode, you can
//choose the nathure of noise ADDITIVE 1 = additive noise || 0 = multiplicative
#define ADDITIVE      0


//Stretch or not the colored volume
#define STRETCH       1

//////////////////////////////////////////////////////////////////

#define GaussRand( rand1, rand2 ) \
    ( _sqrtf( - 2.0f*_logf(_fmaxf(1e-5,rand1)) ) * to_float2 ( _cosf( 6.28f*(rand2) ),_sinf( 6.28f*(rand2) ) ) )

#define GaussNoise(p)  GaussRand( 0.5f+0.01f*snoise(p), 0.5f+0.5f*snoise(p+17.12f) ).x

#define f(x) (0.5f+0.5f*_cosf(x))
#define Pnoise(p) (2.0f* (clamp( noise(p)/0.122f +0.5f ,0.0f,1.0f)) )
//#define Psnoise(p) ( 2.0f*( _expf( snoise(p)) ) )
//#define Psnoise(p)   2.0f*(  snoise(p) + 1.0f )
//#define Psnoise(p)   _fmaxf(0.0f, 1.0f + 1.0f*GaussNoise(p) )
//#define Psnoise(p)   _fmaxf(0.0f, 0.8f + 1.8f*snoise(p) )
  #define Psnoise(p)   _fmaxf(0.0f, 1.0f + 0.4f*GaussNoise(p) )

#define  rnd(v)  fract(_sinf( v * to_float2(12.9898f, 78.233f) ) * 43758.5453f)
#define srnd(v) ( 2.0f* rnd(v) - 1.0f )

struct Camera
{
    float3 pos; //position
    float3 target; //focal point = target point
    float3 forward;
    float3 right;
    float3 up;
    
    mat4 view;
};

struct Matter
{
    float3 pos; //position
    float radius; //accretion disk
    float mass;
};

///////////////////////////////////////////////
//Matter m;
//Camera cam;
///////////////////////////////////////////////

//float3 I = to_float3(1.0f, 0.0f, 0.0f);   //x axis
//float3 J = to_float3(0.0f, 1.0f, 0.0f);  //y axis
//float3 K = to_float3(0.0f, 0.0f, 1.0f);  // z axis
//float3 lightpos = to_float3_s(0.0f);

//#define Bnoise(x) _fabs(noise(x))

//float3 stretching  = to_float3_s( 1.0f ); 

__DEVICE__ float fbm_add( float3 p, float level, float3 stretching ) { // in [-1,1]
    
    float f = 0.0f;
    float3 s = to_float3_s(2.0f);
    #if STRETCH
     p *= stretching*  to_float3_s(   1.0f/8.0f );
    s = 2.0f/pow_f3(stretching,to_float3_s(1.0f/4.0f));
    #endif
    
    for(float l = 0.0f ; l < level ; l++)
    {
       f += (1.0f/_exp2f(l))*snoise(p);   p = p*s;
    }

     return f;
}

__DEVICE__ float fbm_mul( float3 p, float3 stretching ) { // in [-1,1]
    
    float f;
    float3 s = to_float3_s(2.0f);
    #if STRETCH
     p *= stretching* 1.0f/8.0f;
     s = 2.0f/pow_f3(stretching,to_float3_s(1.0f/4.0f));
    #endif

   
    f = Psnoise(p); p = p*s;

    #if HIGH_DEF
    f *=  Psnoise( p ); p = p*s;
    f *=  Psnoise( p ); p = p*s;
    f *=  Psnoise( p ); p = p*s;
    f *=  Psnoise( p ); 
    #endif
    return f;
}

__DEVICE__ float fbm(float3 p, float3 stretching)
{
 
    #if ADDITIVE
    return fbm_add(p, 5.0f, stretching);
    #else
     return fbm_mul(p, stretching);
    #endif
    
}

/* Transparency */
__DEVICE__ float current_transparency(float dist, float material_coef, float density)
{
   return _expf(-dist*material_coef*density); 
}

__DEVICE__ float current_opacity(float t)
{
   return 1.0f-t;   
}

__DEVICE__ float3 current_opacity(float3 rgb_o)
{
   return 1.0f-rgb_o; 
}

#define transp current_transparency

#define ROT rotation_matrix

//end of rotation

__DEVICE__ float3 ray_interpolation(Ray r, float t) 
{
   return (r.origin + r.dir*t);   
}

__DEVICE__ Matter set_matter(float3 pos, 
                float mass,
                float radius)
{
  float zzzzzzzzzzzzzzzzzz;
   //m = Matter(pos, radius, mass);
   Matter m = {pos, mass, radius};
}

__DEVICE__ float sdf_sphere(float3 pXp0, float radius)
{
    return (length(pXp0) - (radius));
}

//__DEVICE__ void init_matter(void)
//{
//   set_matter(to_float3(0.0f, 0.0f, 0.0f), MASS, RADIUS);
//}

__DEVICE__ Camera set_camera(float3 pos, float3 target)
{
    Camera cam;
    cam.pos = pos;
    cam.target = target;
    cam.forward = normalize(pos-target);
    cam.right = cross(normalize(to_float3(0.0f, 1.0f, 0.0f)), cam.forward);
    cam.up = cross(cam.forward, cam.right);
        
    cam.view = to_mat4_f4(to_float4_aw(cam.right, 0.0f), to_float4_aw(cam.up, 0.0f), to_float4_aw(cam.forward, 0.0f), to_float4_s(1.0f) );
    
    return cam;
}

//__DEVICE__ void init_camera(void)
//{
//    init_matter();
//    set_camera(to_float3(0.0f, 0.0f, 3.8f), m.pos); 
//}

__DEVICE__ float energy_t_r(float velocity, float typical_scale)
{
   return 0.5f*(velocity*velocity*velocity)/typical_scale;   
}

__DEVICE__ float local_velocity( float3 p, float disp_turb, float3 stretching)
{
  float disp_rate = disp_turb*fbm(p*2.0f, stretching); //generation of a local dispersion = turbulence * rate

  return 1.60f*DISPERSION_VELOCITY *disp_rate; 
}

__DEVICE__ void ray_march_scene(Ray r, float k, inout float3 *c, inout float3 *transp_tot, Matter m, Camera cam, float3 stretching, float3 lightpos, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float uniform_step = k;
    float jit = 1.0f;
    //jit = 50.0f*fract(1e4*_sinf(1e4*dot(r.dir, to_float3(1.0f, 7.1f, 13.3f))));
   
    float t_gen = 1.0f;

    float param_t = intersect_sphere(r, m.pos, RADIUS);
    if(param_t <= -1.0f)
        return;
    float3 p = ray_interpolation(r, k*jit);        
     
    float vt = -1.0f;
    //rgb transparency               
    
    float3 t_acc = to_float3_s(1.0f);  // accumulated parameters for transparency
    
    for(int s=0; s < 120; s++)
    {               
        float3 dU = to_float3_s(0.0f);

        float dist_dist = dot(p-cam.pos, p-cam.pos);
        float dist_center = length(m.pos-cam.pos);
        float3 center = p-m.pos;

        float3 rgb_t = to_float3_s(0.0f);
        
        //if too far, then big step        
        float d = length(center)-RADIUS-0.5f-jit*k;

        if(length(center)-RADIUS < 0.0f)
        {
            
            float anim = 1.0f;
            #if ANIM      
            //anim = iTime;
            
            #endif
                                                 // --- textured bubble model
            
            float rad_bubl = 2.36f; //radius of bubble, also control the opening of the bubble
            float3 pB = to_float3_s(0); // to_float3(-1.0f,1.0f,-1.0f);//RADIUS/2.0f);
            float r_p = length(p-pB)/2.0f*RADIUS;
            float d = 1.0f +0.1f*( length(p-pB) / (rad_bubl) -1.0f);
            
            //if ( length(p-pB) < rad_bubl ) (*c).x += 0.03f; // hack to display the bubble

            
            float3 dp = to_float3_s(0.0f);
            dp += d*(p-pB);      
            
            float size = length((p-pB))/RADIUS;

#define SQR(x) ( (x)*(x) ) 
            //push bubble
            //float l = _fmaxf(0.0f, 1.0f-d*d);
            float l = _expf(-0.5f*SQR(d/2.0f));
#if 0
            float n = Psnoise( (p-pB+dp)*l)*(_fmaxf(0.0f, d)*l) ;
            float n = 0.5f+0.5f*snoise( (p-pB+dp)*l)*(_fmaxf(0.0f, d)*l) ;

            float mask1 = smoothstep(n,
                                    0.5f*RADIUS,
                                    RADIUS-length(center)
                                    );
            //mask = n;
#else
            //rad_bubl -= 0.2f ;
            float dr = fbm_add( p*5.2f - 124.17f, 4.0f, stretching),
                  de = noise( p - 98.12f ),
                 
            mask = smoothstep ( 0.2f+0.0f*de, 0.0f, _fabs( ( length(p)-(rad_bubl))/rad_bubl  -1.8f*dr )   ) ;
#endif
            //mask = 1.0f;
            
                                                 // --- local transparency 
            float dispersion_turbulence =  clamp( mask, 0.0f,1.0f );            
                                    
            //DISPERSION_VELOCITY is the average dispersion velocity
            float velocity = local_velocity(p, dispersion_turbulence, stretching); //local velocity (sometimes called sigma_v)
            #define VT velocity
            
            //energy 
            float scale = 0.1f;
            float energy_transfer_rate = energy_t_r(VT, scale); //energy transfer rate by unit of mass as a dust grain density (transfer function)

            //primitive of the transfer function
#define INT_E(v)  (0.5f*(v*v*v*v)/(4.0f*scale))       
            
            //preintegration formula
#define PREINT_E(d0, d1)  ((INT_E(d1)-INT_E(d0))/(d1-d0))          
            
            float3 absorb_coef = to_float3(0.1f, 0.1f, 7.0f)*10.0f;   // <<< $PHYS $PARAM (well, part of sigma_t)
            absorb_coef *= _expf( 10.0f*(KeyParam(64+1)-0.5f) ); // $TUNE
            
            float3 sigma_t = to_float3_s(0);
            if(vt <= 0.0f)
            {
                sigma_t = absorb_coef*k*energy_transfer_rate;
                vt = velocity;                
            }
            
            #define PREINTEGRATION  1
            
            #if PREINTEGRATION
            else
                
            {
                sigma_t = absorb_coef*k*(float)(s)*PREINT_E(vt, VT);
                vt = velocity;
            }
            
            #else
            vt = 0.0f;
            sigma_t = absorb_coef*k*energy_transfer_rate;

            #endif
            
            sigma_t /=10.0f;
            
            rgb_t = exp_f3(-sigma_t);    // <<< local transparency
            
float ssssssssssssssssss;            
            
            float3 col_loc = to_float3(1.0f, 0.15f, 0.2f)/(sigma_t+1e-5);  // <<< $PHYS $PARAM albedo, = sigma_s/sigma_t
            if KeyToggle(64+3) col_loc *= absorb_coef; //  $TUNE  control sigma_s rather than albedo
            
            
            // attention: sigma_t = sigma_a + sigma_s , albedo = sigma_s/sigma_t
            //        -> sigma_a = sigma_t ( 1 - albedo ) must be physical. Or always ok ?
            
            // e.g. here: sigma_a = to_float3(0.1f, 0.6f, 10) * Etr(VT,10) * ( 1 - to_float3(0.8f, 0.5f, 0.1f) )
            //            E_tr = 0.5f VT³/10,  VT = 1.6f * 0.15f *256.0f*mask*fbm(p*4.0f)
            //                 = 12000 *(mask*fbm)³
            // note that Dl = k has no unit: part of big coef should go there
            

            float epsilon = k/10.0f;           // --- local lighting
            float3 L = normalize(p-lightpos);
            #define val(x,y,z) energy_t_r( local_velocity(p+epsilon*to_float3(x,y,z), dispersion_turbulence, stretching) , scale)
#if 1
            float3 N =   to_float3( val(1,0,0), val(0,1,0), val(0,0,1) )
                        - energy_transfer_rate;
            N = normalize(N+1e-5);
            float dif = _fabs(dot(N, L));
#else                  
           
            float dif = _fabs(clamp(( val(0,0,0)      // energy_transfer_rate 
                                   - val(L.x,L.y,L.z) // energy_t_r( local_velocity(p+epsilon*L, dispersion_turbulence, stretching) , scale)
                                  )/epsilon
                              , -1.0
                              , 1.0f  ));
#endif
                                           // above : diff = _fabs(Lambert)
            //dif = 0.75f* ( 1.0f + dif*dif ); // Rayleigh phase function

#define Gauss(x,s) 1.0f/(std_dev*2.51f)*_expf(-(X*X)/(2.0f*(std_dev*std_dev)))
            float meansunlight = 0.7f;
            float std_dev = 0.7f;
            float X = (size-meansunlight);
            float L0 = 7.0f;
            L = p-lightpos;
            float source = 1.0f/ dot(L/L0,L/L0)* _expf(- 4.0f*_fmaxf(0.0f, length(L)  - (rad_bubl+3.0f) ) ) ;
          //float source = 5.0f* _expf(- 2.0f*_fmaxf(0.0f, length(L) - (rad_bubl-0.2f) ) ) ;
             float sun = source*(0.1f/(length(m.pos-p)*length(m.pos-p))),//* smoothstep(0.0f, 1.0f/size, Gauss(X,std_dev) ),
                shadow = 1.0f,
            reflec = dif;
            
            float3 emission = to_float3(0.2f, 0.5f, 1.0f);   // <<< $PARAM $PHYS  
            
                                  // --- add current voxel contribution to ray
     //     t_acc *= (rgb_t);         
            *c += t_acc* (col_loc* reflec * sun * shadow) *  (1.0f-rgb_t);
      //    *c += t_acc * col_loc* (prof) *0.3f; // * rgb_t;
            t_acc *= (rgb_t);    
            //*c = to_float3_aw();
        }            

        //if it will never be in the shape anymore, return;
        
        if(length(center)-0.5f < 0.0f)

        
        if(length(p-cam.pos) >(dist_center+m.radius) || 
           (t_acc.x+t_acc.y+t_acc.z < 0.001f && t_acc.x+t_acc.y+t_acc.z > 40.0f ))
        {
           break;
        }
        
        p += r.dir*k;

        k = uniform_step;
    }
    
    //*c =float(s)/to_float3(50,150,20); return;

    *transp_tot = t_acc;
}
  
__KERNEL__ void Nebula18Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    
    
    Matter m  = {to_float3(0.0f, 0.0f, 0.0f), MASS, RADIUS};
    Camera cam;

    cam = set_camera(to_float3(0.0f, 0.0f, 3.8f), m.pos); 
float IIIIIIIIIIIIIIIII;

    float3 lightpos = to_float3_s(0.0f);
    float3 stretching  = to_float3_s( 1.0f ); 
    
    
    float2 R = iResolution, 
        uv = (2.0f*fragCoord- R )/R.y,
         M =  swi2(iMouse,x,y)/R;
        
    float degree = 2.0f*PI * M.x - PI;
    float degree2 = 2.0f*PI * M.y - PI;
    if ( iMouse.z<=0.0f || KeyParam(0)>0.0f ) degree = 0.0f/2.0f, degree2 = 0.0f;
   
    float3 color = to_float3_s(0.0f), transp_tot = to_float3_s(1);
    float3 ray_dir = to_float3_aw(uv, -1.0f);

    m.pos = normalize(to_float3(-10, 20.0f, m.pos.z));
    
 // float2 m = 2.0f*PI * iMouse.xy/R - PI;
    float3 C = cam.pos, ray = normalize(ray_dir);
    swi2S(C,x,z, mul_f2_mat2(swi2(C,x,z) , rot2(degree)));
    swi2S(C,y,z, mul_f2_mat2(swi2(C,y,z) , rot2(degree2)));
    swi2S(ray,x,z, mul_f2_mat2(swi2(ray,x,z) , rot2(degree))); 
    swi2S(ray,y,z, mul_f2_mat2(swi2(ray,y,z) , rot2(degree2)));
    
    cam.pos = C;
    
    Ray raypar = {C, normalize(ray)};
    
    ray_march_scene(raypar, 0.1f, &color, &transp_tot, m, cam, stretching, lightpos, iResolution, iChannel0);  
    float3 sky = to_float3_s(0);
    sky =   0.5f* swi3(texture(iChannel0,fragCoord/R+to_float2(degree,degree2)),x,x,x);//rrr;
  //sky =   0.6f*_powf(texture(iChannel1,fragCoord*2.0f+to_float2(degree,degree2)).rgb, to_float3(7));
  //sky =   _fmaxf(texture(iChannel0,fragCoord/256.0f+to_float2(degree,degree2)).rrr -0.8f,0.0f)/0.2f;
    color += transp_tot * sky;
    
    fragColor = to_float4_aw(pow_f3(color, to_float3_s(1.0f/2.2f)), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}