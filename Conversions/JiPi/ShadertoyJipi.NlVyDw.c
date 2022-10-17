
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// Shadertoy font shader - @P_Malin

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.

// ----------------------------------------------------------------------------------------

//#define LOW_QUALITY

// The main characters are made up from a number of curve segments.
// I made another shader to illustrate how these work:
//
//     https://www.shadertoy.com/view/Xds3Dn
//
// The middle of the characters are filled in triangles or convex quadrilaterals
// Enable this define to see just the curved sections:

//#define CURVES_ONLY

// Initially I made most of characters this way but I ran into the constant register limit. 
// To avoid this, the curved sections of the â€˜oâ€™, â€˜aâ€™ and â€˜dâ€™ are oval shapes. 
// Also I managed to cut the constant data down dramatically by sharing a lot of
// the shapes in the font (see the comments in the function Shadertoy() ). 
// For example the tails for â€˜hâ€™, â€™aâ€™, â€˜dâ€™, â€˜tâ€™, the left hand side of the â€˜yâ€™ and the 
// top of the â€˜hâ€™ all use the same shape! 
// I was probably more happy that I should have been when I realised I could share
// the shape making the curve of the â€˜râ€™ with the little loop on the â€˜oâ€™.
//
// I experimented with a distance field version but it looked like it would involve 
// a lot more work and I thought Iâ€™d already spent too much time on this shader :)

#ifdef LOW_QUALITY

  #define AA_X 1
  #define AA_Y 1

#else

  #define AA_X 2
  #define AA_Y 2

#endif


__DEVICE__ float TestCurve(float2 uv)
{
  uv = 1.0f - uv;
  return 1.0f - dot(uv, uv);
}

__DEVICE__ float Cross( const in float2 A, const in float2 B )
{
  return A.x * B.y - A.y * B.x;
}

__DEVICE__ float2 GetUV(const in float2 A, const in float2 B, const in float2 C, const in float2 P)
{
  float2 vPB = B - P;
  float f1 = Cross(A-B, vPB);
  float f2 = Cross(B-C, vPB);
  float f3 = Cross(C-A, C-P);
  
  return to_float2(f1, f2) / (f1 + f2 + f3);
}

__DEVICE__ float InCurve( const in float2 A, const in float2 B, const in float2 C, const in float2 P )
{
  float2 vCurveUV = GetUV(A, B, C, P);
  
  float fResult = -1.0f;

  fResult = _fmaxf(fResult, (-vCurveUV.x));
  fResult = _fmaxf(fResult, (-vCurveUV.y));
  fResult = _fmaxf(fResult, (vCurveUV.x + vCurveUV.y - 1.0f));

  float fCurveResult = TestCurve(vCurveUV);
    
  fResult = _fmaxf(fResult, fCurveResult);  
  
  return fResult;
}

__DEVICE__ float InCurve2( const in float2 A, const in float2 B, const in float2 C, const in float2 P )
{
  float2 vCurveUV = GetUV(A, B, C, P);

  float fResult = -1.0f;

  fResult = _fmaxf(fResult, (vCurveUV.x + vCurveUV.y - 1.0f));
  
  float fCurveResult = -TestCurve(vCurveUV);
  
  fResult = _fmaxf(fResult, fCurveResult);  
  
  return fResult;
}

__DEVICE__ float InTri( const in float2 A, const in float2 B, const in float2 C, const in float2 P )
{
  #ifdef CURVES_ONLY
  return 1.0f;
  #endif
  
  float f1 = Cross(B-A, A-P);
  float f2 = Cross(C-B, B-P);
  float f3 = Cross(A-C, C-P);

  return (_fmaxf(max(f1, f2), f3));
}

__DEVICE__ float InQuad( const in float2 A, const in float2 B, const in float2 C, const in float2 D, const in float2 P )
{
  #ifdef CURVES_ONLY
  return 1.0f;
  #endif
  
  float f1 = Cross(B-A, A-P);
  float f2 = Cross(C-B, B-P);
  float f3 = Cross(D-C, C-P);
  float f4 = Cross(A-D, D-P);
  
  return (_fmaxf(max(_fmaxf(f1, f2), f3), f4));
}


