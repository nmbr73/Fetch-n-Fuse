
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution

#define PHI  1.61803398874989484820459f // Φ = Golden Ratio 

#ifdef xxxx
const float2 startPos = {0.5f,0.5f};
const float TreeThickness = 3.0f;
const float speed =64.0f*0.016f;
const float TreeSpread = 0.1f;
const int BrancSpacing = 0;
const float TurnSpeed = 0.2f;
const int MinBrancingCondition = 1;
const float InitialBranches = 1.0f;
const float BrancingProbability = 0.94f;


const float4 BrightColor1 = {0.820f,0.906f,0.773f,1.0f};
const float4 DarkColor1 = {0.392f,0.725f,0.631f,1.0f};


const float4 BrightColor2 = {0.686f,0.655f,0.843f,1.0f};   
const float4 DarkColor2 = {0.439f,0.400f,0.220f,1.0f};
const float4 BgColor1 = {0.204f,0.212f,0.294f,0.1f};
const float4 BgColor2 = {0.310f,0.490f,0.631f,0.1f};
#endif    

__DEVICE__ float gold_noise(in float2 xy, in float seed)
{
    return fract(_tanf(distance_f2(xy*PHI, xy)*seed)*xy.x);
}
__DEVICE__ float2 randomDir2(float2 uv, float seed )
{
    float x = gold_noise(uv, seed+1.0f);
    float y = gold_noise(uv, seed-2.0f);
    return normalize(to_float2(x,y)-0.5f);
}


__DEVICE__ float2 quantizeto_float2( float2 o )
{
    float angle = _atan2f(o.y, o.x);
    float minRad = 3.1415926f*0.25f;
    angle = _floor(angle/minRad)*minRad;
    return to_float2(_cosf(angle), _sinf(angle));
}

__DEVICE__ float2 rotto_float2(float2 o, float rad)
{
    //return mat2x2(to_float2(_cosf(rad), -_sinf(rad)),to_float2(_sinf(rad),_cosf(rad))) * o;
    return mul_mat2_f2(to_mat2(_cosf(rad), -_sinf(rad),_sinf(rad),_cosf(rad)) , o);
}

__DEVICE__ float3 pal( in float t, in float3 a, in float3 b, in float3 c, in float3 d )
{
    return a + b*cos_f3( 6.28318f*(c*t+d) );
}

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


__DEVICE__ int sense( float2 pos, int range, float2 R, __TEXTURE2D__ iChannel2)
{
    pos /= 2.0f;
    int count  = 0;
    for ( int i = -range; i <=range; i++)
    {
        for (int j = -range; j <=range; j++)
        {
            //float4 data = texelFetch( iChannel2, to_int2(pos)+to_int2(i,j), 0);
            float4 data = texture( iChannel2, (make_float2(to_int2_cfloat(pos)+to_int2(i,j))+0.5f)/R);
            //if ( length(swi2(data,z,w)) > 0.0f )
            count +=(int)(data.x);
        }
    }
    return count;
}

