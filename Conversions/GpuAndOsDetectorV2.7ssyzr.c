#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define UNKNOWN 0
#define WINDOWS 1
#define LINUX 2
#define OSXIOS 3
#define ANDROID 4

#define NVIDIA 1
#define AMD 2
#define INTEL 3
#define ADRENO 4
#define MALI 5
#define APPLE 6
#define SOFTWARE 7
#define VIDEOCORE 8

#define GL 1
#define ANGLE 2

#define _space 32

#define _0 48
#define _1 49
#define _2 50
#define _3 51
#define _4 52
#define _5 53
#define _6 54
#define _7 55
#define _8 56
#define _9 57

#define _A 65
#define _B 66
#define _C 67
#define _D 68
#define _E 69
#define _F 70
#define _G 71
#define _H 72
#define _I 73
#define _J 74
#define _K 75
#define _L 76
#define _M 77
#define _N 78
#define _O 79
#define _P 80
#define _Q 81
#define _R 82
#define _S 83
#define _T 84
#define _U 85
#define _V 86
#define _W 87
#define _X 88
#define _Y 89
#define _Z 90


#define _a 97
#define _b 98
#define _c 99
#define _d 100
#define _e 101
#define _f 102
#define _g 103
#define _h 104
#define _i 105
#define _j 106
#define _k 107
#define _l 108
#define _m 109
#define _n 110
#define _o 111
#define _p 112
#define _q 113
#define _r 114
#define _s 115
#define _t 116
#define _u 117
#define _v 118
#define _w 119
#define _x 120
#define _y 121
#define _z 122

#define _slash 47

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Font 1' to iChannel0


/*
Math functions are implemented differently in each GPU and drivers.
This will run the same calculation compile time and runtime,
and print the two results in the top-right corner.
It will guess your GPU and OS based on the hashes.

Reciptrocal, sqrt, sin all probably work with constants in a table for a polynom.

If your hardware is unknown or wrong, please comment your hashes and hardware description.

*/
#define DYN_ZERO _fminf(0.0f,iTime) // forcing runtime calculation
__DEVICE__ int hardwareHash(float start)
{
    float a=start;
    for(int i=0;i<20;i++)
    {
        a=fract(normalize(to_float3(a+0.1f,6.11f,5.22f)).x*3.01f);
        a+=_sinf(sqrt(a)*100.3f)*0.31f;
    }
    return int(fract(fract(_fabs(a))*256.0f)*256.0f*256.0f);
}

// #define C(c) printCursor.x-=0.5f;char(printCursor,c);
// __DEVICE__ void char(float2 p, int c)
// {
//     if (p.x<.0|| p.x>1.0f || p.y<0.|| p.y>1.0f) return;
//   printColor= _fmaxf(printColor, texture( iChannel0, p/16.0f + fract( to_float2(c, 15-c/16) / 16.0f ) ).x+backGroundColor);
// }

#define C(c)                                                                                  \
  printCursor.x-=0.5f;                                                                        \
  if (!(printCursor.x<0.0|| printCursor.x>1.0f || printCursor.y<0.|| printCursor.y>1.0f)) {   \
    printColor= _fmaxf(                                                                       \
      printColor, texture(                                                                    \
        iChannel0,                                                                            \
        printCursor/16.0f + fract( to_float2( c, 15-c/16 ) / 16.0f )                          \
      ).x+backGroundColor                                                                     \
    );                                                                                        \
  };                                                                                          \


// __DEVICE__ void printHex(int i)
// {
//     float div = 16.0f*16.0f*16.0f;
//     for(int j=3;j>=0;j--)
//     {
//         float digit = fract(float(i)/div/16.0f)*16.0f;
//         div/=16.0f;
//         C(int((digit<10.?48.:65.-10.0f)+digit));
//     }
// }


