

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;
    vec4 raw = texture(iChannel0, uv);
    // Time varying pixel color
    vec4 col = raw*1.0+ 0.0;
   
    fragColor = vec4(vec3(col),1.);

    
    
    
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Thanks iq for line segment sdf.
float line_segment(in vec2 p, in vec2 a, in vec2 b) {
	vec2 ba = b - a;
	vec2 pa = p - a;
	float h = clamp(dot(pa, ba) / dot(ba, ba), 0., 1.);
	return length(pa - h * ba);
}


vec4 tf(ivec2 p, int i, int j){
    return texelFetch(iChannel0,p+ivec2(i,j),0);
}
vec4 get(int i){
    return texelFetch(iChannel1,ivec2(i,0),0);
}

vec4 state(ivec2 p){
    vec4 colNow = vec4(0,0,0,0);
    vec4 r = tf(p,0,0) * 0.7;
    for(int i = -1; i < 2; i++){
        for(int j = -1; j < 2; j++){
            vec4 u = tf(p,i,j);
            r+= u*0.03;
        }
    }
    return r;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4(0.0,0.0,1.0,1.0);
    vec4 col = state(ivec2(fragCoord));

    if(iFrame < 1){
        vec2 uv = fragCoord/iResolution.xy;
        col = texture(iChannel1, uv*0.5)*0.5-0.25;
    }

    for(int i = 0; i < AMT * AMT; i ++){
        if(i%AMT <= i/AMT){
            vec4 col2 = get(i);
            if(show == 0 || i >= AMT * (AMT-1)){
                if(length(vec2(col2.x,col2.y + iResolution.y*0.4) - fragCoord) < 2.){
                    col += 0.1*vec4(1.*sin(iTime*5. + float(i))+1.0,1.*sin(iTime*5. + 2.1 + float(i))+1.0,1.*sin(iTime*5. + 4.2 + float(i))+1.0,0);
                    if(i%AMT == i/AMT){
                        col += trails*vec4(1.*sin(iTime*5. + float(i))+1.0,1.*sin(iTime*5. + 2.1 + float(i))+1.0,1.*sin(iTime*5. + 4.2 + float(i))+1.0,0);
                    }
                }
                
                vec4 col3 = get(i-1);
                if(i%AMT == 0){
                    col3.x = iResolution.x*0.5;
                    col3.y = iResolution.y*0.5;
                }
                if(line_segment(fragCoord-vec2(0,iResolution.y*0.4),col2.xy,col3.xy) <0.5){
                    col += 0.04*vec4(1.*sin(iTime*5. + float(i))+1.0,1.*sin(iTime*5. + 2.1 + float(i))+1.0,1.*sin(iTime*5. + 4.2 + float(i))+1.0,0);
                }
            }
        }
    }
    
    fragColor = col;

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
vec4 get(int i){
    return texelFetch(iChannel0,ivec2(i,0),0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    int t = int(fragCoord.x);
    fragColor = vec4(0.0,0.0,1.0,1.0);
    if(t < AMT * AMT && t%AMT <= t/AMT){
        
        // Physics
        vec4 r = get(int(fragCoord.x));

        float wlen = iResolution.y*0.8/(1. + float(t/AMT));
        // Center Length
        vec2 center = iResolution.xy*0.5;
        if(t%AMT >=1){
            center = get(t-1).xy;
            wlen += 0.01 * float(t%AMT);
        }
        vec2 delt = r.xy - center;
        float len = wlen - length(delt);
        
        r.z += len * normalize(delt).x * 0.9;
        r.w += len * normalize(delt).y * 0.9; 
        
        r.x = (center.x + wlen * normalize(delt).x) * 0.1 + r.x * 0.9; // force position towards 
        r.y = (center.y + wlen * normalize(delt).y) * 0.1 + r.y * 0.9;
        
        if(t%AMT <= t/AMT - 1){
            center = get(t+1).xy;
        
            delt = r.xy - center;
            len = wlen - length(delt);
        
            r.z += len * normalize(delt).x * 0.9;
            r.w += len * normalize(delt).y * 0.9; 
        
        }
        r.w+= gravity;
        
        if(iMouse.z > 0.0 && t%AMT == t/AMT){// mouse drag
            center = iMouse.xy - vec2(0,iResolution.y*0.4);
            r.x = (center.x ) * 0.03 + r.x * 0.97;  
            r.y = (center.y ) * 0.03 + r.y * 0.97;
            r.z = 0.;
            r.w = 0.; 
        }
        
        r.x += r.z;
        r.y += r.w;
        
        //r.z *= 0.9999;
        //r.w *= 0.9999;
        
        
        
        
        
        fragColor = r;
        
        if(iFrame < 1){            
            fragColor = vec4(iResolution.x * 0.5 + wlen*float(t%AMT+1),iResolution.y*0.5,0.,0.);
            
        }
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
int AMT = 8; // Number of pendulums. Each one being longer than the previous. Currently uses x of a buffer to store each item. 
int show = 0; // if it is 1, then only show the pendulum with the most links
float gravity = -0.02;
float trails = 1.5;