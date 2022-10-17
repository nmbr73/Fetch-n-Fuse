
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


#define PI 3.1415926538f
#define R iResolution

__DEVICE__ float getAngle(float2 v1)
{
    //return _atan2f(v1.x,v1.y) -_atan2f(v2.x,v2.y);
    return mod_f( _atan2f(v1.x,v1.y) -_atan2f(1.0f,0.0f), PI*2.0f)/PI/2.0f; //0 ... TWOPI
    //return mod_f( _atan2f(v1.x,v1.y) -_atan2f(v2.x,v2.y), TWOPI) - PI; //-pi to +pi 
}

__DEVICE__ float random1 (in float2 _st) {
    return fract(_sinf(dot(swi2(_st,x,y), to_float2(12.9898f,78.233f)))* 43758.5453123f);
}

__DEVICE__ float random(float2 p)
{
  float3 p3  = fract_f3(swi3(p,x,y,x) * 0.1031f);
    p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
    return fract((p3.x + p3.y) * p3.z);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
__DEVICE__ float noise (in float2 _st) {
    float2 i = _floor(_st);
    float2 f = fract_f2(_st);

    // Four corners in 2D of a tile
    float a = random1(i);
    float b = random1(i + to_float2(1.0f, 0.0f));
    float c = random1(i + to_float2(0.0f, 1.0f));
    float d = random1(i + to_float2(1.0f, 1.0f));

    float2 u = f * f * (3.0f - 2.0f * f);

    return _mix(a, b, u.x) +
            (c - a)* u.y * (1.0f - u.x) +
            (d - b) * u.x * u.y;
}

__DEVICE__ float sdSegment( in float2 p, in float2 a, in float2 b )
{
    float2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0f, 1.0f );
    return length( pa - ba*h );
}

__DEVICE__ float smin( float a, float b, float k )
{
    float h = _fmaxf( k-_fabs(a-b), 0.0f )/k;
    return _fminf( a, b ) - h*h*h*k*(1.0f/6.0f);
}

__DEVICE__ float2 sminC( float a, float b, float k )
{
    float h = _fmaxf( k-_fabs(a-b), 0.0f )/k;
    float m = h*h*h*0.5f;
    float s = m*k*(1.0f/3.0f); 
    return (a<b) ? to_float2(a-s,m) : to_float2(b-s,1.0f-m);
}


__DEVICE__ float _fwidth(float inp, float2 iR, float Par){
    //simulate fwidth
    float uvx = inp + Par/iR.x;
    float ddx = uvx * uvx - inp * inp;

    float uvy = inp + Par/iR.y;
    float ddy = uvy * uvy - inp * inp;

    return _fabs(ddx) + _fabs(ddy);
}


// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------


__DEVICE__ float2 pointad(float2 i,float dScale){
    //float a = random1(to_float2(i.x*100.2324f+ 102.23f))*PI*2.0f;
    //float d = random1(to_float2(i.x*12.1324f + 0.23452f, i.y/1000.0f*0.0f))*8.0f;
    
    float a = noise(to_float2_s(i.x*1000.2324f+ 102.23f) + i.y/3.0f)*PI*2.0f;
    float d = noise(to_float2_s(i.x*12.1324f + 0.23452f) + i.y)*dScale;
    
    return to_float2(a,d);
}

//float calcPointSDF(

//Need to use noise instead of random so that I get smooth values to alter with iTime;

__DEVICE__ float points(float2 p, int k, int n,float seed, float iTime, float2 R, float Par){
    float m0 = 1.0f;
    float m1 = 1.0f;
    
    float m = 1.0f;
    float smoth =1.3f;
    
    float2 point0 = to_float2_s(0.0f);
    float2 point1 = to_float2_s(0.0f);
    float a0 = 0.0f;
    //vec2 point2 = to_float2_s(0.0f);
    m0 = length(p - point0);
    for(int i = 0;i<k;i++){
        float2 ad = pointad( to_float2((float)(i), iTime/8.0f + seed),8.0f );
        float a = ad.x/1.0f;
        float d = ad.y;

        float2 point = to_float2(_cosf(a),_sinf(a))*d + point0;
        point1 = point;

        float2 direction = normalize(point - point0);
        float perpAngle = a - PI/2.0f;
        float2 perp = to_float2(_cosf(perpAngle),_sinf(perpAngle));
        float dAlongDir = dot(direction,p);

        //d/2.0f is halfway point for a nice stretch
        float scaler = _fabs(dAlongDir- d/2.0f);
        float thinnest = 0.1f;
        scaler = smoothstep(0.0f,5.0f,scaler)*0.1f*1.0f - (0.5f-thinnest);
        //point -= perp*scaler/10.0f;

        m1 = length(p - point);
        m1 = sdSegment(p, point0, point)- scaler;
        m0 = smin( m0, m1, smoth );
        a0 = a;
        
        for(int j = 0;j<n;j++){
            float2 ad = pointad( to_float2((float)(j+i), iTime/1.5f + seed),2.0f );
            float a = ad.x;
            
            float thrs = 0.2f;
            float left = (a/PI *(1.0f-2.0f*thrs) + thrs)*PI;
            float right = ((a - PI)/PI*(1.0f-2.0f*thrs) + (1.0f+thrs))*PI;
            float lrMix = smoothstep(0.0f,1.5f*_fwidth(a-PI,R,Par),a-PI);
            lrMix = smoothstep(0.0f,0.1f,a-PI);
            
            float aLR = _mix(left,right,lrMix);
            
            //a = aLR + a0;
                        
            float d = ad.y;
            
            float2 point2 = to_float2(_cosf(a),_sinf(a))*d + point1;
            float2 direction = normalize(point2 - point1);
            float perpAngle = a - PI/2.0f;
            float2 perp = to_float2(_cosf(perpAngle),_sinf(perpAngle));
            float dAlongDir = dot(direction,p)/length(p);

            //d/2.0f is halfway point for a nice stretch
            float scaler = _fabs(dAlongDir- d/2.0f);
            float thinnest = 0.025f;
            scaler = smoothstep(0.0f,5.0f,scaler)*0.05f*1.0f - (0.5f-thinnest);
            //point -= perp*scaler/10.0f;

            m1 = length(p - point2);
            m1 = sdSegment(p, point1, point2)- scaler;
            m0 = smin( m0, m1, smoth/2.0f );
        }
    }
 
    return m0;
}

