

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = pow(texelFetch(iChannel3,ivec2(fragCoord),0)
, vec4(1./2.2) );}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // Store all Spheres
    int ri = ivec2(fragCoord).x;
    vec4 rp = texelFetch(iChannel0,ivec2(ri,0), 0);
    vec4 rv = texelFetch(iChannel0,ivec2(ri,1), 0);
    
    bool vel = int(fragCoord.y) == 1;
    
    vec4 r = vel ? rv : rp;
    
    if(ri < sphereAmount) {
        for(int i = 0; i < sphereAmount; i++) {
            if(i != ri) { // Collision
                vec4 s = texelFetch(iChannel0,ivec2(i,0), 0);
                vec3 delt = (rp - s).xyz;
                float dis = length(delt) - sphereRadius * 2.;
                if(dis < 0.){
                    if(vel){
                        dis = min(dis,-0.01);
                        r.x -= normalize(delt).x * dis * 0.05;
                        r.y -= normalize(delt).y * dis * 0.05;
                        r.z -= normalize(delt).z * dis * 0.05;
                        r.x *= 0.9;
                        r.y *= 0.9;
                        r.z *= 0.9;

                    }

                }
            }
        }
        // Velocity Update
        if(vel){
            r.z += -0.001;
            r.x *= 0.9995;
            r.y *= 0.9995;
            r.z *= 0.9995;
        }else{
            r.x += rv.x;
            r.y += rv.y;
            r.z += rv.z;
        }
        
        // Bounds
        float bounce = 0.1;
        // Cube Bounds
        if(BOUNDTYPE == 0){
        if(rp.x - sphereRadius < -bounds.x){
            r.x = vel ? abs(rv.x)*bounce : - bounds.x + sphereRadius;
        }
        if(rp.x + sphereRadius > bounds.x){
            r.x = vel ? - abs(rv.x)*bounce : bounds.x - sphereRadius;
        }
        if(rp.y - sphereRadius < -bounds.y){
            r.y = vel ? abs(rv.y)*bounce : - bounds.y + sphereRadius;
        }
        if(rp.y + sphereRadius > bounds.y){
            r.y = vel ? - abs(rv.y)*bounce : bounds.y - sphereRadius;
        }
        if(rp.z - sphereRadius < -bounds.z){
            r.z = vel ? abs(rv.z)*bounce : - bounds.z + sphereRadius;
        }}
        // Sphere Bounds
        if(BOUNDTYPE == 1){
        if(length(rp.xyz) > boundRadius - sphereRadius){
            if(vel){
                float len = length(rp.xyz)-(boundRadius - sphereRadius);
                r.xyz -= 0.9 * normalize(rp.xyz) * len;
            }else{
                 r.xyz = (normalize(rp.xyz) * (boundRadius - sphereRadius));

            }
        }
        }
        
        
        fragColor = r;
    }
    // Take
    if(iFrame % 1500 == 1500*ri/sphereAmount){
        if(BOUNDTYPE == 0){
            fragColor = vec4(0,0,5.0,ri);
        
        }
        if(BOUNDTYPE == 1){
            fragColor = vec4(0,0,boundRadius - sphereRadius,ri);
        
        }
        if(vel){
            fragColor = vec4(0.0,0.0,-0.15,fragCoord.x);
        }
    }
    // Initials
    if(iFrame <= 2){
        fragColor = vec4(0.9*mod(float(ri),5.0)-2.5,0.9*mod(float(ri)/5.0,6.0)-2.5,0.9*mod(float(ri)/12.0,6.0)-1.0,float(ri));
        if(vel){
            fragColor = vec4(0.0,0.0,0.0,ri);
        }
    }
    
   
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
// 0: Cube, 1: Sphere
#define BOUNDTYPE 0 

