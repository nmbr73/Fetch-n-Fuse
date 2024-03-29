
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image 'Texture: RGBA Noise Medium' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


#define EPS 0.00002f

#define MAX_DIST 10.0f

// SHIP SDF /////////////////////////////////////////
__DEVICE__ float sdSphere( float3 p, float s )
{
    return length(p)-s;
}

__DEVICE__ float sdEllipsoid( in float3 p, in float3 r ) // approximated
{
    float k0 = length(p/r);
    float k1 = length(p/(r*r));
    return k0*(k0-1.0f)/k1;
}

// vertical
__DEVICE__ float sdCone( in float3 p, in float2 c, float h )
{
    float2 q = h*to_float2(c.x,-c.y)/c.y;
    float2 w = to_float2( length(swi2(p,x,z)), p.y );
    
    float2 a = w - q*clamp( dot(w,q)/dot(q,q), 0.0f, 1.0f );
    float2 b = w - q*to_float2( clamp( w.x/q.x, 0.0f, 1.0f ), 1.0f );
    float k = sign_f( q.y );
    float d = _fminf(dot( a, a ),dot(b, b));
    float s = _fmaxf( k*(w.x*q.y-w.y*q.x),k*(w.y-q.y)  );
  return _sqrtf(d)*sign_f(s);
}

__DEVICE__ float sdTorus( float3 p, float2 t )
{
  float2 q = to_float2(length(swi2(p,x,z))-t.x,p.y);
  return length(q)-t.y;
}

__DEVICE__ float sdCapsule( float3 p, float3 a, float3 b, float r )
{
  float3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
  return length( pa - ba*h ) - r;
}

__DEVICE__ float sdCappedCone(float3 p, float3 a, float3 b, float ra, float rb)
{
    float rba  = rb-ra;
    float baba = dot(b-a,b-a);
    float papa = dot(p-a,p-a);
    float paba = dot(p-a,b-a)/baba;
    float x = _sqrtf( papa - paba*paba*baba );
    float cax = _fmaxf(0.0f,x-((paba<0.5f)?ra:rb));
    float cay = _fabs(paba-0.5f)-0.5f;
    float k = rba*rba + baba;
    float f = clamp( (rba*(x-ra)+paba*baba)/k, 0.0f, 1.0f );
    float cbx = x-ra - f*rba;
    float cby = paba - f;
    float s = (cbx < 0.0f && cay < 0.0f) ? -1.0f : 1.0f;
    return s*_sqrtf( _fminf(cax*cax + cay*cay*baba,
                            cbx*cbx + cby*cby*baba) );
}


__DEVICE__ float sdRoundBox( float3 p, float3 b, float r )
{
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f) - r;
}


__DEVICE__ float opSubtract( float d1, float d2 ) {
   return _fmaxf(-d1,d2); 
}

__DEVICE__ float3 opElongate(in float3 p, in float3 h )
{
    return p - clamp( p, -h, h );
}

__DEVICE__ float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5f + 0.5f*(d2-d1)/k, 0.0f, 1.0f );
    return _mix( d2, d1, h ) - k*h*(1.0f-h); 

}

__DEVICE__ float shipWindows(in float3 p) {
    return sdRoundBox(to_float3(_fabs(p.x) - 0.56f, p.y - 0.76f, p.z-0.57f), to_float3(0.5f, 0.5f, 0.5f), 0.05f);
}

// returns 0 for body, 1 for window
__DEVICE__ float windowDeform(in float3 p){
  return 1.0f - smoothstep(0.0f, 0.01f, shipWindows(p));
}


__DEVICE__ float shipBody( in float3 p) {
  return opSmoothUnion(
                        sdEllipsoid(p, to_float3(0.3f, 0.4f, 0.3f)),
                        sdCapsule(to_float3(_fabs(p.x), p.y, _fabs(p.z)), to_float3(0.2f, 0.0f, 0.2f), to_float3(0.28f, -0.1f, 0.28f), 0.015f),
                        0.02
                      ) + 0.01f * windowDeform(p);
}

__DEVICE__ float shipEngine( in float3 p){
  return opSubtract(
                   sdCone(p + to_float3(0.0f, 0.34f, 0.0f), to_float2(0.6f, 1.0f), 0.25f),
                   sdCone(p + to_float3(0.0f, 0.42f, 0.0f), to_float2(0.40f, 1.0f), 0.05f) - 0.07
                   );
}


