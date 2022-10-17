

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// created by florian berger (flockaroo) - 2021
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
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
//#define Res iResolution.xy
//#define ROT(v,x) v=mat2(cos(x),sin(x),-sin(x),cos(x))*v;
//#define R(p) cos(((p)+(p).zxy*1.1+(p).yzx*1.3)*10.)
//#define L(n,c) for(int i=0;i<n;i++){c;}
//#define t iTime
//
//#define dd(X,p2) \
//{ \
//    vec3 p=p2; \
//    p+=R(p*.3)*.05; float l,d=1e3,s=2.; \
//    vec3 q,r; \
//    L(4,s*=.5;q=floor(p/s)*s;r=R(q);if(r.x<.5) break) \
//    p=((p-q)/s-.5)*sign(r); s*=8.; \
//    L(3,l=length(p.xy+.5)*s; d=min(d,length(vec2(l-(min(floor(l),s-1.)+.5),(fract(p.z*s+.5*s)-.5)))/s); p.zxy=p*vec3(-1,-1,1) ) \
//    X+=(d*s/8.-2e-3)*.6; \
//}
//
//void mainImage( out vec4 C, in vec2 FC )
//{
//    vec3 p,d=vec3((FC-Res*.5)/Res.x*2.,-.7);
//    ROT(d.yz,t*.2);
//    ROT(d.xy,t*.07);
//    p=vec3(7,2,1)*t/1e2;
//    float x=0.;
//    L(200,dd(x,p+d*x))
//    C=-C+1.-exp(-x/3.);
//    C.w=1.;
//}

// wow!! fabrices take with 548 chars:
//
//#define R2(a)  mat2(cos( iTime*a +vec4(0,33,11,0)))
//#define H(p)   cos( p/.1 +p.zxy*11. + p.yzx*13. )
//#define L(n)   for( int i=0 ; i++ < n ; )
//
//void mainImage( out vec4 C, vec2 U )
//{
//    vec3 R = iResolution,
//         P = vec3(7,2,1) * iTime/1e2, q,r,p,
//         D = vec3( ( U+U - R.xy ) / R.x, -.7 );
//    D.yz *= R2(.2);
//    D.xy *= R2(.07);
//  
//    float x=0.,l,d,s;
//    L( 200 ) { 
//        p = P+D*x;
//        p += H(.3*p)*.05;
//        d=1e3; s=2.; r=R/R;
//        L( 4 && r.x>.5 ) { s*=.5; q = floor(p/s)*s; r = H(q); }
//        p = ( (p-q)/s - .5 ) * sign(r);
//        s *= 8.;
//        L( 3 ) { 
//           l = length(p.xy+.5)*s;
//           d = min(d, length(vec2( l - min(floor(l),s-1.) , 
//                                   fract(p.z*s+.5*s) ) -.5
//                            ) /s);
//           p.zxy = p * vec3(-1,-1,1) ;
//        }
//        x += d*s*.075 -1.2e-3 ;
//    }
//    C = 1.-C-exp(-x/3.);
//}

//!!!and even smaller Xor with 539 chars:
//(and i took another 11 off by (sloppily) replacing the exp by a linear fog
// and using fabrices loop condition) - so now 526 chars:
// ...and a lot of fabrice changes added!
#define A    mat2( cos(vec4(0,33,11,0)+t*//
#define R(p) cos( p/.1+p.zxy*11.+p.yzx*13. )//
#define L(n) for(int i=0; i++<n;)


void mainImage(out vec4 C, vec2 F)
{
    float t = iTime,l,D,s;C-=C;
    vec3 R = iResolution,
    d = vec3((F+F-R.xy)/R.x,-.7),
    p = vec3(7,2,1)*t/1e2,P,q,r;
    
    d.yz *= A .2));
    d.xy *= A .07));
    
    L(200)
    {
        P = p+d*C.x*3.;
        P += R(.3*P)*.05;
        l = D = 1e3, s = 2., r=R;
        
        L(4 && r.x>.5)
            s *= .5,
            q = floor(P/s),
            r = R(s*q);
        
        P = (P/s-q-.5)*sign(r); s*=8.;
        
        L(3)
            D = min(D,length(.5-vec2(min(ceil(l=length(P.xy+.5)*s),s)-l,fract(P.z*s+.5*s)))/s),
            P.zxy=P,P.zx*= -1.;
        
        C += D*s*.025-4e-4;
    }
}
