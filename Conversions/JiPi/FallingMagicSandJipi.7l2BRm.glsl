

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;
    vec3 a=texture(iChannel0,uv).rgb;
    vec3 b=texture(iChannel1,uv*vec2(2.,1.5)+vec2(0,iTime*0.1)).rgb;
    if (length(a)>length(b)){a=normalize(a);}else{a=vec3(0.);}
    
	fragColor = vec4(a,1.);//vec4(uv,0.5+0.5*sin(iTime),1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 poss[]=vec2[3](vec2(-0.004,0.),vec2(0.004,0.),vec2(0.,0.01));
float pit=3.14159*2./3.;
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	vec2 uv = fragCoord.xy / iResolution.xy;
 	vec4 a=vec4(-0.001);
 for (int i=0;i<3;i++){
     a+=texture(iChannel0,uv+poss[i]);}
 	a=a/3.0;
    if(distance(uv,iMouse.xy/iResolution.xy)<0.05){
        a=vec4(sin(iTime),sin(iTime+pit),sin(iTime-pit),1.)*.5+.5;}
    //a=vec4(1.,0.,0.,1.);
 	a.a=1.;
    fragColor = clamp(a,0.,1.);// vec4(0.0,0.0,1.0,1.0);
}