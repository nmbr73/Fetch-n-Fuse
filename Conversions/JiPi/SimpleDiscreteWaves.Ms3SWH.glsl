

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// **************************** INFO ***************************************
//
//
// Click (Hold) the left mouse button to create waves (perform some action)
// 
// Pressing 'c' on your keyboard will change your action. 
// The actions are as follows:
//
// 1. Waves
//		- Creates waves in free regions
// 2. Block
//		- Blocks a small area -> Waves will splash against it
// 3. Clear
//		- Clears both blocks and waves in a small area
// 
// The default action is 'Waves'
//
//
// Holding down 'a' on your keyboard increases the area affected by your mouse
// 
// Pressing 'r' on your keyboard resets everything
//
//
//
// *************************************************************************


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    
  
        vec2 uv = fragCoord.xy / iResolution.xy;
    	vec4 vals = texture(iChannel1,uv);
    
    	
        vec2 delta = vec2(1.,1.)/ iChannelResolution[1].xy;
        
        vec3 offset = vec3(delta.x,delta.y,0.);
        
    
   
    	vec4 fxp = texture(iChannel1,uv + offset.xz);
        vec4 fxm = texture(iChannel1,uv - offset.xz);
        
        vec4 fyp = texture(iChannel1,uv + offset.zy);
        vec4 fym = texture(iChannel1,uv - offset.zy);
        
    	// partial derivatives d/dx, d/dy
    	float dx = fxp.y - fxm.y;
    	float dy = fyp.y - fym.y;
		
    	// partials in 3d space
    	
    	vec3 fx = vec3(2.,0.,dx);
    	vec3 fy = vec3(0.,2.,dy);
    	
    
    	vec3 n = normalize(cross(fx,fy));
    
    	vec3 campos = vec3(0.5,0.5,200.);
    	vec3 p = vec3(uv,0.);
    
    	vec3 v = campos - p;
    
        vec3 l = normalize(vec3(10.,70.,400.));
    
    	vec3 h = normalize(l + v);
    
    	float specular = pow(max(0.,dot(h,n)),16.);
    
    	vec3 r = refract(-v,n,1./1.35);
    	// very simple hacky refraction
    	vec2 roffset = 10.* vals.y*normalize(r.xy - n.xy)/iChannelResolution[2].xy;
    
    	vec3 color = texture(iChannel2,uv + roffset).xyz;
    
    	float block = 1. - vals.w;
    	
    	color*= block;
   
    	float factor = clamp(max(dot(n,l),0.) + specular + 0.2,0.,1.);
    	fragColor = vec4(color*factor,1.);
 
	
    
}
// >>> ___ GLSL:[Buf A] ____________________________________________________________________ <<<
// **************************** INFO ***************************************
//
//
// Click (Hold) the left mouse button to create waves (perform some action)
// 
// Pressing 'c' on your keyboard will change your action. 
// The actions are as follows:
//
// 1. Waves
//		- Creates waves in free regions
// 2. Block
//		- Blocks a small area -> Waves will splash against it
// 3. Clear
//		- Clears both blocks and waves in a small area
// 
// The default action is 'Waves'
//
//
// Holding down 'a' on your keyboard increases the area affected by your mouse
// 
// Pressing 'r' on your keyboard resets everything
//
//
//
// *************************************************************************

const float c = 0.5;
const int STATE_WAVES = 0;
const int STATE_BLOCK = 10;
const int STATE_CLEAR = 20;

bool isPressed(int key)
{
    float val = texture( iChannel2, vec2( (float(key)+0.5)/256.0, 0.25 ) ).x;
	return val > 0.5;
}

bool isToggled(int key)
{
    float val = texture( iChannel2, vec2( (float(key)+0.5)/256.0, 0.75 ) ).x;
	return val > 0.5;
}


// Not the greatest way, I guess, but works... not sure yet how to use the keyboard
// more nicely
int getState(float state)
{
    bool change = isToggled(67);
    
    int iState = int(state);
    bool lastToggle = iState -100 >= 0;
    
    iState = iState - int(lastToggle)*100;
    
    
    change = change != lastToggle;
    if(!change)
    {
    	return iState;
    }
    else if(iState == STATE_WAVES)
    {
        return STATE_BLOCK;
    }
    else if(iState == STATE_BLOCK)
    {
     	return STATE_CLEAR;   
    }
    else
    {
     	return STATE_WAVES;   
    }
}

