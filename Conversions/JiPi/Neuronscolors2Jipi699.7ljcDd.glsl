

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

vec2 pointad(vec2 i,float dScale){
    //float a = random1(vec2(i.x*100.2324+ 102.23))*PI*2.;
    //float d = random1(vec2(i.x*12.1324 + 0.23452, i.y/1000.*0.))*8.;
    
    float a = noise(vec2(i.x*1000.2324+ 102.23) + i.y/3.)*PI*2.;
    float d = noise(vec2(i.x*12.1324 + 0.23452) + i.y)*dScale;
    
    
    return vec2(a,d);
}

//float calcPointSDF(

//Need to use noise instead of random so that I get smooth values to alter with iTime;

float points(vec2 p, int k, int n,float seed){
    float m0 = 1.;
    float m1 = 1.;
    
    float m = 1.;
    float smoth =1.3;
    
    vec2 point0 = vec2(0.);
    vec2 point1 = vec2(0.);
    float a0 = 0.;
    //vec2 point2 = vec2(0.);
    m0 = length(p - point0);
    for(int i = 0;i<k;i++){
        vec2 ad = pointad( vec2(float(i), iTime/8. + seed),8. );
        float a = ad.x/1.;
        float d = ad.y;


        vec2 point = vec2(cos(a),sin(a))*d + point0;
        point1 = point;

        vec2 direction = normalize(point - point0);
        float perpAngle = a - PI/2.;
        vec2 perp = vec2(cos(perpAngle),sin(perpAngle));
        float dAlongDir = dot(direction,p);

        //d/2. is halfway point for a nice stretch
        float scaler = abs(dAlongDir- d/2.);
        float thinnest = 0.1;
        scaler = smoothstep(0.,5.,scaler)*.1*1. - (0.5-thinnest);
        //point -= perp*scaler/10.;

        m1 = length(p - point);
        m1 = sdSegment(p, point0, point)- scaler;
        m0 = smin( m0, m1, smoth );
        a0 = a;
        
        for(int j = 0;j<n;j++){
            vec2 ad = pointad( vec2(float(j+i), iTime/1.5 + seed),2. );
            float a = ad.x;
            
            float thrs = 0.2;
            float left = (a/PI *(1.-2.*thrs) + thrs)*PI;
            float right = ((a - PI)/PI*(1.-2.*thrs) + (1.+thrs))*PI;
            float lrMix = smoothstep(0.,1.5*fwidth(a-PI),a-PI);
            lrMix = smoothstep(0.,0.1,a-PI);
            
            float aLR = mix(left,right,lrMix);
            
            //a = aLR + a0;
            
            
            float d = ad.y;
            

            vec2 point2 = vec2(cos(a),sin(a))*d + point1;

            vec2 direction = normalize(point2 - point1);
            float perpAngle = a - PI/2.;
            vec2 perp = vec2(cos(perpAngle),sin(perpAngle));
            float dAlongDir = dot(direction,p)/length(p);

            //d/2. is halfway point for a nice stretch
            float scaler = abs(dAlongDir- d/2.);
            float thinnest = 0.025;
            scaler = smoothstep(0.,5.,scaler)*.05*1. - (0.5-thinnest);
            //point -= perp*scaler/10.;

            m1 = length(p - point2);
            m1 = sdSegment(p, point1, point2)- scaler;
            m0 = smin( m0, m1, smoth/2. );
        }
        
        
    }
 
    return m0;
}

