
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//select style here:
#define cube to_float3(0.0f,0.0f,0.99f)  // contour
//#define cube to_float3_s(0.0f)  // smooth
//#define cube to_float3_s(0.99f)  // voxel

const float N = 80.0f; //number of voxel x side (TOT=N^3)
#define TRI  


//-------------------------------
float3 offset = _floor(to_float3(0.0f,0.0f,0.0f));
const float2 packedChunkSize= to_float2(5,3);// _ceil(_sqrtf(N) * to_float2(1.25f ,0.8f)); // iResolution/swi2(iResolution,y,x) 
const float heightLimit = packedChunkSize.x * packedChunkSize.y; //must be > N
#define R iResolution
//-----------------------------------------
// VOXEL CACHE FUNCTIONS from fb39ca4

__DEVICE__ float2 unswizzleChunkCoord(float2 storageCoord) {
    float2 s = _floor(storageCoord);
    float dist = _fmaxf(s.x, s.y);
    float o = _floor(dist / 2.0f);
    float neg = step(0.5f, mod_f(dist, 2.0f)) * 2.0f - 1.0f;
    return neg * (s - o);
}

__DEVICE__ float2 swizzleChunkCoord(float2 chunkCoord) {
    float2 c = chunkCoord;
    float dist = _fmaxf(_fabs(c.x), _fabs(c.y));
    float2 c2 = _floor(_fabs(c - 0.5f));
    float o = _fmaxf(c2.x, c2.y);
    float neg = step(c.x + c.y, 0.0f) * -2.0f + 1.0f;
    return (neg * c) + o;
}

__DEVICE__ float calcLoadDist(float2 iResolutionxy) {
    float2  chunks = _floor(iResolutionxy / packedChunkSize); 
    float gridSize = _fminf(chunks.x, chunks.y);    
    return _floor((gridSize - 1.0f) / 2.0f);
}

__DEVICE__ float3 texToVoxCoord(float2 textelCoord, float3 offset) {

    float2 packedChunkSize= packedChunkSize;
    float3 voxelCoord = offset;
    swi2(voxelCoord,x,y) += unswizzleChunkCoord(textelCoord / packedChunkSize);
    voxelCoord.z += mod_f(textelCoord.x, packedChunkSize.x) + packedChunkSize.x * mod_f(textelCoord.y, packedChunkSize.y);
    return voxelCoord;
}

__DEVICE__ float2 voxToTexCoord(float3 voxCoord) {
    float2 packedChunkSize= packedChunkSize;
    float3 p = _floor(voxCoord);
    return swizzleChunkCoord(swi2(p,x,y)) * packedChunkSize + to_float2(mod_f(p.z, packedChunkSize.x), _floor(p.z / packedChunkSize.x));
}

__DEVICE__ float4  getVoxel(float3 p, __TEXTURE2D__ iChannel, float2 iResolution) {
    p.z-= offset.z;
    if(p.z>heightLimit || p.z<0.0f){return to_float4_s(0.0f);}  
    //return texelFetch(iChannel, to_int2(voxToTexCoord(p))  , 0); 
    return texture(iChannel, (make_float2(to_int2(voxToTexCoord(p)))+0.5f)/iResolution); 

}



//-----------Intersection functions--------------------
#define NOHIT 1e5
struct its
{
  float t;
  float3 n;    //normal 
  float3 fuv;  //face & uv 
};
//const its  NO_its = its(NOHIT,to_float3_s(0.0f),to_float3_s(0.0f));
#define NO_its  {NOHIT,to_float3_s(0.0f),to_float3_s(0.0f)}

struct span
{
  its n;
  its f;
};

struct RayOut{   
    float d;
    float3 n;
    float3 fuv; //unused here
    float id;
    float d2;
};

RayOut FastUnion(  RayOut a, RayOut b)
{
   if(a.d<b.d) return a;
   else return b;
}
#define Add(_ro,_func) _ro = FastUnion(_ro,_func);

