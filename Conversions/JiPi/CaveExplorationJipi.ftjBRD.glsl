

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Based on "Stalactite Cave" by mazander - https://www.shadertoy.com/view/ldV3W1

// =================================== General functions ===================================

// Grey scale.
float getGrey(vec3 p){ return p.x*0.299 + p.y*0.587 + p.z*0.114; }

float noise(in vec3 p){
    vec3 i = floor(p); 
    p -= i; 
    p *= p*(3. - 2.*p); //from linear to smooth - derivatives at 0 and 1 are zero.
    // The RGBA noise satisfies G(x+37,y+17) = R(x,y), though only when VFLIP is unchecked.
	p.xy = texture(iChannel1, (p.xy + i.xy + vec2(37, 17)*i.z + .5)/256., -100.).yx;
	return mix(p.x, p.y, p.z);
}

// Project the 2D texture from the three main directions and mix them together to create a 3D texture.
vec4 texcube(sampler2D sam, in vec3 p, in vec3 n){
    p *= 1.5;    
	vec4 x = texture( sam, p.yz );
	vec4 y = texture( sam, p.zx );
	vec4 z = texture( sam, p.xy );
	return x*abs(n.x) + y*abs(n.y) + z*abs(n.z);
}

// =================================== Scene ===================================

// The path for the center of the cave
vec3 path(float time) {
    return vec3(cos(0.51 * time) + sin(0.14 * time), 0.8 * sin(0.27 * time), time); 
}

// Your friendly random orthogonal (rotation) matrix.
const mat3 m = mat3( 0.00, -0.80, -0.60,
                    -0.80,  0.36, -0.48,
                    -0.60, -0.48,  0.64 );

// Distance to the inside of the cave
float mapTerrain(vec3 position) {
    float initialCaveSize = 0.00;

    // an approximation to the distance vector from the path.
    vec3 xyDifference = position - path(position.z);
    
    // stretching the distance - the smaller a coordinate is (X or Y), the larger the cave is along that axis.
    vec3 stretchFactor = 1.2 * vec3(sin(position.z * 0.32), cos(position.z * 0.77), 1.0);
    float d = initialCaveSize-length(xyDifference * stretchFactor);
    
    // Add stalactites by adding noise which has lower frequency along the y-direction (So it changes faster
    // along the XZ plane).
    vec3 stalactites = vec3(6.0, 0.15, 6.0);
    d += 0.7500 * noise(stalactites * position); 
    
    // Add more random noise to add the smaller details
    float strength = 0.5;
    for (int i=0; i<5; i++){
        position = m*position*2.04;
        d += strength * noise( position );
        strength *= 0.5;
    }
        
    return d;
}

// moss color for a given point and normal.
vec3 moss(vec3 position, vec3 normal){
    // I have no idea how moss works...
    float moss_noise = noise(position*100.);
    vec3 moss_color = mix(vec3(0., 0.1, 0.0), vec3(0.3, 0.3, 0.0), moss_noise*moss_noise);
    moss_noise = noise(position*210.+100.);
    moss_color = mix(moss_color, vec3(0.3, 0.6, 0.3)*0.8, pow(moss_noise, 5.));
    moss_color*=2.*texture(iChannel1,position.xz).x;
    
    return moss_color;
}

// =================================== Compute position and normal ===================================

// Sets the orientation of the camera given its position and viewing target.
void camera(in vec3 origin, in vec3 target, out vec3 forward, out vec3 right, out vec3 up){
    forward = normalize(target-origin);
    right = normalize(vec3(forward.z, 0., -forward.x)); // Perpendicular to the forward direction ~ approx (1,0,0).
    up = normalize(cross(forward, right));
}

// Standard ray marching
float rayMarch(in vec3 ro, in vec3 rd){
	float maxd = 20.0;
    float t = 0.1;
    for( int i = 0; i< 160; i++ )
    {
	    float h = mapTerrain( ro + rd * t );
        if( h < (0.001 * t) || t > maxd ) break;
        t += (step(h, 1.) * .05 + 0.1) * h;
    }

    if( t>maxd ) t=-1.0;
    return t;
}

vec3 calcNormal( in vec3 pos, in float t ){
    vec3 eps = vec3( max(0.02,0.001*t),0.0,0.0);
	return normalize( vec3(
           mapTerrain(pos+eps.xyy) - mapTerrain(pos-eps.xyy),
           mapTerrain(pos+eps.yxy) - mapTerrain(pos-eps.yxy),
           mapTerrain(pos+eps.yyx) - mapTerrain(pos-eps.yyx) ) );

}

