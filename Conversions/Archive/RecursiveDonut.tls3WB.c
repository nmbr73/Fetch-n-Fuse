
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


#define PI 3.1415926f
#define fdist 0.3f
#define iters 40
#define tol 0.005f
#define maxdist 5.0f
#define gradient_eps 0.01f

//shape parameters
//#define ring_count 7
//#define ringdiff 5.0f
#define min_rings 3.0f
#define max_rings 10.0f
#define levels 4
#define ratio 0.4f
#define ring_offset 1.5f
#define offsetdiff 0.8f
#define indent 0.2f
#define base_radius 2.0f
#define ao_radius 0.05f
#define ao_min 0.2f
#define repeat_offset 8.0f
#define laplace_factor 100.0f
#define reflections 1
#define reflection_eps 0.01f
#define reflection_albedo 0.3f
#define light_dir to_float3(0.436436f,0.872872f,0.218218f)
#define n1 1.0f
#define n2 1.0f
#define plane_height -2.0f
#define shadow_step 0.05f
#define shadow_eps 0.01f
#define shadow_iters 10
#define shadow_maxdist 1.5f
#define shadow_sharpness 2.0f
#define ambient 0.2f




__DEVICE__ float3 viridis_quintic( float x )
{
  x = clamp( x, 0.0f,1.0f );
  float4 x1 = to_float4( 1.0f, x, x * x, x * x * x ); // 1 x x2 x3
  float4 x2 = x1 * x1.w * x; // x4 x5 x6 x7
  return to_float3(
    dot( swi4(x1,x,y,z,w), to_float4( +0.280268003f, -0.143510503f, +2.225793877f, -14.815088879f ) ) + dot( swi2(x2,x,y), to_float2( +25.212752309f, -11.772589584f ) ),
    dot( swi4(x1,x,y,z,w), to_float4( -0.002117546f, +1.617109353f, -1.909305070f, +2.701152864f ) ) + dot( swi2(x2,x,y), to_float2( -1.685288385f, +0.178738871f ) ),
    dot( swi4(x1,x,y,z,w), to_float4( +0.300805501f, +2.614650302f, -12.019139090f, +28.933559110f ) ) + dot( swi2(x2,x,y), to_float2( -33.491294770f, +13.762053843f ) ) );
}

__DEVICE__ float2 sdTorus( float3 p, float2 t)
{
    float2 q = to_float2(length(swi2(p,x,z))-t.x,p.y);
    float d = length(q)-t.y;
    
    float theta = _atan2f(p.x, p.z); //outer angle
    return to_float2(d, theta);
}

__DEVICE__ float delay_sin(float t) {
    return _cosf(PI*((_fabs(mod_f(t, 2.0f)-1.0f)+t)*0.5f-0.5f));
}
__DEVICE__ float map(float3 p, float4 iMouse, float iTime, float2 iResolution) {
    //p = mod_f(p+0.5f*repeat_offset, repeat_offset)-0.5f*repeat_offset;
    //time-varying parameters (maybe replace with some inputs, or remove)
    float final_offset;
    if (iMouse.z < 1.0f)
        final_offset = offsetdiff*delay_sin(iTime*0.5f+1.0f) + ring_offset;
    else
        final_offset = (iMouse.y/iResolution.y-0.5f)*3.0f+2.0f;
    float final_ratio = ratio/final_offset;
    
    float ringdiff = (max_rings-min_rings)*0.5f;
    float ring_count = (max_rings+min_rings)*0.5f;
    float final_ringcount;
    if (iMouse.z < 1.0f)
      final_ringcount = ringdiff*delay_sin(iTime*0.5f)+ring_count;
    else
      final_ringcount = ringdiff*(iMouse.x/iResolution.x-0.5f)*2.0f + ring_count;
    float sector = 2.0f*PI/(final_ringcount);
    float outerrad = base_radius;
    float innerrad = outerrad*final_ratio;
    float2 h = sdTorus(p, to_float2(outerrad, innerrad));
    int i;
    float currindent = indent;
    float2 minh = h;
    
    for (i=0; i<levels; i++) {
        
        //mod polar coordinates
        float theta = mod_f(_fabs(h.y), sector)-sector/2.0f;
        
        //new cartesian coords
        float s = length(swi2(p,z,x));
        p.z = _cosf(theta)*s - outerrad;
        p.x = _sinf(theta)*s;
        p = swi3(p,z,x,y);
        
        //new torus
        outerrad = innerrad*final_offset;        
        innerrad = outerrad*final_ratio;
        h = sdTorus(p, to_float2(outerrad, innerrad));
        
        minh.x = _fmaxf(minh.x, currindent-h.x);
        if (h.x < minh.x) {
            minh = h;
        }

        currindent = currindent * final_ratio * final_offset;
    }
    return minh.x;
}