__KERNEL__ void OrganicFabricsiJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{
    CONNECT_CHECKBOX0(Reset, 0);

    fragCoord+=0.5f;

    CONNECT_SLIDER0(TreeThickness, -1.0f, 10.0f, 3.0f);
    CONNECT_SLIDER1(speed, -1.0f, 10.0f, 1.024f);
    CONNECT_SLIDER2(TreeSpread, -1.0f, 1.0f, 0.1f);
    CONNECT_INTSLIDER0(BrancSpacing, 0, 5, 0);
    CONNECT_SLIDER3(TurnSpeed, -1.0f, 10.0f, 0.2f);
    CONNECT_SLIDER4(BrancingProbability, -1.0f, 10.0f, 0.94f);

    CONNECT_COLOR0(BrightColor1, 0.820f, 0.906f, 0.773f, 1.0f);
    CONNECT_COLOR1(DarkColor1, 0.392f, 0.725f, 0.631f, 1.0f);
    CONNECT_COLOR2(BrightColor2, 0.686f, 0.655f, 0.843f, 1.0f);
    CONNECT_COLOR3(DarkColor2, 0.439f, 0.400f, 0.220f, 1.0f);

#ifdef xxxx
    //const float2 startPos = {0.5f,0.5f};
    const float TreeThickness = 3.0f;
    const float speed = 64.0f*0.016f;
    const float TreeSpread = 0.1f;
    const int BrancSpacing = 0;
    const float TurnSpeed = 0.2f;
    //const int MinBrancingCondition = 1;
    //const float InitialBranches = 1.0f;
    const float BrancingProbability = 0.94f;


    const float4 BrightColor1 = {0.820f,0.906f,0.773f,1.0f};
    const float4 DarkColor1 = {0.392f,0.725f,0.631f,1.0f};


    const float4 BrightColor2 = {0.686f,0.655f,0.843f,1.0f};   
    const float4 DarkColor2 = {0.439f,0.400f,0.220f,1.0f};
    //const float4 BgColor1 = {0.204f,0.212f,0.294f,0.1f};
    //const float4 BgColor2 = {0.310f,0.490f,0.631f,0.1f};
#endif


    fragColor = to_float4(0.0f,0.0f,0.0f,0.0f);
    float2 uv = fragCoord / iResolution.y;

    if ( iFrame == 0 || Reset)
    {
         float2 center =  iResolution*0.5f/iResolution.y;
         float2 p = fragCoord/iResolution.y;
         float2 dir = (p - center)*50.0f;
         float l = length(dir);
         dir = normalize(dir);
         fragColor = to_float4_f2f2(fragCoord,to_float2(dir.x, dir.y) )* step(l, 1.0f);
    }
    else 
    {
        int count = 0;
        int range = BrancSpacing;

        for ( int i = -2; i <=2; i++)
        {
            for (int j = -2; j <=2; j++)
            {
                //float4 dataOld = texelFetch( iChannel0, to_int2(fragCoord)+to_int2(i,j), 0);
                float4 dataOld = texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord)+to_int2(i,j))+0.5f)/R);
                float2 previousPos = swi2(dataOld,x,y) + swi2(dataOld,z,w)*-speed;
                float4 dataNew = dataOld;
                swi2S(dataNew,x,y, swi2(dataNew,x,y) + swi2(dataNew,z,w)*speed);

                if ( length(swi2(dataNew,z,w)) > 0.0f && (int)(fragCoord.x) == (int)(dataNew.x) && (int)(fragCoord.y) == (int)(dataNew.y))
                {
                    // randomly nudge the dir
                    swi2S(dataNew,z,w, normalize(swi2(dataNew,z,w) + randomDir2(fragCoord,iTime)*TreeSpread));

                    fragColor = dataNew;
                }
                float bp = BrancingProbability ;


                if ( (int)(previousPos.x) == (int)(fragCoord.x) && (int)(previousPos.y) == (int)(fragCoord.y)  && gold_noise(fragCoord,iTime+4.3f) > bp )
                {
                    fragColor = dataOld;
                }
            }
        }
      
        if (length(swi2(fragColor,z,w)) > 0.0f)
        {
            // new dir based on sense
            int senseRange = 5;
            float senseDist = (float)(senseRange)*4.0f+2.0f;
            float2 turnL = rotto_float2(swi2(fragColor,z,w), 3.1415f/3.0f);
            float2 turnR = rotto_float2(swi2(fragColor,z,w), -3.1415f/3.0f);
            int fc = sense(fragCoord+swi2(fragColor,z,w)*senseDist, senseRange,R,iChannel2);
            int lc = sense(fragCoord+turnL*senseDist, senseRange,R,iChannel2);
            int rc = sense(fragCoord+turnR*senseDist, senseRange,R,iChannel2);

            if (fc >= lc && fc >= rc )
            {
                //keep moving
            }
            else if ( lc > rc )
            {
                swi2S(fragColor,z,w, normalize(swi2(fragColor,z,w) + turnL*TurnSpeed));
            }
            else
                swi2S(fragColor,z,w, normalize(swi2(fragColor,z,w) + turnR*TurnSpeed));
        }  
        
        if (length (fragCoord - swi2(iMouse,z,w)) < 5.0f && length(swi2(iMouse,z,w) )> 1.0f)
        {
            float2 dir = normalize(fragCoord - swi2(iMouse,x,y));
            fragColor = to_float4_f2f2(fragCoord,to_float2(dir.x, dir.y) );
        }
    }

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1



