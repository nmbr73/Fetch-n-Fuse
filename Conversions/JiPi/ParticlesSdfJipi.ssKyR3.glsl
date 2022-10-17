

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec3 rand3( vec2 p ) { return textureLod( iChannel2, (p*8.0+0.5)/256.0, 0.0 ).xyw; }

float rand2(vec2 co){
    return fract(sin(dot(co.xy ,vec2(12.9898,78.233))) * 43758.5453);
}

uint mod_i32( int i32_bas , int i32_div ){

    float   flt_res =  mod( float(i32_bas), float(i32_div));
    uint    i32_res = uint( flt_res );
    return( i32_res );
}

vec4 randomColor(int i) {
    float r = rand2(vec2(i, mod_i32(i, 7))) * 0.7;
    float g = rand2(vec2(mod_i32(i, 19), i));
    float b = rand2(vec2(mod_i32(i, 6), mod_i32(i, 13)));
    return vec4(r, g, b, 1.0f);
}

vec3 currentColor = vec3(0.0f);

float scene(vec3 pos, mat3 m) {
    currentColor = vec3(0.0f);
    float k = 10000.0f;
    vec2 scale = 9. * iResolution.xy / max ( iResolution.x, iResolution.y ) ;
    vec3 q = m * pos;
    for (int i = 0; i < 800; ++i) {
        float radius = 0.3;
        vec4 particleData = texelFetch(iChannel0, ivec2(i, 0), 0);
        if (particleData.w == 0.0)
            continue;
        vec2 pos = decode_vec2(particleData.x);
        pos = scale * ( pos - vec2 ( 0.5 ) );
        vec4 color = randomColor(i);
        float dist = sphere(q, vec3(pos.x, -0.00, pos.y), radius); 
        k = smin(k, dist, 5.0f);
        currentColor += 2.0f / (20.0f *dist + 1.0f) * color.xyz;
    }
    //currentColor = clamp(vec3(0.0f), vec3(1.0f), currentColor);
    
    return k;
}

vec3 trace ( in vec3 from, in vec3 dir, out bool hit, mat3 m)
{
	vec3	p         = from;
	float	totalDist = 0.0;
	
	hit = false;
	
    int stepNum = 70;
	for ( int steps = 0; steps < stepNum; steps++ )
	{
        float dist = scene(p, m);

        p += dist * dir;
        
		if ( dist < 0.01 )
		{
			hit = true;
			break;
		}
		
		totalDist += dist;
		
		if ( totalDist > 10.0 )
			break;	
	}
	
	return p;
}

vec3 generateNormal ( vec3 z, float d, mat3 m)
{
    float eps = 0.01;
    float e   = max (d * 0.5, eps );
    float dx1 = scene(z + vec3(e, 0, 0), m);
    float dx2 = scene(z - vec3(e, 0, 0), m);
    float dy1 = scene(z + vec3(0, e, 0), m);
    float dy2 = scene(z - vec3(0, e, 0), m);
    float dz1 = scene(z + vec3(0, 0, e), m);
    float dz2 = scene(z - vec3(0, 0, e), m);
    
    return normalize ( vec3 ( dx1 - dx2, dy1 - dy2, dz1 - dz2 ) );
}


