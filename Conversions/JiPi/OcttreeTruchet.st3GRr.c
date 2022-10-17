
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// created by florian berger (flockaroo) - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//
// 3D-generalization of multi-scale-truchets (Octtree truchet)
//
// ...like Shanes 2D version (not sure, but i think he did it first in 2D)
// "Quadtree Truchet" https://www.shadertoy.com/view/4t3BW4
//
// wasnt sure if it works visually in 3D - kind of crowded, but i like it a lot...
// different regions with different truchet-scale can easily be seen.
//
// golfed it down, but i guess there's still potential...
//
// also here a (only single-scale) version on twigl in under 1 tweet:
// https://twitter.com/flockaroo/status/1454405159224754184

//my original version:
//
//#define Res iResolution
//#define ROT(v,x) v=to_mat2(_cosf(x),_sinf(x),-_sinf(x),_cosf(x))*v;
//#define R(p) _cosf(((p)+swi3((p),z,x,y)*1.1f+swi3((p),y,z,x)*1.3f)*10.0f)
//#define L(n,c) for(int i=0;i<n;i++){c;}
//#define t iTime
//
//#define dd(X,p2) \
//{ \
//    float3 p=p2; \
//    p+=R(p*0.3f)*0.05f; float l,d=1e3,s=2.0f; \
//    float3 q,r; \
//    L(4,s*=0.5f;q=_floor(p/s)*s;r=R(q);if(r.x<0.5f) break) \
//    p=((p-q)/s-0.5f)*sign(r); s*=8.0f; \
//    L(3,l=length(swi2(p,x,y)+0.5f)*s; d=_fminf(d,length(to_float2(l-(_fminf(_floor(l),s-1.0f)+0.5f),(fract(p.z*s+0.5f*s)-0.5f)))/s); p.zxy=p*to_float3(-1,-1,1) ) \
//    X+=(d*s/8.0f-2e-3)*0.6f; \
//}
//
//void mainImage( out float4 C, in float2 FC )
//{
//    float3 p,d=to_float3_aw((FC-Res*0.5f)/Res.x*2.0f,-0.7f);
//    ROT(swi2(d,y,z),t*0.2f);
//    ROT(swi2(d,x,y),t*0.07f);
//    p=to_float3(7,2,1)*t/1e2;
//    float x=0.0f;
//    L(200,dd(x,p+d*x))
//    C=-C+1.0f-_expf(-x/3.0f);
//    C.w=1.0f;
//}

// wow!! fabrices take with 548 chars:
//
//#define R2(a)  mat2(_cosf( iTime*a +to_float4(0,33,11,0)))
//#define H(p)   _cosf( p/0.1f +swi3(p,z,x,y)*11.0f + swi3(p,y,z,x)*13.0f )
//#define L(n)   for( int i=0 ; i++ < n ; )
//
//void mainImage( out float4 C, float2 U )
//{
//    float3 R = iResolution,
//         P = to_float3(7,2,1) * iTime/1e2, q,r,p,
//         D = to_float3( ( U+U - swi2(R,x,y) ) / R.x, -0.7f );
//    swi2(D,y,z) *= R2(0.2f);
//    swi2(D,x,y) *= R2(0.07f);
//  
//    float x=0.0f,l,d,s;
//    L( 200 ) { 
//        p = P+D*x;
//        p += H(0.3f*p)*0.05f;
//        d=1e3; s=2.0f; r=R/R;
//        L( 4 && r.x>0.5f ) { s*=0.5f; q = _floor(p/s)*s; r = H(q); }
//        p = ( (p-q)/s - 0.5f ) * sign(r);
//        s *= 8.0f;
//        L( 3 ) { 
//           l = length(swi2(p,x,y)+0.5f)*s;
//           d = _fminf(d, length(to_float2( l - _fminf(_floor(l),s-1.0f) , 
//                                   fract(p.z*s+0.5f*s) ) -.5
//                            ) /s);
//           swi3(p,z,x,y) = p * to_float3(-1,-1,1) ;
//        }
//        x += d*s*0.075f -1.2e-3 ;
//    }
//    C = 1.0f-C-_expf(-x/3.0f);
//}

//!!!and even smaller Xor with 539 chars:
//(and i took another 11 off by (sloppily) replacing the exp by a linear fog
// and using fabrices loop condition) - so now 526 chars:
// ...and a lot of fabrice changes added!
//#define A    to_mat2( cos_f4(to_float4(0,33,11,0)+t*                //
#define R(p) cos_f3( p/0.1f+swi3(p,z,x,y)*11.0f+swi3(p,y,z,x)*13.0f )//
#define L(n) for(int i=0; i++<n;)


__DEVICE__ inline mat2 to_mat2_f4 ( float4  a )  { mat2 t; t.r0.x = a.x; t.r0.y = a.y; t.r1.x = a.z; t.r1.y = a.w;   return t; }

__KERNEL__ void OcttreeTruchetFuse(float4 C, float2 F, float iTime, float2 iResolution)
{

    CONNECT_CHECKBOX0(Invers, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    float t = iTime,l,D,s;C-=C;
    float2 R = iResolution;
    float3 d = to_float3_aw((F+F-R)/R.x,-0.7f),
    p = to_float3(7,2,1)*t/1e2,P,q,r;

    swi2S(d,y,z, mul_f2_mat2(swi2(d,y,z) , to_mat2_f4( cos_f4(to_float4(0,33,11,0)+t* 0.2f))));
    swi2S(d,x,y, mul_f2_mat2(swi2(d,x,y) , to_mat2_f4( cos_f4(to_float4(0,33,11,0)+t* 0.07f))));
    
    L(200)
    {
        P = p+d*C.x*3.0f;
        P += R(0.3f*P)*0.05f;
        l = D = 1e3, s = 2.0f, r=to_float3_aw(R,0.0f);
        
        L(4 && r.x>0.5f)
            s *= 0.5f,
            q = _floor(P/s),
            r = R(s*q);
        
        P = (P/s-q-0.5f)*sign_f3(r); s*=8.0f;
        
        L(3)
        {
            D = _fminf(D,length(0.5f-to_float2(_fminf(_ceil(l=length(swi2(P,x,y)+0.5f)*s),s)-l,fract(P.z*s+0.5f*s)))/s);
            swi3S(P,z,x,y,P);
            //swi2(P,z,x)*= -1.0f;
            P.z*= -1.0f;
            P.x*= -1.0f;
            
        }
        C += D*s*0.025f-4e-4;
    }

  C = (C + Color) * C.w;

  if (Invers) C = to_float4_s(1.0f) - C;
  C.w=Color.w;

  SetFragmentShaderComputedColor(C);
}