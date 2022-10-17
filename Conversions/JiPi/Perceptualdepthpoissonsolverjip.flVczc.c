
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Preset: Keyboard' to iChannel3
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define STEPS 40  // advection steps

#define ts 0.2f    // advection curl
#define cs -2.0f   // curl scale
#define ls 0.05f   // laplacian scale
#define ps -2.0f   // laplacian of divergence scale
#define ds -0.4f   // divergence scale
#define dp -0.03f  // divergence update scale
#define pl 0.3f    // divergence smoothing
#define amp 1.0f   // self-amplification
#define upd 0.4f   // update smoothing

#define _D 0.6f    // diagonal weight

#define _K0 -20.0f/6.0f // laplacian center weight
#define _K1 4.0f/6.0f   // laplacian edge-neighbors
#define _K2 1.0f/6.0f   // laplacian vertex-neighbors

#define _G0 0.25f      // gaussian center weight
#define _G1 0.125f     // gaussian edge-neighbors
#define _G2 0.0625f    // gaussian vertex-neighbors

//__DEVICE__ bool reset() {
//    return texture(iChannel3, to_float2(32.5f/256.0f, 0.5f) ).x > 0.5f;
//}

__DEVICE__ float2 normz(float2 _x) {
  //return _x == to_float2_s(0.0f) ? to_float2_s(0.0f) : normalize(_x);
  return (_x.x == 0.0f && _x.y == 0.0f) ? to_float2_s(0.0f) : normalize(_x);
}

#define T(d) swi3(texture(iChannel0, fract_f2(aUv+d)),x,y,z)

__DEVICE__ float3 advect(float2 ab, float2 vUv, float2 texel, out float *curl, out float *div, out float3 *lapl, out float3 *blur, __TEXTURE2D__ iChannel0) {
    
    float2 aUv = vUv - ab * texel;
    float4 t = to_float4(texel.x, texel.y, -texel.y, 0.0f);

    float3 uv =    T( swi2(t,w,w)); float3 uv_n =  T( swi2(t,w,y)); float3 uv_e =  T( swi2(t,x,w));
    float3 uv_s =  T( swi2(t,w,z)); float3 uv_w =  T(-1.0f*swi2(t,x,w)); float3 uv_nw = T(-1.0f*swi2(t,x,z));
    float3 uv_sw = T(-1.0f*swi2(t,x,y)); float3 uv_ne = T( swi2(t,x,y)); float3 uv_se = T( swi2(t,x,z));
    
    *curl = uv_n.x - uv_s.x - uv_e.y + uv_w.y + _D * (uv_nw.x + uv_nw.y + uv_ne.x - uv_ne.y + uv_sw.y - uv_sw.x - uv_se.y - uv_se.x);
    *div  = uv_s.y - uv_n.y - uv_e.x + uv_w.x + _D * (uv_nw.x - uv_nw.y - uv_ne.x - uv_ne.y + uv_sw.x + uv_sw.y + uv_se.y - uv_se.x);
    *lapl = _K0*uv + _K1*(uv_n + uv_e + uv_w + uv_s) + _K2*(uv_nw + uv_sw + uv_ne + uv_se);
    *blur = _G0*uv + _G1*(uv_n + uv_e + uv_w + uv_s) + _G2*(uv_nw + uv_sw + uv_ne + uv_se);
    
    return uv;
}

__DEVICE__ float2 rot(float2 v, float th) {
  return to_float2(dot(v, to_float2(_cosf(th), -_sinf(th))), dot(v, to_float2(_sinf(th), _cosf(th)))); 
}


