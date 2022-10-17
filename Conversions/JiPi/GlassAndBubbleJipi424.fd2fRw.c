
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery_0' to iChannel0


// Alternative
__DEVICE__ float3 __refract_f3(float3 I, float3 N, float ior) {
    //float cosi = clamp(dot(N,I), -1.0f,1.0f);  //clamp(-1, 1, I.dot(N));
    float cosi = clamp( -1.0f,1.0f,dot(N,I));  //clamp(-1, 1, I.dot(N));
    float etai = 01.0f, etat = ior*1.0f;
    float3 n = N;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        float temp = etai;
        etai = etat;
        etat = temp;
        n = -N;
    }
    float eta = etai / etat;
    float k = 1.0f - (eta * eta) * (1.0f - (cosi * cosi));
    if (k <= 0) {
        return to_float3_s(0.0f);
    } else {
        //return I.multiply(eta).add(n.multiply(((eta * cosi) - Math.sqrt(k))));
    return I*eta+n*((eta*cosi)-_sqrtf(k));   
	  //return eta * I + (eta * cosi - _sqrtf(1.0f-k)) * N;
    }
}

#define refract_f3 __refract_f3

/*
"Glass and Gold Bubble" by Emmanuel Keller aka Tambako - June 2016
License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
Contact: tamby@tambako.ch
*/

#define pi 3.14159265359f

// Switches, you can play with them!
#define bumped_glass
//#define thick_bottom
#define show_gold
#define specular
#define reflections

//#define antialias

struct Lamp
{
    float3 position;
    float3 color;
    float intensity;
    float attenuation;
};
    
struct TransMat
{
    float3 col_vol;
    float3 col_dif;
    float3 col_fil;
    float3 col_dev;
    float specint;
    float specshin;
    float ior;
};

struct RenderData
{
    float3 col;
    float3 pos;
    float3 norm;
    int objnr;
};
   
// Every object of the scene has its ID
#define SKY_OBJ        0
#define BUBBLE_OBJ     1


// Union operation from iq
__DEVICE__ float2 opU(float2 d1, float2 d2)
{
  return (d1.x<d2.x) ? d1 : d2;
}

__DEVICE__ float2 rotateVec(float2 vect, float angle)
{
  
    float2 rv;
    rv.x = vect.x*_cosf(angle) - vect.y*_sinf(angle);
    rv.y = vect.x*_sinf(angle) + vect.y*_cosf(angle);
    return rv;
}

// 1D hash function
__DEVICE__ float hash(float n)
{
    return fract(_sinf(n)*753.5453123f);
}

// From https://www.shadertoy.com/view/4sfGzS
__DEVICE__ float noise(float3 _x)
{
    //x.x = mod_f(x.x, 0.4f);
    float3 p = _floor(_x);
    float3 f = fract_f3(_x);
    f = f*f*(3.0f-2.0f*f);
  
    float n = p.x + p.y*157.0f + 113.0f*p.z;
    return _mix(_mix(_mix(hash(n+  0.0f), hash(n+  1.0f),f.x),
                     _mix(hash(n+157.0f), hash(n+158.0f),f.x),f.y),
                _mix(_mix(hash(n+113.0f), hash(n+114.0f),f.x),
                     _mix(hash(n+270.0f), hash(n+271.0f),f.x),f.y),f.z);
}

__DEVICE__ float bubblePattern(float3 pos)
{
    return noise(normalize(pos)*2.5f);
}

__DEVICE__ float goldValue(float3 pos)
{
    #ifdef show_gold
    pos+= 0.04f*noise(pos*26.7f);
    return smoothstep(0.63f, 0.64f, bubblePattern(pos));
    #else
    return 0.0f;
    #endif
}

__DEVICE__ float bubbleBump(float3 pos,float iTime)
{
    #ifdef bumped_glass
    float wf = 65.0f + 15.0f*_sinf(iTime*0.08f);
    float bp = bubblePattern(pos);
    float sa = smoothstep(0.1f, 0.2f, bp)*smoothstep(0.8f, 0.65f, bp);
    return sa*_sinf(bp*wf);
    #else
    return 0.0f;
    #endif
}

