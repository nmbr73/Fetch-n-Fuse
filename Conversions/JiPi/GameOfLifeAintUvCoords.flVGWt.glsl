

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
    
      vec3 gol1 = texture(iChannel0,uv).xyz;
      vec3 gol2 = texture(iChannel0,uv + 0.254).xyz;
      vec4 gol = vec4(gol1 + gol2,1.)*.5;
      //fragColor = gol;
      fragColor = texture(iChannel1,uv*0.1 + 0.25* gol.z *gol.xy);
  }
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
vec2 uv = 2. * fragCoord/iResolution.xx;
    if(iFrame < 10 || texelFetch(iChannel2, ivec2(32,0),0).x > 0.){
    fragColor = texture(iChannel0,uv);
    }
    else
    {
     fragColor = texture(iChannel1,fragCoord/iResolution.xy);
     
     vec3 diff = vec3(1,0,-1)/iResolution.x;
     
     vec4 sum = texture(iChannel1,fragCoord/iResolution.xy - diff.xx);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.xy);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.xz);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.yx);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.yz);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.zx);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.zy);
     sum += texture(iChannel1,fragCoord/iResolution.xy - diff.zz);
     
     vec4 result = texture(iChannel1,fragCoord/iResolution.xy);
     float amount = 0.01;
     float minl = 2.;
     float maxl = 3.;
     
     if(sum.z < minl || sum.y > maxl)
         fragColor.x =  max(0.,result.x -amount);
     else 
         fragColor.x = min(1.,result.x + amount);
         
      if(sum.x <minl || sum.z >maxl)
         fragColor.y =  max(0.,result.y -amount);
     else 
         fragColor.y = min(1.,result.y + amount);
         
     if(sum.y < minl || sum.x > maxl)
         fragColor.z =  max(0.,result.z -amount);
     else 
         fragColor.z = min(1.,result.z + amount);

     
    }
    
}