
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

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
__DEVICE__ float2 coord (float2 fragCoord, float2 offset, float2 iResolution){
    float x = mod_f(fragCoord.x + offset.x, iResolution.x);
    float y = mod_f(fragCoord.y + offset.y, iResolution.y);
    return to_float2(x, y)/iResolution;
}
__DEVICE__ void sample_tex (float4 tex[9], float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel0){
    
    tex[0] = (texture(iChannel0, coord(fragCoord, to_float2(-1, 1),iResolution))-0.5f)*10.0f;
    tex[1] = (texture(iChannel0, coord(fragCoord, to_float2(0, 1),iResolution))-0.5f)*10.0f;
    tex[2] = (texture(iChannel0, coord(fragCoord, to_float2(1, 1),iResolution))-0.5f)*10.0f;
    tex[3] = (texture(iChannel0, coord(fragCoord, to_float2(-1, 0),iResolution))-0.5f)*10.0f;
    tex[4] = (texture(iChannel0, coord(fragCoord, to_float2(0, 0),iResolution))-0.5f)*10.0f;
    tex[5] = (texture(iChannel0, coord(fragCoord, to_float2(1, 0),iResolution))-0.5f)*10.0f;
    tex[6] = (texture(iChannel0, coord(fragCoord, to_float2(-1, -1),iResolution))-0.5f)*10.0f;
    tex[7] = (texture(iChannel0, coord(fragCoord, to_float2(0, -1),iResolution))-0.5f)*10.0f;
    tex[8] = (texture(iChannel0, coord(fragCoord, to_float2(1, -1),iResolution))-0.5f)*10.0f;

}

