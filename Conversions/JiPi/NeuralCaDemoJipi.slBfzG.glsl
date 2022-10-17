

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    // Apply zoom (can't figure out how to re-size buffers 
    // so this wastes a lot of compute updating the offscreen parts)
    uv = uv/2.;

    // Read the buffer
    vec3 col = (texture(iChannel0, uv).xyz-vec3(0.5))*10. + vec3(0.5);

    // Output to screen
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Paste your weights hre:
const int nh = 8;
float b1[8] = float[8](-0.005607239902019501,0.20737116038799286,-0.15958012640476227,0.1839800626039505,0.10558734834194183,0.2413249909877777,0.22694732248783112,0.1544546037912368);
float w1[128] = float[128](-0.17097297310829163,0.06744687259197235,0.21897636353969574,0.10879534482955933,-0.14843542873859406,0.20935866236686707,-0.1830487698316574,-0.1983712762594223,0.19102470576763153,0.1465626209974289,-0.1298714131116867,0.12886269390583038,-0.10719338804483414,0.1895245760679245,0.15244363248348236,-0.12479807436466217,0.19730904698371887,0.2103326916694641,0.07020334899425507,0.06035710498690605,0.12206670641899109,-0.1604532152414322,-0.11493607610464096,0.21523287892341614,0.2743070721626282,0.011186826042830944,0.1369483321905136,0.12041302770376205,-0.08342280238866806,0.22404158115386963,0.030259817838668823,-0.1686122715473175,-0.06810998171567917,0.029785193502902985,0.05608399957418442,0.25010204315185547,-0.07401710748672485,-0.08251817524433136,-0.053251057863235474,0.043036624789237976,0.14328479766845703,-0.12106183916330338,-0.15435048937797546,0.09810590744018555,0.054376404732465744,-0.11873634159564972,-0.0808294340968132,0.0697202980518341,0.2648638188838959,-0.06522378325462341,0.060611315071582794,0.22495940327644348,-0.010338977910578251,0.17524294555187225,0.17996922135353088,0.16922366619110107,0.218682199716568,0.02084919810295105,-0.1255321353673935,0.12736909091472626,-0.025168975815176964,0.1348275989294052,-0.08117184787988663,0.12416274845600128,-0.1708059310913086,-0.11634736508131027,0.20006434619426727,0.03734216466546059,-0.10498269647359848,0.12329734861850739,0.16503401100635529,0.08998304605484009,-0.260944128036499,-0.10085125267505646,-0.021763155236840248,-0.15479378402233124,0.011685313656926155,-0.2371322512626648,0.07034227252006531,0.026207149028778076,0.15099263191223145,-0.0033387993462383747,-0.031722985208034515,0.21575583517551422,0.09879902750253677,0.09183311462402344,0.1828172504901886,-0.2255178540945053,-0.012155634351074696,0.1523420810699463,-0.05294913053512573,-0.08934169262647629,0.22459764778614044,-0.20958268642425537,-0.13207976520061493,-0.0333939790725708,-0.08564555644989014,-0.03224276378750801,0.07250171154737473,-0.250521183013916,-0.21712614595890045,0.15493904054164886,0.06420993059873581,0.06199895218014717,-0.2068699151277542,-0.019539864733815193,-0.009239627048373222,0.050233643501996994,0.022854316979646683,-0.2485826462507248,0.19061429798603058,-0.17818520963191986,0.03746657446026802,0.1707015335559845,0.11888623237609863,-0.004525625612586737,-0.002364585641771555,0.0060291169211268425,-0.2719455659389496,-0.0008139413548633456,-0.17593756318092346,0.0024618881288915873,-0.10072672367095947,-0.1509338617324829,-0.21447797119617462,0.11333387345075607,0.18015369772911072,0.158259317278862);
float w2[32] = float[32](0.0004315134137868881,-0.011854984797537327,0.03449397534132004,-0.007808920461684465,0.009794715791940689,0.0007741426234133542,-0.005454638507217169,0.001134452992118895,-0.006665459368377924,-0.010381652973592281,0.030350729823112488,-0.0007826134096831083,0.002680370118469,-0.004545061849057674,0.0008326310198754072,0.0009512250544503331,-0.012409443967044353,-0.009911532513797283,0.02520965039730072,0.004211696796119213,-0.003577346447855234,-0.00684503186494112,0.003175704274326563,0.002208447316661477,-0.007172347512096167,-0.001526405569165945,-0.001191230840049684,0.0031038434244692326,-0.006310776807367802,-0.0006254760664887726,-0.0073636253364384174,0.021423500031232834);



