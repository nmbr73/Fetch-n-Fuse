
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//vec2 R; float4 M; float T; int I;
//#define Main void mainImage(out float4 Q, float2 U) 
//#define UNISET R=iResolution;M=iMouse;T=iTime;I=iFrame;
//#define A(U) texelFetch(iChannel0,to_int2(U),0)
#define A(U) texture(iChannel0,(make_float2(to_int2_cfloat(U))+0.5f)/iResolution)


//#define L  0.08f //spehere radius
#define G 10.0f  //gravitiy foce
#define DT 0.02f //time interval
//#define N 40    //number of spheres
#define VI 8    // number of iterations

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void VerletIntFuse__Buffer_A(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{

  CONNECT_SLIDER0(Radius, 0.01f, 0.2f, 0.08f);
  CONNECT_INTSLIDER0(Count, 1, 40, 40);

  float L = Radius;
  int N = Count;

    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    if(U.y>1.0f || U.x>(float)(N)) { SetFragmentShaderComputedColor(Q); return;} //discard;

    float4 a[40];
    for(int i =0;i<N;i++){
        a[i] = A(to_float2(i,0));
        float2 po=swi2(a[i],z,w), p = swi2(a[i],x,y);
        //verlet integration 
        float2 pn= 2.0f* p - po + 0.5f*DT*DT *2.0f*( M.z>0.0f?G*( swi2(M,x,y)/R.y-R/R.y*0.5f):to_float2(0,-G));  
        a[i]=to_float4_f2f2(pn,p);          
    }
    
    //iterate constraint
    for(int k=0;k<VI;k++){
    
    
        //solve collision
        for(int i =0;i<N;i++)for(int j =i+1;j<N;j++){  
            float d = length(swi2(a[i],x,y) - swi2(a[j],x,y));
            if(d<2.0f*L ){
                float2 m=(swi2(a[i],x,y)+ swi2(a[j],x,y))*0.5f,
                     d0= (1.005f- d/2.0f/L)*(m-swi2(a[i],x,y)),
                     d1= (1.005f- d/2.0f/L)*(m-swi2(a[j],x,y)),
                     v0= swi2(a[i],x,y)-swi2(a[i],z,w),
                     v1= swi2(a[j],x,y)-swi2(a[j],z,w),
                     n= normalize(swi2(a[i],x,y) -swi2(a[j],x,y)),
                     vm= (v0+v1)*0.5f,
                     vn= n* _fminf(dot(n,v0-v1),0.0f)*0.5f;
                //if(k==0) 
                v0= vm-vn;v1=vm+ vn;

                a[i]=to_float4_f2f2(swi2(a[i],x,y)- d0,swi2(a[i],x,y)- d0- v0);
                a[j]=to_float4_f2f2(swi2(a[j],x,y)- d1,swi2(a[j],x,y)- d1- v1 );

            }
        }
        
        //TODO: sticks for ragdoll or ropes
        
        //solve wall
        for(int i =0;i<N;i++){        
            float2 p=swi2(a[i],z,w), pn = swi2(a[i],x,y);
            if(pn.y<L)          { p.y= 2.0f*L-p.y; pn.y=2.0f*L-pn.y;  }
            if(pn.x<L)          { p.x= 2.0f*L-p.x; pn.x=2.0f*L-pn.x; }
            if(pn.x>-L+R.x/R.y) { p.x= -2.0f*L+2.0f*R.x/R.y-p.x; pn.x=-2.0f*L+2.0f*R.x/R.y-pn.x; }
            if(pn.y>-L+1.0f)    { p.y= -2.0f*L+2.0f-p.y; pn.y=-2.0f*L+2.0f-pn.y; }
            a[i]=to_float4_f2f2(pn,p);        
        }
    }
    Q=a[(int)(U.x)];
    if(I<1) {    
         Q = to_float4(0.01f+U.x/(float)(N),0.2f +U.x,U.x/(float)(N),0.2f+U.x);   
    }    

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//Verlet integration test by Kastorp
//---------------------------------------

__KERNEL__ void VerletIntFuse(float4 Q, float2 U, float iTime, float2 iResolution, float4 iMouse, int iFrame)
{
  
  CONNECT_SLIDER0(Radius, 0.01f, 0.2f, 0.08f);
  CONNECT_INTSLIDER0(Count, 1, 40, 40);

  float L = Radius;
  int N = Count;
    
    float2 R=iResolution;float4 M=iMouse;float T=iTime;int I=iFrame;
    
    float ratio = R.x/R.y;        
    
    Q=to_float4(0.8f,0.95f,0.95f,0);  
    for(int i=0;i<N;i++){
        float2 p = swi2(A(to_float2(i,0)),x,y);  
        Q=_mix(Q, (0.6f+0.4f*cos_f4(to_float4(0,2,4,0)+ to_float4_s(i)/(float)(N)*6.28f))* smoothstep(L,0.0f,length(U/R.y-p-L*0.2f)-L*0.5f), smoothstep(0.002f,0.0f,length(U/R.y-p)-L));  
        
        
        if (i==0) 
        {
          float2 tuv = U/iResolution.y-p+0.10f;
          //tuv += 0.10f;
          tuv.x/=ratio;
          float4 tex = texture(iChannel2, tuv);
          Q=_mix(Q, tex* smoothstep(L,0.0f,length(U/R.y-p-L*0.2f)-L*0.5f), smoothstep(0.002f,0.0f,length(U/R.y-p)-L));  
        }          

        if (i==1) 
        {
          float2 tuv = U/iResolution.y-p+0.10f;
          //tuv += 0.10f;
          tuv.x/=ratio;
          float4 tex = texture(iChannel3, tuv);
          Q=_mix(Q, tex* smoothstep(L,0.0f,length(U/R.y-p-L*0.2f)-L*0.5f), smoothstep(0.002f,0.0f,length(U/R.y-p)-L));  
        }                  
        
        
    } 
    Q*= _powf((1.0f - U.y / R.y) *U.x / R.x*(1.0f - U.x / R.x) *U.y / R.y * 10.0f, 0.15f);   


//Q= (0.6f+0.4f*cos_f4(to_float4(0,2,4,0)+ to_float4_s(0)/(float)(40)*6.28f));

  SetFragmentShaderComputedColor(Q);
}