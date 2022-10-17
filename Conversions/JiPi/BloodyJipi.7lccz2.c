
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// Originally from tomkh's wave equation solver
// https://www.shadertoy.com/view/Xsd3DB
//

#define HEIGHTMAPSCALE 90.0f

__DEVICE__ float3 cam( in float2 p, out float3 *cameraPos )
{
    // camera orbits around origin
    float camRadius = 50.0f;
    float theta = -3.141592653f/2.0f;
    float xoff = camRadius * _cosf(theta);
    float zoff = camRadius * _sinf(theta);
    *cameraPos = to_float3(xoff,30.0f,zoff);
     
    // camera target
    float3 target = to_float3(0.0f,0.0f,-30.0f);
     
    // camera frame
    float3 fo = normalize(target - *cameraPos);
    float3 ri = normalize(to_float3(fo.z, 0.0f, -fo.x ));
    float3 up = normalize(cross(fo,ri));
     
    // multiplier to emulate a fov control
    float fov = 0.5f;
  
    // ray direction
    float3 rayDir = normalize(fo + fov*p.x*ri + fov*p.y*up);
  
  return rayDir;
}

__KERNEL__ void BloodyJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    float3 e = to_float3_aw(to_float2_s(1.0f)/iResolution,0.0f);
    float2 q = fragCoord/iResolution;

    float p11 = _tex2DVecN(iChannel0,q.x,q.y,15).x;
    float p10 = texture(iChannel1, q-swi2(e,z,y)).x;
    float p01 = texture(iChannel1, q-swi2(e,x,z)).x;
    float p21 = texture(iChannel1, q+swi2(e,x,z)).x;
    float p12 = texture(iChannel1, q+swi2(e,z,y)).x;
float AAAAAAAAAAAAAAAA;
    // accel on fluid surface
    float d = 0.0f;

    if( iMouse.z > 0.0f )
    {
        float3 ro;
        float3 rd = cam( 2.0f*swi2(iMouse,x,y)/iResolution - 1.0f, &ro );
        if( rd.y < 0.0f )
        {
            float3 mp = ro + rd * ro.y/-rd.y;
            float2 uv = swi2(mp,x,z)/HEIGHTMAPSCALE + 0.5f;
            float screenscale = iResolution.x/640.0f;
            d += 0.06f*smoothstep(20.0f*screenscale,5.0f*screenscale,length(uv*iResolution - fragCoord));
        }
    }
    
    // force from video sampled by buffer B to avoid vid sync problems
    d += _tex2DVecN(iChannel1,q.x,q.y,15).y;

    // The actual propagation:
    d += -(p11-0.5f)*2.0f + (p10 + p01 + p21 + p12 - 2.0f);
    d *= 0.97f; // damping
    if( iFrame == 0 ) d = 0.0f;
    d = d*0.5f + 0.5f;

    fragColor = to_float4(d, 0.0f, 0.0f, 0.0f);
      
    SetFragmentShaderComputedColor(fragColor);
}


// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B '/media/a/35c87bcb8d7af24c54d41122dadb619dd920646a0bd0e477e7bdc6d12876df17.webm' to iChannel2
// Connect Buffer B 'Previsualization: Buffer A' to iChannel1
// Connect Buffer B 'Previsualization: Buffer B' to iChannel0


// Originally from tomkh's wave equation solver
// https://www.shadertoy.com/view/Xsd3DB
//

//#define HEIGHTMAPSCALE 90.0

//float3 cam( in float2 p, out float3 cameraPos );

__KERNEL__ void BloodyJipiFuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

    fragCoord+=0.5f;

    float3 e = to_float3_aw(to_float2_s(1.0f)/iResolution,0.0f);
    float2 q = fragCoord/iResolution;

    float p11 = _tex2DVecN(iChannel0,q.x,q.y,15).x;

    float p10 = texture(iChannel1, q-swi2(e,z,y)).x;
    float p01 = texture(iChannel1, q-swi2(e,x,z)).x;
    float p21 = texture(iChannel1, q+swi2(e,x,z)).x;
    float p12 = texture(iChannel1, q+swi2(e,z,y)).x;

    // accel on fluid surface
    float d = 0.0f;

    if( iMouse.z > 0.0f )
    {
        float3 ro;
        float3 rd = cam( 2.0f*swi2(iMouse,x,y)/iResolution - 1.0f, &ro );
        if( rd.y < 0.0f )
        {
            float3 mp = ro + rd * ro.y/-rd.y;
            float2 uv = swi2(mp,x,z)/HEIGHTMAPSCALE + 0.5f;
            float screenscale = iResolution.x/640.0f;
            d += 0.06f*smoothstep(20.0f*screenscale,5.0f*screenscale,length(uv*iResolution - fragCoord));
        }
    }
    
    // sample video
    float2 vuv = q*3.0f-to_float2(1.0f,0.17f);
    float d_vid = 0.0f;
    //if( vuv.x > 0.0f && vuv.x < 1.0f && vuv.y > 0.0f && vuv.y < 0.0f )
    d_vid = 0.106f*(_tex2DVecN(iChannel2,vuv.x,vuv.y,15).x-0.7f);
    d += d_vid;
    
    // The actual propagation:
    d += -(p11-0.5f)*2.0f + (p10 + p01 + p21 + p12 - 2.0f);
    d *= 0.97f; // damping
    if( iFrame == 0 ) d = 0.0f;
    d = d*0.5f + 0.5f;
