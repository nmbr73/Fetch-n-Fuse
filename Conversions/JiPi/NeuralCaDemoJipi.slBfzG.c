
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Texture: Blending' to iChannel1

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// Random Number Generator
// From https://www.shadertoy.com/view/MsKGWz:
// See Stack Overflow: http://stackoverflow.com/questions/5149544/can-i-generate-a-random-number-inside-a-pixel-shader/10625698#10625698
__DEVICE__ float random_1( float2 p )
{
    float2 r = to_float2(
        23.14069263277926f, // e^pi (Gelfond's constant)
         2.665144142690225f // 2^_sqrtf(2) (Gelfondâ€“Schneider constant)
    );
    return fract( _cosf( mod_f( 12345678.0f, 256.0f * dot(p,r) ) ) );
}

// Samples the neighbourhood (wrapping around where needed)
__DEVICE__ float2 coord (float2 fragCoord, float2 offset, float2 R){
    float x = mod_f(fragCoord.x + offset.x, iResolution.x);
    float y = mod_f(fragCoord.y + offset.y, iResolution.y);
    return to_float2(x, y)/iResolution;
}
__DEVICE__ void sample_tex (float2 fragCoord, float4 tex_arx[9], float2 R, __TEXTURE2D__ iChannel0){
    #ifdef ORG
    float4 tex[9] = {
        (texture(iChannel0, coord(fragCoord, to_float2(-1, 1),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(0, 1),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(1, 1),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(-1, 0),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(0, 0),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(1, 0),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(-1, -1),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(0, -1),R))-0.5f)*10.0f,
        (texture(iChannel0, coord(fragCoord, to_float2(1, -1),R))-0.5f)*10.
    };
    return tex;
    #endif
    tex_arx[0] = (texture(iChannel0, coord(fragCoord, to_float2(-1, 1),R))-0.5f)*10.0f;
    tex_arx[1] = (texture(iChannel0, coord(fragCoord, to_float2(0, 1),R))-0.5f)*10.0f;
    tex_arx[2] = (texture(iChannel0, coord(fragCoord, to_float2(1, 1),R))-0.5f)*10.0f;
    tex_arx[3] = (texture(iChannel0, coord(fragCoord, to_float2(-1, 0),R))-0.5f)*10.0f;
    tex_arx[4] = (texture(iChannel0, coord(fragCoord, to_float2(0, 0),R))-0.5f)*10.0f;
    tex_arx[5] = (texture(iChannel0, coord(fragCoord, to_float2(1, 0),R))-0.5f)*10.0f;
    tex_arx[6] = (texture(iChannel0, coord(fragCoord, to_float2(-1, -1),R))-0.5f)*10.0f;
    tex_arx[7] = (texture(iChannel0, coord(fragCoord, to_float2(0, -1),R))-0.5f)*10.0f;
    tex_arx[8] = (texture(iChannel0, coord(fragCoord, to_float2(1, -1),R))-0.5f)*10.0f;
    
}

// The four kernels used
__DEVICE__ float4 ident(float2 fragCoord, float4 tex[9] ){
    return tex[4]; // no offset
}
__DEVICE__ float4 sobel_x(float2 fragCoord, float4 tex[9]){
    float4 result = -1.0f*tex[0]-2.0f*tex[3]-1.0f*tex[6]+1.0f*tex[2]+2.0f*tex[5]+1.0f*tex[8];
    return result;
}
__DEVICE__ float4 sobel_y(float2 fragCoord, float4 tex[9]){
    float4 result = -1.0f*tex[0]-2.0f*tex[1]-1.0f*tex[2]+1.0f*tex[6]+2.0f*tex[7]+1.0f*tex[8];
    return result;
}
__DEVICE__ float4 lap(float2 fragCoord, float4 tex[9]){
    float4 result = 1.0f*tex[0]+2.0f*tex[1]+1.0f*tex[2]+2.0f*tex[3]-12.0f*tex[4]+2.0f*tex[5]+1.0f*tex[6]+2.0f*tex[7]+1.0f*tex[8]; // was an errant +2.
    return result;
}

// Our activation function
__DEVICE__ float relu(float x){
    if (x > 0.0f){return x;}
    return 0.0f;
}

