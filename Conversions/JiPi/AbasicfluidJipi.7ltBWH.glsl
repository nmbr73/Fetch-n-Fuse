

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// from aiekick： https://www.shadertoy.com/view/lttXDn
vec3 blackbody(float t)
{
	float Temp = t*7500.0;
    vec3 col = vec3(255.);
    col.x = 56100000. * pow(Temp,(-3. / 2.)) + 148.;
   	col.y = 100.04 * log(Temp) - 623.6;
   	if (Temp > 6500.) col.y = 35200000. * pow(Temp,(-3. / 2.)) + 184.;
   	col.z = 194.18 * log(Temp) - 1448.6;
   	col = clamp(col, 0., 255.)/255.;
    if (Temp < 1000.) col *= Temp/1000.;
   	return col;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pos = fragCoord/iResolution.xy;
    vec3 col=texture(iChannel3,pos).xyz;
    col=blackbody(smoothstep(-3.,250.,length(col)));
    float d=distToOcc(pos,iResolution.xy, iTime);
    col.xyz+=vec3(.6)*(1.-smoothstep(0.,.1, d));
    fragColor.xyz=col.xyz;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Advection and body force

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize=1./iResolution.xy;
    vec2 pos=fragCoord.xy*pixelSize;
    
    // semi-lagrangian
    vec2 U=texture(iChannel3,pos).xy;    
    vec2 prevPos=pos-iTimeDelta*U*pixelSize;
    
    // Interpolation is done by sampling
    vec2 Unext = texture(iChannel3,prevPos).xy;
    
    // Body forces
   
    if(pos.x>=0.43 && pos.x<=0.5 && pos.y<=1. && pos.y>=0.8)
    {
        Unext+=iTimeDelta*vec2(0.,-1000.);
    }
    
    if(pos.x>=0. && pos.x<=0.1 && pos.y<=.5 && pos.y>=.45)
    {
        Unext+=iTimeDelta*vec2(1000.,0.);
    }
    
    if(pos.x>=0. && pos.x<=0.1 && pos.y<=.5 && pos.y>=.45)
    {
        Unext+=iTimeDelta*vec2(1000.,0.);
    }
    
     if(pos.x>=.9 && pos.x<=1. && pos.y<=.5 && pos.y>=.45)
    {
        Unext+=iTimeDelta*vec2(-1000.,0.);
    }
    
    if(iMouse.z>0.||iMouse.w>0.)
    {
        float intensity=smoothstep(0.,0.05,length((fragCoord.xy-iMouse.xy)/iResolution.xy));
        Unext*=intensity;
    }
        
    // Vorticity confinement force
    float W=texture(iChannel1, pos).y;
    float Wleft=texture(iChannel1, pos-vec2(pixelSize.x,0)).y;
    float Wright=texture(iChannel1, pos+vec2(pixelSize.x,0)).y;
    float Wdown=texture(iChannel1, pos-vec2(0,pixelSize.y)).y;
    float Wup=texture(iChannel1, pos+vec2(0,pixelSize.y)).y;
    
    vec3 GradW=vec3(Wright-Wleft, Wup-Wdown, 0.)*.5;
    vec2 Fcon=cross(GradW/(length(GradW)+1e-10), vec3(0,0,W)).xy;
    
    Unext+=50.*iTimeDelta*Fcon;
    
        
    // Boundary conditions
    if(pos.x > 1.0 - pixelSize.x ||
      	pos.y > 1.0 - pixelSize.y ||
      	pos.x < pixelSize.x ||
      	pos.y < pixelSize.y)
    {
        Unext = vec2(0.0, 0.0);
    }
    
    // occluder
    if(distToOcc(pos, iResolution.xy, iTime)<=0.)
   	{
        Unext=vec2(0);
    }
   
    // output Fcon is just for visualization
    fragColor=vec4(Unext,Fcon);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Divergence/Vortocity

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize=1./iResolution.xy;
    vec2 pos=fragCoord.xy*pixelSize;
    
    vec2 left=texture(iChannel0, pos-vec2(pixelSize.x,0)).xy;
    vec2 right=texture(iChannel0, pos+vec2(pixelSize.x,0)).xy;
    vec2 down=texture(iChannel0, pos-vec2(0,pixelSize.y)).xy;
    vec2 up=texture(iChannel0, pos+vec2(0,pixelSize.y)).xy;
    
    // Central difference
    float D=((right.x-left.x) + (up.y-down.y))*0.5;
    
    // Vortisity
    float W=((right.y-left.y) - (up.x-down.x))*0.5;
    
    fragColor = vec4(D,W,0,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Jacobian

vec2 pixelSize;
float P;

float getPressure(vec2 pos, bool hori)
{
    if(distToOcc(pos, iResolution.xy, iTime)<=0.)
    {
        if(hori)
        {
            return P+rho*dx/iTimeDelta*texture(iChannel0, pos).x;
        }
        else
        {
            return P+rho*dx/iTimeDelta*texture(iChannel0,pos).y;
        }
        
    }
    if(pos.x<pixelSize.x || pos.x>1.-pixelSize.x)
    {
        return P+rho*dx/iTimeDelta*texture(iChannel0, pos).x;
    }
    else if(pos.y<pixelSize.y || pos.y>1.-pixelSize.y)
    {
        return P+rho*dx/iTimeDelta*texture(iChannel0,pos).y;
    }
    else
    {
        return texture(iChannel2, pos).x;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    pixelSize=1./iResolution.xy;
    vec2 pos=fragCoord.xy*pixelSize;
    
    float D=texture(iChannel1,pos).x;
    
    // Using previous pressure as guess
    float Pleft, Pright, Pup, Pdown;
    
    P=texture(iChannel2,pos).x;
    
    float Usolid=0.;
              
    Pleft=getPressure(pos-vec2(pixelSize.x,0),true);
    Pright=getPressure(pos+vec2(pixelSize.x,0),true);
    Pup=getPressure(pos+vec2(0,pixelSize.y),false);
    Pdown=getPressure(pos-vec2(0,pixelSize.y),false);
    
    P=(Pleft+Pright+Pup+Pdown-D*rho*dx/iTimeDelta)/4.;

    fragColor = vec4(P,0,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// Created by evilryu
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// Velocity field substract pressure gradient force


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 pixelSize=1./iResolution.xy;
    vec2 pos=fragCoord.xy*pixelSize;
	
    vec2 U=texture(iChannel0,pos).xy;
    
    float Pleft=texture(iChannel2, pos-vec2(pixelSize.x,0)).x;
    float Pright=texture(iChannel2, pos+vec2(pixelSize.x,0)).x;
    float Pup=texture(iChannel2, pos+vec2(0,pixelSize.y)).x;
    float Pdown=texture(iChannel2, pos-vec2(0,pixelSize.y)).x;
    

    vec2 dP=vec2((Pright-Pleft)/dx, (Pup-Pdown)/dx)*.5;
    U-=dP*iTimeDelta/rho;
    fragColor = vec4(U,0,0);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

const float rho=1.;
const float dx=1.;

float hash11(float p)
{
    vec2 p2 = fract(vec2(p * 5.3983, p * 5.4427));
    p2 += dot(p2.yx, p2.xy + vec2(21.5351, 14.3137));
    return fract(p2.x * p2.y * 95.4337);
}

float noise(float x)
{
    float p=floor(x);
    float f=fract(x);
    f=f*f*(3.-2.*f);
    return mix(hash11(p),hash11(p+1.0),f);
}

mat3 m_rot(float angle)
{
    float c = cos(angle);
    float s = sin(angle);
    return mat3( c, s, 0, -s, c, 0, 0, 0, 1);
}
mat3 m_trans(float x, float y)
{
    return mat3(1., 0., 0., 0., 1., 0, -x, -y, 1.);
}
mat3 m_scale(float s)
{
    return mat3(s, 0, 0, 0, s, 0, 0, 0, 1);
}

float distToOcc(vec2 pos, vec2 resolution, float time)
{
    float ratio=resolution.x/resolution.y;
    pos.x*=ratio;
	pos=pos*2.-1.;
    pos*=0.6;
    pos.x-=.4;
   
    pos*=6.0;
    pos.y+=3.;
   	vec3 p = vec3(pos, 1.);
    float d = 1.0;
    for(int i = 0; i < 7; ++i)
    {
        d=min(d,(length(max(abs(p.xy)-vec2(0.01,1.0), 0.0)))/p.z);
        p.x=abs(p.x);
        float pi=3.1415936;
        p=m_scale(1.22) * m_rot(0.25*pi) * m_trans(0.,3.*noise(float(i))) * p;

    }
    
    d=smoothstep(0.1, 0.15,d);
    return d;
}
