
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/27012b4eadd0c3ce12498b867058e4f717ce79e10a99568cca461682d84a4b04.bin' to iChannel0


// Copyright Inigo Quilez, 2022 - https://iquilezles.org/
// I am the sole copyright owner of this Work.
// You cannot host, display, distribute or share this Work in any form,
// including physical and digital. You cannot use this Work in any
// commercial or non-commercial product, website or project. You cannot
// sell this Work and you cannot mint an NFTs of it.
// I share this Work for educational purposes, and you can link to it,
// through an URL, proper attribution and unmodified screenshot, as part
// of your educational material. If these conditions are too restrictive
// please contact me and we'll definitely work it out.

// Just a remix of https://www.shadertoy.com/view/MdfGRX

//------------------------------------------------------------------
// noise
//------------------------------------------------------------------
__DEVICE__ float hash( float n )
{
    return fract( n*17.0f*fract( n*0.3183099f ) );
}

__DEVICE__ float noise1( in float x )
{
    float p = _floor(x);
    float w = fract(x);
    float u = w*w*(3.0f-2.0f*w);
    return _mix(hash(p+0.0f),hash(p+1.0f),u);
}

__DEVICE__ float noise( in float3 x )
{
#if 1
    float3 p = _floor(x);
    float3 w = fract_f3(x);
    float3 u = w*w*(3.0f-2.0f*w);
    float n = 1.0f*p.x + 317.0f*p.y + 157.0f*p.z;
    return _mix( _mix( _mix(hash(n+  0.0f),hash(n+  1.0f),u.x),
                       _mix(hash(n+317.0f),hash(n+318.0f),u.x),u.y),
                 _mix( _mix(hash(n+157.0f),hash(n+158.0f),u.x),
                       _mix(hash(n+474.0f),hash(n+475.0f),u.x),u.y),u.z);   
#else
    //return textureLod(iChannel0,x/32.0f,0.0f).x;
#endif    
}

//------------------------------------------------------------------

__DEVICE__ float4 map( float3 p, float time )
{
  // density
  float den = 0.2f - p.y;

  // invert space  
  p = -7.0f*p/dot(p,p);

  // twist space  
  float co = _cosf(0.8f*den);
  float si = _sinf(0.8f*den);
  swi2S(p,x,z, mul_mat2_f2(to_mat2(co,-si,si,co),swi2(p,x,z)));

  // cloud  
  float f;
  float t = time + 9.0f;
  float3 q = p                           - to_float3(0.0f,t*0.2f,0.0f);
  f  = 0.500000f*noise( q ); q = q*2.21f - to_float3(0.0f,t*0.4f,0.0f);
  f += 0.250000f*noise( q ); q = q*2.15f - to_float3(0.0f,t*0.8f,0.0f);
  f += 0.125000f*noise( q ); q = q*2.13f - to_float3(0.0f,t*1.6f,0.0f);
  f += 0.062500f*noise( q ); q = q*2.05f - to_float3(0.0f,t*3.2f,0.0f);
  f += 0.031250f*noise( q );

  den = den + 4.0f*f + 0.015f;
  
  float3 col = _mix( to_float3_s(0.8f), to_float3_s(0.5f), den ) + 0.02f*sin_f3(p);
  
  return to_float4_aw( col, den );
}

__DEVICE__ float3 raymarch( in float3 ro, in float3 rd, in float2 pixel, float time )
{
  // lightining
  float li = 1.0f;
  li *= smoothstep(0.6f,0.65f,noise1( time*11.2f + 6.1f ));
  li *= smoothstep(0.4f,0.45f,noise1( time*1.1f + 6.1f ));

  // raymarch
  float4 sum = to_float4_s( 0.0f );
    
  const float stepFactor = 0.5f;

  // with dithering
  float t = 0.05f *fract(_sinf(time+pixel.x*11.0f+17.0f*pixel.y)*1.317f);    
  for( int i=0; i<256; i++ )
  {
    float3 pos = ro + t*rd;
    float4 col = map( pos, time );

        if( col.w>0.0f )
        {
            float len = length(pos);
            float at = smoothstep(2.0f,0.0f,len);
            swi3S(col,x,y,z, swi3(col,x,y,z) * _mix( 2.5f*to_float3(0.3f,0.4f,0.5f), 0.9f*to_float3(0.4f,0.45f,0.55f), clamp( (pos.y-0.1f)/2.0f, 0.0f, 1.0f ) ));
            swi3S(col,x,y,z, swi3(col,x,y,z) * 1.0f + 0.15f*at + 1.5f*li*at);

            //if( li>0.001f )
            {
            float3 dir = pos/len;
            float nn = _fmaxf(0.0f,col.w - map( pos-dir*0.05f, time ).w);
            swi3S(col,x,y,z, swi3(col,x,y,z) + 2.0f*li*(0.5f+1.5f*at)*nn*to_float3(0.8f,0.8f,0.8f)*(1.0f-col.w));
            }

            // fog
            swi3S(col,x,y,z, swi3(col,x,y,z) * 1.15f*_exp2f(-t*0.1f));

            // compose    
            col.w *= stepFactor;
            //swi3(col,x,y,z) *= col.w;
            col.x*=col.w;
            col.y*=col.w;
            col.z*=col.w;
            
            sum = sum + col*(1.0f - sum.w);  
            if( sum.w > 0.99f ) break;
        }

    t += 0.1f*stepFactor;
  }

  return clamp( swi3(sum,x,y,z), 0.0f, 1.0f );
}

__KERNEL__ void Hell2Fuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, sampler2D iChannel0)
{

  float time = iTime;

  float2 p = (2.0f*fragCoord-iResolution)/iResolution.y;
  
  // camera
  float3 ro = 4.0f*normalize(to_float3(1.0f, 1.5f, 0.0f));
  float3 ta = to_float3(0.0f, 1.0f, 0.0f);
  float cr = 0.4f*_cosf(0.4f*iTime);
  
  // shake    
  ro += 0.01f*(-1.0f+2.0f*noise1(3.1f*time));
  ta += 0.01f*(-1.0f+2.0f*noise1(3.3f*time));
  
  // build ray
  float3 ww = normalize( ta - ro);
  float3 uu = normalize(cross( to_float3(_sinf(cr),_cosf(cr),0.0f), ww ));
  float3 vv = normalize(cross(ww,uu));
  float3 rd = normalize( p.x*uu + p.y*vv + 2.0f*ww );
  
  // raymarch  
    
  float3 col = raymarch( ro, rd, fragCoord, time );

  // color grade
  col = col*col*(3.0f-2.0f*col);
  col = col*0.5f + 0.5f*col*col*(3.0f-2.0f*col);
  col *= 1.2f;
    
  // vignette
  float2 q = fragCoord / iResolution;
  col *= 0.1f + 0.9f*_powf( 16.0f*q.x*q.y*(1.0f-q.x)*(1.0f-q.y), 0.15f );

  fragColor = to_float4_aw( col, 1.0f );

  SetFragmentShaderComputedColor(fragColor);
}