
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


__DEVICE__ float sdSegment( in float2 p, in float2 a, in float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float h21 (float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}


__KERNEL__ void RandomWalkEFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
    CONNECT_BUTTON0(Modus, 1, Var1,  Var2, Var3, Var4, Var5);

    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER1(Increment, 0.0f, 100.0f, 20.0f);
    CONNECT_SLIDER2(NmbrPoints, 0.0f, 100.0f, 50.0f);
    CONNECT_SLIDER3(K, 0.0f, 1.0f, 0.01f);
    CONNECT_SLIDER4(Length, -1.0f, 5.0f, 2.0f);
    CONNECT_SLIDER5(Hash, -1.0f, 5.0f, 0.2f);
    CONNECT_SLIDER6(AlphaThres, 0.0f, 1.0f, 0.1f);

    // number of points
    float m = NmbrPoints;//50.0f;


    float2 uv = (fragCoord - 0.5f * iResolution) / iResolution.y;
       
    // increment time (60.0f * iTime looks smooth, but bugs out faster)
    float t = _floor(Increment * iTime) - 1000.0f;

    // start point
    float2 p = to_float2_s(0.0f);
    
    // initialise d with "large" number
    float d = 1.0f;
    float k = K;//0.01f;
    for (float i = 0.0f; i < m; i+=1.0f) {
        t++;
    
        // next point in sequence, move further if further along the sequence
        // (bad use of h21)
        float2 p2 = p + Hash * (i/m) * (to_float2(h21(k * to_float2(t, t + 1.0f)), 
                                                  h21(k * to_float2(t, t - 1.0f))) - 0.5f);
                                          
        // keep points on screen
        p2 = clamp(p2, -0.48f, 0.48f);
    
        // length from points / segments
        float d2 = _fminf(0.5f * length(uv - p2), Length * sdSegment(uv, p, p2)); //2.0f
        
        float _k = step(h21(to_float2(t, t + 1.0f)), 0.5f);
        p = p2; //_mix(to_float2_s(0.0f), p2, k);
        
        if((int)Modus&2) p = _mix(to_float2_s(0.0f), p2, _k);
        
        d = _fminf(d, d2);
        
        // fade points 
        d += 0.0005f; // + 0.0007f * _cosf(3.0f * iTime + i / m);
        if ((int)Modus&4) d += 0.0007f * _cosf(3.0f * iTime + i / m);
        
    }
    
    // draw stuff
    float s = smoothstep(-0.03f, 0.03f, -d + 0.005f); //-step(d, 0.01f);
    if((int)Modus&8) s-=step(d, 0.01f);
    s = clamp(s, 0.0f, 1.0f);
    s *= 4.0f * s;//4.2f * s * _cosf(4.0f * s + 10.0f * iTime) + 4.5f *s;
    if((int)Modus&16) s *= 4.2f * s * _cosf(4.0f * s + 10.0f * iTime) + 4.5f *s;
    
    float3 col = to_float3_s(s);
    
    fragColor = to_float4_aw(col,1.0f);
    
    if(fragColor.x>AlphaThres) fragColor.w = 1.0f;
    else                       fragColor.w = Color.w;

    fragColor = (fragColor + (Color-0.5f))*fragColor.w;


  SetFragmentShaderComputedColor(fragColor);
}