__DEVICE__ float map_bubble(float3 pos,float iTime,
                            float bubbleThickness, float bubbleRadius, float bumpFactor)
{
   #ifdef thick_bottom
   float bubbleThickness2 = bubbleThickness*(1.0f + 500.0f*smoothstep(-0.25f, -0.4f, pos.y/bubbleRadius));
   #else
   float bubbleThickness2 = bubbleThickness;
   #endif
    
   float outside = length(pos) - bubbleRadius;
   outside-= bumpFactor*bubbleBump(pos,iTime);
   float inside = length(pos) - bubbleRadius + bubbleThickness2;
   inside-= bumpFactor*bubbleBump(pos,iTime);
   float df = _fmaxf(outside, -inside);

   //df = _fmaxf(df, pos.z);
   return df;
}

__DEVICE__ float2 map(float3 pos, bool inside,float iTime,
                      float bubbleThickness, float bubbleRadius, float bumpFactor)
{
    float bubble = map_bubble(pos,iTime,bubbleThickness,bubbleRadius,bumpFactor);
    if (inside) bubble*=-1.0f;
    float2 res = to_float2(bubble, BUBBLE_OBJ);
    
    return res;
}

// Main tracing function
__DEVICE__ float2 trace(float3 cam, float3 ray, float maxdist, bool inside,float iTime,
                        float bubbleThickness, float bubbleRadius, float bumpFactor)
{
    float t = 0.1f;
    float objnr = 0.0f;
    float3 pos;
    float dist;
    float dist2;
    
    for (int i = 0; i < 128; ++i)
    {
      pos = ray*t + cam;
        float2 res = map(pos, inside,iTime,bubbleThickness,bubbleRadius,bumpFactor);
        dist = res.x;
        if (dist>maxdist || _fabs(dist)<0.002f)
            break;
        t+= dist*0.43f;
        objnr = _fabs(res.y);
    }
    return to_float2(t, objnr);
}

// From https://www.shadertoy.com/view/MstGDM
// Here the texture maping is only used for the normal, not the raymarching, so it's a kind of bump mapping. Much faster
__DEVICE__ float3 getNormal(float3 pos, float e, bool inside, float iTime,
                            float bubbleThickness, float bubbleRadius, float bumpFactor)
{  
    float2 q = to_float2(0, e);
    return normalize(to_float3(map(pos + swi3(q,y,x,x), inside,iTime,bubbleThickness,bubbleRadius,bumpFactor).x - map(pos - swi3(q,y,x,x), inside,iTime,bubbleThickness,bubbleRadius,bumpFactor).x,
                               map(pos + swi3(q,x,y,x), inside,iTime,bubbleThickness,bubbleRadius,bumpFactor).x - map(pos - swi3(q,x,y,x), inside,iTime,bubbleThickness,bubbleRadius,bumpFactor).x,
                               map(pos + swi3(q,x,x,y), inside,iTime,bubbleThickness,bubbleRadius,bumpFactor).x - map(pos - swi3(q,x,x,y), inside,iTime,bubbleThickness,bubbleRadius,bumpFactor).x));
}

// Gets the color of the sky
__DEVICE__ float3 sky_color(float3 ray, __TEXTURE2D__ iChannel0)
{ 
    return swi3(decube_f3(iChannel0,ray),x,y,z);
}

__DEVICE__ float3 getGoldColor(float3 pos)
{
  // Gold options
    const float3 goldColor = to_float3(1.1f, 0.91f, 0.52f);
    const float3 goldColor2 = to_float3(1.1f, 1.07f, 0.88f);
    const float3 goldColor3 = to_float3(1.02f, 0.82f, 0.55f);
  
    pos+= 0.4f*noise(pos*24.0f);
    float t = noise(pos*30.0f);
    float3 col = _mix(goldColor, goldColor2, smoothstep(0.55f, 0.95f, t));
    col = _mix(col, goldColor3, smoothstep(0.45f, 0.25f, t));
    return col;
}

