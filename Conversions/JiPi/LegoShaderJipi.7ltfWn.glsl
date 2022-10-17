

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
struct Ray {
    vec3 origin;
    vec3 direction;
};

mat3 rotationMatrix;
mat3 rotationMatrix_inv;

float smoothMin(float a, float b, float k) {
    float h = max(k - abs(a - b), 0.0) / k;
    return min(a, b) - h * h * h * 1.0 / 6.0;
}

float sdSphere(vec3 pos, vec3 spherePos, float sphereRadius) {
    return distance(pos, spherePos) - sphereRadius;
}

float sdRoundBox( vec3 p, vec3 b, float r )
{
    vec3 q = abs(p) - b;
    return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0) - r;
}

float sdBox2( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

float sdBox(vec3 pos, vec3 boxPos, vec3 dimensions) {
    float x = abs(pos.x - boxPos.x) - dimensions.x * 0.5;
    float y = abs(pos.y - boxPos.y) - dimensions.y * 0.5;
    float z = abs(pos.z - boxPos.z) - dimensions.z * 0.5;
    float d = max(x, max(y, z));
    return d;
}

float sdRoundedCylinder( vec3 p, float ra, float rb, float h )
{
  vec2 d = vec2( length(p.xz)-2.0*ra+rb, abs(p.y) - h );
  return min(max(d.x,d.y),0.0) + length(max(d,0.0)) - rb;
}

float sdCylinder(vec3 pos, vec3 cylinderPos, float height, float radius) {
    float y = abs(pos.y - cylinderPos.y) - height * 0.5;
    float xz = distance(pos.xz, cylinderPos.xz) - radius;
    float d = max(xz, y);
    return d;
}

float sdBrick(vec3 pos, vec3 brickPos, vec2 brickSize) {
    return sdRoundBox(pos - brickPos, vec3(0.4 * brickSize.x - 0.03, 0.47, 0.4 * brickSize.y - 0.03), 0.03);
}

float sdSmallBrick(vec3 pos, vec3 brickPos, vec2 brickSize) {
    return sdRoundBox(pos - brickPos, vec3(0.4 * brickSize.x - 0.03, 0.13666, 0.4 * brickSize.y - 0.03), 0.03);
}

float sdKnob(vec3 pos, vec3 knobPos) { 
    return sdRoundedCylinder(pos - knobPos, 0.13, 0.01, 0.16);
}

float sdSmallBrick2x2(vec3 pos, vec3 brickPos) {
    const float offset = 0.29 * 1.5;
    const float k = 0.0;

    return min(
        sdRoundBox(pos - brickPos, vec3(0.77, 0.13666, 0.77), 0.03),
        min(
            min(
                sdRoundedCylinder(pos - (brickPos + vec3(+0.4, 0.1666, +0.4)), 0.13, 0.01, 0.16),
                sdRoundedCylinder(pos - (brickPos + vec3(-0.4, 0.1666, +0.4)), 0.13, 0.01, 0.16)
            ),
            min(
                sdRoundedCylinder(pos - (brickPos + vec3(-0.4, 0.1666, -0.4)), 0.13, 0.01, 0.16),
                sdRoundedCylinder(pos - (brickPos + vec3(+0.4, 0.1666, -0.4)), 0.13, 0.01, 0.16)
            )
        )
    );
}

float orangeMap(vec3 pos) {
    float d = pos.y + 0.1666;
    float t = (-cos(iTime * 0.3) * 0.5 + 0.5) * 1.2 + 0.05;
    
    const vec3 brickSize = vec3(0.8, 1.0, 0.8);
    
    if(t > 0.1) {
        float y = 1.0 - (min(t - 0.1, 0.1) / 0.1);
    
        d = min(d, sdSmallBrick(pos, vec3(0.0, 0.0 + y * 10.0, 0.0), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 0.1666 + y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 0.1666 + y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 0.1666 + y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(+0.4, 0.1666 + y * 10.0, -0.4)));
    }
    
    if(t > 0.8) {
        float y = 1.0 - (min(t - 0.8, 0.1) / 0.1);
        d = min(d, sdSmallBrick(pos, vec3(0.0, 3.7333+ y * 10.0, -0.8), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 3.9+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.9+ y * 10.0, -0.4)));  
        d = min(d, sdKnob(pos, vec3(+0.4, 3.9+ y * 10.0, -1.2)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.9+ y * 10.0, -1.2)));
    }
    
    return d;
}

