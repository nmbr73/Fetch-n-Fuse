

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float rand_n(float r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        r = fract(r * 100.0);
    }
    
    return r;
}

void mainImage(out vec4 frag_color, in vec2 frag_coord)
{
    ivec2 coord = ivec2(frag_coord / scale);
    Particle self = particle_at(coord, iResolution.xy, iChannel0);
    
    switch (self.type)
    {
        case SAND:
        {
            frag_color = vec4(
                mix(0.9, 1.0, rand_n(self.shade, 1)), 
                mix(0.75, 0.8, rand_n(self.shade, 2)),
                mix(0.5, 0.6, rand_n(self.shade, 3)), 1.0) * mix(0.7, 1.0, self.shade);
            return;
        } break;
        
        case WALL:
        {
            frag_color = vec4(
                mix(0.3, 0.4, rand_n(self.shade, 1)), 
                mix(0.3, 0.4, rand_n(self.shade, 2)),
                mix(0.3, 0.4, rand_n(self.shade, 3)), 1.0) * mix(0.7, 1.0, self.shade);
            return;
        } break;
        
        case AIR:
        {
            frag_color = vec4(0.6, 0.8, 1.0, 1.0);
            return;
        }
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// SETTINGS
const float scale        = 2.0;
const float brush_radius = 40.0;

// COMMON
#ifndef HW_PERFORMANCE
uniform vec3      iResolution;
uniform sampler2D iChannel0;
uniform float     iTime;
uniform int       iFrame;
#endif

int seed;

float rand()
{
    int n = (seed++ << 13) ^ seed;
    return float((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.0;
}

#define INIT_SEED() \
    seed = int(iTime * frag_coord.x + frag_coord.y * iResolution.x); \
    seed = int(rand() * 2147483647.0) + iFrame;

const int AIR  = 0;
const int SAND = 1;
const int WALL = 2;

struct Particle
{
    int   type;
    bool  has_moved_this_frame;
    float shade;
};

#define IN_BOUNDS(coord) (0 <= (coord).x && (coord).x < int(iResolution.x / scale) && 0 <= (coord).y && (coord).y < int(iResolution.y / scale))
#define NOT_IN_BOUNDS(coord) ((coord.x) < 0 || int(iResolution.x / scale) <= (coord.x) || (coord.y) < 0 || int(iResolution.y / scale) <= (coord.y))


Particle particle_at(ivec2 coord, vec2 iR, sampler2D iChannel0)
{
    if (NOT_IN_BOUNDS(coord))
    {
        return Particle(WALL, false, 0.0);
    }
    
    //vec4 data = texelFetch(iChannel0, coord, 0);
    vec4 data = texture(iChannel0, (vec2(coord)+0.5)/iR);
    
    return Particle(
        int(data.x),
        bool(data.y),
        data.z
    );
}

vec4 to_vec4(Particle particle)
{
    return vec4(
        float(particle.type),
        float(particle.has_moved_this_frame),
        particle.shade,
        0.0
    );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
float distance_to_line(vec2 a, vec2 b, vec2 p)
{
    // from iq: https://www.youtube.com/watch?v=PMltMdi1Wzg
    float h = clamp(dot(p - a, b - a) / dot(b - a, b - a), 0.0, 1.0);
    return length(p - mix(a, b, h));
}

const float KEY_SPACE = 32.0;
const float KEY_LEFT_SHIFT = 16.0;


bool key_pressed(float key_code)
{
    return texture(iChannel2, vec2((key_code + 0.5) / 256.0, 0.5 / 3.0)).r > 0.0;
}

void mainImage(out vec4 frag_color, in vec2 frag_coord)
{
    ivec2 coord = ivec2(frag_coord);
    if (NOT_IN_BOUNDS(coord))
    {
        return;
    }
    
    INIT_SEED();
    
    //vec4 pMouse = texelFetch(iChannel1, ivec2(0, 0), 0);
    vec4 pMouse = texture(iChannel1, (vec2(ivec2(0, 0))+0.5)/iResolution.xy);
    
    if (pMouse.z > 0.0)
    {
        if (iMouse.z > 0.0 && distance_to_line(pMouse.xy / scale, iMouse.xy / scale, vec2(coord)) < brush_radius)
        {
            frag_color = to_vec4(Particle(
                (key_pressed(KEY_SPACE) ? AIR : key_pressed(KEY_LEFT_SHIFT) ? WALL : SAND), 
                true, mix(0.7, 1.0, rand())
            ));
            return;
        }
    }
    else
    {
        if (iMouse.z > 0.0 && distance(vec2(coord), iMouse.xy / scale) < brush_radius)
        {
            frag_color = to_vec4(Particle(
                (key_pressed(KEY_SPACE) ? AIR : key_pressed(KEY_LEFT_SHIFT) ? WALL : SAND), 
                true, mix(0.7, 1.0, rand())
            ));
            return;
        }
    }
    
    Particle self = particle_at(coord, iResolution.xy, iChannel0);
    
    if(rand()>0.1)
    
    switch (self.type)
    {
        case SAND:
        {
            Particle below = particle_at(coord + ivec2(0, -1), iResolution.xy, iChannel0);
            
            
            if (below.type == AIR)
            {
                frag_color = to_vec4(Particle(AIR, true, 0.0));
                return;
            }
        } break;
        
        case AIR:
        {
            Particle above = particle_at(coord + ivec2(0, 1), iResolution.xy, iChannel0);
            
            if (above.type == SAND)
            {
                above.has_moved_this_frame = true;
                frag_color = to_vec4(above);
                return;
            }
        } break;
    }
    
    frag_color = to_vec4(Particle(self.type, false, self.shade));
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage(out vec4 frag_color, in vec2 frag_coord)
{
    ivec2 coord = ivec2(frag_coord);
    if (NOT_IN_BOUNDS(coord))
    {
        return;
    }
    
    Particle self = particle_at(coord, iResolution.xy, iChannel0);
    
    if (self.has_moved_this_frame)
    {
        frag_color = to_vec4(self);
        return;
    }
    
    INIT_SEED();
    
    int dir = int(rand() < 0.5) * 2 - 1;
  
    //if(rand()>0.5)
    switch (self.type)
    {
        case SAND:
        {
            Particle below = particle_at(coord + ivec2(dir, -1), iResolution.xy, iChannel0);
            
            if (!below.has_moved_this_frame && below.type == AIR)
            {
                frag_color = to_vec4(Particle(AIR, true, 0.0));
                return;
            }
        } break;
        
        case AIR:
        {
            Particle above = particle_at(coord + ivec2(-dir, 1), iResolution.xy, iChannel0);
            
            if (!above.has_moved_this_frame && above.type == SAND)
            {
                above.has_moved_this_frame = true;
                frag_color = to_vec4(above);
                return;
            }
        } break;
    }
    
    frag_color = to_vec4(self);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage(out vec4 frag_color, in vec2 frag_coord)
{
    ivec2 coord = ivec2(frag_coord);
    if (NOT_IN_BOUNDS(coord))
    {
        return;
    }
    
    Particle self = particle_at(coord, iResolution.xy, iChannel0);
    
    if (self.has_moved_this_frame)
    {
        frag_color = to_vec4(self);
        return;
    }
    
    INIT_SEED();
    rand();
    
    int dir = int(rand() >= 0.5) * 2 - 1;

    //if(rand()>0.5)
    switch (self.type)
    {
        case SAND:
        {
            Particle below = particle_at(coord + ivec2(dir, -1), iResolution.xy, iChannel0);
            
            if (!below.has_moved_this_frame && below.type == AIR)
            {
                frag_color = to_vec4(Particle(AIR, true, 0.0));
                return;
            }
        } break;
        
        case AIR:
        {
            Particle above = particle_at(coord + ivec2(-dir, 1), iResolution.xy, iChannel0);
            
            if (!above.has_moved_this_frame && above.type == SAND)
            {
                above.has_moved_this_frame = true;
                frag_color = to_vec4(above);
                return;
            }
        } break;
    }
    
    frag_color = to_vec4(self);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage(out vec4 frag_color, in vec2 frag_coord)
{
    if (frag_coord == vec2(0.5, 0.5))
    {
        frag_color = iMouse;
    }
    else
    {
        frag_color = vec4(0.0);
    }
}