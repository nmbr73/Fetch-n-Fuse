
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel0


// "RayMarching starting point" 
// by Martijn Steinrucken aka The Art of Code/BigWings - 2020
// The MIT License
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
// Email: countfrolic@gmail.com
// Twitter: @The_ArtOfCode
// YouTube: youtube.com/TheArtOfCodeIsCool
// Facebook: https://www.facebook.com/groups/theartofcode/
//

#define MAX_STEPS 200
#define MAX_DIST 30.0f
#define SURF_DIST 0.001f

#define S smoothstep
#define T iTime


__DEVICE__ mat2 Rot(float a) {
    float s=_sinf(a), c=_cosf(a);
    return to_mat2(c, -s, s, c);
}

struct Hit{
    float d;
    float obj;
    float3 id;
};



__DEVICE__ float sdRoundBox( float3 p, float3 b, float r )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(_fmaxf(q.x,_fmaxf(q.y,q.z)),0.0f) - r;
}

__DEVICE__ float sdBox(float3 p, float3 s) {
    p = abs_f3(p)-s;
  return length(_fmaxf(p, to_float3_s(0.0f)))+_fminf(max(p.x, _fmaxf(p.y, p.z)), 0.0f);
}


__DEVICE__ float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); }


__DEVICE__ Hit GetDist(float3 p, float iTime) {

    float3 boxpos=p;

   swi2S(boxpos,x,z, mul_f2_mat2(swi2(boxpos,x,z),Rot(T*0.7f)));
   swi2S(boxpos,x,y, mul_f2_mat2(swi2(boxpos,x,y),Rot(-T*0.5f)));
   swi2S(boxpos,y,z, mul_f2_mat2(swi2(boxpos,y,z),Rot(-T*0.8f)));
    
    float d = sdRoundBox(boxpos, to_float3_s(0.9f),0.2f);
   
   float obj=-0.0f;
 //  swi2(boxpos,x,y)*=Rot(-T);
 //  swi2(boxpos,y,z)*=Rot(T);

    
   float rep=_mix(0.5f,1.8f,0.5f+0.5f*(_sinf(T*0.4f)));
   boxpos+=rep/2.0f;
   float3 q=mod_f((boxpos),rep)-rep/2.0f;
   float3 ids=_floor(boxpos-q);
  
   float s2 = length(q)-(0.08f+(0.05f*_sinf(T+(ids.x+ids.y)+ids.z)))*(rep*2.0f);
   float s2bis = sdBox(q,to_float3_s((0.05f+(0.05f*_sinf(T+(ids.x+ids.y)+ids.z)))*(rep*2.0f)));
   s2=_mix(s2,s2bis,0.5f+0.5f*_sinf(T*2.0f+(ids.x+ids.y)+ids.z));
   s2=_fmaxf(d+0.01f,s2);
   
   d=_fmaxf(d,-s2+0.08f);//*(rep+0.5f));    
    
    if(s2<d)
        obj=1.0f;
    
    d=_fminf(s2,d);
        
    float3 q2=mod_f(p,2.0f)-1.0f;
    float3 id=_floor(p-q2);
    q2.y=p.y+_sinf(T+id.x*id.y)*0.5f+1.35f;
    float ds=length(q2)-0.4f;    
    
    ds=_fmaxf(ds,-sdBox(p,to_float3_s(2.5f)));
    ds=_fmaxf(ds,length(p)-6.0f);
    if(ds<d){
        obj=3.0f;
    }
    d=_fminf(d,ds);
    
    float pl=p.y+1.5f;
    if(pl<d)
        obj=3.0f;
    
    d=opSmoothUnion(d,pl,0.4f);    

    Hit ret = {d,obj,ids};  
    return ret;//Hit(d,obj,ids);
}

__DEVICE__ Hit RayMarch(float3 ro, float3 rd,float direction, float iTime) {
    float dO=0.0f;
    float obj=0.0f;
    float3 id;
    for(int i=0; i<MAX_STEPS; i++) {
      float3 p = ro + rd*dO;
        Hit h=GetDist(p,iTime);
        obj=h.obj;
        id=h.id;
        float dS = h.d*direction;
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    Hit ret = {dO,obj,id};
    return ret;//Hit(dO,obj,id);
}

__DEVICE__ float3 GetNormal(float3 p, float iTime) {
    float d = GetDist(p,iTime).d;
    float2 e = to_float2(0.001f, 0);
    
    float3 n = d - to_float3(
        GetDist(p-swi3(e,x,y,y),iTime).d,
        GetDist(p-swi3(e,y,x,y),iTime).d,
        GetDist(p-swi3(e,y,y,x),iTime).d);
    
    return normalize(n);
}

__DEVICE__ float3 GetRayDir(float2 uv, float3 p, float3 l, float z) {
        float3 f = normalize(l-p),
        r = normalize(cross(to_float3(0,1,0), f)),
        u = cross(f,r),
        c = f*z,
        i = c + uv.x*r + uv.y*u,
        d = normalize(i);
    return d;
}

__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}