float map(vec3 pos) {
    float d = pos.y + 0.1666;
    float t = (-cos(iTime * 0.3) * 0.5 + 0.5) * 1.2 + 0.05;
    
    const vec3 brickSize = vec3(0.8, 1.0, 0.8);
    
    //if(t > 0.1) {
        float y = 1.0 - (min(t - 0.1, 0.1) / 0.1);
    
        d = min(d, sdSmallBrick(pos, vec3(0.0, 0.0 + y * 10.0, 0.0), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 0.1666 + y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 0.1666 + y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 0.1666 + y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(+0.4, 0.1666 + y * 10.0, -0.4)));
    //}
    
    //if(t > 0.2) {
         y = 1.0 - (min(t - 0.2, 0.1) / 0.1);
        d = min(d, sdBrick(pos, vec3(0.0, 0.6666+ y * 10.0, 0.4), vec2(2, 1)));
        d = min(d, sdKnob(pos, vec3(+0.4, 1.1666+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 1.1666+ y * 10.0, +0.4)));
    //}
    
    //if(t > 0.3) {
         y = 1.0 - (min(t - 0.3, 0.1) / 0.1);
        d = min(d, sdSmallBrick(pos, vec3(0.0, 1.3666+ y * 10.0, 0.4), vec2(2, 3)));
        d = min(d, sdKnob(pos, vec3(+0.4, 1.5333+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 1.5333+ y * 10.0, +0.4)));        
        d = min(d, sdKnob(pos, vec3(+0.4, 1.5333+ y * 10.0, +1.2)));
        d = min(d, sdKnob(pos, vec3(-0.4, 1.5333+ y * 10.0, +1.2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 1.5333+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 1.5333+ y * 10.0, -0.4)));
    //}
    
    //if(t > 0.4) {
         y = 1.0 - (min(t - 0.4, 0.1) / 0.1);
        d = min(d, sdBrick(pos, vec3(0.0, 2.0666+ y * 10.0, 0.0), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 2.5666+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 2.5666+ y * 10.0, +0.4)));  
        d = min(d, sdKnob(pos, vec3(+0.4, 2.5666+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 2.5666+ y * 10.0, -0.4)));
    //}
    
    //if(t > 0.5) {
         y = 1.0 - (min(t - 0.5, 0.1) / 0.1);
        d = min(d, sdBrick(pos, vec3(0.0, 2.0666+ y * 10.0, 1.6), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 2.5666+ y * 10.0, +2.0)));
        d = min(d, sdKnob(pos, vec3(-0.4, 2.5666+ y * 10.0, +2.0)));  
        d = min(d, sdKnob(pos, vec3(+0.4, 2.5666+ y * 10.0, +1.2)));
        d = min(d, sdKnob(pos, vec3(-0.4, 2.5666+ y * 10.0, +1.2)));
    //}
    
    //if(t > 0.6) {
         y = 1.0 - (min(t - 0.6, 0.1) / 0.1);
        d = min(d, sdBrick(pos, vec3(0.0, 3.0666+ y * 10.0, 0.8), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 3.5666+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.5666+ y * 10.0, +0.4)));  
        d = min(d, sdKnob(pos, vec3(+0.4, 3.5666+ y * 10.0, +1.2)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.5666+ y * 10.0, +1.2)));
    //}
    
    //if(t > 0.7) {
         y = 1.0 - (min(t - 0.7, 0.1) / 0.1);
        d = min(d, sdBrick(pos, vec3(0.0, 3.0666+ y * 10.0, -0.4), vec2(4, 1)));
        d = min(d, sdKnob(pos, vec3(+0.4, 3.5666+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.5666+ y * 10.0, -0.4)));  
        d = min(d, sdKnob(pos, vec3(+1.2, 3.5666+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-1.2, 3.5666+ y * 10.0, -0.4)));
    //}
    
    //if(t > 0.8) {
         y = 1.0 - (min(t - 0.8, 0.1) / 0.1);
        d = min(d, sdSmallBrick(pos, vec3(0.0, 3.7333+ y * 10.0, -0.8), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 3.9+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.9+ y * 10.0, -0.4)));  
        d = min(d, sdKnob(pos, vec3(+0.4, 3.9+ y * 10.0, -1.2)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.9+ y * 10.0, -1.2)));
    //}
    
    //if(t > 0.9) {
         y = 1.0 - (min(t - 0.9, 0.1) / 0.1);
        d = min(d, sdSmallBrick(pos, vec3(0.0, 3.7333+ y * 10.0, +0.4), vec2(2, 1)));
        d = min(d, sdKnob(pos, vec3(+0.4, 3.9+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 3.9+ y * 10.0, +0.4)));
    //}
    
    //if(t > 1.0) {
         y = 1.0 - (min(t - 1.0, 0.1) / 0.1);
        d = min(d, sdBrick(pos, vec3(0.0, 4.4+ y * 10.0, 0.0), vec2(2, 2)));
        d = min(d, sdKnob(pos, vec3(+0.4, 4.9+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 4.9+ y * 10.0, +0.4)));
        d = min(d, sdKnob(pos, vec3(+0.4, 4.9+ y * 10.0, -0.4)));
        d = min(d, sdKnob(pos, vec3(-0.4, 4.9+ y * 10.0, -0.4)));
    //}
    
    return d;
}
 
