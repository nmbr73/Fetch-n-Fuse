// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


// these I just threw together over in https://shadertoy.com/view/wtVSDw
// but they seem to work, or at least seem to be inverses of each other.

__DEVICE__ int CubeFaceOfDir(float3 d) // just the face id
{
    float3 a = _fabs(d);
    int f = a.x >= a.y ? (a.x >= a.z ? 0 : 2) : (a.y >= a.z ? 1 : 2);
    int i = f + f;
    if (d[f] < 0.0f) ++i;
    return i;
}
// takes normalized direction vector, returns uv in swi2(c,x,y) and face id in c.z
__DEVICE__ float3 DirToCubeFace(float3 d)
{
    int i = CubeFaceOfDir(d)
    , f = i >> 1;
    float2 uv;
    switch (f) {
        case 0: uv = swi2(d,y,z); break;
        case 1: uv = swi2(d,x,z); break;
        case 2: uv = swi2(d,x,y); break;
    }
    uv /= _fabs(d[f]); // project
    if ((i&1) != 0) // negative faces are odd indices
        uv.x = -uv.x; // flip u
    return to_float3_aw(uv, float(i));
}
// takes uv in swi2(c,x,y) and face id in c.z, returns unnormalized direction vector
__DEVICE__ float3 CubeFaceToDir(float3 c)
{
    int i = int(c.z);
    float3 d = to_float3(c.x,c.y, 1.0f - 2.0f * float(i & 1));
    d.x *= d.z; // only unflip u
    switch (i >> 1) { // f
        case 0: d = swi3(d,z,x,y); break;
        case 1: d = swi3(d,x,z,y); break;
        case 2: d = swi3(d,x,y,z); break;
    }
    return d; // needs normalized probably but texture() doesn't mind.
}

// just for debugging so probably broken and imprecise.
// in fact it's a big ol' kludge atm.  what a mess!  I'll try to improve it as I get time.
__DEVICE__ float4 Unwrap(__TEXTURE2D__ ch, float2 q)
{
    float2 uv = q * 0.5f + 0.5f;
    uv *= 4.0f;
    uv -= to_float2(0.0f,0.5f);
    int i = -1;
    if (uv.y >= 1.0f && uv.y < 2.0f) {
        int f = int(_floor(uv.x));
        if (f >= 0 && f < 2) i = 3*f + 1;
       else if (f >= 2 && f < 4) i = 5*f - 10;
        if (f == 2) uv = to_float2(uv.y, -uv.x); // maybe rotate, different directions
        else if (f == 0) uv = to_float2(-uv.y, uv.x);
    } else {
    if (int(uv.x) == 1) {
          if (uv.y >= 0.0f && uv.y < 1.0f) { i = 3; uv.x = 0.0f-uv.x; }
          else if (uv.y >= 2.0f && uv.y < 3.0f) { i = 2; uv.y = 0.0f-uv.y; }
      }
    }
  if (!(i >= 0)) return to_float4_aw(to_float3_s(0.7f),1);
    uv = fract(uv);
    float3 d = CubeFaceToDir(to_float3_aw(uv * 2.0f - 1.0f, float(i)));
//    d = CubeFaceToDir(DirToCubeFace(d)); // ensure can convert back&forth flawlessly
//    d = CubeFaceToDir(DirToCubeFace(d));
    //float4 c = textureLod(ch, d, 0.0f);
    float4 c = cube_texture_f3(ch,d);
    //swi3(c,x,y,z) = _powf(swi3(c,x,y,z), to_float3_s(2.2f)); // gamma correction - skipping as it cancels out here
    return c;
} // result in srgb gamma atm


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect 'Cubemap: Uffizi Gallery_0' to iChannel2
// Connect 'Cubemap: St Peters Basilica_0' to iChannel1
// Connect 'Cubemap: St Peters Basilica Blurred_0' to iChannel0
// Connect 'Cubemap: Forest_0' to iChannel3


// I expect someone has done this better, faster, cheaper somewhere
// and I wouldn't mind seeing how, but I didn't see any other
// examples of such cubemap face debugging tool here on the site,
// so I took the first thing I got working and made a toy out of it.
// Hopefully someone will find it useful.  I'd appreciate any tips.
// heck I probably got the face id's wrong or uv's backward or upside down.

// Hey!  I did find something related, finally:  https://shadertoy.com/view/3l2SDR
// Fabrice has some coordinate conversion code:  https://shadertoy.com/view/WdlGRr
// Wunkolo has some stuff here I hope to grok:   https://shadertoy.com/view/wltXDl

//#define CUBEMAP iChannel1 // pick a channel


__KERNEL__ void CubemapFaceUnwrapFuse(float4 c, float2 p, float2 iResolution, sampler2D iChannel0)
{

    float2 R = iResolution
        , q = (p + p - R)/R.y;
    //swi3(c,x,y,z) = Unwrap(CUBEMAP, q * 0.755f).rgb;
    c = Unwrap(iChannel0, q * 0.755f);
//    swi3(c,x,y,z) = _powf(swi3(c,x,y,z), to_float3_aw(1.0f/2.2f)); // gamma (disable for texture cube sources, or fix those on load)
    c.w = 1.0f;



  SetFragmentShaderComputedColor(c);
}