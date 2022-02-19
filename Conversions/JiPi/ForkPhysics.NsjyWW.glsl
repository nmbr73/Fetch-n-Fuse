

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
How it works:

Buf A find collision points. World space edges of a cube are clamped with the planes of the 
other cube. Then the other way around, giving a wireframe of the intersecting parts.
Both line ends will count as a collision, this gives 48 possible collision points per cube pair.
12x4 pixels are allocated for each pair (i>j)

Buf A on the right side of it  stores a separating plane between each pair. While colliding, 
there is no separating plane, but we need the plane with the least reach through. 
As the best plane is usually a cross product of two edges colliding, or a plane of one of the cube,
which collides with a vertex. That counts 30 posibilities per pair, these are stored in 
separate pixels. The offset of the plane right in the middle of the intersection is stored into w. 
The penetration level is stored as the length of the .xyz vector (defining a plane),
which normally has a length of 1.0, but there is no more component left.

Buf B then selects the best separating plane from Buf A by comparing the length(.xyz) of the 
30 texels, and stores it normalized in the right side of buf B

Buf B in the left side stores updated velocity and rotational velocity after applying
the forces stored in Buf C from last frame. The idea is to use the forces from last frame
as an initial guess for solving the constraints, if they already collided last frame. 
This is verlet integration.

Buf C calculates the forces that would relax the velocity difference at the collision points.
(after having reduced those, by applying forces from last frame in buf B)
It clamps the forces into a friction cone including the force's component, that was already 
added in Buf B. 

Buf D adds the forces stored in Buf C to the velocity and rotational velocity from last frame
(not using the updated velocities in buf B, instead reading buf D) 
It also adds a repulsive force to move the cubes out of penetration.
Buf B has the best separating plane including it's offset, which is used to estimate 
the penetration at a collision point.
Then it updates the translation of the cubes.
The actual rotation is stored as a quaternion. It gets converted to 3 perpendicular
vectors and then back to quaternion after rotating them by the angular velocity (called rotvel)
Velocity is added to position. Gravity is added to velocity after the translation update.

Image: No ray marching



*/

#define CUBECOUNT 16
#define PI 3.141592653

