
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ inline mat3 mat3_sub_mat3( mat3 A, mat3 B) {  
  mat3 C;  

  C.r0 = to_float3(A.r0.x - B.r0.x, A.r0.y - B.r0.y,A.r0.z - B.r0.z);  
  C.r1 = to_float3(A.r1.x - B.r1.x, A.r1.y - B.r1.y,A.r1.z - B.r1.z); 
  C.r2 = to_float3(A.r2.x - B.r2.x, A.r2.y - B.r2.y,A.r2.z - B.r2.z);

  return C;  
  }
    
__DEVICE__ inline mat3 to_mat3_n( float A)  
  {  
	mat3 D;  
	D.r0 = to_float3(A,0.0f,0.0f);
	D.r1 = to_float3(0.0f,A,0.0f);
	D.r2 = to_float3(0.0f,0.0f,A);
	return D;  
  }

#define asin_f2(i) to_float2( _asinf((i).x), _asinf((i).y))
#define asin_f3(i) to_float3( _asinf((i).x), _asinf((i).y), _asinf((i).z))

//CC0 1.0f Universal https://creativecommons.org/publicdomain/zero/1.0f/
//To the extent possible under law, Blackle Mori has waived all copyright and related or neighboring rights to this work.

__DEVICE__ float comp (float3 p, bool Fork) {
    p = asin_f3(sin_f3(p)*0.9f);
    if (Fork) p = acos_f3(sin_f3(p)*0.9f);
    return length(p)-1.0f;
}

__DEVICE__ float3 erot(float3 p, float3 ax, float ro) {
    return _mix(dot(p,ax)*ax,p,_cosf(ro))+_sinf(ro)*cross(ax,p);
}

__DEVICE__ float smin(float a, float b, float k) {
    float h = _fmaxf(0.0f,k-_fabs(b-a))/k;
    return _fminf(a,b)-h*h*h*k/6.0f;
}

__DEVICE__ float4 wrot(float4 p) {
    //return to_float4_aw(dot(p,to_float4(1)), swi3(p,y,z,w) + swi3(p,z,w,y) - swi3(p,w,y,z) - swi3(p,x,x,x))/2.0f;
    return to_float4(dot(p,to_float4_s(1)), p.y + p.z - p.w - p.x, p.z + p.w - p.y - p.x, p.w + p.y - p.z - p.x)/2.0f;
}



__DEVICE__ float scene(float3 p, inout float3 *p2, inout float *doodad, inout float *lazors, inout float *d1, inout float *d2, inout float *d3, float t, float iTime, float bpm, bool Fork ) {
    *p2 = erot(p, to_float3(0,1,0), t);
    *p2 = erot(*p2, to_float3(0,0,1), t/3.0f);
    *p2 = erot(*p2, to_float3(1,0,0), t/5.0f);
 
    float bpt = iTime/60.0f*bpm;
    float4 p4 = to_float4_aw(*p2,0);
    p4=_mix(p4,wrot(p4),smoothstep(-0.5f,0.5f,_sinf(bpt/4.0f)));
    p4 =abs_f4(p4);
    p4=_mix(p4,wrot(p4),smoothstep(-0.5f,0.5f,_sinf(bpt)));
    float fctr = smoothstep(-0.5f,0.5f,_sinf(bpt/2.0f));
    float fctr2 = smoothstep(0.9f,1.0f,_sinf(bpt/16.0f));
    
    if (Fork) fctr = smoothstep(-0.5f,0.5f,_cosf(bpt/2.0f)), fctr2 = smoothstep(0.9f,1.0f,_cosf(bpt/16.0f));
    
    *doodad = length(_fmaxf(abs_f4(p4)-_mix(0.05f,0.07f,fctr),to_float4_s(0.0f))+_mix(-0.1f,0.2f,fctr))-_mix(0.15f,0.55f,fctr*fctr)+fctr2;
    /*
        float4 p4 = to_float4_aw(p2,0);
        p4=wrot(p4);
        p4 = _fabs(p4);
        p4=_mix(p4,wrot(p4),smoothstep(-0.5f,0.5f,_sinf(t)));
        *doodad = length(_fmaxf(_fabs(p4)-0.07f,0)+0.2f)-0.55f;
    }*/

    p.x += _asinf(_sinf(t/80.0f)*0.99f)*80.0f;
    
    *lazors = length(asin_f2(sin_f2(swi2(erot(p,to_float3(1,0,0),t*0.2f),y,z)*0.5f+1.0f))/0.5f)-0.1f;
    *d1 = comp(p,Fork);
    *d2 = comp(erot(p+5.0f, normalize(to_float3(1,3,4)),0.4f),Fork);
    *d3 = comp(erot(p+10.0f, normalize(to_float3(3,2,1)),1.0f), Fork);
    
    if (Fork) *d3 = comp(erot(p+10.0f, normalize(to_float3(1,2,3)),1.0f),Fork);
    
    return _fminf(*doodad,_fminf(*lazors,0.3f-smin(smin(*d1,*d2,0.05f),*d3,0.05f)));
}

