
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//---------------------------------------------------Constant
#define PI  3.14159265359
#define PI2 6.28318530718
#define PIH 1.57079632679

//star number
#define STAR_COUNT 48

//gravitational constant
#define CONST_G 1.0f

//star mass
#define DEFAULT_MASS 1.0f

//star density
#define STAR_DENSITY 1000.0f

//initial stars area
#define GALAXY_RADIUS 10.0f

//physics step time
#define DELTA_TIME 0.02f

//speed up or down
#define TIME_SCALE 1.0f

//trunc light for render [0,x]
#define RENDER_INTENSITY_MAX 50.0f

//pow
#define RENDER_INTENSITY_POW 0.5f

//IF YOU HAVE SERIOUS Trypophobia, SET IT TO false!
//#define INTERGRATE_STAR_CENTER

//---------------------------------------------------Hash
__DEVICE__ float Hash21(float2 p)
{
  return fract(_sinf(dot(p,to_float2(12.99f,78.23f)))*43758.5453f);
}
__DEVICE__ float2 Hash22(float2 p)
{
  return fract_f2(sin_f2(to_float2(
                                      dot(p,to_float2(127.1f,311.7f)),
                                      dot(p,to_float2(269.5f,183.3f))
                                  ))*43758.5453f);
}
__DEVICE__ float3 Hash23(float2 p)
{
    return fract_f3(sin_f3(to_float3(
                                    dot(p,to_float2(127.1f,311.7f)),
                                    dot(p,to_float2(269.5f,183.3f)),
                                    dot(p,to_float2(12.99f,78.23f))
                                ))*43758.5453f);
}
__DEVICE__ float3 HashS3(float2 h2)
{
    float theta = h2.x*PI2;
    float phi = _acosf(h2.y*2.0f-1.0f);
    return to_float3(
                    _sinf(theta)*_sinf(phi),
                    _cosf(theta)*_sinf(phi),
                    _cosf(phi));
  }
__DEVICE__ float3 HashS3(float3 h3)
{
    return HashS3(swi2(h3,x,y))*_powf(h3.z,1.0f/3.0f);
}

//----------------------------------------------fragCoord=>index=>uv
//thanks to dr2! his "index convert function" helped me
//"Faberge Balls" by dr2
__DEVICE__ int XY2I(float2 xy, float sizeX)
{
    xy = _floor(xy);
    int i = (int)(xy.x + xy.y * sizeX);
    return i;
}
__DEVICE__ float2 I2UV(int i, float2 size)
{
    float fi = (float)(i);
    float2 uv;
    uv.x = mod_f(fi, size.x);
    uv.y = _floor(fi / size.x);
    uv = (uv + 0.5f) / size;
    return uv;
}

//----------------------------------------------Quaternion

//identity
#define QUAT_IDENTITY to_float4(0,0,0,1)

//conjugate (inverse rotation)
__DEVICE__ float4 QuatInv(float4 q){return to_float4_aw(-1.0f*swi3(q,x,y,z),q.w);}

//quaternion multiply
__DEVICE__ float4 QuatMul(float4 q, float4 r)
{
    float4 nq;
    nq.x = q.w * r.x + q.x * r.w + q.y * r.z - q.z * r.y;
    nq.y = q.w * r.y - q.x * r.z + q.y * r.w + q.z * r.x;
    nq.z = q.w * r.z + q.z * r.w - q.y * r.x + q.x * r.y;
    nq.w = q.w * r.w - q.x * r.x - q.y * r.y - q.z * r.z;
    return nq;
}
__DEVICE__ float3 QuatMul(float4 lhs, float3 rhs)
{
    float x1 = lhs.x, y1 = lhs.y, z1 = lhs.z, w1 = lhs.w;
    float x2 = rhs.x, y2 = rhs.y, z2 = rhs.z;
    float nx = w1 * x2 + y1 * z2 - z1 * y2;
    float ny = w1 * y2 - x1 * z2 + z1 * x2;
    float nz = w1 * z2 + x1 * y2 - y1 * x2;
    float nw = x1 * x2 + y1 * y2 + z1 * z2;
    float3 nv;
    nv.x = nw * x1 + nx * w1 - ny * z1 + nz * y1;
    nv.y = nw * y1 + nx * z1 + ny * w1 - nz * x1;
    nv.z = nw * z1 - nx * y1 + ny * x1 + nz * w1;
    return nv;
}
__DEVICE__ float3 RotateFWD(float4 q)
{
    float3 nv;
    nv.x = 2.0f * (q.x * q.z + q.y * q.w);
    nv.y = 2.0f * (q.y * q.z - q.x * q.w);
    nv.z = 1.0f - 2.0f * (q.x * q.x + q.y * q.y);
    return nv;
}