int sphereAmount = 26; // Amount of balls
float sphereRadius = 0.5; // Radius of balls
vec3 bounds = vec3(2.1,2.1,1.5); // Bounding rectangle
float boundRadius = 2.6;
float blur = 0.8; // 0: no blur, 1: all previous frames
float dofAmount = 0.03; // How "big" the square "lens" is for depth of field
float ballIOR = 0.93; //  (inverse?) index of refraction of air to balls; Not optimized for ior > 1
float rotationSpeed = 0.15;
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
#define DOF 1

#define rot(a) mat2(cos(a),-sin(a),sin(a),cos(a))

float rand(vec3 po){ // Lazy random function, but only used once/twice per pixel
    return (sin((cos(po.x*290.65+po.y*25.6+po.z*2.97)*5632.75+849.2*cos(po.y*534.24+po.x)+2424.64*cos(po.z*473.76))));
}

// Font SDF code: https://www.shadertoy.com/view/llcXRl
vec2 matmin(vec2 a, vec2 b)
{
    if (a.x < b.x) return a;
    else return b;
}
vec2 matmax(vec2 a, vec2 b)
{
    if (a.x > b.x) return a;
    else return b;
}

vec4 SampleFontTex(vec2 uv, int letter){
    vec2 fl = floor(uv + 0.5);
    uv = fl + fract(uv+0.5)-0.5 + vec2(float(letter%16),float(letter/16));
    // Sample the font texture. Make sure to not use mipmaps.
    // Add a small amount to the distance field to prevent a strange bug on some gpus. Slightly mysterious. :(
    return texture(iChannel0, (uv+0.5)*(1.0/16.0), -100.0) + vec4(0.0, 0.0, 0.0, 0.000000001);
}
float opSmoothUnion( float d1, float d2, float k ) {
    float h = clamp( 0.5 + 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) - k*h*(1.0-h); }

float opSmoothSubtraction( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2+d1)/k, 0.0, 1.0 );
    return mix( d2, -d1, h ) + k*h*(1.0-h); }

float opSmoothIntersection( float d1, float d2, float k ) {
    float h = clamp( 0.5 - 0.5*(d2-d1)/k, 0.0, 1.0 );
    return mix( d2, d1, h ) + k*h*(1.0-h); }

float sdBox(vec3 p, vec3 radius){
  vec3 dist = abs(p) - radius;
  return min(max(dist.x, max(dist.y, dist.z)), 0.0) + length(max(dist, 0.0));
}
float sdSphere( vec3 p, float s )
{
  return length(p)-s;
}
vec2 DistanceToObject(vec3 p, int letter)
{
	// Load the font texture's distance field.
    float letterDistField = (SampleFontTex(p.xy,letter+64).w - 0.5+1.0/256.0);
    // intersect it with a box.
    float cropBox = sdBox(p, vec3(0.3, 0.3, 0.05));
    vec2 letters = matmax(vec2(letterDistField, 0.0), vec2(cropBox, 1.0));
    return letters;
}

vec3 findNormal(vec3 p, float d, int letter){
    return normalize(vec3(
        DistanceToObject(p + vec3(d,0,d),letter).x - DistanceToObject(p - vec3(d,0,0),letter).x,
        DistanceToObject(p + vec3(0,d,0),letter).x - DistanceToObject(p - vec3(0,d,0),letter).x,
        DistanceToObject(p + vec3(0,0,d),letter).x - DistanceToObject(p - vec3(0,0,d),letter).x
        ));
}



