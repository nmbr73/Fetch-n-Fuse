

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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
uint[] pg2020_bitfield = uint[]( 0x0u,0x0u,0x0u,0x003e7c00u,0x00024400u,0x00327c00u,0x00220400u,0x003e0400u,0x0u,0x0u,0x30e30e0u,0x4904900u,0x49e49e0u,0x4824820u,0x31e31e0u,0x0u,0x0u,0x0u );
bool jfig(in uint x, in uint y) {
    uint id = x + (PG2020H-1u-y)*PG2020W;
    if (id>=PG2020W*PG2020H) return false;
    return 0u != (pg2020_bitfield[id/32u] & (1u << (id&31u)));
}

float texel(int s, int t){
    if(s < 0 || s >= int(PG2020W) || t < 0 || t >= int(PG2020H) || jfig(uint(s), uint(t)))
    	return 0.;
    
    return 1.;
}

float pg2020TriangleFilter(vec2 st){
	float s = st.x * float(PG2020W) - 0.5;
    float t = st.y * float(PG2020H) - 0.5;
    int s0 = int(floor(s));
    int t0 = int(floor(t));
    float ds = s - float(s0);
    float dt = t - float(t0);
    return (1. - ds) * (1. - dt) * texel(s0, t0) +
           (1. - ds) * dt * texel(s0, t0 + 1) +
           ds * (1. - dt) * texel(s0 + 1, t0) +
           ds * dt * texel(s0 + 1, t0 + 1);
}

//=========================================================================================================================
//================================================== Material parameters ==================================================
//==================================================== Can be changed =====================================================
//=========================================================================================================================
// Roughness of the glinty material [0.1, 1.]
#define ALPHA_X 0.5
#define ALPHA_Y 0.5

// Microfacet relative area [0.01, 1.]. 
// Set to 0.01 with LOGMICROFACETDENSITY set tot 5. gives sparse glints (snow, sand, sparkling rocks)
#define MICROFACETRELATIVEAREA 1.

// Logarithmic microfacet density [5., 25.]
#define LOGMICROFACETDENSITY 14.

// Maximum anisotropy of the pixel footprint (not realy usefull in this scene)
#define MAXANISOTROPY 8.

// Varnished material (add a specular lobe with a small roughness)
# define VARNISHED true

//=========================================================================================================================
//============================================== Parameters of the dictionary =============================================
//=========================================================================================================================
// Roughness used during the dictionary generation
#define ALPHA_DIC 0.5
// Number of distributions
// In the paper, we use 192 marginal distributions of slope
// In shadertoy, we generate the dictionary on the fly. So we use a large number of different NDFs
#define N 999999
// Number of levels. In the paper : 16. In shadertoy : 8
#define NLEVELS 8
// Size of the tabulated marginal distributions. In the paper : 64. In shadertoy : 32
#define DISTRESOLUTION 32



//=========================================================================================================================
//================================================ Mathematical constants =================================================
//=========================================================================================================================
#define PI 3.141592
#define IPI 0.318309
#define ISQRT2 0.707106



//=========================================================================================================================
//=============================================== Beckmann anisotropic NDF ================================================
//==================== Shadertoy implementation : Arthur Cavalier (https://www.shadertoy.com/user/H4w0) ===================
//========================================= https://www.shadertoy.com/view/WlGXRt =========================================
//=========================================================================================================================

//-----------------------------------------------------------------------------
//-- Beckmann Distribution ----------------------------------------------------
float p22_beckmann_anisotropic(float x, float y, float alpha_x, float alpha_y)
{
    float x_sqr = x*x;
    float y_sqr = y*y;
    float sigma_x = alpha_x * ISQRT2;
    float sigma_y = alpha_y * ISQRT2;
    float sigma_x_sqr = sigma_x*sigma_x;
    float sigma_y_sqr = sigma_y*sigma_y;
    return( 
            exp( -0.5 * ((x_sqr/sigma_x_sqr) + (y_sqr/sigma_y_sqr)) )
    / //-------------------------------------------------------------------
                    ( 2. * PI * sigma_x * sigma_y )
    );
}

float ndf_beckmann_anisotropic(vec3 omega_h, float alpha_x, float alpha_y)
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
vec3 fresnel_schlick(in float wo_dot_wh, in vec3 F0)
{
    return F0 + (1. - F0) * pow(1. - wo_dot_wh, 5.);
}

