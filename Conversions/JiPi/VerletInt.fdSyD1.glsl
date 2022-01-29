

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Verlet integration test by Kastorp
//---------------------------------------

void mainImage(out vec4 Q, vec2 U)  {
    
    vec2 R=iResolution.xy;vec4 M=iMouse;float T=iTime;int I=iFrame;
    Q=vec4(0.8,0.95,0.95,0);  
    for(int i=0;i<N;i++){
        vec2 p = A(vec2(i,0)).xy;  
        Q=mix(Q, (.6+.4*cos(vec4(0,2,4,0)+ vec4(i)/float(N)*6.28))* smoothstep(L,.0,length(U/R.y-p-L*.2)-L*.5), smoothstep(0.002,.0,length(U/R.y-p)-L));  
    } 
    Q*= pow((1. - U.y / R.y) *U.x / R.x*(1. - U.x / R.x) *U.y / R.y * 10.0, 0.15);   
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage(out vec4 Q, vec2 U)  {

    vec2 R=iResolution.xy;vec4 M=iMouse;float T=iTime;int I=iFrame;
    if(U.y>1. || U.x>float(N)) discard;
    
    vec4[N] a;
    for(int i =0;i<N;i++){
         a[i] = A(vec2(i,0));
        vec2 po=a[i].zw, p = a[i].xy;
        //verlet integration 
        vec2 pn= 2.* p - po + .5*DT*DT *2.*( M.z>0.?G*( M.xy/R.y-R.xy/R.y*.5):vec2(0,-G));  
        a[i]=vec4(pn,p);          
    }
    
    //iterate constraint
    for(int k=0;k<VI;k++){
    
    
        //solve collision
        for(int i =0;i<N;i++)for(int j =i+1;j<N;j++){  
            float d = length(a[i].xy -a[j].xy);
            if(d<2.*L ){
                vec2 m=(a[i].xy+ a[j].xy)*.5,
                     d0= (1.005- d/2./L)*(m-a[i].xy),
                     d1= (1.005- d/2./L)*(m-a[j].xy),
                     v0= a[i].xy-a[i].zw,
                     v1= a[j].xy-a[j].zw,
                     n= normalize(a[i].xy -a[j].xy),
                     vm= (v0+v1)*.5,
                     vn= n* min(dot(n,v0-v1),0.)*.5;
                //if(k==0) 
                v0= vm-vn;v1=vm+ vn;

                a[i]=vec4(a[i].xy- d0,a[i].xy- d0- v0);
                a[j]=vec4(a[j].xy- d1,a[j].xy- d1- v1 );

            }
        }
        
        //TODO: sticks for ragdoll or ropes
        
        //solve wall
        for(int i =0;i<N;i++){        
            vec2 p=a[i].zw, pn = a[i].xy;
            if(pn.y<L) { p.y= 2.*L-p.y; pn.y=2.*L-pn.y;  }
            if(pn.x<L) { p.x= 2.*L-p.x; pn.x=2.*L-pn.x; }
            if(pn.x>-L+R.x/R.y) { p.x= -2.*L+2.*R.x/R.y-p.x; pn.x=-2.*L+2.*R.x/R.y-pn.x; }
            if(pn.y>-L+1.) { p.y= -2.*L+2.-p.y; pn.y=-2.*L+2.-pn.y; }
            a[i]=vec4(pn,p);        
        }
    }
    Q=a[int(U.x)];
    if(I<1) {    
         Q = vec4(.01+U.x/float(N),.2 +U.x,U.x/float(N),.2+U.x);   
    }    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//vec2 R; vec4 M; float T; int I;
//#define Main void mainImage(out vec4 Q, vec2 U) 
//#define UNISET R=iResolution.xy;M=iMouse;T=iTime;I=iFrame;
//#define A(U) texelFetch(iChannel0,ivec2(U),0)
#define A(U) texture(iChannel0,(vec2(ivec2(U))+0.5)/iResolution.xy)


#define L  0.08 //spehere radius
#define G 10.0  //gravitiy foce
#define DT 0.02 //time interval
#define N 40    //number of spheres
#define VI 8    // number of iterations
