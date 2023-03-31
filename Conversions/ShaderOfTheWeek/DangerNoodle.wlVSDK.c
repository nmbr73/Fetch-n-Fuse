
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Cubemap: Forest_0' to iChannel0
// Connect Image 'Cubemap: Forest Blurred_0' to iChannel1
// Connect Image 'https://soundcloud.com/extasis_demencial/jungle-sounds' to iChannel2
// Connect Image 'https://soundcloud.com/liam-seagrave/pandemic-prologue-slow-strings-suspense-music-original-horror-piano-composition' to iChannel3


// "Danger Noodle" by Martijn Steinrucken aka BigWings/CountFrolic - 2020
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0f Unported License.
// 
// Email: countfrolic@gmail.com
// Twitter: @The_ArtOfCode
// YouTube: youtube.com/TheArtOfCodeIsCool
//
// Ever since I did a snake scale effect as one of my first ShaderToys
// I have been wanting to do a snake, so here it is.
//
// Watch full screen with sound!

#define MAX_STEPS 200
#define MAX_DIST 60.0f
#define SURF_DIST 0.01f

#define CAM_MOVE 1.0f

#define S smoothstep

#define MAT_TONGUE 1.0f
#define MAT_HEAD   2.0f
#define MAT_BODY   3.0f
#define MAT_EYE    4.0f

