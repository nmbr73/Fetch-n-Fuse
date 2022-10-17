
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 1' to iChannel1
// Connect Image 'Texture: Abstract 2' to iChannel2
// Connect Image 'Texture: Font 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Created by Sebastien Durand - 2022
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// -----------------------------------------------
// The goal of this Shader was to show that we can create a very simple Cut dispersal 
// operator applicable to any scene with a simple function call (opCutDispersal). (opCutVoronoi ).
// -----------------------------------------------
// Other cutting space: 
//   [CrashTest]                       https://www.shadertoy.com/view/wsSGDD
//   [Voronoi broke the scene]         https://www.shadertoy.com/view/7tBBDw
//   [Cubic Dispersal broke the scene] https://www.shadertoy.com/view/7t2fRK
// -----------------------------------------------
// inspired by  Tater [Cubic Dispersal] https://www.shadertoy.com/view/fldXWS


#define WITH_EDGE
//#define WITH_MIN_BLOCK_SIZE // introduce imprecisions



__DEVICE__ mat2 rot(float a) {
    return to_mat2(_cosf(a), _sinf(a), -_sinf(a), _cosf(a));;
}

__DEVICE__ float3 hash33(float3 p) {   
  p = to_float3(dot(p,to_float3(127.1f,311.7f, 74.7f)),
                dot(p,to_float3(269.5f,183.3f,246.1f)),
                dot(p,to_float3(113.5f,271.9f,124.6f)));
  return fract_f3(sin_f3(p)*43758.5453123f);
}

__DEVICE__ float sdBox( float3 p, float3 b ) {
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(_fmaxf(q.x,_fmaxf(q.y,q.z)),0.0f);
}

// --------------------------------------
// Space Operators
// --------------------------------------

// [iq] https://www.shadertoy.com/view/4lyfzw
__DEVICE__ float opExtrussion(float3 p, float sdf, float h) {
    float2 w = to_float2(sdf, _fabs(p.z) - h);
    return _fminf(_fmaxf(w.x,w.y),0.0f) + length(_fmaxf(w,to_float2_s(0.0f)));
}

// [iapafoto] https://www.shadertoy.com/view/7t2fRK
__DEVICE__ float opCutDispersal(inout float3 *uv, float3 kdiv) {


    float3 _uv = *uv;
    swi2S(_uv,x,z, mul_f2_mat2(swi2(_uv,x,z) , rot(0.2f)));
    swi2S(_uv,x,y, mul_f2_mat2(swi2(_uv,x,y) , rot(0.3f)));
    *uv = _uv;
    
#ifdef WITH_MIN_BLOCK_SIZE
    float ITERS = 4.0f;
#else
    float ITERS = 3.0f;
#endif    
    float3 l0 = 2.0f*1.9f*to_float3(4.8f,1.4f,1.2f);
    float3 dMin = -l0*0.5f - kdiv*_powf(2.0f,ITERS-1.0f);
    float3 dMax = l0*0.5f + kdiv*_powf(2.0f,ITERS-1.0f);
    
    float MIN_SIZE = 0.105f;
    float3 diff2 = to_float3_s(1); 
    float3 posTxt = *uv;
    float3 div0;

    float i = 0.0f;
    
    for(; i<ITERS;i++){
        // divide the box into quads
        div0 = to_float3_s(0.1f) + 0.8f*hash33(diff2);  // division sans interval
        
        // here is the magic!
        // conversion of the ratio to keep constant size 
        float3 dd = kdiv*_powf(2.0f,ITERS-1.0f-i),
               a0 = div0*l0,
               a2 = a0 + dd,
               l2 = l0 + 2.0f*dd,
               div2 = a2/l2; // ratio de division en tenant compte des bodures
     
        // On determine la division
        float3 divide = mix_f3(dMin, dMax, div2);
        
#ifdef WITH_MIN_BLOCK_SIZE
        //Find the minimum dimension size
        float3 minAxis = _fminf(abs_f3(a0), abs_f3(l0-a0));
        float minSize = _fminf(minAxis.x, _fminf( minAxis.y, minAxis.z));
        
        // if minimum dimension is too small break out
        // => this introduce imprecision in distance field
        bool smallEnough = minSize < MIN_SIZE;
        if (smallEnough && i + 1.0f > 1.0f) { break; }
#endif

        l0 = mix_f3(l0-a0, a0, step(*uv, divide)); // ne prendre que la partie du bon coté
        
        // update the box domain
        dMax = mix_f3( dMax, divide, step(*uv, divide ));
        dMin = mix_f3( divide, dMin, step(*uv, divide ));

        //Deterministic seeding for future divisions 
        diff2 = step(*uv, divide) - 10.0f*hash33(diff2);
        posTxt -= dd*(0.5f - step(*uv, divide));
    }
     
    //Calculate 2d box sdf
    float3 center = (dMin + dMax)/2.0f;
    float3 dd0 = 0.5f*kdiv*_powf(2.0f, ITERS-(i-1.0f));
    float d = sdBox(*uv-center, 0.5f*(dMax - dMin) - 0.5f*dd0);
    *uv = posTxt;
    _uv = *uv;
    swi2S(_uv,x,y, mul_f2_mat2(swi2(_uv,x,y) , rot(-0.3f)));
    swi2S(_uv,x,z, mul_f2_mat2(swi2(_uv,x,z) , rot(-0.2f)));
    *uv=_uv;
    return d;
}


