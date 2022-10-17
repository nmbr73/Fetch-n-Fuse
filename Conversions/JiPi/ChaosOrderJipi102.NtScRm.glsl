

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// chaos & order by kastorp
//---------------------------
void mainImage( out vec4 O, in vec2 U )
{
    Init
    vec2  p = (U/R.y -R/R.y*.5),mp = (iMouse.xy/R.y -R/R.y*.5),
        c = (p*R.y/R.xy +.5)*float(M) ;    
    int j = int(c.x) +int(c.y)*M, //current region id
        jj= int(step(.5,fract(c.x))+2.*step(.5,fract(c.y)));
    
    O=vec4(0,0,0,10);
    float q;
    for(int i=0,k=i;k<N && i<500;i++){ // scan points in current region (max 500)
        
        
        //int sk=int(texelFetch(iChannel1,ivec2(k%NX,k/NX +j*NY),0)[jj]);
        int sk=int(texture(iChannel1,(vec2(ivec2(k%NX,k/NX +j*NY))+0.5)/R)[jj]);
        if(sk==0) { //the point is in
            //vec4 c=texelFetch(iChannel0,ivec2(k%NX,k/NX ),0);
            vec4 c=texture(iChannel0,(vec2(ivec2(k%NX,k/NX ))+0.5)/R);
            float r= TL*2./(2.+c.w);
            float d =length(p-c.xy), 
                //sh= (1.5- length(p-c.xy-vec2(0,r*.5))/r)*1.5 -.5;sh=.5+sh*sh;
                sh= (1.+dot((p-c.xy) +vec2(0,d+r),vec2(0,d+r))/d/r)*.15; 
            if(c.w<O.w && d<r) O=vec4((0.5 + 0.5*cos(c.z*6.+vec3(0,2,4)))*sh,c.w);
            
            k+=1; //next point
        }
        else k+=int(sk); //skip till first point in region
        q=float(i-N/H)/30.;
    }
    
   
   //O = texture(iChannel1,U/R);
   
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//random points 
//// Hash without Sine (c)2014 David Hoskins
vec3 hash32(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy+p3.yzz)*p3.zyx);
}


void mainImage( out vec4 O, in vec2 U )
{
    Init
    if(int(U.y)>(NY*M*M) || int(U.x)>NX) return ;
    int i= (int(U.y)%NY)*NX + int(U.x); 
        
    vec3 p = mix(vec3(i/(L*L),(i%(L*L))/L,i%L)/float(L)-.5+1./float(L) , (hash32(U)-.5),.5+.5*cos(iTime/2.));    
  
   float a = iTime/4.0 + iMouse.x/iResolution.x*3.;
   
   //a = iMouse.x/iResolution.x*3.0;
   
   p.xy*=mat2(cos(a),sin(a),-sin(a),cos(a));
   p.xz*=mat2(cos(a),sin(a),-sin(a),cos(a));
   O.xy=p.xy *1.4 / (2.+p.z);
   O.z=float(i)/float(N);
   O.w= p.z;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//#define R iResolution.xy

#define N 1000   //number of points
#define H 200  //number of scanned points in buffer B
#define TL 0.015 //sphere size 

#define Init vec2 R =iResolution.xy;\
    int NX =int(iResolution.x);\
    int NY= (N/NX+1);\
    int M= min(10,int(sqrt(iResolution.y/float(NY))));\
    int L= int(pow(float(N),1./3.));


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//points in each region
void mainImage( out vec4 O, in vec2 U )
{
    Init
    O= vec4(H);
    if(int(U.y)>(NY*M*M) || int(U.x)>NX) return ;
       
    int j=int(U.y)/NY, //current region
        i= (int(U.y)%NY)*NX + int(U.x); //current point

    bvec4 f=bvec4(false);
    for( int k = i; k<min(N,i+H);k++){ //iterate on max H points in region
        
        
        //vec4 p= texelFetch(iChannel0,ivec2(k%NX,k/NX),0); //point position
        vec4 p= texture(iChannel0,(vec2(ivec2(k%NX,k/NX))+0.5)/R); //point position
        float r= TL*2./(2.+p.w);
        float sz=1./float(M);
        vec2 s =(vec2((j%M),j/M)+.25);
        vec2 s1=s+.25; s1=s1*sz-.5 ; s1*=R.xy/R.y ; //region center
        if(max(abs(p.x-s1.x)-.5*sz*R.x/R.y,abs(p.y-s1.y)-.5*sz)>r) continue; //outside region
          
        for(int jj=0;jj<4;jj++){
          if(f[jj])continue; //already found next point for subregion
               
          vec2 ss=s +vec2((jj==1|| jj==3) ?.5:0.,(jj==2|| jj==3) ?.5:0.); ss=ss*sz-.5 ; ss*=R.xy/R.y ; //subregion center
      
            if(max(abs(p.x-ss.x)-.25*sz*R.x/R.y,abs(p.y-ss.y)-.25*sz)<r)
            {
                //this point is inside subergion
                O[jj]= float(k-i); 
                f[jj]=true;
            }
       } 
       if(all(f)) return; 
    }
    
       
    
}