__DEVICE__ float3 getBubbleColor(float3 pos, struct TransMat glassMat )
{
  return _mix(glassMat.col_dif, getGoldColor(pos), _powf(goldValue(pos), 4.0f));
}
  
// Combines the colors
__DEVICE__ float3 getColor(float3 norm, float3 pos, int objnr, float3 ray, __TEXTURE2D__ iChannel0, struct TransMat glassMat)
{
   return objnr==BUBBLE_OBJ?getBubbleColor(pos, glassMat):sky_color(ray,iChannel0);
}

// Fresnel reflectance factor through Schlick's approximation: https://en.wikipedia.org/wiki/Schlick's_approximation
__DEVICE__ float fresnel(float3 ray, float3 norm, float n2)
{
   float n1 = 1.0f; // air
   float angle = _acosf(-dot(ray, norm));
   //float r0 = dot((n1-n2)/(n1+n2), (n1-n2)/(n1+n2));
   float r0 = ((n1-n2)/(n1+n2) * (n1-n2)/(n1+n2));
   float r = r0 + (1.0f - r0)*_powf(1.0f - _cosf(angle), 5.0f);
   return clamp(r, 0.0f, 0.8f);
}

// Shading of the objects pro lamp
__DEVICE__ float3 lampShading(struct Lamp lamp, float3 norm, float3 pos, float3 ocol, int objnr, int lampnr, float3 campos, struct TransMat glassMat)
{   
    float3 pl = normalize(lamp.position - pos);
    float dlp = distance_f3(lamp.position, pos);
    float3 pli = pl/_powf(1.0f + lamp.attenuation*dlp, 2.0f);
    float dnp = dot(norm, pli);
      
    // Diffuse shading
    float3 col;
    col = ocol*lamp.color*lamp.intensity*smoothstep(-0.1f, 1.0f, dnp); //clamp(dnp, 0.0f, 1.0f);
    
    // Specular shading
    #ifdef specular

    float specint = glassMat.specint;
    float specshin = glassMat.specshin;  
    //if (dot(norm, lamp.position - pos) > 0.0f)
        col+= lamp.color*lamp.intensity*specint*_powf(_fmaxf(0.0f, dot(reflect(pl, norm), normalize(pos - campos))), specshin);
    #endif
    
    // Softshadow
    #ifdef shadow
    col*= shi*softshadow(pos, normalize(lamp.position - pos), shf, 100.0f) + 1.0f - shi;
    #endif
    
    return col;
}

// Shading of the objects over all lamps
__DEVICE__ float3 lampsShading(float3 norm, float3 pos, float3 ocol, int objnr, float3 campos, struct Lamp lamps[2], struct TransMat glassMat)
{
    float3 col = to_float3_s(0.0f);
    for (int l=0; l<2; l++) // lamps.length()
        col+= lampShading(lamps[l], norm, pos, ocol, objnr, l, campos, glassMat);
    
    return col;
}

// From https://www.shadertoy.com/view/lsSXzD, modified
__DEVICE__ float3 GetCameraRayDir(float2 vWindow, float3 vCameraDir, float fov)
{
  float3 vForward = normalize(vCameraDir);
  float3 vRight = normalize(cross(to_float3(0.0f, 1.0f, 0.0f), vForward));
  float3 vUp = normalize(cross(vForward, vRight));
    
  float3 vDir = normalize(vWindow.x * vRight + vWindow.y * vUp + vForward * fov);

  return vDir;
}

// Sets the position of the camera with the mouse and calculates its direction