__KERNEL__ void SimpleRefractionTestFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
    CONNECT_COLOR0(Color1, 0.54f, 0.3f, 0.7f, 1.0f);
    CONNECT_COLOR1(Color2, 0.2f, 0.1f, 0.8f, 1.0f);
    CONNECT_COLOR2(Color3, 0.03f, 0.08f, 0.1f, 1.0f);
    CONNECT_COLOR3(BKGColor, 0.08f, 0.08f, 0.08f, 1.0f);
    

    CONNECT_SLIDER1(refmul, 0.0f, 1.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, 0.0f, 1.0f, 0.2f);
    
    CONNECT_SLIDER3(Zoom, -10.0f, 10.0f, 0.0f);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = swi2(iMouse,x,y)/iResolution;

    float3 ro = to_float3(0, 1.5f, -5+Zoom);
    if(dot(swi2(m,x,y),swi2(m,x,y))>0.0f){
        swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-_fminf(m.y,0.45f)*3.14f+1.0f)));
        swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f)));
    }
       
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z),Rot(T/2.0f)));
       
    float3 rd = GetRayDir(uv, ro, to_float3(0,0.0f,0), 1.0f);
    float3 col = to_float3_s(0);
   
    float bo=6.0f;
    float fresnel=1.0f;

    bool issecond=false;
    Hit h;
    float i=0.0f;
    float3 p;
    for(;i<bo;i++){
        h=RayMarch(ro, rd,1.0f,iTime);
        float IOR=1.35f;
        //col*=1.0f/bo;

        if(h.d<MAX_DIST){
            if(h.obj==0.0f){
                p = ro + rd * h.d;
                float3 n = GetNormal(p,iTime);
                               
                float3 rIn=_refract_f3(rd,n,1.0f/IOR,refmul,refoff);

                Hit hIn= RayMarch(p-n*0.003f,rIn,-1.0f,iTime);
                
                float dIn=hIn.d;
                float3 pIn=p+rIn*dIn;
                float3 nIn=-1.0f*GetNormal(pIn,iTime);

                float3 rOut=to_float3_s(0.0f);
                float shift=0.01f;

                rOut=_refract_f3(rIn,nIn,IOR,refmul,refoff);
                if(dot(rOut,rOut)==0.0f) rOut=reflect(-rIn,nIn);
                ro=pIn-nIn*0.03f;
                rd=rOut;
                
            }
            else if(h.obj==1.0f){
                float3 p = ro + rd * h.d;
                float3 n = GetNormal(p,iTime);
                float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
                //col+=((0.5f+0.5f*sin_f3((to_float3(0.54f,0.3f,0.7f)+h.id)*T))*fresnel)*0.7f;
                col+=((0.5f+0.5f*sin_f3((swi3(Color1,x,y,z)+h.id)*T))*fresnel)*0.7f;
                col *= to_float3_s(dif);
                //*1.0f/bo;
                break;
            }
            else if(h.obj==2.0f){
                break;
                float3 p = ro + rd * h.d;
                float3 n = GetNormal(p,iTime);
                float dif = dot(n, normalize(to_float3(1,2,3)))*0.5f+0.5f;
                col+=swi3(Color2,x,y,z);//to_float3(0.2f,0.1f,0.8f);

                col *= to_float3_s(dif);
                break;
            }
            else if(h.obj==3.0f){
                p = ro + rd * h.d;
                float3 n = GetNormal(p,iTime);

                ro=p+n*0.003f;
                rd=reflect(rd,n);
                if(!issecond){
                    fresnel=_powf(1.0f-dot(rd,n),2.0f);
                col+=swi3(Color3,x,y,z);//to_float3(0.03f,0.08f,0.1f);
                }
                issecond=true;
            }
            
        }
        else{
            float3 bcolor=swi3(BKGColor,x,y,z);//to_float3_s(0.08f);
            if(i==0.0f ){
                col=bcolor;
                }
            else
                col=_mix((col+swi3(decube_f3(iChannel0,rd),x,y,z))/i*fresnel,bcolor,1.0f-S(15.0f,0.0f,length(p)));
            break;
        }
    }
    
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    
    fragColor = to_float4_aw(col,BKGColor.w);

  SetFragmentShaderComputedColor(fragColor);
}