// The four kernels used
__DEVICE__ float4 ident(float2 fragCoord, float4 tex[9]){
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


__KERNEL__ void ShiftingScalesJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
  CONNECT_CHECKBOX0(Reset, 0);
  fragCoord+=0.5f;
  
  const int nh = 16;
  float b1[16] = {-0.1275949627161026f,-0.09450257569551468f,-0.07566718012094498f,0.2428344190120697f,-0.09019827097654343f,-0.05062813311815262f,-0.03427153825759888f,0.013352488167583942f,0.07250174880027771f,-0.09471696615219116f,-0.2223607748746872f,0.04483039677143097f,0.18174059689044952f,0.11528093367815018f,-0.0667099729180336f,0.0236106030642986f};
  float w1[304] = {-0.14080430567264557f,-0.026868026703596115f,0.21015068888664246f,0.14671064913272858f,-0.11699232459068298f,-0.11786699295043945f,0.20070892572402954f,0.2560640573501587f,0.20556320250034332f,-0.14395472407341003f,0.2366764098405838f,0.05551886186003685f,0.03964449092745781f,0.10342927277088165f,-0.08135916292667389f,0.04789956286549568f,0.08983530849218369f,-0.06019752100110054f,-0.02222045697271824f,-0.1584351807832718f,0.036516666412353516f,0.22627060115337372f,-0.006398756988346577f,0.05043160170316696f,0.06481527537107468f,0.10419649630784988f,-0.09414298832416534f,-0.3358660638332367f,0.34370407462120056f,0.41617557406425476f,0.1428890824317932f,0.03717514127492905f,0.06943638622760773f,-0.016169335693120956f,-0.04023682326078415f,-0.03176407888531685f,-0.03018992766737938f,-0.05130023881793022f,0.3086848258972168f,0.3177115023136139f,-0.016221003606915474f,0.18932989239692688f,0.18927256762981415f,0.1569887101650238f,-0.12147001922130585f,-0.049278683960437775f,-0.10304568707942963f,0.07707950472831726f,0.10394562780857086f,-0.2779878079891205f,0.09752975404262543f,0.1590166836977005f,-0.004657905548810959f,0.17958007752895355f,-0.06824717670679092f,-0.06842119991779327f,-0.03996706008911133f,0.05098485201597214f,0.03025956265628338f,-0.1632341593503952f,-0.2810683250427246f,-0.13400433957576752f,0.13162942230701447f,0.104570172727108f,0.1501365751028061f,-0.3185262978076935f,-0.1166423112154007f,0.1299605667591095f,0.028189338743686676f,-0.1463458240032196f,0.018531061708927155f,-0.008282226510345936f,0.23955872654914856f,0.06511308252811432f,-0.09634167701005936f,-0.02636609971523285f,0.34102270007133484f,-0.16751731932163239f,0.3470216989517212f,-0.10182174295186996f,0.22196798026561737f,0.08256272226572037f,0.052956532686948776f,-0.060923293232917786f,-0.12438315898180008f,0.000639529840555042f,-0.06618203222751617f,-0.24085642397403717f,0.15398550033569336f,-0.1947477012872696f,0.17027224600315094f,0.10721397399902344f,-0.02300598844885826f,0.010672475211322308f,-0.03747137635946274f,-0.19244442880153656f,-0.1534421443939209f,0.33055248856544495f,0.13359026610851288f,-0.15174594521522522f,0.0746116116642952f,0.21380852162837982f,0.16521291434764862f,-0.18752968311309814f,-0.05387669801712036f,0.15426206588745117f,0.012908702716231346f,0.20546625554561615f,-0.022075554355978966f,-0.28372570872306824f,-0.2946659326553345f,-0.07278662174940109f,-0.08274810761213303f,-0.10622761398553848f,-0.32060861587524414f,0.213771253824234f,-0.04379267990589142f,0.24838753044605255f,0.20393241941928864f,-0.17556995153427124f,-0.14584781229496002f,-0.16978538036346436f,-0.2693498432636261f,0.09818701446056366f,-0.07841885089874268f,-0.015524936839938164f,-0.20069633424282074f,0.008305800147354603f,-0.018933942541480064f,0.07203184068202972f,0.12870629131793976f,0.053401075303554535f,0.11041537672281265f,-0.0169256292283535f,0.08602173626422882f,-0.1149032786488533f,-0.01340780220925808f,0.11959466338157654f,-0.004679994657635689f,-0.020545586943626404f,-0.11498360335826874f,-0.21332599222660065f,-0.1739683896303177f,-0.014313604682683945f,0.07053832709789276f,-0.08312220871448517f,0.13829146325588226f,-0.11021738499403f,-0.13740086555480957f,0.037122942507267f,-0.06782133877277374f,0.058642834424972534f,-0.26944705843925476f,-0.31970471143722534f,0.08807267993688583f,0.10960442572832108f,0.25279316306114197f,0.061417900025844574f,-0.19970622658729553f,0.029310228303074837f,-0.053060851991176605f,0.21827828884124756f,-0.08539487421512604f,0.09981755912303925f,-0.014016379602253437f,0.016156861558556557f,-0.12251466512680054f,-0.1017794981598854f,0.06782016903162003f,0.006950834300369024f,-0.0030113793909549713f,-0.23558641970157623f,0.21568229794502258f,-0.019147882238030434f,0.18582695722579956f,-0.2801423668861389f,0.031641583889722824f,-0.044527262449264526f,-0.20764674246311188f,-0.09051618725061417f,-0.05997084453701973f,0.10215991735458374f,-0.22098036110401154f,-0.26914462447166443f,-0.04677315801382065f,-0.19059371948242188f,-0.1518290638923645f,-0.008095433935523033f,-0.026501990854740143f,-0.022799858823418617f,-0.14077332615852356f,-0.03812664747238159f,-0.1889197826385498f,0.022737829014658928f,-0.3482382595539093f,0.1603982001543045f,-0.14505963027477264f,0.028317855671048164f,-0.06561470031738281f,0.2469213902950287f,-0.23129019141197205f,0.003964403178542852f,0.2004251778125763f,0.09487626701593399f,-0.11152387410402298f,-0.08538051694631577f,0.014660230837762356f,0.014864637516438961f,0.007159452885389328f,-0.24635377526283264f,0.09959244728088379f,0.11900382488965988f,0.11503642052412033f,-0.10549198091030121f,0.12244902551174164f,0.036493703722953796f,0.07206176966428757f,0.3015463054180145f,0.16289839148521423f,-0.082069531083107f,-0.1259652078151703f,0.11022020876407623f,-0.32696908712387085f,-0.03532020375132561f,-0.1741977334022522f,0.08424000442028046f,0.08591797202825546f,0.07468967884778976f,0.1837518960237503f,-0.266082763671875f,0.15360139310359955f,-0.16883525252342224f,0.16093234717845917f,0.08374004065990448f,-0.14539769291877747f,-0.07053764909505844f,-0.318446546792984f,0.16329504549503326f,0.19345572590827942f,-0.07228654623031616f,0.26070523262023926f,-0.34841880202293396f,-0.2550112009048462f,-0.08552594482898712f,-0.007146872580051422f,0.0812193900346756f,0.04666871577501297f,-0.09488335996866226f,0.07157552987337112f,-0.10581283271312714f,0.15044979751110077f,0.29023751616477966f,0.1963278353214264f,0.0919988676905632f,-0.14453719556331635f,-0.0524955652654171f,-0.08844055235385895f,-0.016712214797735214f,0.10381287336349487f,0.24415938556194305f,0.024218924343585968f,0.025793403387069702f,-0.1749105006456375f,0.1544051617383957f,0.1833929568529129f,0.14494647085666656f,-0.4505853056907654f,0.06719965487718582f,0.46406736969947815f,-0.1733226627111435f,-0.2336266040802002f,0.17739206552505493f,-0.11394678056240082f,-0.0523369126021862f,-0.1315702497959137f,0.09753775596618652f,0.07248570770025253f,-0.08913727849721909f,0.0829273909330368f,-0.06094783917069435f,0.1144513338804245f,-0.1591494381427765f,0.08176036179065704f,0.10808447748422623f,0.08260544389486313f,-0.2009381651878357f,-0.2083185762166977f,0.08112384378910065f,0.05698838084936142f,0.19468837976455688f,0.1241222620010376f,-0.008474493399262428f,-0.0441143736243248f,0.2414262741804123f,-0.04003392904996872f,-0.1670873761177063f,-0.011282357387244701f,-0.12079014629125595f,0.11966439336538315f,0.10974006354808807f,-0.16521187126636505f,0.030639903619885445f,-0.0009862212464213371f,-0.02830863930284977f};
  float w2[64] = {0.05477364733815193f,0.08509646356105804f,-0.010731047950685024f,-0.09784814715385437f,-0.0999220609664917f,0.08297448605298996f,0.06579071283340454f,-0.013713710010051727f,0.008191213011741638f,0.04046080261468887f,0.05600346624851227f,-0.03470642864704132f,-0.10811358690261841f,-0.07301194220781326f,0.042982764542102814f,0.009564274922013283f,0.028923559933900833f,0.05724873021245003f,-0.05696067586541176f,-0.00801454484462738f,-0.027924543246626854f,0.06163661181926727f,-0.037624940276145935f,-0.014055940322577953f,-0.05337720364332199f,0.016334151849150658f,0.10578610748052597f,-0.03077259659767151f,-0.03537117689847946f,-0.1385769248008728f,0.04417937248945236f,0.052174944430589676f,-0.005542446859180927f,0.20165960490703583f,-0.05577676370739937f,0.01303199864923954f,-0.0012016163673251867f,-0.029802773147821426f,0.015513886697590351f,0.01910318434238434f,0.013644350692629814f,-0.08286455273628235f,0.013271462172269821f,-0.1548565924167633f,0.021827412769198418f,-0.03458331152796745f,0.05645303055644035f,0.013120475225150585f,0.026858780533075333f,0.03326160088181496f,0.05996105074882507f,0.016673076897859573f,0.08056259900331497f,-0.10825452208518982f,-0.0020626832265406847f,0.005089879035949707f,0.03821643441915512f,0.07546956092119217f,-0.022499529644846916f,-0.045474205166101456f,0.041259653866291046f,-0.09594440460205078f,-0.05013250187039375f,-0.0049316855147480965f};
  
  
    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Init 
    if (iFrame==0 || Reset) {fragColor = to_float4_s(0.5f);}
    
    if (uv.x< 0.55f){ // ignore half the screen since we zoom anyway
    
        // If (noise>0.5f) apply update
        float2 p = to_float2(uv.x/2.0f+_sinf(iTime/1000.0f), uv.y/2.0f+_cosf(iTime/1000.0f));
        float pp = random_1(p);
        if (uv.y > 0.51f){pp += 0.4f;} // updates less likely for offscreen
        if (pp < 0.5f){
            
             // Sample BufC for kernels
            float4 tex[9];
            sample_tex(tex,fragCoord, iResolution, iChannel0);

            // Apply filters
            float4 id = ident(fragCoord, tex);
            float4 sx = sobel_x(fragCoord, tex);
            float4 sy = sobel_y(fragCoord, tex);
            float4 ll = lap(fragCoord, tex);
            
            // Sample webcam
            float2 cc = fragCoord*2.0f;
            float4 img = texture(iChannel1,cc/iResolution);

            // Create x (4 channels x 4 filters, per channel conv)
            // + 5 rules
            float x[19];
            x[0] = id.x;x[1] = sx.x;x[2] = sy.x;x[3] = ll.x;
            x[4] = id.y;x[5] = sx.y;x[6] = sy.y;x[7] = ll.y;
            x[8] = id.z;x[9] = sx.z;x[10] = sy.z;x[11] = ll.z;
            x[12] = id.w;x[13] = sx.w;x[14] = sy.w;x[15] = ll.w;
            x[16]=img.x*2.0f-0.3f;x[17]=img.y*2.0f-0.3f;x[18]=img.z*2.0f-0.3f;//x[19]=0.0f;x[20]=0.0f;


            // First layer 
            float l1_out[nh];
            for (int i = 0; i < nh; i++){
                // Dot Product equivalent to:
                // dot_product = x @ w1_i
                float dot_product = 0.0f;
                for (int j = 0; j < 16+3; j++){
                    dot_product += x[j]*w1[i*(16+3)+j];
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
        
            fragColor = (id + y)*0.1f + to_float4_s(0.5f);
        }
        else{
            // Output as prev state
            fragColor = texture(iChannel0, coord(fragCoord, to_float2(0, 0),iResolution));
        }
        
        // Init 
        if (iFrame==0 || Reset) {fragColor = to_float4_s(0.5f);}

        // If (mouse down) paint grey around it
        if(length(fragCoord-swi2(iMouse,x,y)/2.0f)<(20.0f)){
            if (iMouse.z>0.5f) {fragColor = to_float4_s(0.5f);}
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------


__DEVICE__ float colormap_red(float x) {
    if (x < 0.0f) {
        return 54.0f / 255.0f;
    } else if (x < 20049.0f / 82979.0f) {
        return (829.79f * x + 54.51f) / 255.0f;
    } else {
        return 1.0f;
    }
}

__DEVICE__ float colormap_green(float x) {
    if (x < 20049.0f / 82979.0f) {
        return 0.0f;
    } else if (x < 327013.0f / 810990.0f) {
        return (8546482679670.0f / 10875673217.0f * x - 2064961390770.0f / 10875673217.0f) / 255.0f;
    } else if (x <= 1.0f) {
        return (103806720.0f / 483977.0f * x + 19607415.0f / 483977.0f) / 255.0f;
    } else {
        return 1.0f;
    }
}

__DEVICE__ float colormap_blue(float x) {
    if (x < 0.0f) {
        return 54.0f / 255.0f;
    } else if (x < 7249.0f / 82979.0f) {
        return (829.79f * x + 54.51f) / 255.0f;
    } else if (x < 20049.0f / 82979.0f) {
        return 127.0f / 255.0f;
    } else if (x < 327013.0f / 810990.0f) {
        return (792.02249341361393720147485376583f * x - 64.364790735602331034989206222672f) / 255.0f;
    } else {
        return 1.0f;
    }
}

__DEVICE__ float4 colormap(float x) {
    return to_float4(colormap_green(x), colormap_red(x), colormap_blue(x), 1.0f);
}

// https://iquilezles.org/articles/warp
/*float noise( in float2 x )
{
    float2 p = _floor(x);
    float2 f = fract(x);
    f = f*f*(3.0f-2.0f*f);
    float a = textureLod(iChannel0,(p+to_float2(0.5f,0.5f))/256.0f,0.0f).x;
  float b = textureLod(iChannel0,(p+to_float2(1.5f,0.5f))/256.0f,0.0f).x;
  float c = textureLod(iChannel0,(p+to_float2(0.5f,1.5f))/256.0f,0.0f).x;
  float d = textureLod(iChannel0,(p+to_float2(1.5f,1.5f))/256.0f,0.0f).x;
    return _mix(mix( a, b,f.x), _mix( c, d,f.x),f.y);
}*/


__DEVICE__ float rand(float2 n) { 
    return fract(_sinf(dot(n, to_float2(12.9898f, 4.1414f))) * 43758.5453f);
}

__DEVICE__ float noise(float2 p){
    float2 ip = _floor(p);
    float2 u = fract_f2(p);
    u = u*u*(3.0f-2.0f*u);

    float res = _mix(
                _mix(rand(ip),rand(ip+to_float2(1.0f,0.0f)),u.x),
                _mix(rand(ip+to_float2(0.0f,1.0f)),rand(ip+to_float2(1.0f,1.0f)),u.x),u.y);
    return res*res;
}



__DEVICE__ float fbm( float2 p, float iTime )
{
    const mat2 mtx = to_mat2( 0.80f,  0.60f, -0.60f,  0.80f );
    float f = 0.0f;

    f += 0.500000f*noise( p + iTime/2.0f  ); p = mul_mat2_f2(mtx,p*2.02f);
    f += 0.031250f*noise( p ); p = mul_mat2_f2(mtx,p*2.01f);
    f += 0.250000f*noise( p ); p = mul_mat2_f2(mtx,p*2.03f);
    f += 0.125000f*noise( p ); p = mul_mat2_f2(mtx,p*2.01f);
    f += 0.062500f*noise( p ); p = mul_mat2_f2(mtx,p*2.04f);
    f += 0.015625f*noise( p + _sinf(iTime) );

    return f/0.96875f;
}

__DEVICE__ float pattern( in float2 p, float iTime )
{
  float zzzzzzzzzzzz;
  return fbm( p + fbm( p + fbm( p,iTime ), iTime ),iTime );
}

__KERNEL__ void ShiftingScalesJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
float BBBBBBBBBBBBB;
  float2 uv = fragCoord/iResolution.x;
  float shade = pattern(uv, iTime);
  fragColor = to_float4_aw(swi3(colormap(shade),x,y,z), shade);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1


__KERNEL__ void ShiftingScalesJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution;
    
    // Apply zoom (can't figure out how to re-size buffers 
    // so this wastes a lot of compute updating the offscreen parts)
    uv = uv/2.0f;

    // Read the buffer
    float3 col = (swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z)-to_float3_s(0.5f))*10.0f + to_float3_s(0.5f);

    // Output to screen
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}