__DEVICE__ RayOut InfRay(){ RayOut ret = {NOHIT,to_float3_s(0),to_float3_s(0),0.0f,0.0f}; return ret; //RayOut(NOHIT,to_float3_s(0),to_float3_s(0),0.0f,0.0f);}
__DEVICE__ RayOut Ray(span s, float mat){    
    if(s.f.t<0.0f ||(s.f.t==NOHIT && s.n.t<0.0f )) return  InfRay();
    if(s.n.t<0.0f && s.f.t>0.0f  )
    {
      RayOut ret = {0.001f,to_float3_s(0),to_float3_s(0),mat,s.f.t};
      return ret; //RayOut(0.001f,to_float3_s(0),to_float3_s(0),mat,s.f.t);
      
    }
    RayOut ret = { s.n.t,s.n.n,s.n.fuv,mat,s.f.t-s.n.t};
    return ret; //RayOut( s.n.t,s.n.n,s.n.fuv,mat,s.f.t-s.n.t);
}

__DEVICE__ span iSphere( in float3 ro, in float3 rd, float ra )
{

    float3 oc = ro ;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - ra*ra;
    float h = b*b - c;
    if( h<0.0f ) 
    {
      span ret = {NO_its,NO_its}; 
      return span(NO_its,NO_its); // no intersection
    }
    h = _sqrtf( h );
    float3 oNor =normalize(ro-(b+h)*rd); 
    float3 oFuv=to_float3(0.0f,_atan2f(oNor.y,length(swi2(oNor,x,z))),_atan2f(oNor.z,oNor.x))*ra*1.5708f  ;
    float3 fNor= normalize(ro-(b-h)*rd); 
    float3 fFuv=to_float3(0.0f,_atan2f(fNor.y,length(swi2(fNor,x,z))),_atan2f(fNor.z,fNor.x))*ra*1.5708f  ;
    if( h-b < 0.0f )
    {
      spanret = {NO_its,NO_its};
      return  ret; // span(NO_its,NO_its);
    }
    span ret = {{-b-h,oNor,oFuv} , {-b+h,-fNor,fFuv}};
    return ret; // span(its(-b-h,oNor,oFuv) , its(-b+h,-fNor,fFuv));
}

__DEVICE__ span iBox( in float3 ro, in float3 rd, float3 boxSize) 
{
    float3 m = 1.0f/rd; 
    float3 n = m*ro;   
    float3 k = abs_f3(m)*boxSize;

    float3 t1 = -n - k;
    float3 t2 = -n + k;
    float tN = _fmaxf( _fmaxf( t1.x, t1.y ), t1.z );
    float tF = _fminf( _fminf( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.0f) 
    {
      spanret = {NO_its,NO_its};
      return  ret; // span(NO_its,NO_its);
    }
float zzzzzzzzzzzzzzzzz;    
    float3 oNor = -sign_f3(rd)*step(swi3(t1,y,z,x),swi3(t1,x,y,z))*step(swi3(t1,z,x,y),swi3(t1,x,y,z)); 
    float3 oFuv=to_float3( dot(_fabs(oNor),to_float3(1,5,9)+ oNor)/2.0f,dot(ro+rd*tN,swi3(oNor,z,x,y)),dot(ro+rd*tN,swi3(oNor,y,z,x)));      
    float3  fNor=- sign_f3(rd)*step(swi3(t2,x,y,z),swi3(t2,y,z,x))*step(swi3(t2,x,y,z),swi3(t2,z,x,y)); 
    float3 fFuv=to_float3( dot(_fabs(fNor),to_float3(1,5,9)+ fNor)/2.0f,dot(ro+rd*tF,swi3(fNor,z,x,y)),dot(ro+rd*tF,swi3(fNor,y,z,x)));
    
    span ret = {{tN,oNor,oFuv} , {tF,fNor,fFuv}};
    return ret; // span(its(tN,oNor,oFuv) , its(tF,fNor,fFuv));
}

__DEVICE__ span iPlane( in float3 ro, in float3 rd, in float3 n ,float h)
{
    float d= -(dot(ro,n)+h)/dot(rd,n);
    float3  u = normalize(cross(n,to_float3(0,0,1))), v = normalize(cross(u,n) );
    float3 oFuv=to_float3(1.0f,dot(ro+d*rd,u), dot(ro+d*rd,v));
    float3 oNor=n;
    if(d<0.0f)
    {
      spanret = {NO_its,NO_its};
      return  ret; // span(NO_its,NO_its);
    }
    span ret = {{d,oNor,oFuv},NO_its};
    return ret; //span(its(d,oNor,oFuv),NO_its);
}

__DEVICE__ span iCylinder( in float3 ro, in float3 rd,  in float3 pa, in float3 pb, float ra ) // extreme a, extreme b, 
{
    span no_hit =  {NO_its,NO_its};

    float3 ba = pb-pa;

    float3  oc = ro - pa;

    float baba = dot(ba,ba);
    float bard = dot(ba,rd);
    float baoc = dot(ba,oc);
    
    float k2 = baba            - bard*bard;
    float k1 = baba*dot(oc,rd) - baoc*bard;
    float k0 = baba*dot(oc,oc) - baoc*baoc - ra*ra*baba;
    
    float h = k1*k1 - k2*k0;
    if( h<0.0f )  return no_hit;
    h = _sqrtf(h);
    
    float t = (-k1-h)/k2;
    float y = baoc + t*bard; 
    float t2 = ( ((y<0.0f) ? 0.0f : baba) - baoc)/bard;   
    
    if( (y<0.0f || y>baba )  && _fabs(k1+k2*t2)>h) 
    {
      spanret = {NO_its,NO_its};
      return  ret; // span(NO_its,NO_its);
    }
    its iN,iF;
    float3 w = normalize(ba), u = normalize(cross(ba,to_float3(0,0,1))), v = normalize(cross(u,w) );
    
    //entry point
    float3 q = (oc+t*rd-ba)*mat3(u,v,w);   
    if( y>0.0f && y<baba ) iN= its( t, (oc+t*rd - ba*y/baba)/ra,to_float3(0.0f,_atan2f(q.y,q.x)*ra,q.z) ); 
    else iN= its(t2, ba*sign(y)/baba ,to_float3(1.0f,dot(oc+t2*rd-ba,u) ,dot(oc+t2*rd-ba,v) )); 
        
  
    //exit point
    t = (-k1+ h)/k2;
    y = baoc + t*bard; 
    t2 = ( ((y<0.0f) ? 0.0f : baba) - baoc)/bard;
    q = mul_f3_mat3((oc+t*rd-ba) , to_mat3_f3(u,v,w)); 
    if( y>0.0f && y<baba ) iF= its( t, (oc+t*rd - ba*y/baba)/ra,to_float3(0.0f,_atan2f(q.y,q.x)*ra,q.z) ); 
    else iF= its(t2, ba*sign_f(y)/baba ,to_float3(1.0f,dot(oc+t2*rd-ba,u) ,dot(oc+t2*rd-ba,v) )); 
    
    span ret = {iN , iF};
    return ret;//span(iN , iF);
  
}
__DEVICE__ float cross2D(in float2 a, in float2 b) { return a.y * b.x - a.x * b.y; }

//from https://www.shadertoy.com/view/Ns2SzG by Oneshade
__DEVICE__ span iBilinearPatch(in float3 ro, in float3 rd,in float3 a, in float3 b, in float3 c, in float3 d) {
    //bool tr=false;
    if(_fabs(rd.x)<0.0001f){  //not working if rd.x==0.
      {
        span ret = {its(0.01f,to_float3_s(0),to_float3_s(0)),NO_its};
        return ret; //span(its(0.01f,to_float3_aw(0),to_float3(0)),NO_its);
        //tr=true; ro=swi3(ro,y,z,x);rd=swi3(rd,y,z,x);a=swi3(a,y,z,x);b=swi3(b,y,z,x);c=swi3(c,y,z,x);d=swi3(d,y,z,x);
      }
    }
    float2 m = swi2(rd,y,z) / rd.x;
    if(length(a - b + c - d)==0.0f)  a+=to_float3_s(0.001f); //fix coplanar issue
    float3 p = a - b + c - d, q = d - a, r = b - a;
    float2 c1 = swi2(p,y,z) - p.x * m, c2 = swi2(q,y,z) - q.x * m, c3 = swi2(r,y,z) - r.x * m;
    float2 c4 = (ro.x - a.x) * m + swi2(a,y,z) - swi2(ro,y,z);

    // Quadratic coefficients (reversed as a trick to deal with edge cases)
    float qa = cross2D(c4, c2);
    float qb = cross2D(c4, c1) + cross2D(c3, c2);
    float qc = cross2D(c3, c1);
   
    float discr = (qb * qb - 4.0f * qa * qc);
    span s=span(NO_its,NO_its);
    if (discr > 0.0f) {
        float2 v = 2.0f * qa / (to_float2(-1.0f, 1.0f) * _sqrtf(discr) - qb); // Solve quadratic in v
        float2 u = -(c4.x + c3.x * v) / (c1.x * v + c2.x);
        float2 t = (p.x * u * v + q.x * u + r.x * v + a.x - ro.x) / rd.x;

        // Get closest intersection in view and add it to the scene        
        if (_fabs(u[0] - 0.5f) < 0.5f && _fabs(v[0] - 0.5f) < 0.5f) 
             s.n= its(t[0],normalize(cross(p * v[0] + q, p * u[0] + r)),to_float3(0.0f,u[0], v[0]));
        if ( _fabs(u[1] - 0.5f) < 0.5f && _fabs(v[1] - 0.5f) < 0.5f) 
             s.f = its(t[1],normalize(cross(p * v[1] + q, p * u[1] + r)),to_float3(0.0f,u[1], v[1])); 
        if (s.f.t<s.n.t){ its tp=s.n;s.n=s.f;s.f=tp;}     
        s.n.n*=-sign(dot(s.n.n,rd));
        s.f.n*=-sign(dot(s.f.n,rd));
    }
    //if(tr) {s.n.n=s.n.swi3(n,z,x,y);s.f.n=s.f.swi3(n,z,x,y);}
    return s;
}

//based on Iq's
span triIntersect( in float3 ro, in float3 rd, in float3 v0, in float3 v1, in float3 v2 )
{
    float3 v1v0 = v1 - v0;
    float3 v2v0 = v2 - v0;
    float3 rov0 = ro - v0;

    float3  n = cross( v1v0, v2v0 );
    float3  q = cross( rov0, rd );
    float d = 1.0f/dot( rd, n );
    float u = d*dot( -q, v2v0 );
    float v = d*dot(  q, v1v0 );
    float t = d*dot( -n, rov0 );   

    if( u<0.0f || v<0.0f || (u+v)>1.0f ) t = -1.0f;
    
//    return to_float3( t, u, v );

     return  span(its(t,-n,to_float3_aw(0)),NO_its);
}



// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Texture: London' to iChannel1
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0


//ISOSURFACE FUNCTION  (USE YOU OWN HERE)

__DEVICE__ float map(float3 p)
{   

   float d= p.z/2.0f-   length(texture(iChannel1,p.xy/80.0f,0.0f).xyz)*2.5f-0.5f;  
   d=step(0.0f,d)*2.0f-1.0f;  //works also with 1 bit per voxel     
    return d;
}


__KERNEL__ void TerrainMeshJipiFuse__Buffer_A(float4 O, float2 U, sampler2D iChannel0, sampler2D iChannel1)
{
   
    float3 c = texToVoxCoord(_floor(U), offset); 
    float data =map(c); //not stateful here, otherwise use getVoxel( c ,iChannel0);
    O = to_float4(data);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1


// SURFACE NET BUFFER




 struct GRIDCELL{     //calculated for each cube in the following sequence:
   float3 p[8];         //  1.0f cube vertex positions
   float val[8];      //  2.0f isosurface value at each cube vertex
   float nVertex;     //  3.0f number of vertex where val[]>0; if 0<nVertex<8  => surface cube
   float nEdge;       //  4.0f number of cube edges with crossing 
   float3 m;            //  5.0f surface vertex, as an average of all crossing edge positions
   int dirs;          //  6.0f 6bit bitmask of surface edges from each cube;                        
} ;


const float3 v[8] =vec3[8]
(
   to_float3(0,0,0), to_float3(1,0,0),to_float3(1,1,0),to_float3(0,1,0),
   to_float3(0,0,1), to_float3(1,0,1),to_float3(1,1,1),to_float3(0,1,1)
);

const int  e[24] =int[24](
   0,1,   1,2,  2,3,   3,0, 
   4,5,   5,6,  6,7,   7,4,   
   0,4,   1,5,  2,6,   3,7);

const float3 dir[6] =vec3[6]
(
   to_float3(1,0,0), to_float3(0,1,0),to_float3(0,0,1),
   to_float3(-1,0,0), to_float3(0,-1,0),to_float3(0,0,-1)
);


int gFrame=0; 
__DEVICE__ void  getSurface(float3 c, inout GRIDCELL g)
{
    
    g.m=to_float3_s(0.0f);
    g.nVertex=0.0f;
    g.nEdge=0.0f;
    g.dirs=0;

    //gFrame unrolling fails here...
    for(int i=0;i<8;i++)
    {

        //1.0f cube vertex positions
       
        float3 vp=c+ cube/2.0f+ v[i]*(1.0f-cube);
        g.p[i]=vp;

        //  2.0f isosurface value at each cube vertex
        float val = //map(vp);
                    getVoxel( c+  v[i] ,iChannel0).x;
         g.val[i]=val;
        
        //3.0f number of vertex where val[]>0;
        g.nVertex+= (val<=0.?1.:0.);

    }
 
     if(g.nVertex>0.0f && g.nVertex<8.0f)
     {

          for(int i=gFrame;i<24;i+=2)
          {
              //  isosurface weights at each cube edge vertexes
              float d1 = g.val[e[i]],
                  d2 = g.val[e[i+1]],
                  d= d1/(d1-d2);

            //  4.0f number of cube edges with crossing 
             if(d1*d2<0.0f){
                 g.nEdge++;
                 g.m+= g.p[e[i+1]]*d + g.p[e[i]]*(1.0f-d);
                 
                 for(int k =gFrame;k<6;k++) {
                     
                     //  6.0f 6bit bitmask of surface edges from  each cube
                     if(dot((g.p[e[i+1]] +g.p[e[i]])/2.0f- c -0.5f , dir[k] )<0.0f )  g.dirs=g.dirs | (1<<k); 
                     
                 }
             }
          }
         
        //  5.0f surface vertex, as an average of all crossing edge positions
         g.m/= g.nEdge;
         g.m =_fminf(max(g.m,c),c+1.0f); //must be inside the cube
         //g.m=c+csz/2.0f; //orthogonal connections
    } else if(g.nVertex==8.0f ){
        g.dirs=64;g.m=c+0.5f;
    }
}


__KERNEL__ void TerrainMeshJipiFuse__Buffer_B(float4 O, float2 U, sampler2D iChannel0)
{

    float3 c = texToVoxCoord(_floor(U), offset);        
    GRIDCELL g;     
    getSurface(c,g);  
    O = to_float4_aw(g.m,g.dirs);


  SetFragmentShaderComputedColor(O);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: London' to iChannel1
// Connect Image 'Texture: RGBA Noise Small' to iChannel3
// Connect Image 'Previsualization: Buffer B' to iChannel0


//"terrain mesh  with SN"  by Kastorp
//------------------------------------------------
//messy code adapted from previous shader

//Surface net algorith can be parametrized 
// to obtain smooth mesh , voxels or contour levels
//select style in common tab

const float3 dir[6] =vec3[6]
(
   to_float3(1,0,0), to_float3(0,1,0),to_float3(0,0,1),
   to_float3(-1,0,0), to_float3(0,-1,0),to_float3(0,0,-1)
);


__DEVICE__ float4 VoxelHitPos(float3 pos, float3 ro, float3 rd){
    float3 ri = 1.0f/rd;
  float3 rs = sign(rd);
    float3 mini = (pos-ro + 0.5f - 0.5f*to_float3(rs))*ri;
    float t=  _fmaxf ( mini.x, _fmaxf ( mini.y, mini.z ) );
    return to_float4_aw(t*rd+ro,t);
}

__DEVICE__ float3 rayDirection(float3 cameraDir, float2 uv){
    
    float3 cameraPlaneU = to_float3_aw(normalize(to_float2(cameraDir.y, -cameraDir.x)), 0);
    float3 cameraPlaneV = cross(cameraPlaneU, cameraDir) ;
  return normalize(cameraDir + uv.x * cameraPlaneU + uv.y * cameraPlaneV);

}

RayOut trace(float3 ro,float3 rd)
{
    float TK=iMouse.x>0.? step(iMouse.y/R.y,0.5f)*.1:.1; //edge thickness 
    RayOut ray;
    //RAYTRACING BOUNDING BOX
    span tb= iBox(  ro,  rd, to_float3(N*0.5f+0.00001f,N*0.5f+0.00001f,10.0f) ) ;
    if(tb.f.t==NOHIT) return   InfRay();
    
    
    //VOXEL TRAVERSAL
    float3 rs= sign(rd);
    float3 ri = 1.0f/rd;
  float3 rp=ro +  _fmaxf(tb.n.t,0.0f)*rd;  
    float3 mp=_floor(rp);
    float3 sd = (mp-rp + 0.5f + sign(rd)*0.5f) *ri;
    float3 mask=to_float3_s(0.0f);     
    for (int i = 0; i < 200; i++) {


        if(length(rp-ro)>tb.f.t) break; //outside bounding box
       
       //BEGIN SURFACE NETS
       float4 data = getVoxel( mp,iChannel0);         
       int g_dirs =int(data.w);            
       if(g_dirs>0) {          
           //RAYTRACE VOXEL 
           float3 g_m= swi3(data,x,y,z);       
           ray =  InfRay();
           if(g_dirs<64) {
              if(TK>0.0f) Add( ray, Ray(iSphere(ro- g_m,rd,TK),3.0f));
               vec3[6] g_ng;
               for(int k =_fminf(iFrame,0);k<6;k++) {
                  if((g_dirs & (1<<k))>0) {
                      //get neighbour surface vertex along each direction
                      g_ng[k] = getVoxel( mp-dir[k],iChannel0).xyz; 
                      if(TK>0.0f) Add( ray, Ray(iCylinder(ro,rd,g_m,g_ng[k],TK/2.0f),2.0f));
                      if(TK>0.0f) Add( ray, Ray(iSphere(ro- g_ng[k],rd,TK),3.0f));
                      for(int k2= 0;k2<k;k2++){
                          if(k2!=k-3 && (g_dirs & (1<<k2))>0){
                              //get quad opposite vertex
                              float4 g_f = getVoxel( mp-dir[k]-dir[k2],iChannel0); 
                              if(g_f.a>0.&& g_f.a<64.0f){
                                  if(TK>0.0f) Add( ray, Ray(iSphere(ro- swi3(g_f,x,y,z),rd,TK),3.0f));
                                  if(TK>0.0f) Add( ray, Ray(iCylinder(ro,rd,swi3(g_f,x,y,z),g_ng[k],TK/2.0f),2.0f));
                                  if(TK>0.0f) Add( ray, Ray(iCylinder(ro,rd,swi3(g_f,x,y,z),g_ng[k2],TK/2.0f),2.0f));
                                  //thanks Oneshade!! 
                                  #ifndef TRI
                                      Add( ray, Ray(iBilinearPatch(ro,rd,g_m,g_ng[k], swi3(g_f,x,y,z),g_ng[k2]),1.0f));
                                  #else
                                  //triangle version
                                  if(length(g_m-swi3(g_f,x,y,z))>length(g_ng[k]-g_ng[k2])){                               
                                      Add( ray, Ray(triIntersect(ro,rd,g_m,g_ng[k],g_ng[k2]),1.0f));
                                      Add( ray, Ray(triIntersect(ro,rd,g_ng[k], swi3(g_f,x,y,z),g_ng[k2]),1.0f));
                                     // if(TK>0.0f) Add( ray, Ray(iCylinder(ro,rd,g_ng[k2],g_ng[k],TK/2.0f),2.0f));
                                  }else{
                                      Add( ray, Ray(triIntersect(ro,rd,g_m,g_ng[k],swi3(g_f,x,y,z) ),1.0f));
                                      Add( ray, Ray(triIntersect(ro,rd,g_m,swi3(g_f,x,y,z), g_ng[k2]),1.0f));                                     
                                      //if(TK>0.0f) Add( ray, Ray(iCylinder(ro,rd,swi3(g_f,x,y,z),g_m,TK/2.0f),2.0f));                                  
                                  }
                                  #endif
                              }
                          }
                      }                       
                   } 
               }
           }     
           if (ray.d< NOHIT ){//ray.d+=_fmaxf(tb.n.t,0.0f); 
               return ray;}            
        }
        //END SURFACE NETS
        
        mask = step(swi3(sd,x,y,z), swi3(sd,y,z,x)) * step(swi3(sd,x,y,z), swi3(sd,z,x,y));
    sd += mask *  rs *ri;
        mp += mask *  rs;
        rp = VoxelHitPos(mp,rp,rd).xyz+rd*0.0001f; 
  } 
    return Ray(tb,0.0f);
}
// Tri-Planar blending function. Based on an old Nvidia tutorial.
__DEVICE__ float3 tex3D( sampler2D tex, in float3 p, in float3 n ){  
    n = _fmaxf(_fabs(n), 0.001f);//n = _fmaxf((_fabs(n) - 0.2f)*7.0f, 0.001f); //  etc.
    n /= (n.x + n.y + n.z ); 
  p = (texture(tex, swi2(p,y,z))*n.x + texture(tex, swi2(p,z,x))*n.y + texture(tex, swi2(p,x,y))*n.z).xyz;
    return p*p;
}

__KERNEL__ void TerrainMeshJipiFuse(float4 O, float2 U, float4 iMouse, int iFrame, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel3)
{

    float2 uv = (U - 0.5f * swi2(R,x,y)) / R.y;
    
    float2 M= iMouse.x>0.? iMouse.xy/swi2(R,x,y)*3.5f+.5:to_float2_s(1.0f); 
    float3 ro = 12.0f* to_float3_aw(_cosf(M.x),_sinf(M.x),1.0f)*M.y;
    float3 rd = rayDirection(normalize(to_float3_aw(0)-ro) ,uv);
    float3 ld = normalize(to_float3(-6.0f,-0.2f,-4.0f));
    
    RayOut r= trace(ro,rd);    
    if(r.d<NOHIT && r.id>0.0f){
        float3 p = ro+rd*r.d, 
             n = dot(r.n,rd)>0.?r.n:-r.n;
 
 
       
         if(r.id==1.0f){
             O.xyz= _mix(tex3D(iChannel3, (p)*0.5f, n).grb,to_float3(0.110f,0.204f,0.055f),0.4f); //grass
             O.xyz= texture(iChannel1,p.xy/80.0f,0.0f).rgb;
             //swi3(O,x,y,z) = _mix( swi3(O,x,y,z) ,to_float3(0.188f,0.137f,0.051f),smoothstep(0.7f,0.5f,_fmaxf(dot(n,to_float3(0,0,-1)),0.0f))) ;  //dirt                         
         } else
         O.xyz=(r.id==3.?to_float3(0.8f,0.6f,0.6f):to_float3(0.2f,0.4f,0.4f)) ;

        swi3(O,x,y,z) *= smoothstep(-1.0f,1.0f,dot(n,ld)) *(0.5f+0.5f*smoothstep(p.z,2.0f,3.0f)); 
        
#ifdef SHADOWS         
         RayOut s= trace(p+n*0.01f,ld); 
         //if(s.id>0.0f &&  r.id==1.0f &&s.d<0.1f &&_fabs(dot(ld,r.n))<0.05f  ) O.x*=5.0f;
         if( s.id>0.0f ) O*=0.7f;
#endif         
         
         O = to_float4(_sqrtf(clamp(O, 0.0f, 1.0f)));      
    }    
    else O =to_float4(0.4f,0.4f,0.7f,1.0f);
    


  SetFragmentShaderComputedColor(O);
}