//from euler angles
__DEVICE__ float4 QuatEA(float2 eulers)
{
    float2 eh = eulers * 0.5f;
    float c1 = _cosf(eh.x), c2 = _cosf(eh.y);
    float s1 = _sinf(eh.x), s2 = _sinf(eh.y);
    float4 nq;
    nq.x = s1 * c2;
    nq.y = c1 * s2;
    nq.z = -s1 * s2;
    nq.w = c1 * c2;
    return nq;
}


//---------------------------------------------------Camera
struct Camera
{
    float3 target; //target position
    float dist; //target distance
    float4 rot; //rotation
    float near; //near clip plane
    float far; //far clip plane
    float fov; //field of view
    float aspect; //screen w/h
};

//coord transform: camera to world
__DEVICE__ float3 ScreenToCamera(float2 uv, float ld, Camera cam)
{
    float3 pos;
    pos.z = _mix(cam.near, cam.far, ld);
    swi2S(pos,x,y, (uv - 0.5f) * 2.0f);
    pos.x *= cam.aspect;
    float t = _tanf(radians(cam.fov * 0.5f));
    swi2S(pos,x,y, swi2(pos,x,y) * pos.z * t);
    return pos;
}
__DEVICE__ void ScreenRay(float2 uv, Camera cam, out float3 *ro, out float3 *rd)
{
    float3 far = ScreenToCamera(uv, 1.0f, cam);
    float3 near = cam.near / cam.far * far;
    float3 cpos = cam.target - cam.dist * RotateFWD(cam.rot);
    *ro = QuatMul(cam.rot, near) + cpos;
    *rd = QuatMul(cam.rot, far);
}


//---------------------------------------------------Gravitation

//get volume
__DEVICE__ float GetStarVolume(float mass)
{
    return mass / STAR_DENSITY;
}
//get radius
__DEVICE__ float GetStarRadius(float mass)
{
    return _powf(GetStarVolume(mass), 1.0f / 3.0f);
}

//calculate gravitational acceleration
__DEVICE__ float3 CalculateAcceleration(float3 dp, float mass)
{
  float qqqqqqqqqqqqqqqqq;
    float l = length(dp);
    if (l == 0.0f) return to_float3_s(0);
    float g = CONST_G * mass;
    float l3 = l*l*l;
    float r3 = GetStarVolume(mass);
    if (l3 > r3) g = CONST_G * mass / l3;
    else g = CONST_G * STAR_DENSITY;
  return g * dp;
}

//calculate velocity
__DEVICE__ float3 CalculateVelocity(float3 v, float3 g, float dt)
{
  float3 dv = g * dt;
  float3 nv = v + dv;
  return nv;
}

//calculate position
__DEVICE__ float3 CalculatePosition(float3 p, float3 v, float dt)
{
  float3 dp = v * dt;
  float3 np = p + dp;
  return np;
}

//---------------------------------------------------Gravitation Display

//integration function of gravitational acceleration on ray
__DEVICE__ float IntegrateOuter(float x1, float x2, float d)
{
    return x2 / _sqrtf(d*d + x2*x2) - x1 / _sqrtf(d*d + x1*x1);
}
__DEVICE__ float IntegrateOuter(float x, float d)
{
    return x / _sqrtf(d*d + x*x) + 1.0f;
}

//incorrect...
//calculate sum acceleration on ray
__DEVICE__ float3 IntegrateAcceleration(float3 ro, float3 rd, float mass)
{
    float proj = dot(ro, rd);
    float3 dp = ro - proj * rd;
    
    //l = length(dp)  distance between star and ray
    //inner = G*M/V*l * x  star inner acceleration integration
    //outer = G*M/l * x/_sqrtf(l*l+x*x)  star outer acceleration integration
    //limit(x/_sqrtf(l*l+x*x)) = +-1.0f function limit

    float l = length(dp);
    if (l == 0.0f) return to_float3_s(0);
    float g = CONST_G * mass / l * IntegrateOuter(proj, l);
    float r = GetStarRadius(mass);
#ifdef INTERGRATE_STAR_CENTER
    if (l < r)
    {
        float in0 = -_sqrtf(r * r - l * l);
        if (proj > in0)
        {
            float in1 = _fminf(proj, -in0);
            g -= CONST_G * mass / d * IntegrateOuter(in0, in1, l);
            //inner
        #ifdef RIGID_STAR
            g += CONST_G * mass / (r * r * r) * l * (in1 - in0);
        #else
            g += CONST_G * mass / (r * d) * (in1 - in0);
        #endif
        }
    }
#endif
    return g / l * dp;
}

