
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define pi 3.14159265f
#define W(i,n) cexp(to_float2(0,FFT_DIR*2.0f*pi*(float)(i)/(float)(n)))
#define R iResolution
#define fradix (float)(radix)
#define T(c,x,y) texelFetch(c, to_int2(x,y), 0)

#define SUM(expr, ind, len) \
    sum = to_float2_s(0);\
    for(int ind = 0; ind < 64; ind++){\
        if (ind >= len) break;\
        sum += expr;\
    }

#define FFT_DIR (float)((iFrame%2)*2-1)
#define FORWARD   1.0f
#define BACKWARD -1.0f

float2 sum;

__DEVICE__ int x_N0;
__DEVICE__ int y_N0;
__DEVICE__ int x_N1;
__DEVICE__ int y_N1;

__DEVICE__ float dot2(float2 _x) { return dot(_x,_x); }

__DEVICE__ float factor(float _x){
    _x = _floor(_x);
    float f = _floor(_sqrtf(_x));
    while(fract(_x/f)>0.5f/_x){f--;}
    return _x/f;
}

__DEVICE__ void setRadix(float2 R){
    x_N0 = int(R.x/factor(R.x));
    y_N0 = int(R.y/factor(R.y));
    x_N1 = int(R.x)/x_N0;
    y_N1 = int(R.y)/y_N0;
}

__DEVICE__ float2 cprod(float2 a, float2 b){
    return to_mat2(a.x,a.y,-a.y,a.x) * b;
}

__DEVICE__ float2 cis(float t){
    return cos_f2(t - to_float2(0,pi/2.0f));
}
__DEVICE__ float2 cexp(float2 z) {
    return exp_f2(z.x)*cis(z.y);
}
__DEVICE__ int IHash(int a){
  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  return a;
}

__DEVICE__ float Hash(int a){
  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  return (float)(a) / (float)(0x7FFFFFFF);
}
__DEVICE__ float2 rand2(int seed){
    return to_float2(Hash(seed^0x348C5F93),
                     Hash(seed^0x8593D5BB));
}


