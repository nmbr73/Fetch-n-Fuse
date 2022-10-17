
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define T iTime
#define R iResolution
#define LOWRES 320.0f
#define SAT(a) clamp(a, 0.0f,1.0f)
#define S(a, b, c) smoothstep(a, b, c)
#define M(a) to_mat2(_cosf(a), -_sinf(a), _sinf(a), _cosf(a))
#define PI _acosf(-1.0f)
#define TWO_PI (PI * 2.0f)


/////////////////////////////////////////////
// hash2 taken from Dave Hoskins 
// https://www.shadertoy.com/view/4djSRW
/////////////////////////////////////////////

__DEVICE__ float hash2(float2 p)
{  
  float3 p3  = fract_f3((swi3(p,x,y,x)) * 0.2831f);
  p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
  return fract((p3.x + p3.y) * p3.z);
}


/////////////////////////////////////////////
// hash and noise function by Inigo Quilez
/////////////////////////////////////////////

__DEVICE__ float hash( float n ) { 
  return fract(_sinf(n)*75728.5453123f); 
}


__DEVICE__ float noise( in float2 x ) {
    float2 p = _floor(x);
    float2 f = fract_f2(x);
    f = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f;
    return _mix(_mix( hash(n + 0.0f), hash(n + 1.0f), f.x), _mix(hash(n + 57.0f), hash(n + 58.0f), f.x), f.y);
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------


// copacabana pattern + noise distortion + rot
// https://www.shadertoy.com/view/MsXBzB

__KERNEL__ void SoundspectorJipiFuse__Buffer_A(float4 C, float2 U, float2 iResolution, float iTime, float4 iDate)
{
    U+=0.5f;
float AAAAAAAAAAAAAAAAA;  
    U = (U -0.5f * R)/R.x;
    U = mul_f2_mat2(U,M(T * 0.1f));
    U += 0.4f * length(U-0.5f) * noise(U * 4.0f);
    U *= 70.0f;   
    U.y -= _cosf(U.x) + iTime;//iDate.w ; 
    C = to_float4_s(smoothstep(0.5f,
                               1.0f,
                               _mix(_cosf(U.x) * _sinf(U.y),
                               0.3f,
                               _cosf(U.y) * 0.5f) * 8.0f));
        
  SetFragmentShaderComputedColor(C);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/894a09f482fb9b2822c093630fc37f0ce6cfec02b652e4e341323e4b6e4a4543.mp3' to iChannel0
// Connect Image 'Previsualization: Buffer A' to iChannel1


// "SoundSpector"
// by Julien Vergnaud @duvengar-2018
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
//////////////////////////////////////////////////////////////////////////////////////

#define CEL 64.0f                  // How many buckets to divide spectrum into

// the original accurate spectrum was taken from 
// https://www.shadertoy.com/view/lt2fRz by 834144373

#define barSize 1.0f / CEL        // Constant to avoid division in main loop
#define sampleSize 0.2f           // How accurately to sample spectrum

// iq's cosine palette function
__DEVICE__ float3 palette( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}


// logo distance function written specially for the occasion :)
__DEVICE__ float rect(float2 uv,float2 p, float w, float h, float blur){
    
    uv += p;
    float rv = S(w, w + blur, _fabs(uv.x));//length(uv.x));
    float rh = S(h, h + blur, _fabs(uv.y));//length(uv.y));
    return SAT( rv + rh);
}

__DEVICE__ float logo(float2 uv, float blur){    
    uv += to_float2(0.001f,0.008f);
    uv.y *= 1.15f;

    float as = blur;
    // first make 4 circles
    float s1 = S(0.011f - as, 0.011f, length(uv - to_float2(-0.035f,0.0f)));
    float s2 = S(0.019f - as, 0.019f, length(uv - to_float2(-0.015f,0.01f)));
    float s3 = S(0.023f - as, 0.023f, length(uv - to_float2(0.01f,0.02f)));
    float s4 = S(0.0142f - as, 0.0142f, length(uv - to_float2(0.035f,0.0022f)));
    // then a rectangle
    float r1 = rect(uv, to_float2(0.0f,-0.0012f), 0.032f, 0.0101f, as);
    // make vertical lines stripes for part off the screen
    uv = uv.x > -0.0072f ? uv : fract(uv * 220.0f);
    float lines = S(0.7f, 0.7f + as, _fabs(uv.x + 0.25f));//length(uv.x + 0.25f));
    // merge circles together
    float shape =  SAT(s1 * s2 * s3 * s4 * r1); 
    // and finally substract the line from the shape
    shape = 1.0f - _fmaxf(shape, lines);
    
    return SAT(shape);
}

__KERNEL__ void SoundspectorJipiFuse(float4 fragColor, float2 U, float2 iResolution, float iTime, sampler2D iChannel0, sampler2D iChannel1)
{

    CONNECT_COLOR0(Color1, 0.5f, 0.5f, 0.0f, 1.0f);
    CONNECT_COLOR1(Color2, 0.5f, 0.5f, 0.35f, 1.0f);
    CONNECT_COLOR2(Color3, 0.9f, 0.5f, 0.3f, 1.0f);
    CONNECT_COLOR3(ColorBKG, 0.1f, 0.15f, 0.3f, 1.0f);

    CONNECT_SLIDER0(Grain, 0.0f, 1.0f, 0.1f);
    CONNECT_SLIDER1(Vignette, 0.0f, 10.0f, 1.1f);

    U+=0.5f;
    // DOMAIN
    float2 uv = (U -0.5f * R)/R.x;
    float2 uv2  = 1.0f - 2.0f * U / R;                       // additionnal domain for post processing effect       
    float2 st = uv;                            // store original domain before distortion
    uv = mul_f2_mat2(uv,M(TWO_PI / 8.0f * _cosf(length(st)) * 20.0f));      // rotate and twist domain 
    float2 pol = to_float2(_atan2f(uv.x, uv.y), length(uv));     // polar coordinates
    float alfa = pol.x / TWO_PI + 0.5f;                  // full polar domain from -PI to PI
    
    uv = to_float2(alfa , pol.y);                           // convert domain to polar coordinates
         
    // SPECTRUM
    float barStart = _floor(uv.x * CEL) / CEL;          // spectrum buckets id's
    float amp = 0.0f;  
    amp -= texture(iChannel0,to_float2(400.0f,0.0f)).x;      // store global sound amplitude
    
    float intensity = 0.0f;                              // loop get all buckets intensity
    
    for(float i = 0.0f; i < barSize; i += barSize * sampleSize)
    {      
      intensity += 0.9f * texture(iChannel0, to_float2(barStart + i, 0.0f)).x;
    }
    
    intensity *= sampleSize;                           // accurate intensity
    intensity  = clamp(intensity + 0.1f, 0.29f, 1.0f);       // apply floor on lower intensities   
    float height  = S(0.0f,0.005f, intensity - uv.y * 3.0f);   // height of the bucket
    
    
    // SHADING                                         // I'm using iq's cosine color palette for coloring
    float3 pal = palette(intensity -0.2f,
                         swi3(Color1,x,y,z),//to_float3(0.5f, 0.5f, 0.0f),
                         swi3(Color2,x,y,z),//to_float3(0.5f,0.5f,0.35f),
                         swi3(Color3,x,y,z),//to_float3(0.9f,0.5f,0.3f),
                         to_float3(intensity * length(uv) + uv.y, intensity * length(uv) + uv.y, 1.0f));
    
    float4 spectrum =  to_float4_aw(SAT(pal) * height, 1.0f);               // final colored spectrum
    float lines  = S(0.2f, 0.4f, 0.85f* _fabs(fract(uv.x * CEL)-0.5f));     // lines gap to be substrated from the spectrum
    float center = 1.0f- S(0.09f, 0.091f, length(st));                      // circle to be substrated from the spectrum
    float c0     = 1.0f- S(0.082f, 0.083f, length(st));                     // highlight circle of the logo bakground
    float4 bg = ColorBKG;//to_float4(0.1f,0.15f,0.3f, 1.0f);                           // base color bakground
    float3 pat  = swi3(texture(iChannel1, U / R),x,y,z);                    // get image texture from buffer A
    
    bg      += -amp/3.0f+0.1f * to_float4_aw(_fmaxf(pat,to_float3_s(center)), 1.0f);     // connect bg luminosity to global amplitude
    bg      += 0.23f * S(0.03f,0.0f,length(st));                            // hightlight middle of screen
    float lns = S(0.5f,0.51f,_fabs(fract(pol.y*250.0f)));                  // radial lines pattern applied on bg
    bg -= _fabs(pol.y*0.1f) * lns;                       
    bg += 0.05f * _fabs( (st.y + 0.08f)* 30.0f) * c0;                      // apply highlight on logo circle bg
    float4 col = _fminf((spectrum),(spectrum) - (lines + center)) ;         //substract middle of the spectrum
    col = _mix( SAT(bg), SAT(3.0f * col), 0.5f);
        
    // POSTPROD
    col += Grain * hash2(T + st * to_float2(1462.439f, 297.185f));           // animated grain (hash2 function in common tab)
    col *= Vignette * to_float4_s(0.9f - S(0.0f, 2.2f, length(uv2*uv2)));       // vigneting
    col = pow_f4(col, to_float4_s(1.3f));                                   // gamma
    
    // LOGO
    col = 1.1f * col + logo(st * (0.9f+amp*0.4f), 0.003f ) * 4.0f ;          // add soundcloud logo
    col += (0.075f*amp)* logo((st- to_float2(0.0f,-0.009f)) * (0.75f + amp *0.1f), 0.004f) * 2.0f ;   // add soundcloud logo shadows
    
col.w=ColorBKG.w;
    
    fragColor = col ;
    
    
  SetFragmentShaderComputedColor(fragColor);
}