//=========================================================================================================================
//===================================== Microfacet BRDF of Cook and Torrance 1982 =========================================
//=========================================================================================================================
vec3 f_specular(vec3 wo, vec3 wi)
{
    if(wo.z <= 0.) return vec3(0.,0.,0.);
    if(wi.z <= 0.) return vec3(0.,0.,0.);
    vec3 wh = normalize(wo+wi);
    if(wh.z <= 0.) return vec3(0.,0.,0.);
    // Local masking shadowing
    if (dot(wo, wh) <= 0. || dot(wi, wh) <= 0.) return vec3(0.);
    float wi_dot_wh = clamp(dot(wi,wh),0.,1.);

    float D = ndf_beckmann_anisotropic(wh,0.1, 0.1);
    // V-cavity masking shadowing
    float G1wowh = min(1., 2. * wh.z * wo.z / dot(wo, wh));
    float G1wiwh = min(1., 2. * wh.z * wi.z / dot(wi, wh));
    float G = G1wowh * G1wiwh;
    
	vec3 F  = fresnel_schlick(wi_dot_wh,vec3(1., 1., 1.));
        
    return (D * F * G) / ( 4. * wo.z );
}

//=========================================================================================================================
//=============================================== Diffuse Lambertian BRDF =================================================
//=========================================================================================================================
vec3 f_diffuse(vec3 wo, vec3 wi)
{
    if (wo.z <= 0.)
        return vec3(0., 0., 0.);
    if (wi.z <= 0.)
        return vec3(0., 0., 0.);

    return vec3(0.8, 0., 0.) * IPI * wi.z;
}

//=========================================================================================================================
//=============================================== Inverse error function ==================================================
//=========================================================================================================================
float erfinv(float x)
{
    float w, p;
    w = -log((1.0 - x) * (1.0 + x));
    if (w < 5.000000)
    {
        w = w - 2.500000;
        p = 2.81022636e-08;
        p = 3.43273939e-07 + p * w;
        p = -3.5233877e-06 + p * w;
        p = -4.39150654e-06 + p * w;
        p = 0.00021858087 + p * w;
        p = -0.00125372503 + p * w;
        p = -0.00417768164 + p * w;
        p = 0.246640727 + p * w;
        p = 1.50140941 + p * w;
    }
    else
    {
        w = sqrt(w) - 3.000000;
        p = -0.000200214257;
        p = 0.000100950558 + p * w;
        p = 0.00134934322 + p * w;
        p = -0.00367342844 + p * w;
        p = 0.00573950773 + p * w;
        p = -0.0076224613 + p * w;
        p = 0.00943887047 + p * w;
        p = 1.00167406 + p * w;
        p = 2.83297682 + p * w;
    }
    return p * x;
}

//=========================================================================================================================
//================================================== Hash function ========================================================
//================================================== Inigo Quilez =========================================================
//====================================== https://www.shadertoy.com/view/llGSzw ============================================
//=========================================================================================================================
// 
float hashIQ(uint n)
{
    // integer hash copied from Hugo Elias
    n = (n << 13U) ^ n;
    n = n * (n * n * 15731U + 789221U) + 1376312589U;
    return float(n & 0x7fffffffU) / float(0x7fffffff);
}

//=========================================================================================================================
//=============================================== Pyramid size at LOD level ===============================================
//=========================================================================================================================
int pyramidSize(int level)
{
    return int(pow(2., float(NLEVELS - 1 - level)));
}

float normalDistribution1D(float x, float mean, float std_dev) {
    float xMinusMean = x - mean;
    float xMinusMeanSqr = xMinusMean * xMinusMean;
    return exp(-xMinusMeanSqr / (2. * std_dev * std_dev)) /
           (std_dev * 2.506628);
    // 2.506628 \approx sqrt(2 * \pi)
}

//=========================================================================================================================
//========================================= Sampling from a Normal distribution ===========================================
//=========================================================================================================================
float sampleNormalDistribution(float U, float mu, float sigma)
{
    float x = sigma * 1.414213f * erfinv(2.0f * U - 1.0f) + mu;
    return x;
}

//=========================================================================================================================
//==================================== Evaluation of the i th marginal distribution P =====================================
//========================================== with slope x and at LOD level ================================================
//============================ Procedural version, because we cannot use data with Shadertoy ==============================
//=========================================================================================================================

