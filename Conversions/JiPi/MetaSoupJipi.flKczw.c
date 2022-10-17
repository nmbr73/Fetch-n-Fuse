
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


// Define a very large value, will be used as infinity
#define INF 1e5
#define PI _acosf(-1.0f)

#define N_SPHERES 13

struct Hit 
{
    float t;
    float3 point;
    float3 normal;
    int objId;
};

__DEVICE__ void swap(inout float *x0, inout float *x1)
{
    float tmp = *x0;
    *x0 = *x1;
    *x1 = tmp;
}

__DEVICE__ bool solveQuadratic(float a, float b, float c, out float *x0, out float *x1) 
{ 

    float delta = b*b-4.0f*a*c; 
    if (delta < 0.0f) return false; 
    else if (delta == 0.0f) 
    {
        *x0 = *x1 = -0.5f*b/a; 
    }
    else 
    { 
        float q = (b > 0.0f) ? -0.5f * (b + _sqrtf(delta)) : -0.5f * (b - _sqrtf(delta)); 
        *x0 = q/a; 
        *x1 = c/q; 
    } 
    
    return true; 
} 

__DEVICE__ bool traceSphere(float3 eye, float3 ray, float3 center, float radius, out Hit *hit)
{ 
    float t0, t1;

    float3 L = eye-center;
    float a = dot(ray,ray);
    float b = 2.0f * dot(ray,L); 
    float c = dot(L,L) - (radius*radius);
    if (!solveQuadratic(a, b, c, &t0, &t1)) return false; 
    
    if (t0 > t1) swap(&t0, &t1); 

    if (t0 < 0.0f) 
    { 
        t0 = t1;  //if t0 is negative, let's use t1 instead 
        if (t0 < 0.0f) return false;  //both t0 and t1 are negative 
    } 

    (*hit).t = t0;
    (*hit).point = eye + t0*ray;
    (*hit).normal = normalize((*hit).point - center);
    
    return true; 
} 

__DEVICE__ bool tracePlane(float3 origin, float3 dir, float3 normal, float3 P, out Hit *hit) 
{
    float nl = dot(normal,dir);
    if(-nl < 0.000001f) return false; // line and plane parallel
    float t = dot(P-origin,normal)/nl;
    
    (*hit).t = t;
    (*hit).point = origin+t*dir;
    (*hit).normal = normal;
    
    return true;
}

__DEVICE__ float checkBoardTexture(float2 p)
{
    return mod_f(_floor(p.y),2.0f);
}

struct Material
{
    float3 Fresnel0;   // Fresnel value when the light incident angle is 0 (angle between surface normal and to light direction)
    float roughness; // Roughness in [0,1] 
    float3 albedo;
};

// Palette generator tnx Iq
__DEVICE__ float3 getColor(float3 a, float3 b, float3 c, float3 d, float t)
{
    return a + b*cos_f3(2.0f*PI*(t*c+d));
}

// Color palette
__DEVICE__ float3 samplePalette(float t)
{
     float3 a = to_float3(0.5f,0.5f,0.5f);
     float3 b = to_float3(0.5f,0.5f,0.5f);
     float3 c = to_float3(1.0f,1.0f,1.0f);
     float3 d = to_float3(0.0f,0.2f,0.4f);
     
     return getColor(a,b,c,d,t);
} 



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel0


// Francesco S. Spoto
//
// The field is generated from a bounch of metaballs and from the horizontal plane y=0.
//
// First ray trace the scene (metaballs bounding spheres and bottom plane) to get the nearest field point,
// then ray march inside it until the searched level surface.
//
// I will use the quintic polinomial as falloff, that allows to compute analytical derivative,
// as shown by IQ in https://www.shadertoy.com/view/ld2GRz (you're the best ).
//

__DEVICE__ Material getMaterial(int matId, int objId)
{
    Material mat;
    mat.albedo = to_float3_s(1);
    mat.albedo = samplePalette((float)(objId*10)/800.0f);   ;
    return mat;
}