__DEVICE__ float Glyph0(const in float2 uv)
{
  const float2  vP0 = to_float2 ( 0.112f, 0.056f );
  const float2  vP1 = to_float2 ( 0.136f, 0.026f );
  const float2  vP2 = to_float2 ( 0.108f, 0.022f );
  const float2  vP3 = to_float2 ( 0.083f, 0.017f ); 
  const float2  vP4 = to_float2 ( 0.082f, 0.036f ); 
  const float2  vP5 = to_float2 ( 0.088f, 0.062f ); 
  const float2  vP6 = to_float2 ( 0.115f, 0.086f ); 
  const float2  vP7 = to_float2 ( 0.172f, 0.147f ); 
  const float2  vP8 = to_float2 ( 0.100f, 0.184f ); 
  const float2  vP9 = to_float2 ( 0.034f, 0.206f ); 
  const float2 vP10 = to_float2 ( 0.021f, 0.160f ); 
  const float2 vP11 = to_float2 ( 0.011f, 0.114f ); 
  const float2 vP12 = to_float2 ( 0.052f, 0.112f ); 
  const float2 vP13 = to_float2 ( 0.070f, 0.108f ); 
  const float2 vP14 = to_float2 ( 0.075f, 0.126f );
  const float2 vP15 = to_float2 ( 0.049f, 0.124f );
  const float2 vP16 = to_float2 ( 0.047f, 0.148f );
  const float2 vP17 = to_float2 ( 0.046f, 0.169f );
  const float2 vP18 = to_float2 ( 0.071f, 0.171f );
  const float2 vP19 = to_float2 ( 0.098f, 0.171f ); 
  const float2 vP20 = to_float2 ( 0.097f, 0.143f ); 
  const float2 vP21 = to_float2 ( 0.100f, 0.118f ); 
  const float2 vP22 = to_float2 ( 0.080f, 0.100f ); 
  const float2 vP23 = to_float2 ( 0.055f, 0.083f ); 
  const float2 vP24 = to_float2 ( 0.050f, 0.052f ); 
  const float2 vP25 = to_float2 ( 0.052f, 0.004f ); 
  const float2 vP26 = to_float2 ( 0.107f, 0.010f ); 
  const float2 vP27 = to_float2 ( 0.148f, 0.011f ); 
  const float2 vP28 = to_float2 ( 0.140f, 0.041f ); 
  const float2 vP29 = to_float2 ( 0.139f, 0.069f ); 

  float fDist = 1.0f;

  fDist = _fminf( fDist, InCurve2(vP6,vP7,vP8, uv) );
  fDist = _fminf( fDist, InCurve2(vP8,vP9,vP10, uv) );
  fDist = _fminf( fDist, InCurve2(vP10,vP11,vP12, uv) );
  fDist = _fminf( fDist, InCurve2(vP12,vP13,vP14, uv) );
  fDist = _fminf( fDist, InCurve(vP14,vP15,vP16, uv) );
  fDist = _fminf( fDist, InCurve(vP16,vP17,vP18, uv) );
  fDist = _fminf( fDist, InCurve(vP18,vP19,vP20, uv) );
  fDist = _fminf( fDist, InCurve(vP20,vP21,vP22, uv) );
  fDist = _fminf( fDist, InCurve2(vP22,vP23,vP24, uv) );
  fDist = _fminf( fDist, InCurve2(vP24,vP25,vP26, uv) );
  fDist = _fminf( fDist, InCurve2(vP26,vP27,vP28, uv) );
  fDist = _fminf( fDist, InCurve2(vP28,vP29,vP0, uv) );
  fDist = _fminf( fDist, InCurve(vP0,vP1,vP2, uv) );
  fDist = _fminf( fDist, InCurve(vP2,vP3,vP4, uv) );
  fDist = _fminf( fDist, InCurve(vP4,vP5,vP6, uv) );


  fDist = _fminf( fDist, InTri(vP0, vP1, vP28, uv) );
  fDist = _fminf( fDist, InQuad(vP26, vP1, vP2, vP3, uv) );
  fDist = _fminf( fDist, InTri(vP3, vP4, vP24, uv) );
  fDist = _fminf( fDist, InTri(vP4, vP5, vP24, uv) );
  fDist = _fminf( fDist, InTri(vP24, vP5, vP22, uv) );
  fDist = _fminf( fDist, InTri(vP5, vP6, vP22, uv) );
  fDist = _fminf( fDist, InTri(vP22, vP6, vP21, uv) );
  fDist = _fminf( fDist, InTri(vP6, vP8, vP21, uv) );
  fDist = _fminf( fDist, InTri(vP21, vP8, vP20, uv) );
  fDist = _fminf( fDist, InTri(vP20, vP8, vP19, uv) );
  fDist = _fminf( fDist, InTri(vP19, vP8, vP18, uv) );
  fDist = _fminf( fDist, InTri(vP18, vP8, vP10, uv) );
  fDist = _fminf( fDist, InTri(vP10, vP16, vP17, uv) );
  fDist = _fminf( fDist, InTri(vP10, vP15, vP16, uv) );
  fDist = _fminf( fDist, InTri(vP10, vP12, vP16, uv) );
  fDist = _fminf( fDist, InTri(vP12, vP14, vP15, uv) );

  return fDist;
}

