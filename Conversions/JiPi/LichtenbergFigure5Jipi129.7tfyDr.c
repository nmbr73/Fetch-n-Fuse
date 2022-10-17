
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution
#define iR to_int2_cfloat(R)
#define uR make_uint2((uint)R.x,(uint)R.y)
#define IHash3(x,y,z) IHash((int)(x)^IHash((int)(y)^IHash((int)(z))))
#define dot2(o) dot((o),(o))
//#define tx(ch,p,R) texelFetch(ch, Zmod(p,swi2(iR,x,y)),0)
#define tx(ch,p,Res) texture(ch, (make_float2(Zmod((p).x,iR.x),Zmod((p).y,iR.y))+0.5f)/R)

//Roboust/universal integer modulus function
#define Zmod(x,y) (((x)+(y)+(y)+(y))-(((x)+(y)+(y)+(y))/(y))*(y))

//#define Zmod(x,y) (((x)+(y)+(y)+(y))-( make_int2( ((x.x)+(y.x)+(y.x)+(y.x))/(y.x), ((x.y)+(y.y)+(y.y)+(y.y))/(y.y)  ) )*(y))


//#define Zmod(x,y) ((x+y*10)%y)



__DEVICE__ uint pack(float2 x)
{
  
    x = 65534.0f*clamp(0.5f*x+0.5f, 0.0f, 1.0f);
    return uint(round(x.x)) + 65535u*uint(round(x.y));
}

__DEVICE__ float2 unpack(uint a)
{
    float2 x = to_float2(a%65535u, a/65535u);
    return clamp(x/65534.0f, 0.0f,1.0f)*2.0f - 1.0f;
}

union Zahl
 {
   float  _Float; //32bit float
   uint   _Uint;  //32bit unsigend integer
 };


__DEVICE__ float packVec2(float2 _x){
    //return uintBitsToFloat(packSnorm2x16(x/10.0f));
    //return uintBitsToFloat(pack(x/10.0f));
 
	Zahl z;
    uint X = pack(_x/10.0f);
	
	z._Uint = X;
    //return uintBitsToFloat(X); 
	return (z._Float);

}

__DEVICE__ float2 umpackVec2(float _x){
    //return unpackSnorm2x16(floatBitsToUint(x))*10.0f;
    //return unpack(floatBitsToUint(x))*10.0f;
  Zahl z;
    //uint X = floatBitsToUint(x);
	z._Float = _x;
	
  return unpack(z._Uint)*10.0f; 
}

__DEVICE__ bool inbounds(float2 x, float2 y){
    return (x.x>0.0f&&x.y>0.0f&&x.x<y.x&&x.y<y.y);
}

//RNG
__DEVICE__ int IHash(int a){
  a = (a ^ 61) ^ (a >> 16);
  a = a + (a << 3);
  a = a ^ (a >> 4);
  a = a * 0x27d4eb2d;
  a = a ^ (a >> 15);
  float zzzzzzzzzzzzzzzzz;
  return a;
}

