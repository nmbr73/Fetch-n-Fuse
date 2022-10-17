
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------

// Connect Image 'Texture: Bubble' to iChannel1
// Connect Image 'Texture: Background' to iChannel0

// Thanks to https://www.shadertoy.com/view/XsX3zB for the 3d simplex noise

/* discontinuous pseudorandom uniformly distributed in [-0.5f, +0.5]^3 */
__DEVICE__ float3 random3(float3 c) {
  float j = 4096.0f*_sinf(dot(c,to_float3(17.0f, 59.4f, 15.0f)));
  float3 r;
  r.z = fract(512.0f*j);
  j *= 0.125f;
  r.x = fract(512.0f*j);
  j *= 0.125f;
  r.y = fract(512.0f*j);
  return r-0.5f;
}



/* 3d simplex noise */
__DEVICE__ float simplex3d(float3 p) {
  
   /* skew constants for 3d simplex functions */
   const float F3 =  0.3333333f;
   const float G3 =  0.1666667f;
  
   /* 1.0f find current tetrahedron T and it's four vertices */
   /* s, s+i1, s+i2, s+1.0f - absolute skewed (integer) coordinates of T vertices */
   /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
   
   /* calculate s and x */
   float3 s = _floor(p + dot(p, to_float3_s(F3)));
   float3 x = p - s + dot(s, to_float3_s(G3));
   
   /* calculate i1 and i2 */
   float3 e = step(to_float3_s(0.0f), x - swi3(x,y,z,x));
   float3 i1 = e*(1.0f - swi3(e,z,x,y));
   float3 i2 = 1.0f - swi3(e,z,x,y)*(1.0f - e);
     
   /* x1, x2, x3 */
   float3 x1 = x - i1 + G3;
   float3 x2 = x - i2 + 2.0f*G3;
   float3 x3 = x - 1.0f + 3.0f*G3;
   
   /* 2.0f find four surflets and store them in d */
   float4 w, d;
   
   /* calculate surflet weights */
   w.x = dot(x, x);
   w.y = dot(x1, x1);
   w.z = dot(x2, x2);
   w.w = dot(x3, x3);
   
   /* w fades from 0.6f at the center of the surflet to 0.0f at the margin */
   w = _fmaxf(to_float4_s(0.6f) - w, to_float4_s(0.0f));
   
   /* calculate surflet components */
   d.x = dot(random3(s), x);
   d.y = dot(random3(s + i1), x1);
   d.z = dot(random3(s + i2), x2);
   d.w = dot(random3(s + 1.0f), x3);
   
   /* multiply d by w^4 */
   w *= w;
   w *= w;
   d *= w;
   
   /* 3.0f return the sum of the four surflets */
   return dot(d, to_float4_s(52.0f));
}

__DEVICE__ float simplex3d_fractal(float3 m) {
    return   0.5333333f*simplex3d(m)
      +0.2666667f*simplex3d(2.0f*m)
      +0.1333333f*simplex3d(4.0f*m)
      +0.0666667f*simplex3d(8.0f*m);
}

__DEVICE__ float2 rotate( float2 p, float rad )
{
    float c = _cosf(rad);
    float s = _sinf(rad);
    mat2  m = to_mat2(c,-s,s,c);
    return mul_mat2_f2(m,p);
}

// polynomial smooth _fminf (k = 0.1f);
__DEVICE__ float smin( float a, float b, float k )
{
    float h = clamp( 0.5f+0.5f*(b-a)/k, 0.0f, 1.0f );
    return _mix( b, a, h ) - k*h*(1.0f-h);
}

__DEVICE__ float2 smin(float2 a, float2 b, float k) 
{
    return to_float2(smin(a.x,b.x,k),smin(a.y,b.y,k));
}

// f(x,y) divided by analytical gradient
__DEVICE__ float ellipse2(float2 p, float2 c, float2 s)
{
    p = p-c;
    float f = length( p/s );
    return (f-0.5f)*f/(length(p/(s*s)));
}
    