__DEVICE__ float Glyph1(const in float2 uv, const in float2 vOffset)
{
  float2 vP0 = to_float2 ( 0.171f, 0.026f ) + vOffset;
  float2 vP1 = to_float2 ( 0.204f, 0.022f ) + vOffset;
  const float2 vP2 = to_float2 ( 0.170f, 0.185f );
  const float2 vP3 = to_float2 ( 0.137f, 0.185f );
  
  return InQuad(vP0, vP1, vP2, vP3, uv);
}

__DEVICE__ float Glyph3(const in float2 uv, float2 vOffset)
{
  float2 vP0 = to_float2 ( 0.212f, 0.112f ) + vOffset;
  float2 vP2 = to_float2 ( 0.243f, 0.112f ) + vOffset;
  const float2  vP4 = to_float2 ( 0.234f, 0.150f );
  const float2  vP5 = to_float2 ( 0.230f, 0.159f );
  const float2  vP6 = to_float2 ( 0.243f, 0.164f );
  const float2  vP7 = to_float2 ( 0.257f, 0.164f );
  const float2  vP8 = to_float2 ( 0.261f, 0.148f );
  const float2 vP10 = to_float2 ( 0.265f, 0.164f );
  const float2 vP11 = to_float2 ( 0.256f, 0.180f );
  const float2 vP12 = to_float2 ( 0.239f, 0.185f );
  const float2 vP13 = to_float2 ( 0.194f, 0.194f );
  const float2 vP14 = to_float2 ( 0.203f, 0.150f );
  const float2 vP16 = to_float2 ( 0.212f, 0.113f );

  float fDist = 1.0f;
  fDist = _fminf( fDist, InCurve(vP4,vP5,vP6, uv) );
  fDist = _fminf( fDist, InCurve(vP6,vP7,vP8, uv) );
  fDist = _fminf( fDist, InCurve2(vP10,vP11,vP12, uv) );
  fDist = _fminf( fDist, InCurve2(vP12,vP13,vP14, uv) );

  fDist = _fminf( fDist, InQuad(vP0, vP2, vP4, vP14, uv) );
  fDist = _fminf( fDist, InTri(vP14, vP4, vP5, uv) );
  fDist = _fminf( fDist, InTri(vP14, vP5, vP12, uv) );
  fDist = _fminf( fDist, InTri(vP5, vP6, vP12, uv) );
  fDist = _fminf( fDist, InTri(vP6, vP7, vP12, uv) );
  fDist = _fminf( fDist, InTri(vP6, vP10, vP12, uv) );
  fDist = _fminf( fDist, InTri(vP8, vP10, vP7, uv) );
  
  return fDist;
}

__DEVICE__ float Glyph4(const in float2 uv)
{
    float2 vP = uv - to_float2(0.305f, 0.125f);
    vP /= 0.065f;
    vP.x *= 1.5f;
    vP.x += vP.y * 0.25f;
    
    float2 vP2 = vP;

    vP.y = _fabs(vP.y);
    vP.y = _powf(vP.y, 1.2f);
    float f= length(vP);
    
    vP2.x *= 1.2f;
    float f2 = length(vP2 * 1.5f - to_float2(0.6f, 0.0f));
        
    return _fmaxf(f - 1.0f, 1.0f - f2) / 20.0f;
} 

