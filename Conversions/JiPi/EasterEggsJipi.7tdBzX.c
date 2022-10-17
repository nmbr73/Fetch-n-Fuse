
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Egg3' to iChannel3
// Connect Image 'Texture: Egg2' to iChannel2
// Connect Image 'Texture: Egg1' to iChannel1
// Connect Image 'Texture: Audio' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)
#define R iResolution


#define TWO_PI  6.28318530718f
#define PI      3.14159265359f
    
__DEVICE__ mat3 ZRotMatrix(in float a)
{
    return to_mat3( _cosf(a), -_sinf(a), 0.0f,
                    _sinf(a),  _cosf(a), 0.0f, 
                        0.0f,      0.0f, 1.0f);
}

__DEVICE__ float3 sdEgg( in float3 p, in float3 c, float Zr, in float3 s)
{
    //apply transformation
    p -= c;
    p = mul_mat3_f3(ZRotMatrix(Zr) , p);
    p *= s;

    float3 res = to_float3_s(0.0f);
    
    //compute sdf
    float f = _powf(1.2f*dot(swi2(p,x,z), swi2(p,x,z)), 0.8f);
    p.y += 0.15f * f;
    res.x = (length(p) - 0.5f);
    
    //uvs
    swi2S(res,y,z, to_float2( (_atan2f(p.x, p.z)) / (TWO_PI),
                   (sign_f(p.y)*_acosf(dot( normalize(p), normalize(to_float3(p.x,0.0f,p.z))))) / PI
                 ));
                 
    return res;
}

//------------------------------------------------------------------

__DEVICE__ float4 opU( float4 d1, float4 d2 )
{
  return (d1.x<d2.x) ? d1 : d2;
}

__DEVICE__ float4 map( in float3 pos, float iTime, float2 DanceMovEggs[3], float3 MovEggs[3]  )
{
    float4 res = to_float4( 1e10,0.0f,0.0f,0.0f );
    
    float delay = 4.0f;
    //float hmov = _sinf((iChannelTime[0]-delay) * ( iChannelTime[0]>delay ? 6.13f : 0.0f));
    float hmov = _sinf((iTime-delay) * ( iTime ? 6.13f : 0.0f));
    //float scale = 2.0f*_sinf((iChannelTime[0]-delay) * ( iChannelTime[0]>delay ? 24.52f : 0.0f ));
    float scale = 2.0f*_sinf((iTime-delay) * ( iTime ? 24.52f : 0.0f ));
    //res = opU( res, to_float4_aw( sdEgg( pos,to_float3(-hmov*0.1f,0.62f + scale*0.02f,0.0f),hmov*0.2f,to_float3(1.0f+ scale*0.03f, 0.8f - scale*0.03f, 1.0f+ scale*0.03f)), 2.0f) );
    //res = opU( res, to_float4_aw( sdEgg( pos,to_float3(-hmov*0.1f-1.5f,0.62f + scale*0.02f,0.0f),hmov*0.2f,to_float3(1.0f+ scale*0.03f, 0.8f-scale*0.03f, 1.0f+ scale*0.03f)), 3.0f) );
    //res = opU( res, to_float4_aw( sdEgg( pos,to_float3(-hmov*0.1f+1.5f,0.62f+ scale*0.02f,0.0f),hmov*0.2f,to_float3(1.0f+ scale*0.03f, 0.8f- scale*0.03f, 1.0f+ scale*0.03f)), 4.0f) );
    
    float hmov1  = hmov*DanceMovEggs[0].x;
    float scale1 = scale*DanceMovEggs[0].y;

    float hmov2  = hmov*DanceMovEggs[1].x;
    float scale2 = scale*DanceMovEggs[1].y;
    
    float hmov3  = hmov*DanceMovEggs[2].x;
    float scale3 = scale*DanceMovEggs[2].y;
    
float zzzzzzzzzzzzzzzz;    
    res = opU( res, to_float4_aw( sdEgg( pos,to_float3(-hmov1*0.1f,0.62f + scale1*0.02f,0.0f)+MovEggs[0],
                                             hmov1*0.2f,
                                             to_float3(1.0f+ scale1*0.03f, 0.8f - scale1*0.03f, 1.0f+ scale1*0.03f)), 2.0f) );
    res = opU( res, to_float4_aw( sdEgg( pos,to_float3(-hmov2*0.1f-1.5f,0.62f + scale2*0.02f,0.0f)+MovEggs[1],
                                             hmov2*0.2f,
                                             to_float3(1.0f+ scale2*0.03f, 0.8f-scale2*0.03f, 1.0f+ scale2*0.03f)), 3.0f) );
    res = opU( res, to_float4_aw( sdEgg( pos,to_float3(-hmov3*0.1f+1.5f,0.62f+ scale3*0.02f,0.0f)+MovEggs[2],
                                             hmov3*0.2f,
                                             to_float3(1.0f+ scale3*0.03f, 0.8f- scale3*0.03f, 1.0f+ scale3*0.03f)), 4.0f) );

    

    
    return res;
}

