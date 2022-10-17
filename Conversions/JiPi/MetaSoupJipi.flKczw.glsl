

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

Material getMaterial(int matId, int objId)
{
    Material mat;
    mat.albedo = vec3(1);
    mat.albedo = samplePalette(float(objId*10)/800.);   ;
    return mat;
}

vec3 render(vec3 eye, Hit hit)
{
    // No hit -> background
    if(hit.objId == -1) return vec3(0);
    
    // Get the material of the hitten object at the hitten point
    Material mat = getMaterial(hit.objId, 6);
    
    // Define lights in the scene
    vec3 KeyLightPos = vec3(0,1,0);
    vec3 KeyLightColor = vec3(0.5,0.8,0.1);
    vec3 fillLightPos = vec3(0,1,0);
    vec3 fillLightColor = vec3(0.65);
    vec3 backLightPos = vec3(-1,-1,-1);
    vec3 backLightColor = vec3(0.5);
    
    vec3 ambientLight = vec3(0.1,0.1,0.1);
    
    // Vector used in the computation of lighting
    vec3 N = hit.normal;    // Surface normal
    vec3 L = normalize(KeyLightPos);   // To light dir
    vec3 V = normalize(eye-hit.point); // To eye vector
    vec3 H = normalize(V+L);      // Half vector between to light direction and to eye direction
    vec3 R = reflect(-L,N);
    
    // Compute different light components
    vec3 ambient = ambientLight * mat.albedo;
    vec3 diffuse = max(dot(N,L),0.) * KeyLightColor * mat.albedo;
    diffuse += max(dot(N,normalize(fillLightPos)),0.) * fillLightColor * mat.albedo;
    diffuse += max(dot(N,normalize(backLightPos)),0.) * backLightColor * mat.albedo;
    
    diffuse += 0.07*texture(iChannel0, R).xyz;
    float specular = pow(max(dot(R,V),0.),10.);
    float fresnel = 0.2 * pow(1.+dot(N,-V),4.);
    return  diffuse + 0.4*specular + 2.*fresnel;
}

struct Sphere
{
    vec3 center;
    float radius;
};

// Define metaballs
#define N_SPHERES 13
Sphere spheres[N_SPHERES] = Sphere[N_SPHERES]
(
     Sphere(vec3(0,0,0),1.)
    ,Sphere(vec3(0,0,0),2.)
    ,Sphere(vec3(0,0,0),3.)
    ,Sphere(vec3(0,0,0),4.)
    ,Sphere(vec3(0,0,0),5.)
    ,Sphere(vec3(0,0,0),2.)
    ,Sphere(vec3(0,0,0),3.)
    ,Sphere(vec3(0,0,0),4.)
    ,Sphere(vec3(0,0,0),5.)
    ,Sphere(vec3(0,0,0),2.)
    ,Sphere(vec3(0,0,0),3.)
    ,Sphere(vec3(0,0,0),4.)
    ,Sphere(vec3(0,0,0),5.)
);
 
// Get the value of the falloff function and the derivative for a value of x in [0,1]
void getFalloff(float x, out float f)
{
    f = 0.;
    if(x<0.01||x>1.) return; // Field is 0 outside [0,1]
    
    // Quintic falloff 1-6x^5 - 15x^4 + 10x^3
    f = 1.-(x*x*x*(6.*x*x-15.*x + 10.));
}

void getFalloffDerivative(float x, out float df)
{
    df = 0.;
    if(x<0.01||x>1.) return; // Field is 0 outside [0,1]
   
    // Quintic fallof derivative 1-(30x^4 - 60x^2 + 30x)
    df = -(x*x*(30.*x*x - 60.*x + 30.));
}

