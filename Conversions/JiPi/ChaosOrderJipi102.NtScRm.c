
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define R iResolution

#define N 1000    //number of points
#define H 200     //number of scanned points in buffer B
#define TL 0.015f //sphere size 

#define Init float2 R = iResolution;\
    int NX = int(iResolution.x);\
    int NY = (N/NX+1);\
    int M  = _fminf(10,(int)(_sqrtf(iResolution.y/(float)(NY))));\
    int L  = (int)(_powf((float)(N),1.0f/3.0f));


// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------


//random points 
//// Hash without Sine (c)2014 David Hoskins
__DEVICE__ float3 hash32(float2 p)
{
  float hhhhhhhhhhhhhhhhh;
    float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
    return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x));
}


__KERNEL__ void ChaosOrderJipi102Fuse__Buffer_A(float4 O, float2 U, float iTime, float2 iResolution, float4 iMouse)
{

   Init
   if((int)(U.y)>(NY*M*M) || (int)(U.x)>NX) { SetFragmentShaderComputedColor(O); return;}
   int i= ((int)(U.y)%NY)*NX + (int)(U.x);
        
   float3 p = _mix(to_float3(i/(L*L),(i%(L*L))/L,i%L)/(float)(L)-0.5f+1.0f/(float)(L) , (hash32(U)-0.5f),0.5f+0.5f*_cosf(iTime/2.0f));    
  
   float a = iTime/4.0f + iMouse.x/iResolution.x*3.0f;
float AAAAAAAA;   
   //a = iMouse.x/iResolution.x*3.0f;
   
   swi2S(p,x,y, mul_f2_mat2(swi2(p,x,y), to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))));
   swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z), to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))));
   //O.xy=swi2(p,x,y) *1.4f / (2.0f+p.z);
   O.x = p.x *1.4f / (2.0f+p.z);
   O.y = p.y *1.4f / (2.0f+p.z);
   
   O.z=(float)(i)/(float)(N);
   O.w= p.z;

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


//points in each region
__KERNEL__ void ChaosOrderJipi102Fuse__Buffer_B(float4 O, float2 U, float2 iResolution, sampler2D iChannel0)
{

    Init
    float _O[4] = {H,H,H,H};
    if((int)(U.y)>(NY*M*M) || (int)(U.x)>NX) { SetFragmentShaderComputedColor(O); return;}
       
    int j = (int)(U.y)/NY, //current region
        i = ((int)(U.y)%NY)*NX + (int)(U.x); //current point
float BBBBBBBBBBBBBBBBBBB;
    bool f[4] = {false,false,false,false};
    for( int k = i; k<_fminf(N,i+H);k++)
    { //iterate on max H points in region
    
        //vec4 p= texelFetch(iChannel0,to_int2(k%NX,k/NX),0); //point position
        float4 p= texture(iChannel0,(to_float2(k%NX,k/NX)+0.5f)/R); //point position
        float r= TL*2.0f/(2.0f+p.w);
        float sz=1.0f/(float)(M);
        float2 s =(to_float2((j%M),j/M)+0.25f);
        float2 s1=s+0.25f; s1=s1*sz-0.5f ; s1 *= R/R.y ; //region center
        if(_fmaxf(_fabs(p.x-s1.x)-0.5f*sz*R.x/R.y,_fabs(p.y-s1.y)-0.5f*sz)>r) continue; //outside region
          
        for(int jj=0;jj<4;jj++){
          if(f[jj]) continue; //already found next point for subregion
               
          float2 ss=s +to_float2((jj==1|| jj==3) ?0.5f:0.0f,(jj==2|| jj==3) ?0.5f:0.0f); ss=ss*sz-0.5f ; ss *= R/R.y ; //subregion center
      
            if(_fmaxf(_fabs(p.x-ss.x)-0.25f*sz*R.x/R.y,_fabs(p.y-ss.y)-0.25f*sz)<r)
            {
                //this point is inside subergion
                _O[jj]= (float)(k-i); 
                f[jj]=true;
            }
       } 
       
       O = to_float4(_O[0],_O[1],_O[2],_O[3]);
       //if(all(f)) return; 
       if(f[0] && f[1] && f[2] && f[3]) { SetFragmentShaderComputedColor(O); return;} 
    }

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


// chaos & order by kastorp
//---------------------------
__KERNEL__ void ChaosOrderJipi102Fuse(float4 O, float2 U, float4 iMouse, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    Init
    float2  p = (U/R.y -R/R.y*0.5f),mp = (swi2(iMouse,x,y)/R.y -R/R.y*0.5f),
        c = (p*R.y/R +0.5f)*(float)(M) ;    
    int j = (int)(c.x) + (int)(c.y)*M, //current region id
        jj= (int)(step(0.5f,fract(c.x))+2.0f*step(0.5f,fract(c.y)));
    
    O = to_float4(0,0,0,10);
    float q;
    for(int i=0,k=i;k<N && i<500;i++){ // scan points in current region (max 500)
float IIIIIIIIIIIIII;        
        //int sk=int(texelFetch(iChannel1,to_int2(k%NX,k/NX +j*NY),0)[jj]);
        int sk;// = (int)(texture(iChannel1,(to_float2((int)(k%NX,(int)k/NX +j*NY))+0.5f)/R)[jj]);
        
        if (jj==0) sk = (int)(texture(iChannel1,(make_float2((int)k%NX,(int)k/NX +j*NY)+0.5f)/R).x);
        if (jj==1) sk = (int)(texture(iChannel1,(make_float2((int)k%NX,(int)k/NX +j*NY)+0.5f)/R).y);
        if (jj==2) sk = (int)(texture(iChannel1,(make_float2((int)k%NX,(int)k/NX +j*NY)+0.5f)/R).z);
        if (jj==3) sk = (int)(texture(iChannel1,(make_float2((int)k%NX,(int)k/NX +j*NY)+0.5f)/R).w);
        
        if(sk==0) { //the point is in
            //vec4 c = texelFetch(iChannel0,to_int2(k%NX,k/NX ),0);
            float4 c = texture(iChannel0,(to_float2((int)(k%NX),(int)(k/NX))+0.5f)/R);
            float r = TL*2.0f/(2.0f+c.w);
            float d =length(p-swi2(c,x,y)), 
                //sh= (1.5f- length(p-swi2(c,x,y)-to_float2(0,r*0.5f))/r)*1.5f -0.5f;sh=0.5f+sh*sh;
                sh= (1.0f+dot((p-swi2(c,x,y)) +to_float2(0,d+r),to_float2(0,d+r))/d/r)*0.15f; 
            if(c.w<O.w && d<r) O = to_float4_aw((0.5f + 0.5f*cos_f3(c.z*6.0f+to_float3(0,2,4)))*sh,c.w);
            
            k+=1; //next point
        }
        else k+=(int)(sk); //skip till first point in region
        q = (float)(i-N/H)/30.0f;
    }
   //O = texture(iChannel1,U/R);

  SetFragmentShaderComputedColor(O);
}
