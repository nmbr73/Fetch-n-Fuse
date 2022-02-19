

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
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

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    vec2 uv = fragCoord / iResolution.xy;
    fragColor = texture(iChannel0, uv);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
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
#define SPECTRUM_SCALE 0.08

// The part that is used to detect loud bass, making the circle bigger
#define BASS_MAX 0.008
#define BASS_STEPS 8.0

// How loud a frequency has to be in order to be shown
#define MIN_VOLUME 0.8

// How loud the bass has to be in order to be shown
#define MIN_VOLUME_BASS 0.75

// How much to smooth out the spectrum
#define SMOOTH_RANGE 0.02
#define SMOOTH_STEPS 10.0

// The amount of particles
// There are actually 4 times as many, since it is mirrored twice
// This value has a huge impact on performance, so I recommend keeping it low
#define PARTICLES 50

// How far away to spawn particles
#define PARTICLE_SPAWN_Z 4.0

// Radius of the circle
#define CIRCLE_RADIUS 0.12

// How much impact the bass has on the particles
#define BASS_IMPACT_ON_PARTICLES 1.0

// The size of the white line of the circle
#define CIRCLE_BORDER_SIZE 0.008

// Colors
#define SPECTRUM_COLOR_1 vec4(1.0, 1.0, 1.0, 1.0)
#define SPECTRUM_COLOR_2 vec4(1.0, 1.0, 0.0, 0.95)
#define SPECTRUM_COLOR_3 vec4(1.0, 0.5, 0.0, 0.9)
#define SPECTRUM_COLOR_4 vec4(1.0, 0.0, 0.0, 0.85)
#define SPECTRUM_COLOR_5 vec4(1.0, 0.2, 0.3, 0.8)
#define SPECTRUM_COLOR_6 vec4(1.0, 0.0, 1.0, 0.75)
#define SPECTRUM_COLOR_7 vec4(0.0, 0.0, 1.0, 0.7)
#define SPECTRUM_COLOR_8 vec4(0.0, 0.8, 1.0, 0.65)
#define SPECTRUM_COLOR_9 vec4(0.0, 1.0, 0.0, 0.6)

#define PI 3.14159265359
#define TWO_PI 6.28318530718