bool hasFieldValue(vec3 p, float threshold, out float value, out vec3 normal)
{
    normal = vec3(0);
    float f, df;
    value = 0.;
    
    // Compute the field generated both from the spheres ...
    for(int i=0;i<N_SPHERES;i++)
    {
        if(length(p-spheres[i].center)>spheres[i].radius) continue; // Test against sphere BBox
        float d = length((p-spheres[i].center)/spheres[i].radius);
        getFalloff(d, f);
        value += f;
    }
    
    // .. and add the field generated from the plane
    float d = clamp(p.y,0.,1.);
    getFalloff(d, f);
    value += f;
    
    // If we are over the threshold compute also the normal
    if(value>=threshold)
    {
        float df=0.;
        
        // Compute the normals for the spheres ...
        for(int i=0;i<N_SPHERES;i++) 
        {
            float d = length((p-spheres[i].center)/spheres[i].radius);
            getFalloffDerivative(d, df);
            normal += df*normalize(spheres[i].center-p);
        }
        
        // ... add the normal for the plane
        float d = clamp(p.y,0.,1.);
        getFalloffDerivative(d, df);
        normal += df*vec3(0,-1,0); // Normal to the planeThe field increase toward negative y, so the gradient is (0,-1,0)
        normal = normalize(normal);
        return true;
    }
    
    // Not reached the threshold yet
    return false;
}


// Find the nearest distance from origin to the field generated by the spheres on the dir direction, 
// return INF if the ray does not intersect the field
Hit getDistanceToField(vec3 origin, vec3 dir)
{
    float minT = INF;
    float point;
    Hit hit, result;
    
    // Get nearest point from the spheres
    result.t = INF;
    for(int i=0; i<N_SPHERES; i++)
    {
        if(traceSphere(origin, dir, spheres[i].center, spheres[i].radius, hit) && hit.t<result.t) 
        {
            result.t = hit.t;
            result.point = origin + result.t*dir;
            result.objId = 1;
        }
    }
    
    // Get the nearest point from the plane
    if(tracePlane(origin, dir, vec3(0,1,0), vec3(0,1,0), hit) && hit.t<result.t)
    {
        result.t = hit.t;
        result.point = origin + result.t*dir;
        result.normal = vec3(0,1,0);
        result.objId = 2;
    }
    
    return result;
}

// Update the position of the objects in the scene
void updateScene()
{
    for(int i=0; i<N_SPHERES; i++)
    {
        float id = float(i);
        float a = 2.;
        //float upDown = sin(max(float(iFrame-30),0.)*0.01)*max(30.-float(iFrame),1.);
        spheres[i].center = vec3(a*sin(float(iFrame)/200.+id*244.),1.4*abs(sin(id*0.1*iTime+323.3))-1.,a*sin(float(iFrame)/100.+id*1724.)) * 2.;   
    }
}

