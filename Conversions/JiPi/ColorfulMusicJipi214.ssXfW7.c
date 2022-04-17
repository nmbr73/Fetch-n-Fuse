
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


/*
  Free Public License 1.0.0

  Copyright (C) 2018 by tikveel <steven@tikveel.nl>

  Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
  USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// Don't show the full spectrum, since we only care about the bass
#define SPECTRUM_SCALE 0.08f

// The part that is used to detect loud bass, making the circle bigger
#define BASS_MAX 0.008f
#define BASS_STEPS 8.0f

// How loud a frequency has to be in order to be shown
#define MIN_VOLUME 0.8f

// How loud the bass has to be in order to be shown
#define MIN_VOLUME_BASS 0.75f

// How much to smooth out the spectrum
#define SMOOTH_RANGE 0.02f
#define SMOOTH_STEPS 10.0f

// The amount of particles
// There are actually 4 times as many, since it is mirrored twice
// This value has a huge impact on performance, so I recommend keeping it low
#define PARTICLES 50

// How far away to spawn particles
#define PARTICLE_SPAWN_Z 4.0f

// Radius of the circle
#define CIRCLE_RADIUS 0.12f

// How much impact the bass has on the particles
#define BASS_IMPACT_ON_PARTICLES 1.0f

// The size of the white line of the circle
#define CIRCLE_BORDER_SIZE 0.008f

// Colors
#define SPECTRUM_COLOR_1 to_float4(1.0f, 1.0f, 1.0f, 1.0f)
#define SPECTRUM_COLOR_2 to_float4(1.0f, 1.0f, 0.0f, 0.95f)
#define SPECTRUM_COLOR_3 to_float4(1.0f, 0.5f, 0.0f, 0.9f)
#define SPECTRUM_COLOR_4 to_float4(1.0f, 0.0f, 0.0f, 0.85f)
#define SPECTRUM_COLOR_5 to_float4(1.0f, 0.2f, 0.3f, 0.8f)
#define SPECTRUM_COLOR_6 to_float4(1.0f, 0.0f, 1.0f, 0.75f)
#define SPECTRUM_COLOR_7 to_float4(0.0f, 0.0f, 1.0f, 0.7f)
#define SPECTRUM_COLOR_8 to_float4(0.0f, 0.8f, 1.0f, 0.65f)
#define SPECTRUM_COLOR_9 to_float4(0.0f, 1.0f, 0.0f, 0.6f)

#define PI 3.14159265359f
#define TWO_PI 6.28318530718f

// Famous HSV to RGB conversion function
// (I don't know who made this)
__DEVICE__ float3 hsv2rgb(float3 c) {
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
    return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}

// Famous noise function, but without the '*2.0f-1.0' part
// (I don't know who made the original)
__DEVICE__ float noise3D01(float3 p) {
    return fract(_sinf(dot(p ,to_float3(12.9898f,78.233f,128.852f))) * 43758.5453f);
}

// Convert uv coordinates to polar coordinates
__DEVICE__ float2 uv_to_polar(float2 uv, float2 p) {
    float2 translated_uv = uv - p;
    
    // Get polar coordinates
    float2 polar = to_float2(_atan2f(translated_uv.x, translated_uv.y), length(translated_uv));
    
    // Scale to a range of 0 to 1
    polar.s /= TWO_PI;
    polar.s += 0.5f;
    
    return polar;
}

// Circle, using polar coordinates
#define circle_polar(len, r) smooth_circle_polar(len, r, 0.004f)
__DEVICE__ float smooth_circle_polar(float len, float r, float smoothness) {
    float dist = len - r;
    float s = smoothness / 2.0f;
    return 1.0f - smoothstep(r - s, r + s, dist);
}

// Circle, using cartesian coordinates
#define circle(uv, p, radius) smooth_circle(uv-p, radius, 0.003f)
__DEVICE__ float smooth_circle(float2 p, float r, float smoothness) {
    float dist = length(p) - r;
    float s = smoothness / 2.0f;
    return 1.0f - smoothstep(r - s, r + s, dist);
}

// Scale to values higher than another value
__DEVICE__ float cut_lower(float v, float low) {
    return clamp((v - low) * 1.0f / (1.0f - low), 0.0f, 1.0f);
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel1
// Connect Buffer A 'https://soundcloud.com/bossfightswe/elevatia' to iChannel0


/*
  Free Public License 1.0.0

  Copyright (C) 2018 by tikveel <steven@tikveel.nl>

  Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
  USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// This buffer contains audio data
// First col of first row: bass value
// Second col of first row: value used for camera shake and background color cycle
// Second row: scaled and smoothed out spectrum
// All other rows: spectrums of previous frames

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution



#define get_fft(x) texture(iChannel0, to_float2(x, 0.0f)).x

__KERNEL__ void ColorfulMusicJipi214Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    int2 fc = to_int2_cfloat(_floor(fragCoord));
    
    // Bass
    if (fc == to_int2(0, 0)) {
        float step_size = BASS_MAX / BASS_STEPS;
        
        float med = 0.0f;
        
        for (float _x = 0.0f; _x < BASS_MAX; _x += step_size) {
            med += get_fft(_x);
        }
        
        // Get medium value
        med /= BASS_STEPS;
        
        // Cut off low values 
        med = cut_lower(med, MIN_VOLUME_BASS);
        
        fragColor = to_float4(med, 0.0f, 0.0f, 1.0f);
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    // Value used for circle shake and background color cycle
    if (fc.x == 1 &&fc.y ==  0) {
        //float bass = texelFetch(iChannel1, to_int2(0, 0), 0).x;
        float bass = texture(iChannel1, (make_float2(to_int2(0, 0))+0.5)/R).x;
        //float old_value = texelFetch(iChannel1, fc, 0).r;
        float old_value = texture(iChannel1, (make_float2(fc)+0.5f)/R).x;
        
        // This value will get very high but buffers are Float32, so it doesn't really matter.
        fragColor = to_float4(old_value + bass, 0.0f, 0.0f, 1.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    // Scaled and smoothed out spectrum
    if (fc.y == 1) {
        float2 uv = fragCoord / iResolution;
        
        float step_size = SMOOTH_RANGE / SMOOTH_STEPS;
        
        float med = 0.0f;
        
        for (float x = 0.0f; x < SMOOTH_RANGE; x += step_size) {
            // Mirror if out of bounds
            float left = (uv.x - x > 0.0f) ? uv.x - x : 0.0f - (uv.x - x);
            float right = (uv.x + x < 1.0f) ? uv.x + x : 2.0f - (uv.x + x);
            
            med += cut_lower(get_fft(left * SPECTRUM_SCALE), MIN_VOLUME);
            med += cut_lower(get_fft(right * SPECTRUM_SCALE), MIN_VOLUME);
        }
        
        // Get medium value
        med /= SMOOTH_STEPS;
        med /= 2.0f;
        
        fragColor = to_float4(med, 0.0f, 0.0f, 1.0f);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    
    // Slide previous values one row down
    //fragColor = texelFetch(iChannel1, fc + to_int2(0, -1), 0);
    fragColor = texelFetch(iChannel1, (make_float2(fc + to_int2(0, -1))+0.5f)/R);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


/*
  Free Public License 1.0.0

  Copyright (C) 2018 by tikveel <steven@tikveel.nl>

  Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
  USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// A simple starfield

// This buffer contains particle data, spread over multiple rows
// Color layout:
//  x = x coordinate
//  y = y coordinate
//  z = z coordinate
//  w = speed

#if PARTICLES == 0

__KERNEL__ void ColorfulMusicJipi214Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    fragColor = to_float4_s(0.0f);
}

#else

// Move a particle
// uv is also passed, used as a parameter for noise3D
__DEVICE__ float4 get_next_particle(float4 old, float2 uv, float iTime, float2 R, __TEXTURE2D__ iChannel0) {
    float x = old.x;
    float y = old.y;
    float z = old.z;
    float speed = old.w;
    
    // Get bass value
    //float bass = texelFetch(iChannel1, to_int2(0, 0), 0).r;
    float bass = texture(iChannel1, (make_float2(to_int2(0, 0))+0.5f/R).x;
    
    z -= 0.02f * (speed + bass*BASS_IMPACT_ON_PARTICLES);
    
    // Out of screen, load new particle
    if (z <= 0.0f) {
        // Generate random particle
        x = noise3D01(to_float3(uv.x, uv.y, iTime)) * 2.0f + 0.1f;
        y = noise3D01(to_float3(uv.x, uv.y, iTime + 1.0f)) * 1.5f + 0.1f;
        z = PARTICLE_SPAWN_Z;
        speed = noise3D01(to_float3(uv.x, uv.y, iTime + 2.0f)) + 0.2f;
    }
    
    return to_float4(x, y, z, speed);
}

__KERNEL__ void ColorfulMusicJipi214Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{

    int2 fc = to_int2(_floor(fragCoord));
    int2 rs = to_int2(iResolution);
    
    // Particles can be stored in multiple rows
    int num = int(rs.x * fc.y + fc.x);
    if (num > PARTICLES) {
        return;
    }
    
    float2 uv = fragCoord / iResolution;
    
    //float4 old = texelFetch(iChannel0, fc, 0);
    float4 old = texture(iChannel0, (make_float2(fc)+0.5f)/R);
    
    // Output new particle
    fragColor = get_next_particle(old, uv, iTime, R, iChannel0);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


/*
  Free Public License 1.0.0

  Copyright (C) 2018 by tikveel <steven@tikveel.nl>

  Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
  USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// To debug a buffer, set the value to 1
#define DEBUG_BUFFER_A 0
#define DEBUG_BUFFER_B 0

// Draw the background
__DEVICE__ float3 background(float2 uv, float extra, float iTime) {
    const float saturation = 0.6f, value = 0.7f;
    
    // Cycle faster when there is a lot of bass
    float scaled = extra * 0.02f;
    
    // Gradient of cycling hsv colors
    float3 color = _mix(hsv2rgb(to_float3((iTime*0.25f + scaled) * 0.02f, saturation, value)), hsv2rgb(to_float3((iTime*0.15f - scaled) * 0.1f, saturation, value)), uv.y + 0.1f);
    
    return color;
}

// Rotate a coordinate
__DEVICE__ float2 rotate(float2 uv, float angle) {
    float s = _sinf(angle);
    float c = _cosf(angle);
    float2 new = uv * mat2(c, s, -s, c);
    return new;
}

// Draw the inner circle
__DEVICE__ float3 inner_circle(float2 uvmtp) {
    // Rotate
    float2 rotated = rotate(uvmtp, PI * 2.0f * (1.0f - smoothstep(0.0f, 1.0f, cut_lower(fract(iTime * 0.06f), 0.8f))));
    
    // Gradient
    float3 color = _mix(to_float3_s(0.0f), to_float3_s(0.15f), rotated.y * 0.5f + 0.5f) + 0.1f;
    
    // Lines
    color = _mix(color, to_float3_s(0.0f), (_sinf(length(rotated) * 80.0f))*0.05f);
    
    // From: https://thndl.com/square-shaped-shaders.html
    // Draw the triangle
    float a = _atan2f(rotated.x, rotated.y) + PI * 0.5f;
    float b = TWO_PI / 3.0f;
    color = _mix(color, to_float3(rotated.y * 0.5f + 0.8f)*0.5f + ((_sinf(iTime * 2.0f) + 1.0f) * 0.5f)*0.2f, 1.0f - smoothstep(0.5f, 0.52f, _cosf(_floor(0.5f + a/b) * b - a) * length(rotated)));
    
    return color;
}

// Get a particle
__DEVICE__ float4 get_particle(int i, float2 R) {
    int x = i;
    int y = 0;
    
    int2 rs = to_int2_cfloat(iResolution);
    
    // Get coordinate by particle id
    while (x > rs.x) {
        x -= rs.x;
        y++;
        if (y > rs.y) {
            break;
        }
    }
    
    return texelFetch(iChannel1, to_int2(x, y), 0);
}

// Draw particles
__DEVICE__ void particles(float2 uvmtp, inout float3 color, float2 R) {
    for (int i = 0; i < PARTICLES; i++) {
        // Get particle
        float4 particle = get_particle(i,R);
        
        float2 projected;
        float size;
        
        for (int i = 0; i < 4; i++) {
            switch (i) {
            // Normal x, normal y
            case 0: projected = to_float2(particle.x, particle.y) / particle.z; break;
            // Mirrored x, normal y
            case 1: projected = to_float2(0.0f - particle.x, particle.y) / particle.z; break;
            // Normal x, mirrored y
            case 2: projected = to_float2(particle.x, 0.0f - particle.y) / particle.z; break;
            // Mirrored x, mirrored y
            case 3: projected = to_float2(0.0f - particle.x, 0.0f - particle.y) / particle.z; break;
            }
            
            size = (PARTICLE_SPAWN_Z - particle.z) / PARTICLE_SPAWN_Z;
            color = _mix(color, color + to_float3(size * 0.6f), smooth_circle(uvmtp - projected, size * 0.007f, 0.005f + 0.008f * (particle.z / PARTICLE_SPAWN_Z)));
        }
    }
}

// Calculate radius
__DEVICE__ float get_draw_radius(float _x, float r, int fft_y, float2 R) {
    // Get FFT value
    //float fft = texelFetch(iChannel0, to_int2(x * iResolution.x, fft_y), 0).r;
    float fft = texture(iChannel0, (make_float2(to_int2(_x * iResolution.x, fft_y))+0.5f)/R).x;
    
    // Calculate radius
    float radius = CIRCLE_RADIUS + r + fft * 0.07f;
    
    // Clamp to the circle radius
    radius = clamp(radius, CIRCLE_RADIUS + r, 1.0f);
    
    return radius;
}

__KERNEL__ void ColorfulMusicJipi214Fuse__Buffer_C(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    int2 fc = to_int2_cfloat(_floor(fragCoord));
    float2 uv = fragCoord / iResolution;
    float2 uvmtp = (fragCoord - 0.5f * iResolution) / iResolution.y;
    
    // Debug buffer A
#if DEBUG_BUFFER_A
    if (uv.y < 0.8f) {
        // Everything on the left side of the white line is considered bass
        if (uv.x > BASS_MAX/SPECTRUM_SCALE - 0.002f && uv.x < BASS_MAX/SPECTRUM_SCALE + 0.002f) {
            fragColor = to_float4_s(1.0f);
            return;
        }
        
        //float fft = texelFetch(iChannel0, to_int2(uv.x * iResolution.x, uv.y * iResolution.y), 0).r;
        float fft = texture(iChannel0, (make_float2(to_int2(uv.x * iResolution.x, uv.y * iResolution.y))+0.5f)/R).x;
        
        fragColor = to_float4(fft, 0.0f, 0.0f, 1.0f);
        return;
    }
    
    // Bass value
    //fragColor = texelFetch(iChannel0, to_int2(0, 0), 0);
    fragColor = texture(iChannel0, (make_float2(to_int2(0, 0))+0.5)/R);
    return;
#endif
    
    // Debug buffer B
#if DEBUG_BUFFER_B
    // It might be hard to see, but if you look in the bottom left corner, you'll see some colors
    //fragColor = texelFetch(iChannel1, fc, 0);
    //fragColor = texture(iChannel1, (make_float2(fc)+0.5)/R).x;
    fragColor = texture(iChannel1, (make_float2(fc)+0.5)/R).x;
    return;
#endif
    
    // Get bass value
    //float bass = texelFetch(iChannel0, to_int2(0, 0), 0).r;
    float bass = texture(iChannel0, (make_float2(to_int2(0, 0))+0.5)/R).x;
    
    // Shake the bubble
    float2 shake = to_float2(_sinf(iTime*9.0f), _cosf(iTime*5.0f)) * 0.002f;
    uvmtp += shake;
    uv    += shake;
    
    // Get this value
    //float extra = texelFetch(iChannel0, to_int2(1, 0), 0).r;
    float extra = texture(iChannel0, (make_float2(to_int2(1, 0))+0.5)/R).x;
    
    // Draw the background
    float3 color = background(uv, extra, iTime);
    
    // Draw particles
#if PARTICLES != 0
    particles(uvmtp, color,R);
#endif
    
    // Rotate the circle a bit
    uvmtp = rotate(uvmtp, _sinf(iTime * 1.5f + extra) * 0.005f);
    
    // Shake the circle a bit
    float2 circle_shake = to_float2(_cosf(iTime*9.0f + extra*0.3f), _sinf(iTime*9.0f + extra*0.3f))*0.003f;
    uvmtp += circle_shake;
    
    // Get polar coordinates for circle, shaking it a bit as well
    float2 polar = uv_to_polar(uvmtp, to_float2(0.0f, 0.0f));
    
    // Mirror
    float fft_x = polar.s;
    fft_x *= 2.0f;
    if (fft_x > 1.0f) {
        fft_x = 2.0f - fft_x;
    }
    
    // Invert (low frequencies on top)
    fft_x = 1.0f - fft_x;
    
    // How much the circle should grow
    float r = bass*0.03f;
    
    // Draw spectrum
    float radius;
    color = _mix(color, swi3(SPECTRUM_COLOR_9,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 9,R), 0.006f) * SPECTRUM_COLOR_8.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_8,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 8,R), 0.00575f) * SPECTRUM_COLOR_7.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_7,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 7,R), 0.0055f) * SPECTRUM_COLOR_6.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_6,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 6,R), 0.00525f) * SPECTRUM_COLOR_5.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_5,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 5,R), 0.005f) * SPECTRUM_COLOR_4.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_4,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 4,R), 0.00475f) * SPECTRUM_COLOR_3.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_3,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 3,R), 0.0045f) * SPECTRUM_COLOR_2.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_2,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 2,R), 0.00425f) * SPECTRUM_COLOR_2.w);
    color = _mix(color, swi3(SPECTRUM_COLOR_1,x,y,z), smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 1,R), 0.004f) * SPECTRUM_COLOR_1.w);
    
    // Draw inner circle
    color = _mix(color, inner_circle(uvmtp / (CIRCLE_RADIUS + r - CIRCLE_BORDER_SIZE)), circle_polar(polar.t, CIRCLE_RADIUS + r - CIRCLE_BORDER_SIZE));
    
    // Lighten the screen when there is a lot of bass
    color += bass * 0.05f;
    
    // Vignette
    color *= smoothstep(0.0f, 1.0f, 1.7f - length(uvmtp));
    
    // Output the final color, mixed with the previous color to create some sort of motion blur
    // (to make the movement look a little better)
    float3 previous_color = swi3(_tex2DVecN(iChannel2,uv.x,uv.y,15)x,y,z);
    fragColor = to_float4(_mix(previous_color, color, 0.8f), 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer C' to iChannel0


/*
  Free Public License 1.0.0

  Copyright (C) 2018 by tikveel <steven@tikveel.nl>

  Permission to use, copy, modify, and/or distribute this software for any purpose with or without fee is hereby granted.

  THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING
  ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY SPECIAL,
  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
  WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE
  USE OR PERFORMANCE OF THIS SOFTWARE.
*/

