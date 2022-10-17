

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
float greyScale(vec3 color){
    return (color.r + color.g + color.b) / 3.0;


}
vec3 post(vec3 color){
    return floor(color*15.+0.5)/15.;

}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    float[9] Xsobel_kernel =
    float[](-1.0, 0.0, 1.0,
            -2.0, 0.0, 2.0,
            -1.0, 0.0, 1.0);
            
    float[9] Ysobel_kernel =
    float[](-1.0, -2.0, -1.0,
            0.0, 0.0, 0.0,
            1.0, 2.0, 1.0);
            
    vec2 uv = fragCoord/iResolution.xy;
    vec3 og = texture(iChannel2, uv).rgb;
    og = pow(og,vec3(1./2.2));
    og = post(og);
    vec3 pat = mix(vec3(0.4,0.2,0.4), vec3(0.5,0.7,0.8), uv.x+uv.y);
    og = mix(pat, og, greyScale(og)*2.2);
    
    float threshold = .14;
    float Xedge = 0.;
    float Yedge = 0.;
    float kernelPointer = 0.;
    for(float x = 0.0; x < 3.0; x++){
        for(float y = 0.0; y < 3.0; y++){
            float result = greyScale(texture(iChannel0, uv+vec2(x-1.,y-1.)/iResolution.xy).rgb);
            Xedge += result * Xsobel_kernel[int(kernelPointer)];
            kernelPointer++;
        }
    }
    kernelPointer = 0.;
    for(float x = 0.0; x < 3.0; x++){
        for(float y = 0.0; y < 3.0; y++){
            float result = greyScale(texture(iChannel0, uv+vec2(x-1.,y-1.)/iResolution.xy).rgb);
            Yedge += result  * Ysobel_kernel[int(kernelPointer)];
            kernelPointer++;
        }
    }
   
   //remapping values
  //Xedge += 0.5;
  //Yedge += 0.5;
   float finalG = sqrt(Xedge*Xedge + Yedge*Yedge);
   if(finalG < threshold){
       finalG = 0.; 
   }
    
  

   float edgeOrientation = atan(Yedge/Xedge);
   float g = edgeOrientation;
   vec3 col = vec3(finalG);
    
    fragColor = vec4(og-col,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 uv = fragCoord.xy/iResolution.xy;
    float kernelSize = 6.;
    vec3 col = vec3(0.);
    for(float i = 0.; i < kernelSize; i++){
        float pointer = i - floor(kernelSize * 0.5);
        col += texture(iChannel0, uv + vec2(i,0.0)/iResolution.xy ).rgb;
    }
    col/= kernelSize;
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  
    vec2 uv = fragCoord.xy/iResolution.xy;
    float kernelSize = 6.;
    vec3 col = vec3(0.);
    for(float i = 0.; i < kernelSize; i++){
        float pointer = i - floor(kernelSize * 0.5);
        col += texture(iChannel0, uv + vec2(0.0,i)/iResolution.xy ).rgb;
    }
    col/= kernelSize;
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    vec2 uv = fragCoord.xy/iResolution.xy;
    float kernelSize = 20.;
    vec3 col = vec3(0.);
    for(float i = 0.; i < kernelSize; i++){
        float pointer = i - floor(kernelSize * 0.5);
        col += textureLod(iChannel0, uv + vec2(i,0.0)/iResolution.xy,3. ).rgb;
    }
    col/= kernelSize;
    
    fragColor = vec4(col,1.0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
  
    vec2 uv = fragCoord.xy/iResolution.xy;
    float kernelSize = 20.;
    vec3 col = vec3(0.);
    for(float i = 0.; i < kernelSize; i++){
        float pointer = i - floor(kernelSize * 0.5);
        col += textureLod(iChannel0, uv + vec2(0.0,i)/iResolution.xy, 3. ).rgb;
    }
    col/= kernelSize;
    
    fragColor = vec4(col,1.0);
}