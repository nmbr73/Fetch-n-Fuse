
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 3' to iChannel0

#define PI 3.14159265359f

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)



__DEVICE__ mat3 rotateY( float a )
{
    return to_mat3(_cosf(a), 0.0f, _sinf(a),
                       0.0f, 1.0f, 0.0f,
                  -_sinf(a), 0.0f, _cosf(a));
}
__DEVICE__ mat3 rotateX( float a )
{
    return to_mat3(1.0f, 0.0f, 0.0f,
                   0.0f, _cosf(a), _sinf(a),
                   0.0f, -_sinf(a),_cosf(a));
}

// from iq
__DEVICE__ float sdCappedCylinder( float3 p, float h, float r )
{
   float2 d = abs_f2(to_float2(length(swi2(p,x,y)),p.z)) - to_float2(r,h);
   return _fminf(max(d.x,d.y),0.0f) + length(_fmaxf(d,to_float2_s(0.0f)));
}


__DEVICE__ float sdCylinder( float3 p, float r )
{
    return length(swi2(p,x,y)) - r;
}

__DEVICE__ float2 cos_sin( float angle )
{
    return to_float2( _cosf(angle), _sinf(angle) );
}

__DEVICE__ float2 rotate( float2 pos, float angle )
{
    return to_float2( pos.x * _cosf(angle) + pos.y * _sinf(angle)
                    , pos.x *-_sinf(angle) + pos.y * _cosf(angle) );
}

__DEVICE__ float3 modAngle( float3 pos, float fractionOfCircle )
{
    float angle = _atan2f( pos.y, pos.x );
    float newAngle = mod_f( angle + PI*fractionOfCircle, 2.0f*PI*fractionOfCircle ) - PI*fractionOfCircle;
    return to_float3_aw( length(swi2(pos,x,y))*cos_sin(newAngle), pos.z );
}

__DEVICE__ float sdGear( float3 pos_, float thickness, float outerRadius, float innerRadius, float toothSize, float NUM_INNER_TEETH, float INFLATION )
{
    //float angle = _atan2f( pos_.y, pos_.x );
    float3 pos = modAngle( pos_, 1.0f/NUM_INNER_TEETH);

    // bulk
    float ret = sdCappedCylinder( pos, thickness, outerRadius );
    
    // tooth
    ret = _fminf( ret, sdCappedCylinder( pos-to_float3(outerRadius+INFLATION,0.0f,0.0f), thickness, toothSize ) );
    
    // carve out center
    ret = _fmaxf( ret, -sdCylinder( pos, innerRadius ) );
    
    // this makes the gears bumpy!
    //ret += _sinf(20.0f*pos_.x)*_sinf(20.0f*pos_.y)*_sinf(20.0f*pos_.z)*0.02f;    
    return ret;
}

__DEVICE__ float bump( float val )
{
    float TOLERANCE = 0.15f;
    if ( _fabs(val) >= TOLERANCE )
        return 0.0f;
    float t = _fabs(val)/TOLERANCE;
    return _expf(-1.0f/(1.0f-t*t));
}

__DEVICE__ float gear( float3 pos, float gearMotionAngle, out float3 *texturePos, float NUM_INNER_TEETH, float INFLATION, float BIG_GEAR_RADIUS, float SMALL_GEAR_RADIUS, float GEAR_DEPTH, float GEAR_THICKNESS, float TOOTH_SIZE )
{
    float gearMotionRadius = BIG_GEAR_RADIUS - SMALL_GEAR_RADIUS;
    float gearAngle = gearMotionAngle * ( BIG_GEAR_RADIUS / SMALL_GEAR_RADIUS - 1.0f );
    float2 gearPos = gearMotionRadius * cos_sin( gearMotionAngle );
    
    float posAngle = _atan2f( pos.y-gearPos.y, pos.x-gearPos.x );
    
    // warp the gears so that they weave together
    float WARP_AMOUNT = 0.18f + INFLATION*2.0f;
    float warpAngle = fract( (posAngle-gearMotionAngle)/(2.0f*PI) );
    float warpZ = 0.0f;
    warpZ += bump( warpAngle-0.28f ) * WARP_AMOUNT * 0.8f;
    warpZ -= bump( warpAngle-0.74f ) * WARP_AMOUNT * 0.8f;
    warpZ += bump( warpAngle-0.56f ) * WARP_AMOUNT;
    warpZ -= bump( warpAngle-0.44f ) * WARP_AMOUNT;    

    float3 rotatedPos = to_float3_aw( rotate( swi2(pos,x,y) - gearPos, -gearAngle ), pos.z+warpZ );
    *texturePos = rotatedPos;
    return sdGear( rotatedPos, GEAR_DEPTH, SMALL_GEAR_RADIUS-INFLATION, SMALL_GEAR_RADIUS-GEAR_THICKNESS, TOOTH_SIZE-INFLATION, NUM_INNER_TEETH, INFLATION );
}

