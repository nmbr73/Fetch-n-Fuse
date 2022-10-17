

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define PI 3.1415926535
#define SQRT2 1.4142135624

#define CAM_FOV_DEG 45.0
const float CAM_DIST = 1.0/tan(CAM_FOV_DEG * 0.5 * PI / 180.0);

#define saturate(x) clamp(x, 0.0, 1.0)

float lerp(float a, float b, float t) {
    t = saturate(t);
    return (1.0-t)*a + t*b;
}

vec3 lerp(vec3 a, vec3 b, float t) {
    t = saturate(t);
    return (1.0-t)*a + t*b;
}

bool intersectXYPlane(vec3 org, vec3 dir, inout float dist) {
    const float epsilon = 1e-6;
    vec3 normal = vec3(0.0, 0.0, 1.0);

    float denom = dot(-normal, dir); 
    if (denom > epsilon) { 
        dist = dot(-org, -normal) / denom; 
        return (dist >= 0.0); 
    } 
 
    return false; 
}

float hash(vec2 p) {
	return 2.0*fract(sin(dot(p, vec2(12.9898, 78.233))) * 43758.5453)-1.0;   
}

vec2 hash2(vec2 P) {
 	return fract(cos(P*mat2(-64.2,71.3,81.4,-29.8))*8321.3); 
}

float noise(vec2 p) {
    vec2 id = floor(p);
    vec2 u = fract(p);
    
    float a = hash(id+vec2(0,0));
    float b = hash(id+vec2(1,0));
    float c = hash(id+vec2(0,1));
    float d = hash(id+vec2(1,1));
    
    u = u*u*u*(u*(6.0*u-15.0)+10.0);
    
    float k0 = a;
    float k1 = b-a;
    float k2 = c-a;
    float k3 = a-b-c+d;
    
    return k0 + k1*u.x + k2*u.y + k3*u.x*u.y;       
}

float fbm(in vec2 p, int octaves) {
    const float scale_y = 2.0;
    const float scale_xz = 0.125;
    const mat2 rot = mat2(0.8, 0.6, -0.6, 0.8);
    
    p *= scale_xz;

    float res = 0.0;
    mat2 M = mat2(1.0);
    
    float A = 1.0, a = 1.0;
    
    for (int i=1; i<=octaves; i++) {
        res += A*noise(a*M*p);
        
        a *= 2.0;
        A *= 0.5;
        M *= rot;
    }
    
    return scale_y*res;
}

float edge_noise(vec2 p, float grid_size) {
    vec2 q = p - mod(p, vec2(grid_size));
    
    vec2 res = vec2(SQRT2*grid_size);
    
    vec2 first, second;
    
    for (int i=-1; i<=1; i++) {
        for (int j=-1; j<=1; j++) {
            vec2 v = q + grid_size * vec2(i,j);
            vec2 rand = v + grid_size*hash2(v);
            
            float d = length(p-rand);
            
            if (d < res.y) {
                res.x = res.y;
                res.y = d;
                second = first;
                first = rand;
            }
            
            else if (d < res.x) {
                res.x = d;
                second = rand;
            }
        }
    }
    
    float dist = dot(0.5*(first+second) - p, normalize(second-first));
    
    return dist/(SQRT2 * grid_size);
}

float stains(vec2 p) {    
    const float scale = 33.0;
    float wy = saturate(0.7+0.1*fbm(scale*p, 8));
    
    
    return smoothstep(0.7, 0.75, wy - 0.07);
}

float cracks(vec2 p) {
    const float scale = 50.0;
    const vec2 offset = vec2(5.2, 1.3);
    
    p += 0.015 * vec2(fbm(scale*p, 5), fbm(scale*(p+offset), 5));

    const float grid_size = 0.04;
    float we = edge_noise(p, grid_size);
    we = 1.0 - smoothstep(0.0, 0.06, we);
    
    return saturate( (0.44 + fbm(66.0*p, 3)) * we);
}

vec2 brickCoords(vec2 uv, vec2 dims, float offset) {
    if (int(floor(uv.y/dims.y)) % 2 == 0)
        uv += vec2(offset*dims.x, 0.0);

    return dims * mod(uv/dims, 1.0);
}