vec3 castRay(vec3 origin, vec3 dir)
{
    updateScene();
    
    vec3 fieldNormal;
    
    float MAX_DISTANCE = 20.;
    int MAX_ITERATIONS = 1000;

    Hit hitPlane;
    hitPlane.objId=-1;
    if(tracePlane(origin, dir, vec3(0,1,0), vec3(0,1,0), hitPlane)) 
    {
        hitPlane.objId=1;
        hitPlane.normal = vec3(0,1,0);
    }
    
    // Get to the field nearest point
    Hit hit = getDistanceToField(origin, dir);
    float t = hit.t;
    if(t==INF) return render(origin,hitPlane); // No hit -> return
    
    // Ray march inside the field
    vec3 p;
    float value=0.;
    float threshold = 0.4;
    for(int i=0; i<MAX_ITERATIONS; i++)
    {
        if(t>MAX_DISTANCE) break; // No hit
        t+=0.01;
        p = origin + t*dir;
        if(p.y<0.) break; // No hit 
        
        if(hasFieldValue(p, threshold, value, fieldNormal))
        {
            hit.t=t;
            hit.point=p;
            hit.normal=fieldNormal;
            hit.objId=1;            
            break;
        }
    }
    
    if(value < 0.4) hit=hitPlane;
    
    return render(origin,hit);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{  
    // Change coordinates from [0,iResolution.x]X[0,iResolution.y] to [-1,1]X[-1,-1]
    vec2 U = (2.*fragCoord.xy - iResolution.xy) / iResolution.x;

    // Right-handed camera reference system
    //float tetha = iMouse.x/iResolution.x *2.*PI;
    //float phi = iMouse.y/iResolution.y*PI;
    float tetha = iTime*0.3;
    float phi = PI/4.;
    vec3 eye = vec3(12.*cos(tetha)*sin(phi),12.*cos(phi),12.*sin(tetha)*sin(phi));
    vec3 target = vec3(0,0,0);
   
    // Reference frame
    vec3 ww = normalize(eye-target);
    vec3 uu = normalize(cross(vec3(0,1,0),ww));
    vec3 vv = normalize(cross(ww,uu));
    
    // Cast a ray from orgin through pixel, into the scene
    float focalLength = 1.4;
    vec3 ray = normalize(U.x*uu + U.y*vv - focalLength*ww);
    
    // Cast a ray into the scene and sample the pixel color
    fragColor = vec4(castRay(eye,ray),1.);
    
    // Gamma correction
    fragColor = pow(fragColor,vec4(0.45));
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Define a very large value, will be used as infinity
#define INF 1e5
#define PI acos(-1.)

struct Hit 
{
    float t;
    vec3 point;
    vec3 normal;
    int objId;
};

void swap(inout float x0, inout float x1)
{
    float tmp = x0;
    x0=x1;
    x1=tmp;
}

bool solveQuadratic(float a, float b, float c, out float x0, out float x1) 
{ 
    float delta = b*b-4.*a*c; 
    if (delta < 0.) return false; 
    else if (delta == 0.) 
    {
        x0 = x1 = -0.5*b/a; 
    }
    else 
    { 
        float q = (b > 0.) ? -0.5 * (b + sqrt(delta)) : -0.5 * (b - sqrt(delta)); 
        x0 = q/a; 
        x1 = c/q; 
    } 
    
    return true; 
} 

bool traceSphere(vec3 eye, vec3 ray, vec3 center, float radius, out Hit hit)
{ 
    float t0, t1;

    vec3 L = eye-center;
    float a = dot(ray,ray);
    float b = 2. * dot(ray,L); 
    float c = dot(L,L) - (radius*radius);
    if (!solveQuadratic(a, b, c, t0, t1)) return false; 
    
    if (t0 > t1) swap(t0, t1); 

    if (t0 < 0.) 
    { 
        t0 = t1;  //if t0 is negative, let's use t1 instead 
        if (t0 < 0.) return false;  //both t0 and t1 are negative 
    } 

    hit.t = t0;
    hit.point = eye + t0*ray;
    hit.normal = normalize(hit.point - center);
    
    return true; 
} 

bool tracePlane(vec3 origin, vec3 dir, vec3 normal, vec3 P, out Hit hit) 
{
    float nl = dot(normal,dir);
    if(-nl < 0.000001) return false; // line and plane parallel
    float t = dot(P-origin,normal)/nl;
    
    hit.t = t;
    hit.point = origin+t*dir;
    hit.normal = normal;
    
    return true;
}

float checkBoardTexture(vec2 p)
{
    return mod(floor(p.y),2.);
}

struct Material
{
    vec3 Fresnel0;   // Fresnel value when the light incident angle is 0 (angle between surface normal and to light direction)
    float roughness; // Roughness in [0,1] 
    vec3 albedo;
};

// Palette generator tnx Iq
vec3 getColor(vec3 a, vec3 b, vec3 c, vec3 d, float t)
{
    return a + b*cos(2.*PI*(t*c+d));
}

// Color palette
vec3 samplePalette(float t)
{
     vec3 a = vec3(0.5,0.5,0.5);
     vec3 b = vec3(.5,0.5,0.5);
     vec3 c = vec3(1.0,1.0,1.0);
     vec3 d = vec3(0.,0.2,0.4);
     
     return getColor(a,b,c,d,t);
} 


