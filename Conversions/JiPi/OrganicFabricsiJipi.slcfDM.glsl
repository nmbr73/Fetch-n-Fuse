

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = pow( texture(iChannel0, fragCoord/iResolution.xy),vec4(0.8));
    // try this different appearance 
    //fragColor = 1.-pow( texture(iChannel0, fragCoord/iResolution.xy),vec4(2.2));
    
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

const float PHI = 1.61803398874989484820459; // Φ = Golden Ratio 


const vec2 startPos = vec2(0.5,0.5);
const float TreeThickness = 3.;
const float speed =64.*0.016;
const float TreeSpread = 0.1;
const int BrancSpacing = 0;
const float TurnSpeed = 0.2;
const int MinBrancingCondition = 1;
const float InitialBranches = 1.;
const float BrancingProbability = 0.94;


const vec4 BrightColor1 = vec4(0.820,0.906,0.773,1.);
const vec4 DarkColor1 = vec4(0.392,0.725,0.631,1.);


const vec4 BrightColor2 = vec4(0.686,0.655,0.843,1.);   
const vec4 DarkColor2 = vec4(0.439,0.400,0.220,1.);
const vec4 BgColor1 = vec4(0.204,0.212,0.294,0.1);
const vec4 BgColor2 = vec4(0.310,0.490,0.631,0.1);
    

float gold_noise(in vec2 xy, in float seed)
{
    return fract(tan(distance(xy*PHI, xy)*seed)*xy.x);
}
vec2 randomDir2(vec2 uv, float seed )
{
    float x = gold_noise(uv, seed+1.0);
    float y = gold_noise(uv, seed-2.0);
    return normalize(vec2(x,y)-0.5);
}


vec2 quantizevec2( vec2 o )
{
    float angle = atan(o.y, o.x);
    float minRad = 3.1415926*0.25;
    angle = floor(angle/minRad)*minRad;
    return vec2(cos(angle), sin(angle));
}

vec2 rotvec2(vec2 o, float rad)
{
    return mat2x2(vec2(cos(rad), -sin(rad)),vec2(sin(rad),cos(rad))) * o;
}