// From Dave Hoskins
__DEVICE__ float2 Hash22(float2 p) {
    float3 p3 = fract_f3(swi3(p,x,y,x) * to_float3(0.1031f, 0.1030f, 0.0973f));
    p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
    return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

__DEVICE__ float Hash21(float2 p) {
    p = fract_f2(p*to_float2(123.1031f, 324.1030f));
    p += dot(p, p+33.33f);
    return fract(p.x*p.y);
}

__DEVICE__ float sabs(float x,float k) {
    float a = (0.5f/k)*x*x+k*0.5f;
    float b = _fabs(x);
    return b<k ? a : b;
}

__DEVICE__ float2 RaySphere(float3 ro, float3 rd, float4 s) {
    float t = dot(swi3(s,x,y,z)-ro, rd);
    float3 p = ro + rd * t;
    
    float y = length(swi3(s,x,y,z)-p);
    
    float2 o = to_float2(MAX_DIST,MAX_DIST);
    
    if(y<s.w) {
        float x = _sqrtf(s.w*s.w-y*y);
        o.x = t-x;
        o.y = t+x;
    }
    return o;
}

// From IQ
__DEVICE__ float smin( float a, float b, float k ) {
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float smax(float a, float b, float k) {
  return smin(a, b, -k);
}

__DEVICE__ mat2 Rot(float a) {
    float s = _sinf(a);
    float c = _cosf(a);
    return to_mat2(c, -s, s, c);
}

__DEVICE__ float sdGyroid(float3 p, float scale, float thickness, float bias) {
    p *= scale;
    return _fabs(dot(sin_f3(p), cos_f3(swi3(p,z,x,y)))+bias)/scale - thickness;
}

__DEVICE__ float sdSph(float3 p, float3 pos, float3 squash, float r) {
    squash = 1.0f/squash;
    p = (p-pos)*squash;
    return (length(p)-r)/_fmaxf(squash.x, _fmaxf(squash.y, squash.z));
}

__DEVICE__ float4 Scales(float2 uv, float overlap, float skew, float point, float blur) {
    float2 gv = fract_f2(uv*5.0f)-0.5f;
    float2 id = _floor(uv*5.0f);
    
    float m = 0.0f;
    
    gv.y = sabs(gv.y,point);
    
    float w = 0.5f+overlap;
    float2 p1 = (gv+to_float2(overlap,-gv.x*skew))*to_float2(1,1.8f);
    float a1 = _atan2f(p1.x-w, p1.y);
    
    float waveAmp = 0.02f;
    float waves = 10.0f;
    float w1 = _sinf(a1*waves);
    float s1 = S(w, w*blur, length(p1)+w1*waveAmp);
    s1 +=  w1*0.1f*s1;
    s1 *= _mix(1.0f, 0.5f-gv.x, overlap*2.0f);
    
    gv.x -= 1.0f;
    float2 p2 = (gv+to_float2(overlap,-gv.x*skew))*to_float2(1,1.8f);
    float a2 = _atan2f(p2.x-w, p2.y);
    float w2 = _sinf(a2*waves);
    float s2 = S(w, w*blur, length(p2)+w2*waveAmp);
    s2 += w2*0.1f*s2;
    
    s2 *= _mix(1.0f, 0.5f-gv.x, overlap*2.0f);
    
    if(s1>s2) {
        m += s1;
        m -= dot(p1,p1);
    } else {
        m += s2;
        m -= dot(p2,p2);
        id.x += 1.0f;
    }

    return to_float4(1.0f-m, 0.0f, id.x, id.y);
}

__DEVICE__ float4 ScaleTex(float2 uv, float overlap, float skew, float point, float blur) {

    uv *= 2.0f;
    float4 s1 = Scales(uv, overlap, skew, point, blur);
    float4 s2 = Scales(uv+0.1f, overlap, skew, point, blur);
    //swi2(s2,z,w) -= 0.5f;
    s2.z -= 0.5f;
    s2.w -= 0.5f;
    
    return s1.x<s2.x ? s1 : s2;
}


__DEVICE__ float3 sdBody(float3 p, float iTime) {
    float t = iTime*0.3f;
    float neckFade = S(3.0f, 10.0f, p.z);
   
    p.x += _sinf(p.z*0.15f-t)*neckFade*4.0f;
    p.y += _sinf(p.z*0.1f-t)*neckFade;
    
    float2 st = to_float2(_atan2f(p.x, p.y), p.z);
    
    float body = length(swi2(p,x,y))-(0.86f+S(2.0f, 15.0f, p.z)*0.6f-p.z*0.01f);
    body = _fmaxf(0.8f-p.z, body);   
    
    float4 scales = to_float4_s(0);
    if(body<0.1f) {
        float2 uv = to_float2(-st.y*0.25f, st.x/6.2832f+0.5f);
        float a = _sinf(st.x+1.57f)*0.5f+0.5f;
        float fade = a;
        a = S(0.1f, 0.4f, a);

        uv.y = 1.0f-_fabs(uv.y*2.0f-1.0f);
        uv.y *= (uv.y-0.2f)*0.4f;
        scales = ScaleTex(uv*1.3f, 0.3f*a, 0.3f*a, 0.01f, 0.8f);
        body += scales.x*0.02f*(fade+0.2f);
    }
    
    body += S(-0.4f, -0.9f, p.y)*0.2f;  // flatten bottom
    return to_float3(body, scales.z, scales.w);
}

__DEVICE__ float GetHeadScales(float3 p, float3 eye, float3 mouth, float md, float iTime) {    
    float t = iTime;
  
    float jitter = 0.5f;
    jitter *= S(0.1f, 0.3f, _fabs(md));
    jitter *= S(1.2f, 0.5f, p.z);
    
    p.z += 0.5f;
    p.z *= 0.5f;
    
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , Rot(0.6f)));
    float y = _atan2f(p.y, p.x);
    float2 gv = to_float2(p.z*5.0f, y*3.0f);

    float2 id = _floor(gv);
 
    gv = fract_f2(gv)-0.5f;
    
    float d=MAX_DIST;
    for(float y=-1.0f; y<=1.0f; y+=1.0f) {
        for(float x=-1.0f; x<=1.0f; x+=1.0f) {
            float2 offs = to_float2(x, y);

            float2 n = Hash22(id+offs);
            float2 p = offs+sin_f2(n*6.2831f)*jitter;
            p -= gv;
            
            float cd = dot(p,p);
            if(cd<d) d = cd;
        }
    }
    
    d += _sinf(d*20.0f)*0.02f;    
    d *= S(0.0f, 0.5f, length(swi2(p,x,y))-0.1f);
    return d*0.06f;
}

