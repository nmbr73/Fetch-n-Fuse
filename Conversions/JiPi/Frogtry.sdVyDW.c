
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


//Buffer A : slider management (this is not required)
// Bers : https://www.shadertoy.com/view/MscXzn

#define __saturatef(x) clamp(x,0.0f,1.0f)



__DEVICE__ void SLIDER_setValue(float idx, float val, inout float4 *sliderVal)
{
  if(idx<0.0f) return;
  else if(idx<0.25f) (*sliderVal).x = __saturatef(val);
  else if(idx<0.50f) (*sliderVal).y = __saturatef(val);
  else if(idx<0.75f) (*sliderVal).z = __saturatef(val);
  else if(idx<1.00f) (*sliderVal).w = __saturatef(val);
}

__DEVICE__ float SLIDER_getValue(float idx, inout float4 *sliderVal)
{
    if     (idx<0.25f) return (*sliderVal).x;
    else if(idx<0.50f) return (*sliderVal).y;
    else if(idx<0.75f) return (*sliderVal).z;
    else if(idx<1.00f) return (*sliderVal).w;
  else return 0.0f;
}

__DEVICE__ void SLIDER_init(float2 mousePos, float2 cMin, float2 cMax, inout float4 *sliderVal, __TEXTURE2D__ iChannel0 )
{
    float4 cPingPong = texture(iChannel0,to_float2_s(0));
    if(length(cPingPong)>0.001f)
        *sliderVal = cPingPong;
        
    float width = cMax.x-cMin.x;
    float height = cMax.y-cMin.y;
    if(mousePos.x>cMin.x && mousePos.x<cMax.x &&
       mousePos.y>cMin.y && mousePos.y<cMax.y )
    {
        float t = (mousePos.y-cMin.y)/height;
        t = clamp(t/0.75f-0.125f,0.0f,1.0f); //25% top/bottom margins
        SLIDER_setValue((mousePos.x-cMin.x)/width, t, sliderVal);
    }
}

//Returns the distance from point "p" to a given line segment defined by 2 points [a,b]
__DEVICE__ float UTIL_distanceToLineSeg(float2 p, float2 a, float2 b)
{
    //       p
    //      /
    //     /
    //    a--e-------b
    float2 ap = p-a;
    float2 ab = b-a;
    //Scalar projection of ap in the ab direction = dot(ap,ab)/|ab| : Amount of ap aligned towards ab
    //Divided by |ab| again, it becomes normalized along ab length : dot(ap,ab)/(|ab||ab|) = dot(ap,ab)/dot(ab,ab)
    //The clamp provides the line seg limits. e is therefore the "capped orthogogal projection", and length(p-e) is dist.
    float2 e = a+clamp(dot(ap,ab)/dot(ab,ab),0.0f,1.0f)*ab;
    return length(p-e);
}

//uv = slider pixel in local space [0-1], t = slider value [0-1], ar = aspect ratio (w/h)
__DEVICE__ float4 SLIDER_drawSingle(float2 uv, float t, float2 ar, bool bHighlighted, float2 iResolution)
{
  float zzzzzzzzzzzzzzzzz;
    const float3  ITEM_COLOR = to_float3_s(1);
    const float3  HIGHLIGHT_COLOR = to_float3(0.2f,0.7f,0.8f);
    const float RAD = 0.05f;  //Cursor radius, in local space
    const float LW  = 0.030f; //Line width
    float aa  = 14.0f/iResolution.x; //antialiasing width (smooth transition)
    float3 selectionColor = bHighlighted?HIGHLIGHT_COLOR:ITEM_COLOR;
    float3 cheapGloss   = 0.8f*selectionColor+0.2f*smoothstep(-aa,aa,uv.y-t-0.01f+0.01f*_sinf(uv.x*12.0f));
    float2 bottomCenter = to_float2(0.5f,0.0f);
    float2 topCenter    = to_float2(0.5f,1.0f);
    float2 cursorPos    = to_float2(0.5f,t);
    float distBar = UTIL_distanceToLineSeg(uv*ar, bottomCenter*ar, topCenter*ar);
    float distCur = length((uv-cursorPos)*ar)-RAD;
    float alphaBar = 1.0f-smoothstep(2.0f*LW-aa,2.0f*LW+aa, distBar);
    float alphaCur = 1.0f-smoothstep(2.0f*LW-aa,2.0f*LW+aa, distCur);
    float4  colorBar = to_float4_aw(_mix(   to_float3_s(1),to_float3_s(0),smoothstep(LW-aa,LW+aa, distBar)),alphaBar);
    float4  colorCur = to_float4_aw(_mix(cheapGloss,to_float3_s(0),smoothstep(LW-aa,LW+aa, distCur)),alphaCur);
    return _mix(colorBar,colorCur,colorCur.w);
}