__DEVICE__ float4 Blending( __TEXTURE2D__ channel, float2 uv, float4 Q, float Blend, float2 Par, float2 MulOff, int Modus, float2 U, float2 R)
{
   
    if (Blend > 0.0f)
    {
      //float2 tuv = U/R;
      float4 tex = texture(channel,uv);

      if (tex.w > 0.0f)
      {      
        if ((int)Modus&2)
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
          //swi3S(Q,x,y,w, _mix(swi3(Q,x,y,w),(swi3(tex,x,y,z)+MulOff.y)*MulOff.x,Blend));

        if ((int)Modus&4)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par , Blend));
          //swi2S(Q,x,y, _mix( swi2(Q,x,y),  Par, Blend));
          //swi3S(Q,x,y,z, _mix(swi3(Q,x,y,z), (swi3(tex,x,y,z)+MulOff.y)*MulOff.x, Blend));  
          Q = _mix(Q,to_float4(Par.x,Par.y,(tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x),Blend);
        
        
        if ((int)Modus&8)
          //swi2S(Q,x,y, _mix( swi2(Q,x,y), Par, Blend));
          Q = _mix(Q,to_float4((tex.x+MulOff.y)*MulOff.x,(tex.y+MulOff.y)*MulOff.x,Par.x,Par.y),Blend);
          //Q.z = _mix( Q.z,  (tex.x+MulOff.y)*MulOff.x, Blend);
          //swi2S(Q,z,w, _mix( swi2(Q,z,w), swi2(tex,x,y)*Par, Blend));

        if ((int)Modus&16) 
          //swi2S(Q,z,w, _mix(swi2(Q,z,w),  swi2(tex,x,y)*Par, Blend));
          Q = _mix(Q,to_float4(Par.x,Par.y,MulOff.x,MulOff.y),Blend);
      }
      else
        if ((int)Modus&32) //Special
          //swi2S(Q,x,y, _mix(swi2(Q,x,y),(swi2(tex,x,y)+MulOff.y)*MulOff.x,Blend));
          Q = _mix(Q,(tex+MulOff.y)*MulOff.x,Blend);
    }
  
  return Q;
}


