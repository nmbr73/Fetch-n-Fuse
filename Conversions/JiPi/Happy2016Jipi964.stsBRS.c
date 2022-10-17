
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 3' to iChannel1
// Connect Image 'Texture: Pebbles' to iChannel0


// "HAPPY 2016!" by Martijn Steinrucken aka BigWings - 2015
// countfrolic@gmail.com
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// Email:countfrolic@gmail.com Twitter:@The_ArtOfCode

// Use these to change the effect

// if you have a kick ass GPU then double both of these
#define NUM_SPARKLES 75.0f
#define NUM_SUB_SPARKLES 3.0f

#define SUB_SPARKLE_CHANCE 0.4f
#define PRIMARY_PARTICLE_COLOR to_float3(1.0f, 0.8f, 0.5f)
#define SECONDARY_PARTICLE_COLOR to_float3(1.0f, 0.5f, 0.3f)
#define MOTION_BLUR_AMOUNT 0.04f
#define SLOW_MOTION_SPEED 0.05f
#define SLOWMO_CYCLE_DURATION 20.0f
#define NORMAL_MOTION_SPEED 0.9f
#define DOF to_float2(1.0f, 1.5f)
#define MIN_CAM_DISTANCE 1.5f
#define MAX_CAM_DISTANCE 7.0f


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//float CAMERA_DISTANCE;


#define PI 3.1415f
#define S(x,y,z) smoothstep(x,y,z)
#define B(x,y,z,w) S(x-z, x+z, w)*S(y+z, y-z, w)
#define _saturatef(x) clamp(x,0.0f,1.0f)
__DEVICE__ float dist2(float2 P0, float2 P1) { float2 D=P1-P0; return dot(D,D); }
__DEVICE__ float DistSqr(float3 a, float3 b) { float3 D=a-b; return dot(D, D); } 


#define pi     3.141592653589793238f
#define twopi  6.283185307179586f

__DEVICE__ float4 Noise401( float4 _x ) { return fract_f4(sin_f4(_x)*5346.1764f); }
__DEVICE__ float4 Noise4( float4 _x )   { return fract_f4(sin_f4(_x)*5346.1764f)*2.0f - 1.0f; }
__DEVICE__ float Noise101( float _x )   { return fract(_sinf(_x)*5346.1764f); }

#define MOD3 to_float3(0.1031f,0.11369f,0.13787f)
//  3 out, 1 in... DAVE HOSKINS
__DEVICE__ float3 hash31(float p) {
   float3 p3 = fract(to_float3_s(p) * MOD3);
   p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
   return fract_f3(to_float3((p3.x + p3.y)*p3.z, (p3.x+p3.z)*p3.y, (p3.y+p3.z)*p3.x));
}
__DEVICE__ float hash12(float2 p){
    float3 p3  = fract_f3((swi3(p,x,y,x)) * MOD3);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return fract((p3.x + p3.y) * p3.z);
}

struct ray {
    float3 o;
    float3 d;
};
//ray e;        // the eye ray

struct camera {
    float3 p;      // the position of the camera
    float3 forward;  // the camera forward vector
    float3 left;    // the camera left vector
    float3 up;    // the camera up vector
  
    float3 center;  // the center of the screen, in world coords
    float3 i;      // where the current ray intersects the screen, in world coords
    ray ray;    // the current ray: from cam pos, through current uv projected on screen
    float3 lookAt;  // the lookat point
    float zoom;    // the zoom factor
};
//camera cam;

__DEVICE__ mat4 CamToWorldMatrix(camera c) {
    float3 x = c.left;
    float3 y = c.up;
    float3 z = c.forward;
    float3 p = c.p;
    
    return to_mat4( 
        x.x, x.y, x.z, 0.0f,
        y.x, y.y, y.z, 0.0f,
        z.x, z.y, z.z, 0.0f,
        p.x, p.y, p.z, 1.0f
    );
}
__DEVICE__ mat4 WorldToCamMatrix(camera c) {
    float3 x = c.left;
    float3 y = c.up;
    float3 z = c.forward;
    float3 p = c.p;
    
   return to_mat4( 
        x.x, y.x, z.x, -dot(x, p),
        x.y, y.y, z.y, -dot(y, p),
        x.z, y.z, z.z, -dot(z, p),
         0.0f,  0.0f,  0.0f, 0.0f
    );
}