// --------------------------------------
// Distance Functions
// --------------------------------------

// Adapted from [FabriceNeyret2] https://www.shadertoy.com/view/llyXRW
__DEVICE__ float sdFont(float2 p, int c, __TEXTURE2D__ iChannel0) {
    float2 uv = (p + to_float2((float)(c%16), (float)(15-c/16)) + 0.5f)/16.0f;
    return _fmaxf(_fmaxf(_fabs(p.x) - 0.25f, _fmaxf(p.y - 0.35f, -0.38f - p.y)), texture(iChannel0, uv).w - 127.0f/255.0f);
}

__DEVICE__ float sdMessage2D(float2 p, int txt[], float scale, __TEXTURE2D__ iChannel0) { 
    p.y += 0.1f;
    p /= scale;
    float d = 999.0f, w = 0.45f; // letter width  
    
    int cnt = sizeof(txt)/2+1;
    
    cnt = 6; // Irgendwie funktioniert sizeof nicht :-(
        
    p.x += w*(float)(cnt-1)*0.5f; // center text arround 0
    
    
    for (int id = 0; id<cnt; id++){
      d = _fminf(d, sdFont(p, txt[id],iChannel0));   
      p.x -= w; 
    }
    return scale*d;
}

__DEVICE__ float sdMessage3D(in float3 p, int txt[], float scale, float h, __TEXTURE2D__ iChannel0) { 
    return opExtrussion(p, sdMessage2D(swi2(p,x,y), txt, scale,iChannel0), h);
}

// --------------------------------------
// Distance to scene
// --------------------------------------
__DEVICE__ float map(float3 p, float tOpen, int gtxt[], __TEXTURE2D__ iChannel0) {
    float dcut = opCutDispersal(&p, 0.7f*to_float3(0.8f,0.4f,0.8f)*tOpen), //opSuperCut(p),
          dScn = sdMessage3D(p, gtxt,4.0f,1.0f,iChannel0);
    return _fmaxf(dScn, dcut);
}

// --------------------------------------
// Shading Tools
// --------------------------------------
// Find initial space position
__DEVICE__ float4 MCol(float3 p, float tOpen, int gtxt[], __TEXTURE2D__ iChannel0) {
    float dcut = opCutDispersal(&p, 0.7f*to_float3(0.8f,0.4f,0.8f)*tOpen),
          dScn = sdMessage3D(p, gtxt,4.0f,1.0f,iChannel0);
    return to_float4_aw(p, dScn >= dcut ? 1.0f : 2.0f);
}


union A2F
 {
   float3  F; //32bit float
   float   A[3];  //32bit unsigend integer
 };

// Shane - normal + edge
__DEVICE__ float3 normal(float3 p, float3 rd, inout float *edge, float t, float tOpen, int gtxt[], float2 iResolution, int iFrame, __TEXTURE2D__ iChannel0) { 
    float eps = 4.5f/_mix(450.0f, _fminf(850.0f, iResolution.y), 0.35f),
          d = map(p,tOpen,gtxt,iChannel0);
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
    float3 n = to_float3_s(0);
    for( int i=_fminf(iFrame, 0); i<4; i++) {
        float3 e = 0.57735f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1)) - 1.0f);
        n += e*map(p + 0.001f*e,tOpen,gtxt,iChannel0);
    }
    return normalize(n - _fmaxf(0.0f, dot(n,rd))*rd);
}

