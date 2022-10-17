

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//CLICK MOUSE AND MOVE
void mainImage(out vec4 c,in vec2 o){
  c=vec4(0);o-=iResolution.xy/2.0;
  float t=iTime,dt=256.0;vec2 x,y,z;
  vec2 v2=vec2((iMouse.x-iResolution.x/2.0)/iResolution.x,(iMouse.y-iResolution.y/2.0)/iResolution.y)*3.1415;
  for (float i=0.0;i<=16.0;i+=0.25){t=iTime/16.0+i*dt;
      x=vec2(cos(t*1.0)*100.0,sin(t*1.0)*100.0)*v2;
      y=vec2(cos(t*10.0)*60.0,sin(t*10.0)*60.0)*v2;
      z=vec2(cos(t*15.0)*80.0,sin(t*15.0)*80.0);
      c+=vec4(0.0,0.6,0.8,1.0)*vec4(length(o)/length(x+y+z-o)/16.0)/(i+0.5)/2.0;
  }
}