float BBBBBBBBBBBBBBBB;
    fragColor = to_float4(d, d_vid, 0.0f, 0.0f);
    
    SetFragmentShaderComputedColor(fragColor);
}

// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


// remix of milky: https://www.shadertoy.com/view/Msy3D1
// which is based in turn on https://www.shadertoy.com/view/Xsd3DB

// turns out two sim steps per frame is important for motion to
// get nice, fast waves and oscillation

// this makes pretty terrible use of the simulation domain (comment
// out RAYMARCH to see it) but i like how the result looks at this scale.
// i should reduce the simulation domain size a la https://www.shadertoy.com/view/4dKGDw


#define RAYMARCH
//#define HEIGHTMAPSCALE 90.0f
#define MARCHSTEPS 8

__DEVICE__ float hscale = 4.0f;

//float3 cam( in float2 p, out float3 cameraPos );

__DEVICE__ float h( float3 p, __TEXTURE2D__ iChannel0 ) { return hscale*texture(iChannel0, swi2(p,x,z)/HEIGHTMAPSCALE + 0.5f ).x; }
// boost the step size, we resort to the secant method if we overstep the surface
__DEVICE__ float DE( float3 p, __TEXTURE2D__ iChannel0 ) { return 1.2f * ( p.y - h(p, iChannel0) ); }

__KERNEL__ void BloodyJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{

    float2 q = fragCoord/iResolution;
    float2 qq = q*2.0f-1.0f;
    float eps = 0.1f;
float IIIIIIIIIIIIIIIIIIIIIII;    
#ifdef RAYMARCH

    float3 c = to_float3_s(0.0f);
    
    float3 L = normalize(to_float3(0.3f,0.9f,1.0f));
    
    // raymarch the milk surface
    float3 ro;
    float3 rd = cam( qq, &ro );
    float t = 0.0f;
    float d = DE(ro+t*rd, iChannel0);
    
    for( int i = 0; i < MARCHSTEPS; i++ )
    {
        if( _fabs(d) < eps )
            break;
        
        float dNext = DE(ro+(t+d)*rd, iChannel0);
        
        // detect surface crossing, if so use secant method
        // https://www.shadertoy.com/view/Mdj3W3
        float dNext_over_d = dNext/d;
        if( dNext_over_d < 0.0f )
        {
          // estimate position of crossing
          d /= 1.0f - dNext_over_d;
          dNext = DE( ro+rd*(t+d), iChannel0 );
        }
        
    t += d;
    d = dNext;
    }
    
    // hit the BLOOD
    {
        float3 p = ro+t*rd;
        
        // finite difference normal
        float h0 = h(p, iChannel0);
        float2 dd = to_float2(0.01f,0.0f);
        float3 n = normalize(to_float3( h0-h(p + swi3(dd,x,y,y), iChannel0), dd.x, h0-h(p + swi3(dd,y,y,x), iChannel0) ));
        
        // diffuse / subtle subsurface
        float ndotL = clamp(dot(n,L),0.0f,1.0f);
        float dif = 1.52f*(0.7f+0.3f*ndotL);
        float ao = _mix( 0.6f, 0.64f, smoothstep(0.0f,1.0f,(h0+1.5f)/6.0f));
        float3 difCol = to_float3(0.82f,0.0f,0.0f);
        c = difCol*(dif)*ao;
        
        // specular
        float s = 0.6f*_powf( clamp( dot( L, reflect( rd, n ) ), 0.0f, 1.0f ), 4000.0f );
        c += s;
    }
    
  // vignette (borrowed from donfabio's Blue Spiral)
  float2 uv =  swi2(q,x,y)-0.5f;
  float distSqr = dot(uv, uv);
  c *= 1.0f - 1.0f*distSqr;
    
  fragColor = to_float4_aw(c,1.0f);  
    
#else
    float sh = 1.0f - _tex2DVecN(iChannel0,q.x,q.y,15).x;
    float3 c =
       to_float3(_expf(pow(sh-0.25f,2.0f)*-5.0f),
                 _expf(pow(sh-0.4f,2.0f)*-5.0f),
                 _expf(pow(sh-0.7f,2.0f)*-20.0f));
    fragColor = to_float4_aw(c,1.0f);
#endif

  SetFragmentShaderComputedColor(fragColor);

}