__DEVICE__ float4 raycast( in float3 ro, in float3 rd, float iTime, float2 DanceMovEggs[3], float3 MovEggs[3] )
{
    float4 res = to_float4(-1.0f,-1.0f,-1.0f,-1.0f);

    float tmax = 20.0f;

    float tp1 = (-ro.y)/rd.y;
    if( tp1>0.0f )
    {
        tmax = _fminf( tmax, tp1 );
        res = to_float4( tp1, 0.0f,0.0f,1.0f);
    }
    
     
    float t = 0.0f;
    for( int i=0; i<70 && t<tmax; i++ )
    {
        float4 h = map( ro+rd*t, iTime, DanceMovEggs, MovEggs );
        if( _fabs(h.x)<(0.0001f*t) )
        { 
            res = to_float4(t, h.y, h.z, h.w); 
            break;
        }
        t += h.x/2.0f;
    }
    
    return res;
}

__DEVICE__ float softshadow( in float3 ro, in float3 rd, float iTime, float2 DanceMovEggs[3], float3 MovEggs[3])
{   
    float t = 0.1f;
    float tmax = 30.0f;
    float res = 1.0f;
    for(int i=0; i<256; ++i)
    {
        float3 p = ro + rd*t;
        float d = map(p, iTime, DanceMovEggs, MovEggs).x;
        
        res = _fminf(res, 8.0f * d/t);
        if(d < 0.0001f || t > tmax) break;
        
        t+=d;
    }
    return clamp(res,0.0f,1.0f);
}

__DEVICE__ float3 calcNormal( in float3 p, float iTime, float2 DanceMovEggs[3], float3 MovEggs[3] )
{
    float2 e = to_float2(0.01f,0.0f);
  return normalize( to_float3( map( p + swi3(e,x,y,y), iTime, DanceMovEggs, MovEggs ).x - map( p - swi3(e,x,y,y), iTime, DanceMovEggs, MovEggs ).x,
                               map( p + swi3(e,y,x,y), iTime, DanceMovEggs, MovEggs ).x - map( p - swi3(e,y,x,y), iTime, DanceMovEggs, MovEggs ).x,
                               map( p + swi3(e,y,y,x), iTime, DanceMovEggs, MovEggs ).x - map( p - swi3(e,y,y,x), iTime, DanceMovEggs, MovEggs ).x
                             ));
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, float iTime, float2 DanceMovEggs[3], float3 MovEggs[3] )
{
  float occ = 0.0f;
    float sca = 1.0f;
    for( int i=0; i<5; i++ )
    {
        float h = 0.01f + 0.12f*(float)(i)/4.0f;
        float d = map( pos + h*nor, iTime, DanceMovEggs, MovEggs ).x;
        occ += (h-d)*sca;
        sca *= 0.8f;
        if( occ>0.35f ) break;
    }
    return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f );
}

__DEVICE__ float triangularSignal(in float x, in float freq, in float amp)
{
    return _fabs((mod_f(x*freq,2.0f)-1.0f)*amp);
}

