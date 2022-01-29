

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Macroscopic LBM Shallow Water by Nico Ell
// Contact: nico@nicoell.dev


// ----------------------------------------------------------
// Triangulator by nimitz (twitter: @stormoid)
// https://www.shadertoy.com/view/lllGRr
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
// Contact the author for other licensing options

#define ITR 50
#define FAR 1000.
#define BASECOLOR vec3(0.05,0.1,0.85)

mat2 mm2(in float a){float c = cos(a), s = sin(a);return mat2(c,-s,s,c);}
mat2 m2 = mat2(0.934, 0.358, -0.358, 0.934);
float tri(in float x){return abs(fract(x)-0.5);}

float heightmap(in vec2 p)
{
    vec2 uv = (p.xy * .005);
    uv.y += .45;
    uv.x -= 0.5;
    float waterDepth = texture(iChannel0, uv).z;
    float terrainHeight = texture(iChannel0, uv).w;
    if (ReadKey(iChannel3, kToggleWater, true))
    {
        return terrainHeight;
    } else
    {
        return (terrainHeight + waterDepth);
    }
}

//from jessifin (https://www.shadertoy.com/view/lslXDf)
vec3 bary(vec2 a, vec2 b, vec2 c, vec2 p) 
{
    vec2 v0 = b - a, v1 = c - a, v2 = p - a;
    float inv_denom = 1.0 / (v0.x * v1.y - v1.x * v0.y)+1e-9;
    float v = (v2.x * v1.y - v1.x * v2.y) * inv_denom;
    float w = (v0.x * v2.y - v2.x * v0.y) * inv_denom;
    float u = 1.0 - v - w;
    return abs(vec3(u,v,w));
}

/*
	Idea is quite simple, find which (triangular) side of a given tile we're in,
	then get 3 samples and compute height using barycentric coordinates.
*/
float map(vec3 p)
{
    vec3 q = fract(p)-0.5;
    vec3 iq = floor(p);
    vec2 p1 = vec2(iq.x-.5, iq.z+.5);
    vec2 p2 = vec2(iq.x+.5, iq.z-.5);
    
    float d1 = heightmap(p1);
    float d2 = heightmap(p2);
    
    float sw = sign(q.x+q.z); 
    vec2 px = vec2(iq.x+.5*sw, iq.z+.5*sw);
    float dx = heightmap(px);
    vec3 bar = bary(vec2(.5*sw,.5*sw),vec2(-.5,.5),vec2(.5,-.5), q.xz);
    return (bar.x*dx + bar.y*d1 + bar.z*d2 + p.y + 3.)*.9;
}

float march(in vec3 ro, in vec3 rd)
{
	float precis = 0.001;
    float h=precis*2.0;
    float d = 0.;
    for( int i=0; i<ITR; i++ )
    {
        if( abs(h)<precis || d>FAR ) break;
        d += h;
	    float res = map(ro+rd*d)*.1;
        h = res;
    }
	return d;
}

vec3 normal(const in vec3 p)
{  
    vec2 e = vec2(-1., 1.)*0.01;
	return normalize(e.yxx*map(p + e.yxx) + e.xxy*map(p + e.xxy) + 
					 e.xyx*map(p + e.xyx) + e.yyy*map(p + e.yyy) );   
}
// ----------------------------------------------------------

// From https://www.shadertoy.com/view/Xdy3zG
//fancy function to compute a color from the velocity
vec4 computeColor(float normal_value)
{
    vec3 color;
    if(normal_value<0.0) normal_value = 0.0;
    if(normal_value>1.0) normal_value = 1.0;
    float v1 = 0.01/7.0;
    float v2 = 2.0/7.0;
    float v3 = 3.0/7.0;
    float v4 = 4.0/7.0;
    float v5 = 5.0/7.0;
    float v6 = 6.0/7.0;
    //compute color
    if(normal_value<v1)
    {
      float c = normal_value/v1;
      color.x = 140.*(1.-c);
      color.y = 70.*(1.-c);
      color.z = 19.*(1.-c) + 91.*c;
    }
    else if(normal_value<v2)
    {
      float c = (normal_value-v1)/(v2-v1);
      color.x = 0.;
      color.y = 255.*c;
      color.z = 91.*(1.-c) + 255.*c;
    }
    else if(normal_value<v3)
    {
      float c = (normal_value-v2)/(v3-v2);
      color.x =  0.*c;
      color.y = 255.*(1.-c) + 128.*c;
      color.z = 255.*(1.-c) + 0.*c;
    }
    else if(normal_value<v4)
    {
      float c = (normal_value-v3)/(v4-v3);
      color.x = 255.*c;
      color.y = 128.*(1.-c) + 255.*c;
      color.z = 0.;
    }
    else if(normal_value<v5)
    {
      float c = (normal_value-v4)/(v5-v4);
      color.x = 255.*(1.-c) + 255.*c;
      color.y = 255.*(1.-c) + 96.*c;
      color.z = 0.;
    }
    else if(normal_value<v6)
    {
      float c = (normal_value-v5)/(v6-v5);
      color.x = 255.*(1.-c) + 107.*c;
      color.y = 96.*(1.-c);
      color.z = 0.;
    }
    else
    {
      float c = (normal_value-v6)/(1.-v6);
      color.x = 107.*(1.-c) + 223.*c;
      color.y = 77.*c;
      color.z = 77.*c;
    }
    return vec4(color.r/255.0,color.g/255.0,color.b/255.0,1.0);
}