__DEVICE__ float sdOuterGear( float3 pos_, float thickness, float innerRadius, float outerRadius, float toothSize, float NUM_OUTER_TEETH, float INFLATION )
{
    float3 pos = modAngle( pos_, 1.0f/NUM_OUTER_TEETH);

    // bulk
    float ret = sdCappedCylinder( pos, thickness, outerRadius );
    
    // tooth
    ret = _fmaxf( ret, -sdCylinder( pos-to_float3(innerRadius-INFLATION,0.0f,0.0f), toothSize+INFLATION ) );
    
    // carve out center
    ret = _fmaxf( ret, -sdCylinder( pos, innerRadius ) );
    
    //ret += _sinf(20.0f*pos_.x)*_sinf(20.0f*pos_.y)*_sinf(20.0f*pos_.z)*0.05f;
    
    return ret;
}

__DEVICE__ float sdScene( float3 pos, out int *objectId, out float3 *texturePos, float iTime, 
                          float NUM_INNER_TEETH, float INFLATION, float BIG_GEAR_RADIUS, float SMALL_GEAR_RADIUS, float NUM_OUTER_TEETH, float TOUCH_DIST, float GEAR_DEPTH, float GEAR_THICKNESS, float TOOTH_SIZE )
{    
    float dist = 999.0f;
    float d;
    float3 tp;
    d = sdOuterGear( pos, GEAR_DEPTH, BIG_GEAR_RADIUS+INFLATION, BIG_GEAR_RADIUS+GEAR_THICKNESS, TOOTH_SIZE, NUM_OUTER_TEETH, INFLATION ) - INFLATION;
    dist = _fminf( dist, d );
    if ( d <= TOUCH_DIST ) { *objectId = 0; *texturePos = pos; }
            
    float gearMotionAngle = iTime * 0.7f;
    d = gear( pos, gearMotionAngle + PI*0.0f/3.0f, &tp, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) - INFLATION;
    dist = _fminf( dist, d );
    if ( d <= TOUCH_DIST ) { *objectId = 1; *texturePos = tp; }
        
    d = gear( pos, gearMotionAngle + PI*2.0f/3.0f, &tp, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) - INFLATION;
    dist = _fminf( dist, d );
    if ( d <= TOUCH_DIST ) { *objectId = 2; *texturePos = tp; }
        
    d = gear( pos, gearMotionAngle + PI*4.0f/3.0f, &tp, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) - INFLATION;
    dist = _fminf( dist, d );
    if ( d <= TOUCH_DIST ) { *objectId = 3; *texturePos = tp; }
    
    // wall
    d = 0.15f - pos.z;
    dist = _fminf( dist, d );
    if ( d <= TOUCH_DIST ) { *objectId = 4; *texturePos = pos; }
    
            
    return dist;
}

__DEVICE__ float3 calcNormal( in float3 p, float iTime, float NUM_INNER_TEETH, float INFLATION, float BIG_GEAR_RADIUS, float SMALL_GEAR_RADIUS, float NUM_OUTER_TEETH, float TOUCH_DIST, float GEAR_DEPTH, float GEAR_THICKNESS, float TOOTH_SIZE  ) // for function f(p)
{
    const float h = 0.001f; // replace by an appropriate value
    const float2 k = to_float2(1,-1);
    int objectId = -1;
    float3 texturePos;
    return normalize( swi3(k,x,y,y)*sdScene( p + swi3(k,x,y,y)*h, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) + 
                      swi3(k,y,y,x)*sdScene( p + swi3(k,y,y,x)*h, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) + 
                      swi3(k,y,x,y)*sdScene( p + swi3(k,y,x,y)*h, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) + 
                      swi3(k,x,x,x)*sdScene( p + swi3(k,x,x,x)*h, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE ) );
}