__DEVICE__ float4 gradient(in float3 pos, float4 iMouse, float iTime, float2 iResolution) {
    float3 offset = to_float3(-gradient_eps, 0.0f, gradient_eps);
    float dx0 = map(pos+swi3(offset,x,y,y),iMouse,iTime,iResolution);
    float dxf = map(pos+swi3(offset,z,y,y),iMouse,iTime,iResolution);
    float dy0 = map(pos+swi3(offset,y,x,y),iMouse,iTime,iResolution);
    float dyf = map(pos+swi3(offset,y,z,y),iMouse,iTime,iResolution);
    float dz0 = map(pos+swi3(offset,y,y,x),iMouse,iTime,iResolution);
    float dzf = map(pos+swi3(offset,y,y,z),iMouse,iTime,iResolution);
    float ddd = map(pos,iMouse,iTime,iResolution);
    return to_float4_aw(normalize(to_float3(dxf - dx0, dyf - dy0, dzf - dz0)), dx0+dxf+dy0+dyf+dz0+dzf-6.0f*ddd);
}

__DEVICE__ float2 raymarch(float3 pos, float3 dir, float4 iMouse, float iTime, float2 iResolution) {
    int i;
    float d = 0.0f;
    float dist;
    for (i=0; i<iters; i++) {
        dist = map(pos+d*dir,iMouse,iTime,iResolution);
        d += dist;
        if (dist < tol) {
            return to_float2(d, 2.0f);
        } else if (dist > maxdist) {
            break;
        }
    }
  d = (plane_height-pos.y) / dir.y;
  return to_float2(d, step(-d, 0.0f)*step(length(swi2(pos+d*dir,z,x)), 50.0f));
}

//softer soft shadows
//see https://www.shadertoy.com/view/4tBcz3
__DEVICE__ float shadowtrace(float3 pos, float3 dir, float4 iMouse, float iTime, float2 iResolution) {
    int i;
    float d = shadow_eps;
    float dist = map(pos+d*dir,iMouse,iTime,iResolution);
    float fac = 1.0f;
    for (i=0; i<shadow_iters; i++) {
        d += _fmaxf(0.01f, dist);
        dist = map(pos+d*dir,iMouse,iTime,iResolution);
        fac = _fminf(fac, dist * shadow_sharpness / d);
    }
    return _mix(_mix(0.5f, 0.0f, -fac), _mix(0.5f, 1.0f, fac), step(fac, 0.0f));
}

__DEVICE__ float3 skycol(float3 rd) {
    return to_float3(0.6f, 0.7f, 0.8f)*(1.0f+_powf(_fmaxf(dot(rd, light_dir), 0.0f), 2.0f)) + _powf(_fmaxf(0.0f,dot(rd, light_dir)), 5.0f);
}

__DEVICE__ float schlick(float3 rd, float3 n, float R0) {
    return 1.0f-(R0+(1.0f-R0)*_powf(_fmaxf(dot(swi3(n,x,y,z), -rd), 0.0f), 5.0f));
}

