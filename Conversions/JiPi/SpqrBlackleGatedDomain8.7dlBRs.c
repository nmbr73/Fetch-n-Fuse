
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//CC0 1.0f Universal https://creativecommons.org/publicdomain/zero/1.0f/
//To the extent possible under law, Blackle Mori has waived all copyright and related or neighboring rights to this work.


//returns a vector pointing in the direction of the closest neighbouring cell

#define _saturatef(a) (clamp((a),0.0f,1.0f))
__DEVICE__ mat2 rot(float a){
    float s = _sinf(a);
    float c = _cosf(a);
    return to_mat2(c,s,-s,c);
}



__DEVICE__ float hash( float n )
{
    return fract(_sinf(n)*43758.5453123f);
}

// 3d noise function
__DEVICE__ float noise( in float3 _x )
{
  
    float3 p = _floor(_x);
    float3 f = fract_f3(_x);
    f = f*f*(3.0f-2.0f*f);
    float n = p.x + p.y*57.0f + 113.0f*p.z;
    float res = _mix(_mix(_mix( hash(n+  0.0f), hash(n+  1.0f),f.x),
                          _mix( hash(n+ 57.0f), hash(n+ 58.0f),f.x),f.y),
                     _mix(_mix( hash(n+113.0f), hash(n+114.0f),f.x),
                          _mix( hash(n+170.0f), hash(n+171.0f),f.x),f.y),f.z);
    return res;
}

// fbm noise for 2-4 octaves including rotation per octave
__DEVICE__ float fbm( float3 p )
{
  mat3 m = to_mat3( 0.00f,  0.80f,  0.60f,
                 -0.80f,  0.36f, -0.48f,
                 -0.60f, -0.48f,  0.64f );
                 
    float f = 0.0f;
    f += 0.5000f*noise( p );
    p = mul_mat3_f3(m,p*2.02f);
    f += 0.2500f*noise( p ); 
    p = mul_mat3_f3(m,p*2.03f);
    f += 0.1250f*noise( p );
    p = mul_mat3_f3(m,p*2.01f);
    f += 0.0625f*noise( p );
    return f/0.9375f;
}
__DEVICE__ float box(float3 p,float3 s)
{
  float3 d=abs_f3(p)-s;
  return length(_fmaxf(d,to_float3_s(0.0f)));
}



//__DEVICE__ float hash(float a, float b) {
//    return fract(_sinf(a*1.2664745f + b*0.9560333f + 3.0f) * 14958.5453f);
//}



__DEVICE__ float tick (float t){

    float i = _floor(t);
    float r = fract(t);
    r = smoothstep(0.0f,1.0f,r);
    r = smoothstep(0.0f,1.0f,r);
    r = smoothstep(0.0f,1.0f,r);
    r = smoothstep(0.0f,1.0f,r);
    
    return i + r;
}

__DEVICE__ float tock (float t){
    float i = _floor(t);
    float r = fract(t);
    r = smoothstep(0.0f,1.0f,r);
  
    
    return i + r;
}


#define MOD3 to_float3(0.1031f,0.11369f,0.13787f)

//value noise hash
__DEVICE__ float hash31(float3 p3)
{
    p3  = fract_f3(p3 * MOD3);
    p3 += dot(p3, swi3(p3,y,z,x) + 19.19f);
    return -1.0f + 2.0f * fract((p3.x + p3.y) * p3.z);
}
__DEVICE__ float3 randomdir (float n) {
    return fract_f3(to_float3(5.1283f,9.3242f,13.8381f) * hash(n) * 8421.4242f);
}

