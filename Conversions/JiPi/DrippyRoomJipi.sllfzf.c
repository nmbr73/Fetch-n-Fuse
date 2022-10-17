
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ mat3 transpose(mat3 m)
{
  return(to_mat3(m.r0.x,m.r1.x,m.r2.x, m.r0.y,m.r1.y,m.r2.y, m.r0.z,m.r1.z,m.r2.z)); 	
}

__DEVICE__ inline mat3 mul_f_mat3( float B, mat3 A)  
{  
  return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
}


__DEVICE__ float3 _refract_f3(float3 I, float3 N, float eta, float refmul, float refoff) {
   float dotNI = dot(N, I);
   float k = 1.0f - eta * eta * (1.0f - dotNI * dotNI);
   if (k < 0.0f) {
     return to_float3_s(0.0);
   }
   return eta * I - (eta * dotNI * _sqrtf(k)) * N * refmul + refoff; //+0.5f;   * -01.50f;(MarchingCubes)  - 0.15f; (GlassDuck)
}



__KERNEL__ void DrippyRoomJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{
    CONNECT_SLIDER0(density, -1.0f, 10.0f, 0.9f);
  
    fragCoord+=0.5f;

    //float density=0.9f;

    float2 o=to_float2_s(1.0f)/500.0f;
    fragCoord/=iResolution;

    // Load the fluid buffer states. There are two, stored in R and G. Only R is changed per pass,
    // and the two channels are swapped using a swizzle mask.
    //fragColor.xy=_tex2DVecN(iChannel0,fragCoord.x,fragCoord.y,15).gr;
    fragColor.y=  _tex2DVecN(iChannel0,fragCoord.x,fragCoord.y,15).x;

    fragColor.x = (texture(iChannel0,fragCoord+o*to_float2( 0.0f,+1.0f)).x+
                   texture(iChannel0,fragCoord+o*to_float2( 0,-1)).x+
                   texture(iChannel0,fragCoord+o*to_float2(-1, 0)).x+
                   texture(iChannel0,fragCoord+o*to_float2(+1, 0)).x+
                   texture(iChannel0,fragCoord+o*to_float2(-1,-1)).x+
                   texture(iChannel0,fragCoord+o*to_float2(+1,-1)).x+
                   texture(iChannel0,fragCoord+o*to_float2(-1,+1)).x+
                   texture(iChannel0,fragCoord+o*to_float2(+1,+1)).x+
                   texture(iChannel0,fragCoord+o*to_float2( 0, 0)).x)*2.0f/9.0f-
                   texture(iChannel0,fragCoord+o*to_float2( 0, 0)).y;
            
float AAAAAAAAAAAAAAAAAAAA;   
    // Add the interaction with the inner box.
    float ba=iTime;
    mat3 boxxfrm= mul_mat3_mat3(to_mat3(_cosf(ba),_sinf(ba),0,-_sinf(ba),_cosf(ba),0,0,0,1) ,
                                //to_mat3(_cosf(ba),0,_sinf(ba),-_sinf(ba),0,_cosf(ba),0,1,0)*4.0f);
                                to_mat3(_cosf(ba)*4.0f,0,_sinf(ba)*4.0f,-_sinf(ba)*4.0f,0,_cosf(ba)*4.0f,0,1.0f*4.0f,0));
    float3 bp=to_float3(fragCoord.x*2.0f-1.0f,0.1f,fragCoord.y*2.0f-1.0f);
    float3 bp2=mul_mat3_f3(boxxfrm,bp);
    float bd=length(_fmaxf(to_float3_s(0.0f),abs_f3(bp2)-to_float3_s(1.0f)));
    
    if(bd<1e-3)
       fragColor.x+=0.03f;

    // Add some random drips.
    float p=0.01f;
    float c=_floor(mod_f(iTime,64.0f)/p);
    fragColor.x += (1.0f-smoothstep(0.0f,0.01f,distance_f2(fragCoord,1.5f*to_float2(_cosf(c*11.0f),_sinf(c*7.1f)))))*0.8f;    
    
    fragColor.x *= density;

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Wood' to iChannel3
// Connect Image 'Texture: RGBA Noise Medium' to iChannel2
// Connect Image 'Texture: Organic 3' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


//mat3 boxxfrm;
__DEVICE__ float box1(float3 ro,float3 rd)
{
    return _fminf((sign_f(rd.x)-ro.x)/rd.x,_fminf((sign_f(rd.y)-ro.y)/rd.y,(sign_f(rd.z)-ro.z)/rd.z));
}
__DEVICE__ float2 box2(float3 ro,float3 rd)
{
    return to_float2(_fmaxf((-sign_f(rd.x)-ro.x)/rd.x,_fmaxf((-sign_f(rd.y)-ro.y)/rd.y,(-sign_f(rd.z)-ro.z)/rd.z)),box1(ro,rd));
}
__DEVICE__ float3 textureBox1(float3 p, mat3 boxxfrm, __TEXTURE2D__ iChannel1 )
{
    float3 ap=abs_f3(p), f=step(swi3(ap,z,x,y),swi3(ap,x,y,z))*step(swi3(ap,y,z,x),swi3(ap,x,y,z));
    float2 uv=f.x>0.5f ? swi2(p,y,z):f.y>0.5f ? swi2(p,x,z) : swi2(p,x,y);
    float l=clamp(-normalize(p-to_float3(0,1,0)).y,0.0f,1.0f);
    float2 b=box2(mul_mat3_f3(boxxfrm,p), mul_mat3_f3(boxxfrm,(to_float3(0,1,0)-p)));
    // Some lighting and a shadow (and approximated AO).
    float s=_mix(0.2f,1.0f,smoothstep(0.0f,0.8f,length(swi2(p,x,z))));
    float3 d=0.6f*(1.0f-smoothstep(-1.0f,1.0f,p.y))*to_float3(0.3f,0.3f,0.5f)*s+smoothstep(0.9f,0.97f,l)*to_float3(1,1,0.8f)*step(b.y,b.x);
    return swi3(_tex2DVecN(iChannel1,uv.x,uv.y,15),x,y,z)*d;
}

__DEVICE__ float3 textureBox2(float3 p,float3 p2, mat3 boxxfrm, float aspect, __TEXTURE2D__ iChannel3 )
{
    float3 ap=abs_f3(p),f=step(swi3(ap,z,x,y),swi3(ap,x,y,z))*step(swi3(ap,y,z,x),swi3(ap,x,y,z));
    float2 uv=f.x>0.5f ? swi2(p,y,z) : f.y>0.5f ? swi2(p,x,z) : swi2(p,x,y);
    float3 n=normalize(mul_mat3_f3(mul_f_mat3(-1.0f,transpose(boxxfrm)),(f*sign_f3(p))));
    float l=clamp(-normalize(p2-to_float3(0,1,0)).y,0.0f,1.0f);
    float3 d=1.0f*(1.0f-smoothstep(-1.0f,2.5f,p2.y))*to_float3(0.3f,0.3f,0.7f)+smoothstep(0.95f,0.97f,l)*clamp(-n.y,0.0f,1.0f)*2.0f*to_float3(1,1,0.8f)+
                        smoothstep(0.9f,1.0f,l)*clamp(-n.y,0.0f,1.0f)*to_float3(1,1,0.8f);
    return swi3(_tex2DVecN(iChannel3,uv.x*aspect,uv.y,15),x,y,z)*d;
}
__DEVICE__ mat2 rotation2D(float a)
{
    return to_mat2(_cosf(a),_sinf(a),-_sinf(a),_cosf(a));
}


__KERNEL__ void DrippyRoomJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{
    CONNECT_COLOR0(Color, 0.55f,0.75f,1.0f, 1.0f); 
  
    CONNECT_SLIDER1(refmul, -1.0f, 10.0f, 1.0f); 
    CONNECT_SLIDER2(refoff, -1.0f, 10.0f, 0.0f);
    

  
    fragCoord+=0.5f;
    float time=iTime;
    float3 waterc=swi3(Color,x,y,z)*0.9f;//to_float3(0.55f,0.75f,1.0f)*0.9f;
 
    float aspect = iResolution.y/iResolution.x;

    // Set up the primary ray.
    float3 ro=to_float3(0,0.7f,0.8f),rd=normalize(to_float3_aw((fragCoord-iResolution/2.0f)/iResolution.y,-1.0f));
    
    swi2S(rd,y,z, mul_f2_mat2(swi2(rd,y,z),rotation2D(0.7f)));
    swi2S(rd,x,z, mul_f2_mat2(swi2(rd,x,z),rotation2D(time*0.1f)));
    swi2S(ro,x,z, mul_f2_mat2(swi2(ro,x,z),rotation2D(time*0.1f)));
    
    // These are the heights of the planes that the water surface lies within.
    float h0=0.1f,h1=-0.4f;
    
    float ba=time;
    mat3 boxxfrm = mul_mat3_mat3(to_mat3(_cosf(ba),_sinf(ba),0,-_sinf(ba),_cosf(ba),0,0,0,1),
                               //to_mat3(_cosf(ba),0,_sinf(ba),-_sinf(ba),0,_cosf(ba),0,1,0)*4.0f);
                                 to_mat3(_cosf(ba)*4.0f,0,_sinf(ba)*4.0f,-_sinf(ba)*4.0f,0,_cosf(ba)*4.0f,0,1.0f*4.0f,0));
                           
    
    float t0=(h0-ro.y)/rd.y,t1=(h1-ro.y)/rd.y;
    float bt2=box1(ro,rd);
    float2 bt3=box2(mul_mat3_f3(boxxfrm,ro),mul_mat3_f3(boxxfrm,rd));
   
    // Raymarch through the water surface.
    float ht=0.0f,h=0.0f;
    float2 uv;
    const int n=256;
    for(int i=0;i<n;++i)
    {
        ht=_mix(t0,t1,(float)(i)/(float)(n));
        float3 hp=ro+rd*ht;
        uv=swi2(hp,x,z)/2.0f+0.5f;
        h=_tex2DVecN(iChannel0,uv.x,uv.y,15).x;
        if(h<(float)(i)/(float)(n))
            break;
    }
    // Check primary ray intersection with the inner box.
    if(bt3.x < bt3.y && bt3.x < ht)
    {
        fragColor = to_float4_aw(textureBox2(mul_mat3_f3(boxxfrm,(ro+rd*bt3.x)),ro+rd*bt3.x,boxxfrm, aspect, iChannel3), Color.w);
        SetFragmentShaderComputedColor(fragColor);
        return;
    }
    // Check subsequent intersections after water surface intersection.
    if(ht>0.0f && ht<bt2)
    {
        const float e=1e-2;
        float hdx=texture(iChannel0,uv+to_float2(e,0.0f)).x;
        float hdy=texture(iChannel0,uv+to_float2(0.0f,e)).x;
        float3 norm=normalize(to_float3(hdx,e,hdy));
        float fresnel=1.0f-_powf(clamp(1.0f-dot(-rd,norm),0.0f,1.0f),2.0f);
        //float3 r = refract_f3(rd,norm,1.0f/1.333f);
        float3 r = _refract_f3(rd,norm,1.0f/1.333f,refmul,refoff);
        
        float3 r2= reflect(rd,norm);
        ro+=ht*rd;
        bt2=box1(ro,r);
        bt3=box2(mul_mat3_f3(boxxfrm,ro),mul_mat3_f3(boxxfrm,r));
        
        float bt4=box1(ro,r2);
        float2 bt5=box2(mul_mat3_f3(boxxfrm,ro),mul_mat3_f3(boxxfrm,r2));
        
        float3 reflc,refrc;
        
        reflc=textureBox1(ro+r*bt4, boxxfrm, iChannel1);
        if(bt5.x < bt5.y && bt5.x > 0.0f)
        {
            reflc=textureBox2(mul_mat3_f3(boxxfrm,(ro+r2*bt5.x)),ro+r2*bt5.x,boxxfrm, aspect, iChannel3);
        }
        
        refrc=textureBox1(ro+r*bt2, boxxfrm, iChannel1);
        if(bt3.x<bt3.y&&bt3.x>0.0f)
        {
            refrc=textureBox2(mul_mat3_f3(boxxfrm,(ro+r*bt3.x)),ro+r*bt3.x,boxxfrm, aspect, iChannel3);
        } 

        swi3S(fragColor,x,y,z, reflc*(1.0f-fresnel)+refrc*fresnel*waterc);
    }
    else
        swi3S(fragColor,x,y,z, textureBox1(ro+rd*bt2, boxxfrm, iChannel1));
    
    // Apply (very simple) tone mapping and gamma.
    fragColor = to_float4_aw(sqrt_f3(clamp((swi3(fragColor,x,y,z)/(swi3(fragColor,x,y,z)+to_float3_s(1.0f))),0.0f,1.0f)), Color.w);


  SetFragmentShaderComputedColor(fragColor);
}