#define ORG //ORG: 4.0s  else 2.7s
#ifdef ORG
vec3 calcNormal(vec3 pos) {
    const float epsilon = 0.001;
    float dist0 = map(pos);
    float dist1 = map(pos + vec3(epsilon, 0.0, 0.0));
    float dist2 = map(pos + vec3(0.0, epsilon, 0.0));
    float dist3 = map(pos + vec3(0.0, 0.0, epsilon));
    return (vec3(dist1, dist2, dist3) - dist0) / epsilon;
}

#else
vec3 calcNormal(vec3 pos)
{
    const float epsilon = 0.001;
    vec3 n = vec3(0.0);
    for( int i=min(iFrame,0); i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+epsilon*e);
    }
    return normalize(n);
}

#endif

struct Hit {
    vec3 position;
    float closestDistance;
    int raySteps;
    bool hit;
};

Hit castRay(Ray ray) {
    Hit hit;
    hit.position = ray.origin;
    hit.raySteps = 0;
    hit.hit = false;
    
    float hitDist = 0.0;
    for(; hit.raySteps < 100; ++hit.raySteps) {
        hit.position = ray.origin + ray.direction * hitDist;
        
        float sd = map(hit.position * rotationMatrix);
        
        if(sd < 0.01) { 
            hit.hit = true;
            break;
        }
        
        hitDist += sd;
    }
    
    return hit;
}

bool obscured(Ray ray) {
    float occlusion = 1.0;
    vec3 position;
    float hitDist = 0.0;
    for(int raySteps = 0; raySteps < 100; ++raySteps) {
        position = ray.origin + ray.direction * hitDist;
        float sd = map(position * rotationMatrix);
        
        if(sd < 0.01) { 
            return true;
        }
        
        hitDist += sd;
    }
    return false;
}

float softShadow(Ray ray, float k) {
    float visibility = 1.0;
    vec3 position;
    float hitDist = 0.0;
    for(int raySteps = 0; raySteps < 100; ++raySteps) {
        position = ray.origin + ray.direction * hitDist;
        float sd = map(position * rotationMatrix);
        
        if(sd < 0.01) { 
            return 0.0;
        }
        
        visibility = max(smoothMin(visibility, k * sd/hitDist, 0.3), 0.0);
        hitDist += sd;
    }
    return visibility;
}