// Box:  https://www.shadertoy.com/view/ld23DV
__DEVICE__ bool iBox( float3 ro, float3 rd, float3 sz, inout float *tN, inout float *tF) {
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
__DEVICE__ float3 render(float3 ro, float3 rd, float res, float3 pos, float3 n, float3 cobj, float3 light, float3 cback, float spec) {

    float 
         amb = clamp(0.5f+0.5f*n.y, 0.0f, 1.0f),
         dif = clamp(dot( n, light ), 0.0f, 1.0f),
         pp = clamp(dot(reflect(-light,n), -rd),0.0f,1.0f),
         fre = (0.7f+0.3f*dif)*_powf( clamp(1.0f+dot(n,rd),0.0f,1.0f), 2.0f);
    float3 brdf = 0.5f*(amb)+ 1.0f*dif*to_float3(1.0f,0.9f,0.7f),
         sp = 3.0f*_powf(pp,spec)*to_float3(1, 0.6f, 0.2f),
         col = cobj*(brdf + sp) + fre*(0.5f*cobj+0.5f);
    return _mix(col, to_float3(0.02f,0.2f,0.2f),smoothstep(6.0f,20.0f,res));
}

__DEVICE__ mat3 setCamera(float3 ro, float3 ta, float r ) {
  float3 w = normalize(ta-ro),
         p = to_float3(_sinf(r), _cosf(r),0.0f),
         u = normalize( cross(w,p) ),
         v =          ( cross(u,w) );
    return to_mat3_f3( u, v, w );
}


// --------------------------------------
// Main
// --------------------------------------
__KERNEL__ void CubicDispersalBrokeTheSceneFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_COLOR0(Color, 0.8f, 0.4f, 0.0f, 1.0f); 
    CONNECT_SLIDER0(BoxX, 0.0f, 10.0f, 4.8f);
    CONNECT_SLIDER1(ViewZ, 0.0f, 10.0f, 2.5f);
  
    CONNECT_POINT0(LightPosXY, 0.0f, 0.0f);
    CONNECT_POINT1(Irgendwas, 0.0f, 0.0f);
    CONNECT_SLIDER2(LightPosZ, -10.0f, 10.0f, 0.0f);
    //Lightpos
  
    fragCoord+=0.5f;

    // SCENE
    //int gtxt[] = {83,67,69,78,69,83}; //SCENES
    int gtxt[] = {110,109,98,114,55,51}; //nmbr73 BoxX 6.14
    

    float2 r = iResolution, 
           m = swi2(iMouse,x,y) / r,
           q = fragCoord/swi2(r,x,y);
 
    float tOpen = 0.4f*smoothstep(0.6f,0.0f,_cosf(0.3f*iTime));

    float a = _mix(0.3f,3.0f*_cosf(0.4f*3.0f*iTime),0.5f+0.5f*_cosf(0.2f*iTime))+3.14f*m.x;
    
    // camera  
    float3 ta = to_float3_s(0),
         ro = ta + 2.4f*to_float3(4.5f*_cosf(a), 3.0f*_cosf(0.4f*iTime) + 4.0f*m.y, 4.5f*_sinf(a));
    mat3 ca = setCamera( ro, ta, 0.1f*_cosf(0.123f*iTime) );
  
    // ray direction
    float3 rd = mul_mat3_f3(ca , normalize( to_float3_aw((2.0f*fragCoord-swi2(r,x,y))/r.y, ViewZ)));

    float h = 0.1f, t, tN = 0.0f, tF = 20.0f;
    
    // Background color
    float3 c = 0.09f*(hash33(swi3(q,x,y,x)).x + to_float3_s(1.0f));

    if (iBox(ro, rd, to_float3(BoxX,1.4f,1.2f)*(1.0f+to_float3(1.0f,2.0f,3.0f)*tOpen), &tN, &tF)) {    
        t = tN;// - 0.02f*hash33(swi3(q,x,y,x)).x;
  // Ray marching
        for(int i=_fminf(0,iFrame);i<200;i++) { 
            if (h<1e-3 || t>tF) break;
            t += h = map(ro + rd*t, tOpen,gtxt, iChannel0);
        }
    
        // light pos
        float3 lp =  ro + 3.0f*to_float3(0.25f, 2, -0.1f);
        
        //lp += to_float3(LightPosXY.x,LightPosXY.y,LightPosZ);

        // Calculate color on point
        if (t<tF) {
            float3 pos = ro + t * rd;
            float edge = 0.0f;
            float4 txt = MCol(pos, tOpen,gtxt, iChannel0);   
            float3 n = normal(pos, rd, &edge, t,tOpen,gtxt,iResolution,iFrame,iChannel0);     
            //float3 cobj = txt.w<1.5f ? to_float3_s(0.7f) : 1.5f*to_float3(0.8f,0.4f,0.0f);
            float3 cobj = txt.w<1.5f ? to_float3_s(0.7f) : 1.5f*swi3(Color,x,y,z);
            
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
        } 
    } //else{
    //c *= 2.0f; 
    //}
    
    
    // post prod
    c = pow_f3(c, to_float3_s(0.75f));
    c = (c * _powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),0.7f));
    fragColor = to_float4_aw(c, t);  

fragColor=to_float4_f2f2(Irgendwas,Irgendwas);

  SetFragmentShaderComputedColor(fragColor);
}