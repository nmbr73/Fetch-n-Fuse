

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 O, in vec2 C )
{
    vec4 tc=texture(iChannel0,C/iR);
    O.rgb=sqrt(tc.xyz)*tc.z*1.3;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
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
#define iR iResolution.xy
#define iM iMouse
#define iT iTime
const float EPS=1e-3;
const float dt=0.02;
float dot2(vec2 v){return dot(v,v);}
vec4 simulate(sampler2D tex,vec2 C,vec2 resolution,vec4 mouse,vec4 lMouse,float time)
{
    float diffV=.22;
    
    vec2 pixw=1./resolution,uv=(C*pixw);
    vec4 tc=texture(tex,uv),
         tl=texture(tex,uv+vec2(-pixw.x,0)),
         tr=texture(tex,uv+vec2(+pixw.x,0)),
         tu=texture(tex,uv+vec2(0,pixw.y)),
         td=texture(tex,uv+vec2(0,-pixw.y));
    //diffuse
    vec4 O=tc+diffV*(tl+tr+tu+td-4.*tc);
    //advection
    vec2 pos=uv-dt*(tc.xy-.5)*resolution.x*0.0001; 
        //remark:density did't diffuse here, because is coverd
    O.zw=texture(tex,pos).zw;
    

    if(mouse.z>0.&&lMouse.z>0.)
    {
        float d=1./(1.+80000.*dot2((mouse.xy-C)*pixw.x));
        O.z+=d*.5;
        O.xy=clamp(O.xy+(mouse.xy-lMouse.xy)*pixw.x*d*300.,-2.,2.);
        //O.w=mix(O.w,fract(time*0.1),step(.20,d));
    }
    //boundary
    float bound=2.;
    O.x=mix(.5,O.x,step(bound,C.x)*step(C.x,resolution.x-bound));
    O.y=mix(.5,O.y,step(bound,C.y)*step(C.y,resolution.y-bound));
    
    if(length(C-.5)<EPS)
    {
        O.xyz=mouse.xyz;
        O.w=0.;
    }
        
    return O;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 O, in vec2 C )
{
    O=simulate(iChannel0,C,iR,iM,texelFetch(iChannel0,ivec2(.5),0),iT);
    
    //initalizing velocity and density
    vec4 tc=texture(iChannel1,C/iR);
    tc.xy=vec2(0.5);//xy-0.5->velocity
    O=iFrame==0?tc.xyzw:O;
}