__DEVICE__ float2 randn(float2 randuniform){
    float2 r = randuniform;
    r.x = _sqrtf(-2.0f*_logf(1e-9+_fabs(r.x)));
    r.y *= 6.28318f;
    r = r.x*to_float2(_cosf(r.y),_sinf(r.y));
    return r;
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer D' to iChannel0



__DEVICE__ float4 inp(__TEXTURE2D__ ch,int _x, int _y){
    if(FFT_DIR==FORWARD){
        float2 v = T(ch, x, y).xz;
        return texture(ch, fract_f2((-v + to_float2(_x, _y) + rand2(IHash(x^IHash(y^IHash(iFrame)))))/swi2(R,x,y)));
    } else {
        return T(ch, x, y);
    }
}


__KERNEL__ void FftFluidJipiFuse__Buffer_A(float4 O, float2 I, int iFrame, sampler2D iChannel0)
{

    I+=0.5f;

    setRadix(R);
    O = to_float4_s(0);
    int x = (int)(I.x);
    int y = (int)(I.y);  
    
    int n = (x/x_N1);
    SUM( cprod((inp(iChannel0, (x%x_N1)+i*x_N1, y).xy),W(i*n,x_N0)),i,x_N0 );
    swi2(O,x,y) = (cprod(sum, W((x%x_N1)*n,int(R.x))));

    SUM( cprod((inp(iChannel0, (x%x_N1)+i*x_N1, y).zw),W(i*n,x_N0)),i,x_N0 );
    swi2(O,z,w) = (cprod(sum, W((x%x_N1)*n,int(R.x))));

  SetFragmentShaderComputedColor(O);
}



// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0



__KERNEL__ void FftFluidJipiFuse__Buffer_B(float4 O, float2 I, sampler2D iChannel0)
{

    setRadix(R);
    O = to_float4_aw(0);
    int x = int(I.x);
    int y = int(I.y);
    
    int n = (x/x_N0);
    SUM( cprod((T(iChannel0, (x%x_N0)*x_N1+i, y).xy),W(i*n,x_N1)),i,x_N1 );
    swi2(O,x,y) = (sum);
    
    SUM( cprod((T(iChannel0, (x%x_N0)*x_N1+i, y).zw),W(i*n,x_N1)),i,x_N1 );
    swi2(O,z,w) = (sum);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void FftFluidJipiFuse__Buffer_C(float4 O, float2 I, sampler2D iChannel0)
{

    setRadix(R);
    O = to_float4(0);
    int x = int(I.x);
    int y = int(I.y);
    
    int n = (y/y_N1);
    SUM( cprod((T(iChannel0, x, (y%y_N1)+i*y_N1).xy),W(i*n,y_N0)),i,y_N0 );
    swi2(O,x,y) = (cprod(sum, W((y%y_N1)*n,int(R.y))));
    
    SUM( cprod((T(iChannel0, x, (y%y_N1)+i*y_N1).zw),W(i*n,y_N0)),i,y_N0 );
    swi2(O,z,w) = (cprod(sum, W((y%y_N1)*n,int(R.y))));


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Preset: Keyboard' to iChannel2
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void FftFluidJipiFuse__Buffer_D(float4 O, float2 I, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{

    setRadix(R);
    O = to_float4(0);
    int x = int(I.x);
    int y = int(I.y);
    
    int n = (y/y_N0);
    SUM( cprod((T(iChannel0, x, (y%y_N0)*y_N1+i).xy),W(i*n,y_N1)),i,y_N1 );
    swi2(O,x,y) = (sum/_sqrtf(R.x*R.y));
    
    SUM( cprod((T(iChannel0, x, (y%y_N0)*y_N1+i).zw),W(i*n,y_N1)),i,y_N1 );
    swi2(O,z,w) = (sum/_sqrtf(R.x*R.y));
    
    float2 C = mod_f(swi2(I,x,y)+R.xy/2.0f,swi2(R,x,y))-R.xy/2.0f;
    if(FFT_DIR==FORWARD){
        if(texelFetch(iChannel2,to_int2(88,2),0).x<0.5f)
          O*=_expf(-dot2( C )*2e-7);
        if(length(C)>0.0f && texelFetch(iChannel2,to_int2(90,2),0).x<0.5f){
            float l = length(swi2(O,x,z));
          swi2(O,x,z)-=dot(normalize(C),swi2(O,x,z))*normalize(C);
            if(texelFetch(iChannel2,to_int2(67,2),0).x<0.5f)
              swi2(O,x,z) *= (l/(1e-3+length(swi2(O,x,z))));
        }
        if(length(C)<1.0f) O*=0.0f;
        if(length(C)>0.0f && texelFetch(iChannel2,to_int2(90,2),0).x<0.5f){
            float l = length(swi2(O,y,w));
          swi2(O,y,w)-=dot(normalize(C),swi2(O,y,w))*normalize(C);
            if(texelFetch(iChannel2,to_int2(67,2),0).x<0.5f)
              swi2(O,y,w) *= (l/(1e-3+length(swi2(O,y,w))));
        }
        
    } else {
        swi2(O,x,z) += 0.01f*to_float2(swi2(iMouse,x,y)-swi2(R,x,y)*0.5f)*_expf(-0.1f/(1.0f+length(I-swi2(R,x,y)*0.5f))*dot2(I-swi2(R,x,y)*0.5f));
    }
    
    if(iFrame<6 && FFT_DIR==BACKWARD){
        O=to_float4(0);
    }


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel1
// Connect Image 'Previsualization: Buffer D' to iChannel3


//Four step seperated FFT, factored horizontally, vertically, and over major and minor axis for each of x and y
//Total worst case for 2048*2048 image is 2 (x and y) times 4 (factored into 4) 32pt dft's where each pixel/thread 
//must compute one bin of its corresponding dft. Pipelining through A-B-C-D means fft of the whoe screen only takes one frame.
//Both the x and y of the feild need to be fft'd so it takes up all 4 channels to do an fft, so every other frame
//the fft direction is swapped to compute the inverse, and overall the simulation runs at one step per two frames



__KERNEL__ void FftFluidJipiFuse(float4 O, float2 I, sampler2D iChannel1, sampler2D iChannel3)
{

    O = to_float4(0);
    if(FFT_DIR==BACKWARD){
        if(texelFetch(iChannel1,to_int2(32,2),0).x>0.5f) discard;
        float4 t0 = texture(iChannel3, fract(I/swi2(R,x,y)));
        O = to_float4(0.5f*_logf(0.5f*length(t0)));
    } else {
        if(texelFetch(iChannel1,to_int2(32,2),0).x<0.5f) discard;
      O = texture(iChannel3, fract(0.5f+I/swi2(R,x,y)));
        float l0 = dot2(swi2(O,x,y));
        swi2(O,x,y) = O.xy/l0*_logf(1.0f+l0);
        float l1 = dot2(swi2(O,z,w));
        swi2(O,z,w) = O.zw/l1*_logf(1.0f+l1);
        swi3(O,x,y,z) += to_float3(1,1,0)*O.w;
        O=_fabs(O);
    }


  SetFragmentShaderComputedColor(O);
}