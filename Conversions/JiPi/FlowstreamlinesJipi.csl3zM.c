
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//int len = 10; // curve length ( i.e. number of samples )
#define R iResolution
//#define T(U)   texelFetch( iChannel0, to_int2(U), 0 )
#define T(U)   texture( iChannel0, (make_float2(to_int2_cfloat(U))+0.5f)/R )
#define H(p) ( fract_f2(sin_f2((float)(p)*to_float2(269.5f,183.3f)) *43758.5453123f)*R/R.y )

__DEVICE__ float L(float2 p, float2 a,float2 b) { // --- draw line  ( squared distance )
    p -= a, b -= a;
    float h = dot(p, b) / dot(b, b), 
          c = clamp(h, 0.0f, 1.0f);     
    return h==c ? dot(p - b*h ,p) : 1e5; 
}

__DEVICE__ float2 field(float2 U, __TEXTURE2D__ CH, float2 R) {  // --- velocity field induced by texture
    return  swi2(texture(CH, _fmaxf(to_float2(0,2.0f/R.y), U*R.y/R)),x,y) * 0.2f;
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


__DEVICE__ float dFdx(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.x / iResolution.x );
}
__DEVICE__ float dFdy(float value, float2 fragCoord, float2 iResolution)
{
   return ( value*fragCoord.y / iResolution.y );
}

// ---- compute BBoxs of next curves

__KERNEL__ void FlowstreamlinesJipiFuse__Buffer_A(float4 O, float2 u, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(len, -1.0f, 20.0f, 10.0f);
  
    u+=0.5f;
float AAAAAAAAAAAAAA;
    if (u.y>1.0f) { 
      O = texture(iChannel1,u/R); 
      O = to_float4(-dFdy(O.x,u,R),dFdx(O.x,u,R),O.x,0); // contour vectors
   // O = to_float4_aw(0.1f*normalize(to_float2(-dFdy(O.x),dFdx(O.x))),O.x,0);
      SetFragmentShaderComputedColor(O);
      return; 
    }

    float i = u.x-0.5f, l = 9.0f, p = 1.0f/R.y;
    float2  P = H(i+0.5f-iTime), _P=P, m=P,M=P;  // random start point
    
    for( int t=0; t <len; t++, _P = P )      // follow trajectory
        P += field(P,iChannel1,R),
        m = _fminf(m,P), M = _fmaxf(M,P);        

    O = to_float4_f2f2(m-p,M+p);

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// --- draw only curves which BBox covers the pixel

__KERNEL__ void FlowstreamlinesJipiFuse__Buffer_B(float4 O, float2 u, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(len, -1.0f, 20.0f, 10.0f);

    u+=0.5f;
float BBBBBBBBBBBBBBBBB;
    float2 U = u / R.y, P, _P, b;
    O =  0.98f*T(u);                         // cumulates previous draw
   
    for( float i=0.0f,l; i < 400.0f; i+=1.0f ) {   // draw N streamlines per frame
        //float4 B = texelFetch(iChannel1,to_int2(i,0),0); // curve bbox
        float4 B = texture(iChannel1,(make_float2(to_int2(i,0))+0.5f)/R); // curve bbox
        b = step(swi2(B,x,y),U)*step(U,swi2(B,z,w));
     // if (b.x*b.y == 0.0f) continue;       // this form crash Firefox !
        if (b.x*b.y == 1.0f) {               // pixel not in BBox: skip draw
        _P = P = H(i+0.5f-iTime);            // random start point
        l = 9.0f;
        for( int t=0; t <len; t++, _P = P ) // follow trajectory
            P += field(P,iChannel1,R),
            l = _fminf( l, L(U,_P,P) );
        O += 0.2f*_fmaxf(0.0f, 1.0f-R.y*_sqrtf(l));   // draw the curve
       }
    }

  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


// variant of https://shadertoy.com/view/cdfGz7
// field = texture isogrey

__KERNEL__ void FlowstreamlinesJipiFuse(float4 O, float2 u, float2 iResolution, sampler2D iChannel0)
{
    u+=0.5f;
 float IIIIIIIIIIIIIIII;
 // O = _sqrtf(T(u));
    O = sqrt_f4(to_float4_s(1.0f)-exp_f4(-1.0f*T(u)*to_float4(4,2,1,1))); // color

  SetFragmentShaderComputedColor(O);
}