__DEVICE__ camera CameraSetup(float2 uv, float3 position, float3 lookAt, float zoom, float3 up) {
  
     camera cam;
    
    cam.p = position;
    cam.lookAt = lookAt;
    cam.forward = normalize(cam.lookAt-cam.p);
    cam.left = cross(up, cam.forward);
    cam.up = cross(cam.forward, cam.left);
    cam.zoom = zoom;
    
    cam.center = cam.p+cam.forward*cam.zoom;
    cam.i = cam.center+cam.left*uv.x+cam.up*uv.y;
    
    cam.ray.o = cam.p;                   // ray origin = camera position
    cam.ray.d = normalize(cam.i-cam.p);  // ray direction is the vector from the cam pos through the point on the imaginary screen
    
    return cam;
}

__DEVICE__ float within(float2 v, float t) {
  return (t-v.x) / (v.y-v.x);
}


__DEVICE__ float4 tex3D( in float3 pos, in float3 normal, __TEXTURE2D__ sampler ) {
    // by reinder. This is clever as two hamsters feeding three hamsters.
    
  return   texture( sampler, swi2(pos,y,z) )*_fabs(normal.x)+ 
           texture( sampler, swi2(pos,x,z) )*_fabs(normal.y)+ 
           texture( sampler, swi2(pos,x,y) )*_fabs(normal.z);
}

// DE functions from IQ
// https://www.shadertoy.com/view/Xds3zN

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r ) {
  float3 pa = p-a, ba = b-a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}

__DEVICE__ float4 map( in float3 p, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    // returns a float3 with x = distance, y = bump, z = mat transition w = mat id
    
    float t = iTime*0.1f;
    float2 fryInterval = to_float2(2.0f, 2.2f);
    float transition = _saturatef(within(fryInterval, p.y));
    transition = smoothstep(0.0f, 1.0f, transition);
    
    float3 pos = p*3.0f;
    pos.y -= t;
    
    float3 normal = normalize(to_float3(p.x, 0.0f, p.z));
    
    float newBump = tex3D(pos, normal, iChannel1).x*0.003f;
    float burnedBump = tex3D(pos, normal, iChannel0).x*0.05f;
    
    float bump = _mix(newBump, burnedBump, transition);
    
    float d = sdCapsule(p+bump*normal, to_float3(0.0f, -10.0f, 0.0f), to_float3(0.0f, 10.0f, 0.0f), 0.1f);
    
    return to_float4(d, bump, transition, 2.0f);
}

__DEVICE__ float4 castRay( in float3 ro, in float3 rd, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 ) {
    // returns a distance and a material id
    
    float dmin = 1.0f;
    float dmax = 20.0f;
    
    float precis = 0.002f;
    float d = dmin;
    float m = -1.0f;
    float b = 0.0f;
    float t = 0.0f;
    for( int i=0; i<50; i++ )
    {
      float4 res = map( ro+rd*d, iTime, iChannel0, iChannel1 );
      if( res.x<precis || d>dmax ) break;
      d += res.x;
      b = res.y;
      t = res.z;
      m = res.w;
    }

    if( d>dmax ) m=-1.0f;
    return to_float4( d, b, t, m );
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 ) {
  float occ = 0.0f;
    float sca = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float hr = 0.01f + 0.12f*float(i)/4.0f;
        float3 aopos =  nor * hr + pos;
        float dd = map( aopos, iTime, iChannel0, iChannel1 ).x;
        occ += -(dd-hr)*sca;
        sca *= 0.95f;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f );    
}

__DEVICE__ float3 calcNormal( in float3 pos, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 )
{
  float3 eps = to_float3( 0.001f, 0.0f, 0.0f );
  float3 nor = to_float3(
      map(pos+swi3(eps,x,y,y), iTime, iChannel0, iChannel1).x - map(pos-swi3(eps,x,y,y), iTime, iChannel0, iChannel1).x,
      map(pos+swi3(eps,y,x,y), iTime, iChannel0, iChannel1).x - map(pos-swi3(eps,y,x,y), iTime, iChannel0, iChannel1).x,
      map(pos+swi3(eps,y,y,x), iTime, iChannel0, iChannel1).x - map(pos-swi3(eps,y,y,x), iTime, iChannel0, iChannel1).x );
  return normalize(nor);
}


__DEVICE__ float3 ClosestPoint(ray r, float3 p) {
    // returns the closest point on ray r to point p
    return r.o + _fmaxf(0.0f, dot(p-r.o, r.d))*r.d;
}

