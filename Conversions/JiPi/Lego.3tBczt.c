
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


#define STORAGE iChannel0
const int2 resolutionDataLocation = {0, 0};
const int2 rotationDataLocation   = {1, 0};
const int2 brickSizeDataLocation  = {2, 0};
const int2 brickColorDataLocation = {3, 0};

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#ifdef xxx
const int KEY_SHIFT = 16; //0
const int KEY_SPACE = 32; //1
const int KEY_LEFT  = 37; //2
const int KEY_UP    = 38; //3
const int KEY_RIGHT = 39; //4
const int KEY_DOWN  = 40; //5
const int KEY_C     = 67; //6
#endif

#define KEY_SHIFT 0 
#define KEY_SPACE 1 
#define KEY_LEFT  2 
#define KEY_UP    3 
#define KEY_RIGHT 4 
#define KEY_DOWN  5 
#define KEY_C     6 


__DEVICE__ bool saveResolutionData(in int2 fragCoord, out float4 *fragColor, float2 R, __TEXTURE2D__ STORAGE) {
  if (fragCoord.x == resolutionDataLocation.x && fragCoord.y == resolutionDataLocation.y ) {
    //float4 previousResolutionData = texelFetch(STORAGE, fragCoord, 0);
    float4 previousResolutionData = texture(STORAGE, (make_float2(fragCoord)+0.5f)/R);
    float2 oldResolution = swi2(previousResolutionData,x,y);
    *fragColor = to_float4_f2f2(iResolution, oldResolution);

    return true;
  }

  return false;
}

#ifdef xxx
__DEVICE__ bool isKeyUp(in int key) {
  return texelFetch(iChannel1, to_int2(key, 1), 0).x > 0.0f;
}

__DEVICE__ bool isKeyToggled(in int key) {
  return texelFetch(iChannel1, to_int2(key, 2), 0).x > 0.0f;
}
#endif

__DEVICE__ bool saveRotationData(in int2 fragCoord, out float4 *fragColor, float2 R, int iFrame, float4 iMouse, bool KEY[6], __TEXTURE2D__ STORAGE) {
  if (fragCoord.x == rotationDataLocation.x && fragCoord.y == rotationDataLocation.y) {
    if (iFrame == 0) {
      *fragColor = to_float4(0.0f, 0.68f, 0.0f, 0.0f);
    } else {
      
float zzzzzzzzzzzzzzzzzzzzzzzz;      
      //float2 rotation = texelFetch(STORAGE, rotationDataLocation, 0).xy;
      float2 rotation = swi2(texture(STORAGE, (make_float2(rotationDataLocation)+0.5f)/R),x,y);
      rotation.x += KEY[KEY_SPACE] ? 0.0f : 0.003f;
      rotation.y = iMouse.x > 0.0f 
                    ? _mix(rotation.y, -iMouse.y / iResolution.y * 8.0f - 1.0f, 0.03f) 
                    : rotation.y;
      *fragColor = to_float4(rotation.x, rotation.y, 0.0f, 0.0f);
    }
    return true;
  }
    
  return false;
}


__DEVICE__ bool saveBrickSizeData(in int2 fragCoord, out float4 *fragColor, float2 R, int iFrame, bool KEY[6], __TEXTURE2D__ STORAGE) {
  if (fragCoord.x == brickSizeDataLocation.x && fragCoord.y == brickSizeDataLocation.y) {
    if (iFrame == 0) {
      *fragColor = to_float4_aw(to_float3(1.0f, 0.6f, 1.5f), 0.0f);
    } else {
      //float3 size = texelFetch(STORAGE, brickSizeDataLocation, 0).rgb;
      float3 size = swi3(texture(STORAGE, (make_float2(brickSizeDataLocation)+0.5f)/R),x,y,z);
      if (KEY[KEY_DOWN]) {
        size.z -= 0.5f;
      }
      if (KEY[KEY_UP]) {
        size.z += 0.5f;
      }
      if (KEY[KEY_LEFT]) {
        size.x -= 0.5f;
      }
      if (KEY[KEY_RIGHT]) {
        size.x += 0.5f;
      }
      swi2S(size,x,z, clamp(swi2(size,x,z), 0.5f, 8.0f));
      size.y = KEY[KEY_SHIFT] ? 0.2f : 0.6f;
        
      *fragColor = to_float4_aw(size, 0.0f);
    }
    return true;
  }
    
  return false;
}



__DEVICE__ float3 toLinear(in float3 color) { return pow_f3(color, to_float3_s(2.2f)); }

__DEVICE__ bool saveBrickColorData(in int2 fragCoord, out float4 *fragColor, float2 R, int iFrame, bool KEY[6], __TEXTURE2D__ STORAGE) {
  
  const int colorsLength = 12;
  const float3 colors[] = {
  to_float3(0.949f, 0.803f, 0.215f), // Bright Yellow
  to_float3(0.996f, 0.541f, 0.094f), // Bright Orange
  to_float3(0.788f, 0.101f, 0.035f), // Bright Red
  to_float3(0.784f, 0.439f, 0.627f), // Bright Purple
  to_float3(0.0f  , 0.333f, 0.749f), // Bright Blue
  to_float3(0.039f, 0.203f, 0.388f), // Earth Blue
  to_float3(0.027f, 0.545f, 0.788f), // Dark Azur
  to_float3(0.137f, 0.470f, 0.254f), // Dark Green
  to_float3(0.294f, 0.623f, 0.290f), // Bright Green
  to_float3(0.733f, 0.913f, 0.043f), // Bright Yellowish Green
  to_float3(0.345f, 0.164f, 0.070f), // Reddish Brown
  to_float3(0.019f, 0.074f, 0.113f)  // Black
  };
 float fffffffffffffffffffffffffffffffffffffffffff; 
 
  if (fragCoord.x == brickColorDataLocation.x && fragCoord.y == brickColorDataLocation.y) {
    if (iFrame == 0) {
      int index = 2;
      *fragColor = to_float4_aw(toLinear(colors[index]), (float)(index));
    } else {
      //float4 color = texelFetch(STORAGE, brickColorDataLocation, 0);
      float4 color = texture(STORAGE, (make_float2(brickColorDataLocation)+0.5f)/R);
      if (KEY[KEY_C]) {
        color.w += 1.0f;
      }
      color.w = (float)((int)(color.w) % colorsLength);
      swi3S(color,x,y,z, _mix(swi3(color,x,y,z), toLinear(colors[(int)(color.w)]), 0.15f));
        
      *fragColor = color;
    }
    return true;
  }
    
  return false;
}


