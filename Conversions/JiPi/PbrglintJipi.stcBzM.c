
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


// The MIT License
// Copyright © 2020 Xavier Chermain (ICUBE), Basile Sauvage (ICUBE), Jean-Michel Dishler (ICUBE), Carsten Dachsbacher (KIT)
// Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions: The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software. THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

// Shadertoy implementation of
// Procedural Physically based BRDF for Real-Time Rendering of Glints
// Xavier Chermain (ICUBE), Basile Sauvage (ICUBE), Jean-Michel Dishler (ICUBE) and Carsten Dachsbacher (KIT)
// Pacific Graphic 2020, CGF special issue
// Project page: http://igg.unistra.fr/People/chermain/real_time_glint/

// Dictionary is generated on the fly (we cannot use pre-computed data on Shadertoy), 
// but with a limited number of lobes (16) to achieve a reasonable frame rate.
// For a full-featured version (WebGL), see: http://igg.unistra.fr/People/reproctex/Demos/Real_Time_Glint/

//=========================================================================================================================
//================================================== PG2020 title =========================================================
//=========================================================================================================================
#define PG2020W 32u
#define PG2020H 18u



__DEVICE__ bool jfig(in uint x, in uint y) {

uint pg2020_bitfield[] = { 0x0u,0x0u,0x0u,0x003e7c00u,0x00024400u,0x00327c00u,0x00220400u,0x003e0400u,0x0u,0x0u,0x30e30e0u,0x4904900u,0x49e49e0u,0x4824820u,0x31e31e0u,0x0u,0x0u,0x0u };

    uint id = x + (PG2020H-1u-y)*PG2020W;
    if (id>=PG2020W*PG2020H) return false;
    return 0u != (pg2020_bitfield[id/32u] & (1u << (id&31u)));
}

__DEVICE__ float texel(int s, int t){
    if(s < 0 || s >= (int)(PG2020W) || t < 0 || t >= (int)(PG2020H) || jfig((uint)(s), (uint)(t)))
      return 0.0f;
    
    return 1.0f;
}

__DEVICE__ float pg2020TriangleFilter(float2 st){
    float s = st.x * (float)(PG2020W) - 0.5f;
    float t = st.y * (float)(PG2020H) - 0.5f;
    int s0 = (int)(_floor(s));
    int t0 = (int)(_floor(t));
    float ds = s - (float)(s0);
    float dt = t - (float)(t0);
    return (1.0f - ds) * (1.0f - dt) * texel(s0, t0) +
           (1.0f - ds) * dt * texel(s0, t0 + 1) +
           ds * (1.0f - dt) * texel(s0 + 1, t0) +
           ds * dt * texel(s0 + 1, t0 + 1);
}

//=========================================================================================================================
//================================================== Material parameters ==================================================
//==================================================== Can be changed =====================================================
//=========================================================================================================================
// Roughness of the glinty material [0.1, 1.]
#define ALPHA_X 0.5f
#define ALPHA_Y 0.5f

// Microfacet relative area [0.01, 1.]. 
// Set to 0.01f with LOGMICROFACETDENSITY set tot 5.0f gives sparse glints (snow, sand, sparkling rocks)
#define MICROFACETRELATIVEAREA 1.0f

// Logarithmic microfacet density [5., 25.]
#define LOGMICROFACETDENSITY 14.0f

// Maximum anisotropy of the pixel footprint (not realy usefull in this scene)
#define MAXANISOTROPY 8.0f

// Varnished material (add a specular lobe with a small roughness)
# define VARNISHED true

//=========================================================================================================================
//============================================== Parameters of the dictionary =============================================
//=========================================================================================================================
// Roughness used during the dictionary generation
#define ALPHA_DIC 0.5f
// Number of distributions
// In the paper, we use 192 marginal distributions of slope
// In shadertoy, we generate the dictionary on the fly. So we use a large number of different NDFs
#define N 999999
// Number of levels. In the paper : 16.0f In shadertoy : 8
#define NLEVELS 8
// Size of the tabulated marginal distributions. In the paper : 64.0f In shadertoy : 32
#define DISTRESOLUTION 32



//=========================================================================================================================
//================================================ Mathematical constants =================================================
//=========================================================================================================================
#define PI 3.141592f
#define IPI 0.318309f
#define ISQRT2 0.707106f



//=========================================================================================================================
//=============================================== Beckmann anisotropic NDF ================================================
//==================== Shadertoy implementation : Arthur Cavalier (https://www.shadertoy.com/user/H4w0) ===================
//========================================= https://www.shadertoy.com/view/WlGXRt =========================================
//=========================================================================================================================