//---------------------------------------------------Render
__DEVICE__ float GetRenderIntensity(float intensity)
{
    intensity = smoothstep(0.0f, RENDER_INTENSITY_MAX, intensity);
    intensity = _powf(intensity, RENDER_INTENSITY_POW);
    return intensity;
}
__DEVICE__ float4 GetRenderColorRed(float value)
{
    float n = 3.0f;
    //0Black 1Red 2Golden 3White
    
    float4 col;
    //R:(0<2)
    col.x = smoothstep(0.0f/n, 2.0f/n, value);
    //G:(1<3)
    col.y = smoothstep(1.0f/n, 3.0f/n, value);
    //B:(2<3)
    col.z = smoothstep(2.0f/n, 3.0f/n, value);
    col.w = 1.0f;
    return col;
}
__DEVICE__ float4 GetRenderColorBlue(float value)
{
    float n = 3.0f;
    //0Black 1Blue 2BlueClan 3White
float zzzzzzzzzzzzzzzzzzzzzz;    
    float4 col;
    //R:(2<3)
    col.x = smoothstep(2.0f/n, 3.0f/n, value);
    //G:(1<3)
    col.y = smoothstep(1.0f/n, 3.0f/n, value);
    //B:(0<2)
    col.z = smoothstep(0.0f/n, 2.0f/n, value);
    col.w = 1.0f;
    return col;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0



__DEVICE__ float4 X2X(int i,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return texture(iChannel0, I2UV(i, iResolution));
}
__DEVICE__ float4 V2P(int i,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return texture(iChannel0, I2UV(i + STAR_COUNT, iResolution));
}
__DEVICE__ float4 P2V(int i,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return texture(iChannel0, I2UV(i - STAR_COUNT, iResolution));
}

//calculate sum acceleation
__DEVICE__ float3 CalculateSumAcceleration(float3 pos, float mass, float dt, out float *power,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    float3 sa = to_float3_s(0);
    *power = 0.0f;
    for (int i = STAR_COUNT; i < STAR_COUNT * 2; i++)
    {
        float4 pm = X2X(i,iResolution,iChannel0);
        float3 dp = swi3(pm,x,y,z) - pos;
        float dpl2 = dot(dp, dp);
        if (dpl2 > 0.0f)
        {
            sa += CalculateAcceleration(dp, pm.w);
            *power += pm.w / dpl2;
        }
    }
    return sa;
}

//caculate velocity and position

__KERNEL__ void Gravitationfield3DJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    int index = XY2I(fragCoord, iResolution.x);
float AAAAAAAAAAAAAAAAAA;    
    if (index >= STAR_COUNT * 2)
    {
        SetFragmentShaderComputedColor(fragColor);      
        return;
        //discard;
    }
    else if (iFrame == 0 || Reset)
    {
        float3 sphere = HashS3(Hash23(uv)) * GALAXY_RADIUS;
        if (index < STAR_COUNT)
        {
            //init velocity
            fragColor = to_float4_aw(sphere * 0.1f, 0.0f);
        }
        else
        {
            //init position
            fragColor = to_float4_aw(sphere, DEFAULT_MASS);
        }
    }
    else
    {
        float dt = DELTA_TIME * TIME_SCALE;
        float4 vi, pm;
        if (index < STAR_COUNT)
        {
            vi = X2X(index,iResolution,iChannel0);
            pm = V2P(index,iResolution,iChannel0);
        }
        else
        {
            vi = P2V(index,iResolution,iChannel0);
            pm = X2X(index,iResolution,iChannel0);
        }
        float3 g = CalculateSumAcceleration(swi3(pm,x,y,z), pm.w, dt, &(vi.w),iResolution,iChannel0);
        swi3S(vi,x,y,z, CalculateVelocity(swi3(vi,x,y,z), g, dt));
        if (index < STAR_COUNT)
        {
            //save velocity
            fragColor = vi;
        }
        else
        {
            swi3S(pm,x,y,z, CalculatePosition(swi3(pm,x,y,z), swi3(vi,x,y,z), dt));
            //save position
            fragColor = pm;
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Preset: Keyboard' to iChannel3
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1



//-------------------------------------------------------------Keyboard
#ifdef xxx
//from "Key Quest" by TekF
__DEVICE__ bool readKey(int keyCode)
{
  bool toggle = false;
  float2 uv = to_float2((float(keyCode)+0.5f)/256.0f, toggle?.75:.25);
  float keyVal = textureLod(iChannel3,uv,0.0f).x;
  return keyVal>0.5f;
}

#define keyQ 81
#define keyW 87
#define keyE 69
#define keyA 65
#define keyS 83
#define keyD 68

#endif

//-------------------------------------------------------------Camera

#define trackSpeed 0.05f
#define scaleSpeed 1.0f
#define rotateSpeed 1.0f

__DEVICE__ float GetStarPower(int i,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return texture(iChannel0, I2UV(i, iResolution)).w;
}
__DEVICE__ float3 GetStarPosition(int i,float2 iResolution, __TEXTURE2D__ iChannel0)
{
    return swi3(texture(iChannel0, I2UV(i + STAR_COUNT, iResolution)),x,y,z);
}
__DEVICE__ float4 GetLastMouse(float2 iResolution, __TEXTURE2D__ iChannel1)
{
    return texture(iChannel1, I2UV(0, iResolution));
}

//caculate camera target position
//camera will automaticlly track the most powerful place.

__DEVICE__ float3 CalculateCameraTarget(float2 iResolution, __TEXTURE2D__ iChannel0)
{
    int maxi = 0;
    float maxPower = 0.0f;
    for (int i = 0; i < STAR_COUNT; i++)
    {
        float power = GetStarPower(i,iResolution,iChannel0);
        if (maxPower < power)
        {
            maxPower = power;
            maxi = i;
        }
    }
    return GetStarPosition(maxi,iResolution,iChannel0);
}


#define keyQ 5
#define keyW 0
#define keyE 4
#define keyA 2
#define keyS 1
#define keyD 3
#define RESET 6

__DEVICE__ void UpdatePosition(float2 uv, out float4 *result,float2 iResolution, int iFrame, float iTimeDelta, bool KEY[7], __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1)
{
  
    if (iFrame == 0 || KEY[RESET])
    {
        *result = to_float4_aw(to_float3_s(0), GALAXY_RADIUS * 2.0f);
    }
    else
    {
        float4 cs = _tex2DVecN(iChannel1,uv.x,uv.y,15);
        //--track
        float3 tgt = CalculateCameraTarget(iResolution,iChannel0);
        float dt = GALAXY_RADIUS / distance_f3(tgt, swi3(cs,x,y,z)) * iTimeDelta * trackSpeed;
        swi3S(cs,x,y,z, _mix(swi3(cs,x,y,z), tgt, dt));
        //--scale
        float ds = iTimeDelta * scaleSpeed;
        if      (KEY[keyE]) cs.w *= 1.0f - ds;
        else if (KEY[keyQ]) cs.w *= 1.0f + ds;
        
        *result = cs;
    }
}



__DEVICE__ void UpdateRotation(float2 uv, out float4 *result,float2 iResolution, int iFrame, float4 iMouse, float iTimeDelta, bool KEY[7], __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1)
{
    if (iFrame == 0 || KEY[RESET])
    {
        *result = QUAT_IDENTITY;
    }
    else
    {
        float4 cs = _tex2DVecN(iChannel1,uv.x,uv.y,15);
        //--rotate
        float dr = iTimeDelta * rotateSpeed;
        float2 eu = to_float2_s(0);
        //mouse
        float4 lm = GetLastMouse(iResolution,iChannel1);
        float2 dn = swi2(iMouse,x,y) - swi2(iMouse,z,w);
        float2 dp = swi2(iMouse,x,y) - swi2(lm,x,y);
        float2 dm = iMouse.z > lm.z ? dn : dp;
        dm = to_float2(dm.x, -dm.y) / iResolution.y;
        swi2S(eu,x,y, swi2(eu,x,y) + swi2(dm,y,x) * (dr * 100.0f));
        //key
        if (KEY[keyW]) eu.x -= dr;
        if (KEY[keyS]) eu.x += dr;
        if (KEY[keyA]) eu.y -= dr;
        if (KEY[keyD]) eu.y += dr;
        
        *result = QuatMul(cs, QuatEA(eu));
    }
}

//camera target position, distance, rotation buffer

__KERNEL__ void Gravitationfield3DJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, float iTimeDelta, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
  
    CONNECT_CHECKBOX0(Reset, 0);
  
    CONNECT_CHECKBOX1(Up,      0); //KeyW, 0);
    CONNECT_CHECKBOX2(Down,    0); //KeyS, 0);
    CONNECT_CHECKBOX3(Left,    0); //KeyA, 0);
    CONNECT_CHECKBOX4(Right,   0); //KeyD, 0);
    CONNECT_CHECKBOX5(ZoomIn,  0); //KeyE, 0);
    CONNECT_CHECKBOX6(ZoomOut, 0); //KeyQ, 0);
    
    
    
    //bool KEY[6] = {KeyW,KeyS,KeyA,KeyD,KeyE,KeyQ};
    bool KEY[7] = {Up,Down,Left,Right,ZoomIn,ZoomOut,Reset};

    fragCoord+=0.5f;

    int index = XY2I(fragCoord, iResolution.x);
    
    if (index >= 3)
    {
        SetFragmentShaderComputedColor(fragColor);      
        return;
        //discard;
    }
    else if (index == 0)
    {
        fragColor = iMouse;
    }
    else
    {
        float2 uv = fragCoord / iResolution;
        if (index == 1)
        {
            UpdatePosition(uv, &fragColor,iResolution,iFrame,iTimeDelta, KEY,iChannel0, iChannel1);
        }
        else if (index == 2)
        {
            UpdateRotation(uv, &fragColor,iResolution,iFrame,iMouse,iTimeDelta, KEY,iChannel0, iChannel1);
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


/**
 * Written by Cairne.D
 */


//set other constants in label "Common"

//range = [0,2]
#define COLOR_MODE 0

//mix color
__DEVICE__ void MixColor(float lit, out float4 *color, float iTime, int modus, float4 ColorIn[2])
{
  float rlit = GetRenderIntensity(lit);
  if(modus == 1)
    *color = GetRenderColorBlue(rlit);
  else if (modus == 2)
  {
    float t = clamp(_sinf(iTime/15.0f*PI2-PIH)*2.0f + 0.5f, 0.0f, 1.0f);
    float4 blue = GetRenderColorBlue(rlit);
    float4 red = GetRenderColorRed(rlit);
    *color = _mix(blue, red, t);
  }
  else if (modus ==  3)
  {
    //copy from "gravity field - 2" by FabriceNeyret2
    *color = to_float4(_sinf(lit),_sinf(lit/2.0f),_sinf(lit/4.0f),1.0f);
  }
  else if (modus ==  4)
    *color = _mix(ColorIn[0], ColorIn[1], rlit);
  else
    *color = _mix(ColorIn[0], ColorIn[1], lit);
}

__DEVICE__ Camera GetCurrentCamera(float2 iResolution, __TEXTURE2D__ iChannel1)
{
    Camera cam;
    cam.near = 0.001f;
    cam.far = 100000.0f;
    cam.fov = 60.0f;
    cam.aspect = iResolution.x / iResolution.y;
    
    float4 td = texture(iChannel1, I2UV(1, iResolution));
    float4 r = texture(iChannel1, I2UV(2, iResolution));
    cam.target = swi3(td,x,y,z);
    cam.dist = td.w;
    cam.rot = r;
    
    return cam;
}

//calculate field strength on pixel ray
__DEVICE__ float CalculateIntensity(float3 ro, float3 rd, float2 iResolution, __TEXTURE2D__ iChannel0)
{
    rd = normalize(rd);
    float3 g = to_float3_s(0);
    for (int i = STAR_COUNT; i < STAR_COUNT * 2; i++)
    {
        float4 pm = texture(iChannel0, I2UV(i, iResolution));
        g += IntegrateAcceleration(swi3(pm,x,y,z)-ro, rd, pm.w);
    }
    return length(g);
}

__KERNEL__ void Gravitationfield3DJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color1, 1.0f, 0.0f, 0.0f, 1.0f);
    CONNECT_COLOR1(Color2, 0.0f, 1.0f, 0.0f, 1.0f);
    CONNECT_SLIDER0(Level0, -1.0f, 1.0f, 0.0f);

    CONNECT_BUTTON0(Modus, 0, OnlyBlue, BlueRed, Sinus, Col12, Lit);
  
    fragCoord+=0.5f;

    float4 Color[2] = {Color1,Color2};

    float2 uv = fragCoord / iResolution;
float IIIIIIIIIIIIIIII;    
    //apply camera operation
    Camera cam = GetCurrentCamera(iResolution,iChannel1);
    float3 ro, rd;
    ScreenRay(uv, cam, &ro, &rd);

    //calculate field strength per pixel
    float lit = CalculateIntensity(ro, rd,iResolution,iChannel0);
    
    MixColor(lit, &fragColor, iTime, Modus, Color);

  SetFragmentShaderComputedColor(fragColor);
}