__DEVICE__ float3 render( in float3 ro, in float3 rd, float iTime, __TEXTURE2D__ iCh[3], float2 TexPos[3], float TexScale[3], float ratio, float MixTex[3], float2 DanceMovEggs[3], float3 MovEggs[3], float3 Colors[6])
{ 
    // background
    //float3 col = to_float3(0.7f, 0.7f, 0.9f)+Colors[0] - _fmaxf(rd.y,0.0f)*0.3f;
    float3 col = Colors[0] - _fmaxf(rd.y,0.0f)*0.3f;
float zzzzzzzzzzzzzzzzzz;     
    float4 res = raycast(ro,rd, iTime, DanceMovEggs, MovEggs);
    float t = res.x;
    float m = res.w;
    float2 uvs = swi2(res,y,z);
    if( m>-0.5f )
    {
        float3 pos = ro + t*rd;
        float3 nor = (m<1.5f) ? to_float3(0.0f,1.0f,0.0f) : calcNormal( pos, iTime, DanceMovEggs, MovEggs );
        float3 ref = reflect( rd, nor );
        
        // specular coeff        
        float ks = 0.0f;
        
        //ground
        if( m<1.5f )
        {
            col = Colors[1];//to_float3(0.1f,0.2f,0.3f)+Colors[1];
            ks = 0.1f;
        }
        //yellow egg
        else if( m<2.5f )
        {
            col = to_float3(0.4f,0.3f,0.01f);
            ks = 1.4f;
            
            col = _mix(col, to_float3(0.0f,0.05f,0.2f), smoothstep(-0.21f,-0.2f, uvs.y) * smoothstep(-0.12f, -0.125f, uvs.y));
            col = _mix(col, to_float3(0.0f,0.2f,0.2f), smoothstep(-0.105f,-0.10f,uvs.y) * smoothstep(-0.005f, -0.01f, uvs.y));
            
            float redstripefunc = triangularSignal(uvs.x,30.0f,0.04f);
            float redstripefactor = smoothstep( uvs.y-0.155f,uvs.y-0.15f, redstripefunc) * smoothstep( uvs.y-0.1f, uvs.y-0.105f, redstripefunc );
            col = _mix(col, to_float3(0.3f,0.0f,0.0f), redstripefactor);
            ks = _mix(ks, 0.6f, redstripefactor);
            
            float bluestripefunc = triangularSignal(uvs.x,30.0f,0.04f);
            float bluestripefactor = smoothstep( uvs.y-0.225f, uvs.y-0.22f, bluestripefunc) * smoothstep( uvs.y-0.185f, uvs.y-0.19f, bluestripefunc);
            col = _mix(col, to_float3(0.1f,0.3f,0.5f)*0.4f, bluestripefactor);
            ks = _mix(ks, 0.6f, bluestripefactor);
                        
            float pinkdotsfunc = smoothstep(0.5f, 0.6f,(_fabs(_sinf(uvs.x*70.0f))*_sinf(uvs.y*55.0f-0.5f)));
            float pinkdotsfactor = pinkdotsfunc * step(0.0f, uvs.y)*step(uvs.y,0.07f);
            col = _mix(col, to_float3(0.5f,0.4f,0.4f), pinkdotsfactor );
            ks = _mix(ks, 0.6f, pinkdotsfactor);
            
            float reddotsfunc = smoothstep(0.5f, 0.6f,(_fabs(_sinf(uvs.x*70.0f-1.5f))*_sinf(uvs.y*50.0f-2.0f)));
            float reddotsfactor = reddotsfunc * step(-0.1f, uvs.y)*step(uvs.y,0.0f);
            col = _mix(col, to_float3(0.5f,0.0f,0.0f), reddotsfactor);
            ks = _mix(ks, 0.6f, reddotsfactor);
            
            //if(TexPos[0].x != 0.0f)
            {
              col = _mix(col, swi3(_tex2DVecN(iCh[0],((uvs.x*ratio)+TexPos[0].x)*TexScale[0],(uvs.y+TexPos[0].y)*TexScale[0],15),x,y,z),MixTex[0]);
              ks = _mix(ks,0.8f,MixTex[0]);
            }  
            
        }
        else if( m<3.5f )
        {
            ks = 0.8f;
            col = to_float3(0.2f,0.0f,0.3f);
            
            float gl1 = smoothstep(-0.31f,-0.3f, uvs.y) * smoothstep(-0.23f, -0.24f, uvs.y);
            float gl2 = smoothstep(-0.11f,-0.105f, uvs.y) * smoothstep(-0.04f, -0.045f, uvs.y);
            float gl3 = smoothstep(0.08f,0.085f, uvs.y) * smoothstep(0.135f, 0.13f, uvs.y);
            float gl4 = smoothstep(0.23f,0.235f, uvs.y) * smoothstep(0.3f  , 0.295f, uvs.y);
            float gl5 = smoothstep(0.34f,0.345f, uvs.y) * smoothstep( 0.365f, 0.36f, uvs.y);
            
            col = _mix(col, to_float3_s(0.3f),gl1);
            ks = _mix(ks, 0.5f, gl1);
            col = _mix(col, to_float3_s(0.3f),gl2);
            ks = _mix(ks, 0.5f, gl2);
            col = _mix(col, to_float3_s(0.3f),gl3);
            ks = _mix(ks, 0.5f, gl3);
            col = _mix(col, to_float3_s(0.3f),gl4);
            ks = _mix(ks, 0.5f, gl4);
            col = _mix(col, to_float3(0.5f, 0.2f, 0.2f),gl5);
            ks = _mix(ks, 0.5f, gl5);
            
            float pinkdotsbot = smoothstep(0.7f, 0.8f,(_sinf(uvs.x*30.0f)*_sinf(uvs.y*20.0f-1.5f)));
            float pinkdotsbotfactor = pinkdotsbot * step(-0.2f, uvs.y)*step(uvs.y,-0.1f);
            col = _mix(col, to_float3(0.5f, 0.2f, 0.2f), pinkdotsbotfactor );
            ks = _mix(ks, 0.5f, pinkdotsbotfactor);
            
            float yellowdotsbot = smoothstep(0.7f, 0.8f,(_sinf(uvs.x*30.0f+3.2f)*_sinf(uvs.y*20.0f-1.5f)));
            float yellowdotsbotfactor = yellowdotsbot * step(-0.2f, uvs.y)*step(uvs.y,-0.1f);
            col = _mix(col, to_float3(0.6f, 0.4f, 0.0f), yellowdotsbotfactor );
            ks = _mix(ks, 0.5f, yellowdotsbotfactor);
            
            float pinkdotstop = smoothstep(0.7f, 0.8f,(_sinf(uvs.x*30.0f)*_sinf(uvs.y*20.0f-2.1f)));
            float pinkdotstopfactor =pinkdotstop * step(0.1f, uvs.y)*step(uvs.y,0.3f);
            col = _mix(col, to_float3(0.5f, 0.2f, 0.2f), pinkdotstopfactor);
            ks = _mix(ks, 0.5f, pinkdotstopfactor);
            
            float yellowdotstop = smoothstep(0.7f, 0.8f,(_sinf(uvs.x*30.0f+3.2f)*_sinf(uvs.y*20.0f-2.1f)));
            float yellowdotstopfactor = yellowdotstop * step(0.1f, uvs.y)*step(uvs.y,0.3f);
            col = _mix(col, to_float3(0.6f, 0.4f, 0.0f), yellowdotstopfactor);
            ks = _mix(ks, 0.5f, yellowdotstopfactor);
            
            float pinkwave = _sinf(uvs.x*50.0f)*0.04f;
            float pinkwavefactor = smoothstep( uvs.y-0.03f, uvs.y-0.025f, pinkwave) * smoothstep( uvs.y-0.01f, uvs.y-0.015f, pinkwave);
            col = _mix(col, to_float3(0.5f, 0.2f, 0.2f), pinkwavefactor);
            ks = _mix(ks, 0.5f, pinkwavefactor);
            
            //if(TexPos[1].x != 0.0f)
            {
              //col = swi3(_tex2DVecN(iCh[1],((uvs.x*ratio)+TexPos[1].x)*TexScale[1],(uvs.y+TexPos[1].y)*TexScale[1],15),x,y,z);
              col = _mix(col,swi3(_tex2DVecN(iCh[1],((uvs.x*ratio)+TexPos[1].x)*TexScale[1],(uvs.y+TexPos[1].y)*TexScale[1],15),x,y,z),MixTex[1]);
              ks = _mix(ks,0.8f,MixTex[1]);
            }
            
        }
        else if( m<4.5f )
        {
            col = _mix(col, to_float3(0.7f, 0.6f, 0.1f)*0.4f, smoothstep(0.3f,0.5f,(_sinf(uvs.x*170.0f+3.2f)*_sinf(uvs.y*120.0f-2.1f))));
            ks = 0.8f;
            
            //if(TexPos[2].x != 0.0f)
            {
              col = _mix(col, swi3(_tex2DVecN(iCh[2],((uvs.x*ratio)+TexPos[2].x)*TexScale[2],(uvs.y+TexPos[2].y)*TexScale[2],15),x,y,z), MixTex[2]);
            }  
        }

        // lighting taken from iq primitives shader https://www.shadertoy.com/view/Xds3zN
        float occ = calcAO( pos, nor, iTime, DanceMovEggs, MovEggs );
        
        float3 lin = to_float3_s(0.0f);

        // sun
        {
            float3  lig = normalize(Colors[2] );// to_float3(0.5f, 0.4f, 0.5f)+Colors[2] );
            float3  hal = normalize( lig-rd );
            float dif = clamp( dot( nor, lig ), 0.0f, 1.0f );
                  dif *= softshadow( pos, lig, iTime, DanceMovEggs, MovEggs);
            float spe = _powf( clamp( dot( nor, hal ), 0.0f, 1.0f ),16.0f);
                  spe *= dif;
                  spe *= 0.04f+0.96f*_powf(clamp(1.0f-dot(hal,lig),0.0f,1.0f),5.0f);
            lin += col*2.20f*dif*to_float3(1.30f,1.00f,0.70f);
            lin +=     15.00f*spe*to_float3(1.30f,1.00f,0.70f)*ks;
            lin *= 0.5f;
        }
        // sky
        {
            float dif = _sqrtf(clamp( 0.5f+0.5f*nor.y, 0.0f, 1.0f ));
                  dif *= occ;
            float spe = smoothstep( -0.2f, 0.2f, ref.y );
                  spe *= dif;
                  spe *= 0.04f+0.96f*_powf(clamp(1.0f+dot(nor,rd),0.0f,1.0f), 5.0f );
                  spe *= softshadow( pos, ref, iTime, DanceMovEggs, MovEggs);
            lin += col*0.80f*dif*to_float3(0.40f,0.60f,1.15f);
            lin +=     2.00f*spe*to_float3(0.40f,0.60f,1.30f)*ks;
        }
        // back
        {
          //float dif = clamp( dot( nor, normalize(to_float3(0.5f,0.0f,0.6f)+Colors[3])), 0.0f, 1.0f )*clamp( 1.0f-pos.y,0.0f,1.0f);
          float dif = clamp( dot( nor, normalize(Colors[3])), 0.0f, 1.0f )*clamp( 1.0f-pos.y,0.0f,1.0f);
                  dif *= occ;
          lin += col*1.0f*dif*(Colors[4]);//to_float3(0.1f,0.2f,0.3f)+Colors[4]);
        }
        // sss
        {
            float dif = _powf(clamp(1.0f+dot(nor,rd),0.0f,1.0f),2.0f);
                  dif *= occ;
            lin += col*0.25f*dif*to_float3(1.00f,1.00f,1.00f);
        }
        
        col = lin;

        //horizon
        //col = _mix( col, to_float3(0.7f,0.7f,0.9f)+Colors[5], 1.0f-_expf( -0.0001f*t*t*t ) );
        col = _mix( col, Colors[5], 1.0f-_expf( -0.0001f*t*t*t ) );
    }

  return ( clamp(col,0.0f,1.0f) );
}