//-----------------------------------------------------------------------------
//-- Beckmann Distribution ----------------------------------------------------
__DEVICE__ float p22_beckmann_anisotropic(float x, float y, float alpha_x, float alpha_y)
{
    float x_sqr = x*x;
    float y_sqr = y*y;
    float sigma_x = alpha_x * ISQRT2;
    float sigma_y = alpha_y * ISQRT2;
    float sigma_x_sqr = sigma_x*sigma_x;
    float sigma_y_sqr = sigma_y*sigma_y;
    return( 
            _expf( -0.5f * ((x_sqr/sigma_x_sqr) + (y_sqr/sigma_y_sqr)) )
    / //-------------------------------------------------------------------
                    ( 2.0f * PI * sigma_x * sigma_y )
    );
}

__DEVICE__ float ndf_beckmann_anisotropic(float3 omega_h, float alpha_x, float alpha_y)
{
    float slope_x = - (omega_h.x/omega_h.z);
    float slope_y = - (omega_h.y/omega_h.z);
    float cos_theta = omega_h.z;
    float cos_2_theta = omega_h.z * omega_h.z;
    float cos_4_theta = cos_2_theta * cos_2_theta;
    float beckmann_p22 = p22_beckmann_anisotropic(slope_x,slope_y,alpha_x,alpha_y);
    return(
                beckmann_p22
    / //---------------------------
                cos_4_theta
    );
}

//=========================================================================================================================
//======================================== Schlick approximation of Fresnel ===============================================
//=========================================================================================================================
__DEVICE__ float3 fresnel_schlick(in float wo_dot_wh, in float3 F0)
{
    return F0 + (1.0f - F0) * _powf(1.0f - wo_dot_wh, 5.0f);
}

//=========================================================================================================================
//===================================== Microfacet BRDF of Cook and Torrance 1982 =========================================
//=========================================================================================================================
__DEVICE__ float3 f_specular(float3 wo, float3 wi)
{
    if(wo.z <= 0.0f) return to_float3(0.0f,0.0f,0.0f);
    if(wi.z <= 0.0f) return to_float3(0.0f,0.0f,0.0f);
    float3 wh = normalize(wo+wi);
    if(wh.z <= 0.0f) return to_float3(0.0f,0.0f,0.0f);
    // Local masking shadowing
    if (dot(wo, wh) <= 0.0f || dot(wi, wh) <= 0.0f) return to_float3_s(0.0f);
    float wi_dot_wh = clamp(dot(wi,wh),0.0f,1.0f);

    float D = ndf_beckmann_anisotropic(wh,0.1f, 0.1f);
    // V-cavity masking shadowing
    float G1wowh = _fminf(1.0f, 2.0f * wh.z * wo.z / dot(wo, wh));
    float G1wiwh = _fminf(1.0f, 2.0f * wh.z * wi.z / dot(wi, wh));
    float G = G1wowh * G1wiwh;
    
  float3 F  = fresnel_schlick(wi_dot_wh,to_float3(1.0f, 1.0f, 1.0f));
        
    return (D * F * G) / ( 4.0f * wo.z );
}

//=========================================================================================================================
//=============================================== Diffuse Lambertian BRDF =================================================
//=========================================================================================================================
__DEVICE__ float3 f_diffuse(float3 wo, float3 wi)
{
    if (wo.z <= 0.0f)
        return to_float3(0.0f, 0.0f, 0.0f);
    if (wi.z <= 0.0f)
        return to_float3(0.0f, 0.0f, 0.0f);

    return to_float3(0.8f, 0.0f, 0.0f) * IPI * wi.z;
}

//=========================================================================================================================
//=============================================== Inverse error function ==================================================
//=========================================================================================================================
__DEVICE__ float erfinv(float x)
{
    float w, p;
    w = -_logf((1.0f - x) * (1.0f + x));
    if (w < 5.000000f)
    {
        w = w - 2.500000f;
        p = 2.81022636e-08;
        p = 3.43273939e-07 + p * w;
        p = -3.5233877e-06 + p * w;
        p = -4.39150654e-06 + p * w;
        p = 0.00021858087f + p * w;
        p = -0.00125372503f + p * w;
        p = -0.00417768164f + p * w;
        p = 0.246640727f + p * w;
        p = 1.50140941f + p * w;
    }
    else
    {
        w = _sqrtf(w) - 3.000000f;
        p = -0.000200214257f;
        p = 0.000100950558f + p * w;
        p = 0.00134934322f + p * w;
        p = -0.00367342844f + p * w;
        p = 0.00573950773f + p * w;
        p = -0.0076224613f + p * w;
        p = 0.00943887047f + p * w;
        p = 1.00167406f + p * w;
        p = 2.83297682f + p * w;
    }
    return p * x;
}

