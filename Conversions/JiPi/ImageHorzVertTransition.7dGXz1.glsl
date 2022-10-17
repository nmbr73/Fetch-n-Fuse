

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define PI 3.141592653589

void addLight(float t,out vec3 col, vec2 mouse, float offset1, vec2 uv, vec3 barCol, float yOffset){
    vec3 lightCol=mix(vec3(1.,1.0,1.0),barCol,0.3);
    float lightY=0.1+(yOffset - t*0.1);//1.-mod(t*0.1+yOffset, 1.2)-yOffset+0.1;
    lightY=1.-mod(1.-lightY,1.1);
    
    float lightDst = distance(vec2(mouse.x-offset1,lightY),uv);
    float r = abs(sin(t*2.+yOffset*t*2.))*0.03+0.06;
    float lightF=smoothstep(r,0.,lightDst);
    col=mix(col,lightCol,lightF*1.5);
}

//*****************************************************
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    float ratio = iResolution.x/iResolution.y;
    // Normalized pixel coordinates (from 0 to 1)
    vec2 uv = fragCoord/iResolution.xy;

    // Time varying pixel color
    vec2 mouse = iMouse.xy / iResolution.xy;
    vec3 col=vec3(0);
    float thk=0.01;
    vec3 col1=texture(iChannel0, uv).xyz;
    vec3 col2=texture(iChannel1, uv).xyz;
    vec3 col3=texture(iChannel2, uv).xyz;
    vec3 col4=texture(iChannel3, uv).xyz;
    
    //vec3 col1=vec3(0.);
    //vec3 col2=vec3(1.);
    vec3 barCol=vec3(0,1.0,1.0);
    float t = iTime;
    //t=0.;
    float offset1 = sin(uv.y*PI*4.+t*7.)*0.005;
    offset1 += -cos(uv.y*PI*8.+t*3.)*0.007;
    offset1 +=  cos(uv.y*PI*8.+t*3.)*0.007 * cos(4.+uv.y*PI*3.+t*6.);//
    
    
    float offset2 = sin(uv.x*PI*4.+t*7.)*0.005;
    offset2 += -cos(uv.x*PI*8.+t*3.)*0.007;
    offset2 += cos(uv.x*PI*8.+t*3.)*0.007 * cos(4.+uv.x*PI*3.+t*6.) * ratio;//
    
    
    //offset1+=-sin(PI+uv.y*PI*13.+t*4.)*0.002;//
    //offset1+=-cos(7.+uv.y*PI*13.+t*14.)*0.003*sin(3.+uv.y*PI*3.+t*4.);//
    //float offset2=0.;//abs(sin(2.+uv.y*PI*14.))*0.015;
    
    float edgeCloseFactorX = smoothstep(mouse.x-thk-offset1-0.03,mouse.x-offset1,uv.x);
    float edgeCloseFactorY = smoothstep(mouse.y-thk*ratio-offset2-0.03,mouse.y-offset2,uv.y);
    
    
    float barFactorX = edgeCloseFactorX* smoothstep(mouse.x+thk   -offset1, mouse.x-offset1, uv.x);
    float barFactorY = edgeCloseFactorY* smoothstep(mouse.y+thk*7.-offset2, mouse.y-offset2, uv.y);
    
    
     
    
    col = mix(col1,col2,step(mouse.x-offset1, uv.x));
    vec3 _col = mix(col3,col4,step(mouse.x-offset1, uv.x));
    
    
    col = mix(col,_col,step(mouse.y-offset2, uv.y)); 
    
    
    col=mix(col,barCol,barFactorX);
    col=mix(col,barCol,barFactorY);
    
    //addLight(t, col, mouse, offset1, uv, barCol,1.0);
    //addLight(t, col, mouse, offset1, uv, barCol,0.4);
    //addLight(t, col, mouse, offset1, uv, barCol,0.);
    
    

    // Output to screen
    fragColor = vec4(col,1.0);
}