// iq
__DEVICE__ float shadow( in float3 ro, in float3 rd, float mint, float maxt, float iTime, 
                            float NUM_INNER_TEETH, float INFLATION, float BIG_GEAR_RADIUS, float SMALL_GEAR_RADIUS, float NUM_OUTER_TEETH, float TOUCH_DIST, float GEAR_DEPTH, float GEAR_THICKNESS, float TOOTH_SIZE  )
{
  float zzzzzzzzzzzzzzzzzzzzzzz;
    int objectId = -1;
    float3 texturePos;
    for( float t=mint; t<maxt; )
    {
        float h = sdScene( ro + rd*t, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE );
        if( h<0.001f )
            return 0.0f;
        t += h;
    }
    return 1.0f;
}

// iq
__DEVICE__ float softshadow( in float3 ro, in float3 rd, float mint, float maxt, float k, float iTime, 
                                float NUM_INNER_TEETH, float INFLATION, float BIG_GEAR_RADIUS, float SMALL_GEAR_RADIUS, float NUM_OUTER_TEETH, float TOUCH_DIST, float GEAR_DEPTH, float GEAR_THICKNESS, float TOOTH_SIZE )
{
    float res = 1.0f;
    int objectId = -1;
    float3 texturePos;
    for( float t=mint; t<maxt; )
    {
        float h = sdScene( ro + rd*t, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE );
        if( h<0.001f )
            return 0.0f;
        res = _fminf( res, k*h/t );
        t += h;
    }
    return res;
}