__DEVICE__ float3 Blending( __TEXTURE2D__ channel, float2 uv, float3 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float3(Par.x,Par.y,MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float3((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float3(tex.x*Par.x,tex.y*Par.y,tex.z*MulOff.x),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}



__KERNEL__ void PerceptualdepthpoissonsolverjipFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_CHECKBOX0(Reset, 0);

    //Blending
    CONNECT_SLIDER0(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER1(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER2(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);


    fragCoord+=0.5f;

    float2 vUv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;
    
    float3 lapl, blur;
    float curl, div;
    
    float3 uv = advect(to_float2_s(0), vUv, texel, &curl, &div, &lapl, &blur, iChannel0);

    float sp = ps * lapl.z;
    float sc = cs * curl;
    float sd = uv.z + dp * div + pl * lapl.z;
    float2 norm = normz(swi2(uv,x,y));

    float2 off = swi2(uv,x,y);
    float2 offd = off;
    float3 ab = to_float3_s(0);

    for(int i = 0; i < STEPS; i++) {
        advect(off, vUv, texel, &curl, &div, &lapl, &blur, iChannel0);
        offd = rot(offd,ts*curl);
        off += offd;
      ab += blur / (float)(STEPS);  
    }
  
    float2 tab = amp * swi2(ab,x,y) + ls * swi2(lapl,x,y) + norm * sp + swi2(uv,x,y) * ds * sd;    
    float2 rab = rot(tab,sc);
    
    float3 abd = _mix(to_float3_aw(rab,sd), uv, upd);
    
    if (iMouse.z > 0.0f) {
      float2 d = (fragCoord - swi2(iMouse,x,y)) / iResolution.x;
        float2 m = 0.1f * normz(d) * _expf(-length(d) / 0.02f);
        swi2S(abd,x,y, swi2(abd,x,y) + m);
        swi2S(uv,x,y, swi2(uv,x,y) + m);
    }
    
    
    if (Blend1>0.0) abd = Blending(iChannel1, fragCoord/iResolution, abd, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, iResolution);
    
    
    // initialize with noise
    float3 init = swi3(texture(iChannel1, vUv),x,y,z);
    //if(uv == to_float3_s(0) && init != to_float3(0) || reset()) {
    if((uv.x == 0.0f && uv.y == 0.0f && uv.z == 0.0f) && (init.x != 0.0f || init.y != 0.0f || init.z != 0.0f) || Reset) {
        fragColor = 1.0f * to_float4_aw(-0.5f + init, 1);
    } else {
        abd.z = clamp(abd.z, -1.0f, 1.0f);
        swi2S(abd,x,y, clamp(length(swi2(abd,x,y)) > 1.0f ? normz(swi2(abd,x,y)) : swi2(abd,x,y), -1.0f, 1.0f));
        fragColor = to_float4_aw(abd, 0.0f);
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0


// This computes the laplacian of the input


__DEVICE__ float laplacian(__TEXTURE2D__ sampler, float2 fragCoord, float2 iResolution) {
    float2 vUv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;
    
    // 3x3 neighborhood coordinates
    float step_x = texel.x;
    float step_y = texel.y;
    float2 n  = to_float2(0.0f, step_y);
    float2 ne = to_float2(step_x, step_y);
    float2 e  = to_float2(step_x, 0.0f);
    float2 se = to_float2(step_x, -step_y);
    float2 s  = to_float2(0.0f, -step_y);
    float2 sw = to_float2(-step_x, -step_y);
    float2 w  = to_float2(-step_x, 0.0f);
    float2 nw = to_float2(-step_x, step_y);

    float4 uv =    texture(sampler, fract_f2(vUv));
    float4 uv_n =  texture(sampler, fract_f2(vUv+n));
    float4 uv_e =  texture(sampler, fract_f2(vUv+e));
    float4 uv_s =  texture(sampler, fract_f2(vUv+s));
    float4 uv_w =  texture(sampler, fract_f2(vUv+w));
    float4 uv_nw = texture(sampler, fract_f2(vUv+nw));
    float4 uv_sw = texture(sampler, fract_f2(vUv+sw));
    float4 uv_ne = texture(sampler, fract_f2(vUv+ne));
    float4 uv_se = texture(sampler, fract_f2(vUv+se));
float ttttttttttttttt;    
    float2 diff = to_float2(
        0.5f * (uv_e.x - uv_w.x) + 0.25f * (uv_ne.x - uv_nw.x + uv_se.x - uv_sw.x),
        0.5f * (uv_n.y - uv_s.y) + 0.25f * (uv_ne.y + uv_nw.y - uv_se.y - uv_sw.y)
    );
    
    return diff.x + diff.y;
}

__KERNEL__ void PerceptualdepthpoissonsolverjipFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

  fragCoord+=0.5f;
  fragColor = to_float4(laplacian(iChannel0, fragCoord,iResolution),0,0,0);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer B' to iChannel0
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2
// Connect Buffer C 'Previsualization: Buffer D' to iChannel1


/*
  This convolves the Laplacian values from Buf A with a specially-designed Poisson solver kernel.
*/

/* 
  Optionally, blend the result of the large-kernel solver and a Jacobi method solver,
  with the value 0.0f being only the fast solver, and 1.0f being only the Jacobi solver.
  Curiously, using a negative value here may result in faster convergence.
*/
#define SOLVER_BLEND 0.0f

// If enabled, use a function that approximates the kernel. Otherwise, use the kernel.
//#define USE_FUNCTION

#define AMP 0.4375792f
#define OMEGA -1.007177f
#define OFFSET 0.002751625f

__DEVICE__ float neg_exp(float w) {
    return OFFSET + AMP*_expf(OMEGA*w);
}

__DEVICE__ float4 neighbor_avg(__TEXTURE2D__ sampler, float2 uv, float2 tx, float2 iResolution) {

    //const float _K1 = 4.0f/6.0f;   // edge-neighbors
    //const float _K2 = 1.0f/6.0f;   // vertex-neighbors
float zzzzzzzzzzzzzzzz;    
    // 3x3 neighborhood coordinates
    float step_x = tx.x;
    float step_y = tx.y;
    float2 n  = to_float2(0.0f, step_y);
    float2 ne = to_float2(step_x, step_y);
    float2 e  = to_float2(step_x, 0.0f);
    float2 se = to_float2(step_x, -step_y);
    float2 s  = to_float2(0.0f, -step_y);
    float2 sw = to_float2(-step_x, -step_y);
    float2 w  = to_float2(-step_x, 0.0f);
    float2 nw = to_float2(-step_x, step_y);

    float4 p_n =  texture(sampler, fract_f2(uv+n) );
    float4 p_e =  texture(sampler, fract_f2(uv+e) );
    float4 p_s =  texture(sampler, fract_f2(uv+s) );
    float4 p_w =  texture(sampler, fract_f2(uv+w) );
    float4 p_nw = texture(sampler, fract_f2(uv+nw));
    float4 p_sw = texture(sampler, fract_f2(uv+sw));
    float4 p_ne = texture(sampler, fract_f2(uv+ne));
    float4 p_se = texture(sampler, fract_f2(uv+se));
    
    return _K1*(p_n + p_e + p_w + p_s) + _K2*(p_nw + p_sw + p_ne + p_se);
}

__KERNEL__ void PerceptualdepthpoissonsolverjipFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;
float CCCCCCCCCCCCCCCCCC;
    //const float _K0 = 20.0f/6.0f; // center weight

    float2 uv = fragCoord / iResolution;
    float2 tx = 1.0f / iResolution;
    
    /* 
    Poisson solver kernel, computed using a custom tool. The curve ended up being very close
      to _expf(-x) times a constant (0.43757f*_expf(-1.0072f*x), R^2 = 0.9997f).
      The size of the kernel is truncated such that 99% of the summed kernel weight is accounted for. 
  */
    float a[121] = {
        1.2882849374994847E-4, 3.9883638750009155E-4, 9.515166750018973E-4, 0.0017727328875003466f, 0.0025830133546736567f, 0.002936729756271805f, 0.00258301335467621f, 0.0017727328875031007f, 9.515166750027364E-4, 3.988363875000509E-4, 1.2882849374998886E-4,
        3.988363875000656E-4, 0.00122005053750234f, 0.0029276701875229076f, 0.005558204850002636f, 0.008287002243739282f, 0.009488002668845403f, 0.008287002243717386f, 0.005558204850002533f, 0.002927670187515983f, 0.0012200505375028058f, 3.988363875001047E-4,
        9.515166750033415E-4, 0.0029276701875211478f, 0.007226947743770152f, 0.014378101312275642f, 0.02243013709214819f, 0.026345595431380788f, 0.02243013709216395f, 0.014378101312311218f, 0.007226947743759695f, 0.0029276701875111384f, 9.515166750008558E-4,
        0.0017727328875040689f, 0.005558204850002899f, 0.014378101312235814f, 0.030803252137257802f, 0.052905271651623786f, 0.06562027788638072f, 0.052905271651324026f, 0.03080325213733769f, 0.014378101312364885f, 0.005558204849979354f, 0.0017727328874979902f,
        0.0025830133546704635f, 0.008287002243679713f, 0.02243013709210261f, 0.052905271651950365f, 0.10825670746239457f, 0.15882720544362505f, 0.10825670746187367f, 0.05290527165080182f, 0.02243013709242713f, 0.008287002243769156f, 0.0025830133546869602f,
        0.00293672975627608f, 0.009488002668872716f, 0.026345595431503218f, 0.06562027788603421f, 0.15882720544151602f, 0.44102631192030745f, 0.15882720544590473f, 0.06562027788637015f, 0.026345595431065568f, 0.009488002668778417f, 0.0029367297562566848f,
        0.0025830133546700966f, 0.008287002243704267f, 0.022430137092024266f, 0.05290527165218751f, 0.10825670746234733f, 0.1588272054402839f, 0.1082567074615041f, 0.052905271651381314f, 0.022430137092484193f, 0.00828700224375486f, 0.002583013354686416f,
        0.0017727328875014527f, 0.005558204850013428f, 0.01437810131221156f, 0.03080325213737849f, 0.05290527165234342f, 0.06562027788535467f, 0.05290527165227899f, 0.03080325213731504f, 0.01437810131229074f, 0.005558204849973625f, 0.0017727328874977803f,
        9.515166750022218E-4, 0.002927670187526038f, 0.0072269477437592895f, 0.014378101312185454f, 0.02243013709218059f, 0.02634559543148722f, 0.0224301370922164f, 0.014378101312200022f, 0.007226947743773282f, 0.0029276701875125123f, 9.515166750016471E-4,
        3.988363875000695E-4, 0.0012200505375021846f, 0.002927670187525898f, 0.005558204849999022f, 0.008287002243689638f, 0.009488002668901728f, 0.008287002243695645f, 0.0055582048500028335f, 0.002927670187519828f, 0.0012200505375025872f, 3.988363874999818E-4,
        1.2882849374993535E-4, 3.9883638750004726E-4, 9.515166750034058E-4, 0.0017727328875029819f, 0.0025830133546718525f, 0.002936729756279661f, 0.002583013354672541f, 0.0017727328875033709f, 9.515166750023861E-4, 3.988363874999023E-4, 1.2882849374998856E-4
    };
    
    float4 accum = to_float4_s(0);
    for (int i = -5; i <= 5; i++) {
        for (int j = -5; j <= 5; j++) {
            int index = (j + 5) * 11 + (i + 5);
            
            #ifdef USE_FUNCTION
                float w = -neg_exp(_sqrtf((float)(i*i+j*j)));
            #else
              float w = -a[index];
            #endif

            accum += w * texture(iChannel0, fract_f2(uv + tx * to_float2(i,j)));
        }
    }
    
    float4 fast = _tex2DVecN(iChannel1,uv.x,uv.y,15) + accum;
    float4 slow = (neighbor_avg(iChannel2, uv, tx, iResolution) - _tex2DVecN(iChannel0,uv.x,uv.y,15)) / _K0;
  fragColor = _mix(fast, slow, SOLVER_BLEND);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer D                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer D 'Previsualization: Buffer C' to iChannel0


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
__DEVICE__ float gaussian(float w) {
    return _expf(-(w*w) / STDEV);
}


__KERNEL__ void PerceptualdepthpoissonsolverjipFuse__Buffer_D(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
    fragCoord+=0.5f;

    float2 uv = fragCoord / iResolution;
    float2 texel = 1.0f / iResolution;
float DDDDDDDDDDDDDDDDD;
    /*
    This is a Gaussian kernel, computed in a similar fashion to the Poisson solver kernel
    in Buf B, by performing several iterations of blurring with a 3x3 uniform kernel.
    I'm not sure this is an optimal blur kernel, but it has the best convergence properties
        of the kernels I have tried.        
  */    
    
    float a[121] = {
    79.19345413844742f, 186.95982256696507f, 355.7725142792509f, 557.2478169468127f, 728.3177627480716f, 792.5919289564812f, 728.3177627476995f, 557.2478169408488f, 355.77251427912773f, 186.95982256592688f, 79.19345413873424f,
    186.959822566632f, 425.7969440744425f, 791.9803669283782f, 1224.530847511507f, 1577.426364172433f, 1725.1209704031874f, 1577.4263641420696f, 1224.5308475559118f, 791.9803669376721f, 425.79694407230176f, 186.95982256563883f,
    355.7725142799761f, 791.9803669291889f, 1452.3604097140878f, 2209.3999629613313f, 2852.94106149519f, 3081.611616324168f, 2852.941061533745f, 2209.399962949136f, 1452.3604097512411f, 791.9803669249508f, 355.77251427710723f,
    557.2478169431769f, 1224.5308475304664f, 2209.399962931118f, 3366.385791159061f, 4279.72008889292f, 4678.378018687814f, 4279.720088892726f, 3366.3857911950213f, 2209.3999629391506f, 1224.5308475541954f, 557.2478169376274f,
    728.3177627458676f, 1577.4263641493967f, 2852.9410614801927f, 4279.720088978485f, 5506.183369574301f, 5920.756793177247f, 5506.1833697747215f, 4279.720088900363f, 2852.941061422592f, 1577.4263641763532f, 728.3177627380724f,
    792.591928959736f, 1725.1209703621885f, 3081.6116163468955f, 4678.3780186709955f, 5920.7567931890435f, 6475.16792876658f, 5920.756793140686f, 4678.378018700188f, 3081.611616243516f, 1725.1209704374526f, 792.5919289262789f,
    728.3177627503185f, 1577.4263641343025f, 2852.941061521645f, 4279.720088879328f, 5506.1833695725f, 5920.756793175392f, 5506.183369768556f, 4279.72008893864f, 2852.941061394854f, 1577.4263641878938f, 728.3177627378943f,
    557.2478169473138f, 1224.5308475281577f, 2209.3999629803902f, 3366.385791173652f, 4279.720088896571f, 4678.378018637779f, 4279.720088907292f, 3366.3857911422515f, 2209.399962952415f, 1224.5308475544125f, 557.2478169412809f,
    355.7725142789757f, 791.9803669328146f, 1452.3604096970955f, 2209.399962979159f, 2852.9410614343005f, 3081.611616339055f, 2852.941061433329f, 2209.3999629672044f, 1452.360409748826f, 791.9803669233293f, 355.77251427842157f,
    186.9598225669598f, 425.7969440755401f, 791.9803669309789f, 1224.5308475353752f, 1577.426364179527f, 1725.120970397883f, 1577.4263641778723f, 1224.5308475497768f, 791.980366930886f, 425.79694407371545f, 186.95982256558094f,
    79.19345413823653f, 186.9598225671716f, 355.7725142808836f, 557.2478169336465f, 728.317762748316f, 792.5919289394396f, 728.3177627443532f, 557.2478169373627f, 355.7725142796133f, 186.95982256574436f, 79.19345413875728
    };
     
    float4 accum = to_float4_s(0);
    float4 accumw = to_float4_s(0);

    for (int i = -5; i <= 5; i++) {
        for (int j = -5; j <= 5; j++) {
            int index = (j + 5) * 11 + (i + 5);
      
            #ifdef USE_FUNCTION
                float w = gaussian(_sqrtf((float)(i*i+j*j)));
            #else
              float w =  a[index];
            #endif
           
            float4 tx = texture(iChannel0, fract(uv + texel * to_float2(i,j)));

            #ifdef NORMALIZED_CONVOLUTION
                accumw += step(0.0f,tx) * w;
                accum  += step(0.0f,tx) * w * tx;
            #else
                accumw += w;
                accum  += w * tx;
            #endif
            
        }
    }
    
  fragColor = accum / accumw;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Rusty Metal' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel2
// Connect Image 'Previsualization: Buffer C' to iChannel0


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
#define DISP 0.01f

// contrast
#define SIGMOID_CONTRAST 20.0f

// mip level
#define MIP 0.0f

// comment to use the original vector field without running through the Poisson solver
#define POISSON


__DEVICE__ float3 contrast(float3 _x) {
  return 1.0f / (1.0f + exp_f3(-SIGMOID_CONTRAST * (_x - 0.5f)));    
}

__DEVICE__ float3 normz(float3 _x) {
  
  //return x == to_float3_s(0) ? to_float3_s(0) : normalize(x);
  return (_x.x == 0.0f && _x.y == 0.0f && _x.z == 0.0f)  ? to_float3_s(0) : normalize(_x);
}

__KERNEL__ void PerceptualdepthpoissonsolverjipFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    CONNECT_COLOR0(Color, 0.9f, 0.85f, 0.8f, 1.0f);
    CONNECT_SLIDER3(Contrast1, -1.0f, 2.0f, 0.9f);    
    CONNECT_SLIDER4(Contrast2, -1.0f, 2.0f, 0.9f);
    CONNECT_SLIDER5(Spec, -1.0f, 15.0f, 9.0f);
    CONNECT_SLIDER6(Attenuation, -1.0f, 1.0f, 0.25f);
    CONNECT_SLIDER7(Bump, -1.0f, 5.0f, 2.0f);

    CONNECT_CHECKBOX1(AutoLight, 1);
    CONNECT_POINT1(LightPoint1XY, 0.0f, 0.0f );
    CONNECT_SLIDER8(LightPoint1Z, -10.0f, 10.0f, 0.0f);
    
    

    float2 texel = 1.0f / iResolution;
    float2 uv = fragCoord / iResolution;

    float2 n  = to_float2(0.0f, texel.y);
    float2 e  = to_float2(texel.x, 0.0f);
    float2 s  = to_float2(0.0f, -texel.y);
    float2 w  = to_float2(-texel.x, 0.0f);

    #ifdef POISSON
        float d   = _tex2DVecN(iChannel0,uv.x,uv.y,15).x;
        float d_n = texture(iChannel0, fract_f2(uv+n)).x;
        float d_e = texture(iChannel0, fract_f2(uv+e)).x;
        float d_s = texture(iChannel0, fract_f2(uv+s)).x;
        float d_w = texture(iChannel0, fract_f2(uv+w)).x; 

        float d_ne = texture(iChannel0, fract_f2(uv+n+e)).x;
        float d_se = texture(iChannel0, fract_f2(uv+s+e)).x;
        float d_sw = texture(iChannel0, fract_f2(uv+s+w)).x;
        float d_nw = texture(iChannel0, fract_f2(uv+n+w)).x; 

        float dxn[3];
        float dyn[3];

        dyn[0] = d_nw - d_sw;
        dyn[1] = d_n  - d_s; 
        dyn[2] = d_ne - d_se;

        dxn[0] = d_ne - d_nw; 
        dxn[1] = d_e  - d_w; 
        dxn[2] = d_se - d_sw; 
    #else
        float2 d   = swi2(_tex2DVecN(iChannel2,uv.x,uv.y,15),x,y);
        float2 d_n = swi2(texture(iChannel2, fract_f2(uv+n)),x,y);
        float2 d_e = swi2(texture(iChannel2, fract_f2(uv+e)),x,y);
        float2 d_s = swi2(texture(iChannel2, fract_f2(uv+s)),x,y);
        float2 d_w = swi2(texture(iChannel2, fract_f2(uv+w)),x,y); 

        float2 d_ne = swi2(texture(iChannel2, fract_f2(uv+n+e)),x,y);
        float2 d_se = swi2(texture(iChannel2, fract_f2(uv+s+e)),x,y);
        float2 d_sw = swi2(texture(iChannel2, fract_f2(uv+s+w)),x,y);
        float2 d_nw = swi2(texture(iChannel2, fract_f2(uv+n+w)),x,y); 

        float dxn[3];
        float dyn[3];

        dyn[0] = d_n.y;
        dyn[1] = d.y; 
        dyn[2] = d_s.y;

        dxn[0] = d_e.x; 
        dxn[1] = d.x; 
        dxn[2] = d_w.x; 
    #endif

    float3 i   = swi3(texture(iChannel1, fract_f2(to_float2_s(0.5f) + DISP * to_float2(dxn[0],dyn[0]))),x,y,z);
    float3 i_n = swi3(texture(iChannel1, fract_f2(to_float2_s(0.5f) + DISP * to_float2(dxn[1],dyn[1]))),x,y,z);
    float3 i_e = swi3(texture(iChannel1, fract_f2(to_float2_s(0.5f) + DISP * to_float2(dxn[2],dyn[2]))),x,y,z);
    float3 i_s = swi3(texture(iChannel1, fract_f2(to_float2_s(0.5f) + DISP * to_float2(dxn[1],dyn[2]))),x,y,z);
    float3 i_w = swi3(texture(iChannel1, fract_f2(to_float2_s(0.5f) + DISP * to_float2(dxn[2],dyn[0]))),x,y,z);

    // The section below is an antialiased version of 
    // Shane's Bumped Sinusoidal Warp shadertoy here:
    // https://www.shadertoy.com/view/4l2XWK

    float3 sp = to_float3_aw(uv, 0);
    float3 light = to_float3(_cosf(iTime/2.0f)*0.5f, _sinf(iTime/2.0f)*0.5f, -1.0f);
    
    if(AutoLight == false)
       light = to_float3_aw(LightPoint1XY, LightPoint1Z-1.0f);     
  
    float3 ld = light - sp;
    float lDist = _fmaxf(length(ld), 0.001f);
    ld /= lDist;  
    float atten = _fminf(1.0f/(Attenuation + lDist*0.5f + lDist*lDist*0.05f), 1.0f);
    float3 rd = normalize(to_float3_aw(uv - 1.0f, 1.0f));

    float bump = Bump;//2.0f;

    float spec = 0.0f;
    for(int i = 0; i < 3; i++) {
        for(int j = 0; j < 3; j++) {
            float2 dxy = to_float2(dxn[i], dyn[j]);
            float3 bn = normalize(to_float3_aw(dxy * bump, -1.0f));
            spec += _powf(_fmaxf(dot( reflect(-ld, bn), -rd), 0.0f), 8.0f) / Spec;                 
        }
    }

    // end bumpmapping section

    float3 ib = 0.4f * i + 0.15f * (i_n+i_e+i_s+i_w);

    //float3 texCol = 0.9f*contrast(0.9f*ib);
    float3 texCol = Contrast1*contrast(Contrast2*ib);

    fragColor = to_float4_aw((texCol + swi3(Color,x,y,z)*spec) * atten, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}
