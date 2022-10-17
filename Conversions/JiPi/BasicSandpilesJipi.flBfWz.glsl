

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
vec4 color(vec2 p){
    return vec4(.5)+vec4(.5)*cos(6.28*(texture(iChannel0,p/iResolution.xy)*.25+vec4(0,.1,.2,.3)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){	
    
    fragColor += color(fragCoord);
    fragColor += color(fragCoord+vec2( .5, 0));
    fragColor += color(fragCoord+vec2(-.5, 0));
    fragColor += color(fragCoord+vec2( 0, .5));
    fragColor += color(fragCoord+vec2( 0,-.5));
    
    fragColor *= .2;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
    float s = texture(iChannel0,fragCoord/iResolution.xy).r; 
    
    if(s>3.) s-=4.;
    
	float d = texture(iChannel0,(fragCoord+vec2( 0.0,-1.0))/iResolution.xy).r; 
    if(d>3.) s++;
  	float l = texture(iChannel0,(fragCoord+vec2(-1.0, 0.0))/iResolution.xy).r; 
    if(l>3.) s++;
  	float r = texture(iChannel0,(fragCoord+vec2( 1.0, 0.0))/iResolution.xy).r; 
    if(r>3.) s++;
    float u = texture(iChannel0,(fragCoord+vec2( 0.0, 1.0))/iResolution.xy).r; 
	if(u>3.) s++;
    
    if(iMouse.z>0. && distance(fragCoord,iMouse.xy)<1.) s = 1000.;
    
    if(iFrame==0 && distance(fragCoord,.5+iResolution.xy/2.)<.2) s = 80000.;
    
    fragColor = vec4(s);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
    float s = texture(iChannel0,fragCoord/iResolution.xy).r; 
    
    if(s>3.) s-=4.;
    
	float d = texture(iChannel0,(fragCoord+vec2( 0.0,-1.0))/iResolution.xy).r; 
    if(d>3.) s++;
  	float l = texture(iChannel0,(fragCoord+vec2(-1.0, 0.0))/iResolution.xy).r; 
    if(l>3.) s++;
  	float r = texture(iChannel0,(fragCoord+vec2( 1.0, 0.0))/iResolution.xy).r; 
    if(r>3.) s++;
    float u = texture(iChannel0,(fragCoord+vec2( 0.0, 1.0))/iResolution.xy).r; 
	if(u>3.) s++;
    
    fragColor = vec4(s);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
    float s = texture(iChannel0,fragCoord/iResolution.xy).r; 
    
    if(s>3.) s-=4.;
    
	float d = texture(iChannel0,(fragCoord+vec2( 0.0,-1.0))/iResolution.xy).r; 
    if(d>3.) s++;
  	float l = texture(iChannel0,(fragCoord+vec2(-1.0, 0.0))/iResolution.xy).r; 
    if(l>3.) s++;
  	float r = texture(iChannel0,(fragCoord+vec2( 1.0, 0.0))/iResolution.xy).r; 
    if(r>3.) s++;
    float u = texture(iChannel0,(fragCoord+vec2( 0.0, 1.0))/iResolution.xy).r; 
	if(u>3.) s++;
    
    fragColor = vec4(s);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	
    float s = texture(iChannel0,fragCoord/iResolution.xy).r; 
    
    if(s>3.) s-=4.;
    
	float d = texture(iChannel0,(fragCoord+vec2( 0.0,-1.0))/iResolution.xy).r; 
    if(d>3.) s++;
  	float l = texture(iChannel0,(fragCoord+vec2(-1.0, 0.0))/iResolution.xy).r; 
    if(l>3.) s++;
  	float r = texture(iChannel0,(fragCoord+vec2( 1.0, 0.0))/iResolution.xy).r; 
    if(r>3.) s++;
    float u = texture(iChannel0,(fragCoord+vec2( 0.0, 1.0))/iResolution.xy).r; 
	if(u>3.) s++;

    fragColor = vec4(s);
}