__DEVICE__ float sph(float3 p,float r) {
    return length(p) -r ;
}
__DEVICE__ float cyl (float2 p, float r){
    return length(p) - r;
}
__DEVICE__ float map(float3 p, float iTime, float *glow) {

    //geo

    float tt = iTime * 0.3f;
    float3 q = p;
    
    // wierd skylights
    swi2S(q,y,z, mul_f2_mat2(swi2(q,y,z) , rot(0.42f)));
    swi2S(q,x,z, mul_f2_mat2(swi2(q,x,z) , rot(0.4f)));
  
    swi2S(q,x,y, mul_f2_mat2(swi2(q,x,y) , rot(q.z/50.0f)));
    q.x += tt * 10.0f;
    swi2S(q,x,z, mod_f( swi2(q,x,z), 40.0f) - 20.0f);
  
    float uu = cyl(swi2(q,x,z),0.001f);
    *glow += 0.04f/(0.1f+_powf(uu,1.8f));
    
    float domain = 1.4f;
  
      
    float3 id = _floor((p*0.1f)/domain);
    float3 id2 = _floor((p)/domain);
    p = mod_f3(p,domain) - domain/2.0f;
    
    float thresh = fbm(id);
    
    float rando = hash31(id2);

   
    float3 flit = to_float3_s(0.04f);
    swi2S(flit,x,z, mul_f2_mat2(swi2(flit,x,z) , rot(rando*5.1f+tt*2.3f)));
    swi2S(flit,y,z, mul_f2_mat2(swi2(flit,y,z) , rot(rando*4.2f+tt*1.4f)));
    swi2S(flit,x,y, mul_f2_mat2(swi2(flit,x,y) , rot(rando*3.3f+tt*1.1f)));
   
    //vec3 flit = randomdir(hash31(id)) * 0.2f;
    
    float3 jitter = flit * _sinf((tt*9.1f+rando*12.1f));
    
    //(0.5f)hash(float(id)) * to_float3_s(0.5f) * _sinf(iTime*6.0f+3.0f*hash(float(id)));
    
    if (  rando *0.6f< thresh) {
        p = abs_f3(p);
        if (p.x > p.y) swi2S(p,x,y, swi2(p,y,x));
        if (p.y > p.z) swi2S(p,y,z, swi2(p,z,y));
        if (p.x > p.y) swi2S(p,x,y, swi2(p,y,x));
        p.z -= domain;
        //return length(p)-1.0f;
        
        float u = box(p + jitter, to_float3_s(0.4f));
      
        return _fminf(uu,u*0.5f);
        
    } else {
        //return length(p)-1.0f;
        float u = box(p + jitter, to_float3_s(0.4f));
       
        return _fminf(uu,u*0.5f);
    }
 
}


__DEVICE__ float3 norm(float3 p,float2 d, float iTime, float *glow)
{
  return normalize(to_float3(
    map(p+swi3(d,y,x,x),iTime, glow)-map(p-swi3(d,y,x,x),iTime, glow),
    map(p+swi3(d,x,y,x),iTime, glow)-map(p-swi3(d,x,y,x),iTime, glow),
    map(p+swi3(d,x,x,y),iTime, glow)-map(p-swi3(d,x,x,y),iTime, glow)
  ));
}

__DEVICE__ float3 norm(float3 p, float iTime, float *glow) {
    //mat3 k = to_mat3(p,p,p)-to_mat3(0.01f);
    float3 k1 = p - to_float3(0.001f,0.0f,0.0f);
    float3 k2 = p - to_float3(0.0f,0.001f,0.0f);
    float3 k3 = p - to_float3(0.0f,0.0f,0.001f);
    
    return normalize(map(p,iTime, glow) - to_float3( map(k1,iTime, glow),map(k2,iTime, glow),map(k3,iTime, glow) ));
}


__DEVICE__ float3 tex3D( __TEXTURE2D__ tex, float3 p, float3 n, float ratio ){

    n = _fmaxf(abs_f3(n), to_float3_s(0.001));//n = max((abs(n) - 0.2)*7., 0.001); //  etc.
    n /= (n.x + n.y + n.z );
    p = swi3((texture(tex, swi2(p,y,z)*to_float2(ratio,1.0f))*n.x 
            + texture(tex, swi2(p,z,x)*to_float2(ratio,1.0f))*n.y 
            + texture(tex, swi2(p,x,y)*to_float2(ratio,1.0f))*n.z),x,y,z);
    return p*p;
}