float P_procedural(float x, int i, int level) {
    
    // We use even functions
    x = abs(x);
    // After 4 standard deviation sigma, we consider that the distribution equals zero
    float sigma_dist_4 = 4. * ALPHA_DIC / 1.414214; // alpha_dist = 0.5 so sigma_dist \approx 0.3535 (0.5 / sqrt(2))
    if(x >= sigma_dist_4) return 0.;
    
    int nMicrofacetsCurrentLevel = int(pow(2., float(level)));
    float density = 0.;
    // Dictionary should be precomputed, but we cannot use memory with Shadertoy
    // So we generate it on the fly with a very limited number of lobes
    nMicrofacetsCurrentLevel = min(16, nMicrofacetsCurrentLevel);
    
    for (int n = 0; n < nMicrofacetsCurrentLevel; ++n) {
        
        float U_n = hashIQ(uint(i*7333+n*5741));
        // alpha roughness equals sqrt(2) * RMS roughness
        //     ALPHA_DIC     =   1.414214 * std_dev
        // std_dev = ALPHA_DIC / 1.414214 
        float currentMean = sampleNormalDistribution(U_n, 0., ALPHA_DIC / 1.414214);
        density += normalDistribution1D(x, currentMean, 0.05) +
                   normalDistribution1D(-x, currentMean, 0.05);
    }
    return density / float(nMicrofacetsCurrentLevel);
}

//=========================================================================================================================
//=================== Spatially-varying, multiscale, rotated, and scaled slope distribution function ======================
//================================================= Eq. 11, Alg. 3 ========================================================
//=========================================================================================================================
float P22_theta_alpha(vec2 slope_h, int l, int s0, int t0)
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
    float n = pow(2., float(2 * l - (2 * (NLEVELS - 1))));
    n *= exp(LOGMICROFACETDENSITY);

    // Corresponding continuous distribution LOD
    // Alg. 3, line 6
    float l_dist = log(n) / 1.38629; // 2. * log(2) = 1.38629
    
    // Alg. 3, line 7
    float uDensityRandomisation = hashIQ(uint(rngSeed) * 2171U);

    // Fix density randomisation to 2 to have better appearance
    // Notation in the paper: \zeta
    float densityRandomisation = 2.;
    
    // Sample a Gaussian to randomise the distribution LOD around the distribution level l_dist
    // Alg. 3, line 8
    l_dist = sampleNormalDistribution(uDensityRandomisation, l_dist, densityRandomisation);

    // Alg. 3, line 9
    int l_disti = clamp(int(round(l_dist)), 0, NLEVELS);

    // Alg. 3, line 10
    if (l_disti == NLEVELS)
        return p22_beckmann_anisotropic(slope_h.x, slope_h.y, ALPHA_X, ALPHA_Y);

    // Alg. 3, line 13
    float uTheta = hashIQ(uint(rngSeed));
    float theta = 2.0 * PI * uTheta;

    // Uncomment to remove random distribution rotation
    // Lead to glint alignments with a small N
    // theta = 0.;

    float cosTheta = cos(theta);
    float sinTheta = sin(theta);
    
    vec2 scaleFactor = vec2(ALPHA_X / ALPHA_DIC,
                            ALPHA_Y / ALPHA_DIC);

    // Rotate and scale slope
    // Alg. 3, line 16
    slope_h = vec2(slope_h.x * cosTheta / scaleFactor.x + slope_h.y * sinTheta / scaleFactor.y,
                   -slope_h.x * sinTheta / scaleFactor.x + slope_h.y * cosTheta / scaleFactor.y);

    // Alg. 3, line 17
    float u1 = hashIQ(uint(rngSeed) * 16807U);
    float u2 = hashIQ(uint(rngSeed) * 48271U);

    // Alg. 3, line 18
    int i = int(u1 * float(N));
    int j = int(u2 * float(N));
    
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

