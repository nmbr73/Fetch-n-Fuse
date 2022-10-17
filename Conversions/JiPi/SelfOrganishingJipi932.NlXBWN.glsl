

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv;
    vec2 loc_coor;
    uv = fragCoord * .1 + vec2(10, 10);
    loc_coor = fract(uv);
    uv = floor(uv) + vec2(.5, .5);
    uv /= iResolution.xy;
    vec2 uv_shift = vec2(1., 1.) / iResolution.xy;

    
    float lit = 0.;
    
    for (int y = -3; y <= 3; ++y) {
        for (int x = -3; x <= 3; ++x) {
            vec2 cell_uv = uv + uv_shift * vec2(x, y);
            vec2 val = texture(iChannel0, cell_uv).xy;
            val += vec2(x, y);
            lit += clamp(1. - length(val - loc_coor) * 5., 0., 1.);
        }
    }

    // Output to screen
    fragColor = vec4(lit, lit, lit, 1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 Hash22(vec2 p) {
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}


vec2 Hash32(vec3 p) {
	vec3 p3 = fract(vec3(p.xyz) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}


float eval_pixel(vec2 self, vec2 neighbours[8])
{
    vec2 offsets[8] = vec2[8](
        vec2(-1., -1.),
        vec2( 0., -1.),
        vec2( 1., -1.),
        vec2(-1.,  0.),
        vec2( 1.,  0.),
        vec2(-1.,  1.),
        vec2( 0.,  1.),
        vec2( 1.,  1.)
    );
    float distances[8];
    
    float error = 0.;
    
    float mean = 0.;
    for (int i = 0; i < 8; ++i) {
        mean += (distances[i] = length(neighbours[i] + offsets[i] - self));
    }
    
    mean /= 8.;
    float var = 0.;
    
    for (int i = 0; i < 8; ++i) {
        var += pow(distances[i] - mean, 2.);
    }
    var /= 8.;
    
    float off_centre = length(self - vec2(0.5, 0.5));
    
    return pow(1. / (1e-5 + mean), 2.) + pow(abs(var - .1) * 64., 3.) + pow(off_centre * 1., 6.) * .05;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 mouse_offset = vec2(iMouse.xy * .1 - fragCoord + vec2(10., 10.));
    
    vec2 uv_self = fragCoord / iResolution.xy;
    vec2 uv_shift = vec2(1., 1.) / iResolution.xy;

    vec2 val_self = texture(iChannel0, uv_self).xy;
    vec2 val_mom  = texture(iChannel0, uv_self).zw;
    
    vec2 neighbours[8] = vec2[8](
         texture(iChannel0, uv_self + vec2(-uv_shift.x, -uv_shift.y)).xy,
         texture(iChannel0, uv_self + vec2(         0., -uv_shift.y)).xy,
         texture(iChannel0, uv_self + vec2( uv_shift.x, -uv_shift.y)).xy,
         texture(iChannel0, uv_self + vec2(-uv_shift.x,          0.)).xy,
         texture(iChannel0, uv_self + vec2( uv_shift.x,          0.)).xy,
         texture(iChannel0, uv_self + vec2(-uv_shift.x,  uv_shift.y)).xy,
         texture(iChannel0, uv_self + vec2(         0.,  uv_shift.y)).xy,
         texture(iChannel0, uv_self + vec2( uv_shift.x,  uv_shift.y)).xy
    );
    
    vec2 offset;
    float epsilon = 1e-3;
    offset.x = (eval_pixel(val_self + vec2(epsilon, 0.), neighbours) - eval_pixel(val_self + vec2(-epsilon, 0.), neighbours));
    offset.y = (eval_pixel(val_self + vec2(0., epsilon), neighbours) - eval_pixel(val_self + vec2(0., -epsilon), neighbours));
    
    vec2 val_next = val_self + val_mom * 1e-1;
    
    if (iMouse.z > 0.)
        val_next += normalize(mouse_offset) / (1e-3 + length(mouse_offset) * 6.);
    
    if (length(offset) > 1e-3) {
        offset = normalize(offset);
        val_next -= offset * 5e-2 * max(1., 1.5 - iTime * 0.1);
    }
    
    val_next = clamp(val_next, vec2(0., 0.), vec2(1., 1.));
    val_next += (Hash32(vec3(fragCoord * 10., iTime)) - vec2(0.5, 0.5)) * 1e-3;
        
    fragColor.xy = val_next;
    fragColor.zw = val_next - val_self + val_mom * 0.75;
}