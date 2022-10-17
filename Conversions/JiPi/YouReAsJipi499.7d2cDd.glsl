

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Variation of a precedent project
// https://www.shadertoy.com/view/Xt2cRV

// added substance advection/diffusion along the flow (like die coloring in fluid experiment)
// you can modify the parameter (but do it in all buffer)

// Made it look like ice accidently so decided to keep it

// must be modified in all buffers and image
// simulation parameters
const float dt = 1.0/400.0;
const float reynold = 200.0;
const float vorticesStrength = 5.0; //strength of vortices
const float sourceStrength = 40.0; //stength of sources
const float kappa = 0.0; //substance diffusion constant
const float alpha = 12.0; //substance dissipation rate
const float radius = 0.07; //radius of sources



//macro
#define GetVorticity(I,J) texelFetch( iChannel0, ijCoord+ivec2(I,J), 0 ).x
#define GetStream(I,J) texelFetch( iChannel2, ijCoord+ivec2(I,J), 0 ).x
#define GetVelocity(I,J) texelFetch( iChannel3, ijCoord+ivec2(I,J), 0 ).xy
#define GetDensity(I,J) texelFetch( iChannel1, ijCoord+ivec2(I,J), 0 ).y

// COLORMAP

vec3 hot(float t)
{
    return vec3(smoothstep(0.00,0.33,t),
                smoothstep(0.33,0.66,t),
                smoothstep(0.66,1.00,t));
}
// for testing purpose, https://www.shadertoy.com/view/4dlczB
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
    //set grid
    float dx = 1.0 / iResolution.y;
    float dxPow = dx *dx ;
    vec2 uvCoord = dx*fragCoord.xy;
    ivec2 ijCoord = ivec2(floor(fragCoord.xy));

    vec3 col = blackbody(GetDensity(0,0));
    
    fragColor = vec4(col,1);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Set initial condition / perturbation / advection / dissipation

//Select scheme to do backward advection
//#define EULER
#define RUNGE

// comment all to remove driving force.
//#define VORTICES
//#define LATTICE
#define NOISE

// must be modified in all buffers and image
// simulation parameters
const float dt = 1.0/400.0;
const float reynold = 200.0;
const float vorticesStrength = 5.0; //strength of vortices
const float sourceStrength = 40.0; //stength of sources
const float kappa = 0.0; //substance diffusion constant
const float alpha = 12.0; //substance dissipation rate
const float radius = 0.07; //radius of sources


//macro
#define GetVelocity(I,J) texelFetch( iChannel3, ijCoord+ivec2(I,J), 0 ).xy
#define GetDensity(I,J) texelFetch( iChannel1, ijCoord+ivec2(I,J), 0 ).y

#define GetVelocityUV(XY) texture( iChannel3, vec2(XY)).xy
#define GetDensityUV(XY) texture( iChannel1, vec2(XY)).y

vec2 Euler(vec2 posUV){
    vec2 AspectRatio = iResolution.xy/iResolution.y;
    return dt*GetVelocityUV(posUV)/AspectRatio;
}

vec2 Runge(vec2 posUV){
    vec2 AspectRatio = iResolution.xy/iResolution.y;
    vec2 k1 = GetVelocityUV(posUV)/AspectRatio;
    vec2 k2 = GetVelocityUV(posUV-0.5*k1*dt)/AspectRatio;
    vec2 k3 = GetVelocityUV(posUV-0.5*k2*dt)/AspectRatio;
    vec2 k4 = GetVelocityUV(posUV-k3*dt)/AspectRatio;
    return dt/6.*(k1+2.0*k2+2.0*k3+k4);
}

//main
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    // set grid
    float dx = 1.0 / iResolution.y;
    float dxPow = dx *dx ;
    vec2 uvCoord = dx*fragCoord.xy;
    ivec2 ijCoord = ivec2(floor(fragCoord.xy));
    
    // advect via semi-lagrangian method
    float vorticity = vorticesStrength*(texture(iChannel0, mod(fragCoord.xy/ iResolution.y-vec2(0,0.04*iTime), iResolution.xy/ iResolution.y)).x-0.5);
    
   
    vec2 posUV =fragCoord/iResolution.xy;
    #ifdef EULER
    vec2 posAdvUV = posUV-Euler(posUV);
    #endif
    #ifdef RUNGE
    vec2 posAdvUV = posUV-Runge(posUV);
    #endif
    float densityAdv = GetDensityUV(posAdvUV)/(1.0+dt*alpha);

    
    // add ice with mouse
    float pert = length((fragCoord.xy - iMouse.xy) / iResolution.y); 
    if(iMouse.z > 0.0 && pert < radius) {
        densityAdv += dt*sourceStrength;
    }
    if (length(uvCoord.xy-dx*floor(0.5*iResolution.xy) +vec2(0.5*sin(0.75*iTime),0.3*cos(1.0*iTime)))<=radius)
    {
        densityAdv += dt*sourceStrength;
    }
    
    fragColor = vec4(vorticity, densityAdv, 0, 0); 
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// solve for diffusion

// must be modified in all buffers and image
// simulation parameters
const float dt = 1.0/400.0;
const float reynold = 200.0;
const float vorticesStrength = 5.0; //strength of vortices
const float sourceStrength = 40.0; //stength of sources
const float kappa = 0.00005; //substance diffusion constant
const float alpha = 12.0; //substance dissipation rate
const float radius = 0.07; //radius of sources