__DEVICE__ float sdHead(float3 p, float iTime) {    
    p.x = _fabs(p.x*0.9f);
    float d = sdSph(p, to_float3(0,-0.05f,0.154f), to_float3(1,1,1.986f),1.14f); 
    d = smax(d, length(p-to_float3(0,7.89f,0.38f))-8.7f, 0.2f);
    d = smax(d, length(p-to_float3(0,-7.71f,1.37f))-8.7f, 0.15f); // top
    
    d = smax(d, 8.85f-length(p-to_float3(9.16f,-1.0f,-3.51f)), 0.2f);  // cheeks
    
    float3 ep = p-to_float3(0.54f,0.265f,-0.82f);
    float eye = length(ep)-0.35f;
    float brows = S(0.1f, 0.8f, p.y-(p.z+0.9f)*0.5f);
    brows *= brows*brows;
    brows *= S(0.3f, -0.2f, eye);
     d -= brows*0.5f;
    d += S(0.1f, -0.2f, eye)*0.1f;
    
    float2 mp = swi2(p,y,z)-to_float2(3.76f+S(-0.71f, -0.14f, p.z)*(p.z+0.5f)*0.2f, -0.71f); 
    float mouth = length(mp)-4.24f;
    d += S(0.03f,0.0f,_fabs(mouth))*S(0.59f,0.0f, p.z)*0.03f;
    
     d += GetHeadScales(p, ep, swi3(mp,x,y,y), mouth,iTime);
    
    d = _fminf(d, eye);
    
    float nostril = length(swi2(p,z,y)-to_float2(-1.9f-p.x*p.x, 0.15f))-0.05f;
    d = smax(d, -nostril,0.05f);
    return d;
}

__DEVICE__ float sdTongue(float3 p, float iTime) {
  float t = iTime*3.0f;
   
    float inOut = S(0.7f, 0.8f, _sinf(t*0.5f));
    
    if(p.z>-2.0f || inOut==0.0f) return MAX_DIST;    // early out
    
    float zigzag = (_fabs(fract(t*2.0f)-0.5f)-0.25f)*4.0f; // flicker
    float tl = 2.5f;  // length
    
    p+=to_float3(0,0.27f,2);
    p.z *= -1.0f;
    float z = p.z;
    swi2S(p,y,z, mul_f2_mat2(swi2(p,y,z) , Rot(z*0.4f*zigzag)));
    p.z -= inOut*tl;
    
    float width = S(0.0f, -1.0f, p.z);
    float fork = 1.0f-width;
    
    float r = _mix(0.05f, 0.02f, fork);
  
    p.x = sabs(p.x, 0.05f*width*width);
    p.x -= r+0.01f;
    p.x -= fork*0.2f*inOut;

    return length(p-to_float3(0,0,clamp(p.z, -tl, 0.0f)))-r;
}

__DEVICE__ float GetDist(float3 P, float iTime) {
    
    float3 p = P;
    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , Rot(_sinf(iTime*0.3f)*0.1f*S(1.0f, 0.0f, p.z))));
    float d = sdTongue(p,iTime)*0.7f;
    d = _fminf(d, sdHead(p,iTime));
    d = smin(d, sdBody(P,iTime).x, 0.13f);
    
    return d;
}

__DEVICE__ float3 GetMat(float3 p, float iTime) {    
    float d = MAX_DIST;
    
    float tongue = sdTongue(p,iTime)*0.7f;
    float head = sdHead(p,iTime);
    float3 body = sdBody(p,iTime);
    
    float closest = _fminf(tongue, _fminf(head, body.x));
    if(closest == tongue) {
        return to_float3(MAT_TONGUE, 0, 0);
    } else if(closest==head) {
        p.x = _fabs(p.x*0.9f);
        float3 ep = p-to_float3(0.54f,0.265f,-0.82f);
        float eye = length(ep)-0.35f;
        if(eye<SURF_DIST)
            return to_float3(MAT_EYE, ep.y, ep.z);
        else
            return to_float3(MAT_BODY, 0, 0);
            
    }else if(closest==body.x) {
        return to_float3(MAT_BODY, body.y, body.z);
    }
    
    return to_float3_s(0.0f); //Fehler ??
}


__DEVICE__ float RayMarch(float3 ro, float3 rd, float iTime) {
    float dO=0.0f;
    
    for(int i=0; i<MAX_STEPS; i++) {
        float3 p = ro + rd*dO;
        float dS = GetDist(p,iTime);
        dO += dS;
        if(dO>MAX_DIST || _fabs(dS)<SURF_DIST) break;
    }
    
    return dO;
}

