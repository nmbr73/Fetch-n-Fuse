

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//forked from https://www.shadertoy.com/view/stfGzr by lomateron 
void mainImage( out vec4 O, in vec2 U )
{
    
    vec4 v=texelFetch(iChannel0,ivec2(U),0);
    
    O = (.3+.7*smoothstep(.01,.0,v.w) )//border    
     * (sin(     
         v*vec4(.9, 5.2,3.5,0) //colors: base, node, vein
         +vec4(.2) //background
        )*.5+.5); 
   
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define A(u) texelFetch(iChannel0,ivec2(u),0)
#define R iResolution
void mainImage( out vec4 O, in vec2 u )
{       
    float z =8.,dm=1.;
    vec4 k1= vec4(1.,3.24,6.,.2), 
         k2= vec4(-1,1.07,-0.03,-0.03),
         s=vec4(0),b=s;
  
    for(float i = 0.; i<=z;i++) for(float j = 0.; j<=z;j++){
        float  l=(i*i+j*j)/z/z;
        if(l<=1.) {
            vec4 e=exp(-l*k1);
            s+= (i==0.?1.:2.)*(j==0.?1.:2.)* e;
            for(float i1 =-1.;i1<=1.;i1+=2.)for(float j1 =-1.;j1<=1.;j1+=2.){//symmetry
                float d = A(u+vec2(i*i1,j*j1)).x;             
                b+=d*e;
                if(d>=1.)dm=min(dm,l);
            }
        }
    }    
    
    O=vec4(1.,b.x/s.x,b.y/s.y*6.,dm*z)* 
    (((iFrame%1200)==0) ? 
        smoothstep(.01,0.,abs(.1+.01*sin(u.x)-length((2.*u-R.xy)/R.y))):
        clamp( z* dot(b/s,k2),0.,1.));    
    if(iMouse.z>0.) O =mix(O,1.-500.*O, max(0.,.1-length(2.*(u-iMouse.xy)/R.y))); 

}