//=========================================================================================================================
//================================================== Hash function ========================================================
//================================================== Inigo Quilez =========================================================
//====================================== https://www.shadertoy.com/view/llGSzw ============================================
//=========================================================================================================================
// 
__DEVICE__ float hashIQ(uint n)
{
    // integer hash copied from Hugo Elias
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return (float)(n & 0x7fffffffU) / (float)(0x7fffffff);
}

//=========================================================================================================================
//=============================================== Pyramid size at LOD level ===============================================
//=========================================================================================================================
__DEVICE__ int pyramidSize(int level)
{
    return (int)(_powf(2.0f, (float)(NLEVELS - 1 - level)));
}

__DEVICE__ float normalDistribution1D(float x, float mean, float std_dev) {
    float xMinusMean = x - mean;
    float xMinusMeanSqr = xMinusMean * xMinusMean;
    return _expf(-xMinusMeanSqr / (2.0f * std_dev * std_dev)) /
           (std_dev * 2.506628f);
    // 2.506628f \approx _sqrtf(2 * \pi)
}

//=========================================================================================================================
//========================================= Sampling from a Normal distribution ===========================================
//=========================================================================================================================
__DEVICE__ float sampleNormalDistribution(float U, float mu, float sigma)
{
    float x = sigma * 1.414213f * erfinv(2.0f * U - 1.0f) + mu;
    return x;
}

//=========================================================================================================================
//==================================== Evaluation of the i th marginal distribution P =====================================
//========================================== with slope x and at LOD level ================================================
//============================ Procedural version, because we cannot use data with Shadertoy ==============================
//=========================================================================================================================

__DEVICE__ float P_procedural(float x, int i, int level) {
    
    // We use even functions
    x = _fabs(x);
    // After 4 standard deviation sigma, we consider that the distribution equals zero
    float sigma_dist_4 = 4.0f * ALPHA_DIC / 1.414214f; // alpha_dist = 0.5f so sigma_dist \approx 0.3535f (0.5f / _sqrtf(2))
    if(x >= sigma_dist_4) return 0.0f;
    
    int nMicrofacetsCurrentLevel = (int)(_powf(2.0f, (float)(level)));
    float density = 0.0f;
    // Dictionary should be precomputed, but we cannot use memory with Shadertoy
    // So we generate it on the fly with a very limited number of lobes
    nMicrofacetsCurrentLevel = _fminf(16, nMicrofacetsCurrentLevel);
    
    for (int n = 0; n < nMicrofacetsCurrentLevel; ++n) {
        
        float U_n = hashIQ((uint)(i*7333+n*5741));
        // alpha roughness equals _sqrtf(2) * RMS roughness
        //     ALPHA_DIC     =   1.414214f * std_dev
        // std_dev = ALPHA_DIC / 1.414214f 
        float currentMean = sampleNormalDistribution(U_n, 0.0f, ALPHA_DIC / 1.414214f);
        density += normalDistribution1D(x, currentMean, 0.05f) +
                   normalDistribution1D(-x, currentMean, 0.05f);
    }
    return density / (float)(nMicrofacetsCurrentLevel);
}

