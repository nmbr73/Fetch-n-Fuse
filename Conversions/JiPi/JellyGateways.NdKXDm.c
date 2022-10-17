
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define MDIST 100.0f
#define STEPS 128.0f
#define pi 3.1415926535f
#define rot(a) to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a))
#define pmod(p,x) (mod_f(p,x)-0.5f*(x))



__DEVICE__ float3 hsv(float3 c){
    float4 K = to_float4(1.0f, 2.0f / 3.0f, 1.0f / 3.0f, 3.0f);
    float3 p = abs_f3(fract_f3(swi3(c,x,x,x) + swi3(K,x,y,z)) * 6.0f - swi3(K,w,w,w));
    return c.z * _mix(swi3(K,x,x,x), clamp(p - swi3(K,x,x,x), 0.0f, 1.0f), c.y);
}
//iq box sdf
__DEVICE__ float ebox(float3 p, float3 b){
  float3 q = abs_f3(p) - b;
  return length(_fmaxf(q,to_float3_s(0.0f))) + _fminf(max(q.x,_fmaxf(q.y,q.z)),0.0f);
}
__DEVICE__ float ebox(float2 p, float2 b){
  float2 q = abs_f2(p) - b;
  return length(_fmaxf(q,to_float2_s(0.0f))) + _fminf(max(q.x,q.y),0.0f);
}

__DEVICE__ float lim(float p, float s, float lima, float limb){
    return p-s*clamp(round(p/s),lima,limb);
}
__DEVICE__ float idlim(float p, float s, float lima, float limb){
    return clamp(round(p/s),lima,limb);
}

__DEVICE__ float dibox(float3 p,float3 b,float3 rd){
    float3 dir = sign_f3(rd)*b;   
    float3 rc = (dir-p)/rd;
    float dc = _fminf(rc.y,rc.z)+0.01f;
    return dc;
}
__DEVICE__ float easeOutBounce(float x) {
    float n1 = 7.5625f;
    float d1 = 2.75f;
    if (x < 1.0f / d1) {
        return n1 * x * x;
    } 
    else if (x < 2.0f / d1) {
        return n1 * (x -= 1.5f / d1) * x + 0.75f;
    } 
    else if (x < 2.5f / d1) {
        return n1 * (x -= 2.25f / d1) * x + 0.9375f;
    } 
    else {
        return n1 * (x -= 2.625f / d1) * x + 0.984375f;
    }
}
__DEVICE__ float3 map(float3 p, float iTime, float3 rdg){
    float t = -iTime*0.8f;
    float3 rd2 = rdg;
    float2 a = to_float2_s(1);
    float2 b = to_float2_s(2);
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(t*0.3f*pi/3.0f)));
    swi2S(rd2,x,z, mul_f2_mat2(swi2(rd2,x,z),rot(t*0.3f*pi/3.0f)));
    //swi2(p,x,z)*=rot(pi/4.0f);
    //swi2(rd2,x,z)*=rot(pi/4.0f); 
    float3 po = p;
    float dsz = 0.45f;
    float m = 2.42f-dsz;
    float bs = 1.0f-dsz*0.5f;
    
    //VERTIAL TRANSLATION
    p.y+=t*m;
    
    //VERTIAL REP
    float id1 = _floor(p.y/m);
    p.y = pmod(p.y,m);
    
    //ROTATE EACH LAYER
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z),rot(id1*pi/2.0f)));
    swi2S(rd2,x,z, mul_f2_mat2(swi2(rd2,x,z),rot(id1*pi/2.0f)));

    float3 p2 = p; //dibox p1
    
    //Auxillary boxes positions
    float3 p3 = p;
    float3 rd3 = rd2;
     
    swi2S(p3,x,z, mul_f2_mat2(swi2(p3,x,z),rot(pi/2.0f)));
    swi2S(rd3,x,z, mul_f2_mat2(swi2(rd3,x,z),rot(pi/2.0f)));
    float3 p4 = p3; 
    
    //HORIZONTAL REP
    p2.z = pmod(p2.z-m*0.5f,m);
    p4.z = pmod(p4.z-m*0.5f,m);
    
    float cnt = 100.0f;
    float id2 = idlim(p.z,m,-cnt,cnt);
    float id3 = idlim(p3.z,m,-cnt,cnt);
    p.z = lim(p.z,m,-cnt,cnt);
    p3.z = lim(p3.z,m,-cnt,cnt);
    
    //CLOSING ANIMATION 
    float close = _fmaxf((id1-t)*1.0f,-2.0f);
    float close2 = clamp(_fmaxf((id1-t-0.3f)*1.0f,-2.0f)*1.4f,0.0f,1.0f);
    close+=id2*0.025f;
    close = clamp(close*1.4f,0.0f,1.0f);
    close = easeOutBounce(close);
    //close = 1.0f-easeOutBounce(1.0f-close);

    //CLOSING OFFSET
    p.x = _fabs(p.x)-34.5f*0.5f-0.25f*7.0f;
    p.x-=close*34.5f*0.52f-0.055f;
    
    p3.x = _fabs(p3.x)-36.5f;

    p.x-=((id1-t)*0.55f)*close*2.4f;
    p3.x-=((id1-t)*0.55f)*close2*2.4f;
    //WAVEY
    p.x+=(_sinf(id1+id2-t*6.0f)*0.18f+4.0f)*close*2.4f;
    p3.x+=(_sinf(id1+id3-t*6.0f)*0.18f+4.0f)*smoothstep(0.0f,1.0f,close2)*2.4f;
    
    //BOX SDF
    a = to_float2(ebox(p,to_float3(7.5f*2.5f,bs,bs))-0.2f,id2);
    
    //AUXILLARY BOX
    b = to_float2(ebox(p3,to_float3(7.5f*2.5f,bs,bs))-0.2f,id3);
    
    a=(a.x<b.x)?a:b;
    //ARTIFACT REMOVAL
    float c = dibox(p2,to_float3(1,1,1)*m*0.5f,rd2)+0.1f;
    //ARTIFACT REMOVAL 2
    c = _fminf(c,dibox(p4,to_float3(1,1,1)*m*0.5f,rd3)+0.1f);
    
    float nsdf = a.x;
    
    a.x = _fminf(a.x,c); //Combine artifact removal
    a.y = id1;
    return to_float3_aw(a,nsdf);
}
__DEVICE__ float3 norm(float3 p, float iTime, float3 rdg){
    float2 e = to_float2(0.005f,0);
    return normalize(map(p,iTime,rdg).x-to_float3(
                                      map(p-swi3(e,x,y,y),iTime,rdg).x,
                                      map(p-swi3(e,y,x,y),iTime,rdg).x,
                                      map(p-swi3(e,y,y,x),iTime,rdg).x));
}