__DEVICE__ float4 getPoints(float2 p, int k,int n, float iTime, float2 R, float Par){
    float s1 = 23503.23532f;
    float s2 = 533.7345f;
    float s3 = 2525.233f;
    
    float3 c1 = to_float3(0.2f,0.2f,0.7f);
    float3 c2 = to_float3(0.6f,0.1f,0.2f);
    float3 c3 = to_float3(0.7f,0.5f,0.2f);
    float3 c = to_float3_s(0.0f);

    float n1 = points(p,k,n,s1,iTime,R,Par);
    float n2 = points(p- to_float2(5.5f,0.0f),k,n,s2, iTime,R,Par);
    float n3 = points(p - to_float2(-5.0f,1.0f),6,3,s3, iTime,R,Par);
    float2 ns = sminC( n1, n2, 2.0f );
    c = _mix(c1,c2,smoothstep(0.0f,1.0f,ns.y));
    ns = sminC(ns.x,n3,2.0f);
    c = _mix(c,c3,smoothstep(0.0f,1.0f,ns.y));
    return to_float4(ns.x,c.x,c.y,c.z);;
}



__KERNEL__ void Neuronscolors2Jipi699Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, float iTime)
{
    CONNECT_SLIDER0(Par, -100.0f, 100.0f, 1.0f);
    CONNECT_SLIDER1(Par2, -100.0f, 100.0f, 1.0f);

    float2 uv = (fragCoord - R/2.0f)/R.y;
    uv *= 15.0f;
    //uv = _floor(uv*8.0f)/8.0f;
    //uv += iTime;
    float4 pm = getPoints(uv,8,2, iTime,R,Par);
    
    float f = pm.x-0.5f;
    float s = 1.0f-smoothstep(0.0f,1.5f*_fwidth(f,R,Par2),f)*1.0f;
    
    float3 c1 = to_float3(0.7f,0.6f,0.8f);
    float3 c2 = to_float3(0.2f,0.6f,0.4f);
    
    float2 dir = to_float2(-1.0f,1.0f);
    float dither = random1(_floor(uv*40.0f));
    
    
    float d = dot(dir,uv) + dither*1.0f;
    float mi = smoothstep(-3.2f,5.2f,d);
    
    float3 c = _mix(c1,c2,mi);
   
    float br = smoothstep(-2.0f,5.0f,length(uv))+0.5f;
    float edges = smoothstep(-0.15f,-0.05f,f);
    float thrs = 0.05f;
    edges = smoothstep(0.0f,1.5f*_fwidth(f+thrs,R,Par2),f+thrs);
    
    float h = 1.0f-smoothstep(0.0f,1.5f*_fwidth(f+thrs,R,Par2),f+thrs);
    
    
    float3 c12 = to_float3(0.2f,0.2f,0.7f);
    float3 c22 = to_float3(0.1f,0.46f,0.6f);
    float3 c32 = to_float3(0.4f,0.5f,0.7f);
    
    float3 c4 = (c12 + c22 + c32)/3.0f;
    float3 c5 = to_float3(0.5f,0.4f,1.0f);
    
    //vec3 col = to_float3(s)*swi3(pm,y,z,w)-h*0.0f;
    float3 col = _mix(to_float3_s(1.0f),swi3(pm,y,z,w),s); //outer
    col = _mix(to_float3_s(1.0f),col,edges); //inner
    
    col = pow_f3(col,to_float3_s(1.0f/2.2f));

    // Output to screen
    fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}