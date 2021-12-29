
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------



#define A(u) _tex2DVecN(iChannel0,(u.x)/iResolution.x,(u.y)/iResolution.y,15)

	__KERNEL__ void fungusFuse(
  float4 fragColor,
  float2 u,
  float2 iResolution,
  float  iTime,
  float4 iMouse
  ) {


#ifdef XXX
	DEFINE_KERNEL_ITERATORS_XY(x, y);                                                               \
    if (x >= params->width || y >= params->height)                                                  \
      return;                                                                                       \
                                                                                                    \
    float2 iResolution = to_float2(params->width, params->height);                                  \
    float  iTime       = params->iTime * params->frequency;                                         \
    float2 u   = to_float2(x, y);                                                           \
    float4 iMouse      = to_float4(params->mouse_x,params->mouse_y,params->mouse_z,params->mouse_w); \
    float4 fragColor   = to_float4_s(0.0f);
#endif


    float4 a = A((u+to_float2(0,0)));
    float b = 0.0f;

    //kernel convolution that pointifies
    {
        float z = 2.0f;//kernel convolution size
        float t = z*2.0f+1.0f;
              t = 1.0f/(t*t-1.0f);
        float blur = 1.5f/z;
        for(float i = -z; i<=z;++i){
        for(float j = -z; j<=z;++j){
          float s0 = 0.0f;
          float s1 = 0.0f;
          for(float i2 = -z; i2<=z;++i2){
          for(float j2 = -z; j2<=z;++j2){
          float2 c = to_float2(i2,j2)*blur;
          s0 += _fmaxf(0.0f,+A((u+to_float2(i,j)+to_float2(i2,j2))).x)/_expf(dot(c,c));
          s1 += _fmaxf(0.0f,-A((u+to_float2(i,j)+to_float2(i2,j2))).x)/_expf(dot(c,c));
          }}
          if(s0==0.0f)s0 = 1.0f;
          if(s1==0.0f)s1 = 1.0f;
          float2 c = -1.0f*to_float2(i,j)*blur;
          float d = A((u+to_float2(i,j))).x;
          float e = 1.0f/_expf(dot(c,c))*d*(1.0f-_fminf(_fabs(d)+0.01f,1.0f));
          b +=(+_fmaxf(0.0f,+a.x)/s0
               +_fmaxf(0.0f,-a.x)/s1)*e;
        }}

    }
    //kernel convolution that blurs
    {
        float z = 2.0f;//kernel convolution size
        float t = z*2.0f+1.0f;
              t = 1.0f/(t*t-1.0f);
        float blur = 1.5f/z;
        for(float i = -z; i<=z;++i){
        for(float j = -z; j<=z;++j){
          float s = 0.0f;
          for(float i2 = -z; i2<=z;++i2){
          for(float j2 = -z; j2<=z;++j2){
          float2 c = to_float2(i2,j2)*blur;
          s += 1.0f/_expf(dot(c,c));
          }}
          if(s==0.0f)s = 1.0f;
          float2 c = -1.0f*to_float2(i,j)*blur;
          float d = A((u+to_float2(i,j))).x;
          b += d/s/_expf(dot(c,c))*(_fminf(_fabs(d)+0.0f,1.0f))*1.0f;
        }}
    }
    a = to_float4(b,a.x,a.y,a.z);

    if(iMouse.z>0.0f)
    {
        float2 m1 = 2.0f*(u-swi2(iMouse,x,y))/iResolution.y;
        a *= 1.0f-1.0f/_expf(dot(m1,m1));
    }
    if(iTime*30.0f<1)
    {
        a = to_float4_s(0.5f);
        float2 m1 = (2.0f*u-iResolution)/iResolution.y-to_float2(-0.02f,0);
        float2 m2 = (2.0f*u-iResolution)/iResolution.y-to_float2(+0.02f,0);
        a *= +0.1f/_expf(dot(m1,m1)*333.0f)
             -0.1f/_expf(dot(m2,m2)*333.0f);
    }
//    float keyA  = 0.0f;//_tex2DVecN( iChannel1, to_float2(65.5f/256.0f,0.25f) ).x;
//    if(keyA!=0.0f) a = to_float4_s(_tex2DVecN( iChannel2, u.x/iResolution.x,u.y/iResolution.y,15)))-0.6f;

    fragColor = a;

	SetFragmentShaderComputedColor(fragColor);
}



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#ifdef XXX //Buffer to Do


__KERNEL__ void fungusFuse_Image( __CONSTANTREF__ Params*  params, __TEXTURE2D__ iChannel0, __TEXTURE2D_WRITE__ dst )
{
  PROLOGUE(fragColor,fragCoord);


    float2 u = fragCoord/iResolution;
    float4 a = _tex2DVecN(iChannel0,u.x,u.y,15);
    fragColor = a*4.0f+0.5f;

//fragColor = to_float4(0.8f,0.4f,0.2f,1.0f);
//fragColor = a;

  EPILOGUE(fragColor);
}


