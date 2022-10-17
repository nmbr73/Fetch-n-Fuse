
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define R iResolution
#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//#define texel(a, p) texelFetch(a, to_int2(p), 0)
//#define texel(a, p) texture(a, (make_float2(to_int2(p))+0.5f)/R)
#define texel(a, p) texture(a, (make_float2((p))+0.5f)/R)


#define ch0 iChannel0
#define ch1 iChannel1
#define ch2 iChannel2
#define ch3 iChannel3
//#define R iResolution
#define PI 3.14159265f

#define dt 3.0f
#define loop(i,x) for(int i = _fminf(0, iFrame); i < x; i++)

//rendering scale
#define SC 1.0f

#define smoothR 2.5f
#define density 0.036f

//sim stuff
struct obj
{
    int id; //ID
    float2 X; //position
    float2 V; //velocity
    float Pressure; //pressure
    float Rho; //neighbor density
    float SScale; //smooth scale
    float Div; //average distance to neighbors
    float4 Y; //additional data
};
    
__DEVICE__ float Force(float d)
{
    return 0.2f*_expf(-0.05f*d)-2.0f*_expf(-0.5f*d);
}

//40% of the buffer used for particles
#define P 0.5f
#define SN to_int2(4, 2)

__DEVICE__ int2 N; //buffer size
__DEVICE__ int2 sN; //buffer single element size
__DEVICE__ int TN; //buffer length

__DEVICE__ int2 i2xy(int3 sid)
{
    return sN*to_int2(sid.x%N.x, sid.x/N.x) + to_int2(sid.y,sid.z);
}

__DEVICE__ int3 xy2i(int2 p)
{
    int2 pi = to_int2(p.x/sN.x,p.y/sN.y);
    return to_int3(pi.x + pi.y*N.x, p.x%sN.x, p.y%sN.y);
}

__DEVICE__ int2 cross_distribution(int i)
{
    return (1<<(i/4)) * to_int2( ((i&2)/2)^1, (i&2)/2 ) * ( 2*(i%2) - 1 );
}

__DEVICE__ float sqr(float _x)
{
    return _x*_x + 1e-2;
}

