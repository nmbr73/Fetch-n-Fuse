
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/c3a071ecf273428bc72fc72b2dd972671de8da420a2d4f917b75d20e1c24b34c.ogv' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define DX (iMouse.x/iResolution.x)
#define DY (iMouse.y/iResolution.y)
//#define BORDERRADIUS (6)
#define GAMMA        (2.2f)
#define PI           (3.14159265359f)
#define LUMWEIGHT    (to_float3(0.2126f,0.7152f,0.0722f))
#define pow3(x,y)    (pow_f3( _fmaxf(x,to_float3_s(0.0f)) , to_float3_s(y) ))

//#define BORDERRADIUSf   (float)(BORDERRADIUS)
//#define BORDERRADIUS22f (float)(BORDERRADIUS*BORDERRADIUS)

// https://www.shadertoy.com/view/MsS3Wc
// HSV to RGB conversion 
__DEVICE__ float3 hsv2rgb_smooth( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );
  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

__DEVICE__ float2 viewport(float2 p, float2 iResolution)
{   
    return p/(iResolution);
}

__DEVICE__ float3 sampleImage(float2 coord, float2 iResolution, __TEXTURE2D__ iChannel0){
   return pow3(swi3(texture(iChannel0,viewport(coord,iResolution)),x,y,z),GAMMA);
}

__DEVICE__ float kernel(int a,int b, float BORDERRADIUSf, float BORDERRADIUS22f){
    return (float)(a)*_expf(-(float)(a*a + b*b)/BORDERRADIUS22f)/BORDERRADIUSf;
}

__KERNEL__ void ContinuousgradientJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0)
{

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_INTSLIDER0(BORDERRADIUS, 1, 20, 6);
    
    CONNECT_SLIDER0(AlphaThres, -1.0f, 1.0f, 0.1f);
    
    float BORDERRADIUSf   = (float)(BORDERRADIUS);
    float BORDERRADIUS22f = (float)(BORDERRADIUS*BORDERRADIUS);

//#define BORDERRADIUS (6)
//#define BORDERRADIUSf   (float)(BORDERRADIUS)
//#define BORDERRADIUS22f (float)(BORDERRADIUS*BORDERRADIUS)

    float Alpha = 1.0f;

    //swi3S(fragColor,x,y,z, sampleImage(fragCoord, iResolution));
    float3 _fragColor = sampleImage(fragCoord, iResolution, iChannel0);
    
    float3 col;
    float3 colX = to_float3_s(0.0f);
    float3 colY = to_float3_s(0.0f);
    float coeffX,coeffY;
    
    for( int i = -BORDERRADIUS ; i <= BORDERRADIUS ; i++ ){
      for( int j = -BORDERRADIUS ; j <= BORDERRADIUS ; j++ ){
            coeffX = kernel(i,j,BORDERRADIUSf,BORDERRADIUS22f);
          coeffY = kernel(j,i,BORDERRADIUSf,BORDERRADIUS22f);
            
            col = sampleImage(fragCoord+to_float2(i,j),iResolution, iChannel0);
            colX += coeffX*col;
            colY += coeffY*col;
        }
        
    }
    
    float3 derivative = sqrt_f3( (colX*colX + colY*colY) )/(BORDERRADIUSf*BORDERRADIUSf);
    float angle = _atan2f(dot(colY,LUMWEIGHT),dot(colX,LUMWEIGHT))/(2.0f*PI) + iTime*(1.0f - DX)/2.0f;
    float3 derivativeWithAngle = hsv2rgb_smooth(to_float3(angle,1.0f,_powf(dot(derivative,LUMWEIGHT)*3.0f,3.0f)*5.0f));
float IIIIIIIIIIIIIII;    
    _fragColor = _mix(derivative, _fragColor, DX);
    _fragColor = _mix(derivativeWithAngle, _fragColor, DY);
    _fragColor = pow3(_fragColor,1.0f/GAMMA);

    if (_fragColor.x <= AlphaThres && _fragColor.y <= AlphaThres && _fragColor.z <= AlphaThres) Alpha = Color.w;

    _fragColor += (swi3(Color,x,y,z)-0.5f)*Alpha;

    fragColor = to_float4_aw(_fragColor, Alpha);

  SetFragmentShaderComputedColor(fragColor);
}