// ring and landing gear
__DEVICE__ float shipRing( in float3 p){
  return _fminf(
               sdTorus(opElongate(p + to_float3(0, 0.05f, 0), to_float3(0, 0.05f, 0)), to_float2(0.4f, 0.013f)),
               sdCappedCone(to_float3(_fabs(p.x), p.y, _fabs(p.z)), to_float3(0.1f, -0.3f, 0.1f), to_float3(0.17f, -0.5f, 0.17f), 0.02f, 0.01f)
               );
}


__DEVICE__ float ship(in float3 p){
  return _fminf(_fminf(shipBody(p), shipEngine(p)),shipRing(p));
}

// Ship map
__DEVICE__ float map(in float3 p){
    float d2 = dot(p,p);
    if(d2 > 1.0f){
        return _sqrtf(d2)-0.5f;
    }
    return ship(swi3(p,x,z,y)); // Rotat eship so it's horizontal
}

__DEVICE__ float windowMat(in float3 p) {
    return smoothstep(-0.001f, 0.0f,-shipWindows(swi3(p,x,z,y)));
}


__DEVICE__ float engineMat(in float3 p) {
    return smoothstep(-EPS - 0.01f, -EPS, -shipEngine(swi3(p,x,z,y)));
}


// Engine plume
// returns 3D value noise - from https://iquilezles.org/articles/gradientnoise/
__DEVICE__ float noise( in float3 _x, __TEXTURE2D__ iChannel0 )
{
  float3 i = _floor(_x);
  float3 f = fract_f3(_x);
  f = f*f*(3.0f-2.0f*f);
  float2 uv = (swi2(i,x,y)+to_float2(37.0f,17.0f)*i.z) + swi2(f,x,y);
  float2 rg = swi2(texture( iChannel0, (uv+0.5f)/256.0f),y,x);
  return _mix( rg.x, rg.y, f.z ) - 0.5f;
    
}
__DEVICE__ float fbm( in float3 _x, in float H, in int octaves, __TEXTURE2D__ iChannel0)
{    
    float G = _exp2f(-H);
    float f = 1.0f;
    float a = 1.0f;
    float t = 0.0f;
    for( int i=0; i<octaves; i++ )
    {
        t += a*noise(f*_x, iChannel0);
        f *= 2.0f;
        a *= G;
    }
    return t;
}

__DEVICE__ float exhaustNoise(in float3 p, in int octaves, float iTime, __TEXTURE2D__ iChannel0){
    float3 p2 = swi3(p,x,z,y);
    float3 noise_pos = p2 * to_float3(10.0f / p2.y, 2.0f, 10.0f / p2.y) + to_float3(0.0f, 50.0f * iTime, 0.0f); // Moving origin to make exhaust move
   
    float strength = smoothstep(0.25f, 2.0f, -p2.y) * -p2.y * 0.1f;
    float distort = fbm(noise_pos, 1.2f, octaves, iChannel0) * strength;
    return distort;
}

__DEVICE__ float exhaust(in float3 p, in int octaves, float iTime, __TEXTURE2D__ iChannel0) {
    float3 p2 = swi3(p,x,z,y) + to_float3(0.0f, -0.25f, 0.0f); // origin centered on start of exhaust
    float distort = exhaustNoise(p, octaves, iTime, iChannel0);

    return sdCone(p2, to_float2(0.1f, 1.0f), 100.0f) + distort;
}

__DEVICE__ float3 exhaustNormal( in float3 pos, float iTime, __TEXTURE2D__ iChannel0 ) // for function f(p)
{
    const float h = 0.001f;
    #define ZERO 0 //(_fminf(iFrame,0)) // non-constant zero
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*exhaust(pos+e*h, 2, iTime, iChannel0);
    }
    return normalize(n);
}


__DEVICE__ float softshadow( in float3 ro, in float3 rd, float mint, float k )
{
    float res = 1.0f;
    float t = mint;
    float h = 1.0f;
    for( int i=0; i<32; i++ )
    {
        h = map(ro + rd*t);
        res = _fminf( res, k*h/t );
        t += clamp( h, 0.005f, 0.1f );
    }
    return clamp(res,0.0f,1.0f);
}


// From https://iquilezles.org/articles/normalsSDF/
__DEVICE__ float3 calcNormal( in float3 pos ) // for function f(p)
{
    const float h = 0.0001f;      // replace by an appropriate value
    #define ZERO 0 //(_fminf(iFrame,0)) // non-constant zero
    float3 n = to_float3_s(0.0f);
    for( int i=ZERO; i<4; i++ )
    {
        float3 e = 0.5773f*(2.0f*to_float3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0f);
        n += e*map(pos+e*h);
    }
    return normalize(n);
}


