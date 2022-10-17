

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
const float CELL_SIZE = 2.;

float cubeHeight(vec3 cell) {
    float t = iTime * 2.;
    return (sin(cell.x + t*0.125)*0.125+sin(cell.z + t)*0.125) * 15.;
}

vec3 cellTransform(vec3 m) {

   float cellSize = CELL_SIZE;

   vec3 cell = round(m / cellSize);
   cell.y = min(cell.y, -2.);

   m.y -= cubeHeight(cell);
   
   return m - cell * cellSize;
}

float udBox(vec3 p) {
    float a = CELL_SIZE*0.45;
    float corner = a * 0.1;

    return length(max(abs(p) - a, 0.)) - corner;
}

float map(vec3 m) {

   vec3 cell = round(m / CELL_SIZE);
   cell.y = -2.;

   float dist = 9999.;

   for(float i=-1.;i<2.;i+=1.) {
      for(float j=-1.;j<2.;j+=1.) {

            vec3 neighbour = cell;

            neighbour.x += i;
            neighbour.z += j;
        
            vec3 p = m - neighbour * CELL_SIZE;
            p.y -= cubeHeight(neighbour);        
        
            dist = min(dist, udBox(p));
        }
    }

    return dist;
}

bool rayMarch(in vec3 ro, in vec3 rd, out vec3 m, float max) {
    
    float md = 0.0;
   
    while(md < max) {       
    
        m = ro + rd * md;    
        
    	float dist = map(m);
        
        if(dist < 0.01) {
            return true;
        }

        md += min(CELL_SIZE, dist);
    }
    
	return false;    
}

vec3 computeNormal(in vec3 pos) {
	vec3 eps = vec3( 0.005, 0.0, 0.0 );
	vec3 nor = vec3(
	     map(pos+eps.xyy) - map(pos-eps.xyy),
	     map(pos+eps.yxy) - map(pos-eps.yxy),
	     map(pos+eps.yyx) - map(pos-eps.yyx));
	return normalize(nor);
}

const vec3 light = normalize(vec3(1,-1,-1));
const float SHADOW_FADE = 10.;

float computeShadow(vec3 p, vec3 light) {

    vec3 m;
    
    if(rayMarch(p - light * 0.05, -light, m, SHADOW_FADE)) {
        float distFactor = clamp(length(m-p)/SHADOW_FADE, 0., 1.);
        return 0.5 + smoothstep(0., 1., distFactor) * 0.5;
    }

    return 1.;
}

vec4 text3d(vec3 p, vec3 n) {
    p=p*.2;//IQ made it
    vec3 a = n*n;
	vec4 x = texture(iChannel1, p.yz );
	vec4 y = texture(iChannel1, p.zx );
	vec4 z = texture(iChannel1, p.yx );
	return (x*a.x + y*a.y + z*a.z) / (a.x + a.y + a.z);
}

vec4 render(vec3 ro, vec3 rd, vec3 m, vec3 normal, vec4 env) {

    float shadow = computeShadow(m, light);
    float diffuse = clamp(dot(normal, -light), 0.,1.);
    float hilight = pow(clamp(dot(reflect(light, normal), rd), 0., 1.), 40.) * 0.;
    
    vec4 baseColor = text3d(cellTransform(m), normal);
    
    vec4 testMask = vec4(baseColor.g);
//    return testMask;
    
    float fresnel = (1. - clamp(dot(-rd, normal), 0., 1.))*0.25;    
    
    return (vec4(0.25 + diffuse) * mix(baseColor, env, fresnel) + hilight) * shadow;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec3 m;
    vec2 uv = (fragCoord - iResolution.xy * 0.5) / iResolution.y;

    float angleY = 3.1415*0.25*iTime * 0.1;
    float cosa = cos(angleY);
    float sina = sin(angleY);    

    mat3 rotY = mat3(vec3(cosa, 0., sina), vec3(0., 1., 0.), vec3(-sina, 0., cosa));

    float angleX = 0.7;//iTime * 0.;
    cosa = cos(angleX);
    sina = sin(angleX);    

    mat3 rotX = mat3(vec3(1., 0., 0.), vec3(0., cosa, sina), vec3(0., -sina, cosa));

    mat3 transfo = rotY * rotX;

    float camAnim = iTime*0.;
    vec3 camera = vec3(0, -2., -10. + 1.5*sin(camAnim));
    
    vec3 ro = camera;
    vec3 rd	= normalize(vec3(uv.xy, 1.));

    ro = transfo * ro;
    rd = transfo * rd;

    if(rayMarch(ro, rd, m, 100.)) {
        vec3 normal = computeNormal(m);
        
        vec3 refl = reflect(rd, normal);
        vec3 m2;
        
        vec4 env; 
        
        if(rayMarch(m + refl*0.015, refl, m2, 100.)) {
            vec3 normal2 = computeNormal(m2);
            vec3 localRefl = reflect(refl, normal2);
            vec4 localEnv = texture(iChannel0, localRefl); 
            env = render(m, refl, m2, normal2, localEnv);
        }
        else {
            env = texture(iChannel0, refl);
        }
        
        fragColor = render(ro, rd, m, normal, env);
    }
    else {
       fragColor = vec4(0); 
    }
}
    
    