// Perturb the normal using a texture to add more details.
vec3 doBumpMap( sampler2D tex, in vec3 p, in vec3 nor, float bumpfactor){
   
    const float eps = 0.001;
    float ref = getGrey(texcube(tex,  p , nor).xyz);                 
    vec3 grad = vec3( getGrey(texcube(tex, vec3(p.x - eps, p.y, p.z), nor).xyz) - ref,
                      getGrey(texcube(tex, vec3(p.x, p.y - eps, p.z), nor).xyz) - ref,
                      getGrey(texcube(tex, vec3(p.x, p.y, p.z - eps), nor).xyz) - ref )/eps;
             
    // get the gradient's projection to the plane.
    grad -= nor*dot(nor, grad);          
                      
    return normalize(nor + grad*bumpfactor);	
}


// =================================== Light ===================================

/**
 * Look for the closest object along the given ray.
 * The closer this object is to the ray, and the quicker we get close to it,
 * we expect the shadow to be harder.
 * See Inigo Quilez tutorial - https://iquilezles.org/articles/rmshadows/
 */
float softshadow( in vec3 rayOrigin, in vec3 rayDirection, float mint, float maxt, float k )
{
    float res = 1.0;
    float t = mint;
    for( int i=0; i<100; i++ )
    {
        float h = mapTerrain(rayOrigin + rayDirection*t);
        res = min( res, k*max(h,0.0)/t );
        t += clamp( h, 0.01, 0.4 );
		if( res<0.001 || t>=maxt) break;
    }
    return clamp(res,0.0,1.0);
}

// Compute light value at the given point.
float lightValue(vec3 rayOrigin, vec3 position, vec3 normal, vec3 lightPosition){
    // diffuse
    vec3 lightDir = lightPosition - position;
    
    float diffuseFactor = clamp(dot(lightDir, normal), 0., 1.);
    float shadow = softshadow(position, normalize(lightDir), 0., length(lightDir), 2.); 
    
    return shadow * diffuseFactor / pow(length(lightDir),2.5);
}

float wisp(vec3 rayOrigin, vec3 rayDirection, float hitDistance, vec3 lightPosition){
    vec3 lightDirection = lightPosition - rayOrigin;
	float lightValue = pow(dot(normalize(lightDirection),normalize(rayDirection)),2000.0); // maybe add flickering?
    float distanceToLight = length(lightDirection);
    // return the light value if it is before the cave wall.
    return step(distanceToLight, hitDistance) * lightValue;
}

// =================================== Main ===================================

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{   
    // normalize coordinates.
    vec2 q = fragCoord.xy / iResolution.xy;
	vec2 p = -1.0 + 2.0*q;
	p.x *= iResolution.x / iResolution.y;
    
	float time = 3.8 + 0.6 * iTime;
    vec3 rayOrigin = path(time);
    // look further a head, and move head left and right along the way to see the scenery.
    vec3 target = path(time+1.6)+vec3((sin(time)+cos(time/2.))/2.,0,0); 
    // light moves before us
    vec3 lightPosition = path( time+2.5 + cos(time) + sin(1.2*time));
    
	// camera to world transform        
    vec3 forward, right, up;
    camera(rayOrigin, target, forward, right, up);
    vec3 rayDirection = normalize(2.*forward + right*p.x + up*p.y);
    
    vec3 col = vec3(0.0);
    float t = rayMarch(rayOrigin, rayDirection);
    if (t>0.){
        vec3 position = rayOrigin + t * rayDirection;
        vec3 normal = calcNormal(position, t);
        normal = doBumpMap(iChannel0 ,position, normal, 0.005);
        
        vec3 wall_col = texcube( iChannel0, 0.5*position, normal ).xyz;
        vec3 moss_color = moss(position, normal);
        
        float lValue = lightValue(rayOrigin, position, normal, lightPosition);
        wall_col *= lValue;        
        moss_color *= clamp(lValue, 0., 3.); // moss a bit less shiney, I think...
        
        // add more moss when normal is pointing up
        col = mix(wall_col, moss_color, smoothstep(0.1, 0.7, normal.y)); 
        
        // add water
        if(position.y < -1.0) {
         	col.z += 0.005;   // blue tint            
            col *= pow(0.4, position.y * position.y);    // darken when deep
        }
        
        col += wisp(rayOrigin, rayDirection, t, lightPosition);
    }    
    
    fragColor = vec4(col , 1.);    
}