//hash funcs
__DEVICE__ float2 hash22(float2 p)
{
  float3 p3 = fract_f3((swi3(p,x,y,x)) * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
  return fract_f2((swi2(p3,x,x)+swi2(p3,y,z))*swi2(p3,z,y));
}

__DEVICE__ float hash13(float3 p3)
{
  p3  = fract_f3(p3 * 0.1031f);
  p3 += dot(p3, swi3(p3,y,z,x) + 33.33f);
  return fract((p3.x + p3.y) * p3.z);
}

__DEVICE__ float3 hash33(float3 p3)
{
  p3 = fract_f3(p3 * to_float3(0.1031f, 0.1030f, 0.0973f));
  p3 += dot(p3, swi3(p3,y,x,z)+33.33f);
  return fract_f3((swi3(p3,x,x,y) + swi3(p3,y,x,x))*swi3(p3,z,y,x));
}

__DEVICE__ float3 hash31(float p)
{
   float3 p3 = fract_f3(to_float3_s(p) * to_float3(0.1031f, 0.1030f, 0.0973f));
   p3 += dot(p3, swi3(p3,y,z,x)+33.33f);
   return fract_f3((swi3(p3,x,x,y)+swi3(p3,y,z,z))*swi3(p3,z,y,x)); 
}
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//L1 particle buffer - simulation
//L2 directional neighbor graph 4x - sort

//int ID;
//obj O; //this object

//sort arrays
//float4 lnk0, lnk1;
//float4 d0, d1;

//L3
//float4 EA[SN.x]; //element array   -> not used

__DEVICE__ float4 saveObj(int i, obj O)
{
  
    switch(i)
    {
    case 0:  
        return to_float4_f2f2(O.X, O.V);
    case 1:
        return to_float4(O.Pressure, O.Rho, O.SScale, O.Div);
    case 2:
        return O.Y;
    case 3:
        return to_float4_s(0.0f);
    }
}

__DEVICE__ obj getObj(int id, float2 R, __TEXTURE2D__ iChannel0)
{
    obj o;
    
    float4 a = texel(ch0, i2xy(to_int3(id, 0, 0))); 
    o.X = swi2(a,x,y); o.V = swi2(a,z,w);
    
    a = texel(ch0, i2xy(to_int3(id, 1, 0))); 
    o.Pressure = a.x;
    o.Rho = a.y;
    o.SScale = a.z;
    o.Div = a.w;
    
    o.Y = texel(ch0, i2xy(to_int3(id, 2, 0)));
    
    o.id = id;
    return o;
}

__DEVICE__ void insertion_sort(float t, int id, inout float4 *d0, inout float4 *lnk0)
{
  float ssssssssssssssssssssss;
  if((*d0).x > t)
    {
        *d0 = to_float4(t, (*d0).x,(*d0).y,(*d0).z);
        *lnk0 = to_float4(id, (*lnk0).x,(*lnk0).y,(*lnk0).z);
    }else if((*d0).y > t && (*d0).x < t)
    {
        swi3S(*d0,y,z,w, to_float3(t, (*d0).y,(*d0).z));
        swi3S(*lnk0,y,z,w, to_float3(id, (*lnk0).y,(*lnk0).z));
    }else if((*d0).z > t&& (*d0).y < t)
    {
        swi2S(*d0,z,w, to_float2(t, (*d0).z));
        swi2S(*lnk0,z,w, to_float2(id, (*lnk0).z));
    }else if((*d0).w > t && (*d0).z < t)
    {
        (*d0).w = t;
        (*lnk0).w = (float)(id);
    }
}

__DEVICE__ bool iscoincidence(int id, float4 lnk0, int ID)
{
    return (id < 0) || 
          (id == ID) ||
           (lnk0.x == (float)(id) || lnk0.y == (float)(id) || lnk0.z == (float)(id )|| lnk0.w == (float)(id));
           //any(equal(lnk0,to_float4_s(id)));
}

__DEVICE__ void sort0(int idtemp, int D, inout float4 *d0, inout float4 *lnk0, int ID, obj O, float2 R, __TEXTURE2D__ iChannel0) //sort closest objects in sN.x directions
{
    if(iscoincidence(idtemp, *lnk0, ID)) return; //particle already sorted
    
    float2 nbX = swi2(texel(ch0, i2xy(to_int3(idtemp, 0, 0))),x,y); 
   
    float2 dx = nbX - O.X;
    int dir = (int)(2.0f*(_atan2f(dx.y, dx.x)+PI)/PI); 
    
    if(dir != D) return; //not in this sector
    
    float t = length(dx);
float ttttttttttttttttttttt;   
    insertion_sort(t, idtemp,d0,lnk0);
}

__DEVICE__ float SKernel(float d, float h)
{
    return _expf(-(d/h));
}

__DEVICE__ float Kernel(float d, float h)
{
    return _expf(-sqr(d/h))/(PI*sqr(h));
}

__DEVICE__ float KernelGrad(float d, float h)
{
    return 2.0f*d*Kernel(d,h)/sqr(h);
}

__DEVICE__ float2 borderF(float2 p, float2 R)
{
    
    float d = _fminf(min(p.x,p.y),_fminf(R.x-p.x,1e10));
    return _expf(-_fmaxf(d,0.0f)*_fmaxf(d,0.0f))* ((d==p.x) ? to_float2(1,0):(
            (d==p.y) ? to_float2(0,1):(
            (d==R.x-p.x)?to_float2(-1,0):to_float2(0,-1))));
}

__KERNEL__ void SuperSphFuse__Buffer_A(float4 Q, float2 pos, float2 iResolution, float iTime, float4 iMouse, int iFrame)
{
    pos+=0.5f;
    //4 pix per layer, 3 layers
    sN = SN; 
    N = to_int2_cfloat(R*P/make_float2(sN));
    TN = N.x*N.y;
    int S = 3; //_log2f(sN.x)
    
    int2 p = to_int2_cfloat(_floor(pos));
    //if(any(greaterThan(p, sN*N-1)))  {SetFragmentShaderComputedColor(Q); return;} //    discard;
   
    if( (p.x > sN.x*N.x-1) || (p.y > sN.y*N.y-1) ) 
    {
      SetFragmentShaderComputedColor(Q); 
      return;
    }//discard; 
   
    int3 sid = xy2i(p); int ID = sid.x;
    obj O = getObj(ID,R,iChannel0);
    float4 d0 = to_float4_s(1e6); float4 d1 = to_float4_s(1e6);
    float4 lnk0 = to_float4_s(-1); float4 lnk1 = to_float4_s(-1);
    
    switch(sid.z)
    {
    case 0: //particle
        if(sid.z >= 3) { SetFragmentShaderComputedColor(Q); return;}//    discard;
        float sk = 0.0f;
     
        //scale /=sk;
        float2 F =1e-3*to_float2(0.0f,-1.0f);//-0.001f*(O.X - R*0.5f)/(_powf(length(O.X - R*0.5f),1.0f)+1.0f); 
        float2 Fp = to_float2_s(0);
        float avgP = 0.0f;
     
        float scale = 0.21f/density; //radius of smoothing
        float Div = 0.0f;
        float Rho = Kernel(0.0f, scale);
        float2 avgV = (O.V)*Rho;
        float3 avgCol = swi3(O.Y,x,y,z);
        float Gsum = 1.0f;
        float curscale = 1e10;
        float avgSc = 0.0f;
float AAAAAAAAAAAAAAAAAAAAA;           
        loop(j,4)
        {
            float4 _nb = texel(ch0, i2xy(to_int3(ID, j, 1)));
            float nb[4] = {_nb.x,_nb.y,_nb.z,_nb.w};
            loop(i,4)
            {
                if(nb[i] < 0.0f || nb[i] > (float)(TN)) continue;
                obj nbO = getObj((int)(nb[i]),R,iChannel0);
                
               
                float d = distance_f2(O.X, nbO.X);
                float2 dv = (nbO.V - O.V); //delta velocity
                float2 dx = (nbO.X - O.X); //delta position 
                float2 ndir = dx/(d+0.001f); //neighbor direction
                //SPH smoothing kernel
                float K = Kernel(d, scale);
                float dK = KernelGrad(d, scale);
               
                //Gkernel
                float G = 1.0f/(d*d+0.01f);
                float dotv = dot(ndir, dv); //divergence
                float2 pressure = -(nbO.Pressure/sqr(nbO.Rho) + 
                                    O.Pressure/sqr(O.Rho))*ndir*K;//pressure gradient
                curscale = _fminf(curscale, d);
                Gsum += 1.0f;
                Div += dotv*K; // local divergence
                Rho += K;
                avgCol += swi3(nbO.Y,x,y,z);
                avgP += nbO.Pressure*K;
                avgV += nbO.V*K;
                float2 viscosity = 1.4f*(3.0f + 3.0f*length(dv))*ndir*dotv*K;
                F += pressure + viscosity;
                Fp -= ndir*SKernel(d,scale);
            }
        }
        
         //border conditions
        float2 bdf = borderF(O.X,R);
        F += 0.5f*bdf*_fabs(dot(bdf, O.V));
       // Fp += 0.0f*bdf*dot(bdf, O.V);
        
        if(R.x - O.X.x < 2.0f) O.V.x = -_fabs(O.V.x);
        if(O.X.x < 2.0f) O.V.x = _fabs(O.V.x);
        //if(R.y - O.X.y < 2.0f) O.V.y = -_fabs(O.V.y);
        if(O.X.y < 2.0f) O.V.y = _fabs(O.V.y);
        
        
        if(iMouse.z > 0.0f) 
        {
            float d = distance_f2(swi2(iMouse,x,y), O.X);
            swi3S(O.Y,x,y,z, swi3(O.Y,x,y,z) + (0.5f+0.5f*sin_f3(to_float3(1,2,3)*iTime))/(0.2f*d*d+2.0f));
            F += 0.01f*(swi2(iMouse,x,y) - swi2(iMouse,z,w))/(0.2f*d*d+2.0f);
        }
        
        O.Rho = Rho;
        O.Div = Div; //average distance
        O.SScale = avgSc/Gsum; //average average distance
        
        float r = 7.0f;
        float D = 1.0f;
        float waterP = 0.035f*density*(_powf(_fabs(O.Rho/density), r) - D);
        float gasP = 0.03f*O.Rho;
        O.Pressure = _fminf(waterP,0.04f);
        if(iFrame > 20) O.Pressure += 0.0f*(avgP/O.Rho - O.Pressure);
        
        
        O.V += F*dt;
        O.V -= O.V*(0.5f*_tanhf(8.0f*(length(O.V)-1.5f))+0.5f);
        O.X += (O.V)*dt + 0.0f*Fp; //advect
        
        
        //color diffusion
        
        swi3S(O.Y,x,y,z, 0.995f*_mix(avgCol/Gsum, swi3(O.Y,x,y,z),0.995f)
             + 0.01f*(_expf(-0.1f*distance_f2(O.X,R*0.3f))*(0.5f*sin_f3(to_float3(1,2,3)*iTime)+0.5f)
             + _expf(-0.1f*distance_f2(O.X,R*0.7f))*(0.5f*sin_f3(to_float3(2,3,1)*iTime))+0.5f));
        
        
        
        if(iFrame<10)
        {
          float ffffffffffffffffffffffff; 
            O.X = R * make_float2(i2xy(to_int3(ID,0,0)))/make_float2(N*sN);
            O.X += 0.0f*_sinf(10.0f*O.X.x/R.x)*_sinf(10.0f*O.X.y/R.y);
            O.V = 0.0f*(hash22(3.14159f*pos) - 0.5f);
            O.Y = texture(ch1,O.X/R);
            O.Pressure = 0.0f;
            O.Div = 0.0f;
            O.Rho = 5.0f;
            O.SScale = 1.0f;
        }

        Q = saveObj(sid.y,O);
        SetFragmentShaderComputedColor(Q);     
        return;
        
    case 1: //dir graph
        //sort neighbors and neighbor neighbors
        float4 _nb0 = texel(ch0, i2xy(to_int3(ID, sid.y, 1)));
        float nb0[4] = {_nb0.x,_nb0.y,_nb0.z,_nb0.w};
        
        //random sorts
        loop(i,8) sort0((int)((float)(TN)*hash13(to_float3(iFrame, ID, i))), sid.y,&d0, &lnk0,ID,O,R,iChannel0);
        
        loop(i,4)
        {
            sort0((int)(nb0[i]), sid.y,&d0, &lnk0,ID,O,R,iChannel0);  //sort this
            //use a sudorandom direction of the neighbor
            float4 _nb1 = texel(ch0, i2xy(to_int3(nb0[i], (iFrame+ID)%4, 1)));
            float nb1[4] = {_nb1.x,_nb1.y,_nb1.z,_nb1.w};
            loop(j,2)
            {
                sort0((int)(nb1[j]), sid.y,&d0, &lnk0,ID,O,R,iChannel0);  
            }
        }
       
        Q = lnk0;
        
        SetFragmentShaderComputedColor(Q);     
        return;
        
    }
    
  SetFragmentShaderComputedColor(Q);     
}



// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


//voronoi particle tracking + graph augmented

__DEVICE__ float2 sc(float2 p, float2 R)
{
    return SC*(p - 0.5f*R) + 0.5f*R;
}


__DEVICE__ float particleDistance(float2 p, int i, float2 R, __TEXTURE2D__ iChannel0)
{
    return distance_f2(p, sc(swi2(texel(ch0, i2xy(to_int3(i, 0, 0))),x,y),R));
}

__DEVICE__ void sort(inout float *d, inout int *id, float2 p, int utemp, float2 R, __TEXTURE2D__ iChannel0)
{
    if(utemp < 0) return; 
    float dtemp = particleDistance(p,utemp,R,iChannel0);
    if(dtemp < *d) //sorting
    {
      *d = dtemp;
      *id = utemp;
    }
}


__KERNEL__ void SuperSphFuse__Buffer_B(float4 Q, float2 pos, float2 iResolution, int iFrame)
{
    pos+=0.5f;
float BBBBBBBBBBBBBBBBBB;

    sN = SN; 
    N = to_int2_cfloat(R*P/make_float2(sN));
    TN = N.x*N.y;
    float d = 1e10;
    int id = 1;
    float2 p = pos;
    int2 pi = to_int2_cfloat(_floor(pos));
    
    sort(&d, &id,p,1+0*(int)(texel(ch1, pi).x),R,iChannel0);
    
    int ID = id;
    loop(j,8)
    {
      int nbid = (int)(texel(ch1, pi+cross_distribution(j)).x);
      sort(&d, &id,p,nbid,R,iChannel0);
    }
    
    loop(j,4)
    {
      float4 _nb = texel(ch0, i2xy(to_int3(ID, j, 1)));
      float nb[4] = { _nb.x,_nb.y,_nb.z,_nb.w};
      loop(i,4)
      { 
         sort(&d, &id,p,(int)(nb[i]),R,iChannel0);  //sort this
      }
    }
    
    loop(i,5) //random sort
    {
        sort(&d, &id,p,int(float(TN)*hash13(to_float3(iFrame, pi.x, pi.y*i))),R,iChannel0);
    }
    
    Q = to_float4(id, d, 0, 0);

  SetFragmentShaderComputedColor(Q);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2

#ifdef XXX
__DEVICE__ float2 sc(float2 p, float2 R)
{
    return SC*(p - 0.5f*R) + 0.5f*R;
}
#endif

// iq's smooth HSV to RGB conversion 
__DEVICE__ float3 hsv2rgb( in float3 c )
{
  float3 rgb = clamp( abs_f3(mod_f3(c.x*6.0f+to_float3(0.0f,4.0f,2.0f),6.0f)-3.0f)-1.0f, 0.0f, 1.0f );

  rgb = rgb*rgb*(3.0f-2.0f*rgb); // cubic smoothing  

  return c.z * _mix( to_float3_s(1.0f), rgb, c.y);
}

#ifdef XXX
__DEVICE__ obj getObj(int id, float2 R, __TEXTURE2D__ ch0)
{
    obj o;
float zzzzzSCHEISSSSSSSSEEEEEEzzzzzzzzzzzzzzz;    
    float4 a = texel(ch0, i2xy(to_int3(id, 0, 0))); 
    o.X = swi2(a,x,y); o.V = swi2(a,z,w);
    
    a = texel(ch0, i2xy(to_int3(id, 1, 0))); 
    o.Pressure = a.x;
    o.Rho = a.y;
    o.SScale = a.z;
    o.Div = a.w;
    
    o.Y = texel(ch0, i2xy(to_int3(id, 2, 0)));
    
    o.id = id;
    return o;
}
#endif

__KERNEL__ void SuperSphFuse__Buffer_C(float4 fragColor, float2 pos, float2 iResolution)
{
    pos+=0.5f;
float CCCCCCCCCCCCCCCCCCC;
    sN = SN; 
    N = to_int2_cfloat(R*P/make_float2(sN));
    TN = N.x*N.y;
    int2 pi = to_int2_cfloat(_floor(pos));
    
    int ID = int(texel(ch1, pi).x); 
    obj O = getObj(ID, R,iChannel0);
    float d =distance_f2(pos, sc(O.X,R));
    float d1 = _expf(-sqr(d/1.0f)) +  0.0f*_expf(-0.1f*d);
    float d2 = 10.0f*O.Y.x;
   
    /*loop(j,4)
    {
        float4 nb = texel(ch0, i2xy(to_int3(ID, j, 1)));
        loop(i,4)
      {
            if(nb[i] < 0.0f) continue;
            float2 nbX = texel(ch0, i2xy(to_int3(nb[i], 0, 0))).xy; 
          d1 += _expf(-0.5f*distance(pos, sc(nbX,R)));
      }
    }*/
    d1*=1.0f;
    // Output to screen
    float3 pcol = swi3(texel(ch2, pi),x,y,z);
    float3 ncol = 0.0f*sin_f3(to_float3(1,2,3)*d2*0.3f)*d1 + hsv2rgb(to_float3(_atan2f(O.V.x,O.V.y)/PI, 2.0f*length(O.V),20.0f*length(O.V)))*d1;
    fragColor = to_float4_aw(_mix(pcol,ncol,0.5f),1.0f);

  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


__KERNEL__ void SuperSphFuse(float4 fragColor, float2 pos, float2 iResolution)
{
   pos+=0.5f;

float IIIIIIIIIIIIIIIIIIII;
    sN = SN; 
    N = to_int2_cfloat(R*P/make_float2(sN));
    TN = N.x*N.y;
    int2 pi = to_int2_cfloat(_floor(pos));
    
    fragColor = texel(ch2, pi);

  SetFragmentShaderComputedColor(fragColor);
}