// Famous HSV to RGB conversion function
// (I don't know who made this)
vec3 hsv2rgb(vec3 c) {
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

// Famous noise function, but without the '*2.0-1.0' part
// (I don't know who made the original)
float noise3D01(vec3 p) {
    return fract(sin(dot(p ,vec3(12.9898,78.233,128.852))) * 43758.5453);
}

// Convert uv coordinates to polar coordinates
vec2 uv_to_polar(vec2 uv, vec2 p) {
    vec2 translated_uv = uv - p;
    
    // Get polar coordinates
    vec2 polar = vec2(atan(translated_uv.x, translated_uv.y), length(translated_uv));
    
    // Scale to a range of 0 to 1
    polar.s /= TWO_PI;
    polar.s += 0.5;
    
    return polar;
}

// Circle, using polar coordinates
#define circle_polar(len, r) smooth_circle_polar(len, r, 0.004)
float smooth_circle_polar(float len, float r, float smoothness) {
    float dist = len - r;
    float s = smoothness / 2.0;
    return 1.0 - smoothstep(r - s, r + s, dist);
}

// Circle, using cartesian coordinates
#define circle(uv, p, radius) smooth_circle(uv-p, radius, 0.003)
float smooth_circle(vec2 p, float r, float smoothness) {
    float dist = length(p) - r;
    float s = smoothness / 2.0;
    return 1.0 - smoothstep(r - s, r + s, dist);
}

// Scale to values higher than another value
float cut_lower(float v, float low) {
    return clamp((v - low) * 1.0 / (1.0 - low), 0.0, 1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
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

#define get_fft(x) texture(iChannel0, vec2(x, 0.0)).r

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 fc = ivec2(floor(fragCoord));
    
    // Bass
    if (fc == ivec2(0, 0)) {
        float step_size = BASS_MAX / BASS_STEPS;
        
        float med = 0.0;
        
        for (float x = 0.0; x < BASS_MAX; x += step_size) {
            med += get_fft(x);
        }
        
        // Get medium value
        med /= BASS_STEPS;
        
        // Cut off low values 
        med = cut_lower(med, MIN_VOLUME_BASS);
        
        fragColor = vec4(med, 0.0, 0.0, 1.0);
        return;
    }
    
    // Value used for circle shake and background color cycle
    if (fc == ivec2(1, 0)) {
        float bass = texelFetch(iChannel1, ivec2(0, 0), 0).r;
        float old_value = texelFetch(iChannel1, fc, 0).r;
        
        // This value will get very high but buffers are Float32, so it doesn't really matter.
        fragColor = vec4(old_value + bass, 0.0, 0.0, 1.0);
        return;
    }
    
    // Scaled and smoothed out spectrum
    if (fc.y == 1) {
        vec2 uv = fragCoord / iResolution.xy;
        
        float step_size = SMOOTH_RANGE / SMOOTH_STEPS;
        
        float med = 0.0;
        
        for (float x = 0.0; x < SMOOTH_RANGE; x += step_size) {
            // Mirror if out of bounds
            float left = (uv.x - x > 0.0) ? uv.x - x : 0.0 - (uv.x - x);
            float right = (uv.x + x < 1.0) ? uv.x + x : 2.0 - (uv.x + x);
            
            med += cut_lower(get_fft(left * SPECTRUM_SCALE), MIN_VOLUME);
            med += cut_lower(get_fft(right * SPECTRUM_SCALE), MIN_VOLUME);
        }
        
        // Get medium value
        med /= SMOOTH_STEPS;
        med /= 2.0;
        
        fragColor = vec4(med, 0.0, 0.0, 1.0);
        return;
    }
    
    // Slide previous values one row down
    fragColor = texelFetch(iChannel1, fc + ivec2(0, -1), 0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
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

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    fragColor = vec4(0.0);
}

#else

// Move a particle
// uv is also passed, used as a parameter for noise3D
vec4 get_next_particle(vec4 old, vec2 uv) {
    float x = old.x;
    float y = old.y;
    float z = old.z;
    float speed = old.w;
    
    // Get bass value
    float bass = texelFetch(iChannel1, ivec2(0, 0), 0).r;
    
    z -= 0.02 * (speed + bass*BASS_IMPACT_ON_PARTICLES);
    
    // Out of screen, load new particle
    if (z <= 0.0) {
        // Generate random particle
        x = noise3D01(vec3(uv.x, uv.y, iTime)) * 2.0 + 0.1;
        y = noise3D01(vec3(uv.x, uv.y, iTime + 1.0)) * 1.5 + 0.1;
        z = PARTICLE_SPAWN_Z;
        speed = noise3D01(vec3(uv.x, uv.y, iTime + 2.0)) + 0.2;
    }
    
    return vec4(x, y, z, speed);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 fc = ivec2(floor(fragCoord));
    ivec2 rs = ivec2(iResolution.xy);
    
    // Particles can be stored in multiple rows
    int num = int(rs.x * fc.y + fc.x);
    if (num > PARTICLES) {
        return;
    }
    
    vec2 uv = fragCoord / iResolution.xy;
    
    vec4 old = texelFetch(iChannel0, fc, 0);
    
    // Output new particle
    fragColor = get_next_particle(old, uv);
}

#endif
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
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
vec3 background(vec2 uv, float extra) {
    const float saturation = 0.6, value = 0.7;
    
    // Cycle faster when there is a lot of bass
    float scaled = extra * 0.02;
    
    // Gradient of cycling hsv colors
    vec3 color = mix(hsv2rgb(vec3((iTime*0.25 + scaled) * 0.02, saturation, value)), hsv2rgb(vec3((iTime*0.15 - scaled) * 0.1, saturation, value)), uv.y + 0.1);
    
    return color;
}

// Rotate a coordinate
vec2 rotate(vec2 uv, float angle) {
    float s = sin(angle);
    float c = cos(angle);
    vec2 new = uv * mat2(c, s, -s, c);
    return new;
}

// Draw the inner circle
vec3 inner_circle(vec2 uvmtp) {
    // Rotate
    vec2 rotated = rotate(uvmtp, PI * 2.0 * (1.0 - smoothstep(0.0, 1.0, cut_lower(fract(iTime * 0.06), 0.8))));
    
    // Gradient
    vec3 color = mix(vec3(0.0), vec3(0.15), rotated.y * 0.5 + 0.5) + 0.1;
    
    // Lines
    color = mix(color, vec3(0.0), (sin(length(rotated) * 80.0))*0.05);
    
    // From: https://thndl.com/square-shaped-shaders.html
    // Draw the triangle
    float a = atan(rotated.x, rotated.y) + PI * 0.5;
    float b = TWO_PI / 3.0;
    color = mix(color, vec3(rotated.y * 0.5 + 0.8)*0.5 + ((sin(iTime * 2.0) + 1.0) * 0.5)*0.2, 1.0 - smoothstep(0.5, 0.52, cos(floor(0.5 + a/b) * b - a) * length(rotated)));
    
    return color;
}

// Get a particle
vec4 get_particle(int i) {
    int x = i;
    int y = 0;
    
    ivec2 rs = ivec2(iResolution.xy);
    
    // Get coordinate by particle id
    while (x > rs.x) {
        x -= rs.x;
        y++;
        if (y > rs.y) {
            break;
        }
    }
    
    return texelFetch(iChannel1, ivec2(x, y), 0);
}

// Draw particles
void particles(vec2 uvmtp, inout vec3 color) {
    for (int i = 0; i < PARTICLES; i++) {
        // Get particle
        vec4 particle = get_particle(i);
        
        vec2 projected;
        float size;
        
        for (int i = 0; i < 4; i++) {
            switch (i) {
            // Normal x, normal y
            case 0: projected = vec2(particle.x, particle.y) / particle.z; break;
            // Mirrored x, normal y
            case 1: projected = vec2(0.0 - particle.x, particle.y) / particle.z; break;
            // Normal x, mirrored y
            case 2: projected = vec2(particle.x, 0.0 - particle.y) / particle.z; break;
            // Mirrored x, mirrored y
            case 3: projected = vec2(0.0 - particle.x, 0.0 - particle.y) / particle.z; break;
            }
            
            size = (PARTICLE_SPAWN_Z - particle.z) / PARTICLE_SPAWN_Z;
            color = mix(color, color + vec3(size * 0.6), smooth_circle(uvmtp - projected, size * 0.007, 0.005 + 0.008 * (particle.z / PARTICLE_SPAWN_Z)));
        }
    }
}

// Calculate radius
float get_draw_radius(float x, float r, int fft_y) {
    // Get FFT value
    float fft = texelFetch(iChannel0, ivec2(x * iResolution.x, fft_y), 0).r;
    
    // Calculate radius
    float radius = CIRCLE_RADIUS + r + fft * 0.07;
    
    // Clamp to the circle radius
    radius = clamp(radius, CIRCLE_RADIUS + r, 1.0);
    
    return radius;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    ivec2 fc = ivec2(floor(fragCoord));
    vec2 uv = fragCoord / iResolution.xy;
    vec2 uvmtp = (fragCoord - 0.5 * iResolution.xy) / iResolution.y;
    
    // Debug buffer A
#if DEBUG_BUFFER_A
    if (uv.y < 0.8) {
        // Everything on the left side of the white line is considered bass
        if (uv.x > BASS_MAX/SPECTRUM_SCALE - 0.002 && uv.x < BASS_MAX/SPECTRUM_SCALE + 0.002) {
            fragColor = vec4(1.0);
            return;
        }
        
        float fft = texelFetch(iChannel0, ivec2(uv.x * iResolution.x, uv.y * iResolution.y), 0).r;
        
        fragColor = vec4(fft, 0.0, 0.0, 1.0);
        return;
    }
    
    // Bass value
    fragColor = texelFetch(iChannel0, ivec2(0, 0), 0);
    return;
#endif
    
    // Debug buffer B
#if DEBUG_BUFFER_B
    // It might be hard to see, but if you look in the bottom left corner, you'll see some colors
    fragColor = texelFetch(iChannel1, fc, 0);
    return;
#endif
    
    // Get bass value
    float bass = texelFetch(iChannel0, ivec2(0, 0), 0).r;
    
    // Shake the bubble
    vec2 shake = vec2(sin(iTime*9.0), cos(iTime*5.0)) * 0.002;
    uvmtp += shake;
    uv    += shake;
    
    // Get this value
    float extra = texelFetch(iChannel0, ivec2(1, 0), 0).r;
    
    // Draw the background
    vec3 color = background(uv, extra);
    
    // Draw particles
#if PARTICLES != 0
    particles(uvmtp, color);
#endif
    
    // Rotate the circle a bit
    uvmtp = rotate(uvmtp, sin(iTime * 1.5 + extra) * 0.005);
    
    // Shake the circle a bit
    vec2 circle_shake = vec2(cos(iTime*9.0 + extra*0.3), sin(iTime*9.0 + extra*0.3))*0.003;
    uvmtp += circle_shake;
    
    // Get polar coordinates for circle, shaking it a bit as well
    vec2 polar = uv_to_polar(uvmtp, vec2(0.0, 0.0));
    
    // Mirror
    float fft_x = polar.s;
    fft_x *= 2.0;
    if (fft_x > 1.0) {
        fft_x = 2.0 - fft_x;
    }
    
    // Invert (low frequencies on top)
    fft_x = 1.0 - fft_x;
    
    // How much the circle should grow
    float r = bass*0.03;
    
    // Draw spectrum
    float radius;
    color = mix(color, SPECTRUM_COLOR_9.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 9), 0.006) * SPECTRUM_COLOR_8.a);
    color = mix(color, SPECTRUM_COLOR_8.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 8), 0.00575) * SPECTRUM_COLOR_7.a);
    color = mix(color, SPECTRUM_COLOR_7.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 7), 0.0055) * SPECTRUM_COLOR_6.a);
    color = mix(color, SPECTRUM_COLOR_6.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 6), 0.00525) * SPECTRUM_COLOR_5.a);
    color = mix(color, SPECTRUM_COLOR_5.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 5), 0.005) * SPECTRUM_COLOR_4.a);
    color = mix(color, SPECTRUM_COLOR_4.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 4), 0.00475) * SPECTRUM_COLOR_3.a);
    color = mix(color, SPECTRUM_COLOR_3.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 3), 0.0045) * SPECTRUM_COLOR_2.a);
    color = mix(color, SPECTRUM_COLOR_2.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 2), 0.00425) * SPECTRUM_COLOR_2.a);
    color = mix(color, SPECTRUM_COLOR_1.rgb, smooth_circle_polar(polar.t, get_draw_radius(fft_x, r, 1), 0.004) * SPECTRUM_COLOR_1.a);
    
    // Draw inner circle
    color = mix(color, inner_circle(uvmtp / (CIRCLE_RADIUS + r - CIRCLE_BORDER_SIZE)), circle_polar(polar.t, CIRCLE_RADIUS + r - CIRCLE_BORDER_SIZE));
    
    // Lighten the screen when there is a lot of bass
    color += bass * 0.05;
    
    // Vignette
    color *= smoothstep(0.0, 1.0, 1.7 - length(uvmtp));
    
    // Output the final color, mixed with the previous color to create some sort of motion blur
    // (to make the movement look a little better)
    vec3 previous_color = texture(iChannel2, uv).rgb;
    fragColor = vec4(mix(previous_color, color, 0.8), 1.0);
}