__DEVICE__ float calcAO( in float3 pos, in float3 nor )
{
    float occ = 0.0f;
    float sca = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float h = 0.01f + 0.12f*(float)(i)/4.0f;
        float d = map( pos + h*nor );
        occ += (h-d)*sca;
        sca *= 0.95f;
        if( occ>0.35f ) break;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f ) * (0.5f+0.5f*nor.y);
}


__DEVICE__ bool intersect( in float3 ro, in float3 rd, out float *dist )
{
  float h = 1.0f;
  *dist = 0.0f;
    for( int i=0; i<128; i++ )
    {
    if( h < EPS * *dist) return true;
    h = map(ro + rd * *dist);
        *dist += h;
    if( *dist > MAX_DIST) return false;
    }
  return false;
}

// Often fails because of inexact SDF, but this is okay - it leads to a cool pattern in the exhaust.
__DEVICE__ bool intersect_exhaust( in float3 ro, in float3 rd, out float *dist, float iTime, __TEXTURE2D__ iChannel0 )
{
  float h = 1.0f;
  *dist = 0.0f;
    for( int i=0; i<64; i++ )
    {
    if( h < 0.0001f * *dist) return true;
        float3 p = ro + rd * *dist;
    h = exhaust(ro + rd * *dist, 6, iTime, iChannel0);
        *dist += h * 0.9f;
    if( *dist > 20.0f) return false;
    }
  return h < 0.01f;
}

// Faster, lower quality exhaust
__DEVICE__ bool intersect_exhaust_lq( in float3 ro, in float3 rd, out float *dist, float iTime, __TEXTURE2D__ iChannel0)
{
  float h = 1.0f;
  *dist = 0.0f;
    for( int i=0; i<16; i++ )
    {
    if( h < 0.02f * *dist) return true;
        float3 p = ro + rd * *dist;
    h = exhaust(ro + rd * *dist, 1, iTime, iChannel0);
        *dist += h * 0.9f + 0.02f;
    if( *dist > 50.0f) return false;
    }
  return false;
}

// From https://www.shadertoy.com/view/4slSWf
__DEVICE__ void generateRay( out float3 *resRo, out float3 *resRd, in float3 po, in float3 ta, in float2 pi, float2 iResolution )
{
    float2 p = (2.0f*pi-iResolution)/iResolution.y;
        
    // camera matrix
    float3 ww = normalize( ta - po );
    float3 uu = normalize( cross(ww,to_float3(0.0f,1.0f,0.0f) ) );
    float3 vv = normalize( cross(uu,ww));

    // create view ray
    float3 rd = normalize( p.x*uu + p.y*vv + 2.2f*ww );

    *resRo = po;
    *resRd = rd;
}


__DEVICE__ float3 fancyCube( __TEXTURE2D__ sam, in float3 d, in float s, in float b )
{
    float3 colx = swi3(texture( sam, 0.5f + s*swi2(d,y,z)/d.x),x,y,z);
    float3 coly = swi3(texture( sam, 0.5f + s*swi2(d,z,x)/d.y),x,y,z);
    float3 colz = swi3(texture( sam, 0.5f + s*swi2(d,x,y)/d.z),x,y,z);
    
    float3 n = d*d;
    
    return (colx*n.x + coly*n.y + colz*n.z)/(n.x+n.y+n.z);
}


__DEVICE__ float2 hash( float2 p ) { p=to_float2(dot(p,to_float2(127.1f,311.7f)),dot(p,to_float2(269.5f,183.3f))); return fract_f2(sin_f2(p)*43758.5453f); }

__DEVICE__ float2 voronoi( in float2 _x )
{
    float2 n = _floor( _x );
    float2 f = fract_f2( _x );

    float3 m = to_float3_s( 8.0f );
    for( int j=-1; j<=1; j++ )
    for( int i=-1; i<=1; i++ )
    {
        float2  g = to_float2( (float)(i), (float)(j) );
        float2  o = hash( n + g );
        float2  r = g - f + o;
        float d = dot( r, r );
        if( d<m.x )
            m = to_float3( d, o.x, o.y );
    }

    return to_float2( _sqrtf(m.x), m.y+m.z );
}