// ----------------------------------------------------------
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec4 velocityHeight = texture(iChannel0, uv);

    vec4 mixedColor;
    if (!ReadKey(iChannel3, kViewMode, true))
    {
        if (ReadKey(iChannel3, kToggleWater, true))
        {
            mixedColor = vec4(computeColor((velocityHeight.w / MaxShallowWaterDepth)).xyz, 1.0);
        }
        else
        {
            mixedColor = vec4(computeColor((velocityHeight.z / MaxShallowWaterDepth)).xyz, 1.0);
        }
    }
    else
    {
        vec2 eps = vec2(0.1, 0.0);
    
        vec2 st = fragCoord.xy / iResolution.xy;
        float finv = tan(40.0 * 0.5 * pi / 180.0);
        float aspect = iResolution.x / iResolution.y;
        st.x = st.x * aspect;
        st = (st - vec2(aspect * 0.5, 0.95)) * finv;

        vec3 ro = vec3(0., 75., 0.);
        vec3 rd = normalize(vec3(st, .5));

        float rz = march(ro,rd);
        vec3 col = vec3(0.);

        if ( rz < FAR ) 
        {
            vec3 pos = ro+rz*rd;
            vec3 nor= normal(pos);
            vec3 ligt = normalize(vec3(-.2, 0.05, -0.2));

            float dif = clamp(dot( nor, ligt ), 0., 1.);
            float fre = pow(clamp(1.0+dot(nor,rd),0.0,1.0), 3.);
            vec3 brdf = 2.*vec3(0.10,0.11,0.1);
            brdf += 1.9*dif*vec3(.8,1.,.05);
            col = BASECOLOR;
            col = col*brdf + fre*0.5*vec3(.7,.8,1.);
        }
        col = clamp(col,0.,1.);
        col = pow(col,vec3(.9));
        col *= pow( 16.0*uv.x*uv.y*(1.0-uv.x)*(1.0-uv.y), 0.1);
        
        mixedColor = vec4(col, 1.0);
    }


    fragColor = mixedColor;
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 GetWind(vec2 uv)
{
	return sin(iTime) * WindAmplitude *	vec2(-cos(uv.y * pi * 4.0), 0);
}

float GetTerrainHeight(vec2 uv)
{
	float h = (0.4 * max(cos(uv.x * pitwo), cos(uv.y * pitwo)) + 0.6);
	return h * TerrainHeightScale;
}

//Inigo Quilez; https://www.iquilezles.org/www/articles/smin/smin.htm
float smoothmin(float a, float b, float k)
{
	float h = max(k - abs(a - b), 0.0) / k;
	return min(a, b) - h * h * k * (1.0 / 4.0);
}