// Tracing and rendering a ray
__DEVICE__ struct RenderData trace0(float3 tpos, float3 ray, float maxdist, bool inside, float iTime, __TEXTURE2D__ iChannel0, float3 campos, float normdelta, struct Lamp lamps[2], struct TransMat glassMat,
                             float bubbleThickness, float bubbleRadius, float bumpFactor)
{
    // Ambient light
    const float3 ambientColor = to_float3_s(0.3f);
    const float ambientint = 0.025f;
  
    float2 tr = trace(tpos, ray, maxdist, inside,iTime,bubbleThickness,bubbleRadius,bumpFactor);
    float tx = tr.x;
    int objnr = (int)(tr.y);
    float3 col;
    float3 pos = tpos + tx*ray;
    float3 norm;
    
    if (tx<maxdist*0.95f)
    {
        norm = getNormal(pos, normdelta, inside,iTime,bubbleThickness,bubbleRadius,bumpFactor);
        col = getColor(norm, pos, objnr, ray, iChannel0, glassMat);
      
        // Shading
        col = ambientColor*ambientint + lampsShading(norm, pos, col, objnr, campos, lamps, glassMat);
    }
    else
    {
        objnr = SKY_OBJ;
        col = to_float3_s(0.0f);
    }
    
    struct RenderData ret = {col, pos, norm, objnr};
    
    return ret;
}

__DEVICE__ float3 getGlassAbsColor(float dist, float3 color)
{
    return pow_f3(color, to_float3_s(0.1f + _powf(dist*8.0f, 2.0f)));
}

// Main render function with reflections and refractions
__DEVICE__ float4 render(float2 fragCoord, float iTime, float2 iResolution, __TEXTURE2D__ iChannel0, float3 campos, float3 camdir, float fov, struct Lamp lamps[2], struct TransMat glassMat,
                         float bubbleThickness, float bubbleRadius, float bumpFactor)
{   
    const float goldRef = 0.99f;

    // Tracing options
    const float normdelta = 0.001f;
    const float maxdist = 40.0f;
    const int nbref = 7;

    float2 uv = fragCoord / iResolution; 
    uv = uv*2.0f - 1.0f;
    uv.x*= iResolution.x / iResolution.y;

    float3 ray = GetCameraRayDir(uv, camdir, fov);
    struct RenderData traceinf = trace0(campos, ray, maxdist, false, iTime, iChannel0, campos, normdelta, lamps, glassMat,bubbleThickness,bubbleRadius,bumpFactor);
    float3 col = traceinf.col;
    bool inside = false;
    float cior = glassMat.ior;
    float3 glassf = to_float3_s(1.0f);
    float3 refray;

    glassf = to_float3_s(1.0f);

    for (int i=0; i<nbref; i++)
    {
        if (traceinf.objnr==BUBBLE_OBJ)
        {   
            float gv = glassf.x*goldValue(traceinf.pos);
            #ifdef reflections
            refray = reflect(ray, traceinf.norm);
            float rf = fresnel(ray, traceinf.norm, glassMat.ior); 
            float3 colGl = mix_f3(col, sky_color(refray, iChannel0), rf*glassf);
            float3 colGo = _mix(col, getGoldColor(traceinf.pos)*sky_color(refray, iChannel0), goldRef);
          
            if (!inside)
            {
              col = _mix(colGl, colGo, gv);
              glassf*= (1.0f - gv)*(1.0f- rf);
            }
            #endif
            
            cior = inside?1.0f/glassMat.ior:glassMat.ior;

            float3 ray_r = refract_f3(ray, traceinf.norm, 1.0f/cior);
            if (length(ray_r)!=0.0f)
                inside = !inside;
            else
                ray_r = reflect(ray, traceinf.norm);            

            float3 pos = traceinf.pos;

            traceinf = trace0(pos, ray_r, 20.0f, inside, iTime, iChannel0, campos, normdelta, lamps, glassMat,bubbleThickness,bubbleRadius,bumpFactor);
            if (inside)
                glassf*= getGlassAbsColor(distance_f3(pos, traceinf.pos), glassMat.col_vol);
            glassf*= glassMat.col_fil;
            
            col+= clamp(traceinf.col*glassf, 0.0f, 1.0f);

            ray = ray_r;

        }
        if (traceinf.objnr==SKY_OBJ)
        {
            col+= sky_color(ray, iChannel0)*glassf;
            break;
        }
    }
    return to_float4_aw(col, 1.0f);
}