__DEVICE__ float3 norm(float3 p, inout float3 *p2, inout float *doodad, inout float *lazors, inout float *d1, inout float *d2, inout float *d3, float t, float iTime, float bpm, bool Fork) {
    float precis = length(p) < 1.0f ? 0.005f : 0.01f;
    mat3 k = mat3_sub_mat3( to_mat3_f3(p,p,p) , to_mat3_n(precis));
    return normalize(scene(p, p2,doodad,lazors,d1,d2,d3,t,iTime,bpm, Fork) 
         - to_float3(scene(k.r0, p2,doodad,lazors,d1,d2,d3,t,iTime,bpm,Fork),scene(k.r1, p2,doodad,lazors,d1,d2,d3,t,iTime,bpm,Fork),scene(k.r2, p2,doodad,lazors,d1,d2,d3,t,iTime,bpm,Fork)));
}

__KERNEL__ void PartyConcertVisuals2020Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{
  
    CONNECT_CHECKBOX1(Invers, 0);
    CONNECT_CHECKBOX2(ApplyColor, 0);
    CONNECT_CHECKBOX3(Fork, 0);
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.5f, 1.0f); 
    CONNECT_SLIDER0(AlphaThres, 0.0f, 1.0f, 1.0f);

    CONNECT_SLIDER1(BeatPerMinute, 0.0f, 190.0f, 125.0f);

    float d1, d2, d3;
    float t;
    float lazors, doodad;
    float3 p2;
    float bpm = BeatPerMinute;//125.0f;

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;

    float bpt = iTime/60.0f*bpm;
    float bp = _mix(_powf(_sinf(fract(bpt)*3.14f/2.0f),20.0f)+_floor(bpt), bpt,0.4f);
    t = bp;
    float3 cam = normalize(to_float3(0.8f+_sinf(bp*3.14f/4.0f)*0.3f,uv.x,uv.y));
    float3 init = to_float3(-1.5f+_sinf(bp*3.14f)*0.2f,0,0)+cam*0.2f;
    init = erot(init,to_float3(0,1,0),_sinf(bp*0.2f)*0.4f);
    init = erot(init,to_float3(0,0,1),_cosf(bp*0.2f)*0.4f);
    
    if (Fork)
      cam = erot(cam,to_float3(0,1,0),_cosf(bp*0.2f)*0.4f),
      cam = erot(cam,to_float3(0,0,1),_sinf(bp*0.2f)*0.4f);      
    else
      cam = erot(cam,to_float3(0,1,0),_sinf(bp*0.2f)*0.4f),
      cam = erot(cam,to_float3(0,0,1),_cosf(bp*0.2f)*0.4f);
    
    
    float3 p = init;
    bool hit = false;
    float atten = 1.0f;
    float tlen = 0.0f;
    float glo = 0.0f;
    float dist;
    float fog = 0.0f;
    float dlglo = 0.0f;
    bool trg = false;
    for (int i = 0; i <80 && !hit; i++) {
        dist = scene(p, &p2,&doodad,&lazors,&d1,&d2,&d3,t, iTime, bpm, Fork);
        hit = dist*dist < 1e-6;
        glo += 0.2f/(1.0f+lazors*lazors*20.0f)*atten;
        dlglo += 0.2f/(1.0f+doodad*doodad*20.0f)*atten;

        bool lengthP2 = _sinf(_powf(length(p2*p2*p2),0.3f)*120.0f)>0.4f;
        if (Fork) lengthP2 = _cosf(_powf(length(p2*p2*p2),0.3f)*120.0f)>0.4f; 

        //if (hit && ((_sinf(d3*45.0f)<-0.4f && (dist!=doodad )) || (dist==doodad && _sinf(_powf(length(p2*p2*p2),0.3f)*120.0f)>0.4f )) && dist != lazors) { //_cosf(pow
        if (hit && ((_sinf(d3*45.0f)<-0.4f && (dist!=doodad )) || (dist==doodad &&  lengthP2  )) && dist != lazors) { //_cosf(pow
        
        trg = trg || dist==doodad;
            hit = false;
            float3 n = norm(p, &p2,&doodad,&lazors,&d1,&d2,&d3,t, iTime, bpm, Fork);
            atten *= 1.0f-_fabs(dot(cam,n))*0.98f;
            cam = reflect(cam,n);
            dist = 0.1f;
        }
        p += cam*dist;
        tlen += dist;
        fog += dist*atten/30.0f;
    }
    fog = smoothstep(0.0f,1.0f,fog);
    bool lz = lazors == dist;
    bool dl = doodad == dist;
    float3 fogcol = _mix(to_float3(0.5f,0.8f,1.2f), to_float3(0.4f,0.6f,0.9f), length(uv));
    float3 n = norm(p, &p2,&doodad,&lazors,&d1,&d2,&d3,t, iTime, bpm, Fork);
    float3 r = reflect(cam,n);
    float ss = smoothstep(-0.3f,0.3f,scene(p+to_float3_s(0.3f), &p2,&doodad,&lazors,&d1,&d2,&d3,t, iTime, bpm, Fork))+0.5f;
    
    float fact = length(sin_f3(r*(dl?4.0f:3.0f))*0.5f+0.5f)/_sqrtf(3.0f)*0.7f+0.3f;
    float3 matcol = _mix(to_float3(0.9f,0.4f,0.3f), to_float3(0.3f,0.4f,0.8f), smoothstep(-1.0f,1.0f,_sinf(d1*5.0f+iTime*2.0f)));
    matcol = _mix(matcol, to_float3(0.5f,0.4f,1.0f), smoothstep(0.0f,1.0f,_sinf(d2*5.0f+iTime*2.0f)));
    if (dl) matcol = _mix(to_float3_s(1),matcol,0.1f)*0.2f+0.1f;
    float3 col = matcol*fact*ss + _powf(fact,10.0f);
    if (lz) col = to_float3_s(4);
    
    float3 fragColorxyz = col*atten + glo*glo + fogcol*glo;
    fragColorxyz = _mix(fragColorxyz, fogcol, fog);
    if(!dl)         fragColorxyz = abs_f3(erot(fragColorxyz, normalize(sin_f3(p*2.0f)),0.2f*(1.0f-fog)));
    if(!trg&&!dl)   fragColorxyz += dlglo*dlglo*0.1f*to_float3(0.4f,0.6f,0.9f);
    fragColorxyz = sqrt_f3(fragColorxyz);
    fragColorxyz = smoothstep(to_float3_s(0),to_float3_s(1.2f),fragColorxyz);

    fragColor = to_float4_aw(fragColorxyz,fragColor.w);

    if (Invers) fragColor = to_float4_s(1.0f) - fragColor;
    if (ApplyColor)
    {
      if (fragColor.x <= AlphaThres)      fragColor.w = Color.w;  

      fragColor = (fragColor + (Color-0.5f));
    }

  SetFragmentShaderComputedColor(fragColor);
}