__DEVICE__ float3 material(float3 ro, float3 rd, float4 n, float2 record, float _ratio, float scale[2], __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    
    float2 uv = swi2((ro+rd*record.x),z,x);

    uv.x/=_ratio;
    
    float2 uv1 = uv*scale[0];
    float2 uv2 = uv*scale[1];
    
    if (record.y > 1.5f) {
        float edgefac = _fabs(n.w*laplace_factor);
        float3 color = 1.0f-swi3(viridis_quintic(edgefac),y,x,z)*0.5f;
        float fac = _fmaxf(ambient, dot(light_dir, swi3(n,x,y,z)));
        //float ao = _fminf(1.0f,ao_min+(record.z > ao_radius ? 1.0f : record.z/(ao_radius)));
        
        float4 tex = _tex2DVecN(iChannel1, uv2.x,uv2.y,15);
        
        if (tex.w) color = swi3(tex,x,y,z);
        
        
        return fac*color;
    } else if (record.y > 0.5f) {
        //float2 uv = swi2((ro+rd*record.x),z,x);
        //uv = abs_f2(mod_f2(uv, 4.0f)-2.0f);
        float checker = _fabs(step(uv.x, 1.0f) - step(uv.y, 1.0f));
        
        float3 tex = swi3(_tex2DVecN(iChannel0, uv1.x,uv1.y,15),x,y,z);
       
        //return to_float3_s(light_dir.y*(0.5f+0.5f*checker));
        return (light_dir.y*tex);
    } else {
        return skycol(rd);
    }
}

//materials with reflections
__DEVICE__ float3 shade(float3 ro, float3 rd, float4 n, float2 record, float4 iMouse, float iTime, float2 iResolution, float R0, float _ratio, float scale[2], __TEXTURE2D__ iChannel0, __TEXTURE2D__ iChannel1) {
    float3 shadedcolor = material(ro, rd, n, record,_ratio, scale,iChannel0,iChannel1);
    if (record.y > 0.5f) {
        float fac = shadowtrace(ro+rd*record.x, light_dir,iMouse,iTime,iResolution);
        shadedcolor *= _fmaxf(ambient, fac);
    }
    if (record.y > 1.5f) {
        int i;
        float final_albedo = reflection_albedo;
        for (i=0; i<reflections; i++) {
            if (record.y < 1.5f) break;
            final_albedo *= schlick(rd, swi3(n,x,y,z),R0);
            ro = ro+rd*record.x;
            rd = reflect(rd, swi3(n,x,y,z));
            ro += reflection_eps*rd;
            record = raymarch(ro, rd,iMouse,iTime,iResolution);
            n = gradient(ro+rd*record.x,iMouse,iTime,iResolution);
            shadedcolor += final_albedo * material(ro, rd, n, record, _ratio, scale,iChannel0,iChannel1);
        }
        //compute last reflections with just envmap
        if (record.y > 1.5f) {
            final_albedo *= schlick(rd, swi3(n,x,y,z),R0);
            shadedcolor += final_albedo * skycol(reflect(rd, swi3(n,x,y,z)));
        }
    }
    return shadedcolor;
}

__KERNEL__ void RecursiveDonutFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{
  
    CONNECT_SLIDER0(scaleTex1,0.0f,3.0f,1.0f); 
    CONNECT_SLIDER1(scaleTex2,0.0f,3.0f,1.0f); 
  
    float scale[2] = {scaleTex1,scaleTex2};
  
  
    float _ratio = iResolution.x/iResolution.y;
  
    float R0 = (n1-n2)/(n1+n2);
    R0*=R0;
    //camera position
    float s = _sinf(iTime*0.5f);
    float ww = iTime*0.2f;
    float3 ro = (3.0f-s)*to_float3(_cosf(ww),0.5f+0.5f*s,_sinf(ww));
    float3 w = normalize(to_float3(0.0f,-1.5f-s,0.0f)-ro);
    float3 u = normalize(cross(w, to_float3(0.0f, 10.0f, 0.0f)));
    float3 v = cross(u, w);
    float3 rd = normalize(w*fdist+(fragCoord.x/iResolution.x-0.5f)*u+(fragCoord.y-iResolution.y/2.0f)/iResolution.x*v);
  
    float2 record = raymarch(ro, rd,iMouse,iTime,iResolution);
    float4 n = gradient(ro+rd*record.x,iMouse,iTime,iResolution);
    float3 shadedcolor = shade(ro, rd, n, record,iMouse,iTime,iResolution,R0,_ratio,scale,iChannel0,iChannel1);
    
    fragColor = to_float4_aw(shadedcolor, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}