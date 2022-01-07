
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


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



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Texture: Abstract 2' to iChannel1
// Connect 'Texture: Font 1' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)




//#define Cp(c) *printColor+= _char(*printCursor,c,iChannel0).x; *printCursor.x-=0.5f
#define C(c) printColor+= _char(printCursor,c,iChannel0).x; printCursor.x-=0.5f




__DEVICE__ float4 _char(float2 p, int c, __TEXTURE2D__ iChannel0) 
{
    if (p.x<0.0f|| p.x>1.0f || p.y<0.0f|| p.y>1.0f) return to_float4(0,0,0,1e5);
  return texture( iChannel0, p/16.0f + fract_f2( to_float2(c, 15-c/16) / 16.0f ) );
}



__DEVICE__ void printHex(uint i, inout float2 *printCursor, inout float *printColor, __TEXTURE2D__ iChannel0 )
{
  
    for(int j=3;j>=0;j--)
    {
        uint digit = (uint)((i>>(j*4))&15u);
        int c = ((int)((digit<=9u?48u:65u-10u)+digit));
        *printColor += _char(*printCursor,c,iChannel0).x; (*printCursor).x-=0.5f;
    }
}


__DEVICE__ uint hardwareHash( __TEXTURE2D__ iChannel1)
{
    float3 a = normalize(swi3(texture(iChannel1,to_float2(0.50f,0.52f)),x,y,z));
    a += sin_f3(swi3(texture(iChannel1,to_float2(0.51f,0.52f)),x,y,z))*0.1f;
    a += sqrt_f3(swi3(texture(iChannel1,to_float2(0.52f,0.52f)),x,y,z))*0.1f;
    return (uint)(fract_f(_fabs(a.x))*65536.0f*256.0f)&0xFFffu;
}

__KERNEL__ void GpuManufacturerDetectorFuse(float4 O, float2 uv, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    float2 printCursor;
    float printColor=0.0f;

    uv /= iResolution.y;
    float2 position = to_float2(0.1f,0.4f);
    printCursor = ( uv - position)*5.0f;
    
    uint hash = hardwareHash(iChannel1);
    switch((int)(hash))
    {
    case 0x204E:  // nVidia
        C(_n);C(_V);C(_i);C(_d);C(_i);C(_a);
        break;
        
    case 0x23CB: // AMD rx 560
    case 0x2148: // AMD r7 250 ultimate
    case 0x21C6: // AMD-A4-4000 APU
        C(_A);C(_M);C(_D);
        break;
        
    case 0x3DF3: // intel 11th gen
    case 0x2386: // Intel (hybrid graphics?...)
    case 0x1CB1: // Intel old (eg. HD graphics 3000) 
        C(_I);C(_n);C(_t);C(_e);C(_l);
        break;
        
    case 0x3ACF:  // Mali  (Samsung Exynoss, Mediatek and other ARM chipsets)
    case 0x7A00:  // Asus Tinkerboard (low precision result)
        C(_M);C(_a);C(_l);C(_i);
        break;
        
    case 0x3CCD:  // Adreno  ( Qualcomm ARM SoCs)
        C(_A);C(_d);C(_r);C(_e);C(_n);C(_o);
        break;
        
         // software rendering
    case 0x3659: // lavapipe
    case 0x3D59:  // llvmpipe
    case 0x2B7D:  //( MESA llvmpipe,  Chrome crashes, use Firefox)
        C(_s);C(_o);C(_f);C(_t);C(_w);C(_a);C(_r);C(_e);
        break;
        
    case 0x271F:  // VideoCore ( Raspberry PI )
        C(_V);C(_i);C(_d);C(_e);C(_o);C(_C);C(_o);C(_r);C(_e);
        break;

    default: // unknown, print hash
//        C(_u);C(_n);C(_k);C(_n);C(_o);C(_w);C(_n);C(_space);  // Some implementation will crash if shader is too long
        printHex((uint)(hash), &printCursor, &printColor, iChannel0);
    }
    
    //C(_n);C(_V);C(_i);C(_d);C(_i);C(_a);
    
    O = to_float4_s(printColor*0.8f);


  SetFragmentShaderComputedColor(O);
}