__DEVICE__ float3 pixel_color(float2 uv, float iTime, float3 color, bool Tex, __TEXTURE2D__ iChannel0, float ratio) {
   
  
    float glow = 0.0f;
  // nav

    float tt = iTime ;
    float3 jump = to_float3_s(0) * tick(tt*0.05f)*77.2f;
    swi2S(jump,x,z, mul_f2_mat2(swi2(jump,x,z) , rot(tt*0.00001f)));
      
    float3 s = to_float3(10.0f,3.2f,7.1f)*tt*0.18f + jump;
    float3 arm = to_float3(1,0,0);
    swi2S(arm,x,z, mul_f2_mat2(swi2(arm,x,z) , rot(_sinf(tt* 0.19f))));
    swi2S(arm,y,z, mul_f2_mat2(swi2(arm,y,z) , rot(_sinf(tt* 0.23f))));
    //swi2(arm,y,x) *= rot(_sinf(tt*0.28f));
    
    float3 t = s + arm;
  
    float3 cz=normalize(t-s);
    float3 cx=normalize(cross(cz,to_float3(0,1,0)));
    float3 cy=-1.0f*normalize(cross(cz,cx));
    cz -= dot(uv,uv)/15.0f;
    float3 r=normalize(cx*uv.x+cy*uv.y+cz);
       
    float3 p = s;
    bool hit = false;
  
    float d;
    float i;
    float dd = 0.0f;
    //ray marching
    for ( i = 0.0f; i < 1500.0f; i++) {
        
        d = map(p,iTime,&glow);
        d = _fabs(d);
        if ( d < 0.001f) {
           hit = true;
           break;
        }
        if (dd>10000.0f) { break;}
        
        dd += d;
        p+=d*r;
    }
  

    float3 light = normalize(to_float3_s(1));
    float3 n = norm(p,iTime,&glow);



    float3 col = to_float3(0.8f, 0.5f, 0.2f);
    col = color;
    
    if (Tex)
    {
       //col = swi3(texture(iChannel0, swi2(p,x,y)),x,y,z);
       col = tex3D( iChannel0, p, n, ratio );
    }
    
    
    //col = to_float3(0.1f,0.1f,0.2f)*1.0f;
    
    float ao = _powf(1.0f - i/500.0f,6.0f);
    col *= ao;
    col += glow*0.6f;
    
    
    
    // if ( dot(light,n) < 0.0f) { light = -light;}
    float spec =_powf(_fmaxf(dot(reflect(-light,n),-r),0.0f),40.0f) * 10.0f;
    col += spec * 0.1f;
    float diff = _fmaxf(0.0f, dot(n,light)*0.5f +0.5f);
    col *= diff;
    float3 n2 = norm(p, to_float2(0.0f, 1E-2 ), iTime,&glow);// + 3E-2*0.01f) );
    float3 n1 = norm(p, to_float2(0.0f, 2.3E-2), iTime,&glow );

    float edge = _saturatef(length(n1-n2)/0.1f);
    
    if ( edge > 0.01f) {
        col = to_float3_s(0);
    }
    
    if (! hit){
        col = to_float3_s(0);
    }

    return col; 
}



__KERNEL__ void SpqrBlackleGatedDomain8Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution)
{

    CONNECT_CHECKBOX0(Tex, 0);
    //CONNECT_SLIDER0(Alpha, 0.0f, 1.0f, 1.0f);
    CONNECT_COLOR0(Color, 0.8f, 0.5f, 0.2f, 1.0f);

    float2 uv = (fragCoord-0.5f*iResolution)/iResolution.y;
    fragColor = to_float4_s(0);
    float ratio = iResolution.y/iResolution.x;
    fragColor += to_float4_aw(pixel_color(uv,iTime, swi3(Color,x,y,z), Tex, iChannel0, ratio), 1.0f);
    fragColor = to_float4_aw(sqrt_f3(swi3(fragColor,x,y,z))/fragColor.w, Color.w);

  SetFragmentShaderComputedColor(fragColor);
}