vec4 readTex3(int cx,int cy)
{
    return texture(iChannel3,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex2(int cx,int cy)
{
    return texture(iChannel2,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec4 readTex0(int cx,int cy)
{
    return texture(iChannel0,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec3 getCubePos(int ci)
{
    return readTex3(ci,0).xyz;
}
vec4 getCubeQuat(int ci)
{
    return readTex3(ci,1).xyzw;
}

vec3 getCollision(int ci,int cj,int k) // for debugging
{
    return readTex0(ci*12+k%12,cj*4+k/12).xyz;
}


vec3 rotate(vec4 quat,vec3 v)
{
    float sinsqr = (1.0-quat.w*quat.w);
    if (sinsqr!=0.0)
    {
        v=v*quat.w + quat.xyz*((dot(v,quat.xyz)*(1.0-quat.w))*(1.0/sinsqr)) + cross(v,quat.xyz);
        v=v*quat.w + quat.xyz*((dot(v,quat.xyz)*(1.0-quat.w))*(1.0/sinsqr)) + cross(v,quat.xyz);
    }
    return v;
}

vec3 rotateAxis(vec3 axis,vec3 v)
{
    float len = length(axis);
    if (len!=0.0)
    {
        axis = normalize(axis);
		v = v*cos(len) + axis*((v*axis) * (1.0-cos(len))) + cross(v,axis)*sin(len);
    }
    return v;
}


vec4 getCubePlane(int ci,int k)
{
    vec3 norm = vec3( k/2%3==0?1.0:0.0, k/2%3==1?1.0:0.0, k/2%3==2?2.0:0.0)*float(k%2*2-1);
    norm = rotate(getCubeQuat(ci),norm);
    float offset = 1.0 + dot(getCubePos(ci),norm);
    return vec4(norm,offset);
}


vec3 cubeTransform(int ci,vec3 lp)
{
    return getCubePos(ci) + rotate(getCubeQuat(ci),lp);
}

vec3 rayPos,rayDir;
float minDist;
vec3 minDistNormal;
float minDistMaterial;

vec3 sunDir = normalize(vec3(0.8,1.0,-0.3));

void renderCube(int ci)
{
    float backMin = 1e30;
    float frontMax = -1e30;
    vec3 frontNormal = vec3(0.0,0.0,0.0);
    
    for(int side=0;side<6;side++)
    {
        vec4 plane = getCubePlane(ci,side);
        
        float rayTravel = -(dot(rayPos,plane.xyz)-plane.w)/dot(plane.xyz,rayDir);
        if (  dot(plane.xyz,rayDir)<0.0)
        {
            if (frontMax < rayTravel)
            {
                frontMax = rayTravel;
                frontNormal = plane.xyz;
            }
        }
        else
        {
            if (backMin > rayTravel)
            {
                backMin = rayTravel;
            }
        }
    }
    if (frontMax<backMin) // cube hit
    {
        if (minDist>frontMax && frontMax>0.0) // chose closeset cube
        {
            minDist = frontMax;
            minDistNormal = frontNormal;
//            minDistMaterial = 1.0-fract(float(ci)/float(CUBECOUNT)*94.);
            minDistMaterial = fract(float(ci)/3.0)+0.1;
        }
    }
    
}


vec3 backGround(vec3 dir,vec3 pos)
{
	float f = max(dir.y,0.0)*0.5+0.5;
	vec3 color = 1.0-vec3(1,0.85,0.7)*f;
	color *= dir.x*-0.3+1.0;
	
	if (dot(sunDir,dir)>0.0) // sun reflected on cubes
	{
	 f = max(length(cross(sunDir,dir))*10.0,1.0);
		
	 color += vec3(1,0.9,0.7)*40.0/(f*f*f*f);
	}
	return color;
}

vec4 debugColor;
float debugthickness = 0.02;
void debugline(vec3 pa,vec3 pb)
{
    vec3 pl = normalize(cross(cross(pa-pb,rayDir),pa-pb));
    float d = -dot(pl,rayPos-pa)/dot(pl,rayDir);
    vec3 p = rayPos+rayDir*d;
    if (dot(p-pb,pa-pb)>0.0 && dot(p-pb,pa-pb)<dot(pa-pb,pa-pb) && length(cross(p-pa,normalize(pa-pb)))<debugthickness )
    {
        debugColor = vec4(0.9,0.8,0.1,1.0);
    }
}

void debugdot(vec3 p)
{
    float d = pow(length(cross(p-rayPos,rayDir))/debugthickness,4.0);
    debugColor = mix(debugColor,vec4(0.9,0.8,0.1,1.0),max(1.0-d,0.0));
}


void debugCollision(int ci,int cj)
{
    vec4 sep = readTex0(ci+CUBECOUNT*12,cj);
    for(int k=0;k<48;k++)
    {
        vec3 cpos = getCollision(ci,cj,k);
        debugdot(cpos);
    	debugline(cpos,cpos + readTex2(ci*12+k%12,cj*4+k/12).xyz*10.0 );
    }
}

void renderScene()
{
    minDistMaterial = 0.0;
    minDist = 1e30;
    minDistNormal = vec3(0.0,1.0,0.0);
    
    if (rayDir.y<0.0)
    {
        minDist = rayPos.y/-rayDir.y;
    }
    
    for(int i=1;i<(CUBECOUNT);i++)
    {
        if ( length(cross(getCubePos(i)-rayPos,rayDir))<3.0)
        {
    		renderCube(i);
        }
    }
}

vec3 getDiffuse()
{
        vec3 difColor;
        if (minDistMaterial==0.) difColor = texture(iChannel1,rayPos.zx/8.0).xyz; // floor
        else
        {
            difColor = vec3(1.0,1.0-minDistMaterial,0.3-0.3*minDistMaterial); // cube colors
        }
    return difColor;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    debugColor = vec4(0.0,0.0,0.0,0.0);
	vec3 campos = vec3(0.0-fract(float(iFrame)/(60.*8.))*4.0,6.4-sin(iTime/4.0)*0.5,-17.0);
	vec2 uv = fragCoord.xy / iResolution.xy;
	vec3 pdir = vec3(uv*2.0-1.0,2.);
	pdir.y /= iResolution.x/iResolution.y; // wide screen
    pdir = normalize(pdir);
    
    pdir = rotateAxis( vec3(-0.27,0.,0.), pdir);

    rayPos = campos;
    rayDir = pdir;

    renderScene();
//        debugCubeEdges(i);
//	    debugdot(getCubePos(i));
//        for(int j=0;j<8;j++) debugdot(getWCubeVert(i,j));
    
 
//  debugCollision(1,0);
//    debugline(getCubePos(1),g    
    
    
    
    if (minDist<1e30)
    {
        minDistNormal = normalize(minDistNormal);
        const vec3 sunColor = vec3(1.0,0.8,0.5)*1.0;
        const vec3 skyColor = vec3(1.0,1.2,1.5)*0.6;
        rayPos += rayDir*minDist;
        vec3 firstHitPos = rayPos;
        vec3 refdir = reflect(rayDir,minDistNormal);
        float f = 1.-max(dot(minDistNormal,-rayDir),0.);
        float fresnel = 0.65*f*f*f*f*f+0.05;

        vec3 difColor =getDiffuse();
        
        fragColor = vec4(difColor*skyColor*((minDistNormal).y*0.5+0.5),0.);
        float suncos = dot((minDistNormal),sunDir);
        if (suncos>0.0)
        {
            // spot sun light pointing on the thrown cube.
            vec3 v = cross(sunDir,rayPos-getCubePos(CUBECOUNT-1))/20.;
            suncos *= max(0.,1.0-dot(v,v));
            rayDir = sunDir;
    	    renderScene();
	        if (minDist==1e30) 
            {
                fragColor.xyz += difColor * suncos * sunColor;
            }
        }
        
        rayPos = firstHitPos;
        rayDir = refdir;
        renderScene();
        
        vec3 refColor;
        if (minDist<1e30)
        {
            rayPos += rayDir * minDist;
            vec3 difColor = getDiffuse();
            
            refColor = difColor*(normalize(minDistNormal).y*0.5+0.5);
        }
        else
        {
            refColor = backGround(rayDir,rayPos);
        }
        fragColor.xyz = mix(fragColor.xyz,refColor,fresnel);
    }
    else
    {
		fragColor = vec4(backGround(pdir,campos),0.0);
    }
    fragColor.xyz = mix(fragColor.xyz,debugColor.xyz,debugColor.w);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
#define CUBECOUNT 16u


vec4 readTex3(uint cx,uint cy)
{
    return texture(iChannel3,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec3 getCubePos(uint ci)
{
    return readTex3(ci,0u).xyz;
}
vec4 getCubeQuat(uint ci)
{
    return readTex3(ci,1u).xyzw;
}
vec3 rotate(vec4 quat,vec3 v)
{
    float sinsqr = (1.0-quat.w*quat.w);
    if (sinsqr!=0.0)
    {
        v=v*quat.w + quat.xyz*((dot(v,quat.xyz)*(1.0-quat.w))*(1.0/sinsqr)) + cross(v,quat.xyz);
        v=v*quat.w + quat.xyz*((dot(v,quat.xyz)*(1.0-quat.w))*(1.0/sinsqr)) + cross(v,quat.xyz);
    }
    return v;
}
vec3 cubeTransform(uint ci,vec3 lp)
{
    lp.z*=0.5;
    return getCubePos(ci) + rotate(getCubeQuat(ci),lp);
}

vec4 getCubePlane(uint ci,uint k)
{
    vec3 norm = vec3( k%3u==1u?1.0:0.0, k%3u==0u?1.0:0.0, k%3u==2u?2.0:0.0)*(float(k/3u)*-2.0+1.0);
    norm = rotate(getCubeQuat(ci),norm);
    float offset = 1.0 + dot(getCubePos(ci),norm);
    return vec4(norm,offset);
}


vec3 getWCubeVert(uint ci,uint j)
{
    return cubeTransform(ci,vec3(float(j&1u),float((j&2u)/2u),float((j&4u)/4u))*2.0-1.0);
}
vec3 rotateAxisAngle(vec3 axis,float angle,vec3 v)
{
	v = v*cos(angle) + axis*((v*axis) * (1.0-cos(angle))) + cross(v,axis)*sin(angle);
    return v;
}

uint pixelx,pixely;

vec4 findSeparatingPlane_planA()
{

    uint cia = pixelx/6u;
    uint cib = pixely/5u;
    
    if (cia>=CUBECOUNT) discard;
    if (cia<=cib) discard;
    
    
    float bestoffset=-1e30;
    vec3 bestplane;
    float bestplaneoffset;
    
//    for(uint m=0;m<2;m++)
//    for(uint k=0;k<30;k++)
    uint k = (pixelx%6u)+(pixely%5u)*6u;
    {
        vec3 sep;
        vec3 edgea = vec3( k%3u==0u?1.0:0.0, k%3u==1u?1.0:0.0, k%3u==2u?1.0:0.0);
        edgea = rotate(getCubeQuat(cia),edgea);
        vec3 edgeb = vec3( k/3u%3u==0u?1.0:0.0, k/3u%3u==1u?1.0:0.0, k/3u%3u==2u?1.0:0.0);
        edgeb = rotate(getCubeQuat(cib),edgeb);
        if (k%15u<9u)
        { 
            // edge crossings
            if (length(cross(edgea,edgeb))<0.001)
                sep = vec3(0.,0.,0.);  // parallel edges fail
            else
            	sep = normalize(cross(edgea,edgeb));
        }
        else
        {  // normals
            if (k<9u+3u)
            {
                sep = edgea;
            }
            else
            {
                sep =  vec3( k%3u==0u?1.0:0.0, k%3u==1u?1.0:0.0, k%3u==2u?1.0:0.0);
                sep = rotate(getCubeQuat(cib),sep);
            }
        }
        if (k>=15u) sep=-sep;
        
        if (cib==0u)
        {
            sep = vec3(0.,-1.,0.);
        }
        
        float minoffset = -1e30;
        for(uint j=0u;j<8u;j++)
        {
            vec3 v = getWCubeVert(cia,j);
            if (dot(v,sep)>minoffset)
            {
                minoffset = dot(v,sep);
            }
        }

        float maxoffset = 1e30;
        for(uint j=0u;j<8u;j++)
        {
            vec3 v = getWCubeVert(cib,j);
            if (dot(v,sep)<maxoffset)
            {
                maxoffset = dot(v,sep);
            }
        }
        float offset = -minoffset+maxoffset;
        
//        if (offset>bestoffset && offset!=0.0) // no improvement
        {
            bestoffset = offset;
            bestplaneoffset = (minoffset+maxoffset)*0.5;
            bestplane = sep;
        }
    }
    
    if (bestoffset>=0.0)
    {
        return vec4(0.,9999.0,0.,0.);
    }
    
    return vec4(-bestplane*(2.0-bestoffset),-bestplaneoffset);
}

// this alg isn't good, beacuse it finds some local maximum instead of the best solution, 
// but it works ok if the cubes are uintersecting, a separating plane cannot be put, 
// but the plane with the least overlap is found, which is needed in the solver.
vec4 findSeparatingPlane_planB()  
{
    if (pixelx<=pixely) discard;
    if (pixelx>=(CUBECOUNT)) discard;    

    uint cia = pixelx;
    uint cib = pixely;

    if ((length(getCubePos(cia)-getCubePos(cib))>6.0 && cib!=0u)) discard;
    
    vec3 sep = normalize(getCubePos(cib)-getCubePos(cia));
    float offset =0.0;
    float dangle = 0.2;
    float lastoffset=-1e30;
    vec3 lastsep=sep;
    vec3 diff = vec3(0.0,0.0,0.0);
    
    for(uint k=0u;k<64u;k++)
    {
        if (cib==0u)
        {
            sep = vec3(0.,-1.,0.);
        }
        
        float minoffset = -1e30;
        vec3 minvert=vec3(0.0,0.0,0.0);
        for(uint j=0u;j<8u;j++)
        {
            vec3 v = getWCubeVert(cia,j);
            if (dot(v,sep)>minoffset)
            {
                minoffset = dot(v,sep);
                minvert = v;
            }
        }

        float maxoffset = 1e30;
        vec3 maxvert=vec3(0.0,0.0,0.0);
        for(uint j=0u;j<8u;j++)
        {
            vec3 v = getWCubeVert(cib,j);
            if (dot(v,sep)<maxoffset)
            {
                maxoffset = dot(v,sep);
                maxvert = v;
            }
        }
        offset = dot(maxvert-minvert,sep);
        
        dangle*=1.2;
        if (offset<lastoffset) // no improvement
        {
            sep=lastsep;
            dangle*=0.5/1.2;
            offset = lastoffset;
        }
        else
        {
	         diff = maxvert-minvert;
        }
        
        vec3 axis = normalize(cross(diff,sep));
        lastsep = sep;
        lastoffset = offset;
        sep = rotateAxisAngle(axis,dangle,sep);
        offset = (maxoffset+minoffset)*0.5;

    }
    
    return vec4(sep,offset);
}


vec3 edge(uint i,uint j)
{
    vec3 pa,pb;
    if (i<8u)
    {
        pa.x = float(i%4u<2u?1:-1);
        pa.y = float((i+1u)%4u<2u?1:-1);
        pa.z = float(i/4u<1u?1:-1);

        pb.x = float((i+1u)%4u<2u?1:-1);
        pb.y = float((i+2u)%4u<2u?1:-1);
        pb.z = float(i/4u<1u?1:-1);
    }
    else
    {
        pa.x = float(i%4u<2u?1:-1);
        pa.y = float((i+1u)%4u<2u?1:-1);
        pa.z = -1.0;
        pb = vec3(pa.xy,1.0);
                
    }
    
    return j==0u?pa:pb;
}

vec4 findCollisionPouint()
{
    uint ci = pixelx/12u;
    uint cj = pixely/4u;
    if (cj>=ci) discard;
    
    if (length(getCubePos(ci)-getCubePos(cj))>6.0 && cj!=0u) // bounding check
    {
        return vec4(0.,0.,0.,0.);
    }
    
    uint j = pixelx%12u;
    
    if (pixely%4u<2u) // swap the two cubes to check collision both ways
    {
        uint t = ci;
        ci = cj;
        cj = t;
    }

    vec3 pa = cubeTransform(cj,edge(j,0u)); // a world space edge of cube j
    vec3 pb = cubeTransform(cj,edge(j,1u));
    float ea=0.0;
    float eb=1.0;
    for(uint l=0u;l<((ci==0u)?1u:6u);l++) // clamp it with the 6 planes of cube i
    {
        vec4 pl = getCubePlane(ci,l);
        pl/=length(pl.xyz);
        if (abs(dot(pl.xyz,pb-pa))>0.0001)
        {
            float e = -(dot(pl.xyz,pa)-pl.w)/dot(pl.xyz,pb-pa);
            if (dot(pb-pa,pl.xyz)>0.0)
            {
                eb=min(eb,e);
            }
            else
            {
                ea=max(ea,e);
            }
        }
        else
        {
            ea=999999.0; // edge is parallel to plane
        }
    }
    
    vec3 coll = pa+(pb-pa)*((pixely%2u==0u)?ea:eb);
    if (eb<=ea || cj==0u)
    {
        coll = vec3(0.,0.,0.);
    }
    
    
    return  vec4(coll,0.0);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    pixelx = uint(floor(fragCoord+0.01).x);
    pixely = uint(floor(fragCoord+0.01).y);
    
    if (pixelx>=(CUBECOUNT*12u))
    {
        pixelx-=(CUBECOUNT*12u);
        fragColor = findSeparatingPlane_planA();
        return;
    }
    
    fragColor = findCollisionPouint();   
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define CUBECOUNT 16u
#define STATIC_CUBE_COUNT 1u

float gravity = -0.003;

vec4 readTex3(uint cx,uint cy)
{
    return texture(iChannel3,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex2(uint cx,uint cy)
{
    return texture(iChannel2,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex0(uint cx,uint cy)
{
    return texture(iChannel0,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec3 getCubePos(uint ci)
{
    return readTex3(ci,0u).xyz;
}
vec4 getCubeQuat(uint ci)
{
    return readTex3(ci,1u).xyzw;
}
vec3 getCubeVel(uint ci)
{
    return readTex3(ci,2u).xyz;
}
vec3 getCubeRotVel(uint ci)
{
    return readTex3(ci,3u).xyz;
}

uint pixelx,pixely;

vec3 pos;
vec4 quat;
vec3 vel;
vec3 rotvel;

vec4 findbestsepplane()
{
    pixelx-=CUBECOUNT;
    uint cia = pixelx;
    uint cib = pixely;
    if (cia>=CUBECOUNT) discard;
    if (cib>=cia) discard;
    float best=1e30;
    vec4 bestsep;
    for(uint m=0u;m<30u;m++)
    {
        vec4 sep = readTex0(cia*6u+m%6u+CUBECOUNT*12u,cib*5u+m/6u);
        if (length(sep.xyz)<best)
        {
            best = length(sep.xyz);
            bestsep = sep;
        }
    }
    return vec4(normalize(bestsep.xyz),bestsep.w);;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    pixelx = uint(floor(fragCoord+0.01).x);
    pixely = uint(floor(fragCoord+0.01).y);
    
    fragColor = vec4(0.0,0.0,0.0,0.0);
    
    if (pixelx>=(CUBECOUNT)) 
    {
        fragColor = findbestsepplane();
        return;
    }
    if (pixely>=4u || pixely<2u) discard; // just output velocity and rotational velocity
    uint cubei = pixelx;
    
    
    pos = getCubePos(cubei);
    quat = getCubeQuat(cubei);
    vel = getCubeVel(cubei);
    rotvel = getCubeRotVel(cubei);
    
    if (cubei>=STATIC_CUBE_COUNT)
    {
    // apply forces (just the changes)
    for(uint i=0u;i<CUBECOUNT-1u;i++)
    {
        uint ci,cj;
        float scaler;
        if (i<cubei)
        {
	        ci = cubei;
            cj = i;
            scaler  = 1.0;
		 // if the other cube cannot be pushed away, because its's the floor or other unmovable, 
          // this one moves double amount
            if (cj<STATIC_CUBE_COUNT) scaler = 2.0; 
        }
        else
        {
           ci = i+1u;
           cj = cubei;
           scaler = -1.0; // applying the opposite forces on the cube pair
        }
        if (!(length(getCubePos(ci)-getCubePos(cj))>6.0 && cj!=0u)) // bounding check
        for(uint j=0u;j<48u;j++)
        {
            vec3 forcepos = readTex0(ci*12u+j%12u,cj*4u+j/12u).xyz;
            if (forcepos.x!=0.0)
            {
	            vec3 force = readTex2(ci*12u+j%12u,cj*4u+j/12u).xyz;
                
                const float RotationalImmobilityTensor = 1.8;
                force *= scaler;
                vel += force;
                rotvel -= cross(forcepos-pos,force)/RotationalImmobilityTensor;
            }
        }
    }
    

    }
    
    fragColor = vec4(vel, 0.0);
    if (pixely==3u) fragColor = vec4(rotvel, 0.0);
    
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
#define CUBECOUNT 16

vec4 readTex3(int cx,int cy)
{
    return texture(iChannel3,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex2(int cx,int cy)
{
    return texture(iChannel2,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex1(int cx,int cy)
{
    return texture(iChannel1,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec4 readTex0(int cx,int cy)
{
    return texture(iChannel0,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec3 getCubePos(int ci)
{
    return readTex3(ci,0).xyz;
}
vec4 getCubeQuat(int ci)
{
    return readTex3(ci,1).xyzw;
}
vec3 getCubeVel(int ci)
{
    return readTex1(ci,2).xyz;
}
vec3 getCubeRotVel(int ci)
{
    return readTex1(ci,3).xyz;
}
vec3 getCubeVelQP(int ci,vec3 querypos)
{
    return cross(querypos-getCubePos(ci),getCubeRotVel(ci))+getCubeVel(ci);
}



int pixelx,pixely;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    pixelx = int(floor(fragCoord+0.01).x);
    pixely = int(floor(fragCoord+0.01).y);
    
  
    int ci = pixelx/12;
    int cj = pixely/4;
    if (cj>=ci) discard;
    if (ci>=CUBECOUNT) discard;
    
    if ((length(getCubePos(ci)-getCubePos(cj))>6.0 && cj!=0))  // bounding check
    {
        fragColor = vec4(0.,0.,0.,0.);
        return;
    }
    
    int forceid = pixelx%12+pixely%4*12;
    vec3 totalForce;
  
    int i = forceid;
    int lpx = ci*12+i%12;
    int lpy = cj*4+i/12;

    totalForce = readTex2(lpx,lpy).xyz;
    vec3 collpos = readTex0(lpx,lpy).xyz;

    if (collpos.x!=0.0) // x==0 means no collision at the force denoted by this pixel
    {
        vec3 veldiff = getCubeVelQP(cj,collpos)-getCubeVelQP(ci,collpos);

        vec3 collisNormal = readTex1(ci+CUBECOUNT,cj).xyz;

        totalForce += veldiff*0.022;

        float perpart = dot(collisNormal,totalForce);
        vec3 tangentialpart = totalForce-collisNormal*perpart;

        const float FrictionConstant = 0.7;

        if (length(tangentialpart)>perpart*FrictionConstant)
        {
            tangentialpart *= (perpart*FrictionConstant)/length(tangentialpart);
            totalForce = tangentialpart + collisNormal*perpart;
        }


        if (perpart<0.0)
        {
            totalForce = vec3(0.,0.,0.);
        }

    }
    else
    {
        totalForce= vec3(0.,0.,0.);
    }
     
    
    if (iFrame==0) totalForce = vec3(0.,0.,0.);
    fragColor = vec4(totalForce,1.0);
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
#define CUBECOUNT 16
#define STATIC_CUBE_COUNT 1

#define BUFD

#define PI (acos(0.)*2.0)
float gravity = -0.0045;

vec4 readTex3(int cx,int cy)
{
    return texture(iChannel3,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex2(int cx,int cy)
{
    return texture(iChannel2,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec4 readTex1(int cx,int cy)
{
    return texture(iChannel1,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}

vec4 readTex0(int cx,int cy)
{
    return texture(iChannel0,vec2((float(cx)+0.1)/iResolution.x,(float(cy)+0.1)/iResolution.y));
}
vec3 getCubePos(int ci)
{
    return readTex3(ci,0).xyz;
}
vec4 getCubeQuat(int ci)
{
    return readTex3(ci,1).xyzw;
}
vec3 getCubeVel(int ci)
{
    return readTex3(ci,2).xyz;
}
vec3 getCubeRotVel(int ci)
{
    return readTex3(ci,3).xyz;
}

vec3 rotate(vec4 quat,vec3 v)
{
    float sinsqr = (1.0-quat.w*quat.w);
    if (sinsqr!=0.0)
    {
        v=v*quat.w + quat.xyz*((dot(v,quat.xyz)*(1.0-quat.w))*(1.0/sinsqr)) + cross(v,quat.xyz);
        v=v*quat.w + quat.xyz*((dot(v,quat.xyz)*(1.0-quat.w))*(1.0/sinsqr)) + cross(v,quat.xyz);
    }
    return v;
}

int pixelx,pixely;

vec3 rotateAxis(vec3 axis,vec3 v) // the length of the axis defines angle
{
    float len = length(axis);
    if (len!=0.0)
    {
        axis = normalize(axis);
		v = v*cos(len) + axis*((v*axis) * (1.0-cos(len))) + cross(v,axis)*sin(len);
    }
    return v;
}


vec4 rotateRotation(vec4 q,vec3 axis) // Im sure, there is a simpler way to rotate a rotation :)
{
    vec3 x,y,z; // conversion to 3 perpendicular vectors, and rotation
    x = rotateAxis(axis,rotate(q,vec3(1.0,0.0,0.0)));
    y = rotateAxis(axis,rotate(q,vec3(0.0,1.0,0.0)));
    z = rotateAxis(axis,rotate(q,vec3(0.0,0.0,1.0)));
    
    // convert back to quaternion
	float trace = x.x + y.y + z.z; 
	if( trace > 0.0 ) {
		float s = 0.5 / sqrt(trace+ 1.0);
		q.w = 0.25 / s;
		q.x = ( z.y - y.z ) * s;
		q.y = ( x.z - z.x ) * s;
		q.z = ( y.x - x.y ) * s;
	} else {
		if ( x.x > y.y && x.x > z.z ) {
			float s = 2.0 * sqrt( 1.0 + x.x - y.y - z.z);
			q.w = (z.y - y.z ) / s;
			q.x = 0.25 * s;
			q.y = (x.y + y.x ) / s;
			q.z = (x.z + z.x ) / s;
		} else if (y.y > z.z) {
			float s = 2.0 * sqrt( 1.0 + y.y - x.x - z.z);
			q.w = (x.z - z.x ) / s;
			q.x = (x.y + y.x ) / s;
			q.y = 0.25 * s;
			q.z = (y.z + z.y ) / s;
		} else {
			float s = 2.0 * sqrt( 1.0 + z.z - x.x - y.y );
			q.w = (y.x - x.y ) / s;
			q.x = (x.z + z.x ) / s;
			q.y = (y.z + z.y ) / s;
			q.z = 0.25 * s;
		}
	}
    
    q=normalize(q); // no scaling :)
    
    
	return q;    
}



vec3 pos;
vec4 quat;
vec3 vel;
vec3 rotvel;

void initScene(int cubei)
{
        if (cubei==0) // static floor
        {
            pos = vec3(0,-1,5);
            vel = vec3(0.,0.,0.);
            rotvel = vec3(0.,0.,0.);
            quat = vec4(0.0,0.0,0.0,1.0);
            return;
        }
    
    
    	cubei--;
        float cubeif = float(cubei);
    	int div = 5;
    	vec3 ro = vec3(0.,PI*(float(cubei)*2.0)/float(div),0.);
    	pos = rotateAxis(ro,   vec3(0.0,1.0+float(cubei/div)*2.0,2.5));
    
        quat = rotateRotation(normalize(vec4(0.0,0.0,0.0,1.0)),ro);
    
        vel = vec3(-0.00,0.0,0.00);
        rotvel = vec3(cubeif*-0.001*cos(float(iFrame)),0.0,cubeif*-0.001); // randomize start setup
    
    	if (cubei==CUBECOUNT-2) // thrown cube
        {
            pos = vec3(16.,2.0,-1.0+sin(float(iFrame)*1.2)); // randomize 
            vel = vec3(-0.37,0.14+sin(float(iFrame))*0.03,0.0);
        }
        
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    pixelx = int(floor(fragCoord+0.01).x);
    pixely = int(floor(fragCoord+0.01).y);
    
    fragColor = vec4(0.0,0.0,0.0,0.0);
    
    if (pixely>=4) discard;
    if (pixelx>=(CUBECOUNT)) discard;
    int cubei = pixelx;
    
    
    pos = getCubePos(cubei);
    quat = getCubeQuat(cubei);
    vel = getCubeVel(cubei);
    rotvel = getCubeRotVel(cubei);
    
    if (cubei>=STATIC_CUBE_COUNT)
    {
    // apply forces (just the changes)
    for(int i=0;i<CUBECOUNT-1;i++)
    {
        int ci,cj;
        float scaler;
        if (i<cubei)
        {
	        ci = cubei;
            cj = i;
            scaler  = 1.0;
		 // if the other cube cannot be pushed away, because its's the floor or other unmovable, 
          // this one moves double amount
            if (cj<STATIC_CUBE_COUNT) scaler = 2.0; 
        }
        else
        {
           ci = i+1;
           cj = cubei;
           scaler = -1.0; // applying the opposite forces on the cube pair
        }
        
       if (!(length(getCubePos(ci)-getCubePos(cj))>6.0 && cj!=0)) // bounding check
        for(int j=0;j<48;j++)
        {
            vec3 forcepos = readTex0(ci*12+j%12,cj*4+j/12).xyz;
            if (forcepos.x!=0.0)
            {
	            vec3 force = readTex2(ci*12+j%12,cj*4+j/12).xyz;
                
#ifdef BUFD                
                // add repulsive force
                vec4 collisnormal = readTex1(ci+CUBECOUNT,cj);
                collisnormal.xyz *= max(abs(dot(forcepos,collisnormal.xyz)-collisnormal.w)-0.01,0.0);
                force += collisnormal.xyz*0.003;
#endif
                
                const float RotationalImmobilityTensor = 1.8;
                force *= scaler;
                vel += force;
                rotvel -= cross(forcepos-pos,force)/RotationalImmobilityTensor;
            }
        }
    }
    
#ifdef BUFD
    // move by adding velocity to position, and rotate
    pos += vel;
    quat = rotateRotation(quat,rotvel);
    vel.y += gravity;
#endif
    }

#ifdef BUFD    
    if (iFrame%(60*8)==0)
    {
        initScene(cubei);
    }

    fragColor = vec4(pos, 0.0);
    if (pixely==1) fragColor = quat;
#endif
    if (pixely==2) fragColor = vec4(vel, 0.0);
    if (pixely==3) fragColor = vec4(rotvel, 0.0);
    
}