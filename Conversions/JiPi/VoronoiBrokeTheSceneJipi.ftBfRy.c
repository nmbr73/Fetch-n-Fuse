
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 1' to iChannel1
// Connect Image 'Texture: Abstract 2' to iChannel2
// Connect Image 'Texture: Font 1' to iChannel0

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



// Created by Sebastien Durand - 2022
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// -----------------------------------------------
// The goal of this Shader was to show that we can create a very simple voronoi cutting operator
// applicable to any scene with a simple function call (opCutVoronoi ).
// -----------------------------------------------
// Other cutting space : [CrashTest] https://www.shadertoy.com/view/wsSGDD
// -----------------------------------------------


#define WITH_EDGE

// SPACE txt
//int[] gtxt = int[] (83,80,65,67,69);
// SCENE
//int gtxt[] = {74,73,80,73,32};

// [iq] https://www.shadertoy.com/view/XlXcW4

__DEVICE__ float3 hash33(float3 p ) {
    const uint k = 1103515245U;

    uint3 _x = make_uint3(p.x+10.0f,p.y+10.0f,p.z+10.0f);
    //_x = ((_x>>8U)^swi3(x,y,z,x))*k;
    //_x = ((_x>>8U)^swi3(x,y,z,x))*k;
    //_x = ((_x>>8U)^swi3(x,y,z,x))*k;
    _x = (make_uint3((_x.x>>8U)^_x.y,(_x.y>>8U)^_x.z,(_x.z>>8U)^_x.x))*k;
    _x = (make_uint3((_x.x>>8U)^_x.y,(_x.y>>8U)^_x.z,(_x.z>>8U)^_x.x))*k;
    _x = (make_uint3((_x.x>>8U)^_x.y,(_x.y>>8U)^_x.z,(_x.z>>8U)^_x.x))*k;
    
    float3 o= make_float3(_x)*(1.0f/(float)(0xffffffffU));
    return 0.3f*cos_f3(0.2f*(3.0f+o)*6.2f + 2.2f+ o*6.2831853f);
}

__DEVICE__ float hash13(const in float3 p ) {
    float h = dot(p,to_float3(127.1f,311.7f,758.5453123f));  
    return fract(_sinf(h)*43758.5453123f);
}

//---------------------------------------------------------------
// Here is the distance to voronoi3D cell (not exact: over estimate distance on edges)
//---------------------------------------------------------------
__DEVICE__ float sdVoronoi( in float3 x, in float3 cellId) {
    float md = 64.0f;    
    float3 mr = hash33(cellId);
    for( int k=-1; k<=1; k++ )
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ ) {
        if (i==0&&j==0&&k==0) continue;  // skip main cell 
        float3 g = to_float3(i,j,k),     // relative cell Id
             r = g + hash33(cellId + g); // pos of other point
        md = _fminf(md, dot(0.5f*(mr+r)-x, normalize(r-mr))); // distance
    }
    return -1.2f*md;
}

// --------------------------------------
// Space Operators
// --------------------------------------

// [iq] https://www.shadertoy.com/view/4lyfzw
__DEVICE__ float opExtrussion( in float3 p, in float sdf, in float h) {
    float2 w = to_float2(sdf, _fabs(p.z) - h);
    return _fminf(_fmaxf(w.x,w.y),0.0f) + length(_fmaxf(w,to_float2_s(0.0f)));
}

// [iapafoto] https://www.shadertoy.com/view/7tBBDw
__DEVICE__ float opCutVoronoi(inout float3 *p, float k) {
    k += 1.0f;
    float d = 999.0f; //-dout;
    float dm;
    float3 posTxt = *p;

    for( int z=-1; z<=1; z++)
    for( int j=-1; j<=1; j++)
    for( int i=-1; i<=1; i++) {
         float3 g = to_float3(i,j,z)+ /*to_float3_s(1.0f); // +*/ _floor(*p/k);
         if (length(k*g-*p)<1.5f) { // do it only on neighbourhood
             float v = sdVoronoi(*p-k*g, g);
             if (d>v) {
                 posTxt = *p-k*g+g;
             }
             d  = _fminf(d,v);
         }
    }
    *p = posTxt;

    return d;
}