void checkDist(inout float d, float c, inout int obj, int obi){
    if(c < d){
        d = c;
        obj = obi;
    }
}
float sdScene(vec3 p, inout int obj){
    float dist = 10000.;
    obj = -1;
    for(int i = 0; i < sphereAmount; i++){
        vec4 s = texelFetch(iChannel1,ivec2(i,0), 0);
        checkDist(dist, sdSphere(p - vec3(s.x,s.y,s.z),sphereRadius), obj, i+1);
    }
    float douter = 0.;
    
    if(BOUNDTYPE == 0){ // Outer Box
     douter = opSmoothSubtraction(sdBox(p+vec3(0.0,0.0,0), bounds-0.2) - 0.2,
                    sdBox(p+vec3(0.0,0.0,2.9), bounds+0.5)-0.2,0.2);               
    }
    if(BOUNDTYPE == 1){ // Outer Bowl
     douter = opSmoothSubtraction(sdSphere(p+vec3(0.0,0.0,0), boundRadius-0.15) - 0.2,
                    sdSphere(p+vec3(0.0,0.0,0.4), boundRadius-0.2)-0.2,0.2);               
    }
    
    checkDist(dist, douter, obj, 0);
    return dist;
}
float sdScene(vec3 p){
    int obj = 0;
    return sdScene(p, obj);
}

vec3 findNormal(vec3 p, float d){
    return normalize(vec3(
        sdScene(p + vec3(d,0,d)) - sdScene(p - vec3(d,0,0)),
        sdScene(p + vec3(0,d,0)) - sdScene(p - vec3(0,d,0)),
        sdScene(p + vec3(0,0,d)) - sdScene(p - vec3(0,0,d))));
}

// For the transmission rays
vec3 snellLaw(vec3 s, vec3 N, float ior){
    //return ior * cross(N,cross(-N, s)) - N * sqrt(1.- ior * ior * dot(cross(N,s),cross(N,s)));
    float c = dot(-N,s);
    return ior * s + (ior * c - sqrt(1.-ior*ior*(1.-c*c))) * N;

}
// For the Specular Rays
vec3 refl(vec3 d, vec3 n){

    return d - 2.f * n * dot(d,n);
}

vec4 raycastBall(inout vec3 pos, inout vec3 dir, int object){
int inrays = 30;
    vec4 col = vec4(0,0,0,0);
    for(int i = 0; i < inrays; i++){
        float d = sdBox(pos, vec3(1,1,1) * sphereRadius/3.0);
        d = DistanceToObject(pos,object).x;
        d = min(d, -length(pos)+sphereRadius+0.02); // distance to outer edge
        if(d < 0.01 && length(pos) < sphereRadius-0.05) {
            i = inrays;
            col = clamp(vec4(0.5+sin(float(object) + 2.1),0.5+sin(float(object)+4.2),0.5+sin(float(object)),1),0.,1.); // color of letters  
        }else if(length(pos) > sphereRadius + 0.01){ // Reaches outside of sphere
            i = inrays;
        }else {
            pos += dir * d;
        }
    }
    
    return col;
}


