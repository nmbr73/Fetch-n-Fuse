

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
	This shader adds perceived depth to a vector map by running the map through a Poisson solver.
	A vector map is supplied by a dynamical system in Buf A (see: https://www.shadertoy.com/view/Mtc3Dj),
    the Laplacian of the map is computed in Buf B, and the depth of the map is computed using a
    Poisson solver in Buf C and Buf D. The Poisson solver uses a technique conceptually similar to 
    the standard Jacobi method, but with a larger kernel and faster convergence times.

	Comment out "#define POISSON" below to render using the original vector map without using the
    Poisson solver.
*/

// displacement
#define DISP 0.01

// contrast
#define SIGMOID_CONTRAST 20.0

// mip level
#define MIP 0.0

// comment to use the original vector field without running through the Poisson solver
#define POISSON


vec3 contrast(vec3 x) {
	return 1.0 / (1.0 + exp(-SIGMOID_CONTRAST * (x - 0.5)));    
}

vec3 normz(vec3 x) {
	return x == vec3(0) ? vec3(0) : normalize(x);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    vec2 texel = 1. / iResolution.xy;
    vec2 uv = fragCoord.xy / iResolution.xy;

    vec2 n  = vec2(0.0, texel.y);
    vec2 e  = vec2(texel.x, 0.0);
    vec2 s  = vec2(0.0, -texel.y);
    vec2 w  = vec2(-texel.x, 0.0);

    #ifdef POISSON
        float d   = texture(iChannel0, uv).x;
        float d_n = texture(iChannel0, fract(uv+n)).x;
        float d_e = texture(iChannel0, fract(uv+e)).x;
        float d_s = texture(iChannel0, fract(uv+s)).x;
        float d_w = texture(iChannel0, fract(uv+w)).x; 

        float d_ne = texture(iChannel0, fract(uv+n+e)).x;
        float d_se = texture(iChannel0, fract(uv+s+e)).x;
        float d_sw = texture(iChannel0, fract(uv+s+w)).x;
        float d_nw = texture(iChannel0, fract(uv+n+w)).x; 

        float dxn[3];
        float dyn[3];

        dyn[0] = d_nw - d_sw;
        dyn[1] = d_n  - d_s; 
        dyn[2] = d_ne - d_se;

        dxn[0] = d_ne - d_nw; 
        dxn[1] = d_e  - d_w; 
        dxn[2] = d_se - d_sw; 
    #else
        vec2 d   = texture(iChannel2, uv).xy;
        vec2 d_n = texture(iChannel2, fract(uv+n)).xy;
        vec2 d_e = texture(iChannel2, fract(uv+e)).xy;
        vec2 d_s = texture(iChannel2, fract(uv+s)).xy;
        vec2 d_w = texture(iChannel2, fract(uv+w)).xy; 

        vec2 d_ne = texture(iChannel2, fract(uv+n+e)).xy;
        vec2 d_se = texture(iChannel2, fract(uv+s+e)).xy;
        vec2 d_sw = texture(iChannel2, fract(uv+s+w)).xy;
        vec2 d_nw = texture(iChannel2, fract(uv+n+w)).xy; 

        float dxn[3];
        float dyn[3];

        dyn[0] = d_n.y;
        dyn[1] = d.y; 
        dyn[2] = d_s.y;

        dxn[0] = d_e.x; 
        dxn[1] = d.x; 
        dxn[2] = d_w.x; 
    #endif

    vec3 i   = texture(iChannel1, fract(vec2(0.5) + DISP * vec2(dxn[0],dyn[0])), MIP).xyz;
    vec3 i_n = texture(iChannel1, fract(vec2(0.5) + DISP * vec2(dxn[1],dyn[1])), MIP).xyz;
    vec3 i_e = texture(iChannel1, fract(vec2(0.5) + DISP * vec2(dxn[2],dyn[2])), MIP).xyz;
    vec3 i_s = texture(iChannel1, fract(vec2(0.5) + DISP * vec2(dxn[1],dyn[2])), MIP).xyz;
    vec3 i_w = texture(iChannel1, fract(vec2(0.5) + DISP * vec2(dxn[2],dyn[0])), MIP).xyz;

    // The section below is an antialiased version of 
    // Shane's Bumped Sinusoidal Warp shadertoy here:
    // https://www.shadertoy.com/view/4l2XWK

    vec3 sp = vec3(uv, 0);
    vec3 light = vec3(cos(iTime/2.0)*0.5, sin(iTime/2.0)*0.5, -1.);
    vec3 ld = light - sp;
    float lDist = max(length(ld), 0.001);
    ld /= lDist;  
    float atten = min(1./(0.25 + lDist*0.5 + lDist*lDist*0.05), 1.);
    vec3 rd = normalize(vec3(uv - 1.0, 1.));

    float bump = 2.0;



    float spec = 0.0;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            vec2 dxy = vec2(dxn[i], dyn[j]);
            vec3 bn = normalize(vec3(dxy * bump, -1.0));
            spec += pow(max(dot( reflect(-ld, bn), -rd), 0.), 8.) / 9.0;                 
        }
    }

    // end bumpmapping section

    vec3 ib = 0.4 * i + 0.15 * (i_n+i_e+i_s+i_w);

    vec3 texCol = 0.9*contrast(0.9*ib);

    fragColor = vec4((texCol + vec3(0.9, 0.85, 0.8)*spec) * atten,1.0);

}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define STEPS 40  // advection steps