__DEVICE__ float4 render( in float3 ro, in float3 rd, out float *d, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1 ) {
    // outputs a color
    
    float3 col = to_float3_s(0.0f);
    float4 res = castRay(ro,rd, iTime, iChannel0, iChannel1);
    *d = res.x;  // distance
    float b = res.y;  // bump
    float t = res.z;  // transition
    float m = res.w;  // mat id
    if( m>0.5f )
    {
        float3 pos = ro + *d*rd;
        float3 nor = calcNormal( pos, iTime, iChannel0, iChannel1 );
        float3 ref = reflect( rd, nor );
        
        // material        
        col = to_float3(0.05f,0.08f,0.10f)+_mix(0.35f, 0.1f, t);

        // lighitng        
        float occ = calcAO( pos, nor, iTime, iChannel0, iChannel1 );
        float3  lig = normalize( to_float3(-0.6f, 0.7f, -0.5f) );
        float amb = clamp( 0.5f+0.5f*nor.y, 0.0f, 1.0f );
        float dif = clamp( dot( nor, lig ), 0.0f, 1.0f );
        
        float fade = _saturatef(within(to_float2(2.0f, 3.0f), pos.y));
        fade = smoothstep(0.0f, 1.0f, fade);
        fade = _mix(80.0f, 0.0f, fade);
        float3 afterGlow = _powf(_fabs(b)*fade,2.0f) * to_float3(1.0f, 0.1f, 0.02f)*2.0f;
        float whiteGlow = B(2.18f, 2.45f, 0.05f, pos.y+b*10.0f);

        float3 lin = to_float3_s(0.0f);
        lin += amb;
        lin += dif;
        
        col = col*lin;
        
        col += afterGlow;
        col += whiteGlow;
    }

  return to_float4_aw( _saturatef(col), _saturatef(m) );
}

__DEVICE__ float SineWave(float2 pos, float phase, float frequency, float amplitude, float offset, float thickness, float glow) {
    // returns a sine wave band
    // takes a position from -pi,-pi to pi, pi
        
    float dist = _fabs(pos.y-(_sinf(pos.x*frequency+phase)*amplitude-offset));  // distance to a sine wave
    return smoothstep(thickness+glow, thickness, dist);
}

__DEVICE__ float3 background(ray r, float iTime) {
    float x = _atan2f(r.d.x, r.d.z);    // from -pi to pi  
    float y = pi*0.5f-_acosf(r.d.y);      // from -1/2pi to 1/2pi    
    
    float t = iTime;
    
    float band1 = SineWave(to_float2(x, y), 0.0f, 3.0f, 0.25f, 0.0f, 0.001f, 0.5f);
    
    return  _mix(to_float3(0.3f, 0.02f, 0.03f), to_float3_s(0.0f), band1);
}


__DEVICE__ float3 sparkle(ray r, float3 p, float size, float3 color, camera cam, float CAMERA_DISTANCE) {
    float camDist = length(cam.p-p);
    float focus = smoothstep(DOF.y, DOF.x, _fabs(camDist-CAMERA_DISTANCE));
    
    float3 closestPoint = ClosestPoint(r, p);
    float dist = DistSqr(closestPoint, p)*10000.0f;
   
    size = _mix(size*5.0f, size, focus);
    float brightness = size/dist;
    brightness = clamp(brightness,0.0f, 10.0f);
    
    float bokeh = smoothstep(0.01f, 0.04f, brightness)*_saturatef(dist*0.005f+0.4f)*0.15f;
    
    brightness = _mix(bokeh, brightness, focus);
    return color * brightness;
}

