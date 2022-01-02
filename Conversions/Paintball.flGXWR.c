
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

//__DEVICE__ float distance_f2(float2 pt1, float2 pt2){ float2 v = pt2 - pt1; return _sqrtf(dot(v,v));}

__DEVICE__ float _fwidth(float inp, float2 iR){
    //simulate fwidth
    float uvx = inp + 1.0f/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + 1.0f/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}


#define hsv2rgb(h) clamp( abs_f3(mod_f3( h*6.0f+to_float3(0,4,2), 6.0f)-3.0f)-1.0f, 0.0f, 1.0f )



__DEVICE__ float random (float i){
   return fract(_sinf((float)(i)*43.0f)*4790.234f);
}


__DEVICE__ float calcInfluence( float4 ball, float2 uv)
{
    float d = distance_f2(swi2(ball,x,y), uv);
    float inf = _powf( ball.z/d, 3.0f);
    return  inf;
}

__DEVICE__ float3 calcNormal( float4 ball, float2 uv )
{
    return to_float3_aw( swi2(ball,x,y) - uv, 0.1f);
}



//************************
__KERNEL__ void PaintballFuse(
    float2 fragCoord,
    float2 iResolution,
    float4 iMouse,
    float  iTime
   )
{

    const int nBalls = 40;
    // const int nLights = 4;
    CONNECT_TINYINT0(nLights,0,10,4);
    const int nLightsMax = 10;
    //const int numColors = 3; //max 4
    CONNECT_TINYINT1(numColors,0,4,3);
    const float lightZ = -0.2f;

    float3 colors[] = {
        to_float3(255.0f/255.0f, 77.0f/255.0f, 0.0f/255.0f),
        to_float3(10.0f/255.0f, 84.0f/255.0f, 255.0f/255.0f),
        to_float3(255.0f/255.0f, 246.0f/255.0f, 0.0f/255.0f),
        to_float3(0.0f/255.0f, 192.0f/255.0f, 199.0f/255.0f)
    };

    //for gradient?
    float3 colors2[] = {
        to_float3(230.0f/255.0f, 25.0f/255.0f, 56.0f/255.0f),
        to_float3(230.0f/255.0f, 144.0f/255.0f, 25.0f/255.0f),
        to_float3(0.0f/255.0f, 199.0f/255.0f, 152.0f/255.0f),
        to_float3(10.0f/255.0f, 165.0f/255.0f, 255.0f/255.0f)
    };



    // Normalized pixel coordinates (from 0 to 1)
    float2 uv = fragCoord/iResolution.y;
    uv.x -= 0.333f;
    float4 mouse = iMouse / to_float4_s(iResolution.y);
    mouse.x -= 0.333f;

     int i;

    //settings to play with!
    float threshold = 1.0f;
    float shadowIntensity = 0.5f;
    float specularIntensity = 0.75f;
    float specularPower = 300.0f;
    float rimIntensity = 3.0f; //2
    float aoIntensity = 0.75f; //75
    float ambientBrightness =  0.5f + 0.5f * uv.y;
    float lightFalloff = 0.5f;

    bool rainbowMode = false;


     //balls
    float rad = 0.07f;
    float rf = 0.005f;
    float jiggle = _sinf(iTime*(2.0f)) * 0.0125f;

    float speed = 0.3f;

    float4 balls[nBalls];
    //float4 lights[nLights];
    float4 lights[nLightsMax];

    for( i = 0; i < nBalls; i++ ){

        float per = (float)(i)/(float)(nBalls);
        float r = random( per * 7.0f + 0.32f);
        float r2 = random( per * 11.0f + 0.87f );
        float r3 = random( per * 19.0f + 0.121f );
        float time = iTime + r * 11.0f + r2 * 21.0f;
        float x = 0.5f + _sinf(time*speed * (0.5f + 0.5f * r2))*(0.1f + 0.9f * r);
        float y = 0.5f + _cosf(time*speed * (0.5f + 0.5f * r3))*(0.1f + 0.9f * r2);

        int color = i % numColors;
        float rd = rad + 0.9f * rad * _sinf(iTime*0.2f + r*13.0f)*r;

        balls[i] = to_float4( x, y, rd, color );
    }

    for( i = 0; i < nLights; i++ ){

        float per = (float)(i)/(float)(nBalls);
        float r = random( per * 21.0f + 17.0f );
        float r2 = random( per * 31.0f + 13.0f );
        float r3 = random( per * 41.0f + 3.0f );
        float time = iTime + r * 21.0f + r2 * 11.0f;
        float x = 0.5f + _sinf(time*speed * (0.5f + 0.5f * r2))*(0.1f + 0.9f * r);
        float y = 0.5f + _cosf(time*speed * (0.5f + 0.5f * r3))*(0.1f + 0.9f * r2);

        lights[i] = to_float4( x, y, 0.01f, 1.0f );
    }


    int ballCount = nBalls;

    int accumulatorCount = 4;
    float accumulators[] = {
        0.0f,
        0.0f,
        0.0f,
        0.0
    };

    float3 shaders[] = {
      to_float3_s(0),
      to_float3_s(0),
      to_float3_s(0),
      to_float3_s(0)
    };



    //determine color with greatest influence
    for( i = 0; i < ballCount; i++ )
    {
        int idx = (int)( balls[i].w );
        float inf = calcInfluence( balls[i], uv);
        accumulators[idx] += inf;
        shaders[idx] += calcNormal( balls[i], uv) * inf;
    }

    float maxInf = 0.0f;
    int maxIdx = 0;
    float3 avgColor = to_float3(0,0,0);
    float totalInf = 0.0f;

    for( i = 0; i < accumulatorCount; i++ )
    {
        if( accumulators[i] > maxInf )
        {
            maxInf = accumulators[i];
            maxIdx = i;
        }

        totalInf += accumulators[i];
        avgColor += accumulators[i] * colors[i];
    }

    avgColor /= totalInf;

    float influence = accumulators[maxIdx];
    float3 baseColor = colors[maxIdx];
    float3 normal = normalize(shaders[maxIdx]);



    //basecolor
    float3 color = baseColor;
    float3 ambientColor = to_float3_s(ambientBrightness);
    if( rainbowMode )
        ambientColor = avgColor * ambientBrightness;

    //rim light
    float rim = 1.0f - (dot ( to_float3(0.0f,0.0f,-1.0f), -normal));
    color += to_float3_s(1.0f) * rimIntensity * _powf (rim, 2.0f);

    color = color * (1.0f - shadowIntensity);

    for( i = 0; i < nLights; i++ )
    {
        float4 light = lights[i];
        float3 lightDir = normalize( to_float3_aw(swi2(light,x,y), lightZ) - to_float3_aw( uv, 0.0f ) );
        float intensity = _fminf( 1.0f, (lightFalloff * light.w) / _powf( distance_f2(swi2(light,x,y), uv ), 2.0f ));

        //diffuse
        float lighting = _fmaxf(0.0f,dot( -normal, lightDir) );
        lighting *= intensity;
        color += _fmaxf( (baseColor * lighting) - color, to_float3_s(0.0f) );

        // specular blinn phong
        float3 dir = normalize(lightDir + to_float3(0,0,-1.0f) );
        float specAngle = _fmaxf(dot(dir, -normal), 0.0f);
        float specular = _powf(specAngle, specularPower);
        color += to_float3_s(1.0f) * specular * specularIntensity * intensity;
    }




    //ao
    float prox = (maxInf/totalInf);
    prox = _powf( smoothstep( 1.0f, 0.35f, prox), 3.0f );
    float3 aoColor = to_float3_s(0.0f);
    color = _mix( color , aoColor, prox * aoIntensity);

  //shape
    float aa = _fminf( _fwidth( influence, iResolution ) * 1.5f, 1.0f);
    float smo = smoothstep( 0.0f, aa, influence - threshold);
    color = _mix( ambientColor, color, smo);


    for( i = 0; i < nLights; i++ )
    {
      float4 light = lights[i];
      float lightIntensity = calcInfluence( light, uv );
      color += _powf(lightIntensity,0.5f) * 1.0f * light.w;
    }


  SetFragmentShaderComputedColor(to_float4_aw( color, 1.0f ));
}
