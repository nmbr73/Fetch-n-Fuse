
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Preset: Keyboard' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define R    iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define KEYBOARD iChannel1
#define KEY_RESET 82
//#define hash(p)  fract(_sinf(dot(p, to_float2(12.9898f, 78.233f))) * 43758.5453f)

__DEVICE__ float3 Cell( in int2 p, float2 R, __TEXTURE2D__ iChannel0 )
{
    // do wrapping
    int2 r = to_int2_cfloat(R);//(textureSize(iChannel0, 0));
    p = to_int2((p.x+r.x) % r.x, (p.y+r.y) % r.y);
    
    // fetch texel
    //return texelFetch(iChannel0, p, 0 ).rgb;
    return swi3(texture(iChannel0, (make_float2(p)+0.5f)/R),x,y,z);
}


__DEVICE__ float h21(float2 a) {
    return fract(_sinf(dot(swi2(a,x,y), to_float2(12.9898f, 78.233f))) * 43758.5453123f);
}

//__DEVICE__ bool key_down(int key) {
//    return int(texelFetch(KEYBOARD, to_int2(key, 0), 0).x) == 1;
//}


__KERNEL__ void SpaceWarAutomata2Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_CHECKBOX0(Reset, 0);
    
    CONNECT_SLIDER0(Neigbours, -1.0f, 1.0f, 0.5f);
    CONNECT_SLIDER1(Par1, 0.0f, 100000.0f, 10000.0f);
    CONNECT_SLIDER2(Par2, -1.0f, 1.0f, 0.3f);
    CONNECT_SLIDER3(Par3, -1.0f, 1.0f, 0.16f);
    CONNECT_SLIDER4(Par4, 0.0f, 1.0f, 0.6f);
    
    fragCoord+=0.5f;

    int2 px = to_int2_cfloat( fragCoord );    

    if (iFrame==0 || Reset) 
    {    
        float d = length((fragCoord -0.5f* iResolution ) / iResolution.y);
        float2 f = fragCoord + 0.001f * iTime;
        float3 g = to_float3(h21(f), h21(f + 1.0f), h21(f - 1.0f));
        swi3S(fragColor,x,y,z, step(to_float3_s(0.8f), g));
        
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
        
    // center cell
    float3 e = Cell(px,R, iChannel0); 
    float _e[3] = {e.x,e.y,e.z};
    
    // neighbour cells
    float3 t = Cell(px + to_int2(0,-1),R, iChannel0);
    float3 b = Cell(px + to_int2(0,1),R, iChannel0);
    float3 l = Cell(px + to_int2(-1,0),R, iChannel0);
    float3 r = Cell(px + to_int2(1,0),R, iChannel0);   
    
    // "average" of neighbours
    //float3 k = 0.5f * _fmaxf(t + b, l + r);
    //float k[3] = {0.5f * _fmaxf(t.x + b.x, l.x + r.x), 0.5f * _fmaxf(t.y + b.y, l.y + r.y), 0.5f * _fmaxf(t.z + b.z, l.z + r.z)};
    float k[3] = {Neigbours * _fmaxf(t.x + b.x, l.x + r.x), Neigbours * _fmaxf(t.y + b.y, l.y + r.y), Neigbours * _fmaxf(t.z + b.z, l.z + r.z)};
    
    // difference between "average" and center
    //float3 j = abs_f3(e - to_float3(k[0],k[1],k[2]));
    float j[3] = {_fabs(_e[0] - k[0]),_fabs(_e[1] - k[1]),_fabs(_e[2] - k[2])};
    
    for (int i = 0; i < 3; i++) {
        if (_e[i] < k[i] - 0.3f)
            //_e[i] = 1.0f * k[i] +  10000.0f * j[i] * _e[i] * _e[i] * _e[i]; 
          _e[i] = 1.0f * k[i] +  Par1 * j[i] * _e[i] * _e[i] * _e[i]; 
        else if (k[i] > 0.01f && _e[i] > 0.46f && j[i] < 0.5f)
            //_e[i]= k[i] + 0.3f * j[i];      
            _e[i]= k[i] + Par2 * j[i];      
        else 
            _e[i] = k[i];
            //_e[i] -= 0.16f * j[i];
            _e[i] -= Par3 * j[i];
            _e[i] *= 0.99f;
    }    
float AAAAAAAAAAAAAAAAAAA;     
    //float3 e2 = to_float3(_e[0],_e[1],_e[2]);//  e;
    float e2[3] = {_e[0],_e[1],_e[2]};
    for(int i = 0; i < 3; i++) {
        if (_e[(i+1)%3] < _e[i]) {
            //e2[(i-1)%3] += 0.01f;
            //e2[(i)%3] -= 0.05f;// * fragCoord.x/iResolution.x;
            //e2[(i+1)%3] -= 0.01f;
            
            //e2[i] = _mix(_e[i], _e[(i+1)%3], 0.6f);
            e2[i] = _mix(_e[i], _e[(i+1)%3], Par4);
        }
    }
    //e = e2;
    //*/
    //e = clamp(e,0.0f,1.0f);
    e2[0] = clamp(e2[0],0.0f,1.0f);
    e2[1] = clamp(e2[1],0.0f,1.0f);
    e2[2] = clamp(e2[2],0.0f,1.0f);
     
    fragColor = to_float4( e2[0],e2[1],e2[2], 0.0f );


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void SpaceWarAutomata2Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_COLOR0(Color, 0.0f, 0.0f, 0.0f, 1.0f); 
    fragCoord+=0.5f;
    
    //float3 col = texelFetch( iChannel0, to_int2(fragCoord), 0 ).rgb;
    float3 col = swi3(texture( iChannel0, (make_float2(to_int2_cfloat(fragCoord))+0.5f)/R),x,y,z);
    //swi3(fragColor,x,y,z) = to_float3(col);
    fragColor = to_float4_aw(col, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}