__DEVICE__ float Hash(int a){
  return float(IHash(a)) / (float)(0x7FFFFFFF);
}
__DEVICE__ float4 rand4(int seed){
    return to_float4(Hash(seed^0x34F85A93),
                     Hash(seed^0x85FB93D5),
                     Hash(seed^0x6253DF84),
                     Hash(seed^0x25FC3625));
}
__DEVICE__ float3 rand3(int seed){
    return to_float3(Hash(seed^0x348CD593),
                     Hash(seed^0x8593FD5),
                     Hash(seed^0x62A5D384));
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

//Random injective mapping from each pixel to a random new pixel
//By alternatively adding some randomness from y into x and then 
//x into y, a reversible hash function is made. To take the inverse,
//simply undo the last "add randomness to y" step by subtracting the
//same random value. You can calculate the random value that was used
//to modify y because it depends only on x.
//Reversible == one to one == injective
//input iFrame/2 to re randoize the mapping every frame
#define mapping_iters 2
__DEVICE__ float2 forward_mapping(float2 Z,int p, int q, int Fover2){
    //int seed = 0;        // Optionaly keep seed constant for that static randomness look
    int seed = Fover2;
    if(!inbounds(Z,to_float2(p,q)))  {return to_float2_s(0);} //Dont map points from outside the boundry
    int x=(int)(Z.x);
    int y=(int)(Z.y);
    
    //Change iterations here to zero to use the identity function as a mapping
    //Some particles seem to have a better chance of getting drawn...
    //But it shows off the artifacts in all their glory, looks pretty cool after a reset
    for(int i = 0; i < mapping_iters; i++){
        x = Zmod(x + IHash(y^seed)%p,p);
        y = Zmod(y + IHash(x^seed)%q,q);
    }

  //This is the inverse mapping, only difference is - instead of + and the order of x and y
    //uncommenting should have the same effect as reducing iterations above to zero
    //This is a pretty good test of the one to one property of the mapping
    //Originally it seemed to not be working quite right on some platforms so
    //this can confirm if that is happening. The effect of a non injective mapping is collisions
    //And thus many particles getting lost near the final pass.
    /*
    for(int i = 0; i < 5; i++){
        y = Zmod(y - IHash(x)%q,q);
        x = Zmod(x - IHash(y)%p,p);
    }
  */
    
    return to_float2(x,y)+fract_f2(Z);
    
}
__DEVICE__ float2 reverse_mapping(float2 Z,int p, int q, int Fover2){
    //int seed = 0;        // Optionaly keep seed constant for that static randomness look
    int seed = Fover2;
    if(!inbounds(Z,to_float2(p,q))){return to_float2_s(0);} //Dont map points from outside the boundry
    int x=(int)(Z.x);
    int y=(int)(Z.y);
           
    for(int i = 0; i < mapping_iters; i++){
        y = Zmod(y - IHash(x^seed)%q,q);
        x = Zmod(x - IHash(y^seed)%p,p);
    }
    
    return to_float2(x,y)+fract(Z);
}

__DEVICE__ float score(float2 p, float2 I, float2 R){
    if(!inbounds(p,R)) return 1e6; //Bad score for points outside boundry
    //This should get revamped, there is no reasoning to use
    //euclidean distance, this metric probably should reflect the tree strtucture
    //Maybe even output a simple 1 or 0 if the index of this texel leads to the leaf
    //node that this particle p is going towards
    
    //Difference in the noise when using this other metric suggests that 
    //this is indeed screwing performance (likelyhood of missing particles)
    float2 D = p-I;
    D = mod_f2f2(D+R/2.0f,R)-R/2.0f;

    return _fmaxf(_fabs(D.x),_fabs(D.y));
    //use l infinity in toroidal space
    
    //return dot2(I-p);
}

__DEVICE__ void updateRank(float4 t, inout float4 *O, inout float *s, float2 I, float2 R){
    float sp = score(swi2(t,x,y),I,R);
    if(sp<*s){
        *s=sp;
        *O=t;
    }
}

//Update ranking, save a list of two particle xy indices. swi2(O,x,y) is better particle, swi2(O,z,w) is a different not as good one
__DEVICE__ void updateRank2x(float2 t, inout float4 *O, inout float *s0, inout float *s1, float2 I, float2 R){
    float sp = score(t,I,R);
    if(sp<*s0){
        //Shift down the line
        *s1=*s0;
        //(*O).zw=swi2(O,x,y);
        (*O).z=(*O).x;
        (*O).w=(*O).y;
        
        *s0=sp;
        //O.xy=t;
        (*O).x=t.x;
        (*O).y=t.y;
    } else if(sp<*s1){
        //Bump off the bottom one
        *s1=sp;
        //O.zw=t;
        (*O).z=t.x;
        (*O).w=t.y;
    }
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2



//Use O.w for render pass



__DEVICE__ float splat( float O, float2 I, float2 ip, float2 p){
    
    //O = _fminf(O,to_float4_aw(length(p-I))/3.0f);
    float d2 = dot2(I-p);
    O += _expf(-d2*0.6f)/8.0f;
float ttttttttttttttttttttttttttt;        
    return O;
    
    //if(_floor(ip)==_floor(p)) O += (0.255f);
}


//#define T2(a,b) texelFetch(iChannel2, to_int2(p)+to_int2(a,b),0).w
//#define T0(a,b) texelFetch(iChannel0, to_int2(I)+to_int2(a,b),0)

#define T2(a,b) texture(iChannel2, (make_float2(to_int2_cfloat(p)+to_int2(a,b))+0.5)/R).w
#define T0(a,b) texture(iChannel0, (make_float2(to_int2_cfloat(I)+to_int2(a,b))+0.5)/R

__KERNEL__ void LichtenbergFigure5Jipi129Fuse__Buffer_A(float4 O, float2 I, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    I+=0.5f; 

    int stage = iFrame%7;
    
    float2 uv = I/R;
    float2 r1 = rand2(IHash3(I.x,I.y,iFrame));
    float2 r2 = rand2(IHash3(I.x,I.y,34526324^iFrame));
    
    
    
    O = _tex2DVecN(iChannel0,uv.x,uv.y,15);
    float Owp = O.w;
    O.w=0.0f;
    float2 p = swi2(O,x,y);
    if(iFrame<3 || Reset){
      float2 r = rand2(IHash3(-iFrame,I.x,I.y));
        p = R/2.0f;
        swi2S(O,x,y, swi2(R,x,y) *r1);
    }
    else if(iFrame>30) {
        //reset location sometimes
        if(O.x<0.0f||O.y<0.0f||O.x>R.x||O.y>R.y||r1.x<0.075f){
            swi2S(O,x,y, swi2(R,x,y) *r2);
        }

        //Shift every point in the direction of the gradient of the blurred image in buf c
        float2 g = to_float2(T2(1,0)-T2(-1,0),T2(0,1)-T2(0,-1));
        swi2S(O,x,y, swi2(O,x,y) - normalize(g)/(2.0f) * (1.0f+I.y/R.y*4.0f));//*1e-1;
    }
    
    if(stage==0) {
           
      for(int i = 0; i < 9; i++){
        float2 ip = forward_mapping(I-1.0f+make_float2((float)i/3,(float)(i%3)),iR.x,iR.y,(iFrame)/7-1);
          //float4 t = texelFetch(iChannel1,to_int2(ip),0);
          float4 t = texture(iChannel1,(make_float2(to_int2_cfloat(ip))+0.5f)/R);

          swi2S(t,x,y, reverse_mapping(swi2(t,x,y),iR.x,iR.y,(iFrame)/7-1));
          swi2S(t,z,w, reverse_mapping(swi2(t,z,w),iR.x,iR.y,(iFrame)/7-1));
          
          O.w = splat(O.w,I,I,swi2(t,x,y));
          O.w = splat(O.w,I,I,swi2(t,z,w));
      }
    }
    O.w=_powf(O.w,0.7f);
    if(stage!=0)  O.w = Owp;
    else          O.w = _mix(O.w,Owp,0.3f);
    //Smooth over time

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//Strided sort ~= jump flood
//Strided sort summarry:
//Each pass looks at 9 xy locations stored in the previous pass and selects the closest one
//The locations sampled are arranged in a 3x3 with the center located at I, and the spacing
//a power of 3
//Total 7 passes over two frames sized large to small
//A->B->C->D->B->C->D->Image
//Spacing 3^6 ..., 3^1, 3^0
//I think this gives an optimal data path from each pixel to each other pixel under the constraint of 7 passes

//In each buffer, the pixel to get drawn at index is saved in xy and the exact particle location is saved in zw.
//For more complex particles zw should instead be a pointer to the particle
//zw is unused for sorting, sort only based on xy


//large to small
__KERNEL__ void LichtenbergFigure5Jipi129Fuse__Buffer_B(float4 O, float2 I, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    I+=0.5f;
   
  
    //Split frames into two stages
    int stage = iFrame%7;
    int size = (int)(0.5f+_powf(3.0f,(float)(6-stage)));
    
    float2 r = rand2(IHash3(iFrame,I.x,I.y));
   // if (stage==6) discard;
    
    //int size = stage==0?729:27; //729=3^6
    float s0;
    float s1;
    //init with top left corner and center
    if(stage==0){
        float2 t0 = swi2(tx(iChannel0, to_int2_cfloat(I)-size,R),x,y);
        float2 t1 = swi2(tx(iChannel0, to_int2_cfloat(I),R),x,y);
                                                                         
        s0 = score(t0,I,R);
        s1 = score(t1,I,R);
        
        swi2S(O,x,y,  (t0.x==0.0f && t0.y==0.0f) ? to_float2_s(0):forward_mapping(t0, iR.x, iR.y,iFrame/7))
        swi2S(O,z,w,  (t1.x==0.0f && t1.y==0.0f) ? to_float2_s(0):forward_mapping(t1, iR.x, iR.y,iFrame/7))
        
        //Select the better one, make sure scores are in order with s0<s1
        if(s0>s1){
            float2 _ = swi2(O,x,y);
            //swi2(O,x,y) = swi2(O,z,w);
            O.x = O.z;
            O.y = O.w;
            
            //swi2(O,z,w) = _;
            O.z = _.x;
            O.w = _.y;
            
            _.x = s0;
            s0 = s1;
            s1 = _.x;
        }
    } else {
        O = tx(iChannel1, to_int2_cfloat(I)-size,R );
        s0 = score(swi2(O,x,y),I,R);
        s1 = score(swi2(O,z,w),I,R);
    }
    for(int i = 1; i < 9; i++){
        if(stage==0){
          float2 t = swi2(tx(iChannel0,to_int2_cfloat(I)-size+size*to_int2(i/3,i%3),R),x,y);
            t = forward_mapping(t, iR.x, iR.y,iFrame/7);
            updateRank2x(t,&O,&s0,&s1,I,R);
            
        } else {
          float4 t;
            t = tx(iChannel1,to_int2_cfloat(I)-size+size*to_int2(i/3,i%3),R); 
            
            updateRank2x(swi2(t,x,y),&O,&s0,&s1,I,R);
            updateRank2x(swi2(t,z,w),&O,&s0,&s1,I,R);
        }
        
    }

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Preset: Keyboard' to iChannel3
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer A' to iChannel2
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


//Do a multipass blur, radius controlled by iMouse
__KERNEL__ void LichtenbergFigure5Jipi129Fuse__Buffer_C(float4 O, float2 I, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    I+=0.5f;
    //#define T0C(a,b) texelFetch(iChannel0, Zmod((to_int2(I)+to_int2(a,b)),to_int2(swi2(R,x,y))),0).wwww
    //#define T1C(a,b) texelFetch(iChannel1, Zmod((to_int2(I)+to_int2(a,b)),to_int2(swi2(R,x,y))),0).wwww

    //#define T0C(a,b) to_float4_s(texture(iChannel0, (make_float2(Zmod((to_int2_cfloat(I)+to_int2(a,b)),to_int2_cfloat(R)))+0.5f)/R).w)
    //#define T1C(a,b) to_float4_s(texture(iChannel1, (make_float2(Zmod((to_int2_cfloat(I)+to_int2(a,b)),to_int2_cfloat(R)))+0.5f)/R).w)

    #define T0C(a,b) to_float4_s(texture(iChannel0, (make_float2(Zmod((int)(I.x) + a, (int)(R.x)), Zmod((int)(I.y) + b, (int)(R.y)))+0.5f)/R).w)
    #define T1C(a,b) to_float4_s(texture(iChannel1, (make_float2(Zmod((int)(I.x) + a, (int)(R.x)), Zmod((int)(I.y) + b, (int)(R.y)))+0.5f)/R).w)

    O = T1C(0,0);//*0.999999f;
    O -= T0C(0,0);
    O=_mix(O,(T1C(0,0) + T1C(0,1) + T1C(1,0) + T1C(0,-1) + T1C(-1,0))/5.0f,iFrame<60?0.01f:0.85f-0.15f*iMouse.y/R.y);
    if(iFrame<3 || Reset){
        O.w=-10.0f+length(swi2(I,x,y)-R/2.0f); 
        //O.w = 0.0f; 
    }
    if(length(I-R/2.0f)<1.0f){
        O-=1.0f;
    }


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer A' to iChannel0
// Connect Buffer D 'Previsualization: Buffer D' to iChannel1


__KERNEL__ void LichtenbergFigure5Jipi129Fuse__Buffer_D(float4 O, float2 I, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    I+=0.5f;

    O=to_float4_s(texture(iChannel0,I/R).w);
    O=_mix(O,texture(iChannel1,I/R),0.96f);

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0
// Connect Image 'Previsualization: Buffer D' to iChannel1



//Strided sort and spatial decorrelation fit into buf a and b over 7 frames

//Rendering in buf A
__KERNEL__ void LichtenbergFigure5Jipi129Fuse(float4 O, float2 I, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    I+=0.5f;

    //O=_fabs(texture(iChannel0,I/swi2(R,x,y)))/1e3;//textureLod(iChannel0,to_float2_s(0.5f),6.0f)/2.0f;
    O=texture(iChannel1,I/R);
    //O += texelFetch(iChannel2, to_int2(I),0);

    O += (Color-0.5f);
    O.w = Color.w;    

  SetFragmentShaderComputedColor(O);
}
