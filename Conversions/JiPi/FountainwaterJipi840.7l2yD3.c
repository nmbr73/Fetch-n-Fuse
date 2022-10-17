
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Abstract 3' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


__DEVICE__ float3 __refract_f3(float3 I, float3 N, float ior, float mul, float off) {
    //float cosi = clamp(dot(N,I), -1.0f,1.0f);  //clamp(-1, 1, I.dot(N));
    float cosi = clamp( -1.0f,1.0f,dot(N,I));    //clamp(-1, 1, I.dot(N));
    float etai = 01.0f, etat = ior*1.0f;
    float3 n = N;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        float temp = etai;
        etai = etat;
        etat = temp;
        n = -N;
    }
    float eta = etai / etat;
    float k = 1.0f - (eta * eta) * (1.0f - (cosi * cosi));
    if (k <= 0) {
        return to_float3_s(0.0f);
    } else {
        //return I.multiply(eta).add(n.multiply(((eta * cosi) - Math.sqrt(k))));
        //return I*eta+n*((eta*cosi)-_sqrtf(k));  //!!
	      return eta * I + (eta * cosi - _sqrtf(1.0f-k)) * N * mul + off;
    }
}


__DEVICE__ void sinF(float3 p, float freq, float amp, float phase, out float *f, out float3 *df) {
    float m = length(swi2(p,x,z));
    *f = p.y-_cosf(freq*(m-phase))*amp*_fminf(1.0f, 6.0f/m);
    (*df).y = 1.0f;
    (*df).x = p.x/m;
    (*df).z = p.z/m;
    swi2S(*df,x,z, swi2(*df,x,z) * freq*_sinf(freq*(m-phase))*amp*_fminf(1.0f, 20.0f/m));
  
}

__DEVICE__ float map(float3 p, float iTime) {
    float3 p1 = p-to_float3(-2.0f,0.0f,0.0f);
    float f;
    float3 df;
    sinF(p1, 1.0f, 2.0f, 10.0f*iTime, &f, &df);
    return (f)/length(df);
}

__DEVICE__ float3 CalcNormal(float3 p,float iTime) {
    float2 e = to_float2(0.0f, 0.001f);
    float3 n;
    n.x = map(p+swi3(e,y,x,x),iTime) - map(p-swi3(e,y,x,x),iTime);
    n.y = map(p+swi3(e,x,y,x),iTime) - map(p-swi3(e,x,y,x),iTime);
    n.z = map(p+swi3(e,x,x,y),iTime) - map(p-swi3(e,x,x,y),iTime);
    return normalize(n);
}

__DEVICE__ float raymarch(float3 ro, float3 rd,float iTime) {
    float t = 0.001f;
    for (int i=0; i<1000; i++) {
        float h = map(ro+t*rd,iTime);
        if (_fabs(h)<0.0001f*t)
            return t;
        t += 1.0f*h;
        if (t>80.0f) return -1.0f;
    }
}

__DEVICE__ float softShadow(float3 ro, float3 rd,float iTime) {
    float sha = 1.0f;
    float t = 0.01f;
    for (int i=0; i<256; i++) {
      float h = map(ro+t*rd,iTime);
        sha = _fminf(sha, 2.0f*h/t);
        t += 1.0f*clamp(h,0.02f,0.20f);
        if (t>16.0f) break;
    }
    sha = clamp(sha, 0.0f, 1.0f);
    return sha*sha*(3.0f-2.0f*sha);
}

__DEVICE__ mat2 rotMat(float ang) {
    return to_mat2(_cosf(ang), _sinf(ang), -_sinf(ang), _cosf(ang));
}

__DEVICE__ float fresnel(float3 rd, float3 normal) {
    float cosf = clamp(dot(rd, normal), -1.0f, 1.0f);
    float n1, n2;
    if (cosf < 0.0f) {
        n1 = 1.0f;
        n2 = 1.25f;
        cosf = -cosf;
    }
    else {
        n1 = 1.25f;
        n2 = 1.0f;
    }
    float sinf = n1/n2 * _sqrtf(_fmaxf(0.0f, 1.0f-cosf*cosf));
    if (sinf>=1.0f) {
        return 1.0f;
    }
    else {
        float cost = _sqrtf(_fmaxf(0.0f, 1.0f-sinf*sinf));
        float Rs = ((n1 * cosf) - (n2 * cost)) / ((n1 * cosf) + (n2 * cost));
        float Rp = ((n2 * cosf) - (n1 * cost)) / ((n2 * cosf) + (n1 * cost));
        return (Rs * Rs + Rp * Rp) /3.0f;
    }
}