__DEVICE__ float opSuperCut(inout float3 *p, float tOpen) {
    if (tOpen > 0.005f) { // && p.y < 2.0f && p.y > -2.0f && p.z < 2.0f) {
        *p *= 2.0f;
        float d = -0.5f*opCutVoronoi(p, tOpen);
        *p/=2.0f;
        return d;
    } 
    return 999.0f;
}

// --------------------------------------
// Distance Functions
// --------------------------------------

// Adapted from [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
__DEVICE__ float sdFont(in float2 p, in int c, __TEXTURE2D__ iChannel0) {
    float2 uv = (p + to_float2((float)(c%16), (float)(15-c/16)) + 0.5f)/16.0f;
    return _fmaxf(_fmaxf(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel0, uv).w - 127.0f/255.0f);
}

__DEVICE__ float sdMessage2D(in float2 p, in int txt[], in float scale, __TEXTURE2D__ iChannel0) { 
    
    p.y += 0.1f;
    p /= scale;
    float d = 999.0f, w = 0.45f; // letter width  
    
    int cnt = sizeof(txt)/2+1;
    
    cnt = 7;
    
    //p.x += w*(float)(txt.length()-1)*0.5f; // center text arround 0
    //p.x += w*(float)(sizeof(txt)/2-1)*0.5f; // center text arround 0 // sizeof(txt) = 8: warum auch immer
    p.x += w*(float)(cnt-1)*0.5f; // center text arround 0   // 4 sieht schon gut aus 
    
    
    for (int id = 0; id<cnt; id++){
      d = _fminf(d, sdFont(p, txt[id],iChannel0));   
      p.x -= w; 
    }
    return scale*d;//*1.25f; interessant
}

__DEVICE__ float sdMessage3D(in float3 p, in int txt[], in float scale, in float h, __TEXTURE2D__ iChannel0) { 
    return opExtrussion(p, sdMessage2D(swi2(p,x,y), txt, scale,iChannel0), h);
}

// --------------------------------------
// Distance to scene
// --------------------------------------
__DEVICE__ float map(in float3 p, float tOpen, int gtxt[], __TEXTURE2D__ iChannel0) {
    float dcut = opSuperCut(&p,tOpen),
          dScene = sdMessage3D(p, gtxt,2.0f,0.5f,iChannel0);
    return _fmaxf(dScene, -dcut);
}

// --------------------------------------
// Shading Tools
// --------------------------------------
// Find initial space position
__DEVICE__ float4 MCol(in float3 p, float tOpen, int gtxt[], __TEXTURE2D__ iChannel0) {
    float dcut = opSuperCut(&p,tOpen),
          dScene = sdMessage3D(p, gtxt,2.0f,0.5f,iChannel0);
    return to_float4_aw(p, dScene >= -dcut ? 1.0f : 2.0f);
}

union A2F
 {
   float3  F; //32bit float
   float   A[3];  //32bit unsigend integer
 };

// Shane - normal + edge
__DEVICE__ float3 calcNormal(float3 p, float3 rd,  inout float *edge, inout float *crv, float t, float tOpen, int gtxt[], __TEXTURE2D__ iChannel0, int iFrame, float2 R) { 
    float eps = 4.5f/_mix(450.0f, _fminf(850.0f, iResolution.y), 0.35f); // Auflösung ???????
    float d = map(p,tOpen,gtxt,iChannel0);
#ifdef WITH_EDGE
    float3 e = to_float3(eps, 0, 0);
    A2F da;
    da.F = to_float3_s(-2.0f*d);
 
    for(int i = _fminf(iFrame,0); i<3; i++) {
        for( int j=_fminf(iFrame,0); j<2; j++ )
            da.A[i] += map(p + e*(float)(1-2*j),tOpen,gtxt,iChannel0);
        e = swi3(e,z,x,y);
    }
    da.F = abs_f3(da.F);
    *edge = da.F.x + da.F.y + da.F.z;
    *edge = smoothstep(0.0f, 1.0f, _sqrtf(*edge/e.x*2.0f));
#endif
    float3 n = to_float3_s(0.0f);
    for( int i=_fminf(iFrame, 0); i<4; i++) {
        float3 e = 0.57735f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1)) - 1.0f);
        n += e*map(p + 0.001f*e,tOpen,gtxt,iChannel0);
    }
    return normalize(n - _fmaxf(0.0f, dot(n,rd))*rd);
}

