

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define PI 3.1415926
#define SEMI_PI 1.5707963
//from iq
vec3 palete(float h,float s,float v){
   vec3 a = vec3(v,v,v);
   vec3 b = vec3(s,s,s);
   vec3 c = vec3(1,1,1);
   
   vec3 d = vec3(0.0,0.33,0.67);
   return a+b*cos(2.0*PI*(c*h+d)); 
} 

mat2 rot2D(float a){
   float c = cos(a);
   float s = sin(a);
   return mat2(c,s,-s,c); 
}

//from iq https://www.iquilezles.org/www/articles/fbm/fbm.htm
float fbm(vec2 uv) {
    float total = 0.0, amp = 1.0,freq = 1.0;
    float G = exp(-0.7);
    for (int i = 0; i <6; i++) {
        //total += noise(uv*freq) * amp;
        total += texture(iChannel0,uv*freq).x * amp;
        freq = freq*2.0;
        amp *= G;
    }
    return total;
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv   = fragCoord.xy / iResolution.xy-vec2(0.5);
    vec2 cuv  = vec2(uv.x*(iResolution.x/iResolution.y),uv.y);
    float id  = floor(cuv.x/0.4);
    cuv.x     = mod(cuv.x,0.4)-0.2;
    vec3 col  = vec3(0); 
    vec2 dir  = vec2(0,1)*rot2D(0.); cuv.y+=0.2;
    
    float f   = fbm((cuv-dir*(iTime+id)*0.8)*0.031);
    vec2  suv = normalize(vec2(cuv.x,cuv.y*0.23));
    float dp  = clamp(dot(dir,suv),0.0,1.0);
      
    float v   = pow((0.2*dp)/length(suv)/length(cuv),5.0)*dp*pow(f,f*9.0);
    col = vec3(1.1,1.1,0.8)*cuv.x; 
    col += mix(v,pow(0.025*f/length(vec2(cuv.x,cuv.y-0.02)),8.0),0.5);
    col*= palete(0.94+id*0.28,1.0,1.0);
     
    col = smoothstep(0.0,1.0,col);
    //col = pow(col,vec3(1.6));
	fragColor = vec4(col,1.);
}
  