vec3 calculateColor(vec2 seed, vec3 normal, vec3 viewDir, vec3 position, vec3 albedo, vec3 reflection, int raySteps) {
    const vec3 lightDir = normalize(vec3(0.4, 1.0, -0.8));
    float diffuse = max(dot(normal, lightDir), 0.0);
    
    const vec3 orange = vec3(pow(1.0,2.2), pow(0.5,2.2), pow(0.3,2.2));
    const vec3 yellow = vec3(pow(1.0,2.2), pow(0.8,2.2), pow(0.3,2.2));
    
    float r0 = 0.05;
    float ao = 0.0;
    if(position.y > -0.1) {
        ao = pow(1.0 / float(raySteps), 0.3);
        albedo = yellow;
        if(orangeMap(position * rotationMatrix) < 0.01)
            albedo = orange;
    }else{
        r0 = 0.0;
        ao = pow(1.0 / float(raySteps), 0.1);
    }
    
    /*float shadow = 0.0;
    for(int i = 0; i < 4; i++) {
        vec3 rand = textureLod(iChannel0, seed, 0.0).xyz;
        seed = rand.xy;
    
        Ray shadowRay;
        shadowRay.direction = lightDir + rand - 0.5;
        shadowRay.origin = position + shadowRay.direction * 0.15;

        shadow += obscured(shadowRay) ? 0.0 : 1.0;
    }
    shadow /= 4.0*/;
    
    
    Ray shadowRay;
    shadowRay.direction = lightDir;
    shadowRay.origin = position + shadowRay.direction * 0.15;
    float shadow = softShadow(shadowRay, 2.0);
    
    
    diffuse = diffuse * shadow * 0.5 + 0.65;
    
    float fresnel = r0 + (1.0 - r0) * pow(1.0 - dot(-viewDir, normal), 5.0);
    
    float specular = max(dot(reflect(viewDir, normal), lightDir), 0.0) * fresnel;
    
    return (albedo * diffuse + fresnel * reflection + specular) * ao;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 screenPos = (fragCoord - iResolution.xy * 0.5) / iResolution.y;
    
    Ray ray;
    ray.origin = vec3(0.0, 9.0, -8.0);
    ray.direction = normalize(vec3(screenPos, 1.0));
    const float cameraRotation = 40.0;
    const float cameraRotSin = sin(cameraRotation * 0.0174532);
    const float cameraRotCos = cos(cameraRotation * 0.0174532);
    const mat3 cameraMatrix = mat3(
        1.0, 0.0, 0.0,
        0.0, cameraRotCos, -cameraRotSin,
        0.0, cameraRotSin, cameraRotCos
    );
    ray.direction *= cameraMatrix;
    
    float rot = iTime * 0.8;
    float rotSin = sin(rot);
    float rotCos = cos(rot);
    rotationMatrix = mat3(
    rotCos, 0.0, rotSin,
    0.0, 1.0, 0.0,
    -rotSin, 0.0, rotCos);
    
    float rot_inv = -rot;
    float rotSin_inv = sin(rot_inv);
    float rotCos_inv = cos(rot_inv);
    rotationMatrix_inv = mat3(
    rotCos_inv, 0.0, rotSin_inv,
    0.0, 1.0, 0.0,
    -rotSin_inv, 0.0, rotCos_inv);
    
    
    Hit hit = castRay(ray);
    
    if(hit.hit) {
        vec3 normal = normalize(calcNormal(hit.position * rotationMatrix));
        
        Ray reflectionRay;
        reflectionRay.direction = reflect(ray.direction, normal);
        reflectionRay.origin = hit.position + reflectionRay.direction * 0.02;
        Hit reflection = castRay(reflectionRay);
        vec3 reflectionCol = vec3(1.0, 1.0, 1.0);
        if(reflection.hit)
            reflectionCol = calculateColor(screenPos.xy, normalize(calcNormal(reflection.position * rotationMatrix)) * rotationMatrix_inv, reflectionRay.direction, reflection.position, vec3(1.0, 1.0, 1.0), vec3(1.0, 1.0, 1.0), hit.raySteps);
        vec3 col = calculateColor(screenPos.xy, normal * rotationMatrix_inv, ray.direction, hit.position, vec3(1.0, 1.0, 1.0), reflectionCol, hit.raySteps);
        fragColor = vec4(pow(col.x, 1.0 / 2.2), pow(col.y, 1.0 / 2.2), pow(col.z, 1.0 / 2.2), 0.0);//vec4(normal, 1.0) * 0.5 + 0.5;
    } else {
        fragColor = vec4(1.0, 1.0, 1.0, 1.0);
    }   
}