//=========================================================================================================================
//=================== Spatially-varying, multiscale, rotated, and scaled slope distribution function ======================
//================================================= Eq. 11, Alg. 3 ========================================================
//=========================================================================================================================
__DEVICE__ float P22_theta_alpha(float2 slope_h, int l, int s0, int t0)
{
    
    // Coherent index
    // Eq. 18, Alg. 3, line 1
    s0 *= 1 << l;
    t0 *= 1 << l;

    // Seed pseudo random generator
    // Alg. 3, line 2
    int rngSeed = s0 + 1549 * t0;

    // Alg.3, line 3
    float uMicrofacetRelativeArea = hashIQ(uint(rngSeed) * 13U);
    // Discard cells by using microfacet relative area
    // Alg.3, line 4
    if (uMicrofacetRelativeArea > MICROFACETRELATIVEAREA)
        return 0.f;

    // Number of microfacets in a cell
    // Alg. 3, line 5
    float n = _powf(2.0f, (float)(2 * l - (2 * (NLEVELS - 1))));
    n *= _expf(LOGMICROFACETDENSITY);

    // Corresponding continuous distribution LOD
    // Alg. 3, line 6
    float l_dist = _logf(n) / 1.38629f; // 2.0f * _logf(2) = 1.38629
    
    // Alg. 3, line 7
    float uDensityRandomisation = hashIQ((uint)(rngSeed) * 2171U);

    // Fix density randomisation to 2 to have better appearance
    // Notation in the paper: \zeta
    float densityRandomisation = 2.0f;
    
    // Sample a Gaussian to randomise the distribution LOD around the distribution level l_dist
    // Alg. 3, line 8
    l_dist = sampleNormalDistribution(uDensityRandomisation, l_dist, densityRandomisation);

    // Alg. 3, line 9
    int l_disti = clamp((int)(round(l_dist)), 0, NLEVELS);

    // Alg. 3, line 10
    if (l_disti == NLEVELS)
        return p22_beckmann_anisotropic(slope_h.x, slope_h.y, ALPHA_X, ALPHA_Y);

    // Alg. 3, line 13
    float uTheta = hashIQ(uint(rngSeed));
    float theta = 2.0f * PI * uTheta;

    // Uncomment to remove random distribution rotation
    // Lead to glint alignments with a small N
    // theta = 0.0f;

    float cosTheta = _cosf(theta);
    float sinTheta = _sinf(theta);
    
    float2 scaleFactor = to_float2(ALPHA_X / ALPHA_DIC,
                                   ALPHA_Y / ALPHA_DIC);

    // Rotate and scale slope
    // Alg. 3, line 16
    slope_h = to_float2(slope_h.x * cosTheta / scaleFactor.x + slope_h.y * sinTheta / scaleFactor.y,
                       -slope_h.x * sinTheta / scaleFactor.x + slope_h.y * cosTheta / scaleFactor.y);

    // Alg. 3, line 17
    float u1 = hashIQ((uint)(rngSeed) * 16807U);
    float u2 = hashIQ((uint)(rngSeed) * 48271U);

    // Alg. 3, line 18
    int i = int(u1 * (float)(N));
    int j = int(u2 * (float)(N));
    
    float P_i = P_procedural(slope_h.x, i, l_disti);
    float P_j = P_procedural(slope_h.y, j, l_disti);

    // Alg. 3, line 19
    return P_i * P_j / (scaleFactor.x * scaleFactor.y);

}

//=========================================================================================================================
//========================================= Alg. 2, P-SDF for a discrete LOD ==============================================
//=========================================================================================================================

// Most of this function is similar to pbrt-v3 EWA function,
// which itself is similar to Heckbert 1889 algorithm, http://www.cs.cmu.edu/~ph/texfund/texfund.pdf, Section 3.5.9.
// Go through cells within the pixel footprint for a given LOD l

__DEVICE__ float P22_floorP(int l, float2 slope_h, float2 st, float2 dst0, float2 dst1)
{
    // Convert surface coordinates to appropriate scale for level
    float pyrSize = (float)(pyramidSize(l));
    st.x = st.x * pyrSize - 0.5f;
    st.y = st.y * pyrSize - 0.5f;
    dst0.x *= pyrSize;
    dst0.y *= pyrSize;
    dst1.x *= pyrSize;
    dst1.y *= pyrSize;

    // Compute ellipse coefficients to bound filter region
    float A = dst0.y * dst0.y + dst1.y * dst1.y + 1.0f;
    float B = -2.0f * (dst0.x * dst0.y + dst1.x * dst1.y);
    float C = dst0.x * dst0.x + dst1.x * dst1.x + 1.0f;
    float invF = 1.0f / (A * C - B * B * 0.25f);
    A *= invF;
    B *= invF;
    C *= invF;

    // Compute the ellipse's bounding box in texture space
    float det = -B * B + 4.0f * A * C;
    float invDet = 1.0f / det;
    float uSqrt = _sqrtf(det * C), vSqrt = _sqrtf(A * det);
    int s0 = (int)(_ceil(st.x - 2.0f * invDet * uSqrt));
    int s1 = (int)(_floor(st.x + 2.0f * invDet * uSqrt));
    int t0 = (int)(_ceil(st.y - 2.0f * invDet * vSqrt));
    int t1 = (int)(_floor(st.y + 2.0f * invDet * vSqrt));

    // Scan over ellipse bound and compute quadratic equation
    float sum = 0.f;
    float sumWts = 0.0f;
    int nbrOfIter = 0;

    for (int it = t0; it <= t1; ++it)
    {
        float tt = (float)(it) - st.y;
        for (int is = s0; is <= s1; ++is)
        {
            float ss = (float)(is) - st.x;
            // Compute squared radius and filter SDF if inside ellipse
            float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
            if (r2 < 1.0f)
            {
                // Weighting function used in pbrt-v3 EWA function
                float alpha = 2.0f;
                float W_P = _expf(-alpha * r2) - _expf(-alpha);
                // Alg. 2, line 3
                sum += P22_theta_alpha(slope_h, l, is, it) * W_P;
                
                sumWts += W_P;
            }
            nbrOfIter++;
            // Guardrail (Extremely rare case.)
            if (nbrOfIter > 100)
                break;
        }
        // Guardrail (Extremely rare case.)
        if (nbrOfIter > 100)
            break;
    }
    return sum / sumWts;
}