__DEVICE__ float3 sparkles(ray r, float2 uv, float time, float timeFactor, float dist, camera cam, float CAMERA_DISTANCE) {
    float3 col = to_float3_s(0.0f);
    
    float n2 = fract(_sinf(uv.x*123134.2345f)*1231.234255f);
    float n3 = fract(_sinf((n2+uv.y)*234.978f)*789.234f);
    
    float motionBlur = (n3-0.5f)*timeFactor*MOTION_BLUR_AMOUNT;
    
    for(float i=0.0f; i<NUM_SPARKLES; i++) {          
        float t = time+(i/NUM_SPARKLES) + motionBlur;
        float ft = _floor(t);
        t -= ft;
        float3 n = hash31(i+ft*123.324f);      // per particle noise / per cycle noise
        
        
        float3 pStart = to_float3(0.0f, 2.1f+n.y*0.15f, 0.0f);
        pStart.y -= t*t*0.6f;  // gravity
        pStart.y += t;    // account for slow scroll down the stick
        
        float3 pEnd = pStart + (n-0.5f) * to_float3(1.0f, 0.6f, 1.0f)*4.0f;
        float3 p = _mix(pStart, pEnd, t);
         
        if(length(p-cam.p)<dist) {
            float size = _mix(10.0f, 0.5f, smoothstep(0.0f, 0.2f, t)); // in the first 20% it gets smaller very fast
            size *= smoothstep(1.0f, 0.2f, t);          // in the remaining 80% it slowly fades out

            if(t>n.z && _fabs(n.z-0.55f)<SUB_SPARKLE_CHANCE) {
                for(float x=0.0f; x<NUM_SUB_SPARKLES; x++) {
                    float3 ns = hash31(x+i);      // per particle noise
                    float3 sStart = _mix(pStart, pEnd, n.z);
                    float3 sEnd = sStart + (ns-0.5f) *2.0f;
                    float st = _saturatef(within(to_float2(n.z, 1.0f), t));
                    float3 sp = _mix(sStart, sEnd, st);

                    size = _mix(10.0f, 0.5f, smoothstep(0.0f, 0.1f, st));  // explosion in the first 10%
                    size *= smoothstep(1.0f, 0.9f, st);          // fade over the next 90%

                    col += sparkle(r, sp, size, SECONDARY_PARTICLE_COLOR, cam, CAMERA_DISTANCE);
                }
            } else
                 col += sparkle(r, p, size, PRIMARY_PARTICLE_COLOR, cam, CAMERA_DISTANCE);
        }
    }
    
    return col;
}

__DEVICE__ float3 Rainbow(float3 c, float iTime) {
  
    float t=iTime;
    
    //float avg = (c.x+c.y+c.z)/3.0f;
    //c = avg + (c-avg)*_sinf(to_float3(0.0f, 0.333f, 0.666f)+t);
    
    c += sin_f3(to_float3(0.4f, 0.3f, 0.3f)*t + to_float3(1.1244f,3.43215f,6.435f))*to_float3(0.4f, 0.1f, 0.5f);
    
    return c;
}


__KERNEL__ void Happy2016Jipi964Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 0.949f,0.008f,0.290f, 1.0f); 
  
    const float3 up = to_float3(0.0f,1.0f,0.0f);

    float2 uv = (fragCoord / iResolution) - 0.5f;
    uv.y *= iResolution.y/iResolution.x;
    float2 m = swi2(iMouse,x,y)/iResolution;
    
    float t = iTime;
    float timeFactor = fract(t/SLOWMO_CYCLE_DURATION)>0.5f ? SLOW_MOTION_SPEED : NORMAL_MOTION_SPEED;
    t *= timeFactor;
    
    float turn = -m.x*pi*2.0f+iTime*0.1f;
    float s = _sinf(turn);
    float c = _cosf(turn);
    mat3 rot = to_mat3(    c,  0.0f,    s,
                        0.0f,  1.0f, 0.0f,
                           s,  0.0f,   -c);
    
    
    float CAMERA_DISTANCE = _mix(MIN_CAM_DISTANCE, MAX_CAM_DISTANCE, _sinf(iTime*0.0765f)*0.5f+0.5f);
    float3 pos = mul_f3_mat3(to_float3(0.0f, 0.4f, -CAMERA_DISTANCE),rot);
     
    camera cam = CameraSetup(uv, pos, to_float3(0.0f, 2.3f, 0.0f), 1.0f, up);
    
    float3 bg = background(cam.ray,iTime);
    float dist;                    // the distance of the current pixel from the camera
    float4 stick = render(cam.ray.o, cam.ray.d, &dist, iTime, iChannel0, iChannel1);
    dist += 0.08f; // add some distance to make sure particles render on top of the stick when they first come to life
    
    float3 col = _mix(bg, swi3(stick,x,y,z), stick.w);  // composite stick onto bg
    
    col += sparkles(cam.ray, uv, t, timeFactor, dist, cam, CAMERA_DISTANCE);
    
    col = Rainbow(col, iTime);
    
    fragColor = to_float4_aw(col, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}