// Random Number Generator
// From https://www.shadertoy.com/view/MsKGWz:
// See Stack Overflow: http://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader/10625698#10625698
float random_1( vec2 p )
{
    vec2 r = vec2(
        23.14069263277926, // e^pi (Gelfond's constant)
         2.665144142690225 // 2^sqrt(2) (Gelfondâ€“Schneider constant)
    );
    return fract( cos( mod( 12345678., 256. * dot(p,r) ) ) );
}

// Samples the neighbourhood (wrapping around where needed)
vec2 coord (vec2 fragCoord, vec2 offset){
    float x = mod(fragCoord.x + offset.x, iResolution.x);
    float y = mod(fragCoord.y + offset.y, iResolution.y);
    return vec2(x, y)/iResolution.xy;
}
vec4[9] sample_tex (vec2 fragCoord){
    vec4 tex[9] = vec4[9](
        (texture(iChannel0, coord(fragCoord, vec2(-1, 1)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(0, 1)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(1, 1)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(-1, 0)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(0, 0)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(1, 0)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(-1, -1)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(0, -1)))-0.5)*10.,
        (texture(iChannel0, coord(fragCoord, vec2(1, -1)))-0.5)*10.
    );
    return tex;
}

// The four kernels used
vec4 ident(vec2 fragCoord, vec4[9] tex){
    return tex[4]; // no offset
}
vec4 sobel_x(vec2 fragCoord, vec4[9] tex){
    vec4 result = -1.*tex[0]-2.*tex[3]-1.*tex[6]+1.*tex[2]+2.*tex[5]+1.*tex[8];
    return result;
}
vec4 sobel_y(vec2 fragCoord, vec4[9] tex){
    vec4 result = -1.*tex[0]-2.*tex[1]-1.*tex[2]+1.*tex[6]+2.*tex[7]+1.*tex[8];
    return result;
}
vec4 lap(vec2 fragCoord, vec4[9] tex){
    vec4 result = 1.*tex[0]+2.*tex[1]+1.*tex[2]+2.*tex[3]-12.*tex[4]+2.*tex[5]+1.*tex[6]+2.*tex[7]+1.*tex[8]; // was an errant +2.
    return result;
}

// Our activation function
float relu(float x){
    if (x > 0.){return x;}
    return 0.;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    
    // Sample BufC for kernels
    vec4 tex[9] = sample_tex(fragCoord);
    
    // Apply filters
    vec4 id = ident(fragCoord, tex);
    vec4 sx = sobel_x(fragCoord, tex);
    vec4 sy = sobel_y(fragCoord, tex);
    vec4 ll = lap(fragCoord, tex);
    
    // Create x (4 channels x 4 filters, per channel conv)
    float x[16];
    x[0] = id.x;x[1] = sx.x;x[2] = sy.x;x[3] = ll.x;
    x[4] = id.y;x[5] = sx.y;x[6] = sy.y;x[7] = ll.y;
    x[8] = id.z;x[9] = sx.z;x[10] = sy.z;x[11] = ll.z;
    x[12] = id.w;x[13] = sx.w;x[14] = sy.w;x[15] = ll.w;
    
    
    // First layer 
    float l1_out[nh];
    for (int i = 0; i < nh; i++){
        // Dot Product equivalent to:
        // dot_product = x @ w1_i
        float dot_product = 0.;
        for (int j = 0; j < 16; j++){
            dot_product += x[j]*w1[i*16+j];
        }
        // Add bias then RELU
        l1_out[i] = relu(dot_product+b1[i]);  ;
    }
    
    // Second layer
    float l2_out[4];
    for (int i = 0; i < 4; i++){
        float dp2 = 0.;
        for (int j = 0; j < nh; j++){
            dp2 += l1_out[j]*w2[i*nh+j];
        }
        l2_out[i] = dp2; 
    }
    
    // Proposed update
    vec4 y = vec4(l2_out[0], l2_out[1], l2_out[2], l2_out[3]);
    
    // Output as prev state
    fragColor = id*0.1 + vec4(0.5);
    
    
    // If (noise>0.5) apply update
    vec2 p = vec2(uv.x/2.+sin(iTime/1000.), uv.y/2.+cos(iTime/1000.));
    if (random_1(p) < 0.5){
        fragColor = (id + y)*0.1 + vec4(0.5);
    }
    
    // If (mouse down) paint grey around it
    if(length(fragCoord.xy-iMouse.xy/2.)<(20.)){
        if (iMouse.z>0.5){fragColor = vec4(0.5);}
    }
    
    // Init 
    if (iFrame==0){fragColor = vec4(0.5);}
    
    
}