__DEVICE__ float3 render(float3 eye, Hit hit, __TEXTURE2D__ iChannel0)
{
    // No hit -> background
    if(hit.objId == -1) return to_float3_s(0);
    
    
    // Get the material of the hitten object at the hitten point
    Material mat = getMaterial(hit.objId, 6);
    
    // Define lights in the scene
    float3 KeyLightPos = to_float3(0,1,0);
    float3 KeyLightColor = to_float3(0.5f,0.8f,0.1f);
    float3 fillLightPos = to_float3(0,1,0);
    float3 fillLightColor = to_float3_s(0.65f);
    float3 backLightPos = to_float3(-1,-1,-1);
    float3 backLightColor = to_float3_s(0.5f);
    
    float3 ambientLight = to_float3(0.1f,0.1f,0.1f);
    
    // Vector used in the computation of lighting
    float3 N = hit.normal;    // Surface normal
    float3 L = normalize(KeyLightPos);   // To light dir
    float3 V = normalize(eye-hit.point); // To eye vector
    float3 H = normalize(V+L);      // Half vector between to light direction and to eye direction
    float3 R = reflect(-L,N);
    
    // Compute different light components
    float3 ambient = ambientLight * mat.albedo;
    float3 diffuse = _fmaxf(dot(N,L),0.0f) * KeyLightColor * mat.albedo;
    diffuse += _fmaxf(dot(N,normalize(fillLightPos)),0.0f) * fillLightColor * mat.albedo;
    diffuse += _fmaxf(dot(N,normalize(backLightPos)),0.0f) * backLightColor * mat.albedo;
    
    diffuse += 0.07f*swi3(decube_f3(iChannel0,R),x,y,z);
    float specular = _powf(_fmaxf(dot(R,V),0.0f),10.0f);
    float fresnel = 0.2f * _powf(1.0f+dot(N,-V),4.0f);
    return  diffuse + 0.4f*specular + 2.0f*fresnel;
}

struct Sphere
{
    float3 center;
    float radius;
};


 
// Get the value of the falloff function and the derivative for a value of x in [0,1]
__DEVICE__ void getFalloff(float x, out float *f)
{
    *f = 0.0f;
    if(x<0.01||x>1.0f) return; // Field is 0 outside [0,1]
    
    // Quintic falloff 1-6x^5 - 15x^4 + 10x^3
    *f = 1.0f-(x*x*x*(6.0f*x*x-15.0f*x + 10.0f));
}

__DEVICE__ void getFalloffDerivative(float x, out float *df)
{
    *df = 0.0f;
    if(x<0.01||x>1.0f) return; // Field is 0 outside [0,1]
   
    // Quintic fallof derivative 1-(30x^4 - 60x^2 + 30x)
    *df = -(x*x*(30.0f*x*x - 60.0f*x + 30.0f));
}

__DEVICE__ bool hasFieldValue(float3 p, float threshold, out float *value, out float3 *normal, Sphere spheres[N_SPHERES])
{
    *normal = to_float3_s(0);
    float f, df;
    *value = 0.0f;
    
    // Compute the field generated both from the spheres ...
    for(int i=0;i<N_SPHERES;i++)
    {
        if(length(p-spheres[i].center)>spheres[i].radius) continue; // Test against sphere BBox
        float d = length((p-spheres[i].center)/spheres[i].radius);
        getFalloff(d, &f);
        *value += f;
    }
    
    // .. and add the field generated from the plane
    float d = clamp(p.y,0.0f,1.0f);
    getFalloff(d, &f);
    *value += f;
    
    // If we are over the threshold compute also the normal
    if(*value>=threshold)
    {
        float df=0.0f;
        
        // Compute the normals for the spheres ...
        for(int i=0;i<N_SPHERES;i++) 
        {
            float d = length((p-spheres[i].center)/spheres[i].radius);
            getFalloffDerivative(d, &df);
            *normal += df*normalize(spheres[i].center-p);
        }
        
        // ... add the normal for the plane
        float d = clamp(p.y,0.0f,1.0f);
        getFalloffDerivative(d, &df);
        *normal += df*to_float3(0,-1,0); // Normal to the planeThe field increase toward negative y, so the gradient is (0,-1,0)
        *normal = normalize(*normal);
        return true;
    }
    
    // Not reached the threshold yet
    return false;
}


// Find the nearest distance from origin to the field generated by the spheres on the dir direction, 
// return INF if the ray does not intersect the field
__DEVICE__ Hit getDistanceToField(float3 origin, float3 dir, Sphere spheres[N_SPHERES])
{
    float minT = INF;
    float point;
    Hit hit, result;
  
    // Get nearest point from the spheres
    result.t = INF;
    for(int i=0; i<N_SPHERES; i++)
    {
        if(traceSphere(origin, dir, spheres[i].center, spheres[i].radius, &hit) && hit.t<result.t) 
        {
            result.t = hit.t;
            result.point = origin + result.t*dir;
            result.objId = 1;
        }
    }
    
    // Get the nearest point from the plane
    if(tracePlane(origin, dir, to_float3(0,1,0), to_float3(0,1,0), &hit) && hit.t<result.t)
    {
        result.t = hit.t;
        result.point = origin + result.t*dir;
        result.normal = to_float3(0,1,0);
        result.objId = 2;
    }
    
    return result;
}

