
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Utilities
    // macros
#define RES iResolution
#define P fragCoord
#define UV (fragCoord/iResolution)
#define MUV (iMouse.xy/iResolution)

#define pF(c) texelFetch(c, to_int2(P), 0)
#define sF(c) _tex2DVecN(c,UV.x,UV.y,15)

    // screen
__DEVICE__ float2 wrap(in float2 p, in float2 res) {
    if (p.x > res.x) p.x = mod_f(p.x, res.x);
    else if (p.x < 0.0f) p.x = res.x + p.x;
    
    if (p.y > res.y) p.y = mod_f(p.y, res.y);
    else if (p.y < 0.0f) p.y = res.y + p.y;
    
    return p;
}

#define SCALE(v, mx, a, b) (a + (v * (b - a) / mx))
__DEVICE__ float2 scale(float2 mn, float2 mx, mat2 bounds) {
  //return to_float2(SCALE(mn.x, mx.x, bounds[0][0], bounds[0][1]),
  //                 SCALE(mn.y, mx.y, bounds[1][0], bounds[1][1]));
  return to_float2(SCALE(mn.x, mx.x, bounds.r0.x, bounds.r0.y),
                   SCALE(mn.y, mx.y, bounds.r1.x, bounds.r1.y));
}

// math
//Generic 3x3 filter - to_float3(center, edges, diagonals)
#define GAUSSIAN to_float3(0.204f, 0.124f, 0.075f)
#define LAPLACIAN to_float3(-1.0f, 0.2f, 0.05f)
__DEVICE__ float4 filter3x3(in float2 pos, in float3 kernel, in __TEXTURE2D__ channel, in float2 reso) {
    float4 sum = to_float4_s(0.0f);
    
    for(int i=-1; i<=1; i++) {
        for(int j=-1; j<=1; j++) {
            float weight = (i==0 && j==0) ? kernel.x : (_fabs(i-j) == 1 ? kernel.y : kernel.z);
            
            //sum += weight * texelFetch(channel, to_int2(wrap(pos + to_float2(i, j), reso)), 0);
            sum += weight * texture(channel, (make_float2(to_int2_cfloat(wrap(pos + make_float2(i, j), reso)))+0.5f)/reso);
        }
    }
    return sum;
}


    // Sobel
//#define SOBEL_EDGE_COLOR to_float4(0.753f,0.380f,0.796f,1.0f)
__DEVICE__ float4 sobel(in float2 pos, in __TEXTURE2D__ channel, in float2 reso, float4 SOBEL_EDGE_COLOR) {
    // 
    mat3 SX = to_mat3( 1.0f,  2.0f,  1.0f, 
                       0.0f,  0.0f,  0.0f, 
                      -1.0f, -2.0f, -1.0f);
    mat3 SY = to_mat3(1.0f, 0.0f, -1.0f, 
                      2.0f, 0.0f, -2.0f, 
                      1.0f, 0.0f, -1.0f);

    //float4 T = texelFetch(channel, to_int2(pos), 0);
    float4 T = texture(channel, (make_float2(to_int2_cfloat(pos))+0.5f)/reso);
float zzzzzzzzzzzzzzz;
    //mat3 M = to_mat3_f(0.0f);
    float M[3][3];
    for(int i=0; i<3; i++) {
        for(int j=0; j<3; j++) {
            //float4 A = texelFetch(channel, to_int2(pos + to_float2(i-1, j-1)), 0);
            float4 A = texture(channel, (make_float2(to_int2_cfloat(pos + make_float2(i-1, j-1)))+0.5f)/reso);
            M[i][j] = length(A);
        }
    }
    
    //float gx = dot(SX[0], M[0]) + dot(SX[1], M[1]) + dot(SX[2], M[2]);
    //float gy = dot(SY[0], M[0]) + dot(SY[1], M[1]) + dot(SY[2], M[2]);
    float gx = dot(SX.r0, to_float3(M[0][0],M[0][1],M[0][2])) + dot(SX.r1, to_float3(M[1][0],M[1][1],M[1][2])) + dot(SX.r2, to_float3(M[2][0],M[2][1],M[2][2]));
    float gy = dot(SY.r0, to_float3(M[0][0],M[0][1],M[0][2])) + dot(SY.r1, to_float3(M[1][0],M[1][1],M[1][2])) + dot(SY.r2, to_float3(M[2][0],M[2][1],M[2][2]));
    
    
    
    // TODO factor into float sobel() and move this to a buffer pass.
    float g = _sqrtf(gx*gx + gy*gy);
    g = smoothstep(0.15f, 0.98f, g);

    return _mix(T, SOBEL_EDGE_COLOR, g);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A '/media/a/c3a071ecf273428bc72fc72b2dd972671de8da420a2d4f917b75d20e1c24b34c.ogv' to iChannel0


__KERNEL__ void SobelFilterTestJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(TexAnim, 1);
  
    fragCoord+=0.5f;

    //fragColor = _tex2DVecN(iChannel0,UV.x,UV.y,15);
    fragColor = texture(iChannel0, UV - 0.05f*_cosf(iTime)*_sinf(iTime) * TexAnim);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void SobelFilterTestJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER0(Blurred, -1.0f, 2.0f, 0.3333f);
   
    fragCoord+=0.5f;

    float4 blurred = filter3x3(P, GAUSSIAN, iChannel0, RES); 
    fragColor = to_float4_s((blurred.x + blurred.y + blurred.z) * Blurred); //0.3333f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel1


__KERNEL__ void SobelFilterTestJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(SOBEL_EDGE_COLOR, 0.753f,0.380f,0.796f,1.0f);

    fragCoord+=0.5f;

    // Only want the sobel from this source
    float4 T = _tex2DVecN(iChannel1,UV.x,UV.y,15);//* filter3x3(P, GAUSSIAN, iChannel0, RES)) * 0.25f;
    float4 sob = sobel(P, iChannel0, RES, SOBEL_EDGE_COLOR);
    
    fragColor = sob*sob;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer C' to iChannel1


#define STRENGTH 100.0f

__KERNEL__ void SobelFilterTestJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1)
{
  CONNECT_SLIDER1(FinalAlpha, 0.0f, 1.0f, 1.0f);
  CONNECT_SLIDER2(TexAlpha, 0.0f, 1.0f, 0.9f);
  CONNECT_CHECKBOX1(Textur, 0);
  
  fragColor = sF(iChannel1);

  if (Textur)
    if (fragColor.w < TexAlpha)
      fragColor = sF(iChannel0);

  fragColor.w = FinalAlpha;

  SetFragmentShaderComputedColor(fragColor);
}