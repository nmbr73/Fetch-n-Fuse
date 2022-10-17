

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Shadertoy font shader - @P_Malin

// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

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


float TestCurve(vec2 uv)
{
	uv = 1.0 - uv;
    return 1.0 - dot(uv, uv);
}

float Cross( const in vec2 A, const in vec2 B )
{
    return A.x * B.y - A.y * B.x;
}

vec2 GetUV(const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P)
{
    vec2 vPB = B - P;
    float f1 = Cross(A-B, vPB);
    float f2 = Cross(B-C, vPB);
    float f3 = Cross(C-A, C-P);
    
    return vec2(f1, f2) / (f1 + f2 + f3);
}

float InCurve( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P )
{
    vec2 vCurveUV = GetUV(A, B, C, P);
    
    float fResult = -1.0;

	fResult = max(fResult, (-vCurveUV.x));
	fResult = max(fResult, (-vCurveUV.y));
	fResult = max(fResult, (vCurveUV.x + vCurveUV.y - 1.0));

	float fCurveResult = TestCurve(vCurveUV);
		
	fResult = max(fResult, fCurveResult);	
	
    return fResult;
}

float InCurve2( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P )
{
    vec2 vCurveUV = GetUV(A, B, C, P);
	
    float fResult = -1.0;

	fResult = max(fResult, (vCurveUV.x + vCurveUV.y - 1.0));
	
	float fCurveResult = -TestCurve(vCurveUV);
	
	fResult = max(fResult, fCurveResult);	
	
    return fResult;
}

float InTri( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 P )
{
	#ifdef CURVES_ONLY
	return 1.0;
	#endif
	
    float f1 = Cross(B-A, A-P);
    float f2 = Cross(C-B, B-P);
    float f3 = Cross(A-C, C-P);
	
    return (max(max(f1, f2), f3));
}

float InQuad( const in vec2 A, const in vec2 B, const in vec2 C, const in vec2 D, const in vec2 P )
{
	#ifdef CURVES_ONLY
	return 1.0;
	#endif
	
    float f1 = Cross(B-A, A-P);
    float f2 = Cross(C-B, B-P);
    float f3 = Cross(D-C, C-P);
    float f4 = Cross(A-D, D-P);
    
    return (max(max(max(f1, f2), f3), f4));
}