float brick_id(vec2 uv) {
    const vec2 dims = vec2(0.3, 0.1);
    const float offset = 0.5;

    if (int(floor(uv.y/dims.y)) % 2 == 0)
        uv += vec2(offset*dims.x, 0.0);
        
    vec2 ts = floor(uv/dims);
    return hash(ts);
}

float brickHeightmap(vec2 uv, vec2 dims, float offset, vec2 edge){
    vec2 ts = brickCoords(uv, dims, offset);
    
    float x = smoothstep(0.0, edge.x, ts.x) * smoothstep(dims.x, dims.x-edge.x, ts.x);
    float y = smoothstep(0.0, edge.y, ts.y) * smoothstep(dims.y, dims.y-edge.y, ts.y);
    
    return x*y;
}

#define MAT_BRICK 0
#define MAT_MORTAR 1

float Heightmap(vec2 uv, inout int mat_id) {
    //brick params
    const vec2 dims = vec2(0.3, 0.1);
    const float offset = 0.5, height = 1.1;
    const vec2 edge = vec2(0.017);
    //brick noise params
    const float scale = 100.0, strength = 0.15;
    const int octaves = 4;
    //mortar params
    const float m_scale = 100.0, m_strength = 0.25, m_base = 0.4;
    const int m_octaves = 5;
    
    float mortar_height = m_base + m_strength * fbm(m_scale*uv, m_octaves);
    
    float brick_noise = strength * fbm(scale*uv, octaves);
    float brick_height = height * brickHeightmap(uv, dims, offset, edge) + brick_noise;
    brick_height -= 0.1*cracks(uv);
    
    if (brick_height > mortar_height) {
        mat_id = MAT_BRICK;
        return brick_height;
    }
    
    else {
        mat_id = MAT_MORTAR;
        return mortar_height;
    }
}

vec3 normal(vec2 uv) {
    const float smoothness = 0.1;
    const vec2 h = vec2(0.0, 0.001);
    int id;

    float dx = Heightmap(uv + h.yx, id) - Heightmap(uv - h.yx, id);
    float dy = Heightmap(uv + h.xy, id) - Heightmap(uv - h.xy, id);
             
    return normalize(vec3(-dx, -dy, smoothness));   
}

vec3 albedo(vec2 uv) {
    int id = MAT_BRICK;
    float height = Heightmap(uv, id);
    
    vec3 col = vec3(0.0);
    
    if (id == MAT_BRICK) {
        float darkening = 0.25 * abs(brick_id(uv));
        col = vec3(0.6, 0.3, 0.1) - vec3(darkening);
    }
        
    else if (id == MAT_MORTAR) {
        col = vec3(0.70, 0.66, 0.58);
    }
    
    float s = stains(uv);
    float s2 = stains(uv + vec2(7.25, 3.73));
    
    col = lerp(col, s2*vec3(0.22), s2 - 0.25);
    col = lerp(col,  s*vec3(0.66), s  - 0.25);
    
    return col;
}

float light(vec3 pos, vec3 norm, vec3 dir) {
    vec3 l_pos = vec3(-0.2, -0.2, 1.0);
    
    vec3 dif = l_pos - pos;
    
    float attenuation = dot(dif, dif);
    
    dif = normalize(dif);
    
    vec3 R = reflect(dif, norm);
    float spec = 0.1 * pow(saturate(dot(dir, R)), 5.0);
    
    return (spec + saturate(dot(dif, norm)))/attenuation;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = (2.0*fragCoord - iResolution.xy)/iResolution.y;

    vec3 ray_org = vec3(0.0, 0.0, 1.0);
    vec3 ray_dir = normalize(vec3(uv, -CAM_DIST));
    
    vec3 col = vec3(0.0);
    
    float dist = 0.0;
    if (intersectXYPlane(ray_org, ray_dir, dist)) {
        vec3 hit = ray_org + dist * ray_dir;
        
        vec3 norm = normal(hit.xy);
        col = albedo(hit.xy) * vec3(light(hit, norm, ray_dir));
    }
    
    fragColor = vec4(col,1.0);
}