#define ts 0.2    // advection curl
#define cs -2.0   // curl scale
#define ls 0.05   // laplacian scale
#define ps -2.0   // laplacian of divergence scale
#define ds -0.4   // divergence scale
#define dp -0.03  // divergence update scale
#define pl 0.3    // divergence smoothing
#define amp 1.0   // self-amplification
#define upd 0.4   // update smoothing

#define _D 0.6    // diagonal weight

#define _K0 -20.0/6.0 // laplacian center weight
#define _K1 4.0/6.0   // laplacian edge-neighbors
#define _K2 1.0/6.0   // laplacian vertex-neighbors

#define _G0 0.25      // gaussian center weight
#define _G1 0.125     // gaussian edge-neighbors
#define _G2 0.0625    // gaussian vertex-neighbors

bool reset() {
    return texture(iChannel3, vec2(32.5/256.0, 0.5) ).x > 0.5;
}

vec2 normz(vec2 x) {
	return x == vec2(0.0) ? vec2(0.0) : normalize(x);
}

#define T(d) texture(iChannel0, fract(aUv+d)).xyz

vec3 advect(vec2 ab, vec2 vUv, vec2 texel, out float curl, out float div, out vec3 lapl, out vec3 blur) {
    
    vec2 aUv = vUv - ab * texel;
    vec4 t = vec4(texel, -texel.y, 0.0);

    vec3 uv =    T( t.ww); vec3 uv_n =  T( t.wy); vec3 uv_e =  T( t.xw);
    vec3 uv_s =  T( t.wz); vec3 uv_w =  T(-t.xw); vec3 uv_nw = T(-t.xz);
    vec3 uv_sw = T(-t.xy); vec3 uv_ne = T( t.xy); vec3 uv_se = T( t.xz);
    
    curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + _D * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    lapl = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    blur = _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
    
    return uv;
}