vec4 raycast(vec3 pos, vec3 dir, int maxmarch){ // Casts a ray from position in direction, returns color of object. TODO: out direction, materials.
    vec4 col = vec4(0,0,0,0);
    float lastd=0.;
    float totalDist = 0.;
    int ob = -1;
    int obhit = 6;
    float ior = ballIOR;
    vec4 totalcol = vec4(0,0,0,0);
    float totaldim = 1.0;
    for(int i = 0; i < maxmarch; i++){
         float d = sdScene(pos + dir * lastd, ob);
         
         if(d < 0.01 && ob >= 1 && obhit > 1){ // Hits a sphere within 0.05
         
             // Get hitsphere from buffer
             vec4 s = texelFetch(iChannel1,ivec2(ob-1,0), 0);
             vec3 ballRot = vec3(1.0*s.z,1.0*s.y,0);
             pos += dir * lastd*1.;
             vec3 norm = normalize(pos - s.xyz);
             vec3 ref = refl(dir, norm);
             // Add partial fresnel reflections
             float fres = abs(dot(dir,norm));
             totalcol +=  texture(iChannel2, ref.xzy, -100.0).xyzw * totaldim * (1.-fres);
             totaldim *= (fres);
           
             // Updates position, direction when entering sphere 
             vec3 newpo = -s.xyz + pos;
             vec3 newdir = normalize(snellLaw(dir,norm,ior));  // Refraction
             vec4 col2 = vec4(1,1,0,1);

             // Global to Local Transformations 
             newpo.xz*= rot(ballRot.y);
             newdir.xz*= rot(ballRot.y);
             newpo.yz*= rot(ballRot.x);
             newdir.yz*= rot(ballRot.x);
             
             col2 = raycastBall(newpo, newdir, ob); // Casts into sphere
               
             if(col2.w > 0.5){ // hit letter
                 vec3 norma = findNormal(newpo,0.03,ob);
                 norma.yz*= rot(-ballRot.x);
                 norma.xz *= rot(-ballRot.y);
                 vec3 reflcol = texture(iChannel2, norma.xzy, -100.0).xyz * totaldim;
                 totalcol += vec4(reflcol * col2.xyz,0);
                 col = col2;
                 i = maxmarch;
                 
             }else{ // go through sphere
             
                 // Local to Global Transformations 
                 newpo.yz*= rot(-ballRot.x);
                 newdir.yz*= rot(-ballRot.x);
                 newpo.xz *= rot(-ballRot.y);
                 newdir.xz *= rot(-ballRot.y);
                 dir = newdir;
                 pos = s.xyz + newpo;
                 vec3 newnorm = normalize(pos - s.xyz);
                 
                 dir = normalize(snellLaw(dir,-newnorm,1./ior)); // Out refraction
                 lastd = 0.;
                 obhit -= 1;
             
             }
         
         } else if(d < 0.005) { // Hits bowl
              i = maxmarch;
              vec3 norm = findNormal(pos,0.0001);
                
                
              norm += 0.5*rand(floor(pos*10.));
              float s = 1.0;
              
              vec3 refla = refl(dir,norm);
              col = vec4(texture(iChannel2, refla,-100.0)) * 0.2;
              
              totalcol += col * totaldim;

        } else if(d>200.) {
              i = maxmarch;
              
              totalcol +=  texture(iChannel2, dir.xzy, 0.0).xyzw * totaldim  * totaldim;
         } else {
              pos += dir * lastd; // march ray
              totalDist += lastd;
              lastd = d;

         }
    }
    return totalcol;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    

    vec4 outray = vec4(1,1,1,1);

    vec2 uv = (fragCoord-0.5f*iResolution.xy)/iResolution.x;
    
    float dofrang = 2.*4.5;
    vec2 jitter = vec2(0,0);
    if(DOF==1){jitter = dofAmount*vec2(rand(vec3(fragCoord.xy,iTime)),rand(vec3(fragCoord.xy,iTime+1.)));}
    
    
    vec3 pos = vec3(0.0,-4.5,1.5) + vec3(jitter.x,0,jitter.y);
    vec3 dir = normalize(vec3(uv.x-jitter.x/dofrang,0.5,uv.y-jitter.y/dofrang));
    
    if(iMouse.z > 0.){
        pos -= vec3(0.0,0.0,1.5);

        dir.zy *= rot(-2.*iMouse.y/iResolution.y+1.5);
        pos.zy *= rot(-2.*iMouse.y/iResolution.y+1.5);

        dir.xy *= rot(-3.14*iMouse.x/iResolution.x);
        pos.xy *= rot(-3.14*iMouse.x/iResolution.x);
    }else{
    dir.zy *= rot(0.4);
    dir.xy *= rot(iTime*rotationSpeed);
    pos.xy *= rot(iTime*rotationSpeed);
    }
    
    
    vec4 precolor = texelFetch(iChannel3,ivec2(fragCoord),0);

    outray = raycast(pos,dir,250);
    if(iFrame > 10){
    fragColor = precolor * blur + vec4(outray.xyz,1) * (1.-blur);
    }else{
    fragColor = vec4(outray.xyz,1);
    }
    
}