// An audio visualizer inspired by the one used by Trap Nation (https://www.youtube.com/user/AllTrapNation)
// If you have a fast GPU, I recommend changing the amount of particles in the common pass
// This song is Bossfight - Elevatia (https://soundcloud.com/bossfightswe/elevatia)
// Other songs you should try (change in buffer A):
//  Yeah Yeah Yeahs - Heads Will Roll (Jaydon Lewis Remix) (https://soundcloud.com/itsjaydonlewis/headswillroll)
//  Pendulum - Tarantula (https://soundcloud.com/elijahpaul/tarantula-pendulum)
//  Dr. Dre - The Next Episode (San Holo Remix) (https://soundcloud.com/electricspark/dr-dre-the-next-episode-san-holo-remix)
//  Imagine Dragons - Believer (Kid Comet Nebula Remix) (https://soundcloud.com/kidcometmusic/imagine-dragons-believer-kid-comet-nebula-remix)
//  Axel Thesleff - Bad Karma (https://soundcloud.com/axelthesleff/bad-karma)
//  Flowrian - Banani Code (https://soundcloud.com/quantumsaturnus/flowrian-banani-code)

__KERNEL__ void ColorfulMusicJipi214Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 uv = fragCoord / iResolution;
    fragColor = _tex2DVecN(iChannel0,uv.x,uv.y,15);


  SetFragmentShaderComputedColor(fragColor);
}