

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// First experiment with fluid simulation ^_^
//
// This fluid simulation implements the method described in: https://developer.nvidia.com/sites/all/modules/custom/gpugems/books/GPUGems/gpugems_ch38.html
//
// Note1: on shadertoy it's not possible to execute the iterative Jacobi on the texture, 
// so I was inspired from this https://www.shadertoy.com/view/MdSczK to pre-compute 20 steps of the Jacobi solver.
// In the Common tab there is the Python script I used to compute the coefficients that are used in the Buffer C.
// 
// Note2: cannot add the diffusion step due there are no more buffers
//
//
// BUFFER A: output velocity field after advection of BUFFER D
// BUFFER B: output the divergence of the velocity field in x and generate and advect the RGB values colors in yxw
// BUFFER C: solve the Poisson equation and output the pressure field 
// BUFFER D: subtract the gradient of the pressure from the advected velocity field to obtain the divergence free field
//

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4(texture(iChannel1, ( fragCoord-vec2(0.5,0.5))/iResolution.xy).yzw,1);
    
    // Gamma correction
    fragColor = pow(fragColor, vec4(1.0/2.2) );
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// BUFFER A: output velocity field after advection

// Backward advection
vec4 Advect(in vec2 x, in float dt) 
{
    vec2 pos = x - dt*texture(iChannel0, x/iResolution.xy).xy;
    return texture(iChannel0, pos/iResolution.xy);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    float dt = iTimeDelta;
    
    vec2 x = fragCoord;    // Current cell coordinates from 0.5 to iResoltuion-0.5
    vec4 u;                // Holds computed velocity value for the current cell (pixel)
    
    // Set boundaries velocities
    if( x.x < 2.5 || x.x > iResolution.x-2.5 || x.y > iResolution.y-2.5 || x.y < 2.5) u.xy = vec2(-50,50);
    
    if(iFrame < 2) 
    {
        // Init velocity field
       u.xy = vec2(-50,50.);
    }
    else
    {
        // Set velocity at  boundaries
        if( x.x<2. || x.x>iResolution.x-2. || x.y>iResolution.y-2. || x.y<2.) vec2(-100,100);
        else u = Advect(x,1./60.); 
    }
    
    // Heart shape where there is no fluid
    vec2 p = (x*2.0-iResolution.xy)/iResolution.y;
    float d = sdHeart(p*1.8+vec2(0.,0.5));
    if(d<0.) u.xy = vec2(0);
    
    float s = iResolution.x / 100.;
    float d1 = length(x-vec2(iResolution.x*3.7/5.,iResolution.y*0.5/5.));
    if(d1 < s) 
    {
        u.xy = vec2(100,100);
    }
    
    fragColor = vec4(u);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// BUFFER B: output the divergence of the velocity field in x and generate and advect the RGB values colors in yxw

vec3 AdvectColors(in vec2 x, in float dt) 
{
    vec3 c = vec3(0); 

    float s = iResolution.x / 100.;
    float d = length(x-vec2(iResolution.x*3.7/5.,iResolution.y*0.5/5.));
    if(d < s) 
    {
        c = Palette(x.x*10.+50.*sin(iTime*0.001), vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(2.0,1.0,0.0),vec3(0.5,0.20,0.25));
    }
    
    else
    {
        vec2 pos = x - dt*texture(iChannel0, x/iResolution.xy).xy;    
        c = texture(iChannel1, pos/iResolution.xy).yzw;
    }
         
    return c;
}

float Divergence(vec2 x)
{
    float halfrdx = 0.5f; // Half cell size
    
    vec4 xL = texture(iChannel0, (x - vec2(1, 0))/iResolution.xy);
    vec4 xR = texture(iChannel0, (x + vec2(1, 0))/iResolution.xy);
    vec4 xT = texture(iChannel0, (x + vec2(0, 1))/iResolution.xy);
    vec4 xB = texture(iChannel0, (x - vec2(0, 1))/iResolution.xy);
    
    return halfrdx * (-xR.x + xL.x - xT.y + xB.y);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    fragColor = vec4(Divergence(fragCoord), AdvectColors(fragCoord, 1./60.) );
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// BUFFER C: solve the Poisson equation and output the pressure field 

float JacobiSolveForPressureXPart(vec2 x)
{
    vec4 v;
    
    v += 9.09494701772928e-13*texture(iChannel0,(x+vec2(-20,0))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(-19,-1))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(-19,1))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(-18,-2))/iResolution.xy);
    v += 3.63797880709171e-10*texture(iChannel0,(x+vec2(-18,0))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(-18,2))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(-17,-3))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(-17,-1))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(-17,1))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(-17,3))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(-16,-4))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(-16,-2))/iResolution.xy);
    v += 3.28327587340027e-8*texture(iChannel0,(x+vec2(-16,0))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(-16,2))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(-16,4))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(-15,-5))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(-15,-3))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(-15,-1))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(-15,1))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(-15,3))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(-15,5))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(-14,-6))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(-14,-4))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(-14,-2))/iResolution.xy);
    v += 1.18197931442410e-6*texture(iChannel0,(x+vec2(-14,0))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(-14,2))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(-14,4))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(-14,6))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(-13,-7))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(-13,-5))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(-13,-3))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(-13,-1))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(-13,1))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(-13,3))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(-13,5))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(-13,7))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(-12,-8))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(-12,-6))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(-12,-4))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(-12,-2))/iResolution.xy);
    v += 2.13495013667853e-5*texture(iChannel0,(x+vec2(-12,0))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(-12,2))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(-12,4))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(-12,6))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(-12,8))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(-11,-9))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(-11,-7))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(-11,-5))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(-11,-3))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(-11,-1))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(-11,1))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(-11,3))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(-11,5))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(-11,7))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(-11,9))/iResolution.xy);
    v += 1.68034603120759e-7*texture(iChannel0,(x+vec2(-10,-10))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(-10,-8))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(-10,-6))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(-10,-4))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(-10,-2))/iResolution.xy);
    v += 0.000218618893995881*texture(iChannel0,(x+vec2(-10,0))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(-10,2))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(-10,4))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(-10,6))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(-10,8))/iResolution.xy);
    v += 1.68034603120759e-7*texture(iChannel0,(x+vec2(-10,10))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(-9,-11))/iResolution.xy);
    v += 3.36069206241518e-6*texture(iChannel0,(x+vec2(-9,-9))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(-9,-7))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(-9,-5))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(-9,-3))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(-9,-1))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(-9,1))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(-9,3))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(-9,5))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(-9,7))/iResolution.xy);
    v += 3.36069206241518e-6*texture(iChannel0,(x+vec2(-9,9))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(-9,11))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(-8,-12))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(-8,-10))/iResolution.xy);
    v += 3.19265745929442e-5*texture(iChannel0,(x+vec2(-8,-8))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(-8,-6))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(-8,-4))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(-8,-2))/iResolution.xy);
    v += 0.00136636808747426*texture(iChannel0,(x+vec2(-8,0))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(-8,2))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(-8,4))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(-8,6))/iResolution.xy);
    v += 3.19265745929442e-5*texture(iChannel0,(x+vec2(-8,8))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(-8,10))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(-8,12))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(-7,-13))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(-7,-11))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(-7,-9))/iResolution.xy);
    v += 0.000191559447557665*texture(iChannel0,(x+vec2(-7,-7))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(-7,-5))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(-7,-3))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(-7,-1))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(-7,1))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(-7,3))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(-7,5))/iResolution.xy);
    v += 0.000191559447557665*texture(iChannel0,(x+vec2(-7,7))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(-7,9))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(-7,11))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(-7,13))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(-6,-14))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(-6,-12))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(-6,-10))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(-6,-8))/iResolution.xy);
    v += 0.000814127652120078*texture(iChannel0,(x+vec2(-6,-6))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(-6,-4))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(-6,-2))/iResolution.xy);
    v += 0.00546547234989703*texture(iChannel0,(x+vec2(-6,0))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(-6,2))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(-6,4))/iResolution.xy);
    v += 0.000814127652120078*texture(iChannel0,(x+vec2(-6,6))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(-6,8))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(-6,10))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(-6,12))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(-6,14))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(-5,-15))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(-5,-13))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(-5,-11))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(-5,-9))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(-5,-7))/iResolution.xy);
    v += 0.00260520848678425*texture(iChannel0,(x+vec2(-5,-5))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(-5,-3))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(-5,-1))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(-5,1))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(-5,3))/iResolution.xy);
    v += 0.00260520848678425*texture(iChannel0,(x+vec2(-5,5))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(-5,7))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(-5,9))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(-5,11))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(-5,13))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(-5,15))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(-4,-16))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(-4,-14))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(-4,-12))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(-4,-10))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(-4,-8))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(-4,-6))/iResolution.xy);
    v += 0.00651302121696062*texture(iChannel0,(x+vec2(-4,-4))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(-4,-2))/iResolution.xy);
    v += 0.0144322629239468*texture(iChannel0,(x+vec2(-4,0))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(-4,2))/iResolution.xy);
    v += 0.00651302121696062*texture(iChannel0,(x+vec2(-4,4))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(-4,6))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(-4,8))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(-4,10))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(-4,12))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(-4,14))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(-4,16))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(-3,-17))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(-3,-15))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(-3,-13))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(-3,-11))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(-3,-9))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(-3,-7))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(-3,-5))/iResolution.xy);
    v += 0.0130260424339212*texture(iChannel0,(x+vec2(-3,-3))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(-3,-1))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(-3,1))/iResolution.xy);
    v += 0.0130260424339212*texture(iChannel0,(x+vec2(-3,3))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(-3,5))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(-3,7))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(-3,9))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(-3,11))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(-3,13))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(-3,15))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(-3,17))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(-2,-18))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(-2,-16))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(-2,-14))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(-2,-12))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(-2,-10))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(-2,-8))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(-2,-6))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(-2,-4))/iResolution.xy);
    v += 0.0211673189551220*texture(iChannel0,(x+vec2(-2,-2))/iResolution.xy);
    v += 0.0256573563092388*texture(iChannel0,(x+vec2(-2,0))/iResolution.xy);
    v += 0.0211673189551220*texture(iChannel0,(x+vec2(-2,2))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(-2,4))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(-2,6))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(-2,8))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(-2,10))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(-2,12))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(-2,14))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(-2,16))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(-2,18))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(-1,-19))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(-1,-17))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(-1,-15))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(-1,-13))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(-1,-11))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(-1,-9))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(-1,-7))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(-1,-5))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(-1,-3))/iResolution.xy);
    v += 0.0282230919401627*texture(iChannel0,(x+vec2(-1,-1))/iResolution.xy);
    v += 0.0282230919401627*texture(iChannel0,(x+vec2(-1,1))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(-1,3))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(-1,5))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(-1,7))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(-1,9))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(-1,11))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(-1,13))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(-1,15))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(-1,17))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(-1,19))/iResolution.xy);
    v += 9.09494701772928e-13*texture(iChannel0,(x+vec2(0,-20))/iResolution.xy);
    v += 3.63797880709171e-10*texture(iChannel0,(x+vec2(0,-18))/iResolution.xy);
    v += 3.28327587340027e-8*texture(iChannel0,(x+vec2(0,-16))/iResolution.xy);
    v += 1.18197931442410e-6*texture(iChannel0,(x+vec2(0,-14))/iResolution.xy);
    v += 2.13495013667853e-5*texture(iChannel0,(x+vec2(0,-12))/iResolution.xy);
    v += 0.000218618893995881*texture(iChannel0,(x+vec2(0,-10))/iResolution.xy);
    v += 0.00136636808747426*texture(iChannel0,(x+vec2(0,-8))/iResolution.xy);
    v += 0.00546547234989703*texture(iChannel0,(x+vec2(0,-6))/iResolution.xy);
    v += 0.0144322629239468*texture(iChannel0,(x+vec2(0,-4))/iResolution.xy);
    v += 0.0256573563092388*texture(iChannel0,(x+vec2(0,-2))/iResolution.xy);
    v += 0.0310454011341790*texture(iChannel0,(x+vec2(0,0))/iResolution.xy);
    v += 0.0256573563092388*texture(iChannel0,(x+vec2(0,2))/iResolution.xy);
    v += 0.0144322629239468*texture(iChannel0,(x+vec2(0,4))/iResolution.xy);
    v += 0.00546547234989703*texture(iChannel0,(x+vec2(0,6))/iResolution.xy);
    v += 0.00136636808747426*texture(iChannel0,(x+vec2(0,8))/iResolution.xy);
    v += 0.000218618893995881*texture(iChannel0,(x+vec2(0,10))/iResolution.xy);
    v += 2.13495013667853e-5*texture(iChannel0,(x+vec2(0,12))/iResolution.xy);
    v += 1.18197931442410e-6*texture(iChannel0,(x+vec2(0,14))/iResolution.xy);
    v += 3.28327587340027e-8*texture(iChannel0,(x+vec2(0,16))/iResolution.xy);
    v += 3.63797880709171e-10*texture(iChannel0,(x+vec2(0,18))/iResolution.xy);
    v += 9.09494701772928e-13*texture(iChannel0,(x+vec2(0,20))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(1,-19))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(1,-17))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(1,-15))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(1,-13))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(1,-11))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(1,-9))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(1,-7))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(1,-5))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(1,-3))/iResolution.xy);
    v += 0.0282230919401627*texture(iChannel0,(x+vec2(1,-1))/iResolution.xy);
    v += 0.0282230919401627*texture(iChannel0,(x+vec2(1,1))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(1,3))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(1,5))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(1,7))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(1,9))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(1,11))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(1,13))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(1,15))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(1,17))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(1,19))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(2,-18))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(2,-16))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(2,-14))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(2,-12))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(2,-10))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(2,-8))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(2,-6))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(2,-4))/iResolution.xy);
    v += 0.0211673189551220*texture(iChannel0,(x+vec2(2,-2))/iResolution.xy);
    v += 0.0256573563092388*texture(iChannel0,(x+vec2(2,0))/iResolution.xy);
    v += 0.0211673189551220*texture(iChannel0,(x+vec2(2,2))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(2,4))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(2,6))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(2,8))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(2,10))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(2,12))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(2,14))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(2,16))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(2,18))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(3,-17))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(3,-15))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(3,-13))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(3,-11))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(3,-9))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(3,-7))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(3,-5))/iResolution.xy);
    v += 0.0130260424339212*texture(iChannel0,(x+vec2(3,-3))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(3,-1))/iResolution.xy);
    v += 0.0192430172319291*texture(iChannel0,(x+vec2(3,1))/iResolution.xy);
    v += 0.0130260424339212*texture(iChannel0,(x+vec2(3,3))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(3,5))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(3,7))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(3,9))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(3,11))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(3,13))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(3,15))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(3,17))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(4,-16))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(4,-14))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(4,-12))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(4,-10))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(4,-8))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(4,-6))/iResolution.xy);
    v += 0.00651302121696062*texture(iChannel0,(x+vec2(4,-4))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(4,-2))/iResolution.xy);
    v += 0.0144322629239468*texture(iChannel0,(x+vec2(4,0))/iResolution.xy);
    v += 0.0118418567581102*texture(iChannel0,(x+vec2(4,2))/iResolution.xy);
    v += 0.00651302121696062*texture(iChannel0,(x+vec2(4,4))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(4,6))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(4,8))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(4,10))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(4,12))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(4,14))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(4,16))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(5,-15))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(5,-13))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(5,-11))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(5,-9))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(5,-7))/iResolution.xy);
    v += 0.00260520848678425*texture(iChannel0,(x+vec2(5,-5))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(5,-3))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(5,-1))/iResolution.xy);
    v += 0.00888139256858267*texture(iChannel0,(x+vec2(5,1))/iResolution.xy);
    v += 0.00592092837905511*texture(iChannel0,(x+vec2(5,3))/iResolution.xy);
    v += 0.00260520848678425*texture(iChannel0,(x+vec2(5,5))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(5,7))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(5,9))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(5,11))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(5,13))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(5,15))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(6,-14))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(6,-12))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(6,-10))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(6,-8))/iResolution.xy);
    v += 0.000814127652120078*texture(iChannel0,(x+vec2(6,-6))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(6,-4))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(6,-2))/iResolution.xy);
    v += 0.00546547234989703*texture(iChannel0,(x+vec2(6,0))/iResolution.xy);
    v += 0.00444069628429133*texture(iChannel0,(x+vec2(6,2))/iResolution.xy);
    v += 0.00236837135162205*texture(iChannel0,(x+vec2(6,4))/iResolution.xy);
    v += 0.000814127652120078*texture(iChannel0,(x+vec2(6,6))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(6,8))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(6,10))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(6,12))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(6,14))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(7,-13))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(7,-11))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(7,-9))/iResolution.xy);
    v += 0.000191559447557665*texture(iChannel0,(x+vec2(7,-7))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(7,-5))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(7,-3))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(7,-1))/iResolution.xy);
    v += 0.00273273617494851*texture(iChannel0,(x+vec2(7,1))/iResolution.xy);
    v += 0.00177627851371653*texture(iChannel0,(x+vec2(7,3))/iResolution.xy);
    v += 0.000740116047381889*texture(iChannel0,(x+vec2(7,5))/iResolution.xy);
    v += 0.000191559447557665*texture(iChannel0,(x+vec2(7,7))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(7,9))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(7,11))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(7,13))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(8,-12))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(8,-10))/iResolution.xy);
    v += 3.19265745929442e-5*texture(iChannel0,(x+vec2(8,-8))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(8,-6))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(8,-4))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(8,-2))/iResolution.xy);
    v += 0.00136636808747426*texture(iChannel0,(x+vec2(8,0))/iResolution.xy);
    v += 0.00109309446997941*texture(iChannel0,(x+vec2(8,2))/iResolution.xy);
    v += 0.000555087035536417*texture(iChannel0,(x+vec2(8,4))/iResolution.xy);
    v += 0.000174144952325150*texture(iChannel0,(x+vec2(8,6))/iResolution.xy);
    v += 3.19265745929442e-5*texture(iChannel0,(x+vec2(8,8))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(8,10))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(8,12))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(9,-11))/iResolution.xy);
    v += 3.36069206241518e-6*texture(iChannel0,(x+vec2(9,-9))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(9,-7))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(9,-5))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(9,-3))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(9,-1))/iResolution.xy);
    v += 0.000546547234989703*texture(iChannel0,(x+vec2(9,1))/iResolution.xy);
    v += 0.000341592021868564*texture(iChannel0,(x+vec2(9,3))/iResolution.xy);
    v += 0.000130608714243863*texture(iChannel0,(x+vec2(9,5))/iResolution.xy);
    v += 2.90241587208584e-5*texture(iChannel0,(x+vec2(9,7))/iResolution.xy);
    v += 3.36069206241518e-6*texture(iChannel0,(x+vec2(9,9))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(9,11))/iResolution.xy);
    v += 1.68034603120759e-7*texture(iChannel0,(x+vec2(10,-10))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(10,-8))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(10,-6))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(10,-4))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(10,-2))/iResolution.xy);
    v += 0.000218618893995881*texture(iChannel0,(x+vec2(10,0))/iResolution.xy);
    v += 0.000170796010934282*texture(iChannel0,(x+vec2(10,2))/iResolution.xy);
    v += 8.03745933808386e-5*texture(iChannel0,(x+vec2(10,4))/iResolution.xy);
    v += 2.17681190406438e-5*texture(iChannel0,(x+vec2(10,6))/iResolution.xy);
    v += 3.05517460219562e-6*texture(iChannel0,(x+vec2(10,8))/iResolution.xy);
    v += 1.68034603120759e-7*texture(iChannel0,(x+vec2(10,10))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(11,-9))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(11,-7))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(11,-5))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(11,-3))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(11,-1))/iResolution.xy);
    v += 6.83184043737128e-5*texture(iChannel0,(x+vec2(11,1))/iResolution.xy);
    v += 4.01872966904193e-5*texture(iChannel0,(x+vec2(11,3))/iResolution.xy);
    v += 1.33957655634731e-5*texture(iChannel0,(x+vec2(11,5))/iResolution.xy);
    v += 2.29138095164672e-6*texture(iChannel0,(x+vec2(11,7))/iResolution.xy);
    v += 1.52758730109781e-7*texture(iChannel0,(x+vec2(11,9))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(12,-8))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(12,-6))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(12,-4))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(12,-2))/iResolution.xy);
    v += 2.13495013667853e-5*texture(iChannel0,(x+vec2(12,0))/iResolution.xy);
    v += 1.60749186761677e-5*texture(iChannel0,(x+vec2(12,2))/iResolution.xy);
    v += 6.69788278173655e-6*texture(iChannel0,(x+vec2(12,4))/iResolution.xy);
    v += 1.41008058562875e-6*texture(iChannel0,(x+vec2(12,6))/iResolution.xy);
    v += 1.14569047582336e-7*texture(iChannel0,(x+vec2(12,8))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(13,-7))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(13,-5))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(13,-3))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(13,-1))/iResolution.xy);
    v += 5.02341208630241e-6*texture(iChannel0,(x+vec2(13,1))/iResolution.xy);
    v += 2.67915311269462e-6*texture(iChannel0,(x+vec2(13,3))/iResolution.xy);
    v += 7.05040292814374e-7*texture(iChannel0,(x+vec2(13,5))/iResolution.xy);
    v += 7.05040292814374e-8*texture(iChannel0,(x+vec2(13,7))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(14,-6))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(14,-4))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(14,-2))/iResolution.xy);
    v += 1.18197931442410e-6*texture(iChannel0,(x+vec2(14,0))/iResolution.xy);
    v += 8.37235347717069e-7*texture(iChannel0,(x+vec2(14,2))/iResolution.xy);
    v += 2.82016117125750e-7*texture(iChannel0,(x+vec2(14,4))/iResolution.xy);
    v += 3.52520146407187e-8*texture(iChannel0,(x+vec2(14,6))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(15,-5))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(15,-3))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(15,-1))/iResolution.xy);
    v += 1.96996552404016e-7*texture(iChannel0,(x+vec2(15,1))/iResolution.xy);
    v += 8.81300366017967e-8*texture(iChannel0,(x+vec2(15,3))/iResolution.xy);
    v += 1.41008058562875e-8*texture(iChannel0,(x+vec2(15,5))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(16,-4))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(16,-2))/iResolution.xy);
    v += 3.28327587340027e-8*texture(iChannel0,(x+vec2(16,0))/iResolution.xy);
    v += 2.07364792004228e-8*texture(iChannel0,(x+vec2(16,2))/iResolution.xy);
    v += 4.40650183008984e-9*texture(iChannel0,(x+vec2(16,4))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(17,-3))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(17,-1))/iResolution.xy);
    v += 3.45607986673713e-9*texture(iChannel0,(x+vec2(17,1))/iResolution.xy);
    v += 1.03682396002114e-9*texture(iChannel0,(x+vec2(17,3))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(18,-2))/iResolution.xy);
    v += 3.63797880709171e-10*texture(iChannel0,(x+vec2(18,0))/iResolution.xy);
    v += 1.72803993336856e-10*texture(iChannel0,(x+vec2(18,2))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(19,-1))/iResolution.xy);
    v += 1.81898940354586e-11*texture(iChannel0,(x+vec2(19,1))/iResolution.xy);
    v += 9.09494701772928e-13*texture(iChannel0,(x+vec2(20,0))/iResolution.xy);
   
    return v.x;
}

