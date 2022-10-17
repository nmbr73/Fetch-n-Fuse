
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: Rusty Metal' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

#define R iResolution


#define PI 3.14159265359f
#define AA 1

  __DEVICE__ inline mat3 multi( float B, mat3 A)  
  {  
    return to_mat3_f3(A.r0 * B, A.r1 * B, A.r2 * B);  
  }

  __DEVICE__ inline mat3 inverse( mat3 A)  
  {  
   mat3 _R;  
   float result[3][3];  
   float a[3][3] = {{A.r0.x, A.r0.y, A.r0.z},  
                   {A.r1.x, A.r1.y, A.r1.z},  
                   {A.r2.x, A.r2.y, A.r2.z}};  
     
   float det = a[0][0] * a[1][1] * a[2][2]  
			 + a[0][1] * a[1][2] * a[2][0]  
			 + a[0][2] * a[1][0] * a[2][1]  
			 - a[2][0] * a[1][1] * a[0][2]  
			 - a[2][1] * a[1][2] * a[0][0]  
			 - a[2][2] * a[1][0] * a[0][1];  
   if( det != 0.0 )  
   {  
	   result[0][0] = a[1][1] * a[2][2] - a[1][2] * a[2][1];  
	   result[0][1] = a[2][1] * a[0][2] - a[2][2] * a[0][1];  
	   result[0][2] = a[0][1] * a[1][2] - a[0][2] * a[1][1];  
	   result[1][0] = a[2][0] * a[1][2] - a[1][0] * a[2][2];  
	   result[1][1] = a[0][0] * a[2][2] - a[2][0] * a[0][2];  
	   result[1][2] = a[1][0] * a[0][2] - a[0][0] * a[1][2];  
	   result[2][0] = a[1][0] * a[2][1] - a[2][0] * a[1][1];  
	   result[2][1] = a[2][0] * a[0][1] - a[0][0] * a[2][1];  
	   result[2][2] = a[0][0] * a[1][1] - a[1][0] * a[0][1];  
		 
	   _R = to_mat3_f3(make_float3(result[0][0], result[0][1], result[0][2]),   
	                   make_float3(result[1][0], result[1][1], result[1][2]), 
                     make_float3(result[2][0], result[2][1], result[2][2]));  
	   return multi( 1.0f / det, _R);  
   }  
   _R = to_mat3_f3(make_float3(1.0f, 0.0f, 0.0f), make_float3(0.0f, 1.0f, 0.0f), make_float3(0.0f, 0.0f, 1.0f));  
   return _R;  
  } 



//globals
struct leg {float3 p1, p2, p3, p4;};




//random////////////////////////////////////////////////////////////////////////////////

__DEVICE__ uint hash(uint seed){
    seed ^= 2747636419u;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    seed ^= seed >> 16;
    seed *= 2654435769u;
    
    return seed;
}
__DEVICE__ uint initRandomGenerator(float2 fragCoord, float2 R, int iFrame, uint seed){
    seed = uint(fragCoord.y*iResolution.x + fragCoord.x)+uint(iFrame)*uint(iResolution.x)*uint(iResolution.y);
    return seed;
}

__DEVICE__ float random(uint seed){
float xxxxxxxxxxxxxxxxxxxxx;    
    seed = hash(seed);
    return (float)(seed)/4294967295.0f;
}

__DEVICE__ float smoothNoise(float time, uint *seed){
    *seed = ((uint)(time)+45456u) * 23456u;
    float a = random(*seed);
    *seed = ((uint)(time)+45457u) * 23456u;
    float b = random(*seed);
    float t = fract(time);
    return _mix(a, b, t*t*(3.0f - 2.0f*t));
    
}

__DEVICE__ float timeFbm(float time, int depth, uint *seed){
    float a = 0.0f;
    float b = 1.0f;
    float t = 0.0f;
    for(int i = 0; i < depth; i++){
        float n = smoothNoise(time, seed);
        a += b*n;
        t += b;
        b *= 0.6f;
        time *= 2.0f; 
    }
    return a/t;
}
///////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////

__DEVICE__ float sdBox( float3 p, float size){
  return _fmaxf(max(_fabs(p.x), _fabs(p.y)), _fabs(p.z)) - size;
}