//=========================================================================================================================
//=============================== Evaluation of our procedural physically based glinty BRDF ===============================
//==================================================== Alg. 1, Eq. 14 =====================================================
//=========================================================================================================================

__DEVICE__ float2 dfdx(float2 value, float2 fragCoord, float2 iResolution)
{
   return to_float2_s( value.x*fragCoord.x / iResolution.x )*iResolution;
}

__DEVICE__ float2 dfdy(float2 value, float2 fragCoord, float2 iResolution)
{
   return to_float2_s( value.y*fragCoord.y / iResolution.y )*iResolution;
}




__DEVICE__ float3 f_P(float3 wo, float3 wi, float2 uv, float2 fragCoord, float2 iResolution)
{
  
    if (wo.z <= 0.0f)
        return to_float3(0.0f, 0.0f, 0.0f);
    if (wi.z <= 0.0f)
        return to_float3(0.0f, 0.0f, 0.0f);

    // Alg. 1, line 1
    float3 wh = normalize(wo + wi);
    if (wh.z <= 0.0f)
        return to_float3(0.0f, 0.0f, 0.0f);

    // Local masking shadowing
    if (dot(wo, wh) <= 0.0f || dot(wi, wh) <= 0.0f)
        return to_float3_s(0.0f);

    // Eq. 1, Alg. 1, line 2
    float2 slope_h = to_float2(-wh.x / wh.z, -wh.y / wh.z);

    float2 texCoord = uv;

    float D_P = 0.0f;
    float P22_P = 0.0f;

    // ------------------------------------------------------------------------------------------------------
    // Similar to pbrt-v3 MIPMap::Lookup function, http://www.pbr-book.org/3ed-2018/Texture/Image_Texture.html#EllipticallyWeightedAverage

    // Alg. 1, line 3
    float2 dst0 = dfdx(texCoord, fragCoord, iResolution);
    float2 dst1 = dfdy(texCoord, fragCoord, iResolution);

    // Compute ellipse minor and major axes
    float dst0Length = length(dst0);
    float dst1Length = length(dst1);

    if (dst0Length < dst1Length)
    {
        // Swap dst0 and dst1
        float2 tmp = dst0;
        dst0 = dst1;
        dst1 = tmp;
    }
    float majorLength = length(dst0);
    // Alg. 1, line 5
    float minorLength = length(dst1);

    // Clamp ellipse eccentricity if too large
    // Alg. 1, line 4
    if (minorLength * MAXANISOTROPY < majorLength && minorLength > 0.0f)
    {
        float scale = majorLength / (minorLength * MAXANISOTROPY);
        dst1 *= scale;
        minorLength *= scale;
    }
    // ------------------------------------------------------------------------------------------------------

    // Without footprint, we evaluate the Cook Torrance BRDF
    if (minorLength == 0.0f)
    {
        D_P = ndf_beckmann_anisotropic(wh, ALPHA_X, ALPHA_Y);
    }
    else
    {
        // Choose LOD
        // Alg. 1, line 6
        float l = _fmaxf(0.0f, (float)(NLEVELS) - 1.0f + _log2f(minorLength));
        int il = (int)(_floor(l));

        // Alg. 1, line 7
        float w = l - (float)(il);

        // Alg. 1, line 8
        P22_P = _mix(P22_floorP(il, slope_h, texCoord, dst0, dst1),
                    P22_floorP(il + 1, slope_h, texCoord, dst0, dst1),
                    w);

        // Eq. 13, Alg. 1, line 10
        D_P = P22_P / (wh.z * wh.z * wh.z * wh.z);
    }

    // V-cavity masking shadowing
    float G1wowh = _fminf(1.0f, 2.0f * wh.z * wo.z / dot(wo, wh));
    float G1wiwh = _fminf(1.0f, 2.0f * wh.z * wi.z / dot(wi, wh));
    float G = G1wowh * G1wiwh;

    // Fresnel is set to one for simplicity here
    // but feel free to use "real" Fresnel term
    float3 F = to_float3(1.0f, 1.0f, 1.0f);

    // Eq. 14, Alg. 1, line 11
    // (wi dot wg) is cancelled by
    // the cosine weight in the rendering equation
    return (F * G * D_P) / (4.0f * wo.z);
}