vec4 getPoints(vec2 p, int k,int n){
    float s1 = 23503.23532;
    float s2 = 533.7345;
    float s3 = 2525.233;
    
    vec3 c1 = vec3(0.2,.2,0.7);
    vec3 c2 = vec3(0.6,.1,0.2);
    vec3 c3 = vec3(0.7,.5,.2);
    vec3 c = vec3(0.);

    float n1 = points(p,k,n,s1);
    float n2 = points(p- vec2(5.5,0.),k,n,s2);
    float n3 = points(p - vec2(-5.,1.),6,3,s3);
    vec2 ns = sminC( n1, n2, 2. );
    c = mix(c1,c2,smoothstep(0.,1.,ns.y));
    ns = sminC(ns.x,n3,2.);
    c = mix(c,c3,smoothstep(0.,1.,ns.y));
    return vec4(ns.x,c);;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (fragCoord - R/2.)/R.y;
    uv *= 15.;
    //uv = floor(uv*8.)/8.;
    //uv += iTime;
    vec4 pm = getPoints(uv,8,2);


    
    float f = pm.x-0.5;
    float s = 1.-smoothstep(0.,1.5*fwidth(f),f)*1.;
    
    vec3 c1 = vec3(0.7,0.6,0.8);
    vec3 c2 = vec3(0.2,0.6,0.4);
    
    vec2 dir = vec2(-1.,1.);
    float dither = random1(floor(uv*40.));
    
    
    float d = dot(dir,uv) + dither*1.;
    float mi = smoothstep(-3.2,5.2,d);
    
    vec3 c = mix(c1,c2,mi);
    
    float br = smoothstep(-2.,5.,length(uv))+0.5;
    float edges = smoothstep(-.15,-.05,f);
    float thrs = 0.05;
    edges = smoothstep(0.,1.5*fwidth(f+thrs),f+thrs);
    
    float h = 1.-smoothstep(0.,1.5*fwidth(f+thrs),f+thrs);
    
    vec3 c12 = vec3(0.2,.2,0.7);
    vec3 c22 = vec3(0.1,.46,0.6);
    vec3 c32 = vec3(0.4,.5,.7);
    
    vec3 c4 = (c12 + c22 + c32)/3.;
    vec3 c5 = vec3(0.5,0.4,1.);

    
    //vec3 col = vec3(s)*pm.yzw-h*0.;
    vec3 col = mix(vec3(1.),pm.yzw,s); //outer
    col = mix(vec3(1.),col,edges); //inner
    
    col = pow(col,vec3(1./2.2));

    // Output to screen
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define PI 3.1415926538
#define R iResolution.xy

float getAngle(vec2 v1)
{
    //return atan(v1.x,v1.y) -atan(v2.x,v2.y);
    return mod( atan(v1.x,v1.y) -atan(1.,0.), PI*2.)/PI/2.; //0 ... TWOPI
    //return mod( atan(v1.x,v1.y) -atan(v2.x,v2.y), TWOPI) - PI; //-pi to +pi 
}

float random1 (in vec2 _st) {
    return fract(sin(dot(_st.xy,
                         vec2(12.9898,78.233)))*
        43758.5453123);
}

float random(vec2 p)
{
	vec3 p3  = fract(vec3(p.xyx) * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

// Based on Morgan McGuire @morgan3d
// https://www.shadertoy.com/view/4dS3Wd
float noise (in vec2 _st) {
    vec2 i = floor(_st);
    vec2 f = fract(_st);

    // Four corners in 2D of a tile
    float a = random1(i);
    float b = random1(i + vec2(1.0, 0.0));
    float c = random1(i + vec2(0.0, 1.0));
    float d = random1(i + vec2(1.0, 1.0));

    vec2 u = f * f * (3.0 - 2.0 * f);

    return mix(a, b, u.x) +
            (c - a)* u.y * (1.0 - u.x) +
            (d - b) * u.x * u.y;
}

float sdSegment( in vec2 p, in vec2 a, in vec2 b )
{
    vec2 pa = p-a, ba = b-a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h );
}

float smin( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    return min( a, b ) - h*h*h*k*(1.0/6.0);
}

vec2 sminC( float a, float b, float k )
{
    float h = max( k-abs(a-b), 0.0 )/k;
    float m = h*h*h*0.5;
    float s = m*k*(1.0/3.0); 
    return (a<b) ? vec2(a-s,m) : vec2(b-s,1.0-m);
}