#define withinUnitRect(a) (a.x>=0.0f && a.x<=1.0f && a.y>=0.0f && a.y<=1.0f)
__DEVICE__ float4 SLIDER_drawAll(float2 uv, float2 cMin, float2 cMax, float2 muv, float2 iResolution, inout float4 *sliderVal)
{
    float width = cMax.x-cMin.x;
    float height = cMax.y-cMin.y;
    float2 ar = to_float2(0.30f,1.0f);
    uv  = (uv -cMin)/to_float2(width,height); //pixel Normalization
    muv = (muv-cMin)/to_float2(width,height); //mouse Normalization
    if( withinUnitRect(uv))
    {
      float t = SLIDER_getValue(uv.x, sliderVal);
      bool bHighlight = withinUnitRect(muv) && _fabs(_floor(uv.x*4.0f)-_floor(muv.x*4.0f))<0.01f;
      uv.x = fract(uv.x*4.0f); //repeat 4x
      uv.y = uv.y/0.75f-0.125f; //25% margins
      return SLIDER_drawSingle(to_float2(uv.x*2.0f-0.5f, uv.y),t,ar,bHighlight, iResolution);
    }
    return to_float4_s(0);
}

__KERNEL__ void FrogtryFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    float4 sliderVal = to_float4(0.25f,0.22f,0,0.31f); //Default slider values [0-1]
float AAAAAAAAAAAAAA;
    float2 cMinSliders = to_float2(0.8f,0.80f);
    float2 cMaxSliders = to_float2(1.0f,1.0f);
    float2 uvSliders = fragCoord / iResolution;
    float2 mousePos = swi2(iMouse,x,y) / iResolution;
    SLIDER_init(mousePos, cMinSliders, cMaxSliders, &sliderVal, iChannel0);
    float4 cSlider = SLIDER_drawAll(uvSliders,cMinSliders, cMaxSliders, mousePos, iResolution, &sliderVal);
    
    if(length(fragCoord-to_float2(0,0))<1.0f) 
        fragColor = sliderVal;
    else fragColor = cSlider;


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Gray Noise Small' to iChannel0
// Connect Image 'Previsualization: Buffer A' to iChannel2


// Created by sebastien durand - 08/2016
//-------------------------------------------------------------------------------------
// Based on "Dusty nebula 4" by Duke (https://www.shadertoy.com/view/MsVXWW) 
// Sliders from IcePrimitives by Bers (https://www.shadertoy.com/view/MscXzn)
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License
//-------------------------------------------------------------------------------------


#define R(p, a) p = _cosf(a)*p+_sinf(a)*to_float2((p).y, -(p).x)
#define pi 3.14159265f




//const float time = 10.0f;


__DEVICE__ float2 min2(float2 a, float2 b) {
    return a.x<b.x ? a  : b;
} 

__DEVICE__ float hash( const in float3 p ) {
  float h = dot(p,to_float3(127.1f,311.7f,758.5453123f));  
  return fract(_sinf(h)*43758.5453123f);
}

// [iq] https://www.shadertoy.com/view/4sfGzS
__DEVICE__ float noiseText(in float3 x, __TEXTURE2D__ iChannel0) {
  float3 p = _floor(x), f = fract_f3(x);
  f = f*f*(3.0f-f-f);
  float2 uv = (swi2(p,x,y)+to_float2(37.0f,17.0f)*p.z) + swi2(f,x,y),
         rg = swi2(texture( iChannel0, (uv+0.5f)/256.0f),y,x);
  return _mix(rg.x, rg.y, f.z);
}

// ratio: ratio of hight/low frequencies
__DEVICE__ float fbmdust(in float3 p, in float ratio, __TEXTURE2D__ iChannel0) {
    return _mix(noiseText(p*3.0f,iChannel0), noiseText(p*20.0f,iChannel0), ratio);
}

__DEVICE__ float2 spiralArm(in float3 p, in float thickness, in float blurAmout, in float blurStyle, __TEXTURE2D__ iChannel0) {
    float dephase = 2.2f, loop = 4.0f;
    float a = _atan2f(p.x,p.z),  // angle     
      r  = length(swi2(p,x,z)), lr = _logf(r), // distance to center
      th = (0.1f-0.25f*r), // thickness according to distance
      d  = fract(0.5f*(a-lr*loop)/pi); //apply rotation and scaling.
    d = (0.5f/dephase - _fabs(d-0.5f))*2.0f*pi*r;
    d *= (1.0f-lr)/thickness;  // space fct of distance
float uuuuuuuuuuuuuuuuu;  
    // Perturb distance field
    float radialBlur = blurAmout*fbmdust(to_float3(r*4.0f,10.0f*d,10.0f-5.0f*p.y),blurStyle, iChannel0);
    return to_float2_s(_sqrtf(d*d+10.0f*p.y*p.y/thickness)-th*r*0.2f-radialBlur);
}

