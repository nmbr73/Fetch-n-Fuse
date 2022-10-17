
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


    #define AIR   0
    #define SAND  1
    #define WALL  2
    #define WATER 3


// SETTINGS
//const float scale        = 4.0f;
//const float brush_radius = 40.0f;

/*
// COMMON
#ifndef HW_PERFORMANCE
uniform float3      iResolution;
uniform sampler2D iChannel0;
uniform float     iTime;
uniform int       iFrame;
#endif
*/

//int seed;

__DEVICE__ float rand(int seed)
{
    int n = (seed++ << 13) ^ seed;
    return (float)((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff) / 2147483647.0f;
}

#define INIT_SEED() \
    int seed = (int)(iTime * frag_coord.x + frag_coord.y * iResolution.x); \
    seed = (int)(rand(seed) * 2147483647.0f) + iFrame;



struct Particle
{
    int   type;
    bool  has_moved_this_frame;
    float shade;
};

#define IN_BOUNDS(coord)     (0 <= (coord).x && (coord).x < (int)(iResolution.x / scale) && 0 <= (coord).y && (coord).y < (int)(iResolution.y / scale))
#define NOT_IN_BOUNDS(coord) ((coord.x) < 0 || (int)(iResolution.x / scale) <= (coord.x) || (coord.y) < 0 || (int)(iResolution.y / scale) <= (coord.y))


__DEVICE__ Particle particle_at(int2 coord, float2 R, float scale, __TEXTURE2D__ iChannel0)
{
    
  
    if (NOT_IN_BOUNDS(coord))
    {
       Particle ret =  {WALL, false, 0.0f};
       return ret;
    }
  
    float4 data = texture(iChannel0, (make_float2(coord)+0.5f)/R);
    Particle ret =  {(int)(data.x), (bool)(data.y), data.z};
    return ret;
        
}

__DEVICE__ float4 to_vec4(Particle particle)
{
    
    return to_float4(
                      float(particle.type),
                      float(particle.has_moved_this_frame),
                      particle.shade,
                      0.0
                    );
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel2
// Connect Buffer A 'Previsualization: Buffer C' to iChannel0
// Connect Buffer A 'Previsualization: Buffer D' to iChannel1


__DEVICE__ float distance_to_line(float2 a, float2 b, float2 p)
{
    // from iq: https://www.youtube.com/watch?v=PMltMdi1Wzg
    float h = clamp(dot(p - a, b - a) / dot(b - a, b - a), 0.0f, 1.0f);
    return length(p - _mix(a, b, h));
}

//const float KEY_SPACE = 32.0f;
//const float KEY_LEFT_SHIFT = 16.0f;
//const float KEY_LEFT_CONTROL = 17.0f;


//__DEVICE__ bool key_pressed(float key_code)
//{
//    return texture(iChannel2, to_float2((key_code + 0.5f) / 256.0f, 0.5f / 3.0f)).r > 0.0f;
//}




__KERNEL__ void FallingSandManthraxJipiFuse__Buffer_A(float4 frag_color, float2 frag_coord, float2 iResolution, float4 iMouse, float iTime, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);
    CONNECT_BUTTON0(Modus, 0, Air, Sand, Wall, Water, BTN5);

    Modus = Modus == 0 ? 5 : Modus-1;
    
    CONNECT_SLIDER0(scale, 0.0f, 10.0f, 4.0f);
    CONNECT_SLIDER1(brush_radius, 0.0f, 100.0f, 40.0f);
    
    if ( iFrame < 0 || Reset)
    {
        SetFragmentShaderComputedColor(to_float4_s(0.0f));
        return;
    }

    frag_coord+=0.5f;

    // SETTINGS
    //const float scale        = 4.0f;
    //const float brush_radius = 40.0f;

    int2 coord = to_int2_cfloat(frag_coord);
    if (NOT_IN_BOUNDS(coord))
    {
        SetFragmentShaderComputedColor(frag_color);
        return;
    }

    INIT_SEED();
    
    float4 pMouse = texture(iChannel1, (make_float2(to_int2(0, 0))+0.5f)/R);
    
    if (pMouse.z > 0.0f)
    {
        if (iMouse.z > 0.0f && distance_to_line(swi2(pMouse,x,y) / scale, swi2(iMouse,x,y) / scale, make_float2(coord)) < brush_radius)
        {
            Particle param = { Modus, true, _mix(0.7f, 1.0f, rand(seed)) };
            frag_color = to_vec4(param);
            
            SetFragmentShaderComputedColor(frag_color);
            return;
        }
    }
    else
    {
        if (iMouse.z > 0.0f && distance_f2(make_float2(coord), swi2(iMouse,x,y) / scale) < brush_radius)
        {
            Particle param = { Modus, true, _mix(0.7f, 1.0f, rand(seed)) };
            frag_color = to_vec4(param);
            SetFragmentShaderComputedColor(frag_color);
            return;
        }
    }
    
    Particle self = particle_at(coord,R,scale,iChannel0);
    
    if(rand(seed)>0.1f)
    
    switch (self.type)
    {
        case WATER:
        {
            Particle below = particle_at(coord + to_int2(0, -1),R,scale,iChannel0);
            
            if (below.type == AIR)
            {
                Particle param = { AIR, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
        } break;
        
        case SAND:
        {
            Particle below = particle_at(coord + to_int2(0, -1),R,scale,iChannel0);
            
            if (below.type == AIR)
            {
                Particle param = { AIR, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            if (below.type == WATER)
            {
                Particle param = { WATER, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            
        } break;
        
        case AIR:
        {
            Particle above = particle_at(coord + to_int2(0, 1),R,scale,iChannel0);
            
            if (above.type == SAND)
            {
                above.has_moved_this_frame = true;
                frag_color = to_vec4(above);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            if (above.type == WATER)
            {
                above.has_moved_this_frame = true;
                frag_color = to_vec4(above);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
        } break;
    }
    
    Particle param = { self.type, false, self.shade };
    frag_color = to_vec4(param);


  SetFragmentShaderComputedColor(frag_color);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void FallingSandManthraxJipiFuse__Buffer_B(float4 frag_color, float2 frag_coord, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0)
{
    frag_coord+=0.5f;
    
    // SETTINGS
    const float scale        = 4.0f;
    const float brush_radius = 40.0f;
float BBBBBBBBBBBBBBBBBBB;    
    int2 coord = to_int2_cfloat(frag_coord);
    if (NOT_IN_BOUNDS(coord))
    {
        SetFragmentShaderComputedColor(frag_color);
        return;
    }
    
    Particle self = particle_at(coord,R,scale,iChannel0);
    
    if (self.has_moved_this_frame)
    {
        frag_color = to_vec4(self);
        SetFragmentShaderComputedColor(frag_color);
        return;
    }
    
    INIT_SEED();
    
    int dir = (int)(rand(seed) < 0.5f) * 2 - 1;
  
    //if(rand(seed)>0.5f)
    if(false)switch (self.type)
    {
        case WATER:
        {
            Particle below = particle_at(coord + to_int2(dir, -1),R,scale,iChannel0);
            
            if (!below.has_moved_this_frame && below.type == AIR)
            {
                Particle param = { AIR, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
        } break;
        
        case SAND:
        {
            Particle below = particle_at(coord + to_int2(dir, -1),R,scale,iChannel0);
            
            if (!below.has_moved_this_frame && below.type == AIR)
            {
                Particle param = { AIR, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            if (!below.has_moved_this_frame && below.type == WATER)
            {
                Particle param = { WATER, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
        } break;
        
        case AIR:
        {
            Particle above = particle_at(coord + to_int2(-dir, 1),R,scale,iChannel0);
            
            if (!above.has_moved_this_frame){
                if(above.type == SAND)
                {
                    above.has_moved_this_frame = true;
                    frag_color = to_vec4(above);
                    SetFragmentShaderComputedColor(frag_color);
                    return;
                }
                if(above.type == WATER)
                {
                    above.has_moved_this_frame = true;
                    frag_color = to_vec4(above);
                    SetFragmentShaderComputedColor(frag_color);
                    return;
                }
            }
        } break;
    }
    
    frag_color = to_vec4(self);


  SetFragmentShaderComputedColor(frag_color);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void FallingSandManthraxJipiFuse__Buffer_C(float4 frag_color, float2 frag_coord, float2 iResolution, float iTime, int iFrame, sampler2D iChannel0)
{
    frag_coord+=0.5f;
    
    // SETTINGS
    const float scale        = 4.0f;
    const float brush_radius = 40.0f;
    
    int2 coord = to_int2_cfloat(frag_coord);
    if (NOT_IN_BOUNDS(coord))
    {
      SetFragmentShaderComputedColor(frag_color);
      return;
    }
float CCCCCCCCCCCCCCCCCCCCCC;    
    Particle self = particle_at(coord,R,scale,iChannel0);
    
    if (self.has_moved_this_frame)
    {
      frag_color = to_vec4(self);
      SetFragmentShaderComputedColor(frag_color);
      return;
    }
    
    INIT_SEED();
    rand(seed);
    
    int dir = int(rand(seed) >= 0.5f) * 2 - 1;

    //if(rand(seed)>0.5f)
    switch (self.type)
    {
        case WATER:
        {
            Particle below = particle_at(coord + to_int2(dir, -1),R,scale,iChannel0);
               
            if (!below.has_moved_this_frame && below.type == AIR)
            {
                Particle param = { AIR, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            
            Particle side = particle_at(coord + to_int2(-dir, 0),R,scale,iChannel0);
            if (!side.has_moved_this_frame && side.type == AIR)
            {
                side.has_moved_this_frame = true;
                frag_color = to_vec4(side);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            
        } break;
        
        case SAND:
        {
            Particle below = particle_at(coord + to_int2(dir, -1),R,scale,iChannel0);
            
            if (!below.has_moved_this_frame && below.type == AIR)
            {
                Particle param = { AIR, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            
            if (!below.has_moved_this_frame && below.type == WATER)
            {
                Particle param = { WATER, true, 0.0f };
                frag_color = to_vec4(param);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
        } break;
        
        case AIR:
        {
            Particle above = particle_at(coord + to_int2(-dir, 1),R,scale,iChannel0);
            
            if (!above.has_moved_this_frame && above.type == SAND)
            {
                above.has_moved_this_frame = true;
                frag_color = to_vec4(above);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            
            Particle side = particle_at(coord + to_int2(-dir, 0),R,scale,iChannel0);
            if (!side.has_moved_this_frame && side.type == WATER)
            {
                side.has_moved_this_frame = true;
                frag_color = to_vec4(side);
                SetFragmentShaderComputedColor(frag_color);
                return;
            }
            
        } break;
    }
    
    frag_color = to_vec4(self);

  SetFragmentShaderComputedColor(frag_color);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------


//Save off last mouse cursor coordinate so it can form a line with the next cursor position

__KERNEL__ void FallingSandManthraxJipiFuse__Buffer_D(float4 frag_color, float2 frag_coord, float2 iResolution, float4 iMouse)
{
    frag_coord+=0.5f;
float DDDDDDDDDDDDDDDDD;    
    if (frag_coord.x == 0.5f && frag_coord.y == 0.5f)
    {
        frag_color = iMouse;
    }
    else
    {
        frag_color = to_float4_s(0.0f);
    }

  SetFragmentShaderComputedColor(frag_color);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


__DEVICE__ float rand_n(float r, int n)
{
    for (int i = 0; i < n; ++i)
    {
        r = fract(r * 100.0f);
    }
   
    return r;
}

__KERNEL__ void FallingSandManthraxJipiFuse(float4 frag_color, float2 frag_coord, float2 iResolution, sampler2D iChannel0)
{
    frag_coord+=0.5f;
  
    // SETTINGS
    const float scale        = 4.0f;
    const float brush_radius = 40.0f;
    
    int2 coord = to_int2_cfloat(frag_coord / scale);
    Particle self = particle_at(coord,R,scale,iChannel0);
    
    switch (self.type)
    {
        case SAND:
        {
            frag_color = to_float4(
                _mix(0.9f, 1.0f, rand_n(self.shade, 1)), 
                _mix(0.75f, 0.8f, rand_n(self.shade, 2)),
                _mix(0.5f, 0.6f, rand_n(self.shade, 3)), 1.0f) * _mix(0.7f, 1.0f, self.shade);
            SetFragmentShaderComputedColor(frag_color);
            return;
        } break;
        
        case WATER:
        {
            frag_color = to_float4(
                _mix(0.0f, 0.0f, rand_n(self.shade, 1)), 
                _mix(0.5f, 0.2f, rand_n(self.shade, 2)),
                _mix(0.9f, 0.8f, rand_n(self.shade, 3)), 1.0f) * _mix(0.7f, 1.0f, self.shade);
            SetFragmentShaderComputedColor(frag_color);
            return;
        } break;
        
        case WALL:
        {
            frag_color = to_float4(
                _mix(0.3f, 0.4f, rand_n(self.shade, 1)), 
                _mix(0.3f, 0.4f, rand_n(self.shade, 2)),
                _mix(0.3f, 0.4f, rand_n(self.shade, 3)), 1.0f) * _mix(0.7f, 1.0f, self.shade);
            SetFragmentShaderComputedColor(frag_color);
            return;
        } break;
        
        case AIR:
        {
            frag_color = to_float4(0.6f, 0.8f, 1.0f, 1.0f);
            SetFragmentShaderComputedColor(frag_color);
            return;
        }
    }

  SetFragmentShaderComputedColor(frag_color);
}