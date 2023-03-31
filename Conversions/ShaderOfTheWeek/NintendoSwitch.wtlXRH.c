
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Nintendo Switch by jackdavenport
// All code is free to use with credit! :)
// Created 2019

//------------------------------------------------------------------------------------------------//
// Signed Distance Fields
// Source: https://iquilezles.org/articles/distfunctions
__DEVICE__ float dstPlane(float3 p, float4 plane) {
    return dot(p,swi3(plane,x,y,z)) - plane.w;
}
__DEVICE__ float dstBox(float3 p, float3 b) {
    float3 q = abs_f3(p) - b;
    return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(_fmaxf(q.x,_fmaxf(q.y,q.z)),0.0f);
}
__DEVICE__ float dstRoundBox(float3 p, float3 b, float r) {
    return dstBox(p, b) - r;
}
__DEVICE__ float dstCappedCylinder( float3 p, float h, float r )
{
  float2 d = abs_f2(to_float2(length(swi2(p,x,z)),p.y)) - to_float2(h,r);
  return _fminf(_fmaxf(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}
__DEVICE__ float dstBox2D(float2 p, float2 b) {
    float2 q = abs_f2(p) - b;
    return length(_fmaxf(q,to_float2_s(0.0f))) + _fminf(_fmaxf(q.x,q.y),0.0f);
}

//------------------------------------------------------------------------------------------------//
// Helpful directive functions
#define rgb(r,g,b) (to_float3(r,g,b)*0.00392156862f) /* the number is 1/255 */
#define _saturatef(x) clamp(x,0.0f,1.0f)

//------------------------------------------------------------------------------------------------//
// Distance Functions/Booleans
// Some of these are from iq's website
// Source: https://iquilezles.org/articles/distfunctions
__DEVICE__ float2 dstUnion(float2 a, float bt, float bid) {
    if(a.x < bt) return a;
    return to_float2(bt,bid);
}
__DEVICE__ float dstSubtract(float a, float b) {
    return _fmaxf(a,-b);
}
__DEVICE__ float dstIntersect(float a, float b) {
    return _fmaxf(a,b);
}
__DEVICE__ float3 dstElongate(float3 p, float3 h) {
    return p - clamp(p, -h, h);
}

//------------------------------------------------------------------------------------------------//
// Materials/Lighting
struct Material {
    float3 albedo;
    float3 specular;
    float3 emission;
    float shininess;
    float reflectivity;
};

//------------------------------------------------------------------------------------------------//
// Math Functions
__DEVICE__ float2 rot2D(float2 p, float a) {
    float s = _sinf(a), c = _cosf(a);
    return mul_mat2_f2(to_mat2(c,s,-s,c) , p);
}
__DEVICE__ float expFog(float dist, float density) {
    return 1.0f - _expf(-dist*density);
}

//------------------------------------------------------------------------------------------------//

__DEVICE__ struct Material getMaterial(struct Material mat, in float3 p, in float3 n, in float2 t, float3 consoleRot[2], float4 texch0, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    
    mat.emission = to_float3_s(0.0f);  // Inhalt des Displays
    if(t.y == 0.0f) { // ground material
        float2 tuv = (to_float2(p.x*texch0.w,p.z) + swi2(texch0,x,y)) * texch0.z;
    
        //float4 tex = texture(iChannel0, swi2(p,x,z) / 3.5f);    
        float4 tex = texture(iChannel0, tuv);    
        mat.albedo = swi3(tex,x,y,z);
        mat.specular = to_float3_s(0.2f + 0.7f * tex.x);
        mat.shininess = 10.0f + 50.0f * tex.y;
        mat.reflectivity = 0.8f;
    } else if(t.y == 1.0f) { // switch body
    
        p += consoleRot[1];
        //swi2S(p,y,z, rot2D(swi2(p,y,z), consoleRot));
        swi2S(p,y,z, rot2D(swi2(p,y,z), consoleRot[0].x));   // Aufstellen  
        swi2S(p,x,y, rot2D(swi2(p,x,y), consoleRot[0].y));   // Kippen rechts links
        swi2S(p,x,z, rot2D(swi2(p,x,z), consoleRot[0].z));   // Drehen rechts links
                
        float screen = step(0.0f,dstBox2D(swi2(p,x,z), to_float2(0.73f,0.43f))-0.03f);
        if(screen < 0.5f && p.y > 0.05f) {
          float innerScreen = step(0.0f,dstBox2D(swi2(p,x,z), to_float2(0.67f,0.38f)));
          mat.albedo   = _mix(to_float3_s(0.0f), to_float3_s(0.1f), innerScreen);
          mat.emission = _mix(swi3(texture(iChannel1,(swi2(p,x,z)*to_float2(0.7f,1.3f))+to_float2(0.5f,0.5f)),x,y,z), to_float3_s(0.0f), innerScreen);
        } else {
            mat.albedo = rgb(38,38,38);
        }
        mat.specular = to_float3_s(_mix(1.0f,0.4f,screen));
        mat.shininess = _mix(60.0f,30.0f,screen);
        mat.reflectivity = 1.0f - 0.9f * screen;
    } else if(t.y == 2.0f) { // joycons base
    
        p += consoleRot[1];
    
        swi2S(p,y,z, rot2D(swi2(p,y,z), consoleRot[0].x));   // Aufstellen  
        swi2S(p,x,y, rot2D(swi2(p,x,y), consoleRot[0].y));   // Kippen rechts links
        swi2S(p,x,z, rot2D(swi2(p,x,z), consoleRot[0].z));   // Drehen rechts links
          
        mat.albedo = _mix(rgb(247, 57, 47), rgb(46, 182, 255), step(0.0f, p.x));
        mat.specular = to_float3_s(0.2f);
        mat.shininess = 20.0f;
        mat.reflectivity = 0.2f;
    } else if(t.y == 3.0f) { // joysticks/buttons
        mat.albedo = rgb(38,38,38);
        mat.specular = to_float3_s(0.5f);
        mat.shininess = 8.0f;
        mat.reflectivity = 0.1f;
    } else if(t.y == 4.0f) { // home button
        mat.albedo = rgb(54, 53, 52);
        mat.specular = to_float3_s(0.5f);
        mat.shininess = 8.0f;
        mat.reflectivity = 0.1f;
    } else if(t.y == 5.0f) { // ir sensor
        mat.albedo = to_float3_s(0.05f);
        mat.specular = to_float3_s(1.0f);
        mat.shininess = 80.0f;
        mat.reflectivity = 1.0f;
    } else { // default material
        mat.albedo = to_float3(1.0f,0.0f,1.0f);
        mat.specular = to_float3_s(0.0f);
        mat.shininess = 0.0f;
        mat.reflectivity = 0.0f;
    }
    
    return mat;
}

// Thanks knarkowicz!
// Source: https://knarkowicz.wordpress.com/2016/01/06/aces-filmic-tone-mapping-curve/
__DEVICE__ float3 ACESFilm(float3 x)
{
float zzzzzzzzzzzzzzzzzzzzzzzzzzzzz;  
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return _saturatef((x*(a*x+b))/(x*(c*x+d)+e));
}



//------------------------------------------------------------------------------------------------//
// Camera Functions
__DEVICE__ float3 cameraPos(in float3 ro, in float time, in float4 mouse, in float2 res) {
    if(mouse.z < 0.1f) {
      float theta = 3.14159f * time;
      float s = _sinf(theta), c = _cosf(theta);
      ro.x = s * 2.0f;
      ro.z = -c * 2.0f;
    } else {
        float yaw = 3.14159f * 2.0f * (mouse.x / res.x);
        float pitch = _fmaxf(3.14159f * 0.5f * (mouse.y / res.y), 0.4f);
        
        float sy = _sinf(yaw), cy = _cosf(yaw);
        float sp = _sinf(pitch), cp = _cosf(pitch);
        
        ro.x = sy * cp * 2.0f;
        ro.y = sp * 2.0f;
        ro.z = -cy * cp * 2.0f;
    }
    return ro;
}
__DEVICE__ float3 lookAt(in float3 focalPoint, in float3 eyePos, in float3 upDir, in float3 rd) {
    float3 f = normalize(focalPoint - eyePos);
    float3 u = normalize(cross(f, upDir));
    float3 v = normalize(cross(u, f));
    rd = mul_mat3_f3(to_mat3_f3(u, v, f) , rd);
    return rd;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Wood' to iChannel1
// Connect Buffer A 'Cubemap: St Peters Basilica_0' to iChannel0


// Twisting Pylon by jackdavenport
// All code is free to use with credit! :)
// Created 2016
// Link to original: https://www.shadertoy.com/view/XstXW7

#define MAX_ITERATIONS 256
#define MIN_DISTANCE 0.001f

#define LIGHT_COL to_float3_s(1.0f)
#define LIGHT_DIR normalize(to_float3(90.0f,80.0f,-45.0f))

struct Ray { float3 ori; float3 dir; };
struct Dst { float dst;  int id;   };
struct Hit { float3 p;   int id;   };
    
    
__DEVICE__ Dst dstPillar(float3 p, float3 pos, float3 box, float iTime) {
        
    swi2S(p,x,z, rot2D(swi2(p,x,z), (iTime + p.y) * 0.785398163f));
    
    float3  d = abs_f3(pos - p) - box;
    float dst = _fminf(_fmaxf(d.x,_fmaxf(d.y,d.z)), 0.0f) + length(_fmaxf(d, to_float3_s(0.0f)));
    
    Dst ret = {dst,0};
    return ret;
}

__DEVICE__ Dst dstFloor(float3 p, float y) {
 
//    return Dst(p.y - y, 1);
    Dst ret = {p.y - y, 1};
    return ret;
}

__DEVICE__ Dst dstMin(Dst a, Dst b) {
 
    if(b.dst < a.dst) return b;
    return a;
}

__DEVICE__ Dst dstScene(float3 p, float iTime) {
 
    Dst dst = dstPillar(p, to_float3_s(0.0f), to_float3(0.5f,2.0f,0.5f), iTime);
    dst = dstMin(dst, dstFloor(p, -2.0f));
    
    return dst;
}

__DEVICE__ Hit raymarch(Ray ray, float iTime) {
 
    float3 p = ray.ori;
    int id = -1;
    
    for(int i = 0; i < MAX_ITERATIONS; i++) {
     
        Dst scn = dstScene(p,iTime);
        p += ray.dir * scn.dst * 0.75f;
        
        if(scn.dst < MIN_DISTANCE) {
         
            id = scn.id;
            break;
        }
    }
      
//    return Hit(p,id);
    Hit ret = {p,id};
    return ret;
}
  
// Shadow code from the incredible iq
// Source: https://www.shadertoy.com/view/Xds3zN
__DEVICE__ float softShadow( in float3 ro, in float3 rd, in float mint, in float tmax, float iTime )
{
    float res = 1.0f;
    float t = mint;
    for( int i=0; i<24; i++ )
    {
        float h = dstScene( ro + rd*t, iTime ).dst;
        res = _fminf( res,32.0f*h/t );
        t += clamp( h, 0.05f, 0.50f );
        if( h<0.001f || t>tmax ) break;
    }
    return clamp( res, 0.0f, 1.0f );
}

__DEVICE__ float3 calcNormal(float3 p, float iTime) {
 
    float2 eps = to_float2(0.001f,0.0f);
    float3   n = to_float3(dstScene(p + swi3(eps,x,y,y),iTime).dst - dstScene(p - swi3(eps,x,y,y),iTime).dst,
                           dstScene(p + swi3(eps,y,x,y),iTime).dst - dstScene(p - swi3(eps,y,x,y),iTime).dst,
                           dstScene(p + swi3(eps,y,y,x),iTime).dst - dstScene(p - swi3(eps,y,y,x),iTime).dst);
    return normalize(n);
}

__DEVICE__ float3 calcLighting(float3 n, float s, Hit scn) {
 
  float d = _fmaxf(dot(LIGHT_DIR,n), 0.0f);
  d *= s;
    
  return LIGHT_COL * d;
}

__DEVICE__ float3 getPylonDiffuse(float3 n, float s, Hit scn) {
 
    return calcLighting(n, s, scn);
}

__DEVICE__ float3 getFloorDiffuse(Hit scn, float iTime, float ratio, __TEXTURE2D__ iChannel1) {
 
    float2 uv = mod_f2(swi2(scn.p,x,z) / 3.5f, 1.0f);
    uv = swi2(scn.p,x,z);
    float s = softShadow(scn.p, LIGHT_DIR, 0.0015f, 10.0f, iTime);
    
    return swi3(_tex2DVecN(iChannel1,uv.x*ratio,uv.y,15),x,y,z) * calcLighting(to_float3(0.0f, 1.0f, 0.0f), s, scn);
    
}

__DEVICE__ float3 shade(Ray ray, float iTime, float ratio, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {

    Hit scn  = raymarch(ray,iTime);
    float3 col = swi3(decube_f3(iChannel0, ray.dir),x,y,z);
    
    if(scn.id == 0) {
     
        float3 n = calcNormal(scn.p,iTime);
        float3 r = reflect(ray.dir, n);
        
        Ray rr = {scn.p + r * 0.001f, r};
        Hit rh = raymarch(rr, iTime);
        
        float sh = softShadow(scn.p, LIGHT_DIR, 0.0015f, 10.0f, iTime);
        float3  dc = getPylonDiffuse(n, sh, scn);
        float3 rc  = 
            rh.id == 0 ? getPylonDiffuse(calcNormal(rh.p,iTime),softShadow(scn.p, LIGHT_DIR, 0.0015f, 10.0f, iTime),rh) : 
            rh.id == 1 ? getFloorDiffuse(rh, iTime, ratio, iChannel1) :
            swi3(decube_f3(iChannel0, rr.dir),x,y,z);
        
        float3 s = LIGHT_COL * _powf(_fmaxf(dot(LIGHT_DIR,r),0.0f), 30.0f) * softShadow(scn.p, LIGHT_DIR, 0.0015f, 10.0f, iTime);
        float f = _mix(0.1f, 0.9f, 1.0f - _fmaxf(_powf(-dot(ray.dir,n), 0.1f), 0.0f));
        return _mix(dc, rc, f) + s;
        
    } else if(scn.id == 1) {
    
        col = getFloorDiffuse(scn,iTime,ratio,iChannel1);
    }
    
    col = clamp(col,0.0f,1.0f); // make sure colours are clamped for texturing
    return col;
}

__KERNEL__ void NintendoSwitchFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f; 
    float2 uv = (fragCoord - iResolution * 0.5f) / iResolution.y;
    
    float ratio = iResolution.y/iResolution.x;
    
    float3 ori = to_float3(0.0f,0.0f,-5.5f);
    float3 dir = to_float3_aw(uv, 1.0f);

    Ray raypar = {ori,dir};
    
    float3 col = shade(raypar,iTime,ratio,iChannel0,iChannel1);
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: St Peters Basilica_0' to iChannel0
// Connect Image 'Texture: Wood' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel2
// Connect Image 'Texture: Wood' to iChannel3
// Connect Image 'Cubemap: St Peters Basilica Blurred_0' to iChannel4

// Nintendo Switch by jackdavenport
// All code is free to use with credit! :)
// Created 2019

//------------------------------------------------------------------------------------------------//
#define MAX_ITER 128
#define MIN_DIST 0.001f
#define MAX_DIST 20.0f
#define REFL_COUNT 3

//#define DEBUG_NO_LIGHT      // uncomment to disable shading
//#define DEBUG_SHOW_NORMALS  // uncomment to display normals

//------------------------------------------------------------------------------------------------//


//------------------------------------------------------------------------------------------------//
__DEVICE__ float dstJoystick(float3 p) {
    return _fminf(
      dstCappedCylinder(p-to_float3(0.0f,0.03f,0.0f),0.04f,0.06f),
      dstCappedCylinder(p-to_float3(0.0f,0.11f,0.0f),0.04f,0.0005f)-0.02f);
}
__DEVICE__ float dstButtonGrid(float3 p) {
    float d = dstCappedCylinder(p,0.03f,0.005f);
    p.x -= 0.12f; d = _fminf(d, dstCappedCylinder(p,0.03f,0.005f));
    p.x += 0.06f;
    p.z -= 0.08f; d = _fminf(d, dstCappedCylinder(p,0.03f,0.005f));
    p.z += 0.15f; d = _fminf(d, dstCappedCylinder(p,0.03f,0.005f));
    return d;
}
__DEVICE__ float2 dstSceneI(float3 p, float3 consoleRot[2]) {
    float2 dst;
    // ground
    dst = to_float2(dstPlane(p, to_float4(0.0f,1.0f,0.0f,-0.05f)), 0.0f);
    // console body
    p.y -= 0.04f;
    
    p += consoleRot[1];
    //swi2S(p,y,z, rot2D(swi2(p,y,z), consoleRot));
    swi2S(p,y,z, rot2D(swi2(p,y,z), consoleRot[0].x));   // Aufstellen  
    swi2S(p,x,y, rot2D(swi2(p,x,y), consoleRot[0].y));   // Kippen rechts links
    swi2S(p,x,z, rot2D(swi2(p,x,z), consoleRot[0].z));   // Drehen rechts links
    
    float3 baseBox = to_float3(0.85f, 0.025f, 0.5f);
    if(_fabs(p.x) <= 1.0f) {
      float base = dstRoundBox(p, baseBox, 0.025f);
        if(p.z < -0.45f) {
        base = dstSubtract(base, dstRoundBox(p-to_float3(0.0f,0.015f,-0.5f), to_float3(0.007f,0.001f,0.003f), 0.025f));
        base = dstSubtract(base, dstRoundBox(p-to_float3(-0.09f,0.015f,-0.5f), to_float3_s(0.002f), 0.025f));
        base = dstSubtract(base, dstRoundBox(p-to_float3(0.09f,0.015f,-0.5f), to_float3_s(0.002f), 0.025f));
        } else if(p.z > 0.45f) {
            if(p.x > -0.5f && p.x < -0.16f) {
                float3 q = p-to_float3(0.0f,0.0f,0.53f);
                q.x = mod_f(q.x,0.07f);
              base = dstSubtract(base, dstBox(q, to_float3(0.045f,0.025f,0.025f)));
            }
            base = dstSubtract(base, dstRoundBox(p-to_float3(-0.74f,-0.025f,0.53f), to_float3(0.06f,0.03f,0.03f), 0.01f));
            base = _fminf(base, dstRoundBox(p-to_float3(-0.74f,-0.02f,0.505f), to_float3(0.06f,0.02f,0.005f), 0.01f));
            base = _fminf(base, dstRoundBox(p-to_float3(0.65f,0.0f,0.52f), to_float3(0.05f,0.01f,0.01f), 0.01f));
        }
      dst = dstUnion(dst, base, 1.0f);
    }
    // joycons
    if(_fabs(p.x) > 0.8f) {
        // base
      float3 s = to_float3(0.93f,1.0f,0.55f);
      float cutout = dstBox(p, baseBox * to_float3(1.0f,3.5f,1.2f));
      cutout = dstSubtract(dstCappedCylinder(dstElongate(p/s, to_float3(0.7f,0.0005f,0.47f)), 0.5f, 0.025f)-0.035f, cutout);
      dst = dstUnion(dst, cutout, 2.0f);
      // buttons/joysticks
      float intShape = dstRoundBox(p-to_float3(0.0f,0.07f,-0.02f), baseBox * to_float3(1.0f,1.0f,0.5f), 0.3f);
      if(p.x > 0.8f) { // left controls
          dst = dstUnion(dst, dstJoystick(p-to_float3(0.98f,0.0f,0.25f)), 3.0f);  // left stick
          dst = dstUnion(dst, dstButtonGrid(p-to_float3(0.93f,0.068f,0.0f)), 3.0f);  // left buttons
          dst = dstUnion(dst, dstBox(p-to_float3(0.94f,0.043f,-0.22f),to_float3_s(0.025f)), 3.0f); // capture button
          dst = dstUnion(dst, dstBox(p-to_float3(0.94f,0.044f,0.42f),to_float3(0.025f,0.025f,0.005f)), 3.0f); // - button
          dst = dstUnion(dst, dstIntersect(dstBox(p-to_float3(1.0f,0.0f,0.44f),to_float3(0.13f,0.025f,0.1f)), intShape), 3.0f); // r button
          float3 q = p-to_float3(0.97f,-0.07f,0.37f);
          swi2S(q,x,z, rot2D(swi2(q,x,z), 0.5f));
          dst = dstUnion(dst, dstIntersect(dstRoundBox(q,to_float3(0.1f,0.04f,0.04f),0.042f), intShape), 3.0f); // zl button
        } else if(p.x < -0.8f) { // right controls
          dst = dstUnion(dst, dstJoystick(p-to_float3(-0.99f,0.0f,-0.1f)), 3.0f); // right stick
          dst = dstUnion(dst, dstButtonGrid(p-to_float3(-1.05f,0.065f,0.2f)), 3.0f);  // right buttons
          dst = dstUnion(dst, dstCappedCylinder(p-to_float3(-0.92f,0.065f,-0.22f),0.03f,0.005f), 4.0f); // home button
          dst = dstUnion(dst, dstBox(p-to_float3(-0.92f,0.044f,0.42f),to_float3(0.025f,0.025f,0.005f)), 3.0f); // + button
          dst = dstUnion(dst, dstBox(p-to_float3(-0.92f,0.044f,0.42f),to_float3(0.005f,0.025f,0.025f)), 3.0f);
          dst = dstUnion(dst, dstIntersect(dstBox(p-to_float3(-1.0f,0.0f,0.44f),to_float3(0.13f,0.025f,0.1f)), intShape), 3.0f); // r button
          float3 q = p-to_float3(-0.97f,-0.07f,0.37f);
          swi2S(q,x,z, rot2D(swi2(q,x,z), -0.5f));
          dst = dstUnion(dst, dstIntersect(dstRoundBox(q,to_float3(0.1f,0.04f,0.04f),0.042f), intShape), 3.0f); // zr button
          q = p-to_float3(-0.97f,0.006f,-0.42f);
          swi2S(q,x,z, rot2D(swi2(q,x,z), 0.525f));
          dst = dstUnion(dst, dstIntersect(dstRoundBox(q,to_float3(0.05f,0.005f,0.04f),0.022f), intShape), 4.0f); // ir sensor
        }
    }
    // end scene
    return dst;
}

__DEVICE__ float2 raymarchI(float3 ro, float3 rd, in float maxDist, float3 consoleRot[2]) {
    float2 t = to_float2(0.0f,-1.0f);
    
    
    for(int i = 0; i < MAX_ITER; i++) {
        float2 d = dstSceneI(ro+rd*t.x,consoleRot);
        if(d.x < MIN_DIST || t.x >= maxDist) {
            t.y = d.y;
            break;
        }
        // multiplied to reduce visual artefacts
        // if anyone knows a way to avoid doing this, let me know :)
        t.x += d.x * 0.5f;
    }
    return t;
}

// source: https://iquilezles.org/articles/rmshadows
__DEVICE__ float softshadowI( in float3 ro, in float3 rd, float mint, float maxt, float k, float3 consoleRot[2] )
{
    float res = 1.0f;
    for( float t=mint; t < maxt; )
    {
        float h = dstSceneI(ro + rd*t,consoleRot).x;
        if( h<0.001f )
            return 0.0f;
        res = _fminf( res, k*h/t );
        t += h;
    }
    return res;
}

__DEVICE__ float3 calcNormalI(float3 p, float t, float3 consoleRot[2]) {
    float2 e = to_float2(MIN_DIST*t,0.0f);
    float3 n = to_float3(dstSceneI(p+swi3(e,x,y,y),consoleRot).x-dstSceneI(p-swi3(e,x,y,y),consoleRot).x,
                         dstSceneI(p+swi3(e,y,x,y),consoleRot).x-dstSceneI(p-swi3(e,y,x,y),consoleRot).x,
                         dstSceneI(p+swi3(e,y,y,x),consoleRot).x-dstSceneI(p-swi3(e,y,y,x),consoleRot).x);
    return normalize(n);
}

__DEVICE__ float3 calcLightingI(float3 p, float3 n, float3 rd, Material mat, in float3 col,
                                float3 ambientLight, float3 lightPos1, float3 lightPos2, float lightIntensity, float3 consoleRot[2], bool Light) {
  if(Light)
  {
    float3 diff = ambientLight;
    float3 spec = to_float3_s(0.0f);
    
    for(int i = 0; i < 2; i++) {
        // calc light vector and distance
      float3 lv = (i == 0 ? lightPos1 : lightPos2) - p;
      float ld = length(lv);
      lv /= ld;
    
      // calculate shadows
      float shadow = softshadowI(p, lv, 0.01f, ld, 8.0f, consoleRot);
    
      // calculate lighting
      float ndotl = _fmaxf(dot(n,lv),0.0f);
      diff += ndotl * shadow * lightIntensity;
      if(dot(mat.specular,mat.specular) > 0.0f) {
          float3 h = normalize(lv - rd);
          float ndoth = _fmaxf(dot(n,h),0.0f);
          spec += mat.specular * _powf(ndoth, mat.shininess) * shadow * lightIntensity;
      }
    }
        
    // output final colour
    col = mat.albedo * diff + spec + mat.emission;
  }
  else
    col = mat.albedo + mat.emission; // albedo -> Umgebung (und Gerät) emission -> Anzeige

    return col;
}

__DEVICE__ float3 shadeI(float3 ro, float3 rd,
                         float3 ambientLight, float3 lightPos1, float3 lightPos2, float lightIntensity, float3 consoleRot[2], float4 texch0, bool Light, bool ShowNormals,
                         __TEXTURE2D__ iChannel3,  __TEXTURE2D__ iChannel4, __TEXTURE2D__ iChannel2) {
    float3 col = to_float3_s(0.0f);
    float coeff = 1.0f;
    float3 a = to_float3_s(0.0f);
    Material mat;
    
    for(int i = 0; i < REFL_COUNT; i++) {
      float2 scn = raymarchI(ro, rd, MAX_DIST, consoleRot);
    
      if(scn.y > -1.0f && scn.x < MAX_DIST) {
          float3 p = ro + rd * scn.x;
          float3 n = calcNormalI(p, scn.x, consoleRot);
            
          if(!ShowNormals)
          {
            mat = getMaterial(mat, p, n, scn, consoleRot, texch0, iChannel3, iChannel2);
            a = calcLightingI(p, n, rd, mat, a, ambientLight, lightPos1, lightPos2, lightIntensity, consoleRot, Light);
          
            if(i == 0) {
                coeff *= 1.0f-_saturatef((scn.x-5.0f) / 7.5f);
            }
              
            if(mat.reflectivity > 0.0f) {
                float fres = 1.0f-clamp(_powf(_fmaxf(-dot(rd,n),0.0f),0.4f),0.0f,1.0f);
                fres *= mat.reflectivity;
                  
                col += a * coeff * (1.0f - fres);
                coeff *= fres;
                  
                float3 r = normalize(reflect(rd,n));
                ro = p + r * 0.01f;
                rd = r;
              } else {
                  col += a * coeff;
                  break;
              }
          }
          else
          {
            col = n * 0.5f + 0.5f;
            break;
          }
        } else if(i > 0) {
            col += swi3(decube_f3(iChannel4,rd),x,y,z) * coeff;
            break;
        } else {
            break;
        }
    }
        
    // post processing
    //col = _powf(col, to_float3_s(1.0f));
    col = ACESFilm(col);
    return col; //mat.albedo + mat.emission;//a;//
}

__KERNEL__ void NintendoSwitchFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3, sampler2D iChannel4)
{

    CONNECT_CHECKBOX0(DisplayTexture, 0);
    CONNECT_CHECKBOX1(Light, 1);
    CONNECT_CHECKBOX2(ShowNormals, 0);

    CONNECT_POINT0(TexGroundXY, 0.0f, 0.0f );
    CONNECT_SLIDER0(TexGroundZ, -10.0f, 10.0f, 1.0f);

    CONNECT_COLOR0(LightPos1, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR1(LightPos2, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_SLIDER1(lightIntensity, -1.0f, 5.0f, 1.05f);
    CONNECT_COLOR2(AmbientLight, 0.5f, 0.5f, 0.5f, 1.0f);
    
    
    CONNECT_SLIDER3(consoleRotX, -10.0f, 10.0f, 0.15f);
    CONNECT_SLIDER4(consoleRotY, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER5(consoleRotZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER6(consoleMoveX, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(consoleMoveY, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER8(consoleMoveZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_POINT1(ViewXY, 0.0f, 0.0f );
    CONNECT_SLIDER9(ViewZ, -10.0f, 10.0f, 0.0f);
    
    CONNECT_POINT2(LookXY, 0.0f, 0.0f );
    CONNECT_SLIDER10(LookZ, -10.0f, 10.0f, 0.0f);
    
    //Dummy
    mat2 m2;
    mat3 m3;
    
    float3 consoleRot[2] = {
                            to_float3(consoleRotX,consoleRotY,consoleRotZ),
                            to_float3(consoleMoveX,consoleMoveY,consoleMoveZ)
                           };
   
    const float3 lightPos1 = to_float3(3.0f,1.5f,-2.0f) + (swi3(LightPos1,x,y,z)-0.5f)*5.0f;
    const float3 lightPos2 = to_float3(-3.0f,3.5f,2.0f) + (swi3(LightPos2,x,y,z)-0.5f)*5.0f;
    //const float lightIntensity = 1.05f;
    const float3 ambientLight = to_float3(0.2f,0.2f,0.1f) + (swi3(AmbientLight,x,y,z)-0.5f)*5.0f;
    //const float consoleRot = 0.15f;

    float ratio = iResolution.y/iResolution.x;
    float4 texch0 = to_float4(TexGroundXY.x,TexGroundXY.y,TexGroundZ,ratio);

    float2 uv = (fragCoord - iResolution * 0.5f) / iResolution.y;
    float3 ro = to_float3(0.0f,1.2f,0.0f);
    float3 rd = to_float3_aw(uv, 1.0f)+to_float3_aw(LookXY,LookZ);
  
    ro = cameraPos(ro, iTime * 0.06f, iMouse, iResolution)+to_float3_aw(ViewXY,ViewZ);
    rd = lookAt(to_float3(0.0f,0.0f,0.0f), ro, to_float3(0.0f,1.0f,0.0f), rd);
    
    if(DisplayTexture)
      fragColor = to_float4_aw(shadeI(ro, normalize(rd), ambientLight, lightPos1, lightPos2, lightIntensity, consoleRot, texch0, Light, ShowNormals, iChannel3, iChannel4, iChannel1), 1.0f);
    else  
      fragColor = to_float4_aw(shadeI(ro, normalize(rd), ambientLight, lightPos1, lightPos2, lightIntensity, consoleRot, texch0, Light, ShowNormals, iChannel3, iChannel4, iChannel2), 1.0f);
        
  SetFragmentShaderComputedColor(fragColor);
}