__DEVICE__ float2 dfGalaxy(in float3 p, in float thickness, in float blurAmout, in float blurStyle, __TEXTURE2D__ iChannel0) {
  return min2(spiralArm(p,                       thickness, blurAmout, blurStyle,iChannel0),
              spiralArm(to_float3(p.z,p.y,-p.x), thickness, blurAmout, blurStyle,iChannel0));  
}

__DEVICE__ float2 map(in float3 p, float4 sliderVal, float iTime, float4 iMouse, __TEXTURE2D__ iChannel0) {
  //#define R(p, a) p = _cosf(a)*p+_sinf(a)*to_float2((p).y, -(p).x)
  //R(swi2(p,x,z), iMouse.x*0.008f*pi+iTime*0.3f); // Org
  
  float a = iMouse.x*0.008f*pi+iTime*0.3f;
  
  swi2S(p,x,z, _cosf(a)*swi2(p,x,z)+_sinf(a)*to_float2((p).z, -(p).x));
  
  return dfGalaxy(p, clamp(10.0f*sliderVal.x,0.9f,10.0f), sliderVal.y, sliderVal.z,iChannel0);
}

//--------------------------------------------------------------

// assign color to the media
__DEVICE__ float4 computeColor(in float3 p, in float density, in float radius, in float id, float4 colCenter, float4 colEdge, float4 colEdge2, float4 colEdge3) {
  // color based on density alone, gives impression of occlusion within
  // the media
  
  float4 result = _mix( to_float4(1.0f,0.9f,0.8f,1.0f), to_float4(0.4f,0.15f,0.1f,1.0f), density );
  // color added to the media
  result *= _mix(colCenter,
            _mix(colEdge2, 
            _mix(colEdge, colEdge3, step(0.08f,id)), step(-0.05f,id)),
                 smoothstep(0.2f,0.8f,radius) );
  return result;
}

// - Ray / Shapes Intersection -----------------------
__DEVICE__ bool sBox( in float3 ro, in float3 rd, in float3 rad, out float *tN, out float *tF)  {
    float3 m = 1.0f/rd, n = m*ro,
           k = abs_f3(m)*rad,
          t1 = -n - k, t2 = -n + k;
  *tN = _fmaxf( _fmaxf( t1.x, t1.y ), t1.z );
  *tF = _fminf( _fminf( t2.x, t2.y ), t2.z );
  return !(*tN > *tF || *tF < 0.0f);
}

__DEVICE__ bool sSphere(in float3 ro, in float3 rd, in float r, out float *tN, out float *tF) {
  float b = dot(rd, ro), d = b*b - dot(ro, ro) + r;
  if (d < 0.0f) return false;
  *tN = -b - _sqrtf(d);
  *tF = - *tN-b-b;
  return *tF > 0.0f;
}

// ---------------------------------------------------
// Bers : https://www.shadertoy.com/view/MscXzn
__DEVICE__ float4 processSliders(in float2 uv, inout float4 *sliderVal, float2 iResolution, __TEXTURE2D__ iChannel2) {
    *sliderVal = texture(iChannel2,to_float2_s(0));
    if(length(swi2(uv,x,y))>1.0f) {
      return texture(iChannel2,swi2(uv,x,y)/iResolution);
    }
    return to_float4_s(0);
}

