

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

//Strided sort and spatial decorrelation fit into buf a and b over 7 frames

//Rendering in buf A
void mainImage( out vec4 O, in vec2 I )
{
    //O=abs(texture(iChannel0,I/R.xy))/1e3;//textureLod(iChannel0,vec2(.5),6.)/2.;
    O=texture(iChannel1,I/R.xy);
    //O += texelFetch(iChannel2, ivec2(I),0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

//Use O.w for render pass



void splat(inout float O, vec2 I, vec2 ip, vec2 p){
    
    //O = min(O,vec4(length(p-I))/3.);
    float d2 = dot2(I-p);
    O += exp(-d2*.6)/8.;
    
    //if(floor(ip)==floor(p)) O += (.255);
    
}


#define T2(a,b) texelFetch(iChannel2, ivec2(p)+ivec2(a,b),0).w
#define T0(a,b) texelFetch(iChannel0, ivec2(I)+ivec2(a,b),0)


void mainImage( out vec4 O, in vec2 I )
{
    
    int stage = iFrame%7;
    
    vec2 uv = I/R.xy;
    vec2 r1 = rand2(IHash3(I.x,I.y,iFrame));
    vec2 r2 = rand2(IHash3(I.x,I.y,34526324^iFrame));
    
    
    
    O = texture(iChannel0,uv);
    float Owp = O.w;
    O.w=0.;
    vec2 p = O.xy;
    if(iFrame<3 || (texelFetch(iChannel3,ivec2(32,0),0).x>.5)){
    	vec2 r = rand2(IHash3(-iFrame,I.x,I.y));
        p = R.xy/2.;
        O.xy = R.xy *r1;
    }
    else if(iFrame>30) {
        //reset location sometimes
        if(O.x<0.||O.y<0.||O.x>R.x||O.y>R.y||r1.x<.075){
            O.xy = R.xy *r2;
        }

        //Shift every point in the direction of the gradient of the blurred image in buf c
        vec2 g = vec2(T2(1,0)-T2(-1,0),T2(0,1)-T2(0,-1));
        O.xy -= normalize(g)/(2.) * (1.+I.y/R.y*4.);//*1e-1;
    }
    
    if(stage==0) {;
    
        
    for(int i = 0; i < 9; i++){
    	vec2 ip = forward_mapping(I-1.+vec2(i/3,i%3),iR.x,iR.y,(iFrame)/7-1);
        vec4 t = texelFetch(iChannel1,ivec2(ip),0);
        
        t.xy = reverse_mapping(t.xy,iR.x,iR.y,(iFrame)/7-1);
        t.zw = reverse_mapping(t.zw,iR.x,iR.y,(iFrame)/7-1);
        
        splat(O.w,I,I,t.xy);
        splat(O.w,I,I,t.zw);
        
        
    }
    }
    O.w=pow(O.w,.7);
    if(stage!=0) O.w=Owp;
    else O.w = mix(O.w,Owp,0.3);
    //Smooth over time
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define R iResolution
#define iR ivec3(R)
#define uR uvec3(R)
#define IHash3(x,y,z) IHash(int(x)^IHash(int(y)^IHash(int(z))))
#define dot2(o) dot((o),(o))
//#define tx(ch,p,R) texelFetch(ch, Zmod(p,iR.xy),0)
#define tx(ch,p,R) texture(ch, (vec2(Zmod(p,iR.xy))+0.5)/R.xy)

//Roboust/universal integer modulus function
#define Zmod(x,y) (((x)+(y)+(y)+(y))-(((x)+(y)+(y)+(y))/(y))*(y))
//#define Zmod(x,y) ((x+y*10)%y)



uint pack(vec2 x)
{
  
    x = 65534.0f*clamp(0.5f*x+0.5f, 0.0f, 1.0f);
    return uint(round(x.x)) + 65535u*uint(round(x.y));
}

vec2 unpack(uint a)
{
    vec2 x = vec2(a%65535u, a/65535u);
    return clamp(x/65534.0f, 0.0f,1.0f)*2.0f - 1.0f;
}




float packVec2(vec2 x){
    //return uintBitsToFloat(packSnorm2x16(x/10.));
    return uintBitsToFloat(pack(x/10.));
}
vec2 umpackVec2(float x){
    //return unpackSnorm2x16(floatBitsToUint(x))*10.;
    return unpack(floatBitsToUint(x))*10.;
}

bool inbounds(vec2 x, vec2 y){
    return (x.x>0.&&x.y>0.&&x.x<y.x&&x.y<y.y);
}

//RNG
int IHash(int a){
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

float Hash(int a){
	return float(IHash(a)) / float(0x7FFFFFFF);
}
vec4 rand4(int seed){
    return vec4(Hash(seed^0x34F85A93),
                Hash(seed^0x85FB93D5),
                Hash(seed^0x6253DF84),
                Hash(seed^0x25FC3625));
}
vec3 rand3(int seed){
    return vec3(Hash(seed^0x348CD593),
                Hash(seed^0x8593FD5),
                Hash(seed^0x62A5D384));
}
vec2 rand2(int seed){
    return vec2(Hash(seed^0x348C5F93),
                Hash(seed^0x8593D5BB));
}


vec2 randn(vec2 randuniform){
    vec2 r = randuniform;
    r.x = sqrt(-2.*log(1e-9+abs(r.x)));
    r.y *= 6.28318;
    r = r.x*vec2(cos(r.y),sin(r.y));
    return r;
}

//Random injective mapping from each pixel to a random new pixel
//By alternatively adding some randomness from y into x and then 
//x into y, a reversible hash function is made. To take the inverse,
//simply undo the last "add randomness to y" step by subtracting the
//same random value. You can calculate the random value that was used
//to modify y because it depends only on x.
//Reversible == one to one == injective
//input iFrame/2 to re randoize the mapping every frame
#define mapping_iters 2
vec2 forward_mapping(vec2 Z,int p, int q, int Fover2){
    //int seed = 0;        // Optionaly keep seed constant for that static randomness look
	int seed = Fover2;
    if(!inbounds(Z,vec2(p,q))){return vec2(0);} //Dont map points from outside the boundry
    int x=int(Z.x);
    int y=int(Z.y);
    
    //Change iterations here to zero to use the identity function as a mapping
    //Some particles seem to have a better chance of getting drawn...
    //But it shows off the artifacts in all their glory, looks pretty cool after a reset
    for(int i = 0; i < mapping_iters; i++){
        x = Zmod(x + IHash(y^seed)%p,p);
        y = Zmod(y + IHash(x^seed)%q,q);
    }
    
	//This is the inverse mapping, only difference is - instead of + and the order of x and y
    //uncommenting should have the same effect as reducing iterations above to zero
    //This is a pretty good test of the one to one property of the mapping
    //Originally it seemed to not be working quite right on some platforms so
    //this can confirm if that is happening. The effect of a non injective mapping is collisions
    //And thus many particles getting lost near the final pass.
    /*
    for(int i = 0; i < 5; i++){
        y = Zmod(y - IHash(x)%q,q);
        x = Zmod(x - IHash(y)%p,p);
    }
	*/
    
    return vec2(x,y)+fract(Z);
    
}
vec2 reverse_mapping(vec2 Z,int p, int q, int Fover2){
    //int seed = 0;        // Optionaly keep seed constant for that static randomness look
	int seed = Fover2;
    if(!inbounds(Z,vec2(p,q))){return vec2(0);} //Dont map points from outside the boundry
    int x=int(Z.x);
    int y=int(Z.y);
    
    
    
    for(int i = 0; i < mapping_iters; i++){
        y = Zmod(y - IHash(x^seed)%q,q);
        x = Zmod(x - IHash(y^seed)%p,p);
    }
    
    return vec2(x,y)+fract(Z);
    
}

float score(vec2 p, vec2 I, vec3 R){
    if(!inbounds(p,R.xy)) return 1e6; //Bad score for points outside boundry
    //This should get revamped, there is no reasoning to use
    //euclidean distance, this metric probably should reflect the tree strtucture
    //Maybe even output a simple 1 or 0 if the index of this texel leads to the leaf
    //node that this particle p is going towards
    
    //Difference in the noise when using this other metric suggests that 
    //this is indeed screwing performance (likelyhood of missing particles)
    vec2 D = p-I;
    D = mod(D+R.xy/2.,R.xy)-R.xy/2.;
    return max(abs(D.x),abs(D.y));
    //use l infinity in toroidal space
    
    //return dot2(I-p);
}

void updateRank(vec4 t, inout vec4 O, inout float s, vec2 I, vec3 R){
    float sp = score(t.xy,I,R);
    if(sp<s){
        s=sp;
        O=t;
    }
}

//Update ranking, save a list of two particle xy indices. O.xy is better particle, O.zw is a different not as good one
void updateRank2x(vec2 t, inout vec4 O, inout float s0, inout float s1, vec2 I, vec3 R){
    float sp = score(t,I,R);
    if(sp<s0){
        //Shift down the line
        s1=s0;
        O.zw=O.xy;
        s0=sp;
        O.xy=t;
    } else if(sp<s1){
        //Bump off the bottom one
        s1=sp;
        O.zw=t;
        
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Strided sort ~= jump flood
//Strided sort summarry:
//Each pass looks at 9 xy locations stored in the previous pass and selects the closest one
//The locations sampled are arranged in a 3x3 with the center located at I, and the spacing
//a power of 3
//Total 7 passes over two frames sized large to small
//A->B->C->D->B->C->D->Image
//Spacing 3^6 ..., 3^1, 3^0
//I think this gives an optimal data path from each pixel to each other pixel under the constraint of 7 passes

//In each buffer, the pixel to get drawn at index is saved in xy and the exact particle location is saved in zw.
//For more complex particles zw should instead be a pointer to the particle
//zw is unused for sorting, sort only based on xy


//large to small
void mainImage( out vec4 O, in vec2 I )
{
    //Split frames into two stages
    int stage = iFrame%7;
    int size = int(.5+pow(3.,float(6-stage)));
    
    vec2 r = rand2(IHash3(iFrame,I.x,I.y));
   // if (stage==6) discard;
    
    //int size = stage==0?729:27; //729=3^6
    float s0;
    float s1;
    //init with top left corner and center
    if(stage==0){
        vec2 t0 = tx(iChannel0, ivec2(I)-size,R).xy;
        vec2 t1 = tx(iChannel0, ivec2(I),R).xy;
                                                                         
        s0 = score(t0,I,R);
        s1 = score(t1,I,R);
        
        O.xy=t0==vec2(0)?vec2(0):forward_mapping(t0, iR.x, iR.y,iFrame/7);
        O.zw=t1==vec2(0)?vec2(0):forward_mapping(t1, iR.x, iR.y,iFrame/7);
        
        //Select the better one, make sure scores are in order with s0<s1
        if(s0>s1){
            vec2 _ = O.xy;
            O.xy = O.zw;
            O.zw = _;
            _.x = s0;
            s0 = s1;
            s1 = _.x;
        }
    } else {
        O = tx(iChannel1, ivec2(I)-size,R );
        s0 = score(O.xy,I,R);
        s1 = score(O.zw,I,R);
    }
    for(int i = 1; i < 9; i++){
        if(stage==0){
        	vec2 t = tx(iChannel0,ivec2(I)-size+size*ivec2(i/3,i%3),R).xy;
            t = forward_mapping(t, iR.x, iR.y,iFrame/7);
            updateRank2x(t,O,s0,s1,I,R);
            
        } else {
        	vec4 t;
            t = tx(iChannel1,ivec2(I)-size+size*ivec2(i/3,i%3),R); 
            updateRank2x(t.xy,O,s0,s1,I,R);
            updateRank2x(t.zw,O,s0,s1,I,R);
        }
        
    }
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
//Do a multipass blur, radius controlled by iMouse
void mainImage( out vec4 O, in vec2 I )
{
    #define T0(a,b) texelFetch(iChannel0, Zmod((ivec2(I)+ivec2(a,b)),ivec2(R.xy)),0).wwww
    #define T1(a,b) texelFetch(iChannel1, Zmod((ivec2(I)+ivec2(a,b)),ivec2(R.xy)),0).wwww
    O = T1(0,0);//*.999999;
    O -= T0(0,0);
    O=mix(O,(T1(0,0) + T1(0,1) + T1(1,0) + T1(0,-1) + T1(-1,0))/5.,iFrame<60?.01:0.85-.15*iMouse.y/R.y);
    if(iFrame<3||texelFetch(iChannel3,ivec2(32,0),0).x>.5){
        O.w=-10.+length(I.xy-R.xy/2.); 
        //O.w = 0.; 
    }
    if(length(I-R.xy/2.)<1.){
        O-=1.;
    }
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 O, in vec2 I )
{
    O=texture(iChannel0,I/R.xy).wwww;
    O=mix(O,texture(iChannel1,I/R.xy),.96);
}