

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//"terrain mesh  with SN"  by Kastorp
//------------------------------------------------
//messy code adapted from previous shader

//Surface net algorith can be parametrized 
// to obtain smooth mesh , voxels or contour levels
//select style in common tab

const vec3 dir[6] =vec3[6]
(
   vec3(1,0,0), vec3(0,1,0),vec3(0,0,1),
   vec3(-1,0,0), vec3(0,-1,0),vec3(0,0,-1)
);


vec4 VoxelHitPos(vec3 pos, vec3 ro, vec3 rd){
    vec3 ri = 1.0/rd;
	vec3 rs = sign(rd);
    vec3 mini = (pos-ro + 0.5 - 0.5*vec3(rs))*ri;
    float t=  max ( mini.x, max ( mini.y, mini.z ) );
    return vec4(t*rd+ro,t);
}

vec3 rayDirection(vec3 cameraDir, vec2 uv){
    
    vec3 cameraPlaneU = vec3(normalize(vec2(cameraDir.y, -cameraDir.x)), 0);
    vec3 cameraPlaneV = cross(cameraPlaneU, cameraDir) ;
	return normalize(cameraDir + uv.x * cameraPlaneU + uv.y * cameraPlaneV);

}

RayOut trace(vec3 ro,vec3 rd)
{
    float TK=iMouse.x>0.? step(iMouse.y/R.y,0.5)*.1:.1; //edge thickness 
    RayOut ray;
    //RAYTRACING BOUNDING BOX
    span tb= iBox(  ro,  rd, vec3(N*.5+.00001,N*.5+.00001,10.) ) ;
    if(tb.f.t==NOHIT) return   InfRay();
    
    
    //VOXEL TRAVERSAL
    vec3 rs= sign(rd);
    vec3 ri = 1./rd;
	vec3 rp=ro +  max(tb.n.t,0.)*rd;  
    vec3 mp=floor(rp);
    vec3 sd = (mp-rp + 0.5 + sign(rd)*0.5) *ri;
    vec3 mask=vec3(0.);     
    for (int i = 0; i < 200; i++) {


        if(length(rp-ro)>tb.f.t) break; //outside bounding box
       
       //BEGIN SURFACE NETS
       vec4 data = getVoxel( mp,iChannel0);         
       int g_dirs =int(data.a);            
       if(g_dirs>0) {          
           //RAYTRACE VOXEL 
           vec3 g_m= data.xyz;       
           ray =  InfRay();
           if(g_dirs<64) {
              if(TK>0.) Add( ray, Ray(iSphere(ro- g_m,rd,TK),3.));
               vec3[6] g_ng;
               for(int k =min(iFrame,0);k<6;k++) {
                  if((g_dirs & (1<<k))>0) {
                      //get neighbour surface vertex along each direction
                      g_ng[k] = getVoxel( mp-dir[k],iChannel0).xyz; 
                      if(TK>0.) Add( ray, Ray(iCylinder(ro,rd,g_m,g_ng[k],TK/2.),2.));
                      if(TK>0.) Add( ray, Ray(iSphere(ro- g_ng[k],rd,TK),3.));
                      for(int k2= 0;k2<k;k2++){
                          if(k2!=k-3 && (g_dirs & (1<<k2))>0){
                              //get quad opposite vertex
                              vec4 g_f = getVoxel( mp-dir[k]-dir[k2],iChannel0); 
                              if(g_f.a>0.&& g_f.a<64.){
                                  if(TK>0.) Add( ray, Ray(iSphere(ro- g_f.xyz,rd,TK),3.));
                                  if(TK>0.) Add( ray, Ray(iCylinder(ro,rd,g_f.xyz,g_ng[k],TK/2.),2.));
                                  if(TK>0.) Add( ray, Ray(iCylinder(ro,rd,g_f.xyz,g_ng[k2],TK/2.),2.));
                                  //thanks Oneshade!! 
                                  #ifndef TRI
                                      Add( ray, Ray(iBilinearPatch(ro,rd,g_m,g_ng[k], g_f.xyz,g_ng[k2]),1.));
                                  #else
                                  //triangle version
                                  if(length(g_m-g_f.xyz)>length(g_ng[k]-g_ng[k2])){                               
                                      Add( ray, Ray(triIntersect(ro,rd,g_m,g_ng[k],g_ng[k2]),1.));
                                      Add( ray, Ray(triIntersect(ro,rd,g_ng[k], g_f.xyz,g_ng[k2]),1.));
                                     // if(TK>0.) Add( ray, Ray(iCylinder(ro,rd,g_ng[k2],g_ng[k],TK/2.),2.));
                                  }else{
                                      Add( ray, Ray(triIntersect(ro,rd,g_m,g_ng[k],g_f.xyz ),1.));
                                      Add( ray, Ray(triIntersect(ro,rd,g_m,g_f.xyz, g_ng[k2]),1.));                                     
                                      //if(TK>0.) Add( ray, Ray(iCylinder(ro,rd,g_f.xyz,g_m,TK/2.),2.));                                  
                                  }
                                  #endif
                              }
                          }
                      }                       
                   } 
               }
           }     
           if (ray.d< NOHIT ){//ray.d+=max(tb.n.t,0.); 
               return ray;}            
        }
        //END SURFACE NETS
        
        mask = step(sd.xyz, sd.yzx) * step(sd.xyz, sd.zxy);
		sd += mask *  rs *ri;
        mp += mask *  rs;
        rp = VoxelHitPos(mp,rp,rd).xyz+rd*.0001; 
	} 
    return Ray(tb,0.);
}
// Tri-Planar blending function. Based on an old Nvidia tutorial.
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){  
    n = max(abs(n), 0.001);//n = max((abs(n) - 0.2)*7., 0.001); //  etc.
    n /= (n.x + n.y + n.z ); 
	p = (texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
    return p*p;
}

void mainImage(out vec4 O, in vec2 U) {
    vec2 uv = (U - 0.5 * R.xy) / R.y;
    
    vec2 M= iMouse.x>0.? iMouse.xy/R.xy*3.5+.5:vec2(1.); 
    vec3 ro = 12.* vec3(cos(M.x),sin(M.x),1.)*M.y;
    vec3 rd = rayDirection(normalize(vec3(0)-ro) ,uv);
    vec3 ld = normalize(vec3(-6.,-.2,-4.));
    
    RayOut r= trace(ro,rd);    
    if(r.d<NOHIT && r.id>0.){
        vec3 p = ro+rd*r.d, 
             n = dot(r.n,rd)>0.?r.n:-r.n;
 
 
       
         if(r.id==1.){
             O.rgb= mix(tex3D(iChannel3, (p)*.5, n).grb,vec3(0.110,0.204,0.055),.4); //grass
             O.rgb= texture(iChannel1,p.xy/80.,0.).rgb;
             //O.rgb = mix( O.rgb ,vec3(0.188,0.137,0.051),smoothstep(.7,.5,max(dot(n,vec3(0,0,-1)),0.))) ;  //dirt                         
         } else
         O.rgb=(r.id==3.?vec3(.8,.6,.6):vec3(.2,.4,.4)) ;

        O.rgb *= smoothstep(-1.,1.,dot(n,ld)) *(.5+.5*smoothstep(p.z,2.,3.)); 
        
#ifdef SHADOWS         
         RayOut s= trace(p+n*.01,ld); 
         //if(s.id>.0 &&  r.id==1. &&s.d<.1 &&abs(dot(ld,r.n))<.05  ) O.x*=5.;
         if( s.id>.0 ) O*=.7;
#endif         
         
         O = vec4(sqrt(clamp(O, 0., 1.)));      
    }    
    else O =vec4(0.4,0.4,0.7,1.0);
    
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//ISOSURFACE FUNCTION  (USE YOU OWN HERE)

float map(vec3 p)
{   

   float d= p.z/2.-   length(texture(iChannel1,p.xy/80.,0.).xyz)*2.5-.5;  
   d=step(0.,d)*2.-1.;  //works also with 1 bit per voxel     
    return d;
}


void mainImage( out vec4 O, in vec2 U )
{   
    vec3 c = texToVoxCoord(floor(U), offset); 
    float data =map(c); //not stateful here, otherwise use getVoxel( c ,iChannel0);
    O = vec4(data);
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<

//select style here:
#define cube vec3(0.,0.,.99)  // contour
//#define cube vec3(0.)  // smooth
//#define cube vec3(0.99)  // voxel

const float N= 80.; //number of voxel x side (TOT=N^3)
#define TRI  


//-------------------------------
vec3 offset = floor(vec3(0.,0.,0.));
const vec2 packedChunkSize= vec2(5,3);// ceil(sqrt(N) * vec2(1.25 ,0.8)); // iResolution.xy/iResolution.yx 
const float heightLimit = packedChunkSize.x * packedChunkSize.y; //must be > N
#define R iResolution.xy
//-----------------------------------------
// VOXEL CACHE FUNCTIONS from fb39ca4

vec2 unswizzleChunkCoord(vec2 storageCoord) {
 	vec2 s = floor(storageCoord);
    float dist = max(s.x, s.y);
    float o = floor(dist / 2.);
    float neg = step(0.5, mod(dist, 2.)) * 2. - 1.;
    return neg * (s - o);
}

vec2 swizzleChunkCoord(vec2 chunkCoord) {
    vec2 c = chunkCoord;
    float dist = max(abs(c.x), abs(c.y));
    vec2 c2 = floor(abs(c - 0.5));
    float o = max(c2.x, c2.y);
    float neg = step(c.x + c.y, 0.) * -2. + 1.;
    return (neg * c) + o;
}

float calcLoadDist(vec2 iResolutionxy) {
    vec2  chunks = floor(iResolutionxy / packedChunkSize); 
    float gridSize = min(chunks.x, chunks.y);    
    return floor((gridSize - 1.) / 2.);
}

vec3 texToVoxCoord(vec2 textelCoord, vec3 offset) {

    vec2 packedChunkSize= packedChunkSize;
	vec3 voxelCoord = offset;
    voxelCoord.xy += unswizzleChunkCoord(textelCoord / packedChunkSize);
    voxelCoord.z += mod(textelCoord.x, packedChunkSize.x) + packedChunkSize.x * mod(textelCoord.y, packedChunkSize.y);
    return voxelCoord;
}

vec2 voxToTexCoord(vec3 voxCoord) {
    vec2 packedChunkSize= packedChunkSize;
    vec3 p = floor(voxCoord);
    return swizzleChunkCoord(p.xy) * packedChunkSize + vec2(mod(p.z, packedChunkSize.x), floor(p.z / packedChunkSize.x));
}

vec4  getVoxel(vec3 p,sampler2D iChannel) {
    p.z-= offset.z;
    if(p.z>heightLimit || p.z<0.){return vec4(0.);}  
    return texelFetch(iChannel, ivec2(voxToTexCoord(p))  , 0); 

}



//-----------Intersection functions--------------------
#define NOHIT 1e5
struct its
{
	float t;
	vec3 n;    //normal 
	vec3 fuv;  //face & uv 
};
const its  NO_its=its(NOHIT,vec3(0.),vec3(0.));
struct span
{
	its n;
	its f;
};

struct RayOut{   
    float d;
    vec3 n;
    vec3 fuv; //unused here
    float id;
    float d2;
};

RayOut FastUnion(  RayOut a, RayOut b)
{
   if(a.d<b.d) return a;
   else return b;
}
#define Add(_ro,_func) _ro = FastUnion(_ro,_func);

RayOut InfRay(){return RayOut(NOHIT,vec3(0),vec3(0),0.,0.);}
RayOut Ray(span s,float mat){    
    if(s.f.t<0. ||(s.f.t==NOHIT && s.n.t<0. )) return  InfRay();
    if(s.n.t<0. && s.f.t>0.  ) return RayOut(0.001,vec3(0),vec3(0),mat,s.f.t);
    return RayOut( s.n.t,s.n.n,s.n.fuv,mat,s.f.t-s.n.t);
}

span iSphere( in vec3 ro, in vec3 rd, float ra )
{

    vec3 oc = ro ;
    float b = dot( oc, rd );
    float c = dot( oc, oc ) - ra*ra;
    float h = b*b - c;
    if( h<0. ) return span(NO_its,NO_its); // no intersection
    h = sqrt( h );
    vec3 oNor =normalize(ro-(b+h)*rd); 
    vec3 oFuv=vec3(0.,atan(oNor.y,length(oNor.xz)),atan(oNor.z,oNor.x))*ra*1.5708  ;
    vec3 fNor= normalize(ro-(b-h)*rd); 
    vec3 fFuv=vec3(0.,atan(fNor.y,length(fNor.xz)),atan(fNor.z,fNor.x))*ra*1.5708  ;
    if( h-b < 0. ) return  span(NO_its,NO_its);
    return span(its(-b-h,oNor,oFuv) , its(-b+h,-fNor,fFuv));
}

span iBox( in vec3 ro, in vec3 rd, vec3 boxSize) 
{
    vec3 m = 1./rd; 
    vec3 n = m*ro;   
    vec3 k = abs(m)*boxSize;

    vec3 t1 = -n - k;
    vec3 t2 = -n + k;
    float tN = max( max( t1.x, t1.y ), t1.z );
    float tF = min( min( t2.x, t2.y ), t2.z );
    if( tN>tF || tF<0.) return span(NO_its,NO_its); // no intersection
    vec3 oNor = -sign(rd)*step(t1.yzx,t1.xyz)*step(t1.zxy,t1.xyz); 
    vec3 oFuv=vec3( dot(abs(oNor),vec3(1,5,9)+ oNor)/2.,dot(ro+rd*tN,oNor.zxy),dot(ro+rd*tN,oNor.yzx));      
    vec3  fNor=- sign(rd)*step(t2.xyz,t2.yzx)*step(t2.xyz,t2.zxy); 
    vec3 fFuv=vec3( dot(abs(fNor),vec3(1,5,9)+ fNor)/2.,dot(ro+rd*tF,fNor.zxy),dot(ro+rd*tF,fNor.yzx));
    return  span(its(tN,oNor,oFuv) , its(tF,fNor,fFuv));
}

span iPlane( in vec3 ro, in vec3 rd, in vec3 n ,float h)
{
    float d= -(dot(ro,n)+h)/dot(rd,n);
    vec3  u = normalize(cross(n,vec3(0,0,1))), v = normalize(cross(u,n) );
    vec3 oFuv=vec3(1.,dot(ro+d*rd,u), dot(ro+d*rd,v));
    vec3 oNor=n;
    if(d<0.)  return span(NO_its,NO_its);
    return span(its(d,oNor,oFuv),NO_its);
}

span iCylinder( in vec3 ro, in vec3 rd,  in vec3 pa, in vec3 pb, float ra ) // extreme a, extreme b, 
{
    span no_hit =  span(NO_its,NO_its);;

    vec3 ba = pb-pa;

    vec3  oc = ro - pa;

    float baba = dot(ba,ba);
    float bard = dot(ba,rd);
    float baoc = dot(ba,oc);
    
    float k2 = baba            - bard*bard;
    float k1 = baba*dot(oc,rd) - baoc*bard;
    float k0 = baba*dot(oc,oc) - baoc*baoc - ra*ra*baba;
    
    float h = k1*k1 - k2*k0;
    if( h<0.0 )  return no_hit;
    h = sqrt(h);
    
    float t = (-k1-h)/k2;
    float y = baoc + t*bard; 
    float t2 = ( ((y<0.0) ? 0.0 : baba) - baoc)/bard;   
    
    if( (y<0.0 || y>baba )  && abs(k1+k2*t2)>h)  return span(NO_its,NO_its);
    its iN,iF;
    vec3 w = normalize(ba), u = normalize(cross(ba,vec3(0,0,1))), v = normalize(cross(u,w) );
    
    //entry point
    vec3 q = (oc+t*rd-ba)*mat3(u,v,w);   
    if( y>0.0 && y<baba ) iN= its( t, (oc+t*rd - ba*y/baba)/ra,vec3(0.,atan(q.y,q.x)*ra,q.z) ); 
    else iN= its(t2, ba*sign(y)/baba ,vec3(1.,dot(oc+t2*rd-ba,u) ,dot(oc+t2*rd-ba,v) )); 
        
  
    //exit point
    t = (-k1+ h)/k2;
    y = baoc + t*bard; 
    t2 = ( ((y<0.0) ? 0.0 : baba) - baoc)/bard;
    q = (oc+t*rd-ba)*mat3(u,v,w); 
    if( y>0.0 && y<baba ) iF= its( t, (oc+t*rd - ba*y/baba)/ra,vec3(0.,atan(q.y,q.x)*ra,q.z) ); 
    else iF= its(t2, ba*sign(y)/baba ,vec3(1.,dot(oc+t2*rd-ba,u) ,dot(oc+t2*rd-ba,v) )); 
    
    return span(iN , iF);
  
}
float cross2D(in vec2 a, in vec2 b) { return a.y * b.x - a.x * b.y; }

//from https://www.shadertoy.com/view/Ns2SzG by Oneshade
span iBilinearPatch(in vec3 ro, in vec3 rd,in vec3 a, in vec3 b, in vec3 c, in vec3 d) {
    //bool tr=false;
    if(abs(rd.x)<.0001){  //not working if rd.x==0.
         return span(its(.01,vec3(0),vec3(0)),NO_its);
        //tr=true; ro=ro.yzx;rd=rd.yzx;a=a.yzx;b=b.yzx;c=c.yzx;d=d.yzx;
    }
    vec2 m = rd.yz / rd.x;
    if(length(a - b + c - d)==0.)  a+=vec3(.001); //fix coplanar issue
    vec3 p = a - b + c - d, q = d - a, r = b - a;
    vec2 c1 = p.yz - p.x * m, c2 = q.yz - q.x * m, c3 = r.yz - r.x * m;
    vec2 c4 = (ro.x - a.x) * m + a.yz - ro.yz;

    // Quadratic coefficients (reversed as a trick to deal with edge cases)
    float qa = cross2D(c4, c2);
    float qb = cross2D(c4, c1) + cross2D(c3, c2);
    float qc = cross2D(c3, c1);
   
    float discr = (qb * qb - 4.0 * qa * qc);
    span s=span(NO_its,NO_its);
    if (discr > 0.0) {
        vec2 v = 2.0 * qa / (vec2(-1.0, 1.0) * sqrt(discr) - qb); // Solve quadratic in v
        vec2 u = -(c4.x + c3.x * v) / (c1.x * v + c2.x);
        vec2 t = (p.x * u * v + q.x * u + r.x * v + a.x - ro.x) / rd.x;

        // Get closest intersection in view and add it to the scene        
        if (abs(u[0] - 0.5) < 0.5 && abs(v[0] - 0.5) < 0.5) 
             s.n= its(t[0],normalize(cross(p * v[0] + q, p * u[0] + r)),vec3(0.,u[0], v[0]));
        if ( abs(u[1] - 0.5) < 0.5 && abs(v[1] - 0.5) < 0.5) 
             s.f= its(t[1],normalize(cross(p * v[1] + q, p * u[1] + r)),vec3(0.,u[1], v[1])); 
        if (s.f.t<s.n.t){ its tp=s.n;s.n=s.f;s.f=tp;}     
        s.n.n*=-sign(dot(s.n.n,rd));
        s.f.n*=-sign(dot(s.f.n,rd));
    }
    //if(tr) {s.n.n=s.n.n.zxy;s.f.n=s.f.n.zxy;}
    return s;
}

//based on Iq's
span triIntersect( in vec3 ro, in vec3 rd, in vec3 v0, in vec3 v1, in vec3 v2 )
{
    vec3 v1v0 = v1 - v0;
    vec3 v2v0 = v2 - v0;
    vec3 rov0 = ro - v0;

    vec3  n = cross( v1v0, v2v0 );
    vec3  q = cross( rov0, rd );
    float d = 1.0/dot( rd, n );
    float u = d*dot( -q, v2v0 );
    float v = d*dot(  q, v1v0 );
    float t = d*dot( -n, rov0 );   

    if( u<0.0 || v<0.0 || (u+v)>1.0 ) t = -1.0;
    
//    return vec3( t, u, v );

     return  span(its(t,-n,vec3(0)),NO_its);
}



// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// SURFACE NET BUFFER




 struct GRIDCELL{     //calculated for each cube in the following sequence:
   vec3 p[8];         //  1. cube vertex positions
   float val[8];      //  2. isosurface value at each cube vertex
   float nVertex;     //  3. number of vertex where val[]>0; if 0<nVertex<8  => surface cube
   float nEdge;       //  4. number of cube edges with crossing 
   vec3 m;            //  5. surface vertex, as an average of all crossing edge positions
   int dirs;          //  6. 6bit bitmask of surface edges from each cube;                        
} ;


const vec3 v[8] =vec3[8]
(
   vec3(0,0,0), vec3(1,0,0),vec3(1,1,0),vec3(0,1,0),
   vec3(0,0,1), vec3(1,0,1),vec3(1,1,1),vec3(0,1,1)
);

const int  e[24] =int[24](
   0,1,   1,2,  2,3,   3,0, 
   4,5,   5,6,  6,7,   7,4,   
   0,4,   1,5,  2,6,   3,7);

const vec3 dir[6] =vec3[6]
(
   vec3(1,0,0), vec3(0,1,0),vec3(0,0,1),
   vec3(-1,0,0), vec3(0,-1,0),vec3(0,0,-1)
);


int gFrame=0; 
void  getSurface(vec3 c, inout GRIDCELL g)
{
    
    g.m=vec3(0.);
    g.nVertex=0.;
    g.nEdge=0.;
    g.dirs=0;

    //gFrame unrolling fails here...
    for(int i=0;i<8;i++)
    {

        //1. cube vertex positions
       
        vec3 vp=c+ cube/2.+ v[i]*(1.-cube);
        g.p[i]=vp;

        //  2. isosurface value at each cube vertex
        float val = //map(vp);
                    getVoxel( c+  v[i] ,iChannel0).x;
         g.val[i]=val;
        
        //3. number of vertex where val[]>0;
        g.nVertex+= (val<=0.?1.:0.);

    }
 
     if(g.nVertex>0. && g.nVertex<8.)
     {

          for(int i=gFrame;i<24;i+=2)
          {
              //  isosurface weights at each cube edge vertexes
              float d1 = g.val[e[i]],
                  d2 = g.val[e[i+1]],
                  d= d1/(d1-d2);

            //  4. number of cube edges with crossing 
             if(d1*d2<0.){
                 g.nEdge++;
                 g.m+= g.p[e[i+1]]*d + g.p[e[i]]*(1.-d);
                 
                 for(int k =gFrame;k<6;k++) {
                     
                     //  6. 6bit bitmask of surface edges from  each cube
                     if(dot((g.p[e[i+1]] +g.p[e[i]])/2.- c -.5 , dir[k] )<0. )  g.dirs=g.dirs | (1<<k); 
                     
                 }
             }
          }
         
        //  5. surface vertex, as an average of all crossing edge positions
         g.m/= g.nEdge;
         g.m =min(max(g.m,c),c+1.); //must be inside the cube
         //g.m=c+csz/2.; //orthogonal connections
    } else if(g.nVertex==8. ){
        g.dirs=64;g.m=c+.5;
    }
}


void mainImage( out vec4 O, in vec2 U )
{
    vec3 c = texToVoxCoord(floor(U), offset);        
    GRIDCELL g;     
    getSurface(c,g);  
    O = vec4(g.m,g.dirs);
}