// From Nimitz
__DEVICE__ float4 GetNormalAndCurvature(in float3 p, float eps, float iTime) {
    float2 e = to_float2(-1.0f, 1.0f)*eps;   
    float t1 = GetDist(p + swi3(e,y,x,x),iTime), t2 = GetDist(p + swi3(e,x,x,y),iTime);
    float t3 = GetDist(p + swi3(e,x,y,x),iTime), t4 = GetDist(p + swi3(e,y,y,y),iTime);

    float c = 0.25f/e.y*(t1 + t2 + t3 + t4 - 4.0f*GetDist(p,iTime));
    float3 n = normalize(swi3(e,y,x,x)*t1 + swi3(e,x,x,y)*t2 + swi3(e,x,y,x)*t3 + swi3(e,y,y,y)*t4);
    
    return to_float4_aw(n, c);
}

__DEVICE__ float3 GetRayDir(float2 uv, float3 p, float3 l, float z) {
  float3 f = normalize(l-p),
         r = normalize(cross(to_float3(0,1,0), f)),
         u = cross(f,r),
         c = f*z,
         i = c + uv.x*r + uv.y*u,
         d = normalize(i);
    return d;
}


__DEVICE__ float4 Material(float3 ro, float3 rd, float d, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float3 ColTop, float3 ColBottom, float3 ColEye, float3 ColTongue) {
    float3 p = ro + rd * d;
    float4 n = GetNormalAndCurvature(p, _mix(0.01f, 0.03f, S(8.0f, 20.0f, d)),iTime);

    swi2S(p,x,z, mul_f2_mat2(swi2(p,x,z) , Rot(_sinf(iTime*0.3f)*0.1f*S(1.0f, 0.0f, p.z))));
    float3 mat = GetMat(p,iTime);
    
    float3 col = to_float3_s(n.y*0.5f+0.5f);    // diffuse
    col *= 1.0f-_fmaxf(0.0f, 0.3f-n.w);         // curvature shadow
    
    float3 h = normalize(-rd + to_float3(1,1,1));
    float spe = _powf(clamp(dot(h, swi3(n,x,y,z)), 0.0f, 1.0f), 32.0f);
  
    float3 ref = reflect(rd, swi3(n,x,y,z));
    float3 r = swi3(decube_f3(iChannel0,ref),x,y,z);
    
    if(mat.x==MAT_EYE) {
            float2 sph = RaySphere(
            to_float3(_fabs(p.x*0.9f), p.y, p.z), 
            to_float3(-_fabs(rd.x), rd.y, rd.z), 
            to_float4(0.3f,0.265f,-0.82f, 0.52f)
        );

        float3 sp = p+rd*sph.x;
        swi2S(mat,y,z, swi2(sp,y,z)-to_float2(0.265f,-0.82f)+0.05f);

        float t = iTime*0.2f;
        float2 p1 = sin_f2(_floor(t)*to_float2(20.0f, 31.0f));
        float2 p2 = sin_f2(_floor(t+1.0f)*to_float2(20.0f, 31.0f));
        p1 = _mix(p1, p2, S(0.45f, 0.5f, fract(t)));
        swi2S(mat,y,z, swi2(mat,y,z) + p1*to_float2(0.01f, 0.03f)*1.0f);
        float a = _atan2f(mat.y, mat.z);

        float d = _fabs(mat.z)+mat.y*mat.y;
        col *= ColEye;//to_float3(1,1,0.1f) * ColEye;
        col += S(0.1f, 0.0f, length(swi2(mat,y,z)*to_float2(1,2))-0.1f)*0.1f;
        
        float z = S(0.7f, 1.0f, rd.z*rd.z)*0.05f;
        col *= S(0.02f-z, 0.03f+z, d);
        
        float3 gp = to_float3(a, mat.y, mat.z)*20.0f;
        float gyroid = (_fabs(dot(sin_f3(gp), cos_f3(swi3(gp,z,x,y)))));
        col *= 1.0f+gyroid*0.1f;
        
        col += r*r*r*0.3f;
        col += _powf(spe, 6.0f);
    } else if(mat.x==MAT_BODY) {
        float x = mat.y;
        float y = mat.z;
        float wave = S(2.0f, 0.0f, _fabs(y-2.0f+_sinf(x*0.5f)*1.0f));
        wave *= S(2.0f, 3.0f, p.z);
        
        float t = iTime*0.3f;
        float neckFade = S(3.0f, 10.0f, p.z);
        p.y += _sinf(p.z*0.1f-t)*neckFade;
        
        //float3 baseCol = _mix(to_float3(1.0f, 1.0f, 0.2f), to_float3(0.3f, 0.8f, 0.1f), S(-0.55f, -0.1f, p.y));
        float3 baseCol = _mix(ColBottom, ColTop, S(-0.55f, -0.1f, p.y));
        col *= _mix(baseCol, to_float3(0.2f,0.4f,0.2f)*0.5f, wave);
        col += spe*_powf(1.0f-_fabs(n.w), 5.0f)*0.3f;
        
        r = swi3(decube_f3(iChannel1,ref),x,y,z);
        col += r*r*0.05f;
    } else if(mat.x==MAT_TONGUE) {
        col *= ColTongue;//to_float3(0.4f, 0.1f, 0.2f);
        col += _powf(_fminf(1.0f, spe*5.0f), 5.0f);
    }
    
    return to_float4_aw(col, 1);
}

