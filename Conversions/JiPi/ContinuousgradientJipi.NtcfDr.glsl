

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define DX (iMouse.x/iResolution.x)
#define DY (iMouse.y/iResolution.y)
#define BORDERRADIUS (6)
#define GAMMA       (2.2)
#define PI           (3.14159265359)
#define LUMWEIGHT    (vec3(0.2126,0.7152,0.0722))
#define pow3(x,y)      (pow( max(x,0.) , vec3(y) ))

#define BORDERRADIUSf float(BORDERRADIUS)
#define BORDERRADIUS22f float(BORDERRADIUS*BORDERRADIUS)

// https://www.shadertoy.com/view/MsS3Wc
// HSV to RGB conversion 
vec3 hsv2rgb_smooth( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );
	return c.z * mix( vec3(1.0), rgb, c.y);
}

vec2 viewport(vec2 p)
{   
    return p/(iResolution.xy);
}

vec3 sampleImage(vec2 coord){
   return pow3(texture(iChannel0,viewport(coord)).rgb,GAMMA);
}

float kernel(int a,int b){
    return float(a)*exp(-float(a*a + b*b)/BORDERRADIUS22f)/BORDERRADIUSf;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor.rgb = sampleImage(fragCoord.xy);
    
    vec3 col;
    vec3 colX = vec3(0.);
    vec3 colY = vec3(0.);
    float coeffX,coeffY;
    
    for( int i = -BORDERRADIUS ; i <= BORDERRADIUS ; i++ ){
    	for( int j = -BORDERRADIUS ; j <= BORDERRADIUS ; j++ ){
            coeffX = kernel(i,j);
        	coeffY = kernel(j,i);
            
            col = sampleImage(fragCoord.xy+vec2(i,j));
            colX += coeffX*col;
            colY += coeffY*col;
        }
        
    }
    
    vec3 derivative = sqrt( (colX*colX + colY*colY) )/(BORDERRADIUSf*BORDERRADIUSf);
    float angle = atan(dot(colY,LUMWEIGHT),dot(colX,LUMWEIGHT))/(2.*PI) + iTime*(1. - DX)/2.;
    vec3 derivativeWithAngle = hsv2rgb_smooth(vec3(angle,1.,pow(dot(derivative,LUMWEIGHT)*3.,3.)*5.));
    
    fragColor.rgb = mix(derivative,fragColor.rgb,DX);
    fragColor.rgb = mix(derivativeWithAngle,fragColor.rgb,DY);
    fragColor.rgb = pow3(fragColor.rgb,1./GAMMA);
}