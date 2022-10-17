

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;

    fragColor = texture(iChannel0, uv);
        
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define hue(h) clamp( abs( fract(h + vec4(2,1,4,0)/1.) * 6. - 3.) -1. , 0., 1.)

vec2 rand( vec2 p ) {
    return fract(sin(vec2(dot(p,vec2(127.1,311.7)),dot(p,vec2(269.5,183.3))))*43758.5453);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{  
    //stuff to expose
    const float size = 1.5;
    const float uvFac = 10.;
    const float colFac = .5;
    
    
    vec2 ouv = fragCoord/iResolution.xy;        
    vec2 uv = (fragCoord - iResolution.xy*.5) / (iResolution.y*size);    
    vec2 luv = uv;
    
    vec4 texIn = texture(iChannel1, ouv);
    vec2 mp = texIn.rb;
    
    uv *= 100. + sin(iTime*.5+mp.x*uvFac);
   
    vec2 iuv = floor(uv);
    vec2 guv = fract(uv);      

    float mDist = 10.;
   
    vec3 col = vec3(.1);
       
    for (float y= -1.; y <= 1.; y++) {
        for (float x= -1.; x <= 1.; x++) {            
            vec2 neighbor = vec2(x, y);            
            vec2 point = rand(iuv + neighbor);
            point = .5 + .5*sin(iTime*2. + 6.2831*point);
            vec2 diff = neighbor + point - guv;            
            float dist = length(diff);                      
           
            mDist = min(mDist, dist);                        
        }
    } 
       
    float l = length(luv);    
    col = hue(fract(mDist*.95 + iTime*.1 + l + mp.x*colFac)).rgb;
    fragColor = vec4(col,1.0)*.05 + texture(iChannel0, ouv) *.95;    
}