vec2 rot(vec2 v, float th) {
	return vec2(dot(v, vec2(cos(th), -sin(th))), dot(v, vec2(sin(th), cos(th)))); 
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 vUv = fragCoord.xy / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;
    
    vec3 lapl, blur;
    float curl, div;
    
    vec3 uv = advect(vec2(0), vUv, texel, curl, div, lapl, blur);

    float sp = ps * lapl.z;
    float sc = cs * curl;
	float sd = uv.z + dp * div + pl * lapl.z;
    vec2 norm = normz(uv.xy);

    vec2 off = uv.xy;
    vec2 offd = off;
    vec3 ab = vec3(0);

    for(int i = 0; i < STEPS; i++) {
        advect(off, vUv, texel, curl, div, lapl, blur);
        offd = rot(offd,ts*curl);
        off += offd;
    	ab += blur / float(STEPS);  
    }
    
    vec2 tab = amp * ab.xy + ls * lapl.xy + norm * sp + uv.xy * ds * sd;    
    vec2 rab = rot(tab,sc);
    
    vec3 abd = mix(vec3(rab,sd), uv, upd);
    
    if (iMouse.z > 0.0) {
    	vec2 d = (fragCoord.xy - iMouse.xy) / iResolution.x;
        vec2 m = 0.1 * normz(d) * exp(-length(d) / 0.02);
        abd.xy += m;
        uv.xy += m;
    }
    
    // initialize with noise
    vec3 init = texture(iChannel1, vUv, 5.0).xyz;
    if(uv == vec3(0) && init != vec3(0) || reset()) {
        fragColor = 1.0 * vec4(-0.5 + init, 1);
    } else {
        abd.z = clamp(abd.z, -1.0, 1.0);
        abd.xy = clamp(length(abd.xy) > 1.0 ? normz(abd.xy) : abd.xy, -1.0, 1.0);
        fragColor = vec4(abd, 0.0);
    }

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// This computes the laplacian of the input


float laplacian(sampler2D sampler, vec2 fragCoord) {
    vec2 vUv = fragCoord.xy / iResolution.xy;
    vec2 texel = 1. / iResolution.xy;
    
    // 3x3 neighborhood coordinates
    float step_x = texel.x;
    float step_y = texel.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);

    vec4 uv =    texture(iChannel0, fract(vUv));
    vec4 uv_n =  texture(iChannel0, fract(vUv+n));
    vec4 uv_e =  texture(iChannel0, fract(vUv+e));
    vec4 uv_s =  texture(iChannel0, fract(vUv+s));
    vec4 uv_w =  texture(iChannel0, fract(vUv+w));
    vec4 uv_nw = texture(iChannel0, fract(vUv+nw));
    vec4 uv_sw = texture(iChannel0, fract(vUv+sw));
    vec4 uv_ne = texture(iChannel0, fract(vUv+ne));
    vec4 uv_se = texture(iChannel0, fract(vUv+se));
    
    vec2 diff = vec2(
        0.5 * (uv_e.x - uv_w.x) + 0.25 * (uv_ne.x - uv_nw.x + uv_se.x - uv_sw.x),
        0.5 * (uv_n.y - uv_s.y) + 0.25 * (uv_ne.y + uv_nw.y - uv_se.y - uv_sw.y)
    );
    
    return diff.x + diff.y;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4(laplacian(iChannel0, fragCoord),0,0,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
/*
	This convolves the Laplacian values from Buf A with a specially-designed Poisson solver kernel.
*/

/* 
	Optionally, blend the result of the large-kernel solver and a Jacobi method solver,
	with the value 0.0 being only the fast solver, and 1.0 being only the Jacobi solver.
	Curiously, using a negative value here may result in faster convergence.
*/
#define SOLVER_BLEND 0.0

// If enabled, use a function that approximates the kernel. Otherwise, use the kernel.
//#define USE_FUNCTION

#define AMP 0.4375792
#define OMEGA -1.007177
#define OFFSET 0.002751625

float neg_exp(float w) {
    return OFFSET + AMP*exp(OMEGA*w);
}

vec4 neighbor_avg(sampler2D sampler, vec2 uv, vec2 tx) {

    const float _K1 = 4.0/6.0;   // edge-neighbors
    const float _K2 = 1.0/6.0;   // vertex-neighbors
    
    // 3x3 neighborhood coordinates
    float step_x = tx.x;
    float step_y = tx.y;
    vec2 n  = vec2(0.0, step_y);
    vec2 ne = vec2(step_x, step_y);
    vec2 e  = vec2(step_x, 0.0);
    vec2 se = vec2(step_x, -step_y);
    vec2 s  = vec2(0.0, -step_y);
    vec2 sw = vec2(-step_x, -step_y);
    vec2 w  = vec2(-step_x, 0.0);
    vec2 nw = vec2(-step_x, step_y);

    vec4 p_n =  texture(sampler, fract(uv+n) );
    vec4 p_e =  texture(sampler, fract(uv+e) );
    vec4 p_s =  texture(sampler, fract(uv+s) );
    vec4 p_w =  texture(sampler, fract(uv+w) );
    vec4 p_nw = texture(sampler, fract(uv+nw));
    vec4 p_sw = texture(sampler, fract(uv+sw));
    vec4 p_ne = texture(sampler, fract(uv+ne));
    vec4 p_se = texture(sampler, fract(uv+se));
    
    return _K1*(p_n + p_e + p_w + p_s) + _K2*(p_nw + p_sw + p_ne + p_se);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    const float _K0 = 20.0/6.0; // center weight

    vec2 uv = fragCoord / iResolution.xy;
    vec2 tx = 1.0 / iResolution.xy;
    
    /* 
		Poisson solver kernel, computed using a custom tool. The curve ended up being very close
    	to exp(-x) times a constant (0.43757*exp(-1.0072*x), R^2 = 0.9997).
    	The size of the kernel is truncated such that 99% of the summed kernel weight is accounted for. 
	*/
    float a[121] = float[](
        1.2882849374994847E-4, 3.9883638750009155E-4, 9.515166750018973E-4, 0.0017727328875003466, 0.0025830133546736567, 0.002936729756271805, 0.00258301335467621, 0.0017727328875031007, 9.515166750027364E-4, 3.988363875000509E-4, 1.2882849374998886E-4,
        3.988363875000656E-4, 0.00122005053750234, 0.0029276701875229076, 0.005558204850002636, 0.008287002243739282, 0.009488002668845403, 0.008287002243717386, 0.005558204850002533, 0.002927670187515983, 0.0012200505375028058, 3.988363875001047E-4,
        9.515166750033415E-4, 0.0029276701875211478, 0.007226947743770152, 0.014378101312275642, 0.02243013709214819, 0.026345595431380788, 0.02243013709216395, 0.014378101312311218, 0.007226947743759695, 0.0029276701875111384, 9.515166750008558E-4,
        0.0017727328875040689, 0.005558204850002899, 0.014378101312235814, 0.030803252137257802, 0.052905271651623786, 0.06562027788638072, 0.052905271651324026, 0.03080325213733769, 0.014378101312364885, 0.005558204849979354, 0.0017727328874979902,
        0.0025830133546704635, 0.008287002243679713, 0.02243013709210261, 0.052905271651950365, 0.10825670746239457, 0.15882720544362505, 0.10825670746187367, 0.05290527165080182, 0.02243013709242713, 0.008287002243769156, 0.0025830133546869602,
        0.00293672975627608, 0.009488002668872716, 0.026345595431503218, 0.06562027788603421, 0.15882720544151602, 0.44102631192030745, 0.15882720544590473, 0.06562027788637015, 0.026345595431065568, 0.009488002668778417, 0.0029367297562566848,
        0.0025830133546700966, 0.008287002243704267, 0.022430137092024266, 0.05290527165218751, 0.10825670746234733, 0.1588272054402839, 0.1082567074615041, 0.052905271651381314, 0.022430137092484193, 0.00828700224375486, 0.002583013354686416,
        0.0017727328875014527, 0.005558204850013428, 0.01437810131221156, 0.03080325213737849, 0.05290527165234342, 0.06562027788535467, 0.05290527165227899, 0.03080325213731504, 0.01437810131229074, 0.005558204849973625, 0.0017727328874977803,
        9.515166750022218E-4, 0.002927670187526038, 0.0072269477437592895, 0.014378101312185454, 0.02243013709218059, 0.02634559543148722, 0.0224301370922164, 0.014378101312200022, 0.007226947743773282, 0.0029276701875125123, 9.515166750016471E-4,
        3.988363875000695E-4, 0.0012200505375021846, 0.002927670187525898, 0.005558204849999022, 0.008287002243689638, 0.009488002668901728, 0.008287002243695645, 0.0055582048500028335, 0.002927670187519828, 0.0012200505375025872, 3.988363874999818E-4,
        1.2882849374993535E-4, 3.9883638750004726E-4, 9.515166750034058E-4, 0.0017727328875029819, 0.0025830133546718525, 0.002936729756279661, 0.002583013354672541, 0.0017727328875033709, 9.515166750023861E-4, 3.988363874999023E-4, 1.2882849374998856E-4
    );
    
    vec4 accum = vec4(0);
    for (int i = -5; i <= 5; i++) {
        for (int j = -5; j <= 5; j++) {
            int index = (j + 5) * 11 + (i + 5);
            
            #ifdef USE_FUNCTION
                float w = -neg_exp(sqrt(float(i*i+j*j)));
            #else
            	float w = -a[index];
            #endif

            accum += w * texture(iChannel0, fract(uv + tx * vec2(i,j)));
        }
    }
    
    vec4 fast = texture(iChannel1, uv) + accum;
    vec4 slow = (neighbor_avg(iChannel2, uv, tx) - texture(iChannel0, uv)) / _K0;
	fragColor = mix(fast, slow, SOLVER_BLEND);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
/* 
    This uses a blur kernel to iteratively blur the poisson solver output from Buf B
    in order to fill in areas with small laplacian values.
*/

/* 
	Using normalized convolution to throw out negative values from the solver.
	This speeds up convergence but is less accurate overall
*/
//#define NORMALIZED_CONVOLUTION

// If enabled, use a function that approximates the kernel. Otherwise, use the kernel.
//#define USE_FUNCTION

/* 
	The standard deviation was determined by fitting a gaussian to the kernel below.
	This can be changed in order to control the contrast of the resulting solution 
    if desired. Higher values result in less contrast, lower values in higher contrast.
*/
#define STDEV 15.4866382262
float gaussian(float w) {
    return exp(-(w*w) / STDEV);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord / iResolution.xy;
    vec2 texel = 1.0 / iResolution.xy;

    /*
		This is a Gaussian kernel, computed in a similar fashion to the Poisson solver kernel
		in Buf B, by performing several iterations of blurring with a 3x3 uniform kernel.
		I'm not sure this is an optimal blur kernel, but it has the best convergence properties
        of the kernels I have tried.        
	*/    
    
    float a[121] = float[](
		79.19345413844742, 186.95982256696507, 355.7725142792509, 557.2478169468127, 728.3177627480716, 792.5919289564812, 728.3177627476995, 557.2478169408488, 355.77251427912773, 186.95982256592688, 79.19345413873424,
		186.959822566632, 425.7969440744425, 791.9803669283782, 1224.530847511507, 1577.426364172433, 1725.1209704031874, 1577.4263641420696, 1224.5308475559118, 791.9803669376721, 425.79694407230176, 186.95982256563883,
		355.7725142799761, 791.9803669291889, 1452.3604097140878, 2209.3999629613313, 2852.94106149519, 3081.611616324168, 2852.941061533745, 2209.399962949136, 1452.3604097512411, 791.9803669249508, 355.77251427710723,
		557.2478169431769, 1224.5308475304664, 2209.399962931118, 3366.385791159061, 4279.72008889292, 4678.378018687814, 4279.720088892726, 3366.3857911950213, 2209.3999629391506, 1224.5308475541954, 557.2478169376274,
		728.3177627458676, 1577.4263641493967, 2852.9410614801927, 4279.720088978485, 5506.183369574301, 5920.756793177247, 5506.1833697747215, 4279.720088900363, 2852.941061422592, 1577.4263641763532, 728.3177627380724,
		792.591928959736, 1725.1209703621885, 3081.6116163468955, 4678.3780186709955, 5920.7567931890435, 6475.16792876658, 5920.756793140686, 4678.378018700188, 3081.611616243516, 1725.1209704374526, 792.5919289262789,
		728.3177627503185, 1577.4263641343025, 2852.941061521645, 4279.720088879328, 5506.1833695725, 5920.756793175392, 5506.183369768556, 4279.72008893864, 2852.941061394854, 1577.4263641878938, 728.3177627378943,
		557.2478169473138, 1224.5308475281577, 2209.3999629803902, 3366.385791173652, 4279.720088896571, 4678.378018637779, 4279.720088907292, 3366.3857911422515, 2209.399962952415, 1224.5308475544125, 557.2478169412809,
		355.7725142789757, 791.9803669328146, 1452.3604096970955, 2209.399962979159, 2852.9410614343005, 3081.611616339055, 2852.941061433329, 2209.3999629672044, 1452.360409748826, 791.9803669233293, 355.77251427842157,
		186.9598225669598, 425.7969440755401, 791.9803669309789, 1224.5308475353752, 1577.426364179527, 1725.120970397883, 1577.4263641778723, 1224.5308475497768, 791.980366930886, 425.79694407371545, 186.95982256558094,
		79.19345413823653, 186.9598225671716, 355.7725142808836, 557.2478169336465, 728.317762748316, 792.5919289394396, 728.3177627443532, 557.2478169373627, 355.7725142796133, 186.95982256574436, 79.19345413875728
	);
     
    vec4 accum = vec4(0);
    vec4 accumw = vec4(0);

    for (int i = -5; i <= 5; i++) {
        for (int j = -5; j <= 5; j++) {
            int index = (j + 5) * 11 + (i + 5);
			
            #ifdef USE_FUNCTION
                float w = gaussian(sqrt(float(i*i+j*j)));
            #else
            	float w =  a[index];
            #endif
           
            vec4 tx = texture(iChannel0, fract(uv + texel * vec2(i,j)));

            #ifdef NORMALIZED_CONVOLUTION
                accumw += step(0.0,tx) * w;
                accum  += step(0.0,tx) * w * tx;
            #else
                accumw += w;
                accum  += w * tx;
            #endif
            
        }
    }
    
	fragColor = accum / accumw;

}