// Background based off of https://www.shadertoy.com/view/llj3Rz
__DEVICE__ float3 background( in float3 d, in float3 l , in float starsize, __TEXTURE2D__ iChannel1)
{
    float3 col = to_float3_s(0.0f);
    col += 0.6f*pow_f3( swi3(fancyCube( iChannel1, d, 0.05f, 5.0f ),z,y,x), to_float3_s(2.0f) );
    col += 1.5f*to_float3(0.80f,0.5f,0.6f)*pow_f3( swi3(fancyCube( iChannel1, d, 0.1f, 0.0f ),x,x,x), to_float3_s(6.0f) );
    float stars = smoothstep( 0.3f, 0.7f, fancyCube( iChannel1, d, 0.91f, 0.0f ).x );

    col = smoothstep(to_float3_s(0.15f), to_float3_s(0.5f), col);
    
    float3 n = abs_f3(d);
    n = n*n*n;
    
    float2 vxy = voronoi( 30.0f*swi2(d,x,y)*starsize );
    float2 vyz = voronoi( 30.0f*swi2(d,y,z)*starsize );
    float2 vzx = voronoi( 30.0f*swi2(d,z,x)*starsize );
    float2 r = (vyz*n.x + vzx*n.y + vxy*n.z) / (n.x+n.y+n.z);
    col += 0.9f * stars * clamp(1.0f-(3.0f+r.y*5.0f)*r.x,0.0f,1.0f);



    float s = clamp( dot(d,l), 0.0f, 1.0f );
    col += 0.4f*_powf(s,10.0f*starsize)*to_float3(1.0f,0.7f,0.6f)*2.0f;
    col += 0.4f*_powf(s,100.0f*starsize)*to_float3(1.0f,0.9f,0.8f)*2.0f;
    
    return col;
}

__DEVICE__ float3 BlackBody( float t )
{
    const float h = 6.6e-34; // Planck constant
    const float k = 1.4e-23; // Boltzmann constant
    const float c = 3e8;// Speed of light

    float3 w = to_float3( 610.0f, 549.0f, 468.0f ) / 1e9f; // approximate wavelength of primaries
    
    // Planck's law https://en.wikipedia.org/wiki/Planck%27s_law
    
    float3 w5 = w*w*w*w*w;    
    float3 o = 2.0f*h*(c*c) / (w5 * (exp_f3(h*c/(w*k*t)) - 1.0f));

    return o;
}

__DEVICE__ float3 shadeExhaust(float3 p, float3 rd, float iTime, __TEXTURE2D__ iChannel0) {
    float3 norm = exhaustNormal(p, iTime, iChannel0);
    float ndotr = dot(rd, norm);
    float rim = _powf(1.0f-_fabs(ndotr),2.0f);
    float temp_gradient = _mix(1200.0f, 0.0f, -p.z * 0.03f);
    float temperature = temp_gradient + smoothstep(-0.5f, 0.5f, -exhaustNoise(p, 8, iTime, iChannel0)) * 200.0f;
    float thickness = (rim + 0.03f) * 0.00001f;
    
    return (BlackBody(temperature) * thickness);
}