__KERNEL__ void BorromeanGearsJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 0.1f, 0.2f, 0.5f, 1.0f);
  
    CONNECT_POINT0(ViewXY, 0.0f, 0.0f);
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
  
  
    CONNECT_SLIDER1(INFLATION, -1.0f, 1.0f, 0.02f);
    CONNECT_SLIDER2(BIG_GEAR_RADIUS, -10.0f, 10.0f, 1.2f);
    CONNECT_SLIDER3(SMALL_GEAR_RADIUS, -10.0f, 10.0f, 0.65f);
    SMALL_GEAR_RADIUS *= BIG_GEAR_RADIUS; 
    CONNECT_SLIDER4(NUM_OUTER_TEETH, -10.0f, 100.0f, 40.0f);
    CONNECT_SLIDER5(NUM_INNER_TEETH, -10.0f, 10.0f, 0.65f);
    NUM_INNER_TEETH *= NUM_OUTER_TEETH;
  
    CONNECT_SLIDER6(GEAR_DEPTH, -1.0f, 1.0f, 0.03f);
    CONNECT_SLIDER7(GEAR_THICKNESS, -1.0f, 1.0f, 0.14f);
    CONNECT_SLIDER8(TOOTH_SIZE, -1.0f, 1.0f, 0.06f);
    CONNECT_SLIDER9(TOUCH_DIST, -1.0f, 1.0f, 0.0001f);
  
    CONNECT_SCREW0(Mul, -1.0f, 10.0f, 1.0f);
    CONNECT_SCREW1(Off, -10.0f, 10.0f, 0.0f);
  
    //const float INFLATION = 0.02f;
    //const float BIG_GEAR_RADIUS = 1.2f;
    //const float SMALL_GEAR_RADIUS = BIG_GEAR_RADIUS*0.65f;
    //const float NUM_OUTER_TEETH = 40.0f;
    //const float NUM_INNER_TEETH = NUM_OUTER_TEETH*0.65f;
    
    //const float GEAR_DEPTH = 0.03f;
    //const float GEAR_THICKNESS = 0.14f;
    //const float TOOTH_SIZE = 0.06f;

    //const float TOUCH_DIST = 0.0001f;

    float2 uv = (2.0f*fragCoord - iResolution)/iResolution.y;
    
    // camera
    float3 pos = to_float3( 0.0f, 0.0f, -5.0f );
    pos += to_float3_aw(ViewXY,ViewZ);
    
    float3 dir = normalize( to_float3_aw( uv, 5.0f ) );
    
    float tilt = _cosf( (iTime+10.0f)*0.2f ) * -0.6f - 0.85f;
    mat3 modelMatrix = rotateX(tilt);    
    if ( iMouse.z > 0.0f )
    {    
        modelMatrix = rotateX((iMouse.y/iResolution.y-0.5f)*-2.9f);
        modelMatrix = mul_mat3_mat3(modelMatrix , rotateY((iMouse.x/iResolution.x-0.5f)*-2.9f));
    }
    pos = mul_mat3_f3(modelMatrix ,pos);
    dir = mul_mat3_f3(modelMatrix , dir);
    
    float3 LIGHT_DIR = mul_mat3_f3(modelMatrix , normalize( to_float3( 2.0f, -1.0f, 0.5f ) ));
    //vec3 LIGHT_DIR = modelMatrix * normalize( to_float3( 2.0f, -1.0f, 3.0f ) );    
    //vec3 LIGHT_DIR = modelMatrix * normalize( to_float3( 5.0f, -3.0f, 1.0f ) );
    
    float shadow = 1.0f;
    int objectId = -1; // which object did we hit
    float3 texturePos;
    
    //vec3 col = _tex2DVecN( iChannel1,dir.x,dir.y,15).rgb*0.3f;
    float3 col = to_float3_s( 0.0f );
    for ( int i = 0; ; i++ )
    {
        float dist = sdScene( pos, &objectId, &texturePos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE );
        if ( dist < TOUCH_DIST )
        {                     
            shadow = softshadow( pos, -LIGHT_DIR, 0.05f, 10.0f, 8.0f, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE );
            break;
        }
        if ( dist > 99.0f || i >= 200 )
        {
            objectId = 4; // wall            
            break;
        }
        pos += dir * dist * 0.7f;    
    }
    
    
    // calc lighting
    {
        float3 norm = calcNormal( pos, iTime, NUM_INNER_TEETH, INFLATION, BIG_GEAR_RADIUS, SMALL_GEAR_RADIUS, NUM_OUTER_TEETH, TOUCH_DIST, GEAR_DEPTH, GEAR_THICKNESS, TOOTH_SIZE );
        float ambientLight = ( _fmaxf( 0.0f, dot( norm, -LIGHT_DIR ) ) ) * 0.25f;
        float specularLight = _powf( _fmaxf( 0.0f, dot( -1.0f*reflect( LIGHT_DIR, norm ), dir ) ), 15.0f );
        ambientLight *= shadow;
        col = to_float3_s( ambientLight + specularLight ); 

float IIIIIIIIIIIIIIIIIIIII;
        if ( objectId == 4 )
        {                
            col += swi3(Color,x,y,z);//to_float3(0.1f, 0.2f, 0.5f);
        }
        else if ( objectId == 0 )
        {                
            col += (swi3(texture( iChannel0, swi2(texturePos,x,y) ),x,y,z)-0.5f) * 0.3f;
        }
        else
        {
            if ( objectId == 1 ) 
              col += (swi3(texture( iChannel1, swi2(texturePos,x,y)*Mul+Off ),x,y,z)-0.5f) * 0.3f;
            else          
            {
              if ( objectId == 2 ) 
                col += (swi3(texture( iChannel2, swi2(texturePos,x,y)*Mul+Off ),x,y,z)-0.5f) * 0.3f;
              else          
              { 
                if ( objectId == 3 ) 
                  col += (swi3(texture( iChannel3, swi2(texturePos,x,y)*Mul+Off ),x,y,z)-0.5f) * 0.3f;
                else          
                { 
                
                col += (float)(objectId-2)*0.2f + 0.1f;
                col += _fmaxf( 0.0f, (texture( iChannel0, swi2(texturePos,x,y)*0.25f ).x-0.5f) ) * 0.3f;
                }
              }
            }  
        }        
    }

    
    // gamma correction
    col = pow_f3( col, to_float3_s(1.0f/2.2f) );

    fragColor = to_float4_aw( col, 1.0f );


  SetFragmentShaderComputedColor(fragColor);
}