__KERNEL__ void LegoFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel1)
{
  fragCoord+=0.5f;

  CONNECT_CHECKBOX0(SHIFT, 0);
  CONNECT_CHECKBOX1(SPACE, 0);
  CONNECT_CHECKBOX2(LEFT, 0);
  CONNECT_CHECKBOX3(RIGHT, 0);
  CONNECT_CHECKBOX4(DOWN, 0);
  CONNECT_CHECKBOX5(C, 0);
  
float AAAAAAAAAAAAAAAAAAAAA;
  bool KEY[6] = { SHIFT, SPACE, LEFT, RIGHT, DOWN, C}; 


  fragColor = to_float4_s(0.0f);
  int2 iFragCoord = to_int2_cfloat(fragCoord);  
    
  if (saveResolutionData(iFragCoord, &fragColor,R,STORAGE)) {
    SetFragmentShaderComputedColor(fragColor);
    return;
  }
    
  if (saveRotationData(iFragCoord, &fragColor,R,iFrame,iMouse,KEY,STORAGE)) {
    SetFragmentShaderComputedColor(fragColor);
    return;
  }
    
  if (saveBrickSizeData(iFragCoord, &fragColor,R,iFrame,KEY,STORAGE)) {
    SetFragmentShaderComputedColor(fragColor);
    return;
  }
    
  if (saveBrickColorData(iFragCoord, &fragColor,R,iFrame,KEY,STORAGE)) {
    SetFragmentShaderComputedColor(fragColor);  
    return;
  }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1




__DEVICE__ bool sameSide(float3 p1, float3 p2, float3 a, float3 b) {
  float3 cp1 = cross(b - a, p1 - a);
  float3 cp2 = cross(b - a, p2 - a);

  return dot(cp1, cp2) >= 0.0f;
}

__DEVICE__ bool pointInTriangle(float3 p, float3 a, float3 b, float3 c) {
  return sameSide(p, a, b, c) && sameSide(p, b, a, c) && sameSide(p, c, a, b);
}




//----------------------------------------------------------------------------------------------------------------------------------------------------------------
__DEVICE__ bool inPath(float2 p) {
  
   // Created with Shadertoy-SVG: https://zduny.github.io/shadertoy-svg/



const float3 positions[225] =
              {
              to_float3(0.9408613367791228f, 0.43095909706765456f, 0),
              to_float3(0.9264762435267317f, 0.44352663509707624f, 0),
              to_float3(0.8947861254762883f, 0.4638692865687953f, 0),
              to_float3(0.8777536433400679f, 0.4715814511436973f, 0),
              to_float3(0.841995189500208f, 0.48193008580309815f, 0),
              to_float3(0.8048872251098951f, 0.48530129825064333f, 0),
              to_float3(0.7861678547856077f, 0.4842916850607259f, 0),
              to_float3(0.7490796945333518f, 0.47672464768502254f, 0),
              to_float3(0.7309834472672143f, 0.47010427463184123f, 0),
              to_float3(0.6967507470536414f, 0.45128546857518964f, 0),
              to_float3(0.6815966127536668f, 0.4397110894047105f, 0),
              to_float3(0.6551836294986841f, 0.41262743744267205f, 0),
              to_float3(0.634461035245403f, 0.3809322504759403f, 0),
              to_float3(0.626419887197555f, 0.3635934525495947f, 0),
              to_float3(0.6174678863278233f, 0.3359891364381042f, 0),
              to_float3(0.47899216991926896f, -0.24549629059456585f, 0),
              to_float3(0.4741737266143069f, -0.2751084016236367f, 0),
              to_float3(0.473508033500468f, -0.29441126217125624f, 0),
              to_float3(0.4778079185647557f, -0.33209980412273227f, 0),
              to_float3(0.4891422806195873f, -0.36776620212046324f, 0),
              to_float3(0.5069424579864692f, -0.40045137004997605f, 0),
              to_float3(0.5306397889869092f, -0.42919622179679773f, 0),
              to_float3(0.5445221803251843f, -0.44179131469092675f, 0),
              to_float3(0.575999001128787f, -0.4628274056990749f, 0),
              to_float3(0.5934512651744912f, -0.471028632284476f, 0),
              to_float3(0.6313136680076146f, -0.48217771366078943f, 0),
              to_float3(0.6700652677983625f, -0.48548096726553785f, 0),
              to_float3(0.7077538097498388f, -0.48118108220125f, 0),
              to_float3(0.7434202077475696f, -0.46984672014641854f, 0),
              to_float3(0.7761053756770824f, -0.45204654277953654f, 0),
              to_float3(0.8048502274239044f, -0.4283492117790967f, 0),
              to_float3(0.8286956768735618f, -0.3993233888235917f, 0),
              to_float3(0.8466826379115826f, -0.36553773559151465f, 0),
              to_float3(0.8557512754292231f, -0.33741694258942456f, 0),
              to_float3(0.996423388384482f, 0.25472647096242323f, 0),
              to_float3(1, 0.29294405649279703f, 0),
              to_float3(0.9960265197448488f, 0.3307089760104401f, 0),
              to_float3(0.9846915584576656f, 0.3670555420215273f, 0),
              to_float3(0.7570787710671805f, -0.3279246723576778f, 0),
              to_float3(0.7481456607517563f, -0.34489128805259295f, 0),
              to_float3(0.7363053501222789f, -0.3595089915158228f, 0),
              to_float3(0.7220133343540593f, -0.37147883456812697f, 0),
              to_float3(0.705725108622407f, -0.38050186903026445f, 0),
              to_float3(0.6878961681026325f, -0.3862791467229943f, 0),
              to_float3(0.6689820079700466f, -0.3885117194670758f, 0),
              to_float3(0.649438123399958f, -0.38690063908326805f, 0),
              to_float3(0.6304205367457778f, -0.3813442914822551f, 0),
              to_float3(0.6058327710290847f, -0.3668259574428918f, 0),
              to_float3(0.5925016292489125f, -0.3537028573471296f, 0),
              to_float3(0.5819678219692868f, -0.33835579970027974f, 0),
              to_float3(0.5720660623804614f, -0.31216168851770726f, 0),
              to_float3(0.5701398889260951f, -0.28352583320630764f, 0),
              to_float3(0.7155770729851352f, 0.3272211318267263f, 0),
              to_float3(0.7245101833005594f, 0.3441877475216413f, 0),
              to_float3(0.7363504939300369f, 0.35880545098487127f, 0),
              to_float3(0.7506425096982563f, 0.3707752940371754f, 0),
              to_float3(0.7669307354299086f, 0.37979832849931283f, 0),
              to_float3(0.7847596759496831f, 0.38557560619204273f, 0),
              to_float3(0.803673836082269f, 0.38780817893612424f, 0),
              to_float3(0.8330139122006925f, 0.38387499214489884f, 0),
              to_float3(0.8511765563761193f, 0.3765710374186516f, 0),
              to_float3(0.8670548875320945f, 0.36613614277101764f, 0),
              to_float3(0.8803725907900135f, 0.35302297421472734f, 0),
              to_float3(0.8908533512712726f, 0.3376841977625116f, 0),
              to_float3(0.8982208540972669f, 0.3205724794271009f, 0),
              to_float3(0.9021987843893928f, 0.3021404852212259f, 0),
              to_float3(0.49404521200236484f, 0.32051485979824523f, 0),
              to_float3(0.48453276188440575f, 0.3577040643779089f, 0),
              to_float3(0.4771903875232675f, 0.3751107130887582f, 0),
              to_float3(0.4578154766529401f, 0.40706596689622404f, 0),
              to_float3(0.432829864860236f, 0.4345676472763559f, 0),
              to_float3(0.4030049704751837f, 0.4568443358991251f, 0),
              to_float3(0.3691122118278114f, 0.47312461443450343f, 0),
              to_float3(0.3508814516338892f, 0.4787750316912871f, 0),
              to_float3(0.3319230072481476f, 0.4826370645524623f, 0),
              to_float3(0.29392752080375106f, 0.4846864612988684f, 0),
              to_float3(0.2612007307844826f, 0.48048830820567073f, 0),
              to_float3(0.2302822471806536f, 0.47103805054705644f, 0),
              to_float3(0.20168320536497242f, 0.4567572335473808f, 0),
              to_float3(0.17591474071014757f, 0.43806740243099923f, 0),
              to_float3(0.15348798858888713f, 0.41539010242226715f, 0),
              to_float3(0.13491408437389962f, 0.3891468787455399f, 0),
              to_float3(0.12070416343789314f, 0.35975927662517293f, 0),
              to_float3(0.1132180666353646f, 0.33590866248028545f, 0),
              to_float3(-0.025338065404089227f, -0.24549634892148534f, 0),
              to_float3(-0.030138663741522187f, -0.27510780484715514f, 0),
              to_float3(-0.03077473905960626f, -0.29440849931717467f, 0),
              to_float3(-0.026382729388821247f, -0.33208369115772834f, 0),
              to_float3(-0.021501053563444672f, -0.35021629512770946f, 0),
              to_float3(-0.0067323828414551645f, -0.3844667856656981f, 0),
              to_float3(0.01422201609720819f, -0.4152246749992118f, 0),
              to_float3(0.026835852193426657f, -0.4289915274635997f, 0),
              to_float3(0.05597077473090595f, -0.45269631448625475f, 0),
              to_float3(0.07234545200867437f, -0.46239235564396874f, 0),
              to_float3(0.10834321567353711f, -0.47686699975078806f, 0),
              to_float3(0.12770761787487328f, -0.4814513703741509f, 0),
              to_float3(0.16645647691437215f, -0.4847091805549651f, 0),
              to_float3(0.20413166875492594f, -0.4803171708841801f, 0),
              to_float3(0.23976561979432254f, -0.46886097801576615f, 0),
              to_float3(0.27239075643035005f, -0.45092623860369346f, 0),
              to_float3(0.30103950506079724f, -0.4270985893019323f, 0),
              to_float3(0.31357036712323705f, -0.41315793489678415f, 0),
              to_float3(0.3344403332411665f, -0.38158898948668446f, 0),
              to_float3(0.3489149773479858f, -0.3455912258218219f, 0),
              to_float3(0.42894164996584827f, -0.011973834455500498f, 0),
              to_float3(0.42972497189919556f, 0.005285807701407088f, 0),
              to_float3(0.4243979338193449f, 0.022557766661810377f, 0),
              to_float3(0.410058067874302f, 0.03884635586226634f, 0),
              to_float3(0.3951169066462894f, 0.04640446369357787f, 0),
              to_float3(0.381196346171893f, 0.04853615910943173f, 0),
              to_float3(0.29306584924039014f, 0.048288253738397215f, 0),
              to_float3(0.274830481833795f, 0.04272753468825939f, 0),
              to_float3(0.2605446253882744f, 0.030976607914030365f, 0),
              to_float3(0.25170207774583986f, 0.014529271257721282f, 0),
              to_float3(0.24954095118037345f, 0.000010362544709030003f, 0),
              to_float3(0.25170207774583986f, -0.014508546168303221f, 0),
              to_float3(0.2605446253882744f, -0.030955882824612358f, 0),
              to_float3(0.274830481833795f, -0.042706809598841385f, 0),
              to_float3(0.283547839032084f, -0.04635430745454747f, 0),
              to_float3(0.31863004442757803f, -0.048515434020013674f, 0),
              to_float3(0.25202421179653345f, -0.32792467235767736f, 0),
              to_float3(0.2430911014811088f, -0.3448912880525925f, 0),
              to_float3(0.22438277766140802f, -0.36584357985454286f, 0),
              to_float3(0.20067054935175999f, -0.380501869030264f, 0),
              to_float3(0.18284160883198575f, -0.38627914672299396f, 0),
              to_float3(0.15420575352058608f, -0.3882053201773602f, 0),
              to_float3(0.13463670610345768f, -0.3845783042799128f, 0),
              to_float3(0.11660773985991546f, -0.37726960481225597f, 0),
              to_float3(0.09378165831698615f, -0.3605708705373532f, 0),
              to_float3(0.07691326269863996f, -0.3383557997002793f, 0),
              to_float3(0.06947573809880092f, -0.3212402796776524f, 0),
              to_float3(0.0647789303657329f, -0.29324752838512047f, 0),
              to_float3(0.06639001074954098f, -0.2737036438150323f, 0),
              to_float3(0.20590410104503487f, 0.3127127704626628f, 0),
              to_float3(0.21430798481922464f, 0.33602689053214585f, 0),
              to_float3(0.2307763633722193f, 0.3590033597484454f, 0),
              to_float3(0.24497227602406446f, 0.37110352110207073f, 0),
              to_float3(0.2612004704622315f, 0.3802611176093988f, 0),
              to_float3(0.288338328393416f, 0.3877891213826372f, 0),
              to_float3(0.31743891786417455f, 0.3869192865064165f, 0),
              to_float3(0.34946754816430725f, 0.37512846657511933f, 0),
              to_float3(0.3628577585368089f, 0.365782121700417f, 0),
              to_float3(0.37946114119164776f, 0.34811165849076364f, 0),
              to_float3(0.3938616180155652f, 0.3191469867055984f, 0),
              to_float3(0.3981023608114145f, 0.2852790298923329f, 0),
              to_float3(0.40366307986155214f, 0.26704366248573774f, 0),
              to_float3(0.40885812964828094f, 0.2593136830277175f, 0),
              to_float3(0.42314398609380155f, 0.24756275625348856f, 0),
              to_float3(0.4463802520051028f, 0.24175413183231625f, 0),
              to_float3(0.470219090251927f, 0.2480492150414748f, 0),
              to_float3(0.4849219338265358f, 0.2599629758397277f, 0),
              to_float3(0.4902778529410525f, 0.2677196751665549f, 0),
              to_float3(-0.4238290609360067f, -0.3879545880996168f, 0),
              to_float3(-0.42399143141451245f, -0.3857869964555996f, 0),
              to_float3(-0.3435303791941341f, -0.04876145696646751f, 0),
              to_float3(-0.1881453922760149f, -0.04826752864897948f, 0),
              to_float3(-0.17862738206770856f, -0.04635430745454769f, 0),
              to_float3(-0.16218004541139952f, -0.037511759812112966f, 0),
              to_float3(-0.15042911863717046f, -0.023225903366592502f, 0),
              to_float3(-0.14678162078146462f, -0.014508546168303545f, 0),
              to_float3(-0.1448683995870328f, 0.005011261049414758f, 0),
              to_float3(-0.15042911863717046f, 0.02324662845600997f, 0),
              to_float3(-0.16218004541139952f, 0.037532484901530376f, 0),
              to_float3(-0.17862738206770856f, 0.04637503254396521f, 0),
              to_float3(-0.19314629078072088f, 0.04853615910943141f, 0),
              to_float3(-0.3201711275962441f, 0.04853615910943141f, 0),
              to_float3(-0.2395580256922737f, 0.3877292902425807f, 0),
              to_float3(-0.013614947063175453f, 0.3879753131890342f, 0),
              to_float3(0.001553809488533675f, 0.3901364397545005f, 0),
              to_float3(0.010271166686822708f, 0.3937839376102065f, 0),
              to_float3(0.024557023132343314f, 0.4055348643844355f, 0),
              to_float3(0.03339957077477784f, 0.42198220104074474f, 0),
              to_float3(0.035560697340244474f, 0.436501109753757f, 0),
              to_float3(0.03339957077477784f, 0.45102001846676926f, 0),
              to_float3(0.024557023132343314f, 0.46746735512307847f, 0),
              to_float3(0.010271166686822708f, 0.4792182818973075f, 0),
              to_float3(0.001553809488533675f, 0.4828657797530134f, 0),
              to_float3(-0.012965099224478305f, 0.4850269063184797f, 0),
              to_float3(-0.28168609287903024f, 0.48485067306612756f, 0),
              to_float3(-0.3043423804615203f, 0.47698065006041873f, 0),
              to_float3(-0.32039964632717177f, 0.4593075787165124f, 0),
              to_float3(-0.5323543124694348f, -0.42474224806260563f, 0),
              to_float3(-0.5337338883980041f, -0.43998512769155146f, 0),
              to_float3(-0.5289140955923151f, -0.45796681352017904f, 0),
              to_float3(-0.5177230286574426f, -0.4726176319384635f, 0),
              to_float3(-0.5012621748910309f, -0.48217218549675583f, 0),
              to_float3(-0.48805568326353965f, -0.4848137981736519f, 0),
              to_float3(-0.18106368398796246f, -0.4846778019002089f, 0),
              to_float3(-0.16282831658136732f, -0.47911708285007126f, 0),
              to_float3(-0.1485424601358467f, -0.46736615607584225f, 0),
              to_float3(-0.13969991249341207f, -0.4509188194195329f, 0),
              to_float3(-0.13780384309711902f, -0.4313999159620832f, 0),
              to_float3(-0.14369958214938683f, -0.41318230940945977f, 0),
              to_float3(-0.1559911147398615f, -0.3989255304682293f, 0),
              to_float3(-0.17289041011825634f, -0.3901082881133118f, 0),
              to_float3(-0.8900128517657221f, -0.3879545880996168f, 0),
              to_float3(-0.6958469474912434f, 0.43024439706208367f, 0),
              to_float3(-0.6970680454957241f, 0.44937401560737006f, 0),
              to_float3(-0.7052111612782105f, 0.46615542921161646f, 0),
              to_float3(-0.711534261055692f, 0.4730785079255896f, 0),
              to_float3(-0.7279951148221036f, 0.4828153067067156f, 0),
              to_float3(-0.7474699697899465f, 0.48548096726553785f, 0),
              to_float3(-0.756841335071212f, 0.48394209929187737f, 0),
              to_float3(-0.7735159477880796f, 0.4757893172039107f, 0),
              to_float3(-0.7858110168317786f, 0.461806574093651f, 0),
              to_float3(-0.789707952521583f, 0.45298025484212406f, 0),
              to_float3(-0.9986204823983503f, -0.42409851472697496f, 0),
              to_float3(-1, -0.4393413360290013f, 0),
              to_float3(-0.9951802071943109f, -0.4573230218576287f, 0),
              to_float3(-0.9839891402594383f, -0.4719738402759131f, 0),
              to_float3(-0.9675282864930268f, -0.48152839383420565f, 0),
              to_float3(-0.9543217948655356f, -0.48417000651110154f, 0),
              to_float3(-0.6473297955899583f, -0.48403401023765885f, 0),
              to_float3(-0.629094428183363f, -0.4784732911875213f, 0),
              to_float3(-0.6148085717378426f, -0.4667223644132922f, 0),
              to_float3(-0.6059660240954079f, -0.4502750277569829f, 0),
              to_float3(-0.6040688937631473f, -0.43076335438168084f, 0),
              to_float3(-0.6099273821747845f, -0.41268783466083125f, 0),
              to_float3(-0.6220949031388547f, -0.3986636757539205f, 0),
              to_float3(-0.6387381593037988f, -0.3900488756991408f, 0),
              to_float3(-0.6530549597150334f, -0.3879545880996168f, 0),
              to_float3(-0.1875131137334065f, -0.3879545880996168f, 0),
              to_float3(0.4962741058527447f, 0.2909237200595889f, 0),
              to_float3(0.9001729001451884f, 0.2680691160390561f, 0),
              to_float3(0.9714728267726251f, 0.392803750658161f, 0)};
const int3 triangles[219] = {
    to_int3(0, 61, 1), to_int3(0, 62, 61), to_int3(0, 224, 62), to_int3(1, 60, 2),
    to_int3(1, 61, 60), to_int3(2, 60, 3), to_int3(3, 59, 4), to_int3(3, 60, 59),
    to_int3(4, 58, 5), to_int3(4, 59, 58), to_int3(5, 58, 6), to_int3(6, 57, 7),
    to_int3(6, 58, 57), to_int3(7, 56, 8), to_int3(7, 57, 56), to_int3(8, 56, 9),
    to_int3(9, 55, 10), to_int3(9, 56, 55), to_int3(10, 54, 11), to_int3(10, 55, 54),
    to_int3(11, 53, 12), to_int3(11, 54, 53), to_int3(12, 52, 13), to_int3(12, 53, 52),
    to_int3(13, 52, 14), to_int3(14, 52, 15), to_int3(15, 51, 16), to_int3(15, 52, 51),
    to_int3(16, 51, 17), to_int3(17, 50, 18), to_int3(17, 51, 50), to_int3(18, 50, 19),
    to_int3(19, 49, 20), to_int3(19, 50, 49), to_int3(20, 48, 21), to_int3(20, 49, 48),
    to_int3(21, 47, 22), to_int3(21, 48, 47), to_int3(22, 47, 23), to_int3(23, 46, 24),
    to_int3(23, 47, 46), to_int3(24, 46, 25), to_int3(25, 45, 26), to_int3(25, 46, 45),
    to_int3(26, 44, 27), to_int3(26, 45, 44), to_int3(27, 43, 28), to_int3(27, 44, 43),
    to_int3(28, 42, 29), to_int3(28, 43, 42), to_int3(29, 41, 30), to_int3(29, 42, 41),
    to_int3(30, 40, 31), to_int3(30, 41, 40), to_int3(31, 39, 32), to_int3(31, 40, 39),
    to_int3(32, 38, 33), to_int3(32, 39, 38), to_int3(33, 38, 223),
    to_int3(33, 223, 34), to_int3(34, 223, 35), to_int3(35, 65, 36),
    to_int3(35, 223, 65), to_int3(36, 64, 37), to_int3(36, 65, 64),
    to_int3(37, 63, 224), to_int3(37, 64, 63), to_int3(62, 224, 63),
    to_int3(66, 143, 67), to_int3(66, 144, 143), to_int3(66, 222, 144),
    to_int3(67, 143, 68), to_int3(68, 142, 69), to_int3(68, 143, 142),
    to_int3(69, 142, 70), to_int3(70, 141, 71), to_int3(70, 142, 141),
    to_int3(71, 140, 72), to_int3(71, 141, 140), to_int3(72, 139, 73),
    to_int3(72, 140, 139), to_int3(73, 139, 74), to_int3(74, 139, 75),
    to_int3(75, 138, 76), to_int3(75, 139, 138), to_int3(76, 137, 77),
    to_int3(76, 138, 137), to_int3(77, 137, 78), to_int3(78, 136, 79),
    to_int3(78, 137, 136), to_int3(79, 135, 80), to_int3(79, 136, 135),
    to_int3(80, 134, 81), to_int3(80, 135, 134), to_int3(81, 134, 82),
    to_int3(82, 133, 83), to_int3(82, 134, 133), to_int3(83, 133, 84),
    to_int3(84, 132, 85), to_int3(84, 133, 132), to_int3(85, 131, 86),
    to_int3(85, 132, 131), to_int3(86, 131, 87), to_int3(87, 130, 88),
    to_int3(87, 131, 130), to_int3(88, 130, 89), to_int3(89, 129, 90),
    to_int3(89, 130, 129), to_int3(90, 128, 91), to_int3(90, 129, 128),
    to_int3(91, 128, 92), to_int3(92, 127, 93), to_int3(92, 128, 127),
    to_int3(93, 126, 94), to_int3(93, 127, 126), to_int3(94, 126, 95),
    to_int3(95, 125, 96), to_int3(95, 126, 125), to_int3(96, 124, 97),
    to_int3(96, 125, 124), to_int3(97, 123, 98), to_int3(97, 124, 123),
    to_int3(98, 123, 99), to_int3(99, 122, 100), to_int3(99, 123, 122),
    to_int3(100, 121, 101), to_int3(100, 122, 121), to_int3(101, 121, 102),
    to_int3(102, 120, 103), to_int3(102, 121, 120), to_int3(103, 119, 104),
    to_int3(103, 120, 119), to_int3(104, 109, 105), to_int3(104, 119, 109),
    to_int3(105, 107, 106), to_int3(105, 109, 107), to_int3(107, 109, 108),
    to_int3(109, 119, 110), to_int3(110, 113, 111), to_int3(110, 114, 113),
    to_int3(110, 118, 114), to_int3(110, 119, 118), to_int3(111, 113, 112),
    to_int3(114, 118, 115), to_int3(115, 117, 116), to_int3(115, 118, 117),
    to_int3(144, 147, 145), to_int3(144, 148, 147), to_int3(144, 149, 148),
    to_int3(144, 222, 149), to_int3(145, 147, 146), to_int3(149, 222, 150),
    to_int3(150, 222, 151), to_int3(152, 181, 153), to_int3(152, 186, 181),
    to_int3(152, 187, 186), to_int3(152, 221, 187), to_int3(153, 181, 154),
    to_int3(154, 165, 155), to_int3(154, 180, 165), to_int3(154, 181, 180),
    to_int3(155, 159, 156), to_int3(155, 160, 159), to_int3(155, 164, 160),
    to_int3(155, 165, 164), to_int3(156, 158, 157), to_int3(156, 159, 158),
    to_int3(160, 163, 161), to_int3(160, 164, 163), to_int3(161, 163, 162),
    to_int3(165, 180, 166), to_int3(166, 177, 167), to_int3(166, 178, 177),
    to_int3(166, 180, 178), to_int3(167, 172, 168), to_int3(167, 177, 172),
    to_int3(168, 171, 169), to_int3(168, 172, 171), to_int3(169, 171, 170),
    to_int3(172, 177, 173), to_int3(173, 175, 174), to_int3(173, 176, 175),
    to_int3(173, 177, 176), to_int3(178, 180, 179), to_int3(181, 185, 182),
    to_int3(181, 186, 185), to_int3(182, 184, 183), to_int3(182, 185, 184),
    to_int3(187, 193, 188), to_int3(187, 194, 193), to_int3(187, 221, 194),
    to_int3(188, 190, 189), to_int3(188, 193, 190), to_int3(190, 192, 191),
    to_int3(190, 193, 192), to_int3(195, 205, 196), to_int3(195, 206, 205),
    to_int3(195, 211, 206), to_int3(195, 212, 211), to_int3(195, 220, 212),
    to_int3(196, 205, 197), to_int3(197, 205, 198), to_int3(198, 200, 199),
    to_int3(198, 205, 200), to_int3(200, 205, 201), to_int3(201, 205, 202),
    to_int3(202, 205, 203), to_int3(203, 205, 204), to_int3(206, 210, 207),
    to_int3(206, 211, 210), to_int3(207, 209, 208), to_int3(207, 210, 209),
    to_int3(212, 218, 213), to_int3(212, 219, 218), to_int3(212, 220, 219),
    to_int3(213, 218, 214), to_int3(214, 218, 215), to_int3(215, 218, 216),
    to_int3(216, 218, 217)};
    const int len = 219;
float rrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrrr;  
  
  for (int i = 0; i < len; i++) {
    int3 triangle = triangles[i];
    float3 a = positions[triangle.x];
    float3 b = positions[triangle.y];
    float3 c = positions[triangle.z];

    if (pointInTriangle(to_float3_aw(p, 0.0f), a, b, c)) {
      return true;
    }
  }

  return false;
}
//-----------------------------------------------------------------------------------------------------------------------------------------------------------------

__DEVICE__ bool resolutionChanged(float2 R, __TEXTURE2D__ STORAGE) {
  //float4 resolutionData = texelFetch(STORAGE, resolutionDataLocation, 0);
  float4 resolutionData = texture(STORAGE, (make_float2(resolutionDataLocation)+0.5f)/R);
  //return swi2(resolutionData,x,y) != swi2(resolutionData,z,w);
  return ((resolutionData.x) != (resolutionData.z) || (resolutionData.y) != (resolutionData.w));
}

__KERNEL__ void LegoFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel1)
{
  
  fragCoord+=0.5f;

  const int samples = 4; // (square root, actual number = samples^2)  // Vorsicht in Buffer C nochmal anders definiert !!!!
float BBBBBBBBBBBBBBBBBBBBBBBBBBBBBBB;

  if (iFrame > 0 && !resolutionChanged(R, STORAGE)) {
    float2 uv = fragCoord / iResolution;
    fragColor = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    
    SetFragmentShaderComputedColor(fragColor);
    return;
  }  
    
  fragColor = to_float4_aw(to_float3_s(0.0f), 1.0f);
  float normalizer = (float)(samples * samples);  
  float step = 1.0f / (float)(samples);
      
  for (int sx = 0; sx < samples; sx++) {
    for (int sy = 0; sy < samples; sy++) {  
      float2 uv = (fragCoord + to_float2((float)(sx), (float)(sy)) * step) / iResolution;
      uv *= 2.0f;
      uv -= to_float2_s(1.0f);
      uv *= 2.24f;
      if (inPath(uv)) {
        fragColor += to_float4_s(1.0f);
      }
    }
  }
  fragColor /= normalizer;
  fragColor.w = 1.0f;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


const int samples = 30;
const float sigma = (float)(samples) * 0.25f;

#define pi 3.1416f

#define pow2(x) (x * x)

__DEVICE__ float gaussian(float2 i) {
  return 1.0f / (2.0f * pi * pow2(sigma)) *
         _expf(-((pow2(i.x) + pow2(i.y)) / (2.0f * pow2(sigma))));
}

__DEVICE__ float3 blur(__TEXTURE2D__ sp, float2 uv, float2 scale) {
  float3 col = to_float3_s(0.0f);
  float accum = 0.0f;
  float weight;
  float2 offset;
float ccccccccccccccccccccccc;
  for (int x = -samples / 2; x < samples / 2; ++x) {
    for (int y = -samples / 2; y < samples / 2; ++y) {
      offset = to_float2(x, y);
      weight = gaussian(offset);
      col += swi3(texture(sp, uv + scale * offset),x,y,z) * weight;
      accum += weight;
    }
  }

  return col / accum;
}

#ifdef xxx
__DEVICE__ bool resolutionChanged() {
  float4 resolutionData = texelFetch(STORAGE, resolutionDataLocation, 0);
  return swi2(resolutionData,x,y) != swi2(resolutionData,z,w);
}
#endif

__KERNEL__ void LegoFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel1, sampler2D iChannel2)
{
  fragCoord+=0.5f;
float CCCCCCCCCCCCCCCCCCCCCC;
  if (iFrame > 0 && !resolutionChanged(R, STORAGE)) {
    float2 uv = fragCoord / iResolution;
    fragColor = _tex2DVecN(iChannel2,uv.x,uv.y,15);
    
    SetFragmentShaderComputedColor(fragColor);    
    return;
  }
  float2 uv = fragCoord / iResolution;
  fragColor = to_float4_aw(blur(iChannel1, uv, to_float2_s(0.00045f)), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


const float3 background = {0.85f,0.85f,0.85f};
const bool rounded = true;
const bool logo = true;
const bool shadows = true;
const int  reflections = 1;
const bool antialiasing = true;

const bool showLogoTexture = false;
const bool showBlurred = false;

const float tolerance = 0.005f;
const int steps = 256;
const float minStep = 0.001f;

const float studRadius = 0.3f;
const float studHeight = 0.1f;
const float logoHeight = 0.015f;

#define pi 3.1416f
const float3 forward = {0.0f, 0.0f, -1.0f};
const float3 right   = {1.0f, 0.0f, 0.0f};
const float3 up      = {0.0f, 1.0f, 0.0f};

#define rounding  (rounded ? 0.016f : 0.0f)

__DEVICE__ float3 toSRGB(in float3 color) { return pow_f3(color, to_float3_s(1.0f / 2.2f)); }

__DEVICE__ float sdRound(in float sd, in float radius) { return sd - radius; }

__DEVICE__ float sdSmoothUnion(in float sd1, float sd2, in float radius) {
  float h = clamp(0.5f + 0.5f * (sd2 - sd1) / radius, 0.0f, 1.0f);
  return _mix(sd2, sd1, h) - radius * h * (1.0f - h);
}

__DEVICE__ float sdSmoothSubtraction(in float sd1, in float sd2, in float k) {
  float h = clamp(0.5f - 0.5f * (sd1 + sd2) / k, 0.0f, 1.0f);
  return _mix(sd1, -sd2, h) + k * h * (1.0f - h); 
}

__DEVICE__ float sdBox(in float3 position, in float3 dimensions) {
  float3 q = abs_f3(position) - dimensions;
  return length(_fmaxf(q, to_float3_s(0.0f))) + _fminf(_fmaxf(q.x, _fmaxf(q.y, q.z)), 0.0f);
}

__DEVICE__ float sdBrick(in float3 position, in float3 size) {
  float brick = sdBox(position, size - rounding);
  float result = sdRound(brick, rounding);

  return result;
}

__DEVICE__ float sdCylinder(in float3 position, in float radius, in float height) {
  float2 d = abs_f2(to_float2(length(swi2(position,x,z)), position.y)) - to_float2(radius, height);
  return _fminf(_fmaxf(d.x, d.y), 0.0f) + length(_fmaxf(d, to_float2_s(0.0f)));
}

__DEVICE__ float sdStuds(in float3 position, in float3 size) {
  swi2S(position,x,z, fract_f2(swi2(position,x,z)));
  swi2S(position,x,z, swi2(position,x,z) - to_float2_s(0.5f));
  position.y -= size.y + studHeight - rounding * 2.0f;
    
  float studs = sdCylinder(position, 
                           studRadius - rounding * 2.0f, 
                           studHeight);
    
  return sdRound(studs, rounding * 2.0f);
}

__DEVICE__ float sdLogo(in float3 position, __TEXTURE2D__ iChannel2) {
  swi2S(position,x,z, fract_f2(swi2(position,x,z)));
    
  float tex = texture(iChannel2, clamp(to_float2(position.x, 1.0f - position.z), 0.01f, 1.0f)).x;
  return -_powf(tex, 0.4f) * logoHeight;
}

__DEVICE__ float sdCutout(in float3 position, in float3 size) {
  position.y += 0.2f - logoHeight * 2.0f;
    
  float box = sdBox(position, to_float3(size.x - 0.18f - rounding, size.y, size.z - 0.18f - rounding));
    
  return sdRound(box, rounding);
}

__DEVICE__ float sdStudsCutout(in float3 position, in float3 size) {   
  position.x -= fract(size.x);
  position.z -= fract(size.z);
  swi2S(position,x,z, fract_f2(swi2(position,x,z)));
  float cutouts = sdCylinder(position - to_float3(0.5f, 0.0f, 0.5f), 0.15f - rounding, size.y - rounding - 0.01f);
    
  return sdRound(cutouts, rounding);  
}

__DEVICE__ float sdTubes(in float3 position,  in float3 size, in float innerRadius) {
  float radius = innerRadius - rounding;
  float height = size.y - rounding - 0.05f;
  
  float halfWidth = size.x * 0.5f;
 
  if (size.x > 0.5f) {
    position.x = clamp(position.x, -size.x + 0.5f, size.x - 0.5f);
  }
  if (size.z > 0.5f) {
    position.z = clamp(position.z, -size.z + 0.5f, size.z - 0.5f);
  }
  position.x -= fract(size.x) - (size.x < 1.0f ? 0.0f : 0.5f);
  position.z -= fract(size.z) - (size.z < 1.0f ? 0.0f : 0.5f);
  swi2S(position,x,z, fract_f2(swi2(position,x,z)));
  position -= to_float3(0.5f, -0.05f, 0.5f);
  float tubes = sdRound(sdCylinder(position, radius + 0.11f, height), rounding);
  float cutout = sdCylinder(position, innerRadius + 0.01f, height + 0.2f);
float qqqqqqqqqqqqqqqqqqqqqqqq;
  return sdSmoothSubtraction(tubes, cutout, rounding);
}

__DEVICE__ float2 rotation   = {0.0f, 0.68f};
__DEVICE__ float3 brickSize  = {1.0f, 0.6f, 1.5f};
__DEVICE__ float3 brickColor = {0.03f, 0.55f, 0.79f};

__DEVICE__ float map(in float3 position, __TEXTURE2D__ iChannel2) {
  float result = sdBrick(position, brickSize);
    
  if (position.y < brickSize.y) {  
    float cutout = sdCutout(position, brickSize);
    float studs = sdStudsCutout(position, brickSize); 
      
    bool smallTube = brickSize.x < 1.0f || brickSize.z < 1.0f;
      
    result = sdSmoothSubtraction(result, cutout, rounding);
    result = sdSmoothSubtraction(result, studs, rounding);
      
    if (brickSize.x > 0.5f || brickSize.z > 0.5f) {
      float tubes = sdTubes(position, brickSize, smallTube ? 0.073f : studRadius);
      result = sdSmoothUnion(result, tubes, rounding);
    }
  }
    
  if (position.y > brickSize.y) {
    position.x -= mod_f(brickSize.x, 1.0f);
    position.z -= mod_f(brickSize.z, 1.0f);
    float studs = sdStuds(position, brickSize);
    if (studs < logoHeight * 2.0f) { 
      studs += logo ? sdLogo(position, iChannel2) : 0.0f; 
    }
    result = sdSmoothUnion(result, studs, rounded ? 0.015f : 0.0f);  
  }    
    
  return result;
}

__DEVICE__ float3 calculateNormal(in float3 position, in float pixelSize, __TEXTURE2D__ iChannel2) {
  float2 e = to_float2(1.0f, -1.0f) * pixelSize * 0.1f;
  return normalize(
      swi3(e,x,y,y) * map(position + swi3(e,x,y,y),iChannel2) + swi3(e,y,y,x) * map(position + swi3(e,y,y,x),iChannel2) +
      swi3(e,y,x,y) * map(position + swi3(e,y,x,y),iChannel2) + swi3(e,x,x,x) * map(position + swi3(e,x,x,x),iChannel2));
}

struct AABB {
  float3 min;
  float3 max;
};

struct Ray {
  float3 origin;
  float3 direction;
};
    
__DEVICE__ mat4 rotationMatrix(in float3 axis, in float angle) {
  axis = normalize(axis);
  float s = _sinf(angle);
  float c = _cosf(angle);
  float oc = 1.0f - c;

  return to_mat4(
      oc * axis.x * axis.x + c, oc * axis.x * axis.y - axis.z * s,
      oc * axis.z * axis.x + axis.y * s, 0.0f, oc * axis.x * axis.y + axis.z * s,
      oc * axis.y * axis.y + c, oc * axis.y * axis.z - axis.x * s, 0.0f,
      oc * axis.z * axis.x - axis.y * s, oc * axis.y * axis.z + axis.x * s,
      oc * axis.z * axis.z + c, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f);
}

__DEVICE__ mat4 translationMatrix(in float3 translation) {
  return to_mat4(1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f,
                 translation.x, translation.y, translation.z, 1.0f);
}
    
__DEVICE__ Ray createRayPerspective(in float2 resolution, in float2 screenPosition,
                                    in float verticalFov) {
  float2 topLeft = to_float2(-resolution.x, -resolution.y) * 0.5f;
  float z = (resolution.x * 0.5f) / _fabs(_tanf(verticalFov * 0.5f));

  Ray ret = {to_float3_s(0.0f), normalize(to_float3_aw(topLeft + screenPosition, -z))};

  return ret; //Ray(to_float3_s(0.0f), normalize(to_float3_aw(topLeft + screenPosition, -z)));
}

__DEVICE__ float3 positionOnRay(in Ray ray, in float t) {
  return ray.origin + ray.direction * t;
}

__DEVICE__ void transformRay(inout Ray *ray, mat4 matrix) {
  (*ray).origin = swi3(mul_mat4_f4(matrix , to_float4_aw((*ray).origin, 1.0f)),x,y,z);
  (*ray).direction = swi3(normalize(mul_mat4_f4(matrix , to_float4_aw((*ray).direction, 0.0f))),x,y,z);
}

__DEVICE__ void reflectRay(inout Ray *ray, float3 position, float3 normal) {
  (*ray).origin = position + normal * 0.01f;
  (*ray).direction = reflect((*ray).direction, normal);
}

__DEVICE__ void transformNormal(inout float3 *normal, in mat4 matrix) {
  *normal = normalize(swi3(mul_mat4_f4(matrix , to_float4_aw(*normal, 0.0f)),x,y,z));
}

__DEVICE__ bool rayIntersectsAABB(in Ray ray, in AABB aabb, out float *t0, out float *t1) {
  float3 invR = 1.0f / ray.direction;

  float3 tbot = invR * (aabb.min - ray.origin);
  float3 ttop = invR * (aabb.max - ray.origin);

  float3 tmin = _fminf(ttop, tbot);
  float3 tmax = _fmaxf(ttop, tbot);

  float2 t = _fmaxf(swi2(tmin,x,x), swi2(tmin,y,z));
  *t0 = _fmaxf(t.x, t.y);
  t = _fminf(swi2(tmax,x,x), swi2(tmax,y,z));
  *t1 = _fminf(t.x, t.y);

  return *t0 <= *t1;
}

__DEVICE__ float3 shade(in Ray ray, in float3 position, in float3 normal, in float3 light) {
  float3 reflection = reflect(-light, normal); 
    
  float diffuse = _fmaxf(0.0f, dot(light, normal));
  float specular = _powf(_fmaxf(dot(-ray.direction, reflection), 0.0f), 16.0f);
        
  return diffuse * brickColor + to_float3_s(0.3f) * specular;
}

__DEVICE__ float4 blend(in float4 under, in float4 over) {
  float4 result = _mix(under, over, over.w);
  result.w = over.w + under.w * (1.0f - over.w);
    
  return result;
}

__DEVICE__ bool intersectsBrick(inout Ray *ray, in AABB aabb, __TEXTURE2D__ iChannel2) {
  float t, t1;
  if (!rayIntersectsAABB(*ray, aabb, &t, &t1)) {
    return false;
  }
 
  t = _fmaxf(t, 0.0f);  
  for (int i = 0; i < steps; i++) {

    float3 position = positionOnRay(*ray, t);
    float sd = map(position,iChannel2);
     
    if (sd < tolerance) {
      (*ray).origin = position;
      return true;
    }
    
    t += _fmaxf(tolerance, sd);
      
    if (t > t1) {
      return false;
    }
  }
   
  return false;
}

__DEVICE__ float intersectsBrickShadow(inout Ray *ray, in AABB aabb, __TEXTURE2D__ iChannel2) {
  float res = 1.0f;
  float ph = 1e10;
  float k = 10.0f;
    
  float t, t0, t1;
  if (!rayIntersectsAABB(*ray, aabb, &t, &t1)) {
    return res;
  }
  t = _fmaxf(0.0f, t0);
  for (int i = 0; i < steps; i++) {
    float3 position = positionOnRay(*ray, t);
float xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx;    
    float sd = map(position,iChannel2);
     
    float y = sd * sd / (2.0f * ph);
    float d = _sqrtf(sd * sd - y * y);
    res = _fminf(res, k * d / _fmaxf(0.0f, t - y));
    ph = sd;
      
    t += _fminf(sd, 0.005f);
      
    if (res < 0.0001f || t > t1) {
      break;
    }
  }
    
  return clamp(res, 0.0f, 1.0f);
}

__DEVICE__ float4 trace(in Ray ray, __TEXTURE2D__ iChannel2) {
  AABB aabb = {-brickSize, brickSize};
  aabb.min -= to_float3_s(0.01f);
  aabb.max += to_float3_s(0.01f);
  aabb.max.y += (studHeight + logoHeight) * 2.0f;
    
  mat4 transform = mul_mat4_mat4(mul_mat4_mat4(rotationMatrix(up, rotation.x) , 
                                              rotationMatrix(right, rotation.y)) ,
                                translationMatrix(to_float3(0.0f, 0.0f, 6.0f)));
    
  float3 light = to_float3(-0.9f, 0.9f, 2.5f);
  
  float3 forward = forward;
  transformRay(&ray, transform);
  transformNormal(&light, transform);
  transformNormal(&forward, transform);
  
  float mul = 1.0f;
  float3 result = to_float3_s(0.0f);
  for (int i = 0; i <= reflections; i++) {
    if (intersectsBrick(&ray, aabb,iChannel2)) {
      float3 normal = calculateNormal(ray.origin, 0.001f, iChannel2);
      float shadow = 1.0f;
      
      if (shadows) {
        Ray shadowRay = {ray.origin, normalize(light)};
        shadowRay.origin += normal * 0.001f;
        shadow = intersectsBrickShadow(&shadowRay, aabb, iChannel2);
      }
          
      result += mul * brickColor * 0.2f;
      result += mul * shadow * shade(ray, ray.origin, normal, light);
      
      reflectRay(&ray, ray.origin, normal);
      mul *= 0.045f;
    } else {
      result += mul * background;
      break;
    }
  }
float tttttttttttttttttttttttt;     
  return to_float4_aw(result, 1.0f);
}

__DEVICE__ float4 takeSample(in float2 position, float pixelSize, float2 iResolution, __TEXTURE2D__ iChannel2) {
  const float fov = pi / 2.0f;
  Ray ray = createRayPerspective(iResolution, position, fov);
  return trace(ray,iChannel2);
}

__DEVICE__ float4 superSample(in float2 fragCoord, float2 iResolution, __TEXTURE2D__ iChannel2) {
  const int sampleCount = antialiasing ? 4 : 1;
  const float2 samplePositions[] = {                             
                                    to_float2(-0.125f, -0.375f), to_float2(0.375f, -0.125f),      
                                    to_float2(-0.375f,  0.125f), to_float2(0.125f,  0.375f)       
                                    };
  float4 result = to_float4_s(0.0f);                                    
  float samplesSqrt = _sqrtf((float)(sampleCount));                        
  for (int i = 0; i < sampleCount; i++) {                              
    result += takeSample(fragCoord + samplePositions[i],               
                         1.0f / samplesSqrt, iResolution, iChannel2);                           
  }                                                                    
                                                                         
  return result / (float)(sampleCount);                                  
}

__KERNEL__ void LegoFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1, sampler2D iChannel2)
{

  rotation   = swi2(texture(STORAGE, (make_float2(rotationDataLocation)+0.5f)/R),x,y);
  brickSize  = swi3(texture(STORAGE, (make_float2(brickSizeDataLocation)+0.5f)/R),x,y,z);
  brickColor = swi3(texture(STORAGE, (make_float2(brickColorDataLocation)+0.5f)/R),x,y,z);
float IIIIIIIIIIIIIIIIIIIIIIIIIII;    
  if (showLogoTexture) {
    float2 uv = (fragCoord - iResolution * 0.5f) / iResolution.y;
    uv += to_float2_s(0.5f);
    if (showBlurred) {
      fragColor = _tex2DVecN(iChannel2,uv.x,uv.y,15);
    } else {
      fragColor = _tex2DVecN(iChannel1,uv.x,uv.y,15);
    }
    SetFragmentShaderComputedColor(fragColor);
    return;
  }  
    
  fragColor = to_float4_aw(toSRGB(swi3(superSample(fragCoord, iResolution, iChannel2),x,y,z)), 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