void EnforceStabilityConditions(inout vec2 u, inout float h, float WaterDepthLimit, float VelocityMagnitudeLimit)
{
	/*
	* u = velocity
	* g = gravity
	* h = water depth
	* gh = squared wave speed
	* e = lattice speed (speed at which information travels) / MacroscopicParticleSpeed
	*
	* Stability Requirements:
	* |u| < sqrt(gh)
	*	gh < e^2
	* u.u < e^2 && |u| << e
	*	|u| DeltaX / Viscosity < 1
	*
	* => |u| < e/6
	*		|u| < sqrt(gh) < sqrt(e^2) (this is always larger than e/6)
	*		 h	< e^2/g
	*/
	// The factors 0.96 and 0.2 were empirically found.
	// Smoothmin prevents introducing sharp edges, which then produce ripples and waves on the surface.	
	h = smoothmin(h, 0.96 * WaterDepthLimit, 0.2 * WaterDepthLimit);
	// VelocityMagnitudeLimit should always be smaller than sqrt(Gravity * h)
	float speedLimit = VelocityMagnitudeLimit;

	float magnitudeU = length(u);
	if (magnitudeU > 0.0)
	{
		u /= magnitudeU;
		u *= min(magnitudeU, speedLimit);
	}
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{	
	// 5 6 7
	//	\|/		 // D2Q9
	// 0-C-1		// Unweighted raw vectors
	//	/|\		 // These need to be weighted with MacroscopicParticleSpeed
	// 2 3 4

	const vec2 GridVectors[8] = vec2[]
	(
		vec2(-1, 0), vec2(1, 0),
		vec2(-1, -1), vec2(0, -1), vec2(1, -1),
		vec2(-1, 1), vec2(0, 1), vec2(1, 1)
	);
	const vec2 ForceWeights = vec2(1.0 / 3.0, 1.0 / 12.0);
	
	// ----------------------------------------------------------
	// Dynamic derived parameters
	// ----------------------------------------------------------
	float MinEddyViscosity = sqrt(Gravity * MaxShallowWaterDepth)	* MaxShallowWaterDepth / 6.0;
	MinEddyViscosity *= ViscosityModifier;

	float MacroscopicParticleSpeed = (6.0 * MinEddyViscosity) / LatticeSize;
	float VelocityMagnitudeLimit = (1.0 / 6.0) * MacroscopicParticleSpeed;

	float DeltaTime = LatticeSize / sqrt(Gravity * MaxShallowWaterDepth);
	
	float mpsPower = MacroscopicParticleSpeed * MacroscopicParticleSpeed;
	float WaterHeightLimit = mpsPower / Gravity;
	vec4 ParticleSpeedDenominators;
	ParticleSpeedDenominators.x = 1.0 / (6.0 * mpsPower);
	ParticleSpeedDenominators.y = 1.0 / (3.0 * mpsPower);
	ParticleSpeedDenominators.z = 1.0 / (2.0 * mpsPower * mpsPower);
	ParticleSpeedDenominators.w = 1.0 / mpsPower;
	// ----------------------------------------------------------

	vec2 inverseTextureSize = vec2(1.0) / iResolution.xy;
	vec2 uv = fragCoord * inverseTextureSize;
    
    // ----------------------------------------------------------
	// Early out for Reset/Init Simulation
	// ----------------------------------------------------------
    if(ReadKey(iChannel3, kResetSimulation, false) || iFrame == 0)
	{
		vec2 inverseTextureSize = vec2(1.0) / iResolution.xy;
		vec2 uv = fragCoord * inverseTextureSize;

		vec2 initVelocity = vec2(VelocityMagnitudeLimit, 0);
		float initHeight = RelInitialWaterDepth * MaxShallowWaterDepth;
        float terrainHeight = GetTerrainHeight(uv);

		fragColor = vec4(initVelocity, max(initHeight - terrainHeight, 0.0), terrainHeight);
		return;
	}

	// ----------------------------------------------------------
	// Simulated Outputs
	// ----------------------------------------------------------
	vec2 newVelocity = vec2(0.0, 0.0);
	float newHeight = 0.;
	
	// ----------------------------------------------------------
	// Other & Intermediates
	// ----------------------------------------------------------
	float hCenterHalf;
	float terrainElevationCenter = GetTerrainHeight(uv);

	vec2 externalForce = vec2(0.0, 0.0);
	float heightInjection = 0.0;
	
	externalForce += GetWind(uv);

	// User Force Input
	if (ReadKey(iChannel3, kMouseInputType, true))
	{
		if( iMouse.z>0.1 && distance(iMouse.xy,fragCoord) < MouseRadius)
		{
			externalForce += MouseForce;
		}
	}
	else 
	{
		if (iMouse.z>0.1 && distance(iMouse.xy,fragCoord) < MouseRadius)
		{
			heightInjection = MouseHeightInjection;
		}
	}
	
	// ----------------------------------------------------------
	// First Handle hCenter
	// ---------------------------------------------------------- 
	{
		float h = texture(iChannel0, uv).z + heightInjection;
		vec2 u = texture(iChannel0, uv).xy;

		hCenterHalf = h * 0.5;

		float localEquilibrium = -5.0 * Gravity * h * ParticleSpeedDenominators.x;
		localEquilibrium -= 2.0 * dot(u, u) * ParticleSpeedDenominators.y;
		localEquilibrium = localEquilibrium * h + h;

		newHeight = localEquilibrium;
	}
	
			
	// ----------------------------------------------------------
	// Neighbours
	// ----------------------------------------------------------
	for (int i = 0; i < 8; i++)
	{
		vec2 uvNeighbour = uv - GridVectors[i] * inverseTextureSize;
		float terrainElevation = GetTerrainHeight(uvNeighbour);
		float h = texture(iChannel0, uvNeighbour).z + heightInjection;
		vec2 u = texture(iChannel0, uvNeighbour).xy;

		vec2 particleVelocityVector = -GridVectors[i] * MacroscopicParticleSpeed;

		vec2 bedShearStress = BedFrictionCoefficient * u * length(u);
		externalForce -= bedShearStress;

		float doteu = dot(particleVelocityVector, u);
		float localEquilibrium = 0.0;

		if (i == 0 || i == 1 || i == 3 || i == 6) // Direct Neighbours
		{
			localEquilibrium = ParticleSpeedDenominators.x * (Gravity * h - dot(u, u));
			localEquilibrium += doteu * (doteu * ParticleSpeedDenominators.z + ParticleSpeedDenominators.y);
			localEquilibrium *= h;
		} else // Diagonal Neighbours
		{
			localEquilibrium = ParticleSpeedDenominators.x * (Gravity * h - dot(u, u));
			localEquilibrium += doteu * (doteu * ParticleSpeedDenominators.z + ParticleSpeedDenominators.y);
			localEquilibrium *= 0.25 * h;
		}
		
		// Terrain Bed influence
		float actingForce = 0.;
		float hOverline = 0.5 * h + hCenterHalf;
		float terrainSlope = terrainElevation - terrainElevationCenter;

		 // (Gravity * hOverline *	ParticleSpeedDenominators.w) is hOverline / ShallowWaterVolumeHeight and therefore the relative height 0...1
		float relativeShallowWaterHeight = (Gravity * hOverline * ParticleSpeedDenominators.w);
		actingForce += relativeShallowWaterHeight * terrainSlope;

		actingForce += (DeltaTime * ParticleSpeedDenominators.w) * dot(particleVelocityVector, externalForce);
		if (i == 0 || i == 1 || i == 3 || i == 6) // Direct Neighbours
		{
			actingForce *= ForceWeights.x;
		}
		else
		{
			actingForce *= ForceWeights.y;
		}

		float hi = localEquilibrium + actingForce;
		newHeight += hi;
		newVelocity += particleVelocityVector * hi;
	}

	if (newHeight > 0.0)
	{
		newVelocity /= newHeight;
	}
	else
	{
		newVelocity = vec2(0.0, 0.0);
		newHeight = 0.0;
	}

	EnforceStabilityConditions(newVelocity, newHeight, WaterHeightLimit, VelocityMagnitudeLimit);
	

	fragColor = vec4(newVelocity.xy, newHeight, terrainElevationCenter);
}

// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define pi 3.141
#define pitwo 6.28318

#define Gravity 9.81
// LatticeSize at which Simulation is performed
#define LatticeSize 5.5
// Maximum Depth of Water in Meter
#define MaxShallowWaterDepth 5.0
#define RelInitialWaterDepth 0.5
// Guaranteed stable at 1.0
#define ViscosityModifier 1.
#define BedFrictionCoefficient 0.0
#define TerrainHeightScale 5.
#define WindAmplitude .0

// User Input
#define MouseRadius 25.0
#define MouseForce vec2(10., 0)
#define MouseHeightInjection .1

// Keys
const int kA=65,kB=66,kC=67,kD=68,kE=69,kF=70,kG=71,kH=72,kI=73,kJ=74,kK=75,kL=76,kM=77,kN=78,kO=79,kP=80,kQ=81,kR=82,kS=83,kT=84,kU=85,kV=86,kW=87,kX=88,kY=89,kZ=90;
const int k0=48,k1=49,k2=50,k3=51,k4=52,k5=53,k6=54,k7=55,k8=56,k9=57;
const int kSpace=32,kLeft=37,kUp=38,kRight=39,kDown=40;

const int kViewMode = kSpace;
const int kMouseInputType = kQ;
const int kResetSimulation = kR;
const int kToggleWater = kW;

bool ReadKey(sampler2D textureChannel, int key, bool toggle )
{
	float keyVal = texture( textureChannel, vec2( (float(key)+.5)/256.0, toggle?.75:.25 ) ).x;
	return (keyVal>.5)?true:false;
}