__DEVICE__ float Glyph5(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.507f, 0.138f );
    const float2  vP1 = to_float2 ( 0.510f, 0.065f );
    const float2  vP2 = to_float2 ( 0.570f, 0.066f );
    const float2  vP3 = to_float2 ( 0.598f, 0.066f );
    const float2  vP4 = to_float2 ( 0.594f, 0.092f );
    const float2  vP5 = to_float2 ( 0.599f, 0.131f );
    const float2  vP6 = to_float2 ( 0.537f, 0.137f );
    const float2  vP8 = to_float2 ( 0.538f, 0.125f );
    const float2  vP9 = to_float2 ( 0.564f, 0.129f );
    const float2 vP10 = to_float2 ( 0.574f, 0.100f );
    const float2 vP11 = to_float2 ( 0.584f, 0.085f );
    const float2 vP12 = to_float2 ( 0.571f, 0.079f );
    const float2 vP13 = to_float2 ( 0.557f, 0.081f );
    const float2 vP14 = to_float2 ( 0.549f, 0.103f );
    const float2 vP15 = to_float2 ( 0.518f, 0.166f );
    const float2 vP16 = to_float2 ( 0.557f, 0.166f );
    const float2 vP17 = to_float2 ( 0.589f, 0.163f );
    const float2 vP18 = to_float2 ( 0.602f, 0.137f );
    const float2 vP20 = to_float2 ( 0.602f, 0.152f );
    const float2 vP21 = to_float2 ( 0.572f, 0.194f );
    const float2 vP22 = to_float2 ( 0.537f, 0.185f );
    const float2 vP23 = to_float2 ( 0.503f, 0.189f );
    
    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = _fminf( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = _fminf( fDist, InCurve(vP10,vP11,vP12, uv) ); 
    fDist = _fminf( fDist, InCurve(vP12,vP13,vP14, uv) );
    fDist = _fminf( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = _fminf( fDist, InCurve(vP16,vP17,vP18, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP20,vP21,vP22, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP22,vP23,vP0, uv) );

    fDist = _fminf( fDist, InTri(vP0, vP2, vP13, uv) );
    fDist = _fminf( fDist, InTri(vP13, vP2, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP2, vP11, vP12, uv) );
    fDist = _fminf( fDist, InTri(vP2, vP4, vP11, uv) );
    fDist = _fminf( fDist, InTri(vP11, vP4, vP10, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP4, vP9, uv) );
    fDist = _fminf( fDist, InTri(vP6, vP8, vP9, uv) );
    fDist = _fminf( fDist, InTri(vP0, vP13, vP14, uv) );
    fDist = _fminf( fDist, InTri(vP0, vP14, vP15, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP16, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP16, vP17, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP17, vP18, vP20, uv) );
    
    return fDist;
}

__DEVICE__ float Glyph6(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.638f , 0.087f ); 
    const float2  vP1 = to_float2 ( 0.648f , 0.073f ); 
    const float2  vP2 = to_float2 ( 0.673f , 0.068f ); 
    const float2  vP3 = to_float2 ( 0.692f , 0.069f ); 
    const float2  vP4 = to_float2 ( 0.687f , 0.086f ); 
    const float2  vP5 = to_float2 ( 0.688f , 0.104f ); 
    const float2  vP6 = to_float2 ( 0.672f , 0.102f ); 
    const float2  vP7 = to_float2 ( 0.659f , 0.099f ); 
    const float2  vP8 = to_float2 ( 0.663f , 0.092f ); 
    const float2  vP9 = to_float2 ( 0.662f , 0.086f ); 
    const float2 vP10 = to_float2 ( 0.655f , 0.086f ); 
    const float2 vP11 = to_float2 ( 0.644f , 0.087f ); 
    const float2 vP12 = to_float2 ( 0.637f , 0.102f ); 
    const float2 vP13 = to_float2 ( 0.638f , 0.094f ); 

    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP4,vP5,vP6, uv) ); 
    fDist = _fminf( fDist, InCurve2(vP6,vP7,vP8, uv) ); 
    fDist = _fminf( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = _fminf( fDist, InCurve(vP10,vP11,vP12, uv) );

    fDist = _fminf( fDist, InQuad(vP2, vP4, vP6, vP8, uv) );
    fDist = _fminf( fDist, InTri(vP9, vP2, vP8, uv) );
    fDist = _fminf( fDist, InTri(vP10, vP2, vP9, uv) );
    fDist = _fminf( fDist, InQuad(vP0, vP2, vP10, vP11, uv) );
    fDist = _fminf( fDist, InTri(vP11, vP12, vP0, uv) );
    
    return fDist;
}

