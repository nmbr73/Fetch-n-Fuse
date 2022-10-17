

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/**
 * Written by Cairne.D
 */


//set other constants in label "Common"

//range = [0,2]
#define COLOR_MODE 0

//mix color
void MixColor(float lit, out vec4 color)
{
    float rlit = GetRenderIntensity(lit);
#if COLOR_MODE == 0
    color = GetRenderColorBlue(rlit);
#elif COLOR_MODE == 1
    float t = clamp(sin(iTime/15.0*PI2-PIH)*2.0 + 0.5, 0.0, 1.0);
    vec4 blue = GetRenderColorBlue(rlit);
    vec4 red = GetRenderColorRed(rlit);
    color = mix(blue, red, t);
#else
    //copy from "gravity field - 2" by FabriceNeyret2
	color = vec4(sin(lit),sin(lit/2.),sin(lit/4.),1.);
#endif
}

Camera GetCurrentCamera()
{
    Camera cam;
    cam.near = 0.001;
    cam.far = 100000.0;
    cam.fov = 60.0;
    cam.aspect = iResolution.x / iResolution.y;
    
    vec4 td = texture(iChannel1, I2UV(1, iResolution.xy));
    vec4 r = texture(iChannel1, I2UV(2, iResolution.xy));
    cam.target = td.xyz;
    cam.dist = td.w;
    cam.rot = r;
    
    return cam;
}