vec3 lightPos = vec3(0, -6., 0.);

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    float minDist = 1000.0f;
    int minDistParticle = -1;
    
    float border = 0.15;
    bool doVoronoi = uv.x > border && uv.x < 1.0f - border && uv.y > border && uv.y < 1.0f - border;
    
    for (int i = 0; i < 800; ++i) {
        float radius = 0.01;
        vec4 particleData = texelFetch(iChannel0, ivec2(i, 0), 0);
        if (particleData.w == 0.0) {
            continue;
        }
        vec2 pos = decode_vec2(particleData.x);
        float dist = length(pos - uv);
        if (dist < minDist) {
            minDist = dist;
            minDistParticle = i;
        }
        if (dist < radius)
            fragColor = randomColor(i);
    }
    if (doVoronoi) {
        fragColor = randomColor(minDistParticle);
    }
    
    /////////////////////////////////////////////////////
   
    //fragColor = vec4(1.0f - clamp( 0.01, 1.0, minDist));
    
        // Normalized pixel coordinates (from 0 to 1)
    
    mat3 m;
    
    m = mat3(
        vec3(1, 0, 0),
        vec3(0, 1, 0),
        vec3(0, 0, 1)
    );
    
    vec3 cameraPos = vec3(0.,-5., 0.);
    vec3 cameraForward = vec3(0., 1, 0.);
    vec3 cameraUp = vec3 (0., 0., 1.);
    float cameraFocus = 5.;
    
    vec2 scale = 9. * iResolution.xy / max ( iResolution.x, iResolution.y ) ;
    uv    = scale * ( fragCoord/iResolution.xy - vec2 ( 0.5 ) );

    
    vec3 from = vec3(uv.x, (cameraPos + cameraForward * cameraFocus).z, uv.y);
    vec3 dir = normalize(from - cameraPos);
    
    bool hit;
    vec3 p1 = trace(cameraPos, dir, hit, m);
    float d1 = length(cameraPos - p1);
     float c1 = length(p1);
    
    vec3 backgroundColor = vec3(0., 0., 0.);
    vec3 surfaceColor = vec3(1., 1., 1.);
    
    vec4 col;
    
    if (hit) {

       vec3 l = normalize(lightPos - p1);
       vec3 n = generateNormal(p1, 0.001, m);
       vec3 v = normalize(cameraPos - p1);
       float diffuse = max(0., dot(n, l));

       vec3  h  = normalize ( l + v );
       float hn = max ( 0.0, dot ( h, n ) );
       float specular = pow ( hn, 150.0 );

       col = 0.5*vec4 ( diffuse ) * vec4(currentColor, 1.0) + 0.5 * specular * vec4 ( 1, 1, 1, 1 );
    }
    else {
       col = vec4(0.0f);
    }
   

    // Output to screen
    fragColor = col;
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec4 particleData = texelFetch(iChannel1, ivec2(fragCoord), 0);
    vec2 pos = decode_vec2(particleData.x);
    vec2 vel = decode_vec2(particleData.y);
    if (pos.y < 0.0 || pos.y > 1.0) {
        vel.y = -vel.y;
    }
    if (pos.x < 0.0 || pos.x > 1.0) {
        vel.x = -vel.x;
    }
    
    pos += vel * iTimeDelta * 0.2;
    // vel.y -= iTimeDelta * 2.0;
    particleData.x = encode_vec2(pos);
    particleData.y = encode_vec2(vel);
    uint bufferBegin = uint(texelFetch(iChannel0, ivec2(0, 0), 0));
    uint bufferEnd = uint(texelFetch(iChannel0, ivec2(1, 0), 0));
    uint idx = uint(fragCoord.x);
    bool commonFill = idx >= bufferBegin && idx < bufferEnd;
    bool overflowFill = bufferBegin > bufferEnd && (idx >= bufferBegin || idx < bufferEnd);
    if (idx >= bufferBegin && idx <= bufferEnd) {
        particleData = vec4(0, 0, 0, 1);
        particleData.x = encode_vec2(iMouse.xy / iResolution.xy);
        particleData.y = encode_vec2(vec2(random(vec2(fragCoord.x, 5.0)), random(fragCoord)) * 2.0 - 1.0);
        //particleData = vec4(1.0f);
    }
    fragColor = particleData;
    // fragColor = vec4(sin(iTime * 2.0 + fragCoord.x) * 0.1 + 0.5, cos(iTime * 2.0 + fragCoord.x) * 0.1 + 0.5, 1.0, 1.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    uvec2 coords = uvec2(fragCoord);
    if (coords.y == 0u && coords.x < 2u) {
        if (iMouse.z > 0.0) {
            float spawnCount = 5.0;
            fragColor.x = float(int(texelFetch(iChannel0, ivec2(0, 0), 0).x + spawnCount) % 800);
        } else {
            fragColor.x = texelFetch(iChannel0, ivec2(1, 0), 0).x;
        }
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const float QUANT_COUNT = 65536.0 - 1.0;

float encode_vec2(vec2 v) {
    v = clamp(v, vec2(-3.0), vec2(3.0));
    v /= 3.0;
    v *= 0.5;
    v += 0.5;
    v *= QUANT_COUNT;
    return uintBitsToFloat(uint(v.x) << 16 | uint(v.y));
}

vec2 decode_vec2(float vi) {
    vec2 v = vec2(floatBitsToUint(vi) >> 16u, floatBitsToUint(vi) & 0xFFFFu);
    v /= QUANT_COUNT;
    v -= 0.5;
    v *= 2.0;
    v *= 3.0;
    return v;
}

float random(vec2 uv)
{
    return fract(sin(dot(uv,vec2(12.9898,78.233)))*43758.5453123);
}

const float PI = 3.14;

float smin ( float a, float b, float k )
{
	float res = exp ( -k*a ) + exp ( -k*b );
	return -log ( res ) / k;
}

vec4 smin ( vec4 a, vec4 b, float k )
{
	vec4 res = exp ( -k*a ) + exp ( -k*b );
	return -log ( res ) / k;
}


mat3 rotateX(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(1, 0, 0),
        vec3(0, c, -s),
        vec3(0, s, c)
    );
}

mat3 rotateY(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, 0, s),
        vec3(0, 1, 0),
        vec3(-s, 0, c)
    );
}


mat3 rotateZ(float theta) {
    float c = cos(theta);
    float s = sin(theta);
    return mat3(
        vec3(c, -s, 0),
        vec3(s, c, 0),
        vec3(0, 0, 1)
    );
}


float sphere(vec3 pos, vec3 spherePos, float sphereRadius) {
    return length(pos - spherePos) - sphereRadius;
}




mat2 rotate(float angle)
{
    float c = cos(angle);
    float s = sin(angle);

    return mat2(c, -s, s, c);
}
