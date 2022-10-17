

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const vec3 c1 = vec3(0.8, 0.3, 1.0);
const vec3 c2 = vec3(0.2, 0.9, 0.5);
const vec3 c3 = vec3(0.95, 0.65, 0.25);

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    vec3 rps = texture(iChannel0, fragCoord/iResolution.xy).xyz;
    rps.x = 1.0 - pow(1.0 - rps.x, 3.0);
    rps.y = 1.0 - pow(1.0 - rps.y, 3.0);
    rps.z = 1.0 - pow(1.0 - rps.z, 3.0);
    fragColor.rgb = (
        rps.x * c1 +
        rps.y * c2 +
        rps.z * c3
    );
    fragColor = mix(sqrt(fragColor),fragColor,.4) ;
    //fragColor = texture(iChannel0, fragCoord/iResolution.xy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
const float diffusionCoef = 3.0e-5;
const float reactionCoef = 50.;

vec2 hash( vec2 p ) {
	p = vec2( dot(p,vec2(127.1,311.7)), dot(p,vec2(269.5,183.3)) );
	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float simplexNoise( in vec2 p ) {
    const float K1 = 0.366025404; // (sqrt(3)-1)/2;
    const float K2 = 0.211324865; // (3-sqrt(3))/6;

	vec2  i = floor( p + (p.x+p.y)*K1 );
    vec2  a = p - i + (i.x+i.y)*K2;
    float m = step(a.y,a.x); 
    vec2  o = vec2(m,1.0-m);
    vec2  b = a - o + K2;
	vec2  c = a - 1.0 + 2.0*K2;
    vec3  h = max( 0.5-vec3(dot(a,a), dot(b,b), dot(c,c) ), 0.0 );
	vec3  n = h*h*h*h*vec3( dot(a,hash(i+0.0)), dot(b,hash(i+o)), dot(c,hash(i+1.0)));
    return dot( n, vec3(70.0) );
}

vec3 fractalNoise(in vec2 p) {
    vec2 uv;
    vec3 res;
    mat2 m = mat2( 1.6,  1.2, -1.2,  1.6 );
    uv = p*5.0 + vec2(10.76543, 30.384756);
	res.x  = 0.5000*simplexNoise( uv ); uv = m*uv;
	res.x += 0.2500*simplexNoise( uv ); uv = m*uv;
	res.x += 0.1250*simplexNoise( uv ); uv = m*uv;
    res.x += 0.0625*simplexNoise( uv ); uv = m*uv;
    uv = p*5.0 + vec2(14.87443, 508.12743);
	res.y  = 0.5000*simplexNoise( uv ); uv = m*uv;
	res.y += 0.2500*simplexNoise( uv ); uv = m*uv;
	res.y += 0.1250*simplexNoise( uv ); uv = m*uv;
    res.y += 0.0625*simplexNoise( uv ); uv = m*uv;
    uv = p*5.0 + vec2(83.21675, 123.45678);
	res.z  = 0.5000*simplexNoise( uv ); uv = m*uv;
	res.z += 0.2500*simplexNoise( uv ); uv = m*uv;
	res.z += 0.1250*simplexNoise( uv ); uv = m*uv;
    res.z += 0.0625*simplexNoise( uv ); uv = m*uv;
    return res;
}

vec3 laplacian(in vec2 uv) {
    vec2 step = vec2(1.0/iResolution.y, 0.0);
    return (
        texture(iChannel0, mod(uv + step.xy,1.0)).xyz +
        texture(iChannel0, mod(uv - step.xy,1.0)).xyz +
        texture(iChannel0, mod(uv + step.yx,1.0)).xyz +
        texture(iChannel0, mod(uv - step.yx,1.0)).xyz +
        -4.0 * texture(iChannel0, uv).xyz
    ) / (step.x*step.x);
}

void mainImage( out vec4 vals, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    if (iFrame==0) {
        vals.xyz = clamp(fractalNoise(uv+hash(iDate.zw)),0.0,1.0);
    } else {
        vals = texture(iChannel0, uv);
        float rho = vals.x + vals.y + vals.z;
        vals.xyz = vals.xyz + iTimeDelta * (
            diffusionCoef * laplacian(uv).xyz +
            vals.xyz * (1.0 - rho/3.0 - reactionCoef*vals.yzx)
        );
        vals.xyz = clamp(vals.xyz, 0.0, 1.0);
    }
}