// Update the position of the objects in the scene
__DEVICE__ void updateScene(int iFrame, Sphere spheres[N_SPHERES], float iTime)
{
    for(int i=0; i<N_SPHERES; i++)
    {
        float id = (float)(i);
        float a = 2.0f;
        //float upDown = _sinf(_fmaxf(float(iFrame-30),0.0f)*0.01f)*_fmaxf(30.0f-float(iFrame),1.0f);
        spheres[i].center = to_float3(a*_sinf((float)(iFrame)/200.0f+id*244.0f), 1.4f*_fabs(_sinf(id*0.1f*iTime+323.3f))-1.0f, a*_sinf((float)(iFrame)/100.0f+id*1724.0f)) * 2.0f;   
    }
}

__DEVICE__ float3 castRay(float3 origin, float3 dir, int iFrame, float iTime, __TEXTURE2D__ iChannel0)
{
  
  // Define metaballs
Sphere spheres[N_SPHERES] = {
     {to_float3(0,0,0),1.0f}
    ,{to_float3(0,0,0),2.0f}
    ,{to_float3(0,0,0),3.0f}
    ,{to_float3(0,0,0),4.0f}
    ,{to_float3(0,0,0),5.0f}
    ,{to_float3(0,0,0),2.0f}
    ,{to_float3(0,0,0),3.0f}
    ,{to_float3(0,0,0),4.0f}
    ,{to_float3(0,0,0),5.0f}
    ,{to_float3(0,0,0),2.0f}
    ,{to_float3(0,0,0),3.0f}
    ,{to_float3(0,0,0),4.0f}
    ,{to_float3(0,0,0),5.0f}
};
  
  
  
    updateScene(iFrame, spheres, iTime);
    
    float3 fieldNormal;
    
    float MAX_DISTANCE = 20.0f;
    int MAX_ITERATIONS = 1000;

    Hit hitPlane;
    hitPlane.objId=-1;
    if(tracePlane(origin, dir, to_float3(0,1,0), to_float3(0,1,0), &hitPlane)) 
    {
        hitPlane.objId=1;
        hitPlane.normal = to_float3(0,1,0);
    }
    
    // Get to the field nearest point
    Hit hit = getDistanceToField(origin, dir, spheres);
    float t = hit.t;
    if(t==INF) return render(origin, hitPlane, iChannel0); // No hit -> return
    
    // Ray march inside the field
    float3 p;
    float value = 0.0f;
    float threshold = 0.4f;
    for(int i=0; i<MAX_ITERATIONS; i++)
    {
        if(t>MAX_DISTANCE) break; // No hit
        t+=0.01f;
        p = origin + t*dir;
        if(p.y<0.0f) break; // No hit 
        
        if(hasFieldValue(p, threshold, &value, &fieldNormal, spheres))
        {
            hit.t=t;
            hit.point=p;
            hit.normal=fieldNormal;
            hit.objId=1;            
            break;
        }
    }
    
    if(value < 0.4f) hit = hitPlane;
    
    return render(origin, hit, iChannel0);
}

__KERNEL__ void MetaSoupJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Mousecontrol, 0);
  
    // Change coordinates from [0,iResolution.x]X[0,iResolution.y] to [-1,1]X[-1,-1]
    float2 U = (2.0f*fragCoord - iResolution) / iResolution.x;

    // Right-handed camera reference system
    
        
    float tetha = iTime*0.3f;
    float phi = PI/4.0f;
    
    if(Mousecontrol)
    {  
      tetha = iMouse.x/iResolution.x *2.0f*PI;
      phi = iMouse.y/iResolution.y*PI;  
    }
    
    float3 eye = to_float3(12.0f*_cosf(tetha)*_sinf(phi),12.0f*_cosf(phi),12.0f*_sinf(tetha)*_sinf(phi));
    float3 target = to_float3(0,0,0);
float IIIIIIIIIIIIIIIIIIIIIIIIIIII;   
    // Reference frame
    float3 ww = normalize(eye-target);
    float3 uu = normalize(cross(to_float3(0,1,0),ww));
    float3 vv = normalize(cross(ww,uu));
    
    // Cast a ray from orgin through pixel, into the scene
    float focalLength = 1.4f;
    float3 ray = normalize(U.x*uu + U.y*vv - focalLength*ww);
    
    // Cast a ray into the scene and sample the pixel color
    fragColor = to_float4_aw(castRay(eye,ray, iFrame, iTime, iChannel0),1.0f);
    
    // Gamma correction
    fragColor = pow_f4(fragColor,to_float4_s(0.45f));


  SetFragmentShaderComputedColor(fragColor);
}