float JacobiSolveForPressureBPart(vec2 x) 
{
    vec4 v;

v += -9.09494701772928e-13*texture(iChannel1,(x+vec2(-19,0))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(-18,-1))/iResolution.xy);
v += -3.63797880709171e-12*texture(iChannel1,(x+vec2(-18,0))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(-18,1))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(-17,-2))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(-17,-1))/iResolution.xy);
v += -3.42879502568394e-10*texture(iChannel1,(x+vec2(-17,0))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(-17,1))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(-17,2))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(-16,-3))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(-16,-2))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(-16,-1))/iResolution.xy);
v += -1.23691279441118e-9*texture(iChannel1,(x+vec2(-16,0))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(-16,1))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(-16,2))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(-16,3))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(-15,-4))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(-15,-3))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(-15,-2))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(-15,-1))/iResolution.xy);
v += -3.10328687191941e-8*texture(iChannel1,(x+vec2(-15,0))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(-15,1))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(-15,2))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(-15,3))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(-15,4))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(-14,-5))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(-14,-4))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(-14,-3))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(-14,-2))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(-14,-1))/iResolution.xy);
v += -1.00993929663673e-7*texture(iChannel1,(x+vec2(-14,0))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(-14,1))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(-14,2))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(-14,3))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(-14,4))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(-14,5))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(-13,-6))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(-13,-5))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(-13,-4))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(-13,-3))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(-13,-2))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(-13,-1))/iResolution.xy);
v += -1.17924446385587e-6*texture(iChannel1,(x+vec2(-13,0))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(-13,1))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(-13,2))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(-13,3))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(-13,4))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(-13,5))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(-13,6))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(-12,-7))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(-12,-6))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(-12,-5))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(-12,-4))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(-12,-3))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(-12,-2))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(-12,-1))/iResolution.xy);
v += -3.45800071954727e-6*texture(iChannel1,(x+vec2(-12,0))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(-12,1))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(-12,2))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(-12,3))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(-12,4))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(-12,5))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(-12,6))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(-12,7))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(-11,-8))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(-11,-7))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(-11,-6))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(-11,-5))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(-11,-4))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(-11,-3))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(-11,-2))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(-11,-1))/iResolution.xy);
v += -2.36486230278388e-5*texture(iChannel1,(x+vec2(-11,0))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(-11,1))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(-11,2))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(-11,3))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(-11,4))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(-11,5))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(-11,6))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(-11,7))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(-11,8))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(-10,-9))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(-10,-8))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(-10,-7))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(-10,-6))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(-10,-5))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(-10,-4))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(-10,-3))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(-10,-2))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(-10,-1))/iResolution.xy);
v += -6.24149688519537e-5*texture(iChannel1,(x+vec2(-10,0))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(-10,1))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(-10,2))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(-10,3))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(-10,4))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(-10,5))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(-10,6))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(-10,7))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(-10,8))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(-10,9))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(-9,-10))/iResolution.xy);
v += -1.76878529600799e-7*texture(iChannel1,(x+vec2(-9,-9))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(-9,-8))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(-9,-7))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(-9,-6))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(-9,-5))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(-9,-4))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(-9,-3))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(-9,-2))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(-9,-1))/iResolution.xy);
v += -0.000284433263004757*texture(iChannel1,(x+vec2(-9,0))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(-9,1))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(-9,2))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(-9,3))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(-9,4))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(-9,5))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(-9,6))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(-9,7))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(-9,8))/iResolution.xy);
v += -1.76878529600799e-7*texture(iChannel1,(x+vec2(-9,9))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(-9,10))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(-8,-11))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(-8,-10))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(-8,-9))/iResolution.xy);
v += -3.93294612877071e-6*texture(iChannel1,(x+vec2(-8,-8))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(-8,-7))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(-8,-6))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(-8,-5))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(-8,-4))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(-8,-3))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(-8,-2))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(-8,-1))/iResolution.xy);
v += -0.000675835879519582*texture(iChannel1,(x+vec2(-8,0))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(-8,1))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(-8,2))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(-8,3))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(-8,4))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(-8,5))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(-8,6))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(-8,7))/iResolution.xy);
v += -3.93294612877071e-6*texture(iChannel1,(x+vec2(-8,8))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(-8,9))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(-8,10))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(-8,11))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(-7,-12))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(-7,-11))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(-7,-10))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(-7,-9))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(-7,-8))/iResolution.xy);
v += -4.22448356403038e-5*texture(iChannel1,(x+vec2(-7,-7))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(-7,-6))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(-7,-5))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(-7,-4))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(-7,-3))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(-7,-2))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(-7,-1))/iResolution.xy);
v += -0.00223807293514255*texture(iChannel1,(x+vec2(-7,0))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(-7,1))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(-7,2))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(-7,3))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(-7,4))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(-7,5))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(-7,6))/iResolution.xy);
v += -4.22448356403038e-5*texture(iChannel1,(x+vec2(-7,7))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(-7,8))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(-7,9))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(-7,10))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(-7,11))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(-7,12))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(-6,-13))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(-6,-12))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(-6,-11))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(-6,-10))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(-6,-9))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(-6,-8))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(-6,-7))/iResolution.xy);
v += -0.000292745651677251*texture(iChannel1,(x+vec2(-6,-6))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(-6,-5))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(-6,-4))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(-6,-3))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(-6,-2))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(-6,-1))/iResolution.xy);
v += -0.00480667228111997*texture(iChannel1,(x+vec2(-6,0))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(-6,1))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(-6,2))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(-6,3))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(-6,4))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(-6,5))/iResolution.xy);
v += -0.000292745651677251*texture(iChannel1,(x+vec2(-6,6))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(-6,7))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(-6,8))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(-6,9))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(-6,10))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(-6,11))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(-6,12))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(-6,13))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(-5,-14))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(-5,-13))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(-5,-12))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(-5,-11))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(-5,-10))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(-5,-9))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(-5,-8))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(-5,-7))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(-5,-6))/iResolution.xy);
v += -0.00147693132748827*texture(iChannel1,(x+vec2(-5,-5))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(-5,-4))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(-5,-3))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(-5,-2))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(-5,-1))/iResolution.xy);
v += -0.0123926616652170*texture(iChannel1,(x+vec2(-5,0))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(-5,1))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(-5,2))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(-5,3))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(-5,4))/iResolution.xy);
v += -0.00147693132748827*texture(iChannel1,(x+vec2(-5,5))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(-5,6))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(-5,7))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(-5,8))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(-5,9))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(-5,10))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(-5,11))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(-5,12))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(-5,13))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(-5,14))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(-4,-15))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(-4,-14))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(-4,-13))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(-4,-12))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(-4,-11))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(-4,-10))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(-4,-9))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(-4,-8))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(-4,-7))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(-4,-6))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(-4,-5))/iResolution.xy);
v += -0.00581894547212869*texture(iChannel1,(x+vec2(-4,-4))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(-4,-3))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(-4,-2))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(-4,-1))/iResolution.xy);
v += -0.0243988493457437*texture(iChannel1,(x+vec2(-4,0))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(-4,1))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(-4,2))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(-4,3))/iResolution.xy);
v += -0.00581894547212869*texture(iChannel1,(x+vec2(-4,4))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(-4,5))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(-4,6))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(-4,7))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(-4,8))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(-4,9))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(-4,10))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(-4,11))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(-4,12))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(-4,13))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(-4,14))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(-4,15))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(-3,-16))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(-3,-15))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(-3,-14))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(-3,-13))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(-3,-12))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(-3,-11))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(-3,-10))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(-3,-9))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(-3,-8))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(-3,-7))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(-3,-6))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(-3,-5))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(-3,-4))/iResolution.xy);
v += -0.0188449879060499*texture(iChannel1,(x+vec2(-3,-3))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(-3,-2))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(-3,-1))/iResolution.xy);
v += -0.0526613406873366*texture(iChannel1,(x+vec2(-3,0))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(-3,1))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(-3,2))/iResolution.xy);
v += -0.0188449879060499*texture(iChannel1,(x+vec2(-3,3))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(-3,4))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(-3,5))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(-3,6))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(-3,7))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(-3,8))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(-3,9))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(-3,10))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(-3,11))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(-3,12))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(-3,13))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(-3,14))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(-3,15))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(-3,16))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(-2,-17))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(-2,-16))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(-2,-15))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(-2,-14))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(-2,-13))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(-2,-12))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(-2,-11))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(-2,-10))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(-2,-9))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(-2,-8))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(-2,-7))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(-2,-6))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(-2,-5))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(-2,-4))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(-2,-3))/iResolution.xy);
v += -0.0527126982342452*texture(iChannel1,(x+vec2(-2,-2))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(-2,-1))/iResolution.xy);
v += -0.0997894092142815*texture(iChannel1,(x+vec2(-2,0))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(-2,1))/iResolution.xy);
v += -0.0527126982342452*texture(iChannel1,(x+vec2(-2,2))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(-2,3))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(-2,4))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(-2,5))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(-2,6))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(-2,7))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(-2,8))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(-2,9))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(-2,10))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(-2,11))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(-2,12))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(-2,13))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(-2,14))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(-2,15))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(-2,16))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(-2,17))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(-1,-18))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(-1,-17))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(-1,-16))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(-1,-15))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(-1,-14))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(-1,-13))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(-1,-12))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(-1,-11))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(-1,-10))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(-1,-9))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(-1,-8))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(-1,-7))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(-1,-6))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(-1,-5))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(-1,-4))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(-1,-3))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(-1,-2))/iResolution.xy);
v += -0.137381974054733*texture(iChannel1,(x+vec2(-1,-1))/iResolution.xy);
v += -0.205597335680068*texture(iChannel1,(x+vec2(-1,0))/iResolution.xy);
v += -0.137381974054733*texture(iChannel1,(x+vec2(-1,1))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(-1,2))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(-1,3))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(-1,4))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(-1,5))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(-1,6))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(-1,7))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(-1,8))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(-1,9))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(-1,10))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(-1,11))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(-1,12))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(-1,13))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(-1,14))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(-1,15))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(-1,16))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(-1,17))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(-1,18))/iResolution.xy);
v += -9.09494701772928e-13*texture(iChannel1,(x+vec2(0,-19))/iResolution.xy);
v += -3.63797880709171e-12*texture(iChannel1,(x+vec2(0,-18))/iResolution.xy);
v += -3.42879502568394e-10*texture(iChannel1,(x+vec2(0,-17))/iResolution.xy);
v += -1.23691279441118e-9*texture(iChannel1,(x+vec2(0,-16))/iResolution.xy);
v += -3.10328687191941e-8*texture(iChannel1,(x+vec2(0,-15))/iResolution.xy);
v += -1.00993929663673e-7*texture(iChannel1,(x+vec2(0,-14))/iResolution.xy);
v += -1.17924446385587e-6*texture(iChannel1,(x+vec2(0,-13))/iResolution.xy);
v += -3.45800071954727e-6*texture(iChannel1,(x+vec2(0,-12))/iResolution.xy);
v += -2.36486230278388e-5*texture(iChannel1,(x+vec2(0,-11))/iResolution.xy);
v += -6.24149688519537e-5*texture(iChannel1,(x+vec2(0,-10))/iResolution.xy);
v += -0.000284433263004757*texture(iChannel1,(x+vec2(0,-9))/iResolution.xy);
v += -0.000675835879519582*texture(iChannel1,(x+vec2(0,-8))/iResolution.xy);
v += -0.00223807293514255*texture(iChannel1,(x+vec2(0,-7))/iResolution.xy);
v += -0.00480667228111997*texture(iChannel1,(x+vec2(0,-6))/iResolution.xy);
v += -0.0123926616652170*texture(iChannel1,(x+vec2(0,-5))/iResolution.xy);
v += -0.0243988493457437*texture(iChannel1,(x+vec2(0,-4))/iResolution.xy);
v += -0.0526613406873366*texture(iChannel1,(x+vec2(0,-3))/iResolution.xy);
v += -0.0997894092142815*texture(iChannel1,(x+vec2(0,-2))/iResolution.xy);
v += -0.205597335680068*texture(iChannel1,(x+vec2(0,-1))/iResolution.xy);
v += -0.447835985396523*texture(iChannel1,(x+vec2(0,0))/iResolution.xy);
v += -0.205597335680068*texture(iChannel1,(x+vec2(0,1))/iResolution.xy);
v += -0.0997894092142815*texture(iChannel1,(x+vec2(0,2))/iResolution.xy);
v += -0.0526613406873366*texture(iChannel1,(x+vec2(0,3))/iResolution.xy);
v += -0.0243988493457437*texture(iChannel1,(x+vec2(0,4))/iResolution.xy);
v += -0.0123926616652170*texture(iChannel1,(x+vec2(0,5))/iResolution.xy);
v += -0.00480667228111997*texture(iChannel1,(x+vec2(0,6))/iResolution.xy);
v += -0.00223807293514255*texture(iChannel1,(x+vec2(0,7))/iResolution.xy);
v += -0.000675835879519582*texture(iChannel1,(x+vec2(0,8))/iResolution.xy);
v += -0.000284433263004757*texture(iChannel1,(x+vec2(0,9))/iResolution.xy);
v += -6.24149688519537e-5*texture(iChannel1,(x+vec2(0,10))/iResolution.xy);
v += -2.36486230278388e-5*texture(iChannel1,(x+vec2(0,11))/iResolution.xy);
v += -3.45800071954727e-6*texture(iChannel1,(x+vec2(0,12))/iResolution.xy);
v += -1.17924446385587e-6*texture(iChannel1,(x+vec2(0,13))/iResolution.xy);
v += -1.00993929663673e-7*texture(iChannel1,(x+vec2(0,14))/iResolution.xy);
v += -3.10328687191941e-8*texture(iChannel1,(x+vec2(0,15))/iResolution.xy);
v += -1.23691279441118e-9*texture(iChannel1,(x+vec2(0,16))/iResolution.xy);
v += -3.42879502568394e-10*texture(iChannel1,(x+vec2(0,17))/iResolution.xy);
v += -3.63797880709171e-12*texture(iChannel1,(x+vec2(0,18))/iResolution.xy);
v += -9.09494701772928e-13*texture(iChannel1,(x+vec2(0,19))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(1,-18))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(1,-17))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(1,-16))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(1,-15))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(1,-14))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(1,-13))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(1,-12))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(1,-11))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(1,-10))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(1,-9))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(1,-8))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(1,-7))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(1,-6))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(1,-5))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(1,-4))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(1,-3))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(1,-2))/iResolution.xy);
v += -0.137381974054733*texture(iChannel1,(x+vec2(1,-1))/iResolution.xy);
v += -0.205597335680068*texture(iChannel1,(x+vec2(1,0))/iResolution.xy);
v += -0.137381974054733*texture(iChannel1,(x+vec2(1,1))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(1,2))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(1,3))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(1,4))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(1,5))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(1,6))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(1,7))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(1,8))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(1,9))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(1,10))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(1,11))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(1,12))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(1,13))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(1,14))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(1,15))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(1,16))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(1,17))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(1,18))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(2,-17))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(2,-16))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(2,-15))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(2,-14))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(2,-13))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(2,-12))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(2,-11))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(2,-10))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(2,-9))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(2,-8))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(2,-7))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(2,-6))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(2,-5))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(2,-4))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(2,-3))/iResolution.xy);
v += -0.0527126982342452*texture(iChannel1,(x+vec2(2,-2))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(2,-1))/iResolution.xy);
v += -0.0997894092142815*texture(iChannel1,(x+vec2(2,0))/iResolution.xy);
v += -0.0832781583994802*texture(iChannel1,(x+vec2(2,1))/iResolution.xy);
v += -0.0527126982342452*texture(iChannel1,(x+vec2(2,2))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(2,3))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(2,4))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(2,5))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(2,6))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(2,7))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(2,8))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(2,9))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(2,10))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(2,11))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(2,12))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(2,13))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(2,14))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(2,15))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(2,16))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(2,17))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(3,-16))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(3,-15))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(3,-14))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(3,-13))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(3,-12))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(3,-11))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(3,-10))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(3,-9))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(3,-8))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(3,-7))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(3,-6))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(3,-5))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(3,-4))/iResolution.xy);
v += -0.0188449879060499*texture(iChannel1,(x+vec2(3,-3))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(3,-2))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(3,-1))/iResolution.xy);
v += -0.0526613406873366*texture(iChannel1,(x+vec2(3,0))/iResolution.xy);
v += -0.0432285520946607*texture(iChannel1,(x+vec2(3,1))/iResolution.xy);
v += -0.0327308975465712*texture(iChannel1,(x+vec2(3,2))/iResolution.xy);
v += -0.0188449879060499*texture(iChannel1,(x+vec2(3,3))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(3,4))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(3,5))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(3,6))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(3,7))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(3,8))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(3,9))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(3,10))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(3,11))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(3,12))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(3,13))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(3,14))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(3,15))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(3,16))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(4,-15))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(4,-14))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(4,-13))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(4,-12))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(4,-11))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(4,-10))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(4,-9))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(4,-8))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(4,-7))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(4,-6))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(4,-5))/iResolution.xy);
v += -0.00581894547212869*texture(iChannel1,(x+vec2(4,-4))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(4,-3))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(4,-2))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(4,-1))/iResolution.xy);
v += -0.0243988493457437*texture(iChannel1,(x+vec2(4,0))/iResolution.xy);
v += -0.0234868289771839*texture(iChannel1,(x+vec2(4,1))/iResolution.xy);
v += -0.0161373519513290*texture(iChannel1,(x+vec2(4,2))/iResolution.xy);
v += -0.0114720994824893*texture(iChannel1,(x+vec2(4,3))/iResolution.xy);
v += -0.00581894547212869*texture(iChannel1,(x+vec2(4,4))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(4,5))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(4,6))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(4,7))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(4,8))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(4,9))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(4,10))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(4,11))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(4,12))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(4,13))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(4,14))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(4,15))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(5,-14))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(5,-13))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(5,-12))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(5,-11))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(5,-10))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(5,-9))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(5,-8))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(5,-7))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(5,-6))/iResolution.xy);
v += -0.00147693132748827*texture(iChannel1,(x+vec2(5,-5))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(5,-4))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(5,-3))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(5,-2))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(5,-1))/iResolution.xy);
v += -0.0123926616652170*texture(iChannel1,(x+vec2(5,0))/iResolution.xy);
v += -0.0101825625170022*texture(iChannel1,(x+vec2(5,1))/iResolution.xy);
v += -0.00870143855718197*texture(iChannel1,(x+vec2(5,2))/iResolution.xy);
v += -0.00508711260044947*texture(iChannel1,(x+vec2(5,3))/iResolution.xy);
v += -0.00342230207024841*texture(iChannel1,(x+vec2(5,4))/iResolution.xy);
v += -0.00147693132748827*texture(iChannel1,(x+vec2(5,5))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(5,6))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(5,7))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(5,8))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(5,9))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(5,10))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(5,11))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(5,12))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(5,13))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(5,14))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(6,-13))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(6,-12))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(6,-11))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(6,-10))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(6,-9))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(6,-8))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(6,-7))/iResolution.xy);
v += -0.000292745651677251*texture(iChannel1,(x+vec2(6,-6))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(6,-5))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(6,-4))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(6,-3))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(6,-2))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(6,-1))/iResolution.xy);
v += -0.00480667228111997*texture(iChannel1,(x+vec2(6,0))/iResolution.xy);
v += -0.00503071343700867*texture(iChannel1,(x+vec2(6,1))/iResolution.xy);
v += -0.00339872715994716*texture(iChannel1,(x+vec2(6,2))/iResolution.xy);
v += -0.00267353867093334*texture(iChannel1,(x+vec2(6,3))/iResolution.xy);
v += -0.00130621888092719*texture(iChannel1,(x+vec2(6,4))/iResolution.xy);
v += -0.000834164828120265*texture(iChannel1,(x+vec2(6,5))/iResolution.xy);
v += -0.000292745651677251*texture(iChannel1,(x+vec2(6,6))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(6,7))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(6,8))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(6,9))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(6,10))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(6,11))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(6,12))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(6,13))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(7,-12))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(7,-11))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(7,-10))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(7,-9))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(7,-8))/iResolution.xy);
v += -4.22448356403038e-5*texture(iChannel1,(x+vec2(7,-7))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(7,-6))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(7,-5))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(7,-4))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(7,-3))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(7,-2))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(7,-1))/iResolution.xy);
v += -0.00223807293514255*texture(iChannel1,(x+vec2(7,0))/iResolution.xy);
v += -0.00173489178996533*texture(iChannel1,(x+vec2(7,1))/iResolution.xy);
v += -0.00162991425895598*texture(iChannel1,(x+vec2(7,2))/iResolution.xy);
v += -0.000902096042409539*texture(iChannel1,(x+vec2(7,3))/iResolution.xy);
v += -0.000663241306028794*texture(iChannel1,(x+vec2(7,4))/iResolution.xy);
v += -0.000260763452388346*texture(iChannel1,(x+vec2(7,5))/iResolution.xy);
v += -0.000158390301294276*texture(iChannel1,(x+vec2(7,6))/iResolution.xy);
v += -4.22448356403038e-5*texture(iChannel1,(x+vec2(7,7))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(7,8))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(7,9))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(7,10))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(7,11))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(7,12))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(8,-11))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(8,-10))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(8,-9))/iResolution.xy);
v += -3.93294612877071e-6*texture(iChannel1,(x+vec2(8,-8))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(8,-7))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(8,-6))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(8,-5))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(8,-4))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(8,-3))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(8,-2))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(8,-1))/iResolution.xy);
v += -0.000675835879519582*texture(iChannel1,(x+vec2(8,0))/iResolution.xy);
v += -0.000773602703702636*texture(iChannel1,(x+vec2(8,1))/iResolution.xy);
v += -0.000483942043501884*texture(iChannel1,(x+vec2(8,2))/iResolution.xy);
v += -0.000417968447436579*texture(iChannel1,(x+vec2(8,3))/iResolution.xy);
v += -0.000183886848390102*texture(iChannel1,(x+vec2(8,4))/iResolution.xy);
v += -0.000127373421491939*texture(iChannel1,(x+vec2(8,5))/iResolution.xy);
v += -3.78072654712014e-5*texture(iChannel1,(x+vec2(8,6))/iResolution.xy);
v += -2.18790937651647e-5*texture(iChannel1,(x+vec2(8,7))/iResolution.xy);
v += -3.93294612877071e-6*texture(iChannel1,(x+vec2(8,8))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(8,9))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(8,10))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(8,11))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(9,-10))/iResolution.xy);
v += -1.76878529600799e-7*texture(iChannel1,(x+vec2(9,-9))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(9,-8))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(9,-7))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(9,-6))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(9,-5))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(9,-4))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(9,-3))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(9,-2))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(9,-1))/iResolution.xy);
v += -0.000284433263004757*texture(iChannel1,(x+vec2(9,0))/iResolution.xy);
v += -0.000199741101823747*texture(iChannel1,(x+vec2(9,1))/iResolution.xy);
v += -0.000207377233891748*texture(iChannel1,(x+vec2(9,2))/iResolution.xy);
v += -0.000101948855444789*texture(iChannel1,(x+vec2(9,3))/iResolution.xy);
v += -8.20512541395146e-5*texture(iChannel1,(x+vec2(9,4))/iResolution.xy);
v += -2.70361197181046e-5*texture(iChannel1,(x+vec2(9,5))/iResolution.xy);
v += -1.77311976585770e-5*texture(iChannel1,(x+vec2(9,6))/iResolution.xy);
v += -3.53132782038301e-6*texture(iChannel1,(x+vec2(9,7))/iResolution.xy);
v += -1.95008578884881e-6*texture(iChannel1,(x+vec2(9,8))/iResolution.xy);
v += -1.76878529600799e-7*texture(iChannel1,(x+vec2(9,9))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(9,10))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(10,-9))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(10,-8))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(10,-7))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(10,-6))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(10,-5))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(10,-4))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(10,-3))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(10,-2))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(10,-1))/iResolution.xy);
v += -6.24149688519537e-5*texture(iChannel1,(x+vec2(10,0))/iResolution.xy);
v += -8.00984416855499e-5*texture(iChannel1,(x+vec2(10,1))/iResolution.xy);
v += -4.38769347965717e-5*texture(iChannel1,(x+vec2(10,2))/iResolution.xy);
v += -4.19905081798788e-5*texture(iChannel1,(x+vec2(10,3))/iResolution.xy);
v += -1.53331930050626e-5*texture(iChannel1,(x+vec2(10,4))/iResolution.xy);
v += -1.15973198262509e-5*texture(iChannel1,(x+vec2(10,5))/iResolution.xy);
v += -2.55007762461901e-6*texture(iChannel1,(x+vec2(10,6))/iResolution.xy);
v += -1.58909278979991e-6*texture(iChannel1,(x+vec2(10,7))/iResolution.xy);
v += -1.59190676640719e-7*texture(iChannel1,(x+vec2(10,8))/iResolution.xy);
v += -8.40173015603796e-8*texture(iChannel1,(x+vec2(10,9))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(11,-8))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(11,-7))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(11,-6))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(11,-5))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(11,-4))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(11,-3))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(11,-2))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(11,-1))/iResolution.xy);
v += -2.36486230278388e-5*texture(iChannel1,(x+vec2(11,0))/iResolution.xy);
v += -1.43607612699270e-5*texture(iChannel1,(x+vec2(11,1))/iResolution.xy);
v += -1.68375663633924e-5*texture(iChannel1,(x+vec2(11,2))/iResolution.xy);
v += -6.80304947309196e-6*texture(iChannel1,(x+vec2(11,3))/iResolution.xy);
v += -6.06828325544484e-6*texture(iChannel1,(x+vec2(11,4))/iResolution.xy);
v += -1.46988895721734e-6*texture(iChannel1,(x+vec2(11,5))/iResolution.xy);
v += -1.05081926449202e-6*texture(iChannel1,(x+vec2(11,6))/iResolution.xy);
v += -1.15775037556887e-7*texture(iChannel1,(x+vec2(11,7))/iResolution.xy);
v += -6.87414285494015e-8*texture(iChannel1,(x+vec2(11,8))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(12,-7))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(12,-6))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(12,-5))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(12,-4))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(12,-3))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(12,-2))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(12,-1))/iResolution.xy);
v += -3.45800071954727e-6*texture(iChannel1,(x+vec2(12,0))/iResolution.xy);
v += -5.17681837663986e-6*texture(iChannel1,(x+vec2(12,1))/iResolution.xy);
v += -2.30951991397887e-6*texture(iChannel1,(x+vec2(12,2))/iResolution.xy);
v += -2.50313678407110e-6*texture(iChannel1,(x+vec2(12,3))/iResolution.xy);
v += -6.67001586407423e-7*texture(iChannel1,(x+vec2(12,4))/iResolution.xy);
v += -5.58899046154693e-7*texture(iChannel1,(x+vec2(12,5))/iResolution.xy);
v += -6.75354385748506e-8*texture(iChannel1,(x+vec2(12,6))/iResolution.xy);
v += -4.58276190329343e-8*texture(iChannel1,(x+vec2(12,7))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(13,-6))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(13,-5))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(13,-4))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(13,-3))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(13,-2))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(13,-1))/iResolution.xy);
v += -1.17924446385587e-6*texture(iChannel1,(x+vec2(13,0))/iResolution.xy);
v += -5.78991603106260e-7*texture(iChannel1,(x+vec2(13,1))/iResolution.xy);
v += -7.95476807979867e-7*texture(iChannel1,(x+vec2(13,2))/iResolution.xy);
v += -2.32976162806153e-7*texture(iChannel1,(x+vec2(13,3))/iResolution.xy);
v += -2.35570041695610e-7*texture(iChannel1,(x+vec2(13,4))/iResolution.xy);
v += -3.11702024191618e-8*texture(iChannel1,(x+vec2(13,5))/iResolution.xy);
v += -2.46764102485031e-8*texture(iChannel1,(x+vec2(13,6))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(14,-5))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(14,-4))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(14,-3))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(14,-2))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(14,-1))/iResolution.xy);
v += -1.00993929663673e-7*texture(iChannel1,(x+vec2(14,0))/iResolution.xy);
v += -1.87838850251865e-7*texture(iChannel1,(x+vec2(14,1))/iResolution.xy);
v += -6.04195520281792e-8*texture(iChannel1,(x+vec2(14,2))/iResolution.xy);
v += -7.68741301726550e-8*texture(iChannel1,(x+vec2(14,3))/iResolution.xy);
v += -1.11322151497006e-8*texture(iChannel1,(x+vec2(14,4))/iResolution.xy);
v += -1.05756043922156e-8*texture(iChannel1,(x+vec2(14,5))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(15,-4))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(15,-3))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(15,-2))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(15,-1))/iResolution.xy);
v += -3.10328687191941e-8*texture(iChannel1,(x+vec2(15,0))/iResolution.xy);
v += -1.09503162093461e-8*texture(iChannel1,(x+vec2(15,1))/iResolution.xy);
v += -1.87237674253993e-8*texture(iChannel1,(x+vec2(15,2))/iResolution.xy);
v += -2.96859070658684e-9*texture(iChannel1,(x+vec2(15,3))/iResolution.xy);
v += -3.52520146407187e-9*texture(iChannel1,(x+vec2(15,4))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(16,-3))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(16,-2))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(16,-1))/iResolution.xy);
v += -1.23691279441118e-9*texture(iChannel1,(x+vec2(16,0))/iResolution.xy);
v += -3.20233084494248e-9*texture(iChannel1,(x+vec2(16,1))/iResolution.xy);
v += -5.56610757485032e-10*texture(iChannel1,(x+vec2(16,2))/iResolution.xy);
v += -8.81300366017967e-10*texture(iChannel1,(x+vec2(16,3))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(17,-2))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(17,-1))/iResolution.xy);
v += -3.42879502568394e-10*texture(iChannel1,(x+vec2(17,0))/iResolution.xy);
v += -6.54836185276508e-11*texture(iChannel1,(x+vec2(17,1))/iResolution.xy);
v += -1.55523594003171e-10*texture(iChannel1,(x+vec2(17,2))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(18,-1))/iResolution.xy);
v += -3.63797880709171e-12*texture(iChannel1,(x+vec2(18,0))/iResolution.xy);
v += -1.72803993336856e-11*texture(iChannel1,(x+vec2(18,1))/iResolution.xy);
v += -9.09494701772928e-13*texture(iChannel1,(x+vec2(19,0))/iResolution.xy);

    return v.x;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord)
{ 
    float p = JacobiSolveForPressureXPart(fragCoord) +  JacobiSolveForPressureBPart(fragCoord);
    fragColor = vec4(p,0,0,1);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
// BUFFER D: subtract the gradient of the pressure from the advected velocity field to obtain the divergence free field// BUFFER C: solve the Poisson equation and output the pressure field 

// Gradient of the pressure field
vec2 Gradient(vec2 x)
{
    float xL = texture(iChannel0, (x + vec2(1, 0))/iResolution.xy).x;
    float xR = texture(iChannel0, (x - vec2(1, 0))/iResolution.xy).x;
    float xB = texture(iChannel0, (x + vec2(0, 1))/iResolution.xy).x;
    float xT = texture(iChannel0, (x - vec2(0, 1))/iResolution.xy).x;

    return vec2(xR - xL, xT - xB);
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    // Subtract the pressure gradient to the velocity field to obtain the divergence free field
    
    vec4 u = texture(iChannel1, fragCoord/iResolution.xy);
    fragColor = vec4(u.xy - Gradient(fragCoord), u.zw);
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// Tnx Inigo Quilez for palette function, see https://www.shadertoy.com/view/ll2GD3
vec3 Palette( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

float dot2( in vec2 v ) { return dot(v,v); }

float sdHeart(in vec2 p)
{
    p.x = abs(p.x);

    if( p.y+p.x>1.0 )
        return sqrt(dot2(p-vec2(0.25,0.75))) - sqrt(2.0)/4.0;
    return sqrt(min(dot2(p-vec2(0.00,1.00)),
                    dot2(p-0.5*max(p.x+p.y,0.0)))) * sign(p.x-p.y);
}


/*

# Phyton script
#
# The Poisson equation can be solved with iterative Jacobi,  as explained in https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu
# But on Shadertoy it's not possible use Jacobi that way, due it's not possible read and write to a texture more times for frame.
# Anyway it's possible precompute the coefficient of a two filters that summed "pre-compute" the twenty steps.

import numpy as np
import seaborn as sns; sns.set()
from scipy import signal
import sys
from sympy import *
from math import *

dx = 1
dt = 1/60

# This parameters are to precompute the filter for diffusion, not used in the shader
#diffuse velocity
#viscosity = 0.1
#alpha = (dx)*(dx) / (viscosity * dt)
#rBeta = 1/(alpha+4)

#Coefficients for Poisson equation. See https://developer.nvidia.com/gpugems/gpugems/part-vi-beyond-triangles/chapter-38-fast-fluid-dynamics-simulation-gpu
alpha = -(dx)*(dx);
rBeta = 1/4

       
def PrintMatrix2D(M):
    sizeX = shape(M)[0]
    sizeY = shape(M)[1]
    
    for i in range(0,sizeX):
        for j in range(0,sizeY):
          #print('(' + str(simplify(M[i,j])) + ') ')
          print('(' + str(simplify(M[i,j])) + ') ', end='')
        print()

def PrintMatrixShader(M):
    sizeX = shape(M)[0]
    sizeY = shape(M)[1]
    
    ci = floor((sizeX)/2)
    cj = floor((sizeX)/2)
    
    for i in range(0,sizeX):
        for j in range(0,sizeY):
            v = simplify(M[i,j])
            if(v != 0):
                print('v += ' + str(v) + '*U(x+vec2(' + str(i-ci) + ',' + str(j-cj) + '));')

def Jacobi20Step(X, P, n_iter):

    XNEW = X.copy()
    TMP = zeros(shape(X)[0],shape(X)[1])
    for n in range(0,n_iter):
        for i in range(0,shape(X)[0]):
            for j in range(0,shape(X)[1]):
                xT = XNEW[i-1,j] if (i-1>=0 and i-1<shape(X)[0]) else 0;
                xB = XNEW[i+1,j] if (i+1>=0 and i+1<shape(X)[0]) else 0;
                xL = XNEW[i,j-1] if (j-1>=0 and j-1<shape(X)[1]) else 0;
                xR = XNEW[i,j+1] if (j+1>=0 and j+1<shape(X)[1]) else 0;
                p =  XNEW[i,j]*alpha
                TMP[i,j] = (xL + xR + xT + xB + p)*rBeta
        XNEW = TMP.copy();
    return XNEW

n = 20
k = 43

ci = floor((k)/2)
cj = floor((k)/2)

# Generate the first matrix, the ones that apply to the divergence buffer
U = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        U[i,j] = 0
        
P = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        P[i,j] = 0

U[ci,cj] = 1
P[ci,cj] = 0

XU = Jacobi20Step(U, P, n)
PrintMatrixShader(XU)
print()

# Generate the second matrix, the ones that apply to the pressure buffer
U = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        U[i,j] = 0
        
P = zeros(k,k)
for i in range(0, k):
    for j in range(0, k):
        P[i,j] = 0

U[ci,cj] = 0
P[ci,cj] = 1

XU = Jacobi20Step(U, P, n)
PrintMatrixShader(XU)
print()
*/