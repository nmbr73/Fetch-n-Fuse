
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

// neighboor collection (not used)
__DEVICE__ float4 N(__TEXTURE2D__ sam, float2 u, float2 iR)
{
    float4 r = to_float4_s(0);

    r += texture(sam, (u+to_float2(+0.0f, +1.0f) )/iR );
    r += texture(sam, (u+to_float2(+0.0f, -1.0f) )/iR );
    r += texture(sam, (u+to_float2(+1.0f, -0.0f) )/iR );
    r += texture(sam, (u+to_float2(-1.0f, +0.0f) )/iR );
    return r;
}

// grayscale
#define g(c) (0.3f*c.x + 0.59f*c.y + 0.11f*c.z) 

__DEVICE__ float2 cs(float a) { return to_float2(_cosf(a), _sinf(a)); }

//code shortcuts
#define NA(u) N(iChannel0, u, R)
#define NB(u) N(iChannel1, u, R)
#define NC(u) N(iChannel2, u, R)
#define ND(u) N(iChannel3, u, R)

#define A(u) _tex2DVecN(iChannel0,(u).x,(u).y,15)
#define B(u) _tex2DVecN(iChannel1,(u).x,(u).y,15)
#define C(u) _tex2DVecN(iChannel2,(u).x,(u).y,15)
#define D(u) _tex2DVecN(iChannel3,(u).x,(u).y,15)
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


/*
** Following this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/



__DEVICE__ float4 F(float2 p, float2 R, __TEXTURE2D__ iChannel0)
{
  float2 r = swi2(A(p/R),x,y);
  r = p - r;
  return A(r/R);//-NA(r/R)*0.25f;
}

__KERNEL__ void FluidThingy0Jipi409Fuse__Buffer_A(float4 o, float2 f, float2 iResolution, float4 iMouse, int iFrame)
{

    f+=0.5f;
float AAAAAAAAAAAAAAAAAAA;
    const float2 n = to_float2(+0.0f, +1.0f);
    const float2 s = -1.0f*n;
    const float2 w = to_float2(+1.0f, +0.0f);
    const float2 e = -1.0f*w;

    if (f.x < 10.0f || f.y < 10.0f || -f.x+R.x < 10.0f || -f.y+R.y < 10.0f) {o = to_float4_s(0.0f); SetFragmentShaderComputedColor(o); return;}

    float kb = C(to_float2(32.5f/256.0f, 0.25f)).x;
    if (kb > 0.5f || iFrame < 10) {o = to_float4(0.0f,0.0f,0.0f, (g(B(f/R))*2.0f-1.0f)*0.5f ); SetFragmentShaderComputedColor(o); return;}

    o = F(f,R,iChannel0);//-NA(f/R)*0.25f;
    float4 En = F(f+n,R,iChannel0);
    float4 Es = F(f+s,R,iChannel0);
    float4 Ew = F(f+w,R,iChannel0);
    float4 Ee = F(f+e,R,iChannel0);

    o.z = (En + Es + Ew + Ee).z * 0.25f;//06125;

    //swi2(o,x,y) += to_float2(Ee.z - Ew.z, Es.z - En.z) * 0.25f;
    o.x += (Ee.z - Ew.z) * 0.25f;
    o.y += (Es.z - En.z) * 0.25f;

    o.z += (Es.y - En.y + Ee.x - Ew.x) *0.25f;

    //swi2(o,x,y) += (B(f/R).xy -0.5f)/400.0f;
    //o.w += g(B(f/R));

    o.w += (Ee.x*Ee.w-Ew.x*Ew.w+Es.y*Es.w-En.y*En.w) * 0.25f;

    swi2S(o,x,y, swi2(o,x,y) + o.w*cs(o.w*50.0f*1.0f+g(B(f/R))*5000.0f )*0.505f);
    //swi2(o,x,y) += o.w*cs( (o.w*10.0f/(1.0001f+g(B(f/R)))) * 100.0f)*0.10501f;
    //swi2(o,x,y) += -NA(f/R).xy*1.0f/8.0f;
    //o.w += 0.001001f * (g(B(f/R))*2.0f-1.0f);

    if (iMouse.z > 0.5f && length(f-swi2(iMouse,x,y)) < 100.0f) o.w = 0.5f;

    if (f.x < 9.0f || f.y < 9.0f || -f.x+R.x < 9.0f || -f.y+R.y < 9.0f) o *= 0.0f;

    o = clamp(o, -10.0f, 10.0f);


  SetFragmentShaderComputedColor(o);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


/*
** Following this tutorial : http://wyattflanders.com/MeAndMyNeighborhood.pdf
*/

__KERNEL__ void FluidThingy0Jipi409Fuse(float4 o, float2 f, float2 iResolution,)
{
    f+=0.5f;
float IIIIIIIIIIIIIIIIIIIIIII;
    o = A(f/R);
    o = sin_f4(o.z*1.0f+to_float4(0.0f,1.04f,2.08f,0.0f)+3.14f*swi4(o,w,w,w,w)*2.0f);
    /*
    swi3(o,x,y,z) -= _mix(
    to_float3(0.9f,0.8f,0.59f)
        ,
        to_float3(0.7f, 0.3f, 0.4f)
        ,
        o.w
    );
    */
    
  //o = (1.0f*A(f/R).wwww);
  //o = (1.0f*A(f/R).xyxy);


  SetFragmentShaderComputedColor(o);
}