__KERNEL__ void JellyGatewaysFuse(float4 fragColor, float2 fragCoord, float iTime, int iFrame, float2 iResolution, float4 iMouse)
{

    CONNECT_SLIDER0(Sync, -100.0f, 100.0f, 37.5f); //Shadertoy 75.0f

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float3 col = to_float3_s(0);
    float3 ro = to_float3(0,13,-5)*1.5f;
    if(iMouse.z>0.0f){
      swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z),rot(1.0f*(iMouse.y/iResolution.y-0.2f))));
      swi2S(ro,z,x, mul_f2_mat2(swi2(ro,z,x),rot(-7.0f*(iMouse.x/iResolution.x-0.5f))));
    }
    float3 lk = to_float3(0,0,0);
    float3 f = normalize(lk-ro);
    float3 r = normalize(cross(to_float3(0,1,0),f));
    float3 rd = normalize(f*(0.5f)+uv.x*r+uv.y*cross(f,r));  
    float3 rdg = rd;
    float3 p = ro;
    float dO = 0.0f;

    float3 d= to_float3_s(0);
    for(float i = 0.0f; i<STEPS; i+=1.0f){
        p = ro+rd*dO;
        d = map(p,iTime,rdg);
        dO+=d.x;
        if(_fabs(d.x)<0.005f){
            break;
        }
        if(dO>MDIST){
            dO = MDIST;
            break;
        }
    }

    {
        float3 ld = normalize(to_float3(0,45,0)-p);
      
        //sss from nusan
        float sss=0.01f;
        for(float i=1.0f; i<20.0f; ++i){
            float dist = i*0.09f;
            sss += smoothstep(0.0f,1.0f,map(p+ld*dist,iTime,rdg).z/dist)*0.023f;
        }
        float3 al = to_float3(0.204f,0.267f,0.373f);
        float3 n = norm(p,iTime,rdg);
        float3 r = reflect(rd,n);
        float diff = _fmaxf(0.0f,dot(n,ld));
        float amb = dot(n,ld)*0.45f+0.55f;
        float spec = _powf(_fmaxf(0.0f,dot(r,ld)),40.0f);
        float fres = _powf(_fabs(0.7f+dot(rd,n)),3.0f);     
        //ao from blackle 
        #define AO(a,n,p) smoothstep(-a,a,map(p+n*a,iTime,rdg).z)
        float ao = AO(0.3f,n,p)*AO(0.5f,n,p)*AO(0.9f,n,p);

        col = al*
            _mix(to_float3(0.169f,0.000f,0.169f),to_float3(0.984f,0.996f,0.804f),_mix(amb,diff,0.75f))
            +spec*0.3f+fres*_mix(al,to_float3_s(1),0.7f)*0.4f;
            col+=sss*hsv(to_float3(fract(d.y*0.5f+d.y*0.1f+0.001f)*0.45f+0.5f,0.9f,1.35f));
            col*=_mix(ao,1.0f,0.85f);
            col = pow_f3(col,to_float3_s(0.75f));
            
        if (d.y < -(float)(iFrame)/Sync ) col=swi3(texture(iChannel0,fragCoord/iResolution),x,y,z);
            
    }
    col = clamp(col,0.0f,1.0f);
    //col = smoothstep(0.0f,1.0f,col);
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}