// ---------------------------------------------------
// Based on "Dusty nebula 4" by Duke (https://www.shadertoy.com/view/MsVXWW) 
__KERNEL__ void FrogtryFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel2)
{
    fragCoord+=0.5f;
  
    float4 sliderVal;
    
    const float4 
    colCenter = to_float4(1.2f, 1.5f,1.5f,0.25f),
    colEdge  = to_float4(0.1f,0.1f,0.2f,0.5f),
    colEdge2 = to_float4(0.7f,0.54f,0.3f,0.23f),
    colEdge3 = to_float4(0.6f,1.0f,1.3f,0.25f);
    
    
float IIIIIIIIIIIIIIIIII;  
    float4 cSlider = processSliders(fragCoord, &sliderVal, iResolution, iChannel2);

   // camera     
    float a = sliderVal.w*pi;
    float3 ro = to_float3(0.0f, 2.0f*_cosf(a), -4.5f*_sinf(a)),
           ta = to_float3(-0.2f,-0.3f,0);

    // camera tx
    float3 cw = normalize( ta-ro ),
        cp = to_float3( 0.0f, 1.0f, 0.0f ),
        cu = normalize( cross(cw,cp) ),
        cv = normalize( cross(cu,cw) );
    float2 q = (fragCoord)/iResolution,
        p = -1.0f+2.0f*q;
    p.x *= iResolution.x/iResolution.y;
    
    float3 rd = normalize( p.x*cu + p.y*cv + 2.5f*cw );
      
  // ld, td: local, total density 
  // w: weighting factor
  float ld=0.0f, td=0.0f, w=0.0f;

  // t: length of the ray
  // d: distance function
  float d=1.0f, t=0.0f;
    
    const float h = 0.1f;
   
  float4 sum = to_float4_s(0);
   
    float min_dist=0.0f,  max_dist=0.0f,
          min_dist2=0.0f, max_dist2=0.0f;
    
    if(sSphere(ro, rd, 4.0f, &min_dist, &max_dist)) {
        if (sBox(ro, rd, to_float3(4.0f,1.8f,4.0f), &min_dist2, &max_dist2)) {
            min_dist = _fmaxf(0.1f,_fmaxf(min_dist, min_dist2));
            max_dist = _fminf(max_dist, max_dist2);
            
            t = min_dist*step(t,min_dist) + 0.1f*hash(rd+iTime);
      
            
            // raymarch loop
            float4 col;        
            for (int i=0; i<100; i++) {   
                float3 pos = ro + t*rd;

                // Loop break conditions.
                if(td > 0.9f || sum.w > 0.99f || t > max_dist) break;

                // evaluate distance function
                float2 res = map(pos, sliderVal, iTime, iMouse, iChannel0);
                d = _fmaxf(res.x,0.01f); 
        
                // point light calculations
                float3 ldst = pos;
                ldst.y *=1.6f;
                float3 ldst2 = pos;
                ldst2.y *=3.6f;
                float lDist = _fmaxf(length(ldst),0.1f), //_fmaxf(length(ldst), 0.001f);
                lDist2 = _fmaxf(length(ldst2),0.1f);
                // star in center
                float3 lightColor = (1.0f-smoothstep(3.0f,4.5f,lDist*lDist))*
                    _mix(0.015f*to_float3(1.0f,0.5f,0.25f)/(lDist*lDist),
                         0.02f*to_float3(0.5f,0.7f,1.0f)/(lDist2*lDist2), 
                         smoothstep(0.1f,2.0f,lDist*lDist));
                swi3S(sum,x,y,z, swi3(sum,x,y,z) + lightColor); //0.015f*lightColor/(lDist*lDist); // star itself and bloom around the light
                sum.w += 0.003f/(lDist*lDist);;

                if (d<h) {
                    // compute local density 
                    ld = h - d;
                    // compute weighting factor 
                    w = (1.0f - td) * ld;
                    // accumulate density
                    td += w + 1.0f/60.0f;
                    // get color of object (with transparencies)
                    col = computeColor(pos, td,lDist*2.0f, res.y, colCenter, colEdge, colEdge2,  colEdge3);
                    col.w *= td;
                    // colour by alpha
                    swi3S(col,x,y,z, swi3(col,x,y,z) * col.w);
                    // alpha blend in contribution
                    sum += col*(1.0f - sum.w);  
                }
  
                //float pitch = t/iResolution.x;
                //float dt = _fmaxf(d * 0.25f, 0.005f); //pitch);
                // trying to optimize step size near the camera and near the light source
                t += _fmaxf(d * 0.15f * _fmaxf(min(length(ldst), length(ro)),1.0f), 0.005f);
                td += 0.1f/70.0f;
                //t += dt;
            }
            // simple scattering
            sum *= 1.0f / _expf( ld * 0.2f )*0.8f ;  
            sum = clamp( sum, 0.0f, 1.0f );
      }
    }
        
  // Background color
    swi3S(sum,x,y,z, swi3(sum,x,y,z) + to_float3_s(clamp(2.0f*_cosf(0.5f*iTime),0.0f,0.4f))*(1.0f - sum.w)*_powf(16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y),0.3f));  
 float shitshitshit;
    //Apply slider overlay
    fragColor = to_float4_aw(_mix(swi3(sum,x,y,z),swi3(cSlider,x,y,z),cSlider.w), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}