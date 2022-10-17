

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

// Thanks to https://www.shadertoy.com/view/XsX3zB for the 3d simplex noise

/* discontinuous pseudorandom uniformly distributed in [-0.5, +0.5]^3 */
vec3 random3(vec3 c) {
	float j = 4096.0*sin(dot(c,vec3(17.0, 59.4, 15.0)));
	vec3 r;
	r.z = fract(512.0*j);
	j *= .125;
	r.x = fract(512.0*j);
	j *= .125;
	r.y = fract(512.0*j);
	return r-0.5;
}

/* skew constants for 3d simplex functions */
const float F3 =  0.3333333;
const float G3 =  0.1666667;

/* 3d simplex noise */
float simplex3d(vec3 p) {
	 /* 1. find current tetrahedron T and it's four vertices */
	 /* s, s+i1, s+i2, s+1.0 - absolute skewed (integer) coordinates of T vertices */
	 /* x, x1, x2, x3 - unskewed coordinates of p relative to each of T vertices*/
	 
	 /* calculate s and x */
	 vec3 s = floor(p + dot(p, vec3(F3)));
	 vec3 x = p - s + dot(s, vec3(G3));
	 
	 /* calculate i1 and i2 */
	 vec3 e = step(vec3(0.0), x - x.yzx);
	 vec3 i1 = e*(1.0 - e.zxy);
	 vec3 i2 = 1.0 - e.zxy*(1.0 - e);
	 	
	 /* x1, x2, x3 */
	 vec3 x1 = x - i1 + G3;
	 vec3 x2 = x - i2 + 2.0*G3;
	 vec3 x3 = x - 1.0 + 3.0*G3;
	 
	 /* 2. find four surflets and store them in d */
	 vec4 w, d;
	 
	 /* calculate surflet weights */
	 w.x = dot(x, x);
	 w.y = dot(x1, x1);
	 w.z = dot(x2, x2);
	 w.w = dot(x3, x3);
	 
	 /* w fades from 0.6 at the center of the surflet to 0.0 at the margin */
	 w = max(0.6 - w, 0.0);
	 
	 /* calculate surflet components */
	 d.x = dot(random3(s), x);
	 d.y = dot(random3(s + i1), x1);
	 d.z = dot(random3(s + i2), x2);
	 d.w = dot(random3(s + 1.0), x3);
	 
	 /* multiply d by w^4 */
	 w *= w;
	 w *= w;
	 d *= w;
	 
	 /* 3. return the sum of the four surflets */
	 return dot(d, vec4(52.0));
}

float simplex3d_fractal(vec3 m) {
    return   0.5333333*simplex3d(m)
			+0.2666667*simplex3d(2.0*m)
			+0.1333333*simplex3d(4.0*m)
			+0.0666667*simplex3d(8.0*m);
}

vec2 rotate( vec2 p, float rad )
{
    float c = cos(rad);
    float s = sin(rad);
    mat2  m = mat2(c,-s,s,c);
    return m*p;
}

// polynomial smooth min (k = 0.1);
float smin( float a, float b, float k )
{
    float h = clamp( 0.5+0.5*(b-a)/k, 0.0, 1.0 );
    return mix( b, a, h ) - k*h*(1.0-h);
}

vec2 smin(vec2 a, vec2 b, float k) 
{
    return vec2(smin(a.x,b.x,k),smin(a.y,b.y,k));
}

// f(x,y) divided by analytical gradient
float ellipse2(vec2 p, vec2 c, vec2 s)
{
    p = p-c;
    float f = length( p/s );
    return (f-.5)*f/(length(p/(s*s)));
}
    

// signed distance to a 2D triangle
float sdTriangle( in vec2 p0, in vec2 p1, in vec2 p2, in vec2 p )
{
	vec2 e0 = p1 - p0;
	vec2 e1 = p2 - p1;
	vec2 e2 = p0 - p2;

	vec2 v0 = p - p0;
	vec2 v1 = p - p1;
	vec2 v2 = p - p2;

	vec2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0, 1.0 );
	vec2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0, 1.0 );
	vec2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0, 1.0 );
    
    vec2 d = min( min( vec2( dot( pq0, pq0 ), v0.x*e0.y-v0.y*e0.x ),
                       vec2( dot( pq1, pq1 ), v1.x*e1.y-v1.y*e1.x )),
                       vec2( dot( pq2, pq2 ), v2.x*e2.y-v2.y*e2.x ));

	return -sqrt(d.x)*sign(d.y);
}

