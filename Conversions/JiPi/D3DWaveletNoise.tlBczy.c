
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ inline mat3 add_mat3_mat3( mat3 A, mat3 B) {  
  mat3 C;  

  C.r0 = to_float3(A.r0.x + B.r0.x, A.r0.y + B.r0.y,A.r0.z + B.r0.z);  
  C.r1 = to_float3(A.r1.x + B.r1.x, A.r1.y + B.r1.y,A.r1.z + B.r1.z); 
  C.r2 = to_float3(A.r2.x + B.r2.x, A.r2.y + B.r2.y,A.r2.z + B.r2.z);

  return C;  
  }
  
  
__DEVICE__ inline mat3 sub_mat3_mat3( mat3 A, mat3 B) {  
  mat3 C;  

  C.r0 = to_float3(A.r0.x - B.r0.x, A.r0.y - B.r0.y,A.r0.z - B.r0.z);  
  C.r1 = to_float3(A.r1.x - B.r1.x, A.r1.y - B.r1.y,A.r1.z - B.r1.z); 
  C.r2 = to_float3(A.r2.x - B.r2.x, A.r2.y - B.r2.y,A.r2.z - B.r2.z);

  return C;  
  }  
  
  
__DEVICE__ inline mat3 _to_mat3_f( float A)  
  {  
  
	mat3 D = {to_float3_s(0.0f),to_float3_s(0.0f),to_float3_s(0.0f)};  
	//D.r0.x = A;  
  D.r0 = to_float3_s(A);
	D.r1 = to_float3_s(A);  
	D.r2 = to_float3_s(A);
	return D;  
  } 
  
  #define to_mat3_f _to_mat3_f

__DEVICE__ float powcf(float x, float y) {
    float ret = _powf(x,y);
    if (isnan(ret)) {
        ret = 0.0001f;
    }
    return ret;
}




//CC0 1.0f Universal https://creativecommons.org/publicdomain/zero/1.0f/
//To the extent possible under law, Blackle Mori has waived all copyright and related or neighboring rights to this work.

__DEVICE__ float3 erot(float3 p, float3 ax, float ro) {
    return _mix(dot(p,ax)*ax,p,_cosf(ro))+_sinf(ro)*cross(ax,p);
}

__DEVICE__ float WaveletNoise(float3 p, float z, float k) {
    // https://www.shadertoy.com/view/wsBfzK
    float d=0.0f,s=1.0f,m=0.0f, a;
    for(float i=0.0f; i<5.0f; i+=1.0f) {
        float3 q = p*s, g=fract_f3(_floor(q)*to_float3(123.34f,233.53f,314.15f));
        g += dot(g, g+23.234f);
        a = fract(g.x*g.y)*1e3 +z*(mod_f(g.x+g.y, 2.0f)-1.0f); // add vorticity
        q = (fract_f3(q)-0.5f);
        //random rotation in 3d. the +0.1f is to fix the rare case that g == to_float3_aw(0)
        //https://suricrasia.online/demoscene/functions/#rndrot
        q = erot(q, normalize(tan_f3(g+0.1f)), a);
        d += _sinf(q.x*10.0f+z)*smoothstep(0.25f, 0.0f, dot(q,q))/s;
        p = erot(p,normalize(to_float3(-1,1,0)),_atan2f(_sqrtf(2.0f),1.0f))+i; //rotate along the magic angle
        m += 1.0f/s;
        s *= k; 
    }

    return d/m;
}

__DEVICE__ float super(float3 p) {
    return _sqrtf(length(p*p));
}

__DEVICE__ float box(float3 p, float3 d) {
    float3 q = abs_f3(p)-d;
    return super(_fmaxf(q,to_float3_s(0.0f)))+_fminf(0.0f,_fmaxf(q.x,_fmaxf(q.y,q.z)));
}



__DEVICE__ float scene(float3 p, float3 *distorted_p, float iTime, float3 distort_fct) {
    //different noise for each dimension
    p.x += WaveletNoise(p/2.0f, iTime*3.0f, 1.15f)*0.3f;
    p.y += WaveletNoise(p/2.0f+10.0f, iTime*3.0f, 1.15f)*0.3f;
    p.z += WaveletNoise(p/2.0f+20.0f, iTime*3.0f, 1.15f)*0.3f;
    *distorted_p = p*distort_fct;
    return box(p,to_float3_s(1))-0.3f;
}

__DEVICE__ float3 norm(float3 p, float3 *distorted_p, float iTime, float3 distort_fct) {
    //mat3 k = sub_mat3_mat3(to_mat3_f3(p,p,p) , to_mat3_f(0.001f));
    
    float3 k1 = p - to_float3(0.001f,0.0f,0.0f);
    float3 k2 = p - to_float3(0.0f,0.001f,0.0f);
    float3 k3 = p - to_float3(0.0f,0.0f,0.001f);
    
    return normalize(scene(p,distorted_p,iTime, distort_fct) - 
                           to_float3(scene(k1,distorted_p,iTime,distort_fct),
                                     scene(k2,distorted_p,iTime,distort_fct),
                                     scene(k3,distorted_p,iTime,distort_fct)));
}