__KERNEL__ void OrganicFabricsiJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    fragCoord+=0.5f;

    CONNECT_SLIDER0(TreeThickness, -1.0f, 10.0f, 3.0f);
    CONNECT_SLIDER1(speed, -1.0f, 10.0f, 1.024f);
    CONNECT_SLIDER2(TreeSpread, -1.0f, 1.0f, 0.1f);
    CONNECT_INTSLIDER0(BrancSpacing, 0, 5, 0);
    CONNECT_SLIDER3(TurnSpeed, -1.0f, 10.0f, 0.2f);
    CONNECT_SLIDER4(BrancingProbability, -1.0f, 10.0f, 0.94f);

    CONNECT_CHECKBOX1(Pal, 1);

    CONNECT_COLOR0(BrightColor1, 0.820f, 0.906f, 0.773f, 1.0f);
    CONNECT_COLOR1(DarkColor1, 0.392f, 0.725f, 0.631f, 1.0f);
    CONNECT_COLOR2(BrightColor2, 0.686f, 0.655f, 0.843f, 1.0f);
    CONNECT_COLOR3(DarkColor2, 0.439f, 0.400f, 0.220f, 1.0f);

    float scale = 1.0f;//+12.0f*((_sinf(iTime*5.0f)+1.0f)*0.5f);
    
    int kernelSize = (int)(TreeThickness);

    if (kernelSize <1)
        kernelSize = 1;
    
        bool found = false;
        float foundDist = 10000.0f;
        float2 angle ;

        for ( int i = -kernelSize; i <=kernelSize; i++)
        {
            if (found )
                break;
            for (int j = -kernelSize; j <=kernelSize; j++)
            {
                if (j*j + i*i < kernelSize*kernelSize )
                {
                    if (length(swi2(texture(iChannel0, (fragCoord+to_float2((float)(i),(float)(j)))/iResolution),z,w) ) > 0.0f)
                    {
                        found = true;
                        float dist = length(to_float2((float)(i),(float)(j)));
                        if ( dist < foundDist )
                            foundDist = dist;
                        angle = normalize(to_float2((float)(i),(float)(j)));

                        break;
                    }
                }
                
            }
        }

        float4 oldColor = pow_f4(texture(iChannel1, fragCoord/iResolution),to_float4_s(1.011f));
        
        if (found)
        {
            float a = 0.5f*(dot(angle, normalize(to_float2(1.0f,0.0f))) + 1.0f)*smoothstep(0.0f,float(kernelSize),foundDist);
            a = clamp(a,0.0f,1.0f);
            
            float ga = 0.0f;
            float4 darkColor   = swi4(pal( 59.0f/30.0f, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,1.0f,1.0f),to_float3(0.0f,0.10f,0.20f) ),x,y,z,z);//_mix(DarkColor1, DarkColor2, ga);
            float4 brightColor = swi4(pal( 59.0f/30.0f, to_float3(0.5f,0.5f,0.5f),to_float3(0.5f,0.5f,0.5f),to_float3(1.0f,0.7f,0.4f),to_float3(0.0f,0.15f,0.20f) ),x,y,z,z);// _mix(BrightColor1, BrightColor2, ga);
            
            if (Pal==false)
            {
              darkColor   =  _mix(DarkColor1, DarkColor2, ga);
              brightColor =  _mix(BrightColor1, BrightColor2, ga);
            } 
            
            float blendIn = smoothstep((float)(kernelSize),0.0f,foundDist);
            blendIn = clamp(blendIn, 0.0f,1.0f);
            fragColor = _mix(oldColor,_mix(darkColor, brightColor,a),blendIn );
        }
        else
            fragColor = oldColor;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void OrganicFabricsiJipiFuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    fragCoord *=2.0f;
    fragColor.x  = sign_f(length(swi2(texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(0,0))+0.5f)/R),z,w)));
    fragColor.x += sign_f(length(swi2(texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(0,1))+0.5f)/R),z,w)));
    fragColor.x += sign_f(length(swi2(texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(1,0))+0.5f)/R),z,w)));
    fragColor.x += sign_f(length(swi2(texture(iChannel0, (make_float2(to_int2_cfloat(fragCoord) + to_int2(1,1))+0.5f)/R),z,w)));

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer B' to iChannel0


__KERNEL__ void OrganicFabricsiJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    fragCoord+=0.5f;

    fragColor = pow_f4( texture(iChannel0, fragCoord/iResolution),to_float4_s(0.8f));
    // try this different appearance 
    //fragColor = 1.0f-_powf( texture(iChannel0, fragCoord/iResolution),to_float4_s(2.2f));

  SetFragmentShaderComputedColor(fragColor);
}