//calculate field strength on pixel ray
float CalculateIntensity(vec3 ro, vec3 rd)
{
    rd = normalize(rd);
    vec3 g = vec3(0);
    for (int i = STAR_COUNT; i < STAR_COUNT * 2; i++)
    {
        vec4 pm = texture(iChannel0, I2UV(i, iResolution.xy));
        g += IntegrateAcceleration(pm.xyz-ro, rd, pm.w);
    }
    return length(g);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    
    //apply camera operation
    Camera cam = GetCurrentCamera();
    vec3 ro, rd;
    ScreenRay(uv, cam, ro, rd);

    //calculate field strength per pixel
    float lit = CalculateIntensity(ro, rd);
    
    MixColor(lit, fragColor);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//---------------------------------------------------Constant
#define PI  3.14159265359
#define PI2 6.28318530718
#define PIH 1.57079632679

//star number
#define STAR_COUNT 48

//gravitational constant
#define CONST_G 1.0

//star mass
#define DEFAULT_MASS 1.0

//star density
#define STAR_DENSITY 1000.0

//initial stars area
#define GALAXY_RADIUS 10.0

//physics step time
#define DELTA_TIME 0.02

//speed up or down
#define TIME_SCALE 1.0

//trunc light for render [0,x]
#define RENDER_INTENSITY_MAX 50.0

//pow
#define RENDER_INTENSITY_POW 0.5

//IF YOU HAVE SERIOUS Trypophobia, SET IT TO false!
//#define INTERGRATE_STAR_CENTER

//---------------------------------------------------Hash
float Hash21(vec2 p)
{
	return fract(sin(dot(p,vec2(12.99,78.23)))*43758.5453);
}
vec2 Hash22(vec2 p)
{
	return fract(sin(vec2(
        dot(p,vec2(127.1,311.7)),
        dot(p,vec2(269.5,183.3))
    ))*43758.5453);
}
vec3 Hash23(vec2 p)
{
    return fract(sin(vec3(
        dot(p,vec2(127.1,311.7)),
        dot(p,vec2(269.5,183.3)),
        dot(p,vec2(12.99,78.23))
    ))*43758.5453);
}
vec3 HashS3(vec2 h2)
{
    float theta = h2.x*PI2;
    float phi = acos(h2.y*2.0-1.0);
    return vec3(
        sin(theta)*sin(phi),
        cos(theta)*sin(phi),
        cos(phi));
}
vec3 HashS3(vec3 h3)
{
    return HashS3(h3.xy)*pow(h3.z,1.0/3.0);
}

//----------------------------------------------fragCoord=>index=>uv
//thanks to dr2! his "index convert function" helped me
//"Faberge Balls" by dr2
int XY2I(vec2 xy, float sizeX)
{
    xy = floor(xy);
    int i = int(xy.x + xy.y * sizeX);
    return i;
}
vec2 I2UV(int i, vec2 size)
{
    float fi = float(i);
    vec2 uv;
    uv.x = mod(fi, size.x);
    uv.y = floor(fi / size.x);
    uv = (uv + 0.5) / size;
    return uv;
}

//----------------------------------------------Quaternion

//identity 元
#define QUAT_IDENTITY vec4(0,0,0,1)

//conjugate (inverse rotation)
vec4 QuatInv(vec4 q){return vec4(-q.xyz,q.w);}

//quaternion multiply
vec4 QuatMul(vec4 q, vec4 r)
{
    vec4 nq;
    nq.x = q.w * r.x + q.x * r.w + q.y * r.z - q.z * r.y;
    nq.y = q.w * r.y - q.x * r.z + q.y * r.w + q.z * r.x;
    nq.z = q.w * r.z + q.z * r.w - q.y * r.x + q.x * r.y;
    nq.w = q.w * r.w - q.x * r.x - q.y * r.y - q.z * r.z;
    return nq;
}
vec3 QuatMul(vec4 lhs, vec3 rhs)
{
    float x1 = lhs.x, y1 = lhs.y, z1 = lhs.z, w1 = lhs.w;
    float x2 = rhs.x, y2 = rhs.y, z2 = rhs.z;
    float nx = w1 * x2 + y1 * z2 - z1 * y2;
    float ny = w1 * y2 - x1 * z2 + z1 * x2;
    float nz = w1 * z2 + x1 * y2 - y1 * x2;
    float nw = x1 * x2 + y1 * y2 + z1 * z2;
    vec3 nv;
    nv.x = nw * x1 + nx * w1 - ny * z1 + nz * y1;
    nv.y = nw * y1 + nx * z1 + ny * w1 - nz * x1;
    nv.z = nw * z1 - nx * y1 + ny * x1 + nz * w1;
    return nv;
}
vec3 RotateFWD(vec4 q)
{
    vec3 nv;
    nv.x = 2.0 * (q.x * q.z + q.y * q.w);
    nv.y = 2.0 * (q.y * q.z - q.x * q.w);
    nv.z = 1.0 - 2.0 * (q.x * q.x + q.y * q.y);
    return nv;
}

//from euler angles
vec4 QuatEA(vec2 eulers)
{
    vec2 eh = eulers * 0.5;
    float c1 = cos(eh.x), c2 = cos(eh.y);
    float s1 = sin(eh.x), s2 = sin(eh.y);
    vec4 nq;
    nq.x = s1 * c2;
    nq.y = c1 * s2;
    nq.z = -s1 * s2;
    nq.w = c1 * c2;
    return nq;
}


//---------------------------------------------------Camera
struct Camera
{
    vec3 target; //target position
    float dist; //target distance
    vec4 rot; //rotation
    float near; //near clip plane
    float far; //far clip plane
    float fov; //field of view
    float aspect; //screen w/h
};

//coord transform: camera to world
vec3 ScreenToCamera(vec2 uv, float ld, Camera cam)
{
    vec3 pos;
    pos.z = mix(cam.near, cam.far, ld);
    pos.xy = (uv - 0.5) * 2.0;
    pos.x *= cam.aspect;
    float t = tan(radians(cam.fov * 0.5));
    pos.xy *= pos.z * t;
    return pos;
}
void ScreenRay(vec2 uv, Camera cam, out vec3 ro, out vec3 rd)
{
    vec3 far = ScreenToCamera(uv, 1.0, cam);
    vec3 near = cam.near / cam.far * far;
    vec3 cpos = cam.target - cam.dist * RotateFWD(cam.rot);
    ro = QuatMul(cam.rot, near) + cpos;
    rd = QuatMul(cam.rot, far);
}


//---------------------------------------------------Gravitation

//get volume
float GetStarVolume(float mass)
{
    return mass / STAR_DENSITY;
}
//get radius
float GetStarRadius(float mass)
{
    return pow(GetStarVolume(mass), 1.0 / 3.0);
}

//calculate gravitational acceleration
vec3 CalculateAcceleration(vec3 dp, float mass)
{
    float l = length(dp);
    if (l == 0.0) return vec3(0);
	float g = CONST_G * mass;
    float l3 = l*l*l;
    float r3 = GetStarVolume(mass);
    if (l3 > r3) g = CONST_G * mass / l3;
    else g = CONST_G * STAR_DENSITY;
	return g * dp;
}

//calculate velocity
vec3 CalculateVelocity(vec3 v, vec3 g, float dt)
{
	vec3 dv = g * dt;
	vec3 nv = v + dv;
	return nv;
}

//calculate position
vec3 CalculatePosition(vec3 p, vec3 v, float dt)
{
	vec3 dp = v * dt;
	vec3 np = p + dp;
	return np;
}

//---------------------------------------------------Gravitation Display

//integration function of gravitational acceleration on ray
float IntegrateOuter(float x1, float x2, float d)
{
    return x2 / sqrt(d*d + x2*x2) - x1 / sqrt(d*d + x1*x1);
}
float IntegrateOuter(float x, float d)
{
    return x / sqrt(d*d + x*x) + 1.0;
}

//incorrect...
//calculate sum acceleration on ray
vec3 IntegrateAcceleration(vec3 ro, vec3 rd, float mass)
{
    float proj = dot(ro, rd);
    vec3 dp = ro - proj * rd;
    
    //l = length(dp)  distance between star and ray
    //inner = G*M/V*l * x  star inner acceleration integration
    //outer = G*M/l * x/sqrt(l*l+x*x)  star outer acceleration integration
    //limit(x/sqrt(l*l+x*x)) = +-1.0 function limit

    float l = length(dp);
    if (l == 0.0) return vec3(0);
    float g = CONST_G * mass / l * IntegrateOuter(proj, l);
    float r = GetStarRadius(mass);
#ifdef INTERGRATE_STAR_CENTER
    if (l < r)
    {
        float in0 = -sqrt(r * r - l * l);
        if (proj > in0)
        {
            float in1 = min(proj, -in0);
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
float GetRenderIntensity(float intensity)
{
    intensity = smoothstep(0., RENDER_INTENSITY_MAX, intensity);
    intensity = pow(intensity, RENDER_INTENSITY_POW);
    return intensity;
}
vec4 GetRenderColorRed(float value)
{
    float n = 3.0;
    //0Black 1Red 2Golden 3White
    
    vec4 col;
    //R:(0<2)
    col.r = smoothstep(0.0/n, 2.0/n, value);
    //G:(1<3)
    col.g = smoothstep(1.0/n, 3.0/n, value);
    //B:(2<3)
    col.b = smoothstep(2.0/n, 3.0/n, value);
    col.a = 1.0;
    return col;
}
vec4 GetRenderColorBlue(float value)
{
    float n = 3.0;
    //0Black 1Blue 2BlueClan 3White
    
    vec4 col;
    //R:(2<3)
    col.r = smoothstep(2.0/n, 3.0/n, value);
    //G:(1<3)
    col.g = smoothstep(1.0/n, 3.0/n, value);
    //B:(0<2)
    col.b = smoothstep(0.0/n, 2.0/n, value);
    col.a = 1.0;
    return col;
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

vec4 X2X(int i)
{
    return texture(iChannel0, I2UV(i, iResolution.xy));
}
vec4 V2P(int i)
{
    return texture(iChannel0, I2UV(i + STAR_COUNT, iResolution.xy));
}
vec4 P2V(int i)
{
    return texture(iChannel0, I2UV(i - STAR_COUNT, iResolution.xy));
}

//calculate sum acceleation
vec3 CalculateSumAcceleration(vec3 pos, float mass, float dt, out float power)
{
    vec3 sa = vec3(0);
    power = 0.0;
    for (int i = STAR_COUNT; i < STAR_COUNT * 2; i++)
    {
        vec4 pm = X2X(i);
        vec3 dp = pm.xyz - pos;
        float dpl2 = dot(dp, dp);
        if (dpl2 > 0.0)
        {
            sa += CalculateAcceleration(dp, pm.w);
            power += pm.w / dpl2;
        }
    }
    return sa;
}

//caculate velocity and position

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    int index = XY2I(fragCoord, iResolution.x);
    
    if (index >= STAR_COUNT * 2)
    {
        discard;
    }
    else if (iFrame == 0)
    {
        vec3 sphere = HashS3(Hash23(uv)) * GALAXY_RADIUS;
        if (index < STAR_COUNT)
        {
            //init velocity
            fragColor = vec4(sphere * 0.1, 0.0);
        }
        else
        {
            //init position
            fragColor = vec4(sphere, DEFAULT_MASS);
        }
    }
    else
    {
        float dt = DELTA_TIME * TIME_SCALE;
        vec4 vi, pm;
        if (index < STAR_COUNT)
        {
            vi = X2X(index);
            pm = V2P(index);
        }
        else
        {
            vi = P2V(index);
            pm = X2X(index);
        }
        vec3 g = CalculateSumAcceleration(pm.xyz, pm.w, dt, vi.w);
        vi.xyz = CalculateVelocity(vi.xyz, g, dt);
        if (index < STAR_COUNT)
        {
            //save velocity
            fragColor = vi;
        }
        else
        {
            pm.xyz = CalculatePosition(pm.xyz, vi.xyz, dt);
            //save position
            fragColor = pm;
        }
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

//-------------------------------------------------------------Keyboard
//from "Key Quest" by TekF
bool readKey(int keyCode)
{
	bool toggle = false;
    vec2 uv = vec2((float(keyCode)+.5)/256., toggle?.75:.25);
	float keyVal = textureLod(iChannel3,uv,0.).x;
    return keyVal>.5;
}
#define keyQ 81
#define keyW 87
#define keyE 69
#define keyA 65
#define keyS 83
#define keyD 68

//-------------------------------------------------------------Camera

#define trackSpeed 0.05
#define scaleSpeed 1.0
#define rotateSpeed 1.0

float GetStarPower(int i)
{
    return texture(iChannel0, I2UV(i, iResolution.xy)).w;
}
vec3 GetStarPosition(int i)
{
    return texture(iChannel0, I2UV(i + STAR_COUNT, iResolution.xy)).xyz;
}
vec4 GetLastMouse()
{
    return texture(iChannel1, I2UV(0, iResolution.xy));
}

//caculate camera target position
//camera will automaticlly track the most powerful place.

vec3 CalculateCameraTarget()
{
    int maxi = 0;
    float maxPower = 0.0;
    for (int i = 0; i < STAR_COUNT; i++)
    {
        float power = GetStarPower(i);
        if (maxPower < power)
        {
            maxPower = power;
            maxi = i;
        }
    }
    return GetStarPosition(maxi);
}

void UpdatePosition(vec2 uv, out vec4 result)
{
    if (iFrame == 0)
    {
        result = vec4(vec3(0), GALAXY_RADIUS * 2.0);
    }
    else
    {
        vec4 cs = texture(iChannel1, uv);
        //--track
        vec3 tgt = CalculateCameraTarget();
        float dt = GALAXY_RADIUS / distance(tgt, cs.xyz) * iTimeDelta * trackSpeed;
        cs.xyz = mix(cs.xyz, tgt, dt);
        //--scale
        float ds = iTimeDelta * scaleSpeed;
        if (readKey(keyE))      cs.w *= 1.0 - ds;
        else if (readKey(keyQ)) cs.w *= 1.0 + ds;
        
        result = cs;
    }
}

void UpdateRotation(vec2 uv, out vec4 result)
{
    if (iFrame == 0)
    {
        result = QUAT_IDENTITY;
    }
    else
    {
        vec4 cs = texture(iChannel1, uv);
        //--rotate
        float dr = iTimeDelta * rotateSpeed;
        vec2 eu = vec2(0);
        //mouse
        vec4 lm = GetLastMouse();
        vec2 dn = iMouse.xy - iMouse.zw;
        vec2 dp = iMouse.xy - lm.xy;
        vec2 dm = iMouse.z > lm.z ? dn : dp;
        dm = vec2(dm.x, -dm.y) / iResolution.y;
        eu.xy += dm.yx * (dr * 100.0);
        //key
        if (readKey(keyW)) eu.x -= dr;
        if (readKey(keyS)) eu.x += dr;
        if (readKey(keyA)) eu.y -= dr;
        if (readKey(keyD)) eu.y += dr;
        
        result = QuatMul(cs, QuatEA(eu));
    }
}

//camera target position, distance, rotation buffer

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    int index = XY2I(fragCoord, iResolution.x);
    
    if (index >= 3)
    {
        discard;
    }
    else if (index == 0)
    {
        fragColor = iMouse;
    }
    else
    {
        vec2 uv = fragCoord / iResolution.xy;
        if (index == 1)
        {
            UpdatePosition(uv, fragColor);
        }
        else if (index == 2)
        {
            UpdateRotation(uv, fragColor);
        }
    }
}