//=========================================================================================================================
//===================================================== Renderer ==========================================================
//=========================================================================================================================
__KERNEL__ void PbrglintJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    // Light intensity
    float3 lightIntensity = to_float3_s(50000.0f);
    
    // Texture position
    float2 uv = fragCoord/iResolution.y * 400.0f;
    
    // Vertex position
    float3 vertexPos = to_float3_aw(fragCoord - iResolution/2.0f, 0.0f);
    
    // Light position (varies over time)
    float x_i = _cosf(iTime*0.6f) * iResolution.x / 2.0f;
    float y_i = _cosf(iTime) * iResolution.y / 2.0f;
    float3 lightPos = to_float3(x_i, y_i, 100);
    
    // Camera position
    float3 cameraPos = to_float3(0, 0, 100);
    
    // Compute normal from JFIG heightfield
    float diff = 10.0f;
    float hJFIGsm1t0 = pg2020TriangleFilter(to_float2((fragCoord.x - diff)/iResolution.x, (fragCoord.y)/iResolution.y));
    float hJFIGs1t0 = pg2020TriangleFilter(to_float2((fragCoord.x + diff)/iResolution.x, (fragCoord.y)/iResolution.y));
    float hJFIGs0tm1 = pg2020TriangleFilter(to_float2((fragCoord.x)/iResolution.x, (fragCoord.y - diff)/iResolution.y));
    float hJFIGs0t1 = pg2020TriangleFilter(to_float2((fragCoord.x)/iResolution.x, (fragCoord.y + diff)/iResolution.y));
    float2 slope = to_float2((hJFIGs1t0 - hJFIGsm1t0)/2.0f,
                      (hJFIGs0t1 - hJFIGs0tm1)/2.0f);
    slope *= 4.0f;
    float3 vertexNormal = to_float3(-slope.x, -slope.y, 1.0f) / _sqrtf(slope.x*slope.x+slope.y*slope.y+1.0f);
    
    float3 vertexTangent = to_float3(1.0f, 0.0f, 0.0f);
    // Gram–Schmidt process
    vertexTangent = vertexTangent - (dot(vertexNormal, vertexTangent) / dot(vertexNormal, vertexNormal)) * vertexNormal;
    float3 vertexBinormal = cross(vertexNormal, vertexTangent);
    
    // Matrix for transformation to tangent space
    mat3 toLocal = to_mat3(
                          vertexTangent.x, vertexBinormal.x, vertexNormal.x,
                          vertexTangent.y, vertexBinormal.y, vertexNormal.y,
                          vertexTangent.z, vertexBinormal.z, vertexNormal.z ) ;
                        
    // Incident direction
    float3 wi = normalize(mul_mat3_f3(toLocal , normalize(lightPos - vertexPos)));
    // Observer direction
    float3 wo = normalize(mul_mat3_f3(toLocal , normalize(cameraPos)));
    
    float3 radiance_glint = to_float3_s(0.0f);
    float3 radiance_diffuse = to_float3_s(0.0f);
    float3 radiance = to_float3_s(0.0f);
    
    float distanceSquared = distance_f3(vertexPos, lightPos);
    distanceSquared *= distanceSquared;
    float3 Li = lightIntensity / distanceSquared;
    
    radiance_diffuse = f_diffuse(wo, wi) * Li;
    
    // Call our physically based glinty BRDF
    radiance_glint = f_P(wo, wi, uv, fragCoord, iResolution) * Li;
    
    radiance = 0.33f*radiance_diffuse + to_float3(0.13f,0.0f,0.0f);
        
    radiance += 0.33f*radiance_glint;
    if(VARNISHED){
        radiance += 0.1f * f_specular(wo, wi) * Li;
    }
   
    // Gamma
    radiance = pow_f3(radiance, to_float3_s(1.0f / 2.2f));

    // Output to screen
    fragColor = to_float4_aw(radiance, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}