__KERNEL__ void GpuAndOsDetectorV2Fuse(float4 O, float2 uv, float iTime, float2 iResolution, sampler2D iChannel0)
{

    // font and printing
    float2 printCursor;
    float backGroundColor=0.0f;
    float printColor=0.0f;

    uv /= _fminf(iResolution.y,iResolution.x/1.9f);


    int hash_runtime = hardwareHash(0.0f+DYN_ZERO);
    int hash_comptime = hardwareHash(0.0f);


    int os=UNKNOWN;
    int gpu=UNKNOWN;
    int gldx=UNKNOWN;

    // nVidia
    if (hash_runtime==0x1EC4) { gpu=NVIDIA; } // 3060 WindowsGL and Linux
    if (hash_runtime==0xFA2D) { os=LINUX; gpu=NVIDIA; }  // 1050
    if (hash_runtime==0x716D) { gpu=NVIDIA; } // Quatro

    // AMD
    if (hash_runtime==0x58A4) { os=WINDOWS; gpu=AMD; gldx=GL;}// AMD GCN4
    if (hash_runtime==0x3F8E) { os=WINDOWS; gpu=AMD; gldx=ANGLE;}// AMD GCN4
    if (hash_runtime==0x76B5) { os=LINUX; gpu=AMD; gldx=GL;}// AMD GCN4
    if (hash_runtime==0x47EF) { gpu=AMD; } // AMD RDNA
    if (hash_runtime==0xF04A) { gpu=AMD; os=WINDOWS; } // AMD RDNA angle
    if (hash_runtime==0x40DC) { gpu=AMD; } // AMD GCN5  (Vega 64)
    if (hash_runtime==0x553A) { gpu=AMD; os=OSXIOS; gldx=GL; } // CT:00E2


    // intel
    if (hash_runtime==0x859D) { gpu=INTEL; } // Intel 7.gen // Windows?
    if (hash_runtime==0xEEE7) { gpu=INTEL; } // Intel 2.gen or 7.0f gen linux?
    if (hash_runtime==0x289F) { gpu=INTEL; os=OSXIOS; gldx=GL; }  // OSX ( Intel 2.gen Mac mini Catalina)
    if (hash_runtime==0xE6DB) { gpu=INTEL; os=OSXIOS; gldx=GL; } // CT: 8DEA

    // software rendering
    if (hash_runtime==0xFDf0) { gpu=SOFTWARE; } // Linux?
    if (hash_runtime==0x884E) { gpu=SOFTWARE; gldx=GL; }

    // adreno
    if (hash_runtime==0xF09B) { gpu=ANDROID; os=ANDROID; gldx=GL; }

    // mali
    if (hash_runtime==0x5Af5) { gpu=MALI; os=LINUX; gldx=GL; } // Asus Tinkerboard
    if (hash_runtime==0xACB3) { gpu=MALI; os=ANDROID; gldx=GL; }

    // apple
    if (hash_runtime==0x89D8) { gpu=APPLE; os=OSXIOS; gldx=GL; } // CT=0xEE26 iPad 6.gen
    if (hash_runtime==0x8C7B) { gpu=APPLE; os=OSXIOS; gldx=GL; } // CT=0xEE26 Apple A14
    if (hash_runtime==0x9F12) { gpu=APPLE; os=OSXIOS; gldx=GL; } // CT=0xEE26 iPhone Safari


    if (hash_comptime==0x83CA) { gldx = GL; os=WINDOWS; }; // OpenGL Windows AMD
    if (hash_comptime==0xFDF0) { gldx = GL; }; // OpenGL Windows, Linux (non-nVidia)
    if (hash_comptime==0xE6DB) { gldx = GL; os=LINUX; }; // OpenGL Linux  AMD/Intel/Software
    if (hash_comptime==0x1D23) { gldx = GL; } // only with nVidia, any OS
    if (hash_comptime==0x239C) { gldx = ANGLE; os = WINDOWS; };
    if (hash_comptime==0x00E2) { gpu=AMD; os=OSXIOS; gldx=GL; }; // Maxbook AMD Radeon Pro 5500M


    uv *= 10.0f;
    int line = 9-int(uv.y);
    int column = int(uv.x/5.0f);
    printCursor = to_float2(mod_f(uv.x,5.0f),fract(uv.y));

    if (column==0)
    {
        backGroundColor = (line==gpu)?0.5:0.;
        if (line==UNKNOWN) { C(_u);C(_n);C(_k);C(_n);C(_o);C(_w);C(_n); }
        if (line==NVIDIA) {C(_n);C(_V);C(_i);C(_d);C(_i);C(_a);}
        if (line==AMD) {C(_A);C(_M);C(_D);}
        if (line==INTEL) {C(_I);C(_n);C(_t);C(_e);C(_l);}
        if (line==ADRENO) { C(_A);C(_d);C(_r);C(_e);C(_n);C(_o); }
        if (line==MALI) {C(_M);C(_a);C(_l);C(_i);}
        if (line==SOFTWARE) {C(_s);C(_o);C(_f);C(_t);C(_w);C(_a);C(_r);C(_e);}
        if (line==APPLE) {C(_A);C(_p);C(_p);C(_l);C(_e);}
    }
    if (column==1)
    {
        backGroundColor = (line==os)?0.5:0.;
        if (line==UNKNOWN) { C(_u);C(_n);C(_k);C(_n);C(_o);C(_w);C(_n); }
        if (line==WINDOWS) { C(_W);C(_i);C(_n); }
        if (line==LINUX) { C(_L);C(_i);C(_n);C(_u);C(_x); }
        if (line==OSXIOS) { C(_O);C(_S);C(_X); C(_slash); C(_i);C(_O);C(_S); }
        if (line==ANDROID) { C(_A);C(_n);C(_d);C(_r);C(_o);C(_i);C(_d); };
    }
    if (column==2)
    {
        backGroundColor = (line==gldx)?0.5:0.;
        if (line==UNKNOWN) { C(_u);C(_n);C(_k);C(_n);C(_o);C(_w);C(_n); }
        if (line==GL) {C(_G);C(_L);}
        if (line==ANGLE) { C(_A);C(_N);C(_G);C(_L);C(_E);}
    }
    if (column==3)
    {
        if (line==0 || line==1)
        {   int i= (line==0?hash_runtime:hash_comptime);
            float div = 16.0f*16.0f*16.0f;
            for(int j=3;j>=0;j--)
            {
                #if 1
                  float digit = fract(float(i)/div/16.0f)*16.0f; // hiert knallt das Macro unter OpenCL!?!
                  div/=16.0f;
                  C(int((digit<10.0f?48.0f:65.0f-10.0f)+digit));
                #else
                  float f=float(i)/div/16.0f;
                  float digit=(f-_floor(f)) *16.0f;
                  div/=16.0f;
                  f=int((digit<10.0f?48.0f:65.0f-10.0f)+digit);
                  C(f);
                #endif



            }
        }

    }


    O = to_float4(printColor,_powf(printColor,2.0f),_powf(printColor,3.0f),0.0f)*0.8f;


  SetFragmentShaderComputedColor(O);
}