float Glyph0(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.112, 0.056 );
    const vec2  vP1 = vec2 ( 0.136, 0.026 );
    const vec2  vP2 = vec2 ( 0.108, 0.022 );
    const vec2  vP3 = vec2 ( 0.083, 0.017 ); 
    const vec2  vP4 = vec2 ( 0.082, 0.036 ); 
    const vec2  vP5 = vec2 ( 0.088, 0.062 ); 
    const vec2  vP6 = vec2 ( 0.115, 0.086 ); 
    const vec2  vP7 = vec2 ( 0.172, 0.147 ); 
    const vec2  vP8 = vec2 ( 0.100, 0.184 ); 
    const vec2  vP9 = vec2 ( 0.034, 0.206 ); 
    const vec2 vP10 = vec2 ( 0.021, 0.160 ); 
    const vec2 vP11 = vec2 ( 0.011, 0.114 ); 
    const vec2 vP12 = vec2 ( 0.052, 0.112 ); 
    const vec2 vP13 = vec2 ( 0.070, 0.108 ); 
    const vec2 vP14 = vec2 ( 0.075, 0.126 );
    const vec2 vP15 = vec2 ( 0.049, 0.124 );
    const vec2 vP16 = vec2 ( 0.047, 0.148 );
    const vec2 vP17 = vec2 ( 0.046, 0.169 );
    const vec2 vP18 = vec2 ( 0.071, 0.171 );
    const vec2 vP19 = vec2 ( 0.098, 0.171 ); 
    const vec2 vP20 = vec2 ( 0.097, 0.143 ); 
    const vec2 vP21 = vec2 ( 0.100, 0.118 ); 
    const vec2 vP22 = vec2 ( 0.080, 0.100 ); 
    const vec2 vP23 = vec2 ( 0.055, 0.083 ); 
    const vec2 vP24 = vec2 ( 0.050, 0.052 ); 
    const vec2 vP25 = vec2 ( 0.052, 0.004 ); 
    const vec2 vP26 = vec2 ( 0.107, 0.010 ); 
    const vec2 vP27 = vec2 ( 0.148, 0.011 ); 
    const vec2 vP28 = vec2 ( 0.140, 0.041 ); 
    const vec2 vP29 = vec2 ( 0.139, 0.069 ); 

    float fDist = 1.0;

	fDist = min( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = min( fDist, InCurve2(vP8,vP9,vP10, uv) );
	fDist = min( fDist, InCurve2(vP10,vP11,vP12, uv) );
    fDist = min( fDist, InCurve2(vP12,vP13,vP14, uv) );
	fDist = min( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = min( fDist, InCurve(vP16,vP17,vP18, uv) );
    fDist = min( fDist, InCurve(vP18,vP19,vP20, uv) );
    fDist = min( fDist, InCurve(vP20,vP21,vP22, uv) );
	fDist = min( fDist, InCurve2(vP22,vP23,vP24, uv) );
    fDist = min( fDist, InCurve2(vP24,vP25,vP26, uv) );
    fDist = min( fDist, InCurve2(vP26,vP27,vP28, uv) );
    fDist = min( fDist, InCurve2(vP28,vP29,vP0, uv) );
	fDist = min( fDist, InCurve(vP0,vP1,vP2, uv) );
	fDist = min( fDist, InCurve(vP2,vP3,vP4, uv) );
    fDist = min( fDist, InCurve(vP4,vP5,vP6, uv) );


    fDist = min( fDist, InTri(vP0, vP1, vP28, uv) );
	fDist = min( fDist, InQuad(vP26, vP1, vP2, vP3, uv) );
    fDist = min( fDist, InTri(vP3, vP4, vP24, uv) );
    fDist = min( fDist, InTri(vP4, vP5, vP24, uv) );
    fDist = min( fDist, InTri(vP24, vP5, vP22, uv) );
    fDist = min( fDist, InTri(vP5, vP6, vP22, uv) );
    fDist = min( fDist, InTri(vP22, vP6, vP21, uv) );
    fDist = min( fDist, InTri(vP6, vP8, vP21, uv) );
    fDist = min( fDist, InTri(vP21, vP8, vP20, uv) );
    fDist = min( fDist, InTri(vP20, vP8, vP19, uv) );
    fDist = min( fDist, InTri(vP19, vP8, vP18, uv) );
    fDist = min( fDist, InTri(vP18, vP8, vP10, uv) );
    fDist = min( fDist, InTri(vP10, vP16, vP17, uv) );
    fDist = min( fDist, InTri(vP10, vP15, vP16, uv) );
    fDist = min( fDist, InTri(vP10, vP12, vP16, uv) );
    fDist = min( fDist, InTri(vP12, vP14, vP15, uv) );

    return fDist;
}

float Glyph1(const in vec2 uv, const in vec2 vOffset)
{
    vec2 vP0 = vec2 ( 0.171, 0.026 ) + vOffset;
    vec2 vP1 = vec2 ( 0.204, 0.022 ) + vOffset;
    const vec2 vP2 = vec2 ( 0.170, 0.185 );
    const vec2 vP3 = vec2 ( 0.137, 0.185 );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}

float Glyph3(const in vec2 uv, vec2 vOffset)
{
    vec2 vP0 = vec2 ( 0.212, 0.112 ) + vOffset;
    vec2 vP2 = vec2 ( 0.243, 0.112 ) + vOffset;
    const vec2  vP4 = vec2 ( 0.234, 0.150 );
    const vec2  vP5 = vec2 ( 0.230, 0.159 );
    const vec2  vP6 = vec2 ( 0.243, 0.164 );
    const vec2  vP7 = vec2 ( 0.257, 0.164 );
    const vec2  vP8 = vec2 ( 0.261, 0.148 );
    const vec2 vP10 = vec2 ( 0.265, 0.164 );
    const vec2 vP11 = vec2 ( 0.256, 0.180 );
    const vec2 vP12 = vec2 ( 0.239, 0.185 );
    const vec2 vP13 = vec2 ( 0.194, 0.194 );
    const vec2 vP14 = vec2 ( 0.203, 0.150 );
    const vec2 vP16 = vec2 ( 0.212, 0.113 );

    float fDist = 1.0;
    fDist = min( fDist, InCurve(vP4,vP5,vP6, uv) );
    fDist = min( fDist, InCurve(vP6,vP7,vP8, uv) );
    fDist = min( fDist, InCurve2(vP10,vP11,vP12, uv) );
    fDist = min( fDist, InCurve2(vP12,vP13,vP14, uv) );

    fDist = min( fDist, InQuad(vP0, vP2, vP4, vP14, uv) );
    fDist = min( fDist, InTri(vP14, vP4, vP5, uv) );
    fDist = min( fDist, InTri(vP14, vP5, vP12, uv) );
    fDist = min( fDist, InTri(vP5, vP6, vP12, uv) );
    fDist = min( fDist, InTri(vP6, vP7, vP12, uv) );
    fDist = min( fDist, InTri(vP6, vP10, vP12, uv) );
    fDist = min( fDist, InTri(vP8, vP10, vP7, uv) );
    
    return fDist;
}

float Glyph4(const in vec2 uv)
{
    vec2 vP = uv - vec2(0.305, 0.125);
    vP /= 0.065;
    vP.x *= 1.5;
    vP.x += vP.y * 0.25;
    
    vec2 vP2 = vP;

    vP.y = abs(vP.y);
    vP.y = pow(vP.y, 1.2);
    float f= length(vP);
    
    vP2.x *= 1.2;
    float f2 = length(vP2 * 1.5 - vec2(0.6, 0.0));
        
    return max(f - 1.0, 1.0 - f2) / 20.0;
} 

float Glyph5(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.507, 0.138 );
    const vec2  vP1 = vec2 ( 0.510, 0.065 );
    const vec2  vP2 = vec2 ( 0.570, 0.066 );
    const vec2  vP3 = vec2 ( 0.598, 0.066 );
    const vec2  vP4 = vec2 ( 0.594, 0.092 );
    const vec2  vP5 = vec2 ( 0.599, 0.131 );
    const vec2  vP6 = vec2 ( 0.537, 0.137 );
    const vec2  vP8 = vec2 ( 0.538, 0.125 );
    const vec2  vP9 = vec2 ( 0.564, 0.129 );
    const vec2 vP10 = vec2 ( 0.574, 0.100 );
    const vec2 vP11 = vec2 ( 0.584, 0.085 );
    const vec2 vP12 = vec2 ( 0.571, 0.079 );
    const vec2 vP13 = vec2 ( 0.557, 0.081 );
    const vec2 vP14 = vec2 ( 0.549, 0.103 );
    const vec2 vP15 = vec2 ( 0.518, 0.166 );
    const vec2 vP16 = vec2 ( 0.557, 0.166 );
    const vec2 vP17 = vec2 ( 0.589, 0.163 );
    const vec2 vP18 = vec2 ( 0.602, 0.137 );
    const vec2 vP20 = vec2 ( 0.602, 0.152 );
    const vec2 vP21 = vec2 ( 0.572, 0.194 );
    const vec2 vP22 = vec2 ( 0.537, 0.185 );
    const vec2 vP23 = vec2 ( 0.503, 0.189 );
    
    float fDist = 1.0;
    fDist = min( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = min( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = min( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = min( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = min( fDist, InCurve(vP10,vP11,vP12, uv) ); 
    fDist = min( fDist, InCurve(vP12,vP13,vP14, uv) );
    fDist = min( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = min( fDist, InCurve(vP16,vP17,vP18, uv) ); 
    fDist = min( fDist, InCurve2(vP20,vP21,vP22, uv) ); 
    fDist = min( fDist, InCurve2(vP22,vP23,vP0, uv) );

    fDist = min( fDist, InTri(vP0, vP2, vP13, uv) );
    fDist = min( fDist, InTri(vP13, vP2, vP12, uv) );
    fDist = min( fDist, InTri(vP2, vP11, vP12, uv) );
    fDist = min( fDist, InTri(vP2, vP4, vP11, uv) );
    fDist = min( fDist, InTri(vP11, vP4, vP10, uv) );
    fDist = min( fDist, InTri(vP10, vP4, vP9, uv) );
    fDist = min( fDist, InTri(vP6, vP8, vP9, uv) );
    fDist = min( fDist, InTri(vP0, vP13, vP14, uv) );
    fDist = min( fDist, InTri(vP0, vP14, vP15, uv) );
    fDist = min( fDist, InTri(vP15, vP16, vP22, uv) );
    fDist = min( fDist, InTri(vP16, vP17, vP22, uv) );
    fDist = min( fDist, InTri(vP17, vP18, vP20, uv) );
    
    return fDist;
}

float Glyph6(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.638 , 0.087 ); 
    const vec2  vP1 = vec2 ( 0.648 , 0.073 ); 
    const vec2  vP2 = vec2 ( 0.673 , 0.068 ); 
    const vec2  vP3 = vec2 ( 0.692 , 0.069 ); 
    const vec2  vP4 = vec2 ( 0.687 , 0.086 ); 
    const vec2  vP5 = vec2 ( 0.688 , 0.104 ); 
    const vec2  vP6 = vec2 ( 0.672 , 0.102 ); 
    const vec2  vP7 = vec2 ( 0.659 , 0.099 ); 
    const vec2  vP8 = vec2 ( 0.663 , 0.092 ); 
    const vec2  vP9 = vec2 ( 0.662 , 0.086 ); 
    const vec2 vP10 = vec2 ( 0.655 , 0.086 ); 
    const vec2 vP11 = vec2 ( 0.644 , 0.087 ); 
    const vec2 vP12 = vec2 ( 0.637 , 0.102 ); 
    const vec2 vP13 = vec2 ( 0.638 , 0.094 ); 

    float fDist = 1.0;
    fDist = min( fDist, InCurve2(vP0,vP1,vP2, uv) ); 
    fDist = min( fDist, InCurve2(vP2,vP3,vP4, uv) ); 
    fDist = min( fDist, InCurve2(vP4,vP5,vP6, uv) ); 
    fDist = min( fDist, InCurve2(vP6,vP7,vP8, uv) ); 
    fDist = min( fDist, InCurve(vP8,vP9,vP10, uv) ); 
    fDist = min( fDist, InCurve(vP10,vP11,vP12, uv) );

    fDist = min( fDist, InQuad(vP2, vP4, vP6, vP8, uv) );
    fDist = min( fDist, InTri(vP9, vP2, vP8, uv) );
    fDist = min( fDist, InTri(vP10, vP2, vP9, uv) );
    fDist = min( fDist, InQuad(vP0, vP2, vP10, vP11, uv) );
    fDist = min( fDist, InTri(vP11, vP12, vP0, uv) );
    
    return fDist;
}

float Glyph7(const in vec2 uv)
{
    const vec2 vP0 = vec2 ( 0.693 , 0.068 );
    const vec2 vP1 = vec2 ( 0.748 , 0.069 );
    const vec2 vP2 = vec2 ( 0.747 , 0.078 );
    const vec2 vP3 = vec2 ( 0.691 , 0.077 );
    
    return InQuad(vP0, vP1, vP2, vP3, uv);
}


float Glyph8(const in vec2 uv)
{ 
    vec2 vP = uv - vec2(0.788, 0.125);
    vP /= 0.065;
    vP.x *= 1.4;
    vP.x += vP.y * 0.25;
    
    vec2 vP2 = vP;
    
    vP.y = abs(vP.y);
    vP.y = pow(vP.y, 1.2);
    float f= length(vP);
    
    vP2.x *= 1.5;
    float f2 = length(vP2 * 1.5 - vec2(0.3, 0.0));
    
    
    return max(f - 1.0, 1.0 - f2) / 20.0;
}

float Glyph11(const in vec2 uv)
{
    const vec2  vP0 = vec2 ( 0.921 , 0.070 );
    const vec2  vP2 = vec2 ( 0.955 , 0.070 );
    const vec2  vP4 = vec2 ( 0.926 , 0.202 );
    const vec2  vP5 = vec2 ( 0.926 , 0.240 );
    const vec2  vP6 = vec2 ( 0.885 , 0.243 );
    const vec2  vP7 = vec2 ( 0.852 , 0.239 );
    const vec2  vP8 = vec2 ( 0.859 , 0.219 );
    const vec2  vP9 = vec2 ( 0.862 , 0.192 );
    const vec2 vP10 = vec2 ( 0.889 , 0.189 );
    const vec2 vP12 = vec2 ( 0.928 , 0.178 );
    const vec2 vP13 = vec2 ( 0.949 , 0.173 );
    const vec2 vP14 = vec2 ( 0.951 , 0.162 );
    const vec2 vP15 = vec2 ( 0.960 , 0.150 );
    const vec2 vP16 = vec2 ( 0.960 , 0.144 );
    const vec2 vP18 = vec2 ( 0.971 , 0.144 );
    const vec2 vP19 = vec2 ( 0.968 , 0.157 );
    const vec2 vP20 = vec2 ( 0.957 , 0.171 );
    const vec2 vP21 = vec2 ( 0.949 , 0.182 );
    const vec2 vP22 = vec2 ( 0.922 , 0.189 );
    const vec2 vP24 = vec2 ( 0.900 , 0.196 );
    const vec2 vP25 = vec2 ( 0.866 , 0.205 );
    const vec2 vP26 = vec2 ( 0.871 , 0.217 );
    const vec2 vP27 = vec2 ( 0.871 , 0.225 );
    const vec2 vP28 = vec2 ( 0.880 , 0.224 );
    const vec2 vP29 = vec2 ( 0.889 , 0.218 );
    const vec2 vP30 = vec2 ( 0.893 , 0.203 );

    float fDist = 1.0;
    fDist = min( fDist, InCurve2(vP4,vP5,vP6, uv) );
    fDist = min( fDist, InCurve2(vP6,vP7,vP8, uv) );
    fDist = min( fDist, InCurve2(vP8,vP9,vP10, uv) );
    fDist = min( fDist, InCurve(vP12,vP13,vP14, uv) );

    fDist = min( fDist, InCurve(vP14,vP15,vP16, uv) );
    fDist = min( fDist, InCurve2(vP18,vP19,vP20, uv) );
    fDist = min( fDist, InCurve2(vP20,vP21,vP22, uv) );

    fDist = min( fDist, InCurve(vP24,vP25,vP26, uv) );
    fDist = min( fDist, InCurve(vP26,vP27,vP28, uv) );
    fDist = min( fDist, InCurve(vP28,vP29,vP30, uv) );
    
    fDist = min( fDist, InQuad(vP0, vP2, vP4, vP30, uv) );

    fDist = min( fDist, InQuad(vP10, vP12, vP22, vP24, uv) );
        
    fDist = min( fDist, InTri(vP30, vP4, vP6, uv) );
    fDist = min( fDist, InTri(vP30, vP6, vP29, uv) );
    fDist = min( fDist, InTri(vP28, vP29, vP6, uv) );
    fDist = min( fDist, InTri(vP28, vP6, vP27, uv) );
    
    fDist = min( fDist, InTri(vP8, vP27, vP6, uv) );
    
    fDist = min( fDist, InTri(vP8, vP26, vP27, uv) );
    fDist = min( fDist, InTri(vP8, vP25, vP26, uv) );
    fDist = min( fDist, InTri(vP25, vP10, vP24, uv) );
    
    fDist = min( fDist, InTri(vP12, vP13, vP20, uv) );
    fDist = min( fDist, InTri(vP12, vP20, vP22, uv) );
    fDist = min( fDist, InTri(vP13, vP14, vP20, uv) );
    fDist = min( fDist, InTri(vP15, vP20, vP14, uv) );
    fDist = min( fDist, InTri(vP15, vP18, vP20, uv) );
    fDist = min( fDist, InTri(vP15, vP16, vP18, uv) );
    
    return fDist;
}

float Shadertoy(in vec2 uv)
{
    float fResult = 1.0;
    
    fResult = min(fResult, Glyph0(uv)); // S

    vec2 vUVOffset = vec2(0.001, 0.0); // tail of h
    vec2 vTailOffset = vec2(0.0, 0.0);  
    float fUVScale = 1.0;

    if(uv.x < 0.3)
    {
        if(uv.y < 0.12)
        {
            // top of h
            fUVScale = -1.0;
            vUVOffset = vec2(0.448, 0.25);  
            vTailOffset = vec2(0.0, 0.0);   
        }
    }
    else if(uv.x < 0.4)    
    {
        // tail of a
        vUVOffset = vec2(-0.124, 0.0);  
        vTailOffset = vec2(0.01, -0.04);    
    }
    else if(uv.x < 0.6)
    {
        // tail of d
        vUVOffset = vec2(-0.248, 0.0);  
        vTailOffset = vec2(0.02, -0.1); 
    }
    else if(uv.x < 0.83)
    {
        // stalk of t
        vUVOffset = vec2(-0.48, 0.0);   
        vTailOffset = vec2(0.02, -0.1); 
    }
    else
    {
        // start of y
        vUVOffset = vec2(-0.645, 0.0);  
        vTailOffset = vec2(0.005, -0.042);  
    }
    
    fResult = min(fResult, Glyph3(uv * fUVScale + vUVOffset, vTailOffset)); // tails h, a, d, t, start of y and top of h


    vec2 vUVOffset3 = vec2(0.0, 0.0);   // vertical of h
    vec2 vTailOffset3 = vec2(0.0, 0.0);
    
    if(uv.x > 0.5)
    {
        // vertical of r
        vUVOffset3 = vec2(-0.45, 0.0);  
        vTailOffset3 = vec2(-0.01, 0.04);   
    }
    
    fResult = min(fResult, Glyph1(uv + vUVOffset3, vTailOffset3)); // vertical of h, r

    vec2 vUVOffset2 = vec2(0.0, 0.0); // curve of a
    if(uv.x > 0.365)
    {
        vUVOffset2 = vec2(-0.125, 0.0); // curve of d
    }

    fResult = min(fResult, Glyph4(uv + vUVOffset2)); // curve of a, d
    
    fResult = min(fResult, Glyph5(uv)); // e

    vec2 vUVOffset4 = vec2(0.001, 0.0); // top of r
    vec2 vUVScale4 = vec2(1.0, 1.0);        
    
    if(uv.x > 0.7)
    {
        // o loop
        vUVOffset4.x = 1.499;
        vUVOffset4.y = 0.19;
        
        vUVScale4.x = -1.0;
        vUVScale4.y = -1.0;
    }
    
    fResult = min(fResult, Glyph6(uv * vUVScale4 + vUVOffset4)); // top of r and o loop

    fResult = min(fResult, Glyph7(uv)); // cross t    
    
    fResult = min(fResult, Glyph8(uv)); // o1
    
    fResult = min(fResult, Glyph11(uv)); // y2        

    return fResult; 
}

vec2 GetUVCentre(const vec2 vInputUV)
{
	vec2 vFontUV = vInputUV;
    vFontUV.y -= 0.35;
		
	return vFontUV;
}

vec2 GetUVScroll(const vec2 vInputUV, float t)
{
	vec2 vFontUV = vInputUV;
	vFontUV *= 0.25;
	
    vFontUV.y -= 0.005;
	vFontUV.x += t * 3.0 - 1.5;
	
	return vFontUV;
}

vec2 GetUVRepeat(const vec2 vInputUV, float t2)
{
	vec2 vFontUV = vInputUV;
	
	vFontUV *= vec2(1.0, 4.0);
	
	vFontUV.x += floor(vFontUV.y) * t2;
	
	vFontUV = fract(vFontUV);
	
	vFontUV /= vec2(1.0, 4.0);
		
	return vFontUV;
}

vec2 GetUVRotate(const vec2 vInputUV, float t)
{
	vec2 vFontUV = vInputUV - 0.5;
	
	float s = sin(t);
	float c = cos(t);
	
	vFontUV = vec2(  vFontUV.x * c + vFontUV.y * s,
			        -vFontUV.x * s + vFontUV.y * c );
	
	vFontUV += 0.5;
	
	return vFontUV;
}

vec3 StyleDefault( float f )
{
	return mix(vec3(0.25), vec3(1.0), f);
}

vec3 StyleScanline( float f, in vec2 fragCoord )
{
	float fShade = f * 0.8 + 0.2;
	
	fShade *= mod(fragCoord.y, 2.0);
	
	return mix(vec3(0.01, 0.2, 0.01), vec3(0.01, 1.0, 0.02), fShade);
}

vec3 StyleStamp( float fFont, vec2 uv )
{
	vec3 t1 = texture(iChannel0, uv + 0.005).rgb;
	vec3 t2 = texture(iChannel0, uv).rgb;
	float dt = clamp(0.5 + (t1.x - t2.x), 0.0, 1.0);
	float fWear = clamp((0.9 - t2.x) * 4.0, 0.0, 1.0);
	float f =  clamp(fFont * fWear, 0.0, 1.0);
	return mix( vec3(1.0, 0.98, 0.9) * (dt * 0.1 + 0.9), vec3(0.7, 0.0, 0.0), f);
}

vec3 StyleWood( float fFont, vec2 uv )
{
	vec3 t = texture(iChannel0, uv).rgb;
	float fWear = fFont * smoothstep(0.0, 0.4, t.b);
	return mix(t, vec3(0.0), fWear);
}

vec4 GetRandom4(float x)
{
	return fract(vec4(987.65, 432.10, 765.43, 210.98) * sin(vec4(123.456, 789.123, 456.789, 567.890) * x));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	float fSequenceLength = 5.0;
	
	float fTime = iTime;
	
	float fBlendSpeed = 0.05;
	
	// Skip the initial fade-in
	fTime += fBlendSpeed * fSequenceLength;
	
	float fInt = floor(fTime / fSequenceLength);
	float fFract = fract(fTime / fSequenceLength);
	
	vec4 vRandom4 = GetRandom4(fInt);
	vec2 vRandom2 = floor(vRandom4.xy * vec2(1234.56, 123.45));
	
	float fUVEffect = mod(vRandom2.x, 4.0);
	float fScreenEffect = mod(vRandom2.y, 4.0);

	if(fInt < 0.5)
	{
		fUVEffect = 0.0;
		fScreenEffect = 0.0;
	}

	vec4 vResult = vec4(0.0);
		
	float fX = 0.0;
	for(int iX=0; iX<AA_X; iX++)
	{
		float fY = 0.0;
		for(int y=0; y<AA_Y; y++)
		{
	
			vec2 vUV = (fragCoord.xy + vec2(fX, fY)) / iResolution.xy;
			vUV.x = ((vUV.x - 0.5) * (iResolution.x / iResolution.y)) + 0.5;    
			vUV.y = 1.0 - vUV.y;
				
			vec2 vFontUV = vUV;
			vec2 vBgUV = vUV;
			
			if(iMouse.z > 0.0)
			{
				fUVEffect = 999.0;
				fScreenEffect = 0.0;
				fFract = 0.5;
				
				vFontUV *= 0.25;
				vFontUV += iMouse.xy / iResolution.xy;
				vFontUV.y -= 0.5;
				vBgUV = vFontUV;
			}	
			
			if(fUVEffect < 0.5)
			{
				vFontUV = GetUVCentre(vBgUV);
			}
			else
			if(fUVEffect < 1.5)
			{
				vBgUV = GetUVScroll(vBgUV, fFract);
				vFontUV = vBgUV;
			}
			else
			if(fUVEffect < 2.5)
			{
				float fSpeed = 0.1 + vRandom4.z;
				vBgUV.x += fFract * fSpeed;
				vFontUV = GetUVRepeat(vBgUV, 0.25);
			}
			else
			if(fUVEffect < 3.5)
			{
				float fSpeed = 1.0 + vRandom4.z * 2.0;
				if(vRandom4.w > 0.5)
				{
					fSpeed = -fSpeed;
				}
				vBgUV = GetUVRotate(vBgUV, 1.0 + fSpeed * fFract);
				vFontUV = GetUVRepeat(vBgUV, 0.0);
			}
			
			float fShadertoy = step(Shadertoy(vFontUV), 0.0);
				
			if(fScreenEffect < 0.5)
			{
				vResult += vec4(StyleDefault(fShadertoy), 1.0);
			}
			else if(fScreenEffect < 1.5)
			{
				vResult += vec4(StyleScanline(fShadertoy, fragCoord), 1.0);
			}
			else if(fScreenEffect < 2.5)
			{
				vResult += vec4(StyleStamp(fShadertoy, vBgUV), 1.0);
			}
			else
			{
				vResult += vec4(StyleWood(fShadertoy, vBgUV), 1.0);
			}

			fY += 1.0 / float(AA_Y);
		}
		
		fX += 1.0 / float(AA_X);
	}
	
	vResult.xyz /= vResult.w;

	float fFade = 0.0;	
	if(fFract > (1.0 - fBlendSpeed))
	{
		fFade = smoothstep(1.0 - fBlendSpeed, 1.0, fFract);
	}

	if(fFract < fBlendSpeed)
	{
		fFade = smoothstep(fBlendSpeed, 0.0, fFract);
	}

	vResult = mix(vResult, vec4(1.0), fFade);
	
    fragColor = vec4(vResult.xyz, 1.0);
}
