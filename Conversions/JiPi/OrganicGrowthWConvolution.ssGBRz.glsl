

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// This just paints whatever is in BufferA
void mainImage(out vec4 color, in vec2 coord) {
    vec2 uv = coord / iResolution.xy;
    color = texture(iChannel0, uv);    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// blur convolution
/*
const int r = 1;
const int l = (2*r+1)*(2*r+1);
float m[l] = float[](
    0., 1., 0.,
    1., 0., 1.,
    0., 1., 0.
);
float mt = 4.01;
*/

// organic growth convolution
const int r = 1;
const int l = (2*r+1)*(2*r+1);
float m[l] = float[](
    -1., 5., -1.,
    5., 0., 5.,
    -1., 5., -1.
);
float mt = 16.1;

void mainImage(out vec4 color, in vec2 coord) {
    color.rgb = vec3(0.);

    vec2 uv = coord / iResolution.xy;
    vec2 mouseUv = iMouse.xy / iResolution.xy;
    
    for (int x = -1; x <=1; x++) {
        for (int y = -1; y <=1; y++) {
            int i = (y+1)*3+x+1;
            float v = m[i] / mt;
            color += 0.9 * v * texture(
                iChannel0,
                vec2(
                    uv.x+float(x)/iResolution.x,
                    uv.y+float(y)/iResolution.y
                )
            );
        }
    }        
    
    if (distance(uv, mouseUv) < 0.05 && iMouse.z > 0.) {
        color.rgb = 1. - color.rgb;
    }
    
    color.rgb = clamp(color.rgb, 0., 1.);
    
}