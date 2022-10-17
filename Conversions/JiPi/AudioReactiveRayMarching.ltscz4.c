
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 2' to iChannel1
// Connect Image 'Cubemap: Uffizi Gallery Blurred_0' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

__DEVICE__ float3 _refract_f3(float3 I, float3 N, float ior) {
    //float cosi = clamp(dot(N,I), -1.0f,1.0f);  //clamp(-1, 1, I.dot(N));
    float cosi = clamp( -1.0f,1.0f,dot(N,I));  //clamp(-1, 1, I.dot(N));
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
	  return eta * I + (eta * cosi - _sqrtf(1.0f-k)) * N;
    }
}
 
// based on my 2d shader https://www.shadertoy.com/view/llSSWD
__DEVICE__ float3 effect(float2 g, float iTime, __TEXTURE2D__ iChannel1, float3 tune) 
{
  float t = iTime * 1.5f;
  g /= 200.0f;
  g.x -= t * 0.015f;
  g.y += _sinf(g.x * 46.5f + t) * 0.12f;
  //float3 c = textureLod(iChannel1, g, 4.0f*(_sinf(t)*0.5f+0.5f)).rgb;
  float3 c = swi3(texture(iChannel1, g),x,y,z);
  //c = smoothstep(c+0.5f, c, to_float3_s(0.71f));
  c = smoothstep(c+0.5f, c, tune);
  return c;
}

///////FRAMEWORK////////////////////////////////////
#define mPi 3.14159f
#define m2Pi 6.28318f
__DEVICE__ float2 uvMap(float3 p)
{
  p = normalize(p);
  float2 tex2DToSphere3D;
  tex2DToSphere3D.x = 0.5f + _atan2f(p.z, p.x) / m2Pi;
  tex2DToSphere3D.y = 0.5f - asin(p.y) / mPi;
  return tex2DToSphere3D;
}

__DEVICE__ float4 displacement(float3 p, float iTime, __TEXTURE2D__ iChannel1, float3 tune)
{
  float3 col = effect(swi2(p,x,z), iTime, iChannel1, tune);
    
  col = clamp(col, to_float3_s(0), to_float3_s(1.0f));
    
  float dist = dot(col,to_float3_s(0.3f)); 
    
  return to_float4(dist,col.x,col.y,col.z);
}

////////BASE OBJECTS///////////////////////
__DEVICE__ float obox( float3 p, float3 b ){ return length(_fmaxf(abs_f3(p)-b,to_float3_s(0.0f)));}
__DEVICE__ float osphere( float3 p, float r ){ return length(p)-r;}
////////MAP////////////////////////////////
__DEVICE__ float4 map(float3 p, float iTime, __TEXTURE2D__ iChannel1, float3 tune)
{
  float scale = 1.0f; // displace scale
  float dist = 0.0f;
    
  float x = 6.0f;
  float z = 6.0f;
    
  float4 disp = displacement(p,iTime,iChannel1, tune);
        
  float y = 1.0f - smoothstep(0.0f, 1.0f, disp.x) * scale;
    
  dist = osphere(p, +5.0f-y);
    
  return to_float4(dist,disp.y,disp.z,disp.w);
}

__DEVICE__ float3 calcNormal( in float3 pos, float prec, float iTime, __TEXTURE2D__ iChannel1, float3 tune )
{
  float3 eps = to_float3( prec, 0.0f, 0.0f );
  float3 nor = to_float3(
        map(pos+swi3(eps,x,y,y),iTime,iChannel1, tune).x - map(pos-swi3(eps,x,y,y),iTime,iChannel1, tune).x,
        map(pos+swi3(eps,y,x,y),iTime,iChannel1, tune).x - map(pos-swi3(eps,y,x,y),iTime,iChannel1, tune).x,
        map(pos+swi3(eps,y,y,x),iTime,iChannel1, tune).x - map(pos-swi3(eps,y,y,x),iTime,iChannel1, tune).x );
  return normalize(nor);
}

__DEVICE__ float calcAO( in float3 pos, in float3 nor, float iTime, __TEXTURE2D__ iChannel1, float3 tune )
{
  float occ = 0.0f;
  float sca = 1.0f;
  for( int i=0; i<5; i++ )
  {
    float hr = 0.01f + 0.12f*(float)(i)/4.0f;
    float3 aopos =  nor * hr + pos;
    float dd = map( aopos,iTime,iChannel1, tune ).x;
    occ += -(dd-hr)*sca;
    sca *= 0.95f;
  }
  return clamp( 1.0f - 3.0f*occ, 0.0f, 1.0f );    
}