__KERNEL__ void GlassAndBubbleJipi424Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    //CONNECT_COLOR0(Color, 0.8f, 0.2f, 1.0f, 1.0f);
    CONNECT_SLIDER0(B_Radius, 0.0f, 3.0f, 1.2f);
    CONNECT_SLIDER1(B_Thickness, -0.10f, 0.1f, 0.001f);
    CONNECT_SLIDER2(BumpFactor, -0.10f, 0.1f, 0.014f);

  
    // Campera options
    float3 campos = to_float3(0.0f, -0.0f, 10.0f);
    float3 camtarget = to_float3(0.0f, 0.0f, 0.0f);
    float3 camdir = to_float3(0.0f, 0.0f, 0.0f);
    float fov = 6.0f;
   
    // Glass perameters
    float bubbleRadius = B_Radius;//1.2f;
    float bubbleThickness = B_Thickness;//0.001f;
    float bumpFactor = BumpFactor;//0.014f;
       
   
   
    // Antialias. Change from 1 to 2 or more AT YOUR OWN RISK! It may CRASH your browser while compiling!
    const float aawidth = 0.8f;
    const int aasamples = 2;
   
    // Init
    struct Lamp lamps[2]  = {{to_float3(-5.0f, 3.0f, -5.0f), to_float3(1.0f, 1.0f, 1.0f), 1.5f, 0.01f},
                      {to_float3(1.5f, 4.0f, 2.0f), to_float3(0.7f, 0.8f, 1.0f), 1.7f, 0.01f}};

    struct TransMat glassMat = {to_float3(0.96f, 0.99f, 0.96f),
                         to_float3(0.01f, 0.02f, 0.02f),
                         to_float3_s(1.0f),
                         to_float3(0.3f, 0.5f, 0.9f),
                         0.4f,
                         45.0f,
                         1.47f};
                         
   
   
   //Camera
   const float axm = 4.0f;
   const float aym = 1.5f;
  
   float2 iMouse2;
   if (iMouse.x==0.0f && iMouse.y==0.0f)
      iMouse2 = to_float2(0.5f, 0.5f);
   else
      iMouse2 = swi2(iMouse,x,y)/iResolution;
   
   campos = to_float3(8.5f, 0.0f, 0.0f);
   swi2S(campos,x,y, rotateVec(swi2(campos,x,y), -iMouse2.y*aym + aym*0.5f));
   swi2S(campos,y,z, rotateVec(swi2(campos,y,z), -iMouse2.y*aym + aym*0.5f));
   swi2S(campos,x,z, rotateVec(swi2(campos,x,z), -iMouse2.x*axm));

   camtarget = to_float3_s(0.0f);
   camdir = camtarget - campos; 
    
    
    // Antialiasing.
    #ifdef antialias
    float4 vs = to_float4_s(0.0f);
    for (int j=0;j<aasamples ;j++)
    {
       float oy = (float)(j)*aawidth/_fmaxf((float)(aasamples-1), 1.0f);
       for (int i=0;i<aasamples ;i++)
       {
          float ox = (float)(i)*aawidth/_fmaxf((float)(aasamples-1), 1.0f);
          vs+= render(fragCoord + to_float2(ox, oy), iTime, iResolution, iChannel0, campos, camdir, fov, lamps, glassMat,bubbleThickness,bubbleRadius,bumpFactor);
       }
    }
    float2 uv = fragCoord / iResolution;
    fragColor = vs/to_float4(aasamples*aasamples);
    #else
    fragColor = render(fragCoord, iTime, iResolution, iChannel0, campos, camdir, fov, lamps, glassMat,bubbleThickness,bubbleRadius,bumpFactor);
    #endif


  SetFragmentShaderComputedColor(fragColor);
}