__DEVICE__ float3 Render(float2 uv, float2 m, float t, float iTime, __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1, float3 ColTop, float3 ColBottom, float3 ColEye, float3 ColTongue) {
    float3 ro = to_float3(0, 0, -3)*(8.0f+_sinf(t*0.2f)*2.0f*CAM_MOVE);
    swi2S(ro,y,z, mul_f2_mat2(swi2(ro,y,z) , Rot(-m.y*3.14f+_sinf(t*0.03f)*CAM_MOVE*0.2f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z) , Rot(-m.x*6.2831f*2.0f+_sinf(t*0.05f)*CAM_MOVE)));
    
    float3 rd = GetRayDir(uv, ro, to_float3(0,0,_sinf(t*0.11f)), 6.0f);
    
    float d = RayMarch(ro, rd, iTime);
    
    float3 col = to_float3_s(0);
    
    float2 env = RaySphere(ro, rd, to_float4(0,0,0,20));
    
    if(d<MAX_DIST) {
        float4 snake = Material(ro, rd, d, iTime, iChannel0, iChannel1, ColTop, ColBottom, ColEye, ColTongue);
        swi3S(snake,x,y,z, swi3(snake,x,y,z) * S(60.0f, 10.0f, d));
        col = _mix(col, swi3(snake,x,y,z), snake.w);
    } else {
        col = (rd.y*0.5f+0.5f)*to_float3(0.4f, 1.0f,0.2f);
        col *= swi3(decube_f3(iChannel0,rd),x,y,z);
        col *= 1.0f-S(0.8f, 1.0f, rd.z);
        
        if(env.y>0.0f)  // vines behind
            col *= S(0.0f, 1.1f, sdGyroid(ro + env.y*rd, 0.4f, 0.1f, 0.0f))*0.5f+0.5f;
    }
    
    if(env.x>0.0f)  // vines in front
        col *= S(0.0f, 0.25f, sdGyroid(ro + env.x*rd, 0.25f, 0.1f, 0.0f));
    
    return col;
}


__KERNEL__ void DangerNoodleFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_COLOR0(ColorTop, 0.3f, 0.8f, 0.1f, 1.0f);
    CONNECT_COLOR1(ColorBottom, 1.0f, 1.0f, 0.2f, 1.0f);
    CONNECT_COLOR2(ColorEye, 1.0f, 1.0f, 0.2f, 1.0f);
    CONNECT_COLOR3(ColorTongue, 0.4f, 0.1f, 0.2f, 1.0f);
    

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 m = (swi2(iMouse,x,y)-0.5f*iResolution)/iResolution;    
    if(m.x<-0.49f && m.y<-0.49f) m*=0.0f;
 
    float3 col = Render(uv, m, iTime, iTime, iChannel0,iChannel1, swi3(ColorTop,x,y,z), swi3(ColorBottom,x,y,z), swi3(ColorEye,x,y,z), swi3(ColorTongue,x,y,z));
    
    col *= 1.5f;                              // exposure adjustment
    col = pow_f3(col, to_float3_s(0.4545f));  // gamma correction
    col *= 1.0f-dot(uv,uv)*0.3f;              // vignette

    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}