// http://iquilezles.org/www/articles/smin/smin.htm
__DEVICE__ float2 smin( float2 a, float2 b, float k )
{
    float h = clamp( 0.5f+0.5f*(b.x-a.x)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}


//https://www.shadertoy.com/view/Xds3zN
__DEVICE__ float dot2(float3 v) {return dot(v, v);}
__DEVICE__ float sdRoundCone(float3 p, float3 a, float3 b, float r1, float r2)
{
  //return length(p - b)  - 0.05f;
  // sampling independent computations (only depend on shape)
  float3  ba = b - a;
  float l2 = dot(ba,ba);
  float rr = r1 - r2;
  float a2 = l2 - rr*rr;
  float il2 = 1.0f/l2;
    
  // sampling dependant computations
  float3 pa = p - a;
  float y = dot(pa,ba);
  float z = y - l2;
  float x2 = dot2( pa*l2 - ba*y );
  float y2 = y*y*l2;
  float z2 = z*z*l2;

  // single square root!
  float k = sign_f(rr)*rr*rr*x2;
  if( sign_f(z)*a2*z2>k ) return  _sqrtf(x2 + z2)        *il2 - r2;
  if( sign_f(y)*a2*y2<k ) return  _sqrtf(x2 + y2)        *il2 - r1;
                          return (_sqrtf(x2*a2*il2)+y*rr)*il2 - r1;
}



//animation///////////////////////////////////////////////////////////////////////////
__DEVICE__ float2 walking_cycle(float t){
    return (t < 0.5f) ? to_float2(-1.0f + 4.0f*t, _sinf(2.0f*PI*t)) : to_float2(3.0f - 4.0f*t, 0.0f);
} 


__DEVICE__ void inverse_kinematic(in float3 p1, out float3 *p2, out float3 *p3, inout float3 *p4){
    
    const float leg_segment = 0.25f; // length of the leg / 3
    
    float d = distance_f3(p1, *p4);
    float3 u = (*p4 - p1)/d;
    
    if (d > leg_segment * 3.0f){
        *p2 = p1 + u * leg_segment;
        *p3 = p1 + 2.0f * u * leg_segment;
        *p4 = p1 + 3.0f * u * leg_segment;
        return;
    }
    
    float a = (d - leg_segment) * 0.5f;
    float h = _sqrtf(leg_segment*leg_segment - a*a);
    
    float3 b1 = cross(u, to_float3(0.0f, 1.0f, 0.0f));
    float3 up = normalize(cross(u, b1));
    
    *p2 =  p1 + u*a + h* up * sign_f(up.y);
    *p3 = *p2 + u*leg_segment;
    
    return; 
}

__DEVICE__ void compute_leg_pos(float t, uint *seed, mat3 *rotX, mat3 *rotZ, float3 *center, leg legs[4] ){
    *center =  to_float3(timeFbm(t, 3, seed)*0.2f - 0.1f, 
                         timeFbm(t + 10.0f, 4, seed)*0.2f - 0.1f + 0.05f*_sinf(4.0f*PI*t), 
                         timeFbm(t + 20.0f, 3, seed)*0.2f - 0.1f);
    
    float angleX = 0.3f*(timeFbm(t*4.0f + 30.0f, 2, seed)*2.0f - 1.0f);
    float angleZ = 0.3f*(timeFbm(t*4.0f + 30.0f, 2, seed)*2.0f - 1.0f);
    
    *rotX = to_mat3(1.0f, 0.0f, 0.0f,
                    0.0f, _cosf(angleX), -_sinf(angleX),
                    0.0f, _sinf(angleX), _cosf(angleX));
    
    *rotZ = to_mat3( _cosf(angleZ), -_sinf(angleZ),   0.0f,
                     _sinf(angleZ),  _cosf(angleZ),   0.0f,
                          0.0f    ,       0.0f    ,   1.0f   );
    
    for(int i=0; i < 4; i++){
        float i_f = (float)(i);
        float angle = 0.5f*PI*i_f;
        float3 direction = to_float3(_cosf(angle), 0.0f, _sinf(angle));
        
        legs[i].p1 = (*center + mul_mat3_f3(*rotZ , mul_mat3_f3(*rotX , direction * 0.2f)));
        legs[i].p4 = to_float3(0.2f, 0.1f, 0.1f)*swi3(walking_cycle(fract(t + 0.5f*i_f)),x,y,x) 
                    + to_float3(0.0f, -0.4f, 0.0f) + 0.53f * direction;
float zzzzzzzzzzzzzzz;        

        float3 _p2=legs[i].p2, _p3=legs[i].p3, _p4=legs[i].p4;  
        //inverse_kinematic(legs[i].p1, &legs[i].p2, &legs[i].p3, &legs[i].p4);
        inverse_kinematic(legs[i].p1, &_p2, &_p3, &_p4);
        legs[i].p2=_p2; legs[i].p3=_p3; legs[i].p4=_p4;  
        
    }
    
    *rotX = inverse(*rotX);
    *rotZ = inverse(*rotZ);
    
    return;
}

//render//////////////////////////////////////////////////////////////////////////////

__DEVICE__ float map(float3 p, out int *matID, float inside, mat3 rotX, mat3 rotZ, float3 center, leg legs[4]){
    float d = 1e5;
    float d2;
    *matID = 0;
     for(int i=0; i < 4; i++){
        d = _fminf(d, sdRoundCone(p, legs[i].p1, legs[i].p2, 0.1f, 0.06f));
        d = _fminf(d, sdRoundCone(p, legs[i].p2, legs[i].p3, 0.06f, 0.03f));
        d = _fminf(d, sdRoundCone(p, legs[i].p3, legs[i].p4, 0.03f, 0.001f));
     
    }
     
    float3 p2 = mul_mat3_f3(rotZ , mul_mat3_f3(rotX , p));
    d = _fminf(d,  sdBox(p2 - center, 0.17f));
    
    if (d > p.y + 0.4f){ //plane
        d = p.y + 0.4f;
        *matID = 2;
    }
    
    d *= inside;
    
    d2 = 1e5;
    for(int i=0; i < 4; i++){
        d2 = _fminf(d2, length(p - legs[i].p1) - 0.13f);
    }
    
    if (d > d2){ 
        d = d2;
        *matID = 1;
    }
    
    return d;
}

__DEVICE__ float map(float3 p, float inside, mat3 rotX, mat3 rotZ, float3 center, leg legs[4]){
    int i_;
    return map(p, &i_, inside, rotX, rotZ, center, legs);
}



#define OFFSET 0.0005f 
__DEVICE__ float3 normal(float3 p, float inside, mat3 rotX, mat3 rotZ, float3 center, leg legs[4]){
    return normalize(to_float3(map(p+to_float3(OFFSET,0,0),inside, rotX, rotZ, center, legs)-map(p-to_float3(OFFSET,0,0),inside, rotX, rotZ, center, legs),
                               map(p+to_float3(0,OFFSET,0),inside, rotX, rotZ, center, legs)-map(p-to_float3(0,OFFSET,0),inside, rotX, rotZ, center, legs),
                               map(p+to_float3(0,0,OFFSET),inside, rotX, rotZ, center, legs)-map(p-to_float3(0,0,OFFSET),inside, rotX, rotZ, center, legs))); 
}

__DEVICE__ float softshadow( in float3 ro, in float3 rd, float inside, mat3 rotX, mat3 rotZ, float3 center, leg legs[4]){
    float res = 1.0f;
    float tmax = 12.0f;  
    float t = OFFSET*4.0f;
    
    for( int i=0; i<100; i++ )
    {
    float h = map( ro + rd*t, inside, rotX, rotZ, center, legs);
        res = _fminf(res, 16.0f*h/t);
        t += h;
        if( h<OFFSET ) return 0.0f;
        if( t>tmax ) break;
    }
    return clamp(res, 0.0f, 1.0f);
}

__DEVICE__ float distance_scene(float3 ro, float3 rd, out int *matID, float inside, mat3 rotX, mat3 rotZ, float3 center, leg legs[4]){
    float t = 0.02f;
    float dt;
    
    for(int i=0; i < 100; i++){
        dt = map(ro + t*rd, matID, inside, rotX, rotZ, center, legs);
        t += dt;
        
        if(_fabs(dt) < OFFSET)
            return t;
        
        if(t > 10.0f)
            return -1.0f;
        
    }
    return -1.0f;
}

__DEVICE__ float fresnel(float3 rd, float3 n){
    float n2 = 1.0f, n1 = 1.330f;
    float R0 = _powf((n2 - n1)/(n2 + n1), 2.0f);
    float teta = -dot(n, rd);
    float _R = R0 + (1.0f - R0)*_powf(1.0f - teta, 5.0f);
    return _R;
}

__DEVICE__ float3 march(float3 ro, float3 rd, float iTime, float2 R, __TEXTURE2D__ iChannel1, mat3 rotX, mat3 rotZ, float3 center, leg legs[4]){
    float t;
    float3 n, col, mask, p;
    int matID;
    
        
    const float3 sun_dir = normalize(to_float3(-0.8f, 1.0f, 0.5f));
    
    float inside = 1.0f;
    mask = to_float3_s(1.0f);
    col = to_float3_s(0.0f);
    
    float3 bgCol = to_float3_s(0.0f);
    
    for(int i = 0; i < 6; i++){
        t = distance_scene(ro, rd, &matID, inside, rotX, rotZ, center, legs);
        if(t < 0.0f) return col + mask * bgCol;
        p = ro + t*rd;
        n = normal(p, inside, rotX, rotZ, center, legs); 
        ro = p;
        
        if (matID == 0){ //glass
            float f = fresnel(rd, n);
                float3 rd2 = reflect(rd, n);
                float s = softshadow(p, sun_dir, inside, rotX, rotZ, center, legs);
                
                
                if(dot(n, sun_dir) > 0.0f){
                    float3 sky = _mix(to_float3(0.5f, 0.4f, 0.4f), (to_float3_s(2.0f)), 0.5f*n.y + 0.5f);
                    col += s*f*mask*(sky + 70.0f*_powf(_fmaxf(dot(rd2, sun_dir), 0.0f), 30.0f));
                }
                mask *= 1.0f - f;
                rd = refract_f3(rd, n, 1.0f/1.33f);
                inside *= -1.0f;
                mask *= to_float3(1.0f, 0.85f, 0.85f);
            
            
        }
        
        if(matID == 1){ //cube
            mask *= to_float3(1.0f, 0.1f, 0.1f);
            col += mask *( to_float3(0.1f, 0.1f, 0.2f) * (map(p + n*0.02f, inside, rotX, rotZ, center, legs) / 0.02f) * (_fminf(1.0f, 1.0f + dot(sun_dir, n)))
                          + 1.0f*to_float3(1.30f,1.00f,0.70f) * dot(sun_dir, n) * softshadow(p, sun_dir, inside, rotX, rotZ, center, legs) );
            
            return col;
        }
        
        if(matID == 2){ //plane
            float3 tex = swi3(texture(iChannel1, swi2(p,x,z)*0.2f + 0.8f*to_float2(0.2f, 0.1f)*iTime),x,y,z);
            mask *= tex*tex;
            mask *= _expf(-dot(swi2(p,x,z) + to_float2(-0.5f, 0.5f), swi2(p,x,z) + to_float2(-0.5f, 0.5f)));
            col += mask *( to_float3(0.1f, 0.1f, 0.2f) * (_fminf(1.0f, 1.0f + dot(sun_dir, n)))
                          + 1.0f*to_float3(1.30f,1.00f,0.70f) * dot(sun_dir, n) * softshadow(p, sun_dir, inside, rotX, rotZ, center, legs) );
            return col;
        }

    }
    return col;
}



__KERNEL__ void WalkingRobotFuse__Buffer_A(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, int iFrame, sampler2D iChannel1)
{

    fragCoord+=0.5f;

    leg legs[4];

    mat3 rotX;
    mat3 rotZ;
    float3 center;

    uint seed = 0u;
    compute_leg_pos(iTime, &seed, &rotX, &rotZ, &center, legs);
    
    
    seed = initRandomGenerator(fragCoord, R,iFrame, seed);
    
    float2 uv = fragCoord/iResolution*2.0f-1.0f;
    uv.x *= iResolution.x/iResolution.y;
    
    float3 ro = to_float3(_sinf(0.2f), 0.5f, _cosf(0.2f));
    
    float3 dir0 = normalize(-ro);
    float3 up = to_float3(0.0f, 1.0f, 0.0f);
    float3 right = normalize(cross(up, dir0));
    up = cross(dir0, right);
    float3 rd;
    
    float3 col = to_float3_s(0.0f);
    for(int x = 0; x < AA; x++){
    for(int y = 0; y < AA; y++){
        rd = normalize(dir0 + right*(uv.x + (float)(x)/(float)(AA)/iResolution.x) 
                            + up*(uv.y + (float)(y)/(float)(AA)/iResolution.x));


        col += march(ro, rd, iTime, R, iChannel1, rotX, rotZ, center, legs);
    }
    }
    
    col /= (float)(AA*AA);
    
    fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Lichen' to iChannel1
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void WalkingRobotFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1)
{
    CONNECT_SLIDER0(Brightness, 0.0f, 10.0f, 1.0f);

    fragCoord+=0.5f;

    float2 uv = fragCoord/iResolution;
    float vign = _powf( 16.0f*uv.x*uv.y*(1.0f-uv.x)*(1.0f-uv.y), 0.8f );

    float3 col = swi3(_tex2DVecN(iChannel0,uv.x,uv.y,15),x,y,z);
    
    col = col*col*(3.0f - 2.0f*col);
    
    col += 0.01f*step(0.3f, texture(iChannel1, uv*2.0f).x * (1.0f - vign));
    
    col = to_float3_s(1.0f) - exp_f3(-0.5f*col);
    
    col = pow_f3(col, to_float3_s(1.0f/2.2f));
    
    col *= 0.3f + 0.7f*vign;
    
    col*=Brightness;
    
    fragColor = to_float4_aw(col, 1.0f);


  SetFragmentShaderComputedColor(fragColor);
}