// signed distance to a 2D triangle
__DEVICE__ float sdTriangle( in float2 p0, in float2 p1, in float2 p2, in float2 p )
{
  float2 e0 = p1 - p0;
  float2 e1 = p2 - p1;
  float2 e2 = p0 - p2;

  float2 v0 = p - p0;
  float2 v1 = p - p1;
  float2 v2 = p - p2;

  float2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0f, 1.0f );
  float2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0f, 1.0f );
  float2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0f, 1.0f );
    
    float2 d = _fminf( _fminf( to_float2( dot( pq0, pq0 ), v0.x*e0.y-v0.y*e0.x ),
                               to_float2( dot( pq1, pq1 ), v1.x*e1.y-v1.y*e1.x )),
                               to_float2( dot( pq2, pq2 ), v2.x*e2.y-v2.y*e2.x ));

  return -_sqrtf(d.x)*sign_f(d.y);
}

__DEVICE__ float udRoundBox( float2 p, float2 c, float2 b, float r )
{
  return length(_fmaxf(abs_f2(p-c)-b,to_float2_s(0.0f)))-r;
}

__KERNEL__ void BubbleMagicJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse)
{

    CONNECT_CHECKBOX0(BKG_Tex, 0);
    CONNECT_CHECKBOX1(Bubble_Tex, 0);
    CONNECT_POINT0(BubblePosXY, 0.0f, 0.0f );
    
    CONNECT_POINT1(BubbleSizeXY, 0.0f, 0.0f );
    
    CONNECT_COLOR0(Color, 0.5f, 0.5f, 0.85f, 1.0f);
    
    CONNECT_SLIDER0(Level0, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER1(Level1, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER2(Level2, -10.0f, 10.0f, 1.0f);
    CONNECT_SLIDER3(Level3, -10.0f, 10.0f, 1.0f);
    
    CONNECT_SLIDER4(Level4, -10.0f, 10.0f, 1.0f);
    
  
    CONNECT_SLIDER5(TailW, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER6(TailRounding, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER7(TailTwist, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER8(JoinSmoothing, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER9(WobbleSize, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER10(WobbleFrequency, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER11(BodyWobble, -10.0f, 10.0f, 0.0f);
    CONNECT_SLIDER12(TailWobble, -10.0f, 10.0f, 0.0f);
    
    CONNECT_SLIDER13(ALP, 0.0f, 1.0f, 0.75f);
    

    float2 uv = fragCoord / iResolution.y;
    float2 tuv = uv;
    float2 mouse = swi2(iMouse,x,y) / iResolution.y;
    //vec2 uv = (-iResolution + 2.0f*fragCoord) / iResolution.y;
    float px = 2.0f/iResolution.y;
    float ratio = iResolution.x/iResolution.y;
    mouse.x = _fmaxf(mouse.x,0.5f*ratio-0.4f);
    mouse.x = _fminf(mouse.x,0.5f*ratio+0.4f);
    mouse.y = _fmaxf(mouse.y,0.2f);
    mouse.y = _fminf(mouse.y,0.8f);
    
    float3 col = to_float3_s(0.0f);
    float3 emi = to_float3_s(0.0f);
    
    if(BKG_Tex)
    {
        col = swi3(_tex2DVecN(iChannel0, uv.x/ratio,uv.y,15),x,y,z);
    }
    else
    // board
    {
        col = 0.6f*to_float3(0.4f,0.6f,0.7f)*(1.0f-0.4f*length( uv ));
        col *= 1.0f - 0.25f*smoothstep( 0.05f,0.15f,_sinf(uv.x*140.0f)*_sinf(uv.y*140.0f));
    }
    
    
    const bool box = true; // Set to false for elliptical bubble, true for rounded rectangle
    const bool bubbly = true; // Set to true for "thought bubble" bulbous protrusions
    
    float width = 0.5f + BubbleSizeXY.x;
    float height = 0.25f + BubbleSizeXY.y;
    float tailW = 0.075f + TailW; // Should not be larger than half the smallest dimension
    float tailRounding = 2.0f + TailRounding; // Will expand the tail by x pixels
    float tailTwist = -1.5f + TailTwist; // 
    float joinSmoothing = 0.05f + JoinSmoothing;
    
    float wobbleSize = 0.25f + WobbleSize;
    float wobbleFrequency = 0.5f + WobbleFrequency;
    float bodyWobble = 0.075f + BodyWobble;
    float tailWobble = 0.005f + TailWobble;
    
    
    float2 shadowOffset = to_float2(-5.5f,5.5f)*px;
    
    float rounding = width*0.15f;
    
    float3 color = swi3(Color,x,y,z);//to_float3(0.5f,0.5f,0.85f); // Blue
             //to_float3(0.75f,0.5f,0.25f); // Orange
    
    if(Bubble_Tex)
    {
       color = swi3(_tex2DVecN(iChannel1, tuv.x/ratio,tuv.y,15),x,y,z);
    }
    
    
    float2 bcen = to_float2(0.5f*ratio,0.5f); 
    float alp = ALP; //0.75f;
    float mouseDist = length(mouse-bcen);
    float rotation = tailTwist*sign_f((bcen - mouse).x)*sign_f((bcen - mouse).y);//*(mouse-bcen).x*-sign((mouse-bcen).y);
    float2 offset = normalize(to_float2(-(bcen - mouse).y,(bcen - mouse).x));
    
    //vec2 uvb = uv + fbm4(uv/wobbleSize,iTime*wobbleFrequency)*wobbleAmplitude;
    
    uv-=BubblePosXY;
    
    float noise = simplex3d(to_float3_aw(uv/wobbleSize,iTime*wobbleFrequency));
    if(bubbly) noise = -_powf(_fabs(noise),0.75f);
    
    float fshad = smin(sdTriangle(bcen+tailW*offset,bcen-tailW*offset,
                                  rotate(mouse-bcen,mouseDist*rotation)+bcen,
                                  rotate(uv+shadowOffset-bcen,length(uv+shadowOffset-bcen)*rotation)+bcen)-tailRounding*px+tailWobble*noise, 
                   box ? udRoundBox( uv+shadowOffset, bcen, 0.5f*to_float2(width,height)-rounding, rounding )+noise*bodyWobble
                       : ellipse2(uv+shadowOffset, bcen, to_float2(width,height))+noise*bodyWobble,joinSmoothing);
    
    float f = smin(sdTriangle(bcen+tailW*offset,bcen-tailW*offset,
                              rotate(mouse-bcen,mouseDist*rotation)+bcen,
                              rotate(uv-bcen,length(uv-bcen)*rotation)+bcen)-tailRounding*px+tailWobble*noise, 
                   box ? udRoundBox( uv, bcen, 0.5f*to_float2(width,height)-rounding, rounding )+noise*bodyWobble
                       : ellipse2(uv, bcen, to_float2(width,height))+noise*bodyWobble,joinSmoothing);
    col -= smoothstep(0.05f,-0.05f,fshad)*0.15f;
float IIIIIIIIIIIII;
    
    
    color *= 0.7f  + 0.3f*smoothstep( -3.0f*px, -1.0f*px, f );
    //color *= 0.75f + 8.0f*smoothstep( -3.0f*px, -1.0f*px, f );
    color *= 0.75f + Level0 + 8.0f*smoothstep( -3.0f*px*Level2, -1.0f*px*Level3, f+Level4 ) * Level1;
    
    //color = _mix(color,swi3(Color,x,y,z),f+Level4);
    
    
    col = _mix( col, color, alp*(1.0f-smoothstep( -px*2.0f, px*1.0f, f )) );

  fragColor = to_float4_aw(col,1.0f);

  SetFragmentShaderComputedColor(fragColor);
}