float udRoundBox( vec2 p, vec2 c, vec2 b, float r )
{
  return length(max(abs(p-c)-b,0.0))-r;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.y;
    vec2 mouse = iMouse.xy / iResolution.y;
    //vec2 uv = (-iResolution.xy + 2.0*fragCoord.xy) / iResolution.y;
    float px = 2.0/iResolution.y;
    float ratio = iResolution.x/iResolution.y;
    mouse.x = max(mouse.x,.5*ratio-.4);
    mouse.x = min(mouse.x,.5*ratio+.4);
    mouse.y = max(mouse.y,.2);
    mouse.y = min(mouse.y,.8);
    
    vec3 col = vec3(0.0);
    vec3 emi = vec3(0.0);
    
    // board
    {
        col = 0.6*vec3(0.4,0.6,0.7)*(1.0-0.4*length( uv ));
        col *= 1.0 - 0.25*smoothstep( 0.05,0.15,sin(uv.x*140.0)*sin(uv.y*140.0));
    }
    
    const bool box = true; // Set to false for elliptical bubble, true for rounded rectangle
    const bool bubbly = true; // Set to true for "thought bubble" bulbous protrusions
    
    float width = .5;
    float height = .25;
    float tailW = .075; // Should not be larger than half the smallest dimension
    float tailRounding = 2.; // Will expand the tail by x pixels
    float tailTwist = -1.5; // 
    float joinSmoothing = .05;
    
    float wobbleSize = 0.25;
    float wobbleFrequency = 0.5;
    float bodyWobble = 0.075;
    float tailWobble = 0.005;
    
    
    vec2 shadowOffset = vec2(-5.5,5.5)*px;
    
    float rounding = width*0.15;
    vec3 color = vec3(.5,.5,.85); // Blue
        		 //vec3(.75,.5,.25); // Orange
    
    vec2 bcen = vec2(.5*ratio,.5); 
    float alp = .75;
    float mouseDist = length(mouse-bcen);
    float rotation = tailTwist*sign((bcen - mouse).x)*sign((bcen - mouse).y);//*(mouse-bcen).x*-sign((mouse-bcen).y);
    vec2 offset = normalize(vec2(-(bcen - mouse).y,(bcen - mouse).x));
    
    //vec2 uvb = uv + fbm4(uv/wobbleSize,iTime*wobbleFrequency)*wobbleAmplitude;
    float noise = simplex3d(vec3(uv/wobbleSize,iTime*wobbleFrequency));
    if(bubbly) noise = -pow(abs(noise),0.75);
    
    float fshad = smin(sdTriangle(bcen+tailW*offset,bcen-tailW*offset,
                              rotate(mouse-bcen,mouseDist*rotation)+bcen,
                              rotate(uv+shadowOffset-bcen,length(uv+shadowOffset-bcen)*rotation)+bcen)-tailRounding*px+tailWobble*noise, 
                   box?udRoundBox( uv+shadowOffset, bcen, 0.5*vec2(width,height)-rounding, rounding )+noise*bodyWobble: 
                   ellipse2(uv+shadowOffset, bcen, vec2(width,height))+noise*bodyWobble,
                       joinSmoothing);
    
    float f = smin(sdTriangle(bcen+tailW*offset,bcen-tailW*offset,
                              rotate(mouse-bcen,mouseDist*rotation)+bcen,
                              rotate(uv-bcen,length(uv-bcen)*rotation)+bcen)-tailRounding*px+tailWobble*noise, 
                   box?udRoundBox( uv, bcen, 0.5*vec2(width,height)-rounding, rounding )+noise*bodyWobble:
                   ellipse2(uv, bcen, vec2(width,height))+noise*bodyWobble,
                   joinSmoothing);
    col -= smoothstep(.05,-.05,fshad)*.15;

    color *= 0.7 + 0.3*smoothstep( -3.0*px, -1.0*px, f );
    color *= 0.75 + 8.*smoothstep( -3.0*px, -1.0*px, f );
    col = mix( col, color, alp*(1.0-smoothstep( -px*2., px*1., f )) );

	fragColor = vec4(col,1.0);
}