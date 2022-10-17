

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const float INFLATION = .02;
const float BIG_GEAR_RADIUS = 1.2;
const float SMALL_GEAR_RADIUS = BIG_GEAR_RADIUS*.65;
const float NUM_OUTER_TEETH = 40.;
const float NUM_INNER_TEETH = NUM_OUTER_TEETH*.65;
const float PI = 3.14159265359;
const float GEAR_DEPTH = .03;
const float GEAR_THICKNESS = .14;
const float TOOTH_SIZE = .06;

const float TOUCH_DIST = .0001;

mat3 rotateY( float a )
{
    return mat3x3(cos(a), 0.0, sin(a),
                     0.0, 1.0, 0.0,
                 -sin(a), 0.0, cos(a));
}
mat3 rotateX( float a )
{
    return mat3x3(1.0, 0.0, 0.0,
                  0.0, cos(a), sin(a),
                  0.0, -sin(a),cos(a));
}

// from iq
float sdCappedCylinder( vec3 p, float h, float r )
{
   vec2 d = abs(vec2(length(p.xy),p.z)) - vec2(r,h);
   return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}


float sdCylinder( vec3 p, float r )
{
    return length(p.xy) - r;
}

vec2 cos_sin( float angle )
{
    return vec2( cos(angle), sin(angle) );
}

vec2 rotate( vec2 pos, float angle )
{
    return vec2( pos.x * cos(angle) + pos.y * sin(angle)
               , pos.x *-sin(angle) + pos.y * cos(angle) );
}

vec3 modAngle( vec3 pos, float fractionOfCircle )
{
    float angle = atan( pos.y, pos.x );
    float newAngle = mod( angle + PI*fractionOfCircle, 2.*PI*fractionOfCircle ) - PI*fractionOfCircle;
    return vec3( length(pos.xy)*cos_sin(newAngle), pos.z );
}

float sdGear( vec3 pos_, float thickness, float outerRadius, float innerRadius, float toothSize )
{
    //float angle = atan( pos_.y, pos_.x );
    vec3 pos = modAngle( pos_, 1./NUM_INNER_TEETH);

    // bulk
    float ret = sdCappedCylinder( pos, thickness, outerRadius );
    
    // tooth
    ret = min( ret, sdCappedCylinder( pos-vec3(outerRadius+INFLATION,0.,0.), thickness, toothSize ) );
    
    // carve out center
    ret = max( ret, -sdCylinder( pos, innerRadius ) );
    
    // this makes the gears bumpy!
    //ret += sin(20.*pos_.x)*sin(20.*pos_.y)*sin(20.*pos_.z)*.02;    
    return ret;
}

float bump( float val )
{
    float TOLERANCE = .15;
    if ( abs(val) >= TOLERANCE )
        return 0.;
    float t = abs(val)/TOLERANCE;
    return exp(-1./(1.-t*t));
}

float gear( vec3 pos, float gearMotionAngle, out vec3 texturePos )
{
    float gearMotionRadius = BIG_GEAR_RADIUS - SMALL_GEAR_RADIUS;
    float gearAngle = gearMotionAngle * ( BIG_GEAR_RADIUS / SMALL_GEAR_RADIUS - 1. );
    vec2 gearPos = gearMotionRadius * cos_sin( gearMotionAngle );
    
    float posAngle = atan( pos.y-gearPos.y, pos.x-gearPos.x );
    
    // warp the gears so that they weave together
    float WARP_AMOUNT = .18 + INFLATION*2.;
    float warpAngle = fract( (posAngle-gearMotionAngle)/(2.*PI) );
    float warpZ = 0.;
    warpZ += bump( warpAngle-.28 ) * WARP_AMOUNT * .8;
    warpZ -= bump( warpAngle-.74 ) * WARP_AMOUNT * .8;
    warpZ += bump( warpAngle-.56 ) * WARP_AMOUNT;
    warpZ -= bump( warpAngle-.44 ) * WARP_AMOUNT;    

    vec3 rotatedPos = vec3( rotate( pos.xy - gearPos, -gearAngle ), pos.z+warpZ );
    texturePos = rotatedPos;
    return sdGear( rotatedPos, GEAR_DEPTH, SMALL_GEAR_RADIUS-INFLATION, SMALL_GEAR_RADIUS-GEAR_THICKNESS, TOOTH_SIZE-INFLATION );
}

float sdOuterGear( vec3 pos_, float thickness, float innerRadius, float outerRadius, float toothSize )
{
    vec3 pos = modAngle( pos_, 1./NUM_OUTER_TEETH);

    // bulk
    float ret = sdCappedCylinder( pos, thickness, outerRadius );
    
    // tooth
    ret = max( ret, -sdCylinder( pos-vec3(innerRadius-INFLATION,0.,0.), toothSize+INFLATION ) );
    
    // carve out center
    ret = max( ret, -sdCylinder( pos, innerRadius ) );
    
    //ret += sin(20.*pos_.x)*sin(20.*pos_.y)*sin(20.*pos_.z)*.05;
    
    return ret;
}