__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, float3 p, float3 n ){

    n = _fmaxf(abs_f3(n), to_float3_s(0.001));//n = max((abs(n) - 0.2)*7., 0.001); //  etc.
    n /= (n.x + n.y + n.z );
    p = swi3((texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z),x,y,z);
    return p*p;
}


__KERNEL__ void D3DWaveletNoiseFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    //CONNECT_CHECKBOX0(Alpha, 1);
    CONNECT_CHECKBOX1(Textur, 0);
    CONNECT_CHECKBOX2(Background, 1);
    
    CONNECT_SLIDER0(Frens, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER1(Spec, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER2(SpecPow, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER3(Diff, -2.0f, 2.0f, 1.0f);
    CONNECT_SLIDER4(Alpha, 0.0f, 1.0f, 1.0f);
    CONNECT_SLIDER5(DistortZ, -1.0f, 3.0f, 1.0f);
    CONNECT_POINT0(DistortXY, 1.0f, 1.0f);

    float3 Distort = to_float3(DistortXY.x,DistortXY.y,DistortZ);

    float3 distorted_p = to_float3_s(0.0f);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    float2 mouse = (swi2(iMouse,x,y)-0.5f*iResolution)/iResolution.y;

    float3 cam = normalize(to_float3(1.2f,uv.x,uv.y));
    float3 init = to_float3(-7.0f,0,0);
    
    float yrot = 0.5f;
    float zrot = iTime*0.2f;
    if (iMouse.z > 0.0f) {
        yrot += -4.0f*mouse.y;
        zrot = 4.0f*mouse.x;
    }
    cam = erot(cam, to_float3(0,1,0), yrot);
    init = erot(init, to_float3(0,1,0), yrot);
    cam = erot(cam, to_float3(0,0,1), zrot);
    init = erot(init, to_float3(0,0,1), zrot);
    
    float3 p = init;
    bool hit = false;
    for (int i = 0; i < 250 && !hit; i++) {
        float dist = scene(p, &distorted_p, iTime, Distort);
        hit = dist*dist < 1e-6;
        p+=dist*cam*0.9f;
        if (distance_f3(p,init)>50.0f) break;
    }
    float3 local_coords = distorted_p;
    float3 n = norm(p, &distorted_p, iTime, Distort);
    float3 r = reflect(cam,n);
    float ss = smoothstep(-0.05f,0.05f,scene(p+to_float3_s(0.05f)/_sqrtf(3.0f), &distorted_p, iTime, Distort));
    float tex = WaveletNoise(local_coords*3.0f, 0.0f, 1.5f)+0.5f;
    float diff = (_mix(length(sin_f3(n*2.0f)*0.5f+0.5f)/_sqrtf(3.0f),ss,0.7f)+0.1f)*Diff;
    float spec = ((length(sin_f3(r*4.0f)*0.5f+0.5f)/_sqrtf(3.0f)))*Spec;
    float specpow = (_mix(3.0f,10.0f,tex))*SpecPow;
    float frens = (1.0f - powcf(dot(cam,n),2.0f)*0.98f) * Frens;
    //float3 col = to_float3(0.7f,0.2f,0.4f)*diff + _powf(spec,specpow)*frens;
    float3 col = to_float3(0.9f,0.83f,0.004f)*diff + _powf(spec,specpow)*frens;
    
    //col = texture(iChannel0,p.xy).xyz;
    
    if(Textur)
    {
      col = tex3D(iChannel0,p*0.25f+0.5f,n)*diff + _powf(spec,specpow)*frens;
      /* Funktioniert nicht 
      float texw = texture(iChannel0,uv).w;
      if (texw > 0.0f)
      {
        float3 tex = tex3D(iChannel0,p*0.25f+0.5f,n)*diff + _powf(spec,specpow)*frens;
        col = tex;//swi3(tex,x,y,z);
      }
      */
    }
    
    float bgdot = length(sin_f3(cam*3.5f)*0.4f+0.6f)/_sqrtf(3.0f);
    float3 bg = to_float3(0.2f,0.2f,0.3f) * bgdot + _powf(bgdot, 10.0f)*2.0f;
    
    if (Background==0) bg = to_float3_s(0.0f);
    
    swi3S(fragColor,x,y,z, hit ? col : bg);
    fragColor = sqrt_f4(fragColor);
    fragColor *= 1.0f- dot(uv,uv)*0.6f;

    fragColor.w = hit ? 1.0f : Alpha;
     
  SetFragmentShaderComputedColor(fragColor);
}