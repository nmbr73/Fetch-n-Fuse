
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15) 

#define time iTime
#define rez iResolution
#define pi 3.14159
__DEVICE__ float4 load(float _x, float _y, float2 rez, __TEXTURE2D__ iChannel0){return texture(iChannel0,to_float2(_x+0.5f,_y+0.5f)/rez);}
__DEVICE__ mat2 rmat(float a){return to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a));}
__DEVICE__ float2 cmul(float2 a, float2 b){return to_float2(a.x*b.x-a.y*b.y,dot(a,swi2(b,y,x)));}

__KERNEL__ void BrokenBuddhaFuse__Buffer_A(float4 O, float2 U, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0)
{

  U+=0.5f;
  
  
  if(iFrame<1) { O=to_float4_s(0.0f); SetFragmentShaderComputedColor(O); return;}
  
  O=texture(iChannel0,U/rez);
  U=_floor(U);
  float4 L=load(0.0f,0.0f, rez, iChannel0);
  float f=(float)(iFrame);
  float a=_floor(f)*0.001f;
  
  if(U.x+U.y==0.0f){
    int iters=(int)(_floor(a))+7;
    a*=pi;
    float2 p=to_float2(_cosf(a),_sinf(a/1.5f)),rd=-p;p.x-=0.5f*_cosf(a*0.007f);
    for(int i=0;i<32;i++){
      float dr=1.0f,r=length(p);
      float2 C=p,Z=p;
      for(int n=0;n<iters && r<2000.0f;n++){
        Z=cmul(Z,Z)+C;
        dr=dr*r*2.0f+1.0f;
        r=length(Z);
      }
      p+=rd*0.5f*_logf(r)*r/dr;
    }
    p+=rd*(1.0f+_cosf(a*0.3f))*0.1f;
    O=to_float4(p.x,p.y,p.x,p.y); 
  }else{
    float2 p0=swi2(L,x,y);
    float2 u=(2.0f*U-swi2(rez,x,y))/rez.y;
    for(int j=0;j<5;j++){
      float2 p=p0+to_float2_s(0.001f)*(float)(j);
      for(int n=0;n<20 && dot(p,p)<300000.0f;n++){
        p=cmul(p,p)+p0;
        float d=length(abs_f2(u)-abs_f2(p));d=smoothstep(2.0f/rez.y,0.0f,d)*0.2f;///(1.0f+40.0f*O.x*O.x);
        float3 col=to_float3_aw(d*mul_f2_mat2(p,rmat(a*7.0f+(float)(j)*0.3f)),d);
        swi2S(col,y,z, mul_f2_mat2(swi2(col,y,z),rmat(a*5.0f+float(j+n)*0.03f)));
        O+=to_float4_aw(abs_f3(swi3(col,x,y,z)),0.0f);
      }
    }
    O=O*0.995f;
  }
  
  

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


//broken buddha by eiffie 
//track the position of points near the border of the mandelbrot set as they orbit
#define time iTime
#define rez iResolution

__KERNEL__ void BrokenBuddhaFuse(float4 O, float2 U, float iTime, float2 iResolution, sampler2D iChannel0)
{
  CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
  CONNECT_CHECKBOX0(Alpha, 0);
  CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 0.0f);

  O=texture(iChannel0,U/swi2(rez,x,y));

  O = to_float4_aw( swi3(O,x,y,z)+swi3(Color,x,y,z)-0.5f ,Color.w);
  
  if (Alpha&&O.x>AlphaThres&&O.y>AlphaThres&&O.z>AlphaThres) O.w =1.0f;
  
  SetFragmentShaderComputedColor(O);
}