__KERNEL__ void EasterEggsJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime, int iFrame,float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    CONNECT_POINT0(TexPos1, 0.0f, 0.0f );
    CONNECT_SLIDER0(TexScale1, -10.0f, 10.0f, 1.0f);

    CONNECT_POINT1(TexPos2, 0.0f, 0.0f );
    CONNECT_SLIDER1(TexScale2, -10.0f, 10.0f, 1.0f);

    CONNECT_POINT2(TexPos3, 0.0f, 0.0f );
    CONNECT_SLIDER2(TexScale3, -10.0f, 10.0f, 1.0f);
    
    CONNECT_SLIDER3(MixTex1, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER4(MixTex2, 0.0f, 1.0f, 0.0f);
    CONNECT_SLIDER5(MixTex3, 0.0f, 1.0f, 0.0f);
    
    
    CONNECT_CHECKBOX0(DTOn, 0);
    CONNECT_SLIDER5(dt, 0.0f, 1.0f, 0.0f);

    CONNECT_SLIDER6(HMovEgg1, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER7(ScaleEgg1, 0.0f, 10.0f, 1.0f);

    CONNECT_SLIDER8(HMovEgg2, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER9(ScaleEgg2, 0.0f, 10.0f, 1.0f);

    CONNECT_SLIDER10(HMovEgg3, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER11(ScaleEgg3, 0.0f, 10.0f, 1.0f);
    //CONNECT_POINT3(DanceMovEgg1, 0.0f, 0.0f );
    //CONNECT_POINT4(DanceMovEgg2, 0.0f, 0.0f );
    //CONNECT_POINT5(DanceMovEgg3, 1.0f, 1.0f );
    
    CONNECT_POINT3(PosXYEgg1, 0.0f, 0.0f );
    CONNECT_SLIDER12(PosZEgg1, -30.0f, 10.0f, 0.0f);
    CONNECT_POINT4(PosXYEgg2, 0.0f, 0.0f );
    CONNECT_SLIDER13(PosZEgg2, -30.0f, 10.0f, 0.0f);
    CONNECT_POINT5(PosXYEgg3, 0.0f, 0.0f );
    CONNECT_SLIDER14(PosZEgg3, -30.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER15(taViewZ, -30.0f, 30.0f, 0.0f);
    
    CONNECT_POINT6(roViewXY, 0.0f, 0.0f );
    CONNECT_SLIDER16(roViewZ, -30.0f, 30.0f, 0.0f);
    
    CONNECT_COLOR0(Sky, 0.7f, 0.7f, 0.9f, 1.0f);
    CONNECT_COLOR1(Ground, 0.1f, 0.2f, 0.3f, 1.0f);
    CONNECT_COLOR2(Sunlight, 0.5f, 0.4f, 0.5f, 1.0f);
    CONNECT_COLOR3(Background1, 0.5f, 0.0f, 0.6f, 1.0f);
    CONNECT_COLOR4(Background2, 0.1f, 0.2f, 0.3f, 1.0f);    
    CONNECT_COLOR5(Horizon, 0.7f, 0.7f, 0.9f, 1.0f);  

    float3 Colors[6] = {swi3(Sky,x,y,z),swi3(Ground,x,y,z),swi3(Sunlight,x,y,z),swi3(Background1,x,y,z),swi3(Background2,x,y,z),swi3(Horizon,x,y,z)};

    float2 DanceMovEggs[3] = {to_float2(HMovEgg1,ScaleEgg1),to_float2(HMovEgg2,ScaleEgg2),to_float2(HMovEgg3,ScaleEgg3)};
    float3 MovEggs[3]      = {to_float3_aw(PosXYEgg1,PosZEgg1),to_float3_aw(PosXYEgg2,PosZEgg2),to_float3_aw(PosXYEgg3,PosZEgg3)};

    
    __TEXTURE2D__ iCh[3] = {iChannel1,iChannel2,iChannel3};
    float2 TexPos[3] = { TexPos1, TexPos2,TexPos3};
    float TexScale[3] = {TexScale1, TexScale2,TexScale3};
    
    float MixTex[3] = {MixTex1,MixTex2, MixTex3};
    
    float ratio = iResolution.y/iResolution.x;
    float2 taViewXY = swi2(iMouse,x,y)/iResolution - 0.5f;

    float2 scrPos = fragCoord/iResolution;
    scrPos = scrPos*2.0f - 1.0f;
    scrPos.x *= iResolution.x / iResolution.y;
float IIIIIIIIIIIIIIIII;    
    float3 ta = to_float3( 0.0f, 0.8f, 0.0f ) + to_float3_aw(taViewXY,taViewZ);
    float3 ro = to_float3(0.0f, 1.0f, 3.5f) + to_float3_aw(roViewXY,roViewZ);
    
    float3 f = normalize(ta-ro);
    float3 r = normalize(cross(f,to_float3(0.0f,1.0f,0.0f)));
    float3 t = normalize(cross(r,f));
    float3 rd = normalize( (scrPos.x*r+scrPos.y*t+f*2.5f) );
     
    float3 col = render( ro, rd, iTime, iCh, TexPos, TexScale,ratio, MixTex, DanceMovEggs, MovEggs, Colors);

    fragColor = to_float4_aw( pow_f3( col, to_float3_s(0.4545f)) , 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}