float sdScene( vec3 pos, out int objectId, out vec3 texturePos )
{    
    float dist = 999.;
    float d;
    vec3 tp;
    d = sdOuterGear( pos, GEAR_DEPTH, BIG_GEAR_RADIUS+INFLATION, BIG_GEAR_RADIUS+GEAR_THICKNESS, TOOTH_SIZE ) - INFLATION;
    dist = min( dist, d );
    if ( d <= TOUCH_DIST ) { objectId = 0; texturePos = pos; }
            
    float gearMotionAngle = iTime * .7;
    d = gear( pos, gearMotionAngle + PI*0./3., tp ) - INFLATION;
    dist = min( dist, d );
    if ( d <= TOUCH_DIST ) { objectId = 1; texturePos = tp; }
        
    d = gear( pos, gearMotionAngle + PI*2./3., tp ) - INFLATION;
    dist = min( dist, d );
    if ( d <= TOUCH_DIST ) { objectId = 2; texturePos = tp; }
        
    d = gear( pos, gearMotionAngle + PI*4./3., tp ) - INFLATION;
    dist = min( dist, d );
    if ( d <= TOUCH_DIST ) { objectId = 3; texturePos = tp; }
    
    // wall
    d = .15 - pos.z;
    dist = min( dist, d );
    if ( d <= TOUCH_DIST ) { objectId = 4; texturePos = pos; }
    
            
    return dist;
}

vec3 calcNormal( in vec3 p ) // for function f(p)
{
    const float h = 0.001; // replace by an appropriate value
    const vec2 k = vec2(1,-1);
    int objectId = -1;
    vec3 texturePos;
    return normalize( k.xyy*sdScene( p + k.xyy*h, objectId, texturePos ) + 
                      k.yyx*sdScene( p + k.yyx*h, objectId, texturePos ) + 
                      k.yxy*sdScene( p + k.yxy*h, objectId, texturePos ) + 
                      k.xxx*sdScene( p + k.xxx*h, objectId, texturePos ) );
}

// iq
float shadow( in vec3 ro, in vec3 rd, float mint, float maxt )
{
    int objectId = -1;
    vec3 texturePos;
    for( float t=mint; t<maxt; )
    {
        float h = sdScene( ro + rd*t, objectId, texturePos );
        if( h<0.001 )
            return 0.0;
        t += h;
    }
    return 1.0;
}

// iq
float softshadow( in vec3 ro, in vec3 rd, float mint, float maxt, float k )
{
    float res = 1.0;
    int objectId = -1;
    vec3 texturePos;
    for( float t=mint; t<maxt; )
    {
        float h = sdScene( ro + rd*t, objectId, texturePos );
        if( h<0.001 )
            return 0.0;
        res = min( res, k*h/t );
        t += h;
    }
    return res;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (2.*fragCoord - iResolution.xy)/iResolution.y;
    
    // camera
    vec3 pos = vec3( 0., 0., -5. );
    vec3 dir = normalize( vec3( uv, 5. ) );
    
    float tilt = cos( (iTime+10.)*.2 ) * -.6 - .85;
    mat3 modelMatrix = rotateX(tilt);    
    if ( iMouse.z > 0. )
    {    
        modelMatrix = rotateX((iMouse.y/iResolution.y-.5)*-2.9);
        modelMatrix = modelMatrix * rotateY((iMouse.x/iResolution.x-.5)*-2.9);    
    }
    pos = modelMatrix * pos;
    dir = modelMatrix * dir;
    
    vec3 LIGHT_DIR = modelMatrix * normalize( vec3( 2., -1., .5 ) );    
    //vec3 LIGHT_DIR = modelMatrix * normalize( vec3( 2., -1., 3. ) );    
    //vec3 LIGHT_DIR = modelMatrix * normalize( vec3( 5., -3., 1. ) );
    
    float shadow = 1.;
    int objectId = -1; // which object did we hit
    vec3 texturePos;
    
    //vec3 col = texture( iChannel1, dir ).rgb*.3;
    vec3 col = vec3( 0. );
    for ( int i = 0; ; i++ )
    {
        float dist = sdScene( pos, objectId, texturePos );
        if ( dist < TOUCH_DIST )
        {                     
            shadow = softshadow( pos, -LIGHT_DIR, .05, 10., 8. );
            break;
        }
        if ( dist > 99. || i >= 200 )
        {
            objectId = 4; // wall            
            break;
        }
        pos += dir * dist * .7;    
    }
    
    
    // calc lighting
    {
        vec3 norm = calcNormal( pos );
        float ambientLight = ( max( 0., dot( norm, -LIGHT_DIR ) ) ) * .25;
        float specularLight = pow( max( 0., dot( -reflect( LIGHT_DIR, norm ), dir ) ), 15. );
        ambientLight *= shadow;
        col = vec3( ambientLight + specularLight ); 


        if ( objectId == 4 )
        {                
            col += vec3(.1, .2, .5);
        }
        else if ( objectId == 0 )
        {                
            col += (texture( iChannel0, texturePos.xy ).rgb-.5) * .3;
        }
        else
        {
            col += float(objectId-2)*.2 + .1;
            col += max( 0., (texture( iChannel0, texturePos.xy*.25 ).r-.5) ) * .3;
        }        
    }
    
    
    
    
    
    // gamma correction
    col = pow( col, vec3(1.0/2.2) );

    fragColor = vec4( col, 1. );
}