float P22_floorP(int l, vec2 slope_h, vec2 st, vec2 dst0, vec2 dst1)
{
    // Convert surface coordinates to appropriate scale for level
    float pyrSize = float(pyramidSize(l));
    st[0] = st[0] * pyrSize - 0.5f;
    st[1] = st[1] * pyrSize - 0.5f;
    dst0[0] *= pyrSize;
    dst0[1] *= pyrSize;
    dst1[0] *= pyrSize;
    dst1[1] *= pyrSize;

    // Compute ellipse coefficients to bound filter region
    float A = dst0[1] * dst0[1] + dst1[1] * dst1[1] + 1.;
    float B = -2. * (dst0[0] * dst0[1] + dst1[0] * dst1[1]);
    float C = dst0[0] * dst0[0] + dst1[0] * dst1[0] + 1.;
    float invF = 1. / (A * C - B * B * 0.25f);
    A *= invF;
    B *= invF;
    C *= invF;

    // Compute the ellipse's bounding box in texture space
    float det = -B * B + 4. * A * C;
    float invDet = 1. / det;
    float uSqrt = sqrt(det * C), vSqrt = sqrt(A * det);
    int s0 = int(ceil(st[0] - 2. * invDet * uSqrt));
    int s1 = int(floor(st[0] + 2. * invDet * uSqrt));
    int t0 = int(ceil(st[1] - 2. * invDet * vSqrt));
    int t1 = int(floor(st[1] + 2. * invDet * vSqrt));

    // Scan over ellipse bound and compute quadratic equation
    float sum = 0.f;
    float sumWts = 0.;
    int nbrOfIter = 0;

    for (int it = t0; it <= t1; ++it)
    {
        float tt = float(it) - st[1];
        for (int is = s0; is <= s1; ++is)
        {
            float ss = float(is) - st[0];
            // Compute squared radius and filter SDF if inside ellipse
            float r2 = A * ss * ss + B * ss * tt + C * tt * tt;
            if (r2 < 1.)
            {
                // Weighting function used in pbrt-v3 EWA function
                float alpha = 2.;
                float W_P = exp(-alpha * r2) - exp(-alpha);
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
vec3 f_P(vec3 wo, vec3 wi, vec2 uv)
{
	
    if (wo.z <= 0.)
        return vec3(0., 0., 0.);
    if (wi.z <= 0.)
        return vec3(0., 0., 0.);

    // Alg. 1, line 1
    vec3 wh = normalize(wo + wi);
    if (wh.z <= 0.)
        return vec3(0., 0., 0.);

    // Local masking shadowing
    if (dot(wo, wh) <= 0. || dot(wi, wh) <= 0.)
        return vec3(0.);

    // Eq. 1, Alg. 1, line 2
    vec2 slope_h = vec2(-wh.x / wh.z, -wh.y / wh.z);

    vec2 texCoord = uv;

    float D_P = 0.;
    float P22_P = 0.;

    // ------------------------------------------------------------------------------------------------------
    // Similar to pbrt-v3 MIPMap::Lookup function, http://www.pbr-book.org/3ed-2018/Texture/Image_Texture.html#EllipticallyWeightedAverage

    // Alg. 1, line 3
    vec2 dst0 = dFdx(texCoord);
    vec2 dst1 = dFdy(texCoord);

    // Compute ellipse minor and major axes
    float dst0Length = length(dst0);
    float dst1Length = length(dst1);

    if (dst0Length < dst1Length)
    {
        // Swap dst0 and dst1
        vec2 tmp = dst0;
        dst0 = dst1;
        dst1 = tmp;
    }
    float majorLength = length(dst0);
    // Alg. 1, line 5
    float minorLength = length(dst1);

    // Clamp ellipse eccentricity if too large
    // Alg. 1, line 4
    if (minorLength * MAXANISOTROPY < majorLength && minorLength > 0.)
    {
        float scale = majorLength / (minorLength * MAXANISOTROPY);
        dst1 *= scale;
        minorLength *= scale;
    }
    // ------------------------------------------------------------------------------------------------------

    // Without footprint, we evaluate the Cook Torrance BRDF
    if (minorLength == 0.)
    {
        D_P = ndf_beckmann_anisotropic(wh, ALPHA_X, ALPHA_Y);
    }
    else
    {
        // Choose LOD
        // Alg. 1, line 6
        float l = max(0., float(NLEVELS) - 1. + log2(minorLength));
        int il = int(floor(l));

        // Alg. 1, line 7
        float w = l - float(il);

        // Alg. 1, line 8
        P22_P = mix(P22_floorP(il, slope_h, texCoord, dst0, dst1),
                    P22_floorP(il + 1, slope_h, texCoord, dst0, dst1),
                    w);

        // Eq. 13, Alg. 1, line 10
        D_P = P22_P / (wh.z * wh.z * wh.z * wh.z);
    }

    // V-cavity masking shadowing
    float G1wowh = min(1., 2. * wh.z * wo.z / dot(wo, wh));
    float G1wiwh = min(1., 2. * wh.z * wi.z / dot(wi, wh));
    float G = G1wowh * G1wiwh;

    // Fresnel is set to one for simplicity here
    // but feel free to use "real" Fresnel term
    vec3 F = vec3(1., 1., 1.);

    // Eq. 14, Alg. 1, line 11
    // (wi dot wg) is cancelled by
    // the cosine weight in the rendering equation
    return (F * G * D_P) / (4. * wo.z);
}

//=========================================================================================================================
//===================================================== Renderer ==========================================================
//=========================================================================================================================
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Light intensity
    vec3 lightIntensity = vec3(50000.);
    
    // Texture position
    vec2 uv = fragCoord/iResolution.y * 400.;
    
    // Vertex position
    vec3 vertexPos = vec3(fragCoord - iResolution.xy/2., 0.);
    
    // Light position (varies over time)
    float x_i = cos(iTime*0.6) * iResolution.x / 2.;
    float y_i = cos(iTime) * iResolution.y / 2.;
    vec3 lightPos = vec3(x_i, y_i, 100);
    
    // Camera position
    vec3 cameraPos = vec3(0, 0, 100);
    
    // Compute normal from JFIG heightfield
    float diff = 10.;
    float hJFIGsm1t0 = pg2020TriangleFilter(vec2((fragCoord.x - diff)/iResolution.x, (fragCoord.y)/iResolution.y));
    float hJFIGs1t0 = pg2020TriangleFilter(vec2((fragCoord.x + diff)/iResolution.x, (fragCoord.y)/iResolution.y));
    float hJFIGs0tm1 = pg2020TriangleFilter(vec2((fragCoord.x)/iResolution.x, (fragCoord.y - diff)/iResolution.y));
    float hJFIGs0t1 = pg2020TriangleFilter(vec2((fragCoord.x)/iResolution.x, (fragCoord.y + diff)/iResolution.y));
    vec2 slope = vec2((hJFIGs1t0 - hJFIGsm1t0)/2.,
                      (hJFIGs0t1 - hJFIGs0tm1)/2.);
    slope *= 4.;
    vec3 vertexNormal = vec3(-slope.x, -slope.y, 1.) / sqrt(slope.x*slope.x+slope.y*slope.y+1.);
    
    vec3 vertexTangent = vec3(1., 0., 0.);
    // Gram–Schmidt process
    vertexTangent = vertexTangent - (dot(vertexNormal, vertexTangent) / dot(vertexNormal, vertexNormal)) * vertexNormal;
    vec3 vertexBinormal = cross(vertexNormal, vertexTangent);
    
    // Matrix for transformation to tangent space
    mat3 toLocal = mat3(
        vertexTangent.x, vertexBinormal.x, vertexNormal.x,
        vertexTangent.y, vertexBinormal.y, vertexNormal.y,
        vertexTangent.z, vertexBinormal.z, vertexNormal.z ) ;
    
    // Incident direction
    vec3 wi = normalize(toLocal * normalize(lightPos - vertexPos));
    // Observer direction
    vec3 wo = normalize(toLocal * normalize(cameraPos));
    
    vec3 radiance_glint = vec3(0.);
    vec3 radiance_diffuse = vec3(0.);
    vec3 radiance = vec3(0.);
    
    float distanceSquared = distance(vertexPos, lightPos);
    distanceSquared *= distanceSquared;
    vec3 Li = lightIntensity / distanceSquared;
    
    radiance_diffuse = f_diffuse(wo, wi) * Li;
    
    // Call our physically based glinty BRDF
    radiance_glint = f_P(wo, wi, uv) * Li;
    
    radiance = 0.33*radiance_diffuse + vec3(0.13f,0.,0.);
        
    radiance += 0.33*radiance_glint;
    if(VARNISHED){
        radiance += 0.1 * f_specular(wo, wi) * Li;
    }
   
    // Gamma
    radiance = pow(radiance, vec3(1.0 / 2.2));

    // Output to screen
    fragColor = vec4(radiance, 1.0);
}