__DEVICE__ float Glyph7(const in float2 uv)
{
    const float2 vP0 = to_float2 ( 0.693f , 0.068f );
    const float2 vP1 = to_float2 ( 0.748f , 0.069f );
    const float2 vP2 = to_float2 ( 0.747f , 0.078f );
    const float2 vP3 = to_float2 ( 0.691f , 0.077f );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}


__DEVICE__ float Glyph8(const in float2 uv)
{ 
    float2 vP = uv - to_float2(0.788f, 0.125f);
    vP /= 0.065f;
    vP.x *= 1.4f;
    vP.x += vP.y * 0.25f;
    
    float2 vP2 = vP;
    
    vP.y = _fabs(vP.y);
    vP.y = _powf(vP.y, 1.2f);
    float f= length(vP);
    
    vP2.x *= 1.5f;
    float f2 = length(vP2 * 1.5f - to_float2(0.3f, 0.0f));
    
    
    return _fmaxf(f - 1.0f, 1.0f - f2) / 20.0f;
}

__DEVICE__ float Glyph11(const in float2 uv)
{
    const float2  vP0 = to_float2 ( 0.921f , 0.070f );
    const float2  vP2 = to_float2 ( 0.955f , 0.070f );
    const float2  vP4 = to_float2 ( 0.926f , 0.202f );
    const float2  vP5 = to_float2 ( 0.926f , 0.240f );
    const float2  vP6 = to_float2 ( 0.885f , 0.243f );
    const float2  vP7 = to_float2 ( 0.852f , 0.239f );
    const float2  vP8 = to_float2 ( 0.859f , 0.219f );
    const float2  vP9 = to_float2 ( 0.862f , 0.192f );
    const float2 vP10 = to_float2 ( 0.889f , 0.189f );
    const float2 vP12 = to_float2 ( 0.928f , 0.178f );
    const float2 vP13 = to_float2 ( 0.949f , 0.173f );
    const float2 vP14 = to_float2 ( 0.951f , 0.162f );
    const float2 vP15 = to_float2 ( 0.960f , 0.150f );
    const float2 vP16 = to_float2 ( 0.960f , 0.144f );
    const float2 vP18 = to_float2 ( 0.971f , 0.144f );
    const float2 vP19 = to_float2 ( 0.968f , 0.157f );
    const float2 vP20 = to_float2 ( 0.957f , 0.171f );
    const float2 vP21 = to_float2 ( 0.949f , 0.182f );
    const float2 vP22 = to_float2 ( 0.922f , 0.189f );
    const float2 vP24 = to_float2 ( 0.900f , 0.196f );
    const float2 vP25 = to_float2 ( 0.866f , 0.205f );
    const float2 vP26 = to_float2 ( 0.871f , 0.217f );
    const float2 vP27 = to_float2 ( 0.871f , 0.225f );
    const float2 vP28 = to_float2 ( 0.880f , 0.224f );
    const float2 vP29 = to_float2 ( 0.889f , 0.218f );
    const float2 vP30 = to_float2 ( 0.893f , 0.203f );

    float fDist = 1.0f;
    fDist = _fminf( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = _fminf( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = _fminf( fDist, InCurve2(vP8,vP9,vP10, uv) );
    fDist = _fminf( fDist, InCurve(vP12,vP13,vP14, uv) );

    fDist = _fminf( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = _fminf( fDist, InCurve2(vP18,vP19,vP20, uv) );
    fDist = _fminf( fDist, InCurve2(vP20,vP21,vP22, uv) );

    fDist = _fminf( fDist, InCurve(vP24,vP25,vP26, uv) );
    fDist = _fminf( fDist, InCurve(vP26,vP27,vP28, uv) );
    fDist = _fminf( fDist, InCurve(vP28,vP29,vP30, uv) );
    
    fDist = _fminf( fDist, InQuad(vP0, vP2, vP4, vP30, uv) );

    fDist = _fminf( fDist, InQuad(vP10, vP12, vP22, vP24, uv) );
        
    fDist = _fminf( fDist, InTri(vP30, vP4, vP6, uv) );
    fDist = _fminf( fDist, InTri(vP30, vP6, vP29, uv) );
    fDist = _fminf( fDist, InTri(vP28, vP29, vP6, uv) );
    fDist = _fminf( fDist, InTri(vP28, vP6, vP27, uv) );
    
    fDist = _fminf( fDist, InTri(vP8, vP27, vP6, uv) );
    
    fDist = _fminf( fDist, InTri(vP8, vP26, vP27, uv) );
    fDist = _fminf( fDist, InTri(vP8, vP25, vP26, uv) );
    fDist = _fminf( fDist, InTri(vP25, vP10, vP24, uv) );
    
    fDist = _fminf( fDist, InTri(vP12, vP13, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP12, vP20, vP22, uv) );
    fDist = _fminf( fDist, InTri(vP13, vP14, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP20, vP14, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP18, vP20, uv) );
    fDist = _fminf( fDist, InTri(vP15, vP16, vP18, uv) );
    
    return fDist;
}

__DEVICE__ float Shadertoy(in float2 uv)
{
    float fResult = 1.0f;
    
    fResult = _fminf(fResult, Glyph0(uv)); // S

    float2 vUVOffset = to_float2(0.001f, 0.0f); // tail of h
    float2 vTailOffset = to_float2(0.0f, 0.0f);  
    float fUVScale = 1.0f;

    if(uv.x < 0.3f)
    {
        if(uv.y < 0.12f)
        {
            // top of h
            fUVScale = -1.0f;
            vUVOffset = to_float2(0.448f, 0.25f);  
            vTailOffset = to_float2(0.0f, 0.0f);   
        }
    }
    else if(uv.x < 0.4f)    
    {
        // tail of a
        vUVOffset = to_float2(-0.124f, 0.0f);  
        vTailOffset = to_float2(0.01f, -0.04f);    
    }
    else if(uv.x < 0.6f)
    {
        // tail of d
        vUVOffset = to_float2(-0.248f, 0.0f);  
        vTailOffset = to_float2(0.02f, -0.1f); 
    }
    else if(uv.x < 0.83f)
    {
        // stalk of t
        vUVOffset = to_float2(-0.48f, 0.0f);   
        vTailOffset = to_float2(0.02f, -0.1f); 
    }
    else
    {
        // start of y
        vUVOffset = to_float2(-0.645f, 0.0f);  
        vTailOffset = to_float2(0.005f, -0.042f);  
    }
    
    fResult = _fminf(fResult, Glyph3(uv * fUVScale + vUVOffset, vTailOffset)); // tails h, a, d, t, start of y and top of h


    float2 vUVOffset3 = to_float2(0.0f, 0.0f);   // vertical of h
    float2 vTailOffset3 = to_float2(0.0f, 0.0f);
    
    if(uv.x > 0.5f)
    {
        // vertical of r
        vUVOffset3 = to_float2(-0.45f, 0.0f);  
        vTailOffset3 = to_float2(-0.01f, 0.04f);   
    }
    
    fResult = _fminf(fResult, Glyph1(uv + vUVOffset3, vTailOffset3)); // vertical of h, r

    float2 vUVOffset2 = to_float2(0.0f, 0.0f); // curve of a
    if(uv.x > 0.365f)
    {
        vUVOffset2 = to_float2(-0.125f, 0.0f); // curve of d
    }

    fResult = _fminf(fResult, Glyph4(uv + vUVOffset2)); // curve of a, d
    
    fResult = _fminf(fResult, Glyph5(uv)); // e

    float2 vUVOffset4 = to_float2(0.001f, 0.0f); // top of r
    float2 vUVScale4 = to_float2(1.0f, 1.0f);        
    
    if(uv.x > 0.7f)
    {
        // o loop
        vUVOffset4.x = 1.499f;
        vUVOffset4.y = 0.19f;
        
        vUVScale4.x = -1.0f;
        vUVScale4.y = -1.0f;
    }
    
    fResult = _fminf(fResult, Glyph6(uv * vUVScale4 + vUVOffset4)); // top of r and o loop

    fResult = _fminf(fResult, Glyph7(uv)); // cross t    
    
    fResult = _fminf(fResult, Glyph8(uv)); // o1
    
    fResult = _fminf(fResult, Glyph11(uv)); // y2        

    return fResult; 
}

__DEVICE__ float2 GetUVCentre(const float2 vInputUV)
{
  float2 vFontUV = vInputUV;
  vFontUV.y -= 0.35f;
    
  return vFontUV;
}

__DEVICE__ float2 GetUVScroll(const float2 vInputUV, float t)
{
  float2 vFontUV = vInputUV;
  vFontUV *= 0.25f;
  
  vFontUV.y -= 0.005f;
  vFontUV.x += t * 3.0f - 1.5f;
  
  return vFontUV;
}

__DEVICE__ float2 GetUVRepeat(const float2 vInputUV, float t2)
{
  float2 vFontUV = vInputUV;
  
  vFontUV *= to_float2(1.0f, 4.0f);
  
  vFontUV.x += _floor(vFontUV.y) * t2;
  
  vFontUV = fract(vFontUV);
  
  vFontUV /= to_float2(1.0f, 4.0f);
    
  return vFontUV;
}

__DEVICE__ float2 GetUVRotate(const float2 vInputUV, float t)
{
  float2 vFontUV = vInputUV - 0.5f;
  
  float s = _sinf(t);
  float c = _cosf(t);
  
  vFontUV = to_float2(  vFontUV.x * c + vFontUV.y * s,
                       -vFontUV.x * s + vFontUV.y * c );
  
  vFontUV += 0.5f;
  
  return vFontUV;
}

__DEVICE__ float3 StyleDefault( float f )
{
  return _mix(to_float3_s(0.25f), to_float3_s(1.0f), f);
}

__DEVICE__ float3 StyleScanline( float f, in float2 fragCoord )
{
  float fShade = f * 0.8f + 0.2f;
  
  fShade *= mod_f(fragCoord.y, 2.0f);
  
  return _mix(to_float3(0.01f, 0.2f, 0.01f), to_float3(0.01f, 1.0f, 0.02f), fShade);
}

__DEVICE__ float3 StyleStamp( float fFont, float2 uv, __TEXTURE2D__ iChannel0 )
{
  float3 t1 = swi3(_tex2DVecN(iChannel0, uv.x + 0.005f, uv.y + 0.005f, 15),x,y,z);//.rgb;
  float3 t2 = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
  float dt = clamp(0.5f + (t1.x - t2.x), 0.0f, 1.0f);
  float fWear = clamp((0.9f - t2.x) * 4.0f, 0.0f, 1.0f);
  float f =  clamp(fFont * fWear, 0.0f, 1.0f);
  return _mix( to_float3(1.0f, 0.98f, 0.9f) * (dt * 0.1f + 0.9f), to_float3(0.7f, 0.0f, 0.0f), f);
}

__DEVICE__ float3 StyleWood( float fFont, float2 uv, __TEXTURE2D__ iChannel0  )
{
  float3 t = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);//.rgb;
  float fWear = fFont * smoothstep(0.0f, 0.4f, t.z);
  return _mix(t, to_float3_s(0.0f), fWear);
}

__DEVICE__ float4 GetRandom4(float x)
{
  return fract_f4(to_float4(987.65f, 432.10f, 765.43f, 210.98f) * sin_f4(to_float4(123.456f, 789.123f, 456.789f, 567.890f) * x));
}

__KERNEL__ void ShadertoyJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{
  
  CONNECT_SLIDER0(fSequenceLength, -1.0f, 10.0f, 5.0f);
  CONNECT_SLIDER1(fBlendSpeed, -1.0f, 1.0f, 0.05f);
  
  CONNECT_SLIDER2(M_fUVEffect, 0.0f, 3.0f, 3.0f);
  CONNECT_SLIDER3(M_fScreenEffect, 0.0f, 3.0f, 3.0f);
  CONNECT_SLIDER4(M_fFract, -1.0f, 10.0f, 0.5f);
  CONNECT_SLIDER5(M_Zoom, -1.0f, 10.0f, 0.25f);
  
  //float fSequenceLength = 5.0f;
  
  float fTime = iTime;
  
  //float fBlendSpeed = 0.05f;
  
  // Skip the initial fade-in
  fTime += fBlendSpeed * fSequenceLength;
  
  float fInt = _floor(fTime / fSequenceLength);
  float fFract = fract(fTime / fSequenceLength);
  
  float4 vRandom4 = GetRandom4(fInt);
  float2 vRandom2 = _floor(swi2(vRandom4,x,y) * to_float2(1234.56f, 123.45f));
  
  float fUVEffect = mod_f(vRandom2.x, 4.0f);
  float fScreenEffect = mod_f(vRandom2.y, 4.0f);

  if(fInt < 0.5f)
  {
    fUVEffect = 0.0f;
    fScreenEffect = 0.0f;
  }

  float4 vResult = to_float4_s(0.0f);
    
  float fX = 0.0f;
  for(int iX=0; iX<AA_X; iX++)
  {
    float fY = 0.0f;
    for(int y=0; y<AA_Y; y++)
    {
  
      float2 vUV = (fragCoord + to_float2(fX, fY)) / iResolution;
      vUV.x = ((vUV.x - 0.5f) * (iResolution.x / iResolution.y)) + 0.5f;    
      vUV.y = 1.0f - vUV.y;
        
      float2 vFontUV = vUV;
      float2 vBgUV = vUV;
      
      if(iMouse.z > 0.0f)
      {
        fUVEffect = M_fUVEffect;//999.0f;
        fScreenEffect = M_fScreenEffect;//0.0f;
        fFract = M_fFract;//0.5f;
        
        vFontUV *= M_Zoom;//0.25f;
        vFontUV += swi2(iMouse,x,y) / iResolution;
        vFontUV.y -= 0.5f;
        vBgUV = vFontUV;
      }  
      
      if(fUVEffect < 0.5f)
      {
        vFontUV = GetUVCentre(vBgUV);
      }
      else
      if(fUVEffect < 1.5f)
      {
        vBgUV = GetUVScroll(vBgUV, fFract);
        vFontUV = vBgUV;
      }
      else
      if(fUVEffect < 2.5f)
      {
        float fSpeed = 0.1f + vRandom4.z;
        vBgUV.x += fFract * fSpeed;
        vFontUV = GetUVRepeat(vBgUV, 0.25f);
      }
      else
      if(fUVEffect < 3.5f)
      {
        float fSpeed = 1.0f + vRandom4.z * 2.0f;
        if(vRandom4.w > 0.5f)
        {
          fSpeed = -fSpeed;
        }
        vBgUV = GetUVRotate(vBgUV, 1.0f + fSpeed * fFract);
        vFontUV = GetUVRepeat(vBgUV, 0.0f);
      }
      
      float fShadertoy = step(Shadertoy(vFontUV), 0.0f);
        
      if(fScreenEffect < 0.5f)
      {
        vResult += to_float4_aw(StyleDefault(fShadertoy), 1.0f);
      }
      else if(fScreenEffect < 1.5f)
      {
        vResult += to_float4_aw(StyleScanline(fShadertoy, fragCoord), 1.0f);
      }
      else if(fScreenEffect < 2.5f)
      {
        vResult += to_float4_aw(StyleStamp(fShadertoy, vBgUV, iChannel0), 1.0f);
      }
      else
      {
        vResult += to_float4_aw(StyleWood(fShadertoy, vBgUV, iChannel0), 1.0f);
      }

      fY += 1.0f / (float)(AA_Y);
    }
    
    fX += 1.0f / (float)(AA_X);
  }
  
  swi3S(vResult,x,y,z, swi3(vResult,x,y,z) / vResult.w);

  float fFade = 0.0f;  
  if(fFract > (1.0f - fBlendSpeed))
  {
    fFade = smoothstep(1.0f - fBlendSpeed, 1.0f, fFract);
  }

  if(fFract < fBlendSpeed)
  {
    fFade = smoothstep(fBlendSpeed, 0.0f, fFract);
  }

  vResult = _mix(vResult, to_float4_s(1.0f), fFade);
  
  fragColor = to_float4_aw(swi3(vResult,x,y,z), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}