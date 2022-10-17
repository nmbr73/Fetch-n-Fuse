

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//Four step seperated FFT, factored horizontally, vertically, and over major and minor axis for each of x and y
//Total worst case for 2048*2048 image is 2 (x and y) times 4 (factored into 4) 32pt dft's where each pixel/thread 
//must compute one bin of its corresponding dft. Pipelining through A-B-C-D means fft of the whoe screen only takes one frame.
//Both the x and y of the feild need to be fft'd so it takes up all 4 channels to do an fft, so every other frame
//the fft direction is swapped to compute the inverse, and overall the simulation runs at one step per two frames



void mainImage( out vec4 O, in vec2 I )
{
    O = vec4(0);
    if(FFT_DIR==BACKWARD){
        if(texelFetch(iChannel1,ivec2(32,2),0).x>.5) discard;
        vec4 t0 = texture(iChannel3, fract(I/R.xy));
        O = vec4(.5*log(.5*length(t0)));
    } else {
        if(texelFetch(iChannel1,ivec2(32,2),0).x<.5) discard;
    	O = texture(iChannel3, fract(.5+I/R.xy));
        float l0 = dot2(O.xy);
        O.xy = O.xy/l0*log(1.+l0);
        float l1 = dot2(O.zw);
        O.zw = O.zw/l1*log(1.+l1);
        O.xyz += vec3(1,1,0)*O.w;
        O=abs(O);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define pi 3.14159265
#define W(i,n) cexp(vec2(0,FFT_DIR*2.*pi*float(i)/float(n)))
#define R iResolution
#define fradix float(radix)
#define T(c,x,y) texelFetch(c, ivec2(x,y), 0)
#define SUM(expr, ind, len) \
    sum = vec2(0);\
    for(int ind = 0; ind < 64; ind++){\
        if (ind >= len) break;\
        sum += expr;\
    }

#define FFT_DIR float((iFrame%2)*2-1)
#define FORWARD 1.
#define BACKWARD -1.

vec2 sum;

int x_N0;
int y_N0;
int x_N1;
int y_N1;

float dot2(vec2 x) { return dot(x,x); }

float factor(float x){
    x = floor(x);
    float f = floor(sqrt(x));
    while(fract(x/f)>.5/x){f--;}
    return x/f;
}

void setRadix(vec3 R){
    
    x_N0 = int(R.x/factor(R.x));
    y_N0 = int(R.y/factor(R.y));
    x_N1 = int(R.x)/x_N0;
    y_N1 = int(R.y)/y_N0;
    
}

vec2 cprod(vec2 a, vec2 b){
    return mat2(a,-a.y,a.x) * b;
}

vec2 cis(float t){
    return cos(t - vec2(0,pi/2.));
}
vec2 cexp(vec2 z) {
    return exp(z.x)*cis(z.y);
}
int IHash(int a){
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return a;
}

float Hash(int a){
	a = (a ^ 61) ^ (a >> 16);
	a = a + (a << 3);
	a = a ^ (a >> 4);
	a = a * 0x27d4eb2d;
	a = a ^ (a >> 15);
	return float(a) / float(0x7FFFFFFF);
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
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec4 inp(sampler2D ch,int x, int y);
void mainImage( out vec4 O, in vec2 I )
{
    setRadix(R);
    O = vec4(0);
    int x = int(I.x);
    int y = int(I.y);  
    
    int n = (x/x_N1);
    SUM( cprod((inp(iChannel0, (x%x_N1)+i*x_N1, y).xy),W(i*n,x_N0)),i,x_N0 );
    O.xy = (cprod(sum, W((x%x_N1)*n,int(R.x))));

    SUM( cprod((inp(iChannel0, (x%x_N1)+i*x_N1, y).zw),W(i*n,x_N0)),i,x_N0 );
    O.zw = (cprod(sum, W((x%x_N1)*n,int(R.x))));
}


vec4 inp(sampler2D ch,int x, int y){
    if(FFT_DIR==FORWARD){
        vec2 v = T(ch, x, y).xz;
        return texture(ch, fract((-v + vec2(x, y) + rand2(IHash(x^IHash(y^IHash(iFrame)))))/R.xy));
    } else {
        return T(ch, x, y);
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<

void mainImage( out vec4 O, in vec2 I )
{
    setRadix(R);
    O = vec4(0);
    int x = int(I.x);
    int y = int(I.y);
    
    int n = (x/x_N0);
    SUM( cprod((T(iChannel0, (x%x_N0)*x_N1+i, y).xy),W(i*n,x_N1)),i,x_N1 );
    O.xy = (sum);
    
    SUM( cprod((T(iChannel0, (x%x_N0)*x_N1+i, y).zw),W(i*n,x_N1)),i,x_N1 );
    O.zw = (sum);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
void mainImage( out vec4 O, in vec2 I )
{
    setRadix(R);
    O = vec4(0);
    int x = int(I.x);
    int y = int(I.y);
    
    int n = (y/y_N1);
    SUM( cprod((T(iChannel0, x, (y%y_N1)+i*y_N1).xy),W(i*n,y_N0)),i,y_N0 );
    O.xy = (cprod(sum, W((y%y_N1)*n,int(R.y))));
    
    SUM( cprod((T(iChannel0, x, (y%y_N1)+i*y_N1).zw),W(i*n,y_N0)),i,y_N0 );
    O.zw = (cprod(sum, W((y%y_N1)*n,int(R.y))));
}
// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
void mainImage( out vec4 O, in vec2 I )
{
    setRadix(R);
    O = vec4(0);
    int x = int(I.x);
    int y = int(I.y);
    
    int n = (y/y_N0);
    SUM( cprod((T(iChannel0, x, (y%y_N0)*y_N1+i).xy),W(i*n,y_N1)),i,y_N1 );
    O.xy = (sum/sqrt(R.x*R.y));
    
    SUM( cprod((T(iChannel0, x, (y%y_N0)*y_N1+i).zw),W(i*n,y_N1)),i,y_N1 );
    O.zw = (sum/sqrt(R.x*R.y));
    
    vec2 C = mod(I.xy+R.xy/2.,R.xy)-R.xy/2.;
    if(FFT_DIR==FORWARD){
        if(texelFetch(iChannel2,ivec2(88,2),0).x<.5)
        	O*=exp(-dot2( C )*2e-7);
        if(length(C)>0. && texelFetch(iChannel2,ivec2(90,2),0).x<.5){
            float l = length(O.xz);
        	O.xz-=dot(normalize(C),O.xz)*normalize(C);
            if(texelFetch(iChannel2,ivec2(67,2),0).x<.5)
            	O.xz *= (l/(1e-3+length(O.xz)));
        }
        if(length(C)<1.) O*=0.;
        if(length(C)>0. && texelFetch(iChannel2,ivec2(90,2),0).x<.5){
            float l = length(O.yw);
        	O.yw-=dot(normalize(C),O.yw)*normalize(C);
            if(texelFetch(iChannel2,ivec2(67,2),0).x<.5)
            	O.yw *= (l/(1e-3+length(O.yw)));
        }
        
    } else {
        O.xz += .01*vec2(iMouse.xy-R.xy*.5)*exp(-.1/(1.+length(I-R.xy*.5))*dot2(I-R.xy*.5));
    }
    
    if(iFrame<6 && FFT_DIR==BACKWARD){
        O=vec4(0);
    }
}