__KERNEL__ void NeuralCaDemoJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0)
{
    CONNECT_CHECKBOX0(Reset, 0);
  
      //Blending
    CONNECT_SLIDER2(Blend1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER3(Blend1Off, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER4(Blend1Mul, -10.0f, 10.0f, 1.0f);
    CONNECT_BUTTON0(Modus, 1, Start,  Velo, Mass, InvMass, Special);
    CONNECT_POINT2(Par1, 0.0f, 0.0f);
  
    fragCoord+=0.5f;

    // Paste your weights hre:
    const int nh  = 8;
    float b1[8]   = {-0.005607239902019501f,0.20737116038799286f,-0.15958012640476227f,0.1839800626039505f,0.10558734834194183f,0.2413249909877777f,0.22694732248783112f,0.1544546037912368f};
    float w1[128] = {-0.17097297310829163f,0.06744687259197235f,0.21897636353969574f,0.10879534482955933f,-0.14843542873859406f,0.20935866236686707f,-0.1830487698316574f,-0.1983712762594223f,0.19102470576763153f,0.1465626209974289f,-0.1298714131116867f,0.12886269390583038f,-0.10719338804483414f,0.1895245760679245f,0.15244363248348236f,-0.12479807436466217f,0.19730904698371887f,0.2103326916694641f,0.07020334899425507f,0.06035710498690605f,0.12206670641899109f,-0.1604532152414322f,-0.11493607610464096f,0.21523287892341614f,0.2743070721626282f,0.011186826042830944f,0.1369483321905136f,0.12041302770376205f,-0.08342280238866806f,0.22404158115386963f,0.030259817838668823f,-0.1686122715473175f,-0.06810998171567917f,0.029785193502902985f,0.05608399957418442f,0.25010204315185547f,-0.07401710748672485f,-0.08251817524433136f,-0.053251057863235474f,0.043036624789237976f,0.14328479766845703f,-0.12106183916330338f,-0.15435048937797546f,0.09810590744018555f,0.054376404732465744f,-0.11873634159564972f,-0.0808294340968132f,0.0697202980518341f,0.2648638188838959f,-0.06522378325462341f,0.060611315071582794f,0.22495940327644348f,-0.010338977910578251f,0.17524294555187225f,0.17996922135353088f,0.16922366619110107f,0.218682199716568f,0.02084919810295105f,-0.1255321353673935f,0.12736909091472626f,-0.025168975815176964f,0.1348275989294052f,-0.08117184787988663f,0.12416274845600128f,-0.1708059310913086f,-0.11634736508131027f,0.20006434619426727f,0.03734216466546059f,-0.10498269647359848f,0.12329734861850739f,0.16503401100635529f,0.08998304605484009f,-0.260944128036499f,-0.10085125267505646f,-0.021763155236840248f,-0.15479378402233124f,0.011685313656926155f,-0.2371322512626648f,0.07034227252006531f,0.026207149028778076f,0.15099263191223145f,-0.0033387993462383747f,-0.031722985208034515f,0.21575583517551422f,0.09879902750253677f,0.09183311462402344f,0.1828172504901886f,-0.2255178540945053f,-0.012155634351074696f,0.1523420810699463f,-0.05294913053512573f,-0.08934169262647629f,0.22459764778614044f,-0.20958268642425537f,-0.13207976520061493f,-0.0333939790725708f,-0.08564555644989014f,-0.03224276378750801f,0.07250171154737473f,-0.250521183013916f,-0.21712614595890045f,0.15493904054164886f,0.06420993059873581f,0.06199895218014717f,-0.2068699151277542f,-0.019539864733815193f,-0.009239627048373222f,0.050233643501996994f,0.022854316979646683f,-0.2485826462507248f,0.19061429798603058f,-0.17818520963191986f,0.03746657446026802f,0.1707015335559845f,0.11888623237609863f,-0.004525625612586737f,-0.002364585641771555f,0.0060291169211268425f,-0.2719455659389496f,-0.0008139413548633456f,-0.17593756318092346f,0.0024618881288915873f,-0.10072672367095947f,-0.1509338617324829f,-0.21447797119617462f,0.11333387345075607f,0.18015369772911072f,0.158259317278862f};
    float w2[32]  = {0.0004315134137868881f,-0.011854984797537327f,0.03449397534132004f,-0.007808920461684465f,0.009794715791940689f,0.0007741426234133542f,-0.005454638507217169f,0.001134452992118895f,-0.006665459368377924f,-0.010381652973592281f,0.030350729823112488f,-0.0007826134096831083f,0.002680370118469f,-0.004545061849057674f,0.0008326310198754072f,0.0009512250544503331f,-0.012409443967044353f,-0.009911532513797283f,0.02520965039730072f,0.004211696796119213f,-0.003577346447855234f,-0.00684503186494112f,0.003175704274326563f,0.002208447316661477f,-0.007172347512096167f,-0.001526405569165945f,-0.001191230840049684f,0.0031038434244692326f,-0.006310776807367802f,-0.0006254760664887726f,-0.0073636253364384174f,0.021423500031232834f};


    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Sample BufC for kernels
    float4 tex[9]; 
    sample_tex(fragCoord, tex, R, iChannel0);
    
    // Apply filters
    float4 id = ident(fragCoord, tex);
    float4 sx = sobel_x(fragCoord, tex);
    float4 sy = sobel_y(fragCoord, tex);
    float4 ll = lap(fragCoord, tex);
    
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
        float dot_product = 0.0f;
        for (int j = 0; j < 16; j++){
            dot_product += x[j]*w1[i*16+j];
        }
        // Add bias then RELU
        l1_out[i] = relu(dot_product+b1[i]);  ;
    }
    
    // Second layer
    float l2_out[4];
    for (int i = 0; i < 4; i++){
        float dp2 = 0.0f;
        for (int j = 0; j < nh; j++){
            dp2 += l1_out[j]*w2[i*nh+j];
        }
        l2_out[i] = dp2; 
    }
    
    // Proposed update
    float4 y = to_float4(l2_out[0], l2_out[1], l2_out[2], l2_out[3]);
    
    // Output as prev state
    fragColor = id*0.1f + to_float4_s(0.5f);
    
    
    // If (noise>0.5f) apply update
    float2 p = to_float2(uv.x/2.0f+_sinf(iTime/1000.0f), uv.y/2.0f+_cosf(iTime/1000.0f));
    if (random_1(p) < 0.5f){
        fragColor = (id + y)*0.1f + to_float4_s(0.5f);
    }
    
    // If (mouse down) paint grey around it
    if(length(fragCoord-swi2(iMouse,x,y)/1.0f)<(20.0f)){
        if (iMouse.z>0.5f){fragColor = to_float4_s(0.5f);}
    }
    
    if (Blend1>0.0) fragColor = Blending(iChannel1, fragCoord/R, fragColor, Blend1, Par1, to_float2(Blend1Mul,Blend1Off), Modus, fragCoord, R);
    
    // Init 
    if (iFrame==0 || Reset)  {fragColor = to_float4_s(0.5f);}

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void NeuralCaDemoJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f);

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Apply zoom (can't figure out how to re-size buffers 
    // so this wastes a lot of compute updating the offscreen parts)
    uv = uv/1.0f;

    // Read the buffer
    float3 col = (swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z)-to_float3_s(0.5f))*10.0f + to_float3_s(0.5f);

    col+=swi3(Color,x,y,z)-0.5f;

    // Output to screen
    fragColor = to_float4_aw(col,Color.w);

  SetFragmentShaderComputedColor(fragColor);
}