// Box:  https://www.shadertoy.com/view/ld23DV
__DEVICE__ bool iBox( in float3 ro, in float3 rd, in float3 sz, inout float *tN, inout float *tF) {
    float3 m = sign_f3(rd)/_fmaxf(abs_f3(rd), to_float3_s(1e-8)),
         n = m*ro,
         k = abs_f3(m)*sz,
         t1 = -n - k,
         t2 = -n + k;
  *tN = _fmaxf( _fmaxf( t1.x, t1.y ), t1.z );
  *tF = _fminf( _fminf( t2.x, t2.y ), t2.z );
    return !(*tN > *tF || *tF <= 0.0f);
}

//----------------------------------
// Texture 3D (Shane)
//----------------------------------

// Tri-Planar blending function. Based on an old Nvidia writeup:
// GPU Gems 3 - Ryan Geiss: http://http.developer.nvidia.com/GPUGems3/gpugems3_ch01.html
__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, in float3 p, in float3 n ){  
    n = _fmaxf(n*n, to_float3_s(0.001f));
    n /= n.x + n.y + n.z;  
  return swi3((texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z),x,y,z);
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
__DEVICE__ float3 doBumpMap( __TEXTURE2D__ tx, in float3 p, in float3 n, float bf){   
    const float2 e = to_float2(0.001f, 0);
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = to_mat3_f3( tex3D(tx, p - swi3(e,x,y,y), n), tex3D(tx, p - swi3(e,y,x,y), n), tex3D(tx, p - swi3(e,y,y,x), n));
    float3 g = mul_f3_mat3(to_float3(0.299f, 0.587f, 0.114f) , m); // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), to_float3(0.299f, 0.587f, 0.114f)) )/e.x; 
    g -= n*dot(n, g);
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
}


//----------------------------------
// Shading
//----------------------------------
__DEVICE__ float3 render(in float3 ro, in float3 rd, in float res, in float3 pos, in float3 n, in float3 cobj, in float3 light, float3 cback, float spec) {
    float 
         amb = clamp(0.5f+0.5f*n.y, 0.0f, 1.0f),
         dif = clamp(dot( n, light ), 0.0f, 1.0f),
         pp = clamp(dot(reflect(-light,n), -rd),0.0f,1.0f),
         fre = (0.7f+0.3f*dif)*_powf( clamp(1.0f+dot(n,rd),0.0f,1.0f), 2.0f);
         float3 brdf = 0.5f*(amb)+ 1.0f*dif*to_float3(1.0f,0.9f,0.7f),
         sp = 3.0f*_powf(pp,spec)*to_float3(1.0f, 0.6f, 0.2f),
         col = cobj*(brdf + sp) + fre*(0.5f*cobj+0.5f);
    return _mix(col, to_float3(0.02f,0.2f,0.2f),smoothstep(3.0f,10.0f,res));
}

__DEVICE__ mat3 setCamera( in float3 ro, in float3 ta, float cr ) {
  float3 cw = normalize(ta-ro),
         cp = to_float3(_sinf(cr), _cosf(cr),0.0f),
         cu = normalize( cross(cw,cp) ),
         cv =          ( cross(cu,cw) );
    return to_mat3_f3( cu, cv, cw );
}


// --------------------------------------
// Main
// --------------------------------------
__KERNEL__ void VoronoiBrokeTheSceneJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(BoxX, 0.0f, 10.0f, 2.3f);
    CONNECT_SLIDER1(ViewZ, 0.0f, 10.0f, 2.5f);
  
    fragCoord+=0.5f;

    int gtxt[] = {65,66,67,68,69,70,71};

    float2 r = iResolution, 
           m = swi2(iMouse,x,y) / r,
           q = fragCoord/swi2(r,x,y);
 
    float tOpen = 0.5f*smoothstep(0.6f,0.0f,_cosf(0.3f*iTime));

    float a = _mix(0.3f,3.0f*_cosf(0.4f*3.0f*iTime),0.5f+0.5f*_cosf(0.2f*iTime))+3.14f*m.x;
    
    // camera  
    float3 ta = to_float3_s(0),
         ro = ta + 1.2f*to_float3(4.5f*_cosf(a), 3.0f*_cosf(0.4f*iTime) + 4.0f*m.y, 4.5f*_sinf(a));
    mat3 ca = setCamera( ro, ta, 0.1f*_cosf(0.123f*iTime) );
 
    float2 p = (2.0f*fragCoord-swi2(r,x,y))/r.y;        
    // ray direction
    //float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(p,2.5f) ));
    float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw(p,ViewZ) ));

    float h = 0.1f, t, tN = 0.0f, tF = 10.0f;
    // Background color
    float3 c = to_float3_s(0.11f);

  
    //if (iBox(ro, rd, to_float3(2.3f,0.7f,0.6f)*(1.0f+tOpen), &tN, &tF)) {  //Boxgröße !!!  
    if (iBox(ro, rd, to_float3(BoxX,0.7f,0.6f)*(1.0f+tOpen), &tN, &tF)) {    
        t = tN + 0.05f*hash13(swi3(q,x,y,x));;
  // Ray marching
        for(int i=_fminf(0,iFrame);i<100;i++) { 
            if (h<1e-3 || t>tF) break;
            t += h = map(ro + rd*t,tOpen,gtxt,iChannel0);
        }
    }
  
    float3 lp =  ro + 3.0f*to_float3(0.25f, 2, -0.1f);
            
    // Calculate color on point
  if (h < 1e-2) {
    float3 pos = ro + t * rd;
        float edge = 0.0f, crv = 1.0f;
        float4 txt = MCol(pos,tOpen,gtxt,iChannel0);   


        float3 n = calcNormal(pos, rd, &edge, &crv, t,tOpen,gtxt,iChannel0,iFrame,R),     
             cobj = txt.w<1.5f ? to_float3_s(0.7f) : 1.5f*to_float3(0.8f,0.4f,0.0f);
        if (txt.w<1.5f) {
            n = doBumpMap(iChannel1, swi3(txt,x,y,z)*2.0f, n, 0.01f);
        } else {
            n = doBumpMap(iChannel2, swi3(txt,x,y,z)*2.0f, n, 0.02f);
        }
        // keep in visible side
        n = normalize(n - _fmaxf(0.0f,dot(n,rd))*rd);
        // Shading
        c = render(ro, rd, t, pos, n, cobj, normalize(lp-pos), c, txt.w<1.5f ? 99.0f : 16.0f);
#ifdef WITH_EDGE
        c *= 1.0f - edge*0.8f;
#endif
  } else {
        c *= 0.5f+0.5f*hash13(swi3(q,x,y,x));   
    }
    
    // post prod
    c = pow_f3(c, to_float3_s(0.75f));
    c = (c * _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.7f));
    fragColor = to_float4_aw(clamp(c, to_float3_s(0), to_float3_s(1)), t);  

//float2 uv = fragCoord/R;
//if (uv.x<=0.1f && uv.y<=0.10f) fragColor = to_float4_s(sizeof(gtxt)/2); //sizeof(txt)/2+1

  SetFragmentShaderComputedColor(fragColor);
}