//macro
#define GetVorticity(I,J) texelFetch( iChannel0, ijCoord+ivec2(I,J), 0 ).x
#define GetDensity(I,J) texelFetch( iChannel0, ijCoord+ivec2(I,J), 0 ).y

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    
    // set grid
    float dx = 1.0 / iResolution.y;
    float dxPow = dx *dx ;
    vec2 uvCoord = dx*fragCoord.xy;
    ivec2 ijCoord = ivec2(floor(fragCoord.xy));

    
    //to compute finite difference approximaton
    float vortij = GetVorticity(0,0); 
    
    //to compute density finite difference approximaton
    float densij = GetDensity(0,0); 
    float densip1j = GetDensity(1,0); 
    float densim1j = GetDensity(-1,0); 
    float densijp1 = GetDensity(0,1); 
    float densijm1 = GetDensity(0,-1); 
    
    
    //should use more than 1 iteration...
    //solve with jacobi for new velocity with laplacian
    float coef = kappa*dt/(dxPow);
    densij = (densij+coef*(densip1j+densim1j+densijp1+densijm1))/(1.0+4.0*coef);
    
    fragColor = vec4(vortij,densij,0,0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// solve for stream

// must be modified in all buffers and image
// simulation parameters
const float dt = 1.0/400.0;
const float reynold = 200.0;
const float vorticesStrength = 5.0; //strength of vortices
const float sourceStrength = 40.0; //stength of sources
const float kappa = 0.0; //substance diffusion constant
const float alpha = 12.0; //substance dissipation rate
const float radius = 0.07; //radius of sources


//macro
#define GetVorticity(I,J) texelFetch( iChannel0, ijCoord+ivec2(I,J), 0 ).x
#define GetStream(I,J) texelFetch( iChannel2, ijCoord+ivec2(I,J), 0 ).x

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    //set grid
    float dx = 1.0 / iResolution.y;
    float dxPow = dx *dx ;
    vec2 uvCoord = dx*fragCoord.xy;
    ivec2 ijCoord = ivec2(floor(fragCoord.xy));

        //to compute finite difference approximaton
        float vortij = GetVorticity(0,0); 

        //to compute finite difference approximaton
        float streamij = GetStream(0,0); 
        float streamip1j = GetStream(1,0); 
        float streamim1j = GetStream(-1,0); 
        float streamijp1 = GetStream(0,1);
        float streamijm1 = GetStream(0,-1);

        //Set boundary condition (image method) 
        // Left

        if (ijCoord.x == 0) 
        {
            streamim1j = GetStream(-ijCoord.x+int(iResolution.x)-1,0);
        }
        // Right
        if (ijCoord.x == int(iResolution.x)-1) 
        {
            streamip1j = GetStream(-ijCoord.x,0);
        }

        // Down
        if (ijCoord.y == 0) 
        {
            streamijm1 = GetStream(0,-ijCoord.y+int(iResolution.y)-1);
        }

        // Up
        if (ijCoord.y == int(iResolution.y)-1) 
        {
            streamijp1 = GetStream(0,-ijCoord.y);
        }


        // should use more than 1 iteration...
        // compute stream via jacobi iteration... 
        // sadly it take a while for the stream initial condition to be computed...
        streamij = (-vortij+dt/dxPow*(streamip1j+streamim1j+streamijp1+streamijm1))/(1.0+4.0*dt/dxPow);

        fragColor = vec4(streamij,0,0,0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// compute velocity

// must be modified in all buffers and image
// simulation parameters
const float dt = 1.0/400.0;
const float reynold = 200.0;
const float vorticesStrength = 5.0; //strength of vortices
const float sourceStrength = 40.0; //stength of sources
const float kappa = 0.0; //substance diffusion constant
const float alpha = 12.0; //substance dissipation rate
const float radius = 0.07; //radius of sources


//macro
#define GetStream(I,J) texelFetch( iChannel2, ijCoord+ivec2(I,J), 0 ).x
#define GetVelocity(I,J) texelFetch( iChannel3, ijCoord+ivec2(I,J), 0 ).xy

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    //set grid
    float dx = 1.0 / iResolution.y;
    float dxPow = dx *dx ;
    vec2 uvCoord = dx*fragCoord.xy;
    ivec2 ijCoord = ivec2(floor(fragCoord.xy));
        //to compute finite difference approximaton
        float streamij = GetStream(0,0); 
        float streamip1j = GetStream(1,0); 
        float streamim1j = GetStream(-1,0); 
        float streamijp1 = GetStream(0,1);
        float streamijm1 = GetStream(0,-1);

        //Set boundary condition (image method) 
        // Left

        if (ijCoord.x == 0) 
        {
            streamim1j = GetStream(-ijCoord.x+int(iResolution.x)-1,0);
        }
        // Right
        if (ijCoord.x == int(iResolution.x)-1) 
        {
            streamip1j = GetStream(-ijCoord.x,0);
        }

        // Down
        if (ijCoord.y == 0) 
        {
            streamijm1 = GetStream(0,-ijCoord.y+int(iResolution.y)-1);
        }

        // Up
        if (ijCoord.y == int(iResolution.y)-1) 
        {
            streamijp1 = GetStream(0,-ijCoord.y);
        }

        //compute velocity from stream function
        vec2 uij =  0.5*vec2(streamijp1-streamijm1, -(streamip1j-streamim1j))/dx+vec2(0.0,0.25);

        fragColor = vec4(uij, 0, 0);

}