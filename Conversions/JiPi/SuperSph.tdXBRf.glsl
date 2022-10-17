

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 pos )
{
    sN = SN; 
    N = ivec2(R*P/vec2(sN));
    TN = N.x*N.y;
    ivec2 pi = ivec2(floor(pos));
    
    fragColor = texel(ch2, pi);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define texel(a, p) texelFetch(a, ivec2(p), 0)
#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3
#define R iResolution.xy
#define PI 3.14159265

#define dt 3.
#define loop(i,x) for(int i = min(0, iFrame); i < x; i++)

//rendering scale
#define SC 1.

#define smoothR 2.5
#define density 0.036

//sim stuff
struct obj
{
    int id; //ID
    vec2 X; //position
    vec2 V; //velocity
    float Pressure; //pressure
    float Rho; //neighbor density
    float SScale; //smooth scale
    float Div; //average distance to neighbors
    vec4 Y; //additional data
};
    
float Force(float d)
{
    return 0.2*exp(-0.05*d)-2.*exp(-0.5*d);
}

//40% of the buffer used for particles
#define P 0.5
#define SN ivec2(4, 2)

ivec2 N; //buffer size
ivec2 sN; //buffer single element size
int TN; //buffer length

ivec2 i2xy(ivec3 sid)
{
    return sN*ivec2(sid.x%N.x, sid.x/N.x) + sid.yz;
}

ivec3 xy2i(ivec2 p)
{
    ivec2 pi = p/sN;
    return ivec3(pi.x + pi.y*N.x, p.x%sN.x, p.y%sN.y);
}

ivec2 cross_distribution(int i)
{
    return (1<<(i/4)) * ivec2( ((i&2)/2)^1, (i&2)/2 ) * ( 2*(i%2) - 1 );
}

float sqr(float x)
{
return x*x + 1e-2;
}

//hash funcs
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

float hash13(vec3 p3)
{
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 hash33(vec3 p3)
{
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);
}

vec3 hash31(float p)
{
   vec3 p3 = fract(vec3(p) * vec3(.1031, .1030, .0973));
   p3 += dot(p3, p3.yzx+33.33);
   return fract((p3.xxy+p3.yzz)*p3.zyx); 
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//L1 particle buffer - simulation
//L2 directional neighbor graph 4x - sort

int ID;
obj O; //this object

//sort arrays
vec4 lnk0, lnk1;
vec4 d0, d1;

//L3
vec4 EA[SN.x]; //element array

void insertion_sort(float t, int id);
obj getObj(int id); vec4 saveObj(int i);
bool iscoincidenceEA(int id);
void sort0(int idtemp, int D); void sort1(int idtemp, int D);

float SKernel(float d, float h)
{
    return exp(-(d/h));
}

float Kernel(float d, float h)
{
    return exp(-sqr(d/h))/(PI*sqr(h));
}

float KernelGrad(float d, float h)
{
    return 2.*d*Kernel(d,h)/sqr(h);
}

vec2 borderF(vec2 p)
{
    
    float d = min(min(p.x,p.y),min(R.x-p.x,1e10));
    return exp(-max(d,0.)*max(d,0.))*((d==p.x)?vec2(1,0):(
    		(d==p.y)?vec2(0,1):(
            (d==R.x-p.x)?vec2(-1,0):vec2(0,-1))));
}

void mainImage( out vec4 Q, in vec2 pos )
{
    //4 pix per layer, 3 layers
    sN = SN; 
    N = ivec2(R*P/vec2(sN));
    TN = N.x*N.y;
    int S = 3; //log2(sN.x)
    
    ivec2 p = ivec2(floor(pos));
    if(any(greaterThan(p, sN*N-1))) discard;
   
    ivec3 sid = xy2i(p); ID = sid.x;
    O = getObj(ID);
    d0 = vec4(1e6); d1 = vec4(1e6);
    lnk0 = vec4(-1); lnk1 = vec4(-1);
    
    switch(sid.z)
    {
    case 0: //particle
        if(sid.z >= 3) discard;
        float sk = 0.;
        
        //scale /=sk;
        vec2 F =1e-3*vec2(0.,-1.);//-0.001*(O.X - R*0.5)/(pow(length(O.X - R*0.5),1.)+1.); 
        vec2 Fp = vec2(0);
        float avgP = 0.;
     
         float scale = 0.21/density; //radius of smoothing
        float Div = 0.;
        float Rho = Kernel(0., scale);
           vec2 avgV = vec2(O.V)*Rho;
        vec3 avgCol = vec3(O.Y.xyz);
        float Gsum = 1.;
        float curscale = 1e10;
        float avgSc = 0.;
        
        loop(j,4)
        {
            vec4 nb = texel(ch0, i2xy(ivec3(ID, j, 1)));
            loop(i,4)
            {
                if(nb[i] < 0. || nb[i] > float(TN)) continue;
                obj nbO = getObj(int(nb[i]));
                
               
                float d = distance(O.X, nbO.X);
                vec2 dv = (nbO.V - O.V); //delta velocity
                vec2 dx = (nbO.X - O.X); //delta position 
                vec2 ndir = dx/(d+0.001); //neighbor direction
                //SPH smoothing kernel
                float K = Kernel(d, scale);
                float dK = KernelGrad(d, scale);
               
                //Gkernel
                float G = 1./(d*d+0.01);
                float dotv = dot(ndir, dv); //divergence
                vec2 pressure = -(nbO.Pressure/sqr(nbO.Rho) + 
                                    O.Pressure/sqr(O.Rho))*ndir*K;//pressure gradient
                curscale = min(curscale, d);
                Gsum += 1.;
                Div += dotv*K; // local divergence
                Rho += K;
                avgCol += nbO.Y.xyz;
                avgP += nbO.Pressure*K;
                avgV += nbO.V*K;
                vec2 viscosity = 1.4*(3. + 3.*length(dv))*ndir*dotv*K;
                F += pressure + viscosity;
                Fp -= ndir*SKernel(d,scale);
            }
        }
        
         //border conditions
        vec2 bdf = borderF(O.X);
        F += 0.5*bdf*abs(dot(bdf, O.V));
       // Fp += 0.*bdf*dot(bdf, O.V);
        
        if(R.x - O.X.x < 2.) O.V.x = -abs(O.V.x);
        if(O.X.x < 2.) O.V.x = abs(O.V.x);
        //if(R.y - O.X.y < 2.) O.V.y = -abs(O.V.y);
        if(O.X.y < 2.) O.V.y = abs(O.V.y);
        
        
        if(iMouse.z > 0.) 
        {
            float d = distance(iMouse.xy, O.X);
            O.Y.xyz +=(0.5+0.5*sin(vec3(1,2,3)*iTime))/(0.2*d*d+2.);
            F += 0.01*(iMouse.xy - iMouse.zw)/(0.2*d*d+2.);
        }
        
        O.Rho = Rho;
        O.Div = Div; //average distance
        O.SScale = avgSc/Gsum; //average average distance
        
        float r = 7.;
        float D = 1.;
        float waterP = 0.035*density*(pow(abs(O.Rho/density), r) - D);
        float gasP = 0.03*O.Rho;
        O.Pressure = min(waterP,0.04);
        if(iFrame > 20) O.Pressure += 0.*(avgP/O.Rho - O.Pressure);
        
        
        O.V += F*dt;
        O.V -= O.V*(0.5*tanh(8.*(length(O.V)-1.5))+0.5);
        O.X += (O.V)*dt + 0.*Fp; //advect
        
        
        
        
        
        //color diffusion
        
        O.Y.xyz = 0.995*mix(avgCol/Gsum, O.Y.xyz,0.995)
        + 0.01*(exp(-0.1*distance(O.X,R*0.3))*(0.5*sin(vec3(1,2,3)*iTime)+0.5)
             + exp(-0.1*distance(O.X,R*0.7))*(0.5*sin(vec3(2,3,1)*iTime))+0.5);
        
        
        
        if(iFrame<10)
        {
            O.X = R*vec2(i2xy(ivec3(ID,0,0)))/vec2(N*sN);
            O.X += 0.*sin(10.*O.X.x/R.x)*sin(10.*O.X.y/R.y);
			O.V = 0.*(hash22(3.14159*pos) - 0.5);
            O.Y = texture(ch1,O.X/R);
            O.Pressure = 0.;
            O.Div = 0.;
            O.Rho = 5.;
            O.SScale = 1.;
        }

        Q = saveObj(sid.y);
        return;
        
    case 1: //dir graph
        //sort neighbors and neighbor neighbors
        vec4 nb0 = texel(ch0, i2xy(ivec3(ID, sid.y, 1)));
        
        //random sorts
        loop(i,8) sort0(int(float(TN)*hash13(vec3(iFrame, ID, i))), sid.y);
        
        loop(i,4)
        {
            sort0(int(nb0[i]), sid.y);  //sort this
            //use a sudorandom direction of the neighbor
            vec4 nb1 = texel(ch0, i2xy(ivec3(nb0[i], (iFrame+ID)%4, 1)));
            loop(j,2)
            {
                sort0(int(nb1[j]), sid.y);  
            }
        }
        
        
        
        Q = lnk0;
        return;
    }
     
}

vec4 saveObj(int i)
{
    switch(i)
    {
    case 0:  
        return vec4(O.X, O.V);
    case 1:
        return vec4(O.Pressure, O.Rho, O.SScale, O.Div);
    case 2:
        return O.Y;
    case 3:
        return vec4(0.);
    }
}

obj getObj(int id)
{
    obj o;
    
    vec4 a = texel(ch0, i2xy(ivec3(id, 0, 0))); 
    o.X = a.xy; o.V = a.zw;
    
    a = texel(ch0, i2xy(ivec3(id, 1, 0))); 
    o.Pressure = a.x;
    o.Rho = a.y;
    o.SScale = a.z;
    o.Div = a.w;
    
    o.Y = texel(ch0, i2xy(ivec3(id, 2, 0)));
    
    o.id = id;
    return o;
}

void insertion_sort(float t, int id)
{
	if(d0.x > t)
    {
        d0 = vec4(t, d0.xyz);
        lnk0 = vec4(id, lnk0.xyz);
    }else if(d0.y > t && d0.x < t)
    {
        d0.yzw = vec3(t, d0.yz);
        lnk0.yzw = vec3(id, lnk0.yz);
    }else if(d0.z > t&& d0.y < t)
    {
        d0.zw = vec2(t, d0.z);
        lnk0.zw = vec2(id, lnk0.z);
    }else if(d0.w > t && d0.z < t)
    {
        d0.w = t;
        lnk0.w = float(id);
    }
}

bool iscoincidence(int id)
{
    return (id < 0) || 
      		(id == ID) ||
           any(equal(lnk0,vec4(id)));
}

void sort0(int idtemp, int D) //sort closest objects in sN.x directions
{
    if(iscoincidence(idtemp)) return; //particle already sorted
    
    vec2 nbX = texel(ch0, i2xy(ivec3(idtemp, 0, 0))).xy; 
   
    vec2 dx = nbX - O.X;
    int dir = int(2.*(atan(dx.y, dx.x)+PI)/PI); 
    
    if(dir != D) return; //not in this sector
    
    float t = length(dx);
   
    insertion_sort(t, idtemp);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//voronoi particle tracking + graph augmented

vec2 sc(vec2 p)
{
    return SC*(p - 0.5*R) + 0.5*R;
}

float d;
int id;
vec2 p;

float particleDistance(int i)
{
    return distance(p, sc(texel(ch0, i2xy(ivec3(i, 0, 0))).xy));
}

void sort(int utemp)
{
    if(utemp < 0) return; 
   	float dtemp = particleDistance(utemp);
    if(dtemp < d) //sorting
    {
        d = dtemp;
        id = utemp;
    }
}


void mainImage( out vec4 Q, in vec2 pos )
{
    sN = SN; 
    N = ivec2(R*P/vec2(sN));
    TN = N.x*N.y;
    d = 1e10;
    id = 1;
    p = pos;
    ivec2 pi = ivec2(floor(pos));
    
    sort(1+0*int(texel(ch1, pi).x));
    
   int ID = id;
    loop(j,8)
    {
        
        int nbid = int(texel(ch1, pi+cross_distribution(j)).x);
        sort(nbid);
    }
    
    loop(j,4)
    {
        vec4 nb = texel(ch0, i2xy(ivec3(ID, j, 1)));
        loop(i,4)
    	{ 
            sort(int(nb[i]));  //sort this
        }
    }
    
    loop(i,5) //random sort
    {
        sort(int(float(TN)*hash13(vec3(iFrame, pi.x, pi.y*i))));
    }
    
   	Q = vec4(id, d, 0, 0);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
vec2 sc(vec2 p)
{
    return SC*(p - 0.5*R) + 0.5*R;
}

// iq's smooth HSV to RGB conversion 
vec3 hsv2rgb( in vec3 c )
{
    vec3 rgb = clamp( abs(mod(c.x*6.0+vec3(0.0,4.0,2.0),6.0)-3.0)-1.0, 0.0, 1.0 );

	rgb = rgb*rgb*(3.0-2.0*rgb); // cubic smoothing	

	return c.z * mix( vec3(1.0), rgb, c.y);
}

obj getObj(int id)
{
    obj o;
    
    vec4 a = texel(ch0, i2xy(ivec3(id, 0, 0))); 
    o.X = a.xy; o.V = a.zw;
    
    a = texel(ch0, i2xy(ivec3(id, 1, 0))); 
    o.Pressure = a.x;
    o.Rho = a.y;
    o.SScale = a.z;
    o.Div = a.w;
    
    o.Y = texel(ch0, i2xy(ivec3(id, 2, 0)));
    
    o.id = id;
    return o;
}

void mainImage( out vec4 fragColor, in vec2 pos )
{
    sN = SN; 
    N = ivec2(R*P/vec2(sN));
    TN = N.x*N.y;
    ivec2 pi = ivec2(floor(pos));
    
    int ID = int(texel(ch1, pi).x); 
    obj O = getObj(ID);
    float d =distance(pos, sc(O.X));
    float d1 = exp(-sqr(d/1.)) +  0.*exp(-0.1*d);
    float d2 = 10.*O.Y.x;
   
    /*loop(j,4)
    {
        vec4 nb = texel(ch0, i2xy(ivec3(ID, j, 1)));
        loop(i,4)
    	{
            if(nb[i] < 0.) continue;
            vec2 nbX = texel(ch0, i2xy(ivec3(nb[i], 0, 0))).xy; 
        	d1 += exp(-0.5*distance(pos, sc(nbX)));
    	}
    }*/
    d1*=1.;
    // Output to screen
 	vec3 pcol = texel(ch2, pi).xyz;
    vec3 ncol = 0.*sin(vec3(1,2,3)*d2*0.3)*d1 + hsv2rgb(vec3(atan(O.V.x,O.V.y)/PI, 2.*length(O.V),20.*length(O.V)))*d1;
    fragColor = vec4(mix(pcol,ncol,0.5),1.0);
}