vec3 pal( in float t, in vec3 a, in vec3 b, in vec3 c, in vec3 d )
{
    return a + b*cos( 6.28318*(c*t+d) );
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
int sense( vec2 pos, int range)
{
    pos /= 2.;
    int count  = 0;
    for ( int i = -range; i <=range; i++)
    {
        for (int j = -range; j <=range; j++)
        {
            vec4 data = texelFetch( iChannel2, ivec2(pos)+ivec2(i,j), 0);
            //if ( length(data.zw) > 0. )
                count +=int(data.x);
        }
    }
    return count;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = vec4(0.0,0.0,0.0,0.0);
    vec2 uv = fragCoord / iResolution.y;

    if ( iFrame == 0)
    {
     
         vec2 center =  iResolution.xy*0.5/iResolution.y;
         vec2 p = fragCoord/iResolution.y;
         vec2 dir = vec2(p - center)*50.;
         float l = length(dir);
         dir = normalize(dir);
        fragColor = vec4(fragCoord,vec2(dir.x, dir.y) )* step(l, 1.0);
    }
    else 
    {
    
   
        int count = 0;
        int range = BrancSpacing;

        for ( int i = -2; i <=2; i++)
        {
            for (int j = -2; j <=2; j++)
            {
                
                vec4 dataOld = texelFetch( iChannel0, ivec2(fragCoord)+ivec2(i,j), 0);
                vec2 previousPos = dataOld.xy + dataOld.zw*-speed;
                vec4 dataNew = dataOld;
                dataNew.xy += dataNew.zw*speed;
                

                if ( length(dataNew.zw) > 0. && int(fragCoord.x) == int(dataNew.x) && int(fragCoord.y) == int(dataNew.y))
                {
                    // randomly nudge the dir
                    dataNew.zw = normalize(dataNew.zw + randomDir2(fragCoord,iTime)*TreeSpread);

                    fragColor = dataNew;
                }
                float bp = BrancingProbability ;


                if ( int(previousPos.x) == int(fragCoord.x) && int(previousPos.y) == int(fragCoord.y)  && gold_noise(fragCoord,iTime+4.3) > bp )
                {
                    fragColor = dataOld;

                }
            }
        }
        
        if (length(fragColor.zw) > 0.)
        {
            // new dir based on sense
            int senseRange = 5;
            float senseDist = float(senseRange)*4.+2.;
            vec2 turnL = rotvec2(fragColor.zw, 3.1415/3.);
            vec2 turnR = rotvec2(fragColor.zw, -3.1415/3.);
            int fc = sense(fragCoord+fragColor.zw*senseDist, senseRange);
            int lc = sense(fragCoord+turnL*senseDist, senseRange);
            int rc = sense(fragCoord+turnR*senseDist, senseRange);

            if (fc >= lc && fc >= rc )
            {
                //keep moving
            }
            else if ( lc > rc )
            {
                fragColor.zw = normalize(fragColor.zw + turnL*TurnSpeed);
            }
            else
                fragColor.zw = normalize(fragColor.zw + turnR*TurnSpeed);
        }  
        
        if (length (fragCoord - iMouse.zw) < 5. && length(iMouse.zw )> 1.)
        {
            vec2 dir = normalize(fragCoord - iMouse.xy);
            fragColor = vec4(fragCoord,vec2(dir.x, dir.y) );
        }
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    float scale = 1.;//+12.*((sin(iTime*5.)+1.)*0.5);
    
    int kernelSize = int(TreeThickness);

    if (kernelSize <1)
        kernelSize = 1;

    
    
        bool found = false;
        float foundDist = 10000.;
        vec2 angle ;

        for ( int i = -kernelSize; i <=kernelSize; i++)
        {
            if (found )
                break;
            for (int j = -kernelSize; j <=kernelSize; j++)
            {
                if (j*j + i*i < kernelSize*kernelSize )
                {
                    if (length(texture(iChannel0, (fragCoord+vec2(float(i),float(j)))/iResolution.xy).zw ) > 0.)
                    {
                        found = true;
                        float dist = length(vec2(float(i),float(j)));
                        if ( dist < foundDist )
                            foundDist = dist;
                        angle = normalize(vec2(float(i),float(j)));

                        break;
                    }
                }
                
            }
        }

        vec4 oldColor = pow(texture(iChannel1, fragCoord/iResolution.xy),vec4(1.011));
        
        if (found)
        {
            float a = 0.5*(dot(angle, normalize(vec2(1.,0.))) + 1.)*smoothstep(0.,float(kernelSize),foundDist);
            a = clamp(a,0.,1.);
            
            float ga = 0.0;
            vec4 darkColor =  pal( 59./30., vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.10,0.20) ).xyzz;//mix(DarkColor1, DarkColor2, ga);
            vec4 brightColor = pal( 59./30., vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,0.7,0.4),vec3(0.0,0.15,0.20) ).xyzz;// mix(BrightColor1, BrightColor2, ga);
             
            float blendIn = smoothstep(float(kernelSize),0.,foundDist);
            blendIn = clamp(blendIn, 0.,1.);
            fragColor = mix(oldColor,mix(darkColor, brightColor,a),blendIn );
        }
        else
            fragColor = oldColor;
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragCoord *=2.;
    fragColor.x = sign(length(texelFetch(iChannel0, ivec2(fragCoord) + ivec2(0,0),0).zw));
    fragColor.x += sign(length(texelFetch(iChannel0, ivec2(fragCoord) + ivec2(0,1),0).zw));
    fragColor.x += sign(length(texelFetch(iChannel0, ivec2(fragCoord) + ivec2(1,0),0).zw));
    fragColor.x += sign(length(texelFetch(iChannel0, ivec2(fragCoord) + ivec2(1,1),0).zw));
    
    
}