float setState(int state)
{
 	return float(state) + 100.*float(isToggled(67));    
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
   
    vec2 delta = vec2(1.,1.)/ iResolution.xy;
        
    vec3 offset = vec3(delta.x,delta.y,0.);
    
    vec2 uv = fragCoord.xy * delta;
    
    vec4 f = texture(iChannel0,uv);
    
    float state = f.z;
    
    int iState = getState(state);
    
    float mouseRadius = 10. + float(isPressed(65))*10.;
    // Initial conditions
    if(iTime < 0.5)
    {
        
      // Some seed drops
      float rand = texture(iChannel1,uv/4.).x;
   
      if(rand > 0.9 ||rand < 0.05 )
         fragColor = vec4(0.0,5.,setState(STATE_WAVES),0.0);  
      else
          fragColor = vec4(0.0,0.,setState(STATE_WAVES),0.0);
    
     	   
    }
    else if(isPressed(82))
    {
        fragColor = vec4(0.0,0.,setState(STATE_WAVES),0.0);
    }
    else if(fragCoord.x == 0. || fragCoord.y == 0. || 
            fragCoord.x == iResolution.x-1. || fragCoord.y == iResolution.y -1.)
    {
        // Boundary conditions
        fragColor = vec4(0.,0.,setState(iState),1.0);  
    }
     else if(iState == STATE_CLEAR && iMouse.w > 0. && length(fragCoord.xy - iMouse.xy) < mouseRadius)
     {	
           fragColor = vec4(0.,0.,setState(iState),0.0);  
     }
    else if(f.w > 0.0)
    {
        fragColor = vec4(0.,0.,setState(iState),1.0);  
    }
    else if(iState == STATE_BLOCK && iMouse.w > 0. && length(fragCoord.xy - iMouse.xy) < mouseRadius)
     {	
           fragColor = vec4(0.,0.,setState(iState),1.0);  
     }
      else if(iState == STATE_WAVES && iMouse.w > 0. && length(fragCoord.xy - iMouse.xy) < mouseRadius)
     {	
         	float dist = length(fragCoord.xy - iMouse.xy);
           fragColor = vec4(f.y,40.*exp(-0.001*dist*dist),setState(iState),0.0);  
     }
    else
    {
        
        
        vec2 mouse = iMouse.xy *delta;

       
        // Sample stuff for derivatives
        vec4 fxp = texture(iChannel0,uv + offset.xz);
        vec4 fxm = texture(iChannel0,uv - offset.xz);
        
        vec4 fyp = texture(iChannel0,uv + offset.zy);
        vec4 fym = texture(iChannel0,uv - offset.zy);
        
        // Discrete wave pde
        // Taken from http://www.mtnmath.com/whatrh/node66.html
        float ft = c*c*(fxp.y + fxm.y + fyp.y + fym.y - 4.0*f.y) - f.x + 2.0*f.y;
        
        // x = value at t-1, y = value at t, z = state, w = blocked
        // Little bit of damping so everything settles down
		fragColor = vec4(vec2(f.y,ft)*0.995,setState(iState),0.0);
    }
        
        
	
    
}
// >>> ___ GLSL:[Buf B] ____________________________________________________________________ <<<
// **************************** INFO ***************************************
//
//
// Click (Hold) the left mouse button to create waves (perform some action)
// 
// Pressing 'c' on your keyboard will change your action. 
// The actions are as follows:
//
// 1. Waves
//		- Creates waves in free regions
// 2. Block
//		- Blocks a small area -> Waves will splash against it
// 3. Clear
//		- Clears both blocks and waves in a small area
// 
// The default action is 'Waves'
//
//
// Holding down 'a' on your keyboard increases the area affected by your mouse
// 
// Pressing 'r' on your keyboard resets everything
//
//
//
// *************************************************************************


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Binomial filter
    
    vec2 delta = vec2(1.,1.)/ iChannelResolution[0].xy;
        
    vec3 offset = vec3(delta.x,delta.y,0.);
    
    vec2 uv = fragCoord.xy / iResolution.xy;
    
    vec4 col = vec4(0.);
    
    
    col += texture(iChannel0,uv + vec2(-delta.x,delta.y));
    col += 2.*texture(iChannel0,uv + vec2(0.,delta.y));
    col += texture(iChannel0,uv + vec2(delta.x,delta.y));
    
    col += 2.*texture(iChannel0,uv + vec2(-delta.x,0.));
    col += 4.*texture(iChannel0,uv);
    col += 2.*texture(iChannel0,uv + vec2(delta.x,0.));
    
    col += texture(iChannel0,uv + vec2(-delta.x,-delta.y));
    col += 2.*texture(iChannel0,uv + vec2(0.,-delta.y));
    col += texture(iChannel0,uv + vec2(delta.x,-delta.y));
    
    fragColor = col/16.;
}