
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

/*-------------------------------------------------------------
I have read papers for several days to deal with N-S Equation, 
but I found there are too many pre knowledge I don't know yet,
so it's too hard for me to completely understand this equation and 
how to solve it. :(
So I have to decide to give up this time to do another thing
what I read:
    WIKIPEDIA:Navier-Stokes Equations
    Real-Time Fluid Dynamics for Games by Jos Stam
and
https://www.shadertoy.com/view/4tGfDW
    Chimera's Breath by nimitz
I really like this shader, and I learned some thing too from it, thanks for this cool man
-------------------------------------------------------------*/
#define iR iResolution
#define iM iMouse
#define iT iTime
#define EPS 1e-3
#define dt  0.02f
__DEVICE__ float dot2(float2 v){return dot(v,v);}
__DEVICE__ float4 simulate(__TEXTURE2D__ tex, float2 C, float2 resolution, float4 mouse, float4 lMouse, float time)
{
    float diffV=0.22f;
    
    float2 pixw=1.0f/resolution, uv=(C*pixw);
    float4 tc=_tex2DVecN(tex,uv.x,uv.y,15),
         tl=texture(tex,uv+to_float2(-pixw.x,0)),
         tr=texture(tex,uv+to_float2(+pixw.x,0)),
         tu=texture(tex,uv+to_float2(0,pixw.y)),
         td=texture(tex,uv+to_float2(0,-pixw.y));
    //diffuse
    float4 O=tc+diffV*(tl+tr+tu+td-4.0f*tc);
    //advection
    float2 pos=uv-dt*(swi2(tc,x,y)-0.5f)*resolution.x*0.0001f; 
        //remark:density did't diffuse here, because is coverd
    swi2S(O,z,w, swi2(_tex2DVecN(tex,pos.x,pos.y,15),z,w));
    

    if(mouse.z>0.0f && lMouse.z>0.0f)
    {
        float d=1.0f/(1.0f+80000.0f*dot2((swi2(mouse,x,y)-C)*pixw.x));
        O.z+=d*0.5f;
        swi2S(O,x,y, clamp(swi2(O,x,y)+(swi2(mouse,x,y)-swi2(lMouse,x,y))*pixw.x*d*300.0f,-2.0f,2.0f));
        //O.w=_mix(O.w,fract(time*0.1f),step(0.20f,d));
    }
    //boundary
    float bound=2.0f;
    O.x=_mix(0.5f,O.x,step(bound,C.x)*step(C.x,resolution.x-bound));
    O.y=_mix(0.5f,O.y,step(bound,C.y)*step(C.y,resolution.y-bound));
    
    if(length(C-0.5f)<EPS)
    {
        swi3S(O,x,y,z, swi3(mouse,x,y,z));
        O.w=0.0f;
    }
float zzzzzzzzzzzzzzzzzz;        
    return O;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void FailedfluidsimulationJipiFuse__Buffer_A(float4 O, float2 C, float2 iResolution, float iTime, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    C+=0.5f;
//__DEVICE__ float4 simulate(__TEXTURE2D__ tex, float2 C, float2 resolution, float4 mouse, float4 lMouse, float time)
    //O=simulate(iChannel0,C,iResolution,iMouse, texelFetch(iChannel0,ito_float2_s(0.5f),0),iTime);
    
    float4 tmp = texture(iChannel0, (make_float2(to_int2(0.5f,0.5f))+0.5f)/iResolution);
    
    O = simulate(iChannel0,C,iResolution,iMouse, tmp,iTime);
float AAAAAAAAAAAAAAAAA;    
    //initalizing velocity and density
    float4 tc=texture(iChannel1,C/iResolution);
    //tc.xy=to_float2_s(0.5f);//xy-0.5f->velocity
    tc.x=(0.5f);
    tc.y=(0.5f);
    O = iFrame==0 ? swi4(tc,x,y,z,w) : O;

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void FailedfluidsimulationJipiFuse(float4 O, float2 C, float2 iResolution, sampler2D iChannel0)
{
    C+=0.5f;
 float IIIIIIIIIIIIIIIIIII;
    float4 tc = texture(iChannel0,C/iResolution);
    //O.xyz=_sqrtf(swi3(tc,x,y,z))*tc.z*1.3f;
    O = to_float4_aw(sqrt_f3(swi3(tc,x,y,z))*tc.z*1.3f, 1.0f);

  SetFragmentShaderComputedColor(O);
}
