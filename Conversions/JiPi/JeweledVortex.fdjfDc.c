
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define SCALE 2.0f
#define PI radians(180.0f)
#define TAU (PI*2.0f)

__DEVICE__ float digitIsOn( int digit, float2 id ) { 
  const int CHARACTERS[14] = {31599,9362,31183,31207,23524,29671,29679,30994,31727,31719,1488,448,2,3640}; 

  if ( id.x < 0.0f || id.y < 0.0f || id.x > 2.0f || id.y > 4.0f ) return 0.0f; 
  return _floor( mod_f( (float)( CHARACTERS[ (int)( digit ) ] ) / _powf( 2.0f, id.x + id.y * 3.0f ), 2.0f ) ); } 

__DEVICE__ float digitSign( float v, float2 id ) { 
  return digitIsOn( 10 - (int)( ( sign_f( v ) - 1.0f ) * 0.5f ), id ); } 
  
__DEVICE__ int digitCount( float v ) { 
  return (int)( _floor( _logf( _fmaxf( v, 1.0f ) ) / _logf( 10.0f ) ) ); } 
  
__DEVICE__ float digitFirst( float2 uv, float scale, float v, int decimalPlaces ) { 
  float2 id = _floor( uv * scale );
  if ( 0.0f < digitSign( v, id ) ) return 1.0f; 
  v = _fabs( v ); 
  int digits = digitCount( v ); 
  float power = _powf( 10.0f, (float)( digits ) ); 
  float offset = _floor( 0.1f * scale ); 
  id.x -= offset; 
  float n; 
  for ( int i = 0 ; i < 33 ; i++, id.x -= offset, v -= power * n, power /= 10.0f ) { 
    n = _floor( v / power ); 
    if ( 0.0f < digitIsOn( (int)( n ), id ) ) return 1.0f; 
    if ( i == digits ) { 
      id.x -= offset; 
      if ( 0.0f < digitIsOn( (int)( 12 ), id ) ) return 1.0f; 
    } 
    if ( i >= digits + decimalPlaces ) return 0.0f; 
  } 
  return 0.0f; 
  } 
__DEVICE__ float digitFirst( float2 uv, float scale, float v ) { 
  return digitFirst( uv, scale, v, 3 ); 
  } 
__DEVICE__ float3 digitIn( float3 color, float3 fontColor, float2 uv, float scale, float v ) { 
  float f = digitFirst( uv, scale, v ); 
  return _mix( color, fontColor, f ); 
  } 
  
//__DEVICE__ float3 digitIn( float3 color, float2 uv, float scale, float v ) {
//  return digitIn( color, to_float3_s(1.0f), uv, scale, v ); 
//  } 
  
__KERNEL__ void JeweledVortexFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
    CONNECT_CHECKBOX0(ShowValue, 1);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    
    CONNECT_SLIDER0(Colpar1, -1.0f, 5.0f, 2.0f);
    CONNECT_SLIDER1(Colpar2, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER2(Spirals, -1.0f, 1.0f, 0.2f);
    CONNECT_SLIDER2(VD, -1.0f, 5.0f, 2.0f);
float IIIIIIIIIIIIIIIII;
    float2 m = ((swi2(iMouse,x,y)-0.5f*iResolution)*2.0f/iResolution.y);
    float st = radians(-31.0f); // start time
    float t = (iMouse.z > 0.0f) ? _atan2f(m.x, -m.y): st+(iTime*TAU)/3600.0f;
    float n = (_cosf(t) > 0.0f) ? _sinf(t): 1.0f/_sinf(t);
    float z = clamp(_powf(500.0f, n), 1e-17, 1e+17);
    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y*SCALE*z;
    float ls = (iTime*TAU)/5.0f; // light animation speed
    float a = _atan2f(uv.x, -uv.y); // screen arc
    float i = a/TAU; // spiral increment 0.5f per 180°
    float r = _powf(length(uv), 0.5f/n)-i; // archimedean at 0.5
    float cr = _ceil(r); // round up radius
    float wr = cr+i; // winding ratio
    float vd = (cr*TAU+a) / (n*2.0f); // visual denominator
    float vd2 = vd*VD;//2.0f;
    float3 col = to_float3_s(_sinf(wr*vd+ls)); // blend it
    col *= _powf(sin(fract(r)*PI), _floor(_fabs(n*2.0f))+5.0f); // smooth edges
    col *= _sinf(vd2*wr+PI/2.0f+ls*2.0f); // this looks nice
    //col *= 0.2f+_fabs(_cosf(vd2)); // dark spirals
    col *= Spirals+_fabs(_cosf(vd2)); // dark spirals
    float3 g = _mix(to_float3_s(0), to_float3_s(1), _powf(length(uv)/z, -1.0f/n)); // dark gradient
    col = _fminf(col, g); // blend gradient with spiral
    float3 rgb = to_float3( _cosf(vd2)+1.0f, _fabs(_sinf(t)), _cosf(PI+vd2)+1.0f );
    //col += (col*2.0f)-(rgb*0.5f); // add color
    col += (col*Colpar1)-(rgb*Colpar2); // add color
    if (iMouse.z > 0.0f) // on mouse click
    {
      uv = (fragCoord-0.5f*iResolution)/iResolution.y;
      if (ShowValue)
        col = digitIn(col, to_float3_s(1), (uv*2.0f)-m, 44.0f, n); // show value
    }
    fragColor = to_float4_aw(col+(swi3(Color,x,y,z)-0.5f), Color.w);

  SetFragmentShaderComputedColor(fragColor);
}