__DEVICE__ float3 shadeShip(float3 p, float3 rd, float3 lightDir, float3 shipColor, float iTime, float3 Tex, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    float window = windowMat(p);
    float engine = engineMat(p);
    float3 norm = calcNormal(p);
    float spec_strength = _mix(0.6f, 0.9f, window);
    spec_strength = _mix(spec_strength, 0.01f, engine);

    float ndotl = dot(norm,lightDir);
    float ao = _mix(0.6f, 1.0f, calcAO(p, norm));
    float shadowAttenuation = softshadow(p + norm * EPS, lightDir, 0.01f, 3.0f);
    float occ = ao * shadowAttenuation;

    
    // Diffuse
    float3 albedo = _mix(shipColor, to_float3(0.05f, 0.05f, 0.05f), window); //to_float3(0.9f, 0.1f, 0.1f), to_float3(0.05f, 0.05f, 0.05f)
    albedo = _mix(albedo, to_float3_s(0.05f), engine);
    
    float3 light = smoothstep(-0.1f, 1.0f, ndotl) * to_float3_s(1.0f);

    float3 col = albedo * light * occ;
  
    float3 exhaust_dir = to_float3(0.0f, 0.0f, -1.0f);
    float ndotexhaust = dot(norm, exhaust_dir);
    light = smoothstep(-0.1f, 1.0f, ndotexhaust) * to_float3(1.0f, 0.8f, 0.2f) * 15.0f; //to_float3(1.0f, 0.8f, 0.2f)
    float exhaust_occ = softshadow(p + norm * EPS, exhaust_dir, 0.001f, 0.5f) + 0.2f;
    
    col += albedo * light * exhaust_occ;
      
    // specular
    float3 h = normalize(lightDir-rd);
    float s = _powf(_fmaxf(0.0f,dot(norm,h)),50.0f) * spec_strength;
    
    float3 specular = s*to_float3_s(1.0f);


    // Reflections
    // I think these look too sharp on the body of the ship. 
    // Skybox reflection
    float3 rr = reflect(rd, norm);
    float rr_atten = softshadow(p + norm * EPS, rr, 0.01f, 100.0f);
    specular += _mix(albedo * occ * spec_strength, background(rr, lightDir, spec_strength, iChannel1), rr_atten);
    
    // Exhaust reflection
    float exhaust_d;
    bool rr_exhaust_hit = intersect_exhaust_lq(p, rr, &exhaust_d, iTime, iChannel0);
    if(rr_exhaust_hit){
        specular += rr_atten * spec_strength * shadeExhaust (p + rr * exhaust_d, rr, iTime, iChannel0);
    }
  
  float ndotr = dot(norm,rd);
  float fresnel = _powf(1.0f-_fabs(ndotr),5.0f) * spec_strength;
  fresnel = _mix(spec_strength, 1.0f, fresnel );

  col = _mix( col, specular, fresnel );
  
  return col;
}




__DEVICE__ float3 encodeSRGB(float3 linearRGB)
{
    float3 a = 12.92f * linearRGB;
    float3 b = 1.055f * pow_f3(linearRGB, to_float3_s(1.0f / 2.4f)) - 0.055f;
    float3 c = step(to_float3_s(0.0031308f), linearRGB);
    return mix_f3(a, b, c);
}


__KERNEL__ void SpaceShipFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);
    CONNECT_COLOR1(ShipColor, 0.9f, 0.1f, 0.1f, 1.0f);
    CONNECT_POINT0(Tex, 1.0f, 1.0f);
    CONNECT_SLIDER1(TexZ, -10.0f, 10.0f, 1.0f);
    
    CONNECT_SLIDER0(ViewZ, -10.0f, 10.0f, 0.0f);
    
    Color += 0.5f;

    // camera
    float an = 0.3f*iTime;
    float3  po = to_float3(1.0f + 3.5f*_sinf(an),1.0f,3.5f*_cosf(an));
    float3  ta = to_float3(0.0f,0.0f,0.0f);
    
    ta.x = iMouse.x/R.x;
    ta.y = iMouse.y/R.y;
    ta.z = ViewZ;
    

    float3 ro,  rd;  generateRay( &ro,  &rd,  po, ta, fragCoord, iResolution );
    
    float4 ship_sphere = to_float4_aw(to_float3_s(0.0f), 0.6f);
    
    float3 col = to_float3_s(0.0f);

    float3 star_dir = normalize(to_float3(1.0f, 1.0f, 1.0f));//  star_dir = normalize(to_float3(_sinf(0.5f*iTime),2.0f * _sinf(0.34253f*iTime),_cosf(0.5f*iTime)));

    
    float dist; bool hit;
    float exhaust_dist;
    hit = intersect(ro, rd, &dist);
    if(hit){
        float3 pos = ro + dist * rd;
        col = shadeShip(pos, rd, star_dir,swi3(ShipColor,x,y,z), iTime, to_float3_aw(Tex,TexZ), iChannel0, iChannel1);
        hit = intersect_exhaust(ro, rd, &exhaust_dist, iTime, iChannel0);
        if(hit && exhaust_dist < dist){
            col += shadeExhaust(ro + rd * exhaust_dist, rd, iTime, iChannel0);
        }
    } else {
        col = background(rd*to_float3_aw(Tex,TexZ), star_dir, 1.5f, iChannel1) * 0.5f;
        hit = intersect_exhaust(ro, rd, &exhaust_dist, iTime, iChannel0);
        if(hit){
            col += shadeExhaust(ro + rd * exhaust_dist, rd, iTime, iChannel0);
        }
    }

    col = col * swi3(Color,x,y,z);//to_float3(1.0f, 1.0f, 1.0f);
    col = encodeSRGB(col);
    // Output to screen
    fragColor = to_float4_aw(col, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}