////////MAIN///////////////////////////////
__KERNEL__ void AudioReactiveRayMarchingFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(FreqR, 0.0f, 1.0f, 0.05f);
    CONNECT_SLIDER1(FreqG, 0.0f, 1.0f, 0.3f);
    CONNECT_SLIDER2(FreqB, 0.0f, 1.0f, 0.65f);
    
    CONNECT_SLIDER3(AmpR, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER4(AmpG, 0.0f, 10.0f, 1.0f);
    CONNECT_SLIDER5(AmpB, 0.0f, 10.0f, 1.0f);
    
    CONNECT_COLOR0(Tune, 0.71f, 0.71f, 0.71f, 1.0f);
    CONNECT_CHECKBOX0(Audio, 0);
    
    
    float Alpha = Tune.w;
    float3 tune = swi3(Tune,x,y,z);
    
    if (Audio)
    {
      tune.x = (1.0f-texture(iChannel2, to_float2(FreqR, 0)).x * AmpR);
      tune.y = (1.0f-texture(iChannel2, to_float2(FreqG, 0)).x * AmpG);
      tune.z = (1.0f-texture(iChannel2, to_float2(FreqB, 0)).x * AmpB);
    }
      
    
    
    float dstef = 0.0f; 

    float cam_a = 3.3f; // angle z
    
    float cam_e = 6.0f; // elevation
    float cam_d = 2.0f; // distance to origin axis
     
    float3 camUp=to_float3(0,1,0);//Change camere up vector here
    float3 camView=to_float3(0,0,0); //Change camere view here
    float li = 0.6f; // light intensity
    float prec = 0.00001f; // ray marching precision
    float maxd = 50.0f; // ray marching distance max
    float refl_i = 0.6f; // reflexion intensity
    float refr_a = 1.2f; // refraction angle
    float refr_i = 0.8f; // refraction intensity
    float bii = 0.35f; // bright init intensity
    float marchPrecision = 0.5f; // ray marching tolerance precision
    
    /////////////////////////////////////////////////////////
    if ( iMouse.z>0.0f) cam_e = iMouse.x/iResolution.x * 10.0f; // mouse x axis 
    if ( iMouse.z>0.0f) cam_d = iMouse.y/iResolution.y * 50.0f; // mouse y axis 
    /////////////////////////////////////////////////////////
    
    float2 uv = fragCoord / iResolution * 2.0f -1.0f;
    uv.x*=iResolution.x/iResolution.y;
    
    float3 col = to_float3_s(0.0f);
    
    float3 ro = to_float3(-_sinf(cam_a)*cam_d, cam_e+1.0f, _cosf(cam_a)*cam_d); //
    float3 rov = normalize(camView-ro);
    float3 u = normalize(cross(camUp,rov));
    float3 v = cross(rov,u);
    float3 rd = normalize(rov + uv.x*u + uv.y*v);
    
    float b = bii;
    
    float d = 0.0f;
    float3 p = ro+rd*d;
    float s = prec;
    
    float3 ray, cubeRay;
    
  for(int i=0;i<250;i++)
  {      
    if (s<prec||s>maxd) break;
    s = map(p,iTime,iChannel1, tune).x*marchPrecision;
    d += s;
    p = ro+rd*d;
  }

  if (d<maxd)
  {
    float2 e = to_float2(-1.0f, 1.0f)*0.005f; 
    float3 n = calcNormal(p, 0.1f,iTime,iChannel1, tune);

    b=li;

    ray = reflect(rd, n);
    cubeRay = swi3(decube_f3(iChannel0,ray),x,y,z)  * refl_i ;

    ray = _refract_f3(ray, n, refr_a);
    cubeRay += swi3(decube_f3(iChannel0,ray),x,y,z) * refr_i;

    col = cubeRay+_powf(b,15.0f); 

            
    // lighting        
    float occ = calcAO( p, n, iTime, iChannel1, tune );
    float3  lig = normalize( to_float3(-0.6f, 0.7f, -0.5f) );
    float amb = clamp( 0.5f+0.5f*n.y, 0.0f, 1.0f );
    float dif = clamp( dot( n, lig ), 0.0f, 1.0f );
    float bac = clamp( dot( n, normalize(to_float3(-lig.x,0.0f,-lig.z))), 0.0f, 1.0f )*clamp( 1.0f-p.y,0.0f,1.0f);
    float dom = smoothstep( -0.1f, 0.1f, cubeRay.y );
    float fre = _powf( clamp(1.0f+dot(n,rd),0.0f,1.0f), 2.0f );
    float spe = _powf(clamp( dot( cubeRay, lig ), 0.0f, 1.0f ),16.0f);

    float3 brdf = to_float3_s(0.0f);
    brdf += 1.20f*dif*to_float3(1.00f,0.90f,0.60f);
    brdf += 1.20f*spe*to_float3(1.00f,0.90f,0.60f)*dif;
    brdf += 0.30f*amb*to_float3(0.50f,0.70f,1.00f)*occ;
    brdf += 0.40f*dom*to_float3(0.50f,0.70f,1.00f)*occ;
    brdf += 0.30f*bac*to_float3(0.25f,0.25f,0.25f)*occ;
    brdf += 0.40f*fre*to_float3(1.00f,1.00f,1.00f)*occ;
    brdf += 0.02f;
    col = col*brdf;

    col = _mix( col, to_float3(0.8f,0.9f,1.0f), 1.0f-_expf( -0.0005f*d*d ) );

    col = _mix(col, swi3(map(p,iTime,iChannel1, tune),y,z,w), 0.5f);

  }
  else
  {
    col = swi3(decube_f3(iChannel0,rd),x,y,z);
  }

  //col *= dstef;
    
  //swi3S(fragColor,x,y,z, col);
  fragColor = to_float4_aw(col,Alpha);

  SetFragmentShaderComputedColor(fragColor);
}