__KERNEL__ void FountainwaterJipi840Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

    CONNECT_COLOR0(Color2, 0.9f, 0.6f, 0.2f, 1.0f);
    //float4 _Color2 = to_float4_v(_shaderParameters->ctrlColor[0]); // was ist anders ?????? Hier geht's nicht !!!!!
    
    CONNECT_COLOR1(AndererName, 0.9f, 0.6f, 0.2f, 1.0f);

    CONNECT_SLIDER0(Mul, -20.0f, 20.0f, 1.0f);
    CONNECT_SLIDER1(Off, -20.0f, 20.0f, 0.0f);
    
    CONNECT_SLIDER2(RotY, -20.0f, 20.0f, -0.9f);
    CONNECT_SLIDER3(RotX, -20.0f, 20.0f, 0.0f);
    
    CONNECT_POINT0(RoXY,0.0f,15.0f);
    CONNECT_SLIDER4(RoZ, -100.0f, 100.0f, -20.0f);
    
    CONNECT_SLIDER5(waterDepth, -10.0f, 10.0f, 2.0f);
    
    CONNECT_SLIDER6(DebRot,  0.0f, 1.0f, 0.9f);
    CONNECT_SLIDER7(DebGelb, 0.0f, 1.0f, 0.6f);
    CONNECT_SLIDER8(DebBlau, 0.0f, 1.0f, 0.2f);

    float2 uv = (2.0f*fragCoord - iResolution) / iResolution.y;
    //float3 ro = to_float3(0.0f,15.0f,-20.0f);
    float3 ro = to_float3(RoXY.x,RoXY.y,RoZ);
    
    float3 rd = normalize(to_float3_aw(uv, 1.0f));
    //swi2S(rd,z,y, mul_mat2_f2(rotMat(-0.9f) , swi2(rd,z,y)));
    swi2S(rd,z,y, mul_mat2_f2(rotMat(RotY) , swi2(rd,z,y)));
    swi2S(rd,z,x, mul_mat2_f2(rotMat(RotX) , swi2(rd,z,x)));


    
    float3 lig = normalize(to_float3(0.5f, 1.0f, -0.5f));
    
    float r = raymarch(ro,rd, iTime);
    
    float3 backCol2 = to_float3(0.9f, 0.6f, 0.2f);
    float3 backCol = swi3(texture(iChannel1, uv),x,y,z);//floorPos*0.04f),x,y,z)
    
    //float waterDepth = 2.0f;
    float3 col;
    if (r>=0.0f) {
      float3 pos = ro + r*rd;
      float3 normal = CalcNormal(pos,iTime);
     
        float glow = _fmaxf(0.0f, dot(reflect(rd, normal), lig));
        glow = _powf(glow, 50.0f);
        float fres = fresnel(rd, normal);
        float fresOp = 1.0f-fres;
        float3 refVec = __refract_f3(rd, normal, 1.0f/1.5f, Mul, Off);
        float2 floorPos = swi2(refVec,x,z) / refVec.y * (pos.y+waterDepth) + swi2(pos,x,z);
        
        if (pos.y+waterDepth<0.0f) floorPos = to_float2_s(0.0f);
      
        col += swi3(texture(iChannel0, floorPos*0.04f),x,y,z)*1.2f;
        col += to_float3_s(glow);
        col *= fresOp;
        
        //col = col + fres*backCol2;
        
        float4 _Color2 = to_float4_v(_shaderParameters->ctrlColor[0]); // was ist anders ??????
        col = col + fres*swi3(_Color2,x,y,z); //-> führt zu Fehler
        //col = col + to_float3(fres*Color2.x,fres*Color2.y,fres*Color2.z); //NotOkay
        //col += fres*Deb;  //Okay
        
        //col += fres*to_float3(DebRot,DebGelb,DebBlau); //Okay
        //col = col + fres*to_float3(Color2.x,Color2.y,Color2.z); //NotOkay
        
        //col = col + fres*Color2.x;  //NotOKAY
        //col = col + fres*_shaderParameters->ctrlColor[1][0];  //OKAY !!!
        
        //float4 test = to_float4_v(_shaderParameters->ctrlColor[1]); // und hier geht's ???????????????????????????? Okay !!!
        //col = col + fres*swi3(test,x,y,z);  //OKAY !!!
        
         
    }
    else {
        col = backCol;
    }
    
    col = pow_f3(col,to_float3_s(0.8f));
    
    fragColor = to_float4_aw(col, 1.0f);

  SetFragmentShaderComputedColor(fragColor);
}