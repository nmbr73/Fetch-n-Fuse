

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//3D Brownian Tree (sorta because there is a bias towards the center)
//Drag with the mouse to look-around

const float TAU=2.*acos(-1.);

//Distance between sphere and ray if they intersect, -1 otherwise
float sphereRayDist(vec3 o, vec3 r, vec4 s){
    vec3 oc = o-s.xyz;
    float b = dot(oc,r);
    float c = dot(oc,oc)-s.w*s.w;
    float t = b*b-c;
    if( t > 0.0) 
        t = -b - sqrt(t);
    return t;
}

//Thanks to iq for this helpful shadow function
float sphereRayShadow(vec3 o,vec3 r,vec4 s,float k){
    vec3 oc = o - s.xyz;
    float b = dot( oc, r );
    float c = dot( oc, oc ) - s.w*s.w;
    float h = b*b - c;
    return (b>0.0) ? step(-0.0001,c) : smoothstep( 0.0, 1.0, h*k/b );
}

//color palette code from IQ https://www.shadertoy.com/view/ll2GD3
vec3 pal(float t,vec3 a,vec3 b,vec3 c,vec3 d){
    return a+b*cos(TAU*(c*t+d));
}

//render scene with balls
vec3 trace(vec3 o,vec3 r){
    int irx=int(iResolution.x);
    bool hit=false;
    float md=2e20;
    vec4 msp=vec4(-1.);
    for(int i=1;i<=NUM_BALLS;i++){
        vec4 sphere=SPHERE_RANGE*texelFetch(iChannel0,ivec2(i%irx,i/irx),0);
        float d=sphereRayDist(o,r,sphere);
        if(d<md&&d>=0.){
        	msp=sphere;
            md=d;
            hit=true;
        }
    }
    
    if(hit){
        vec3 no=o+r*md;
        vec3 c=pal(msp.w,vec3(0.5,0.5,0.5),vec3(0.5,0.5,0.5),vec3(1.0,1.0,1.0),vec3(0.0,0.33,0.67) );
        vec3 lp=vec3(.1,.1,0);
        float l;
        l=dot(normalize(r-lp),normalize(msp.xyz-(o+r*md)));
        l+=pow(l,32.);
        r=-normalize(r-lp);
        no+=r*.001;
        float shd=1.;
        for(int i=1;i<=NUM_BALLS;i++){
        	vec4 sphere=SPHERE_RANGE*texelFetch(iChannel0,ivec2(i%irx,i/irx),0);
            shd*=sphereRayShadow(no,r,sphere,40.);
        }
        return c*l*shd;
    }else{
        //super lazy stars in the background
        return vec3(hash13(round(r*100.))>.99);
    }
}

//Apply Quaternion Rotation to vec3
vec3 rot(vec4 q,vec3 r) {return 2.*cross(q.xyz,q.w*r+cross(q.xyz,r));}

void mainImage(out vec4 fragColor,in vec2 fragCoord ){
    vec2 uv=(2.*fragCoord-iResolution.xy)/iResolution.y;
    vec3 o = vec3(0.,0.,-1.2*SPHERE_RANGE);
    vec3 r = normalize(vec3(uv,1.));
    vec4 t=texelFetch(iChannel1,ivec2(0,0),0);
    r+=rot(t,r);
    o+=rot(t,o);
    fragColor=vec4(trace(o,normalize(r)),1.);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//Ball Motion

//nudge balls around until they are touching the center structure
//when they touch the center structure they become part of it
//ball size is based on distance to center
//balls have a bias with their random motion to move towards the center
void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    int md=int(fragCoord.x+floor(fragCoord.y)*iResolution.x);
    int irx=int(iResolution.x);
    float rC=length(texelFetch(iChannel0,ivec2(0),0).xy-iResolution.xy);
    if(iFrame==0||rC>0.){
        fragColor=vec4((.5*hash13(vec3(fragCoord,int(iDate.w)))+.5)*normalize(hash33(vec3(fragCoord,int(iDate.w)))-.5),-.02);
        if(md==1){fragColor=vec4(0.,0.,0.,MAX_SIZE);}
        if(md==0){fragColor=vec4(iResolution.xy,0.,0.);}
    }else if(md<=NUM_BALLS&&md>0){
        float d=2e20;
        vec4 p=texelFetch(iChannel0,ivec2(fragCoord),0).xyzw;
        if(p.w>0.){
        	fragColor=p;return;
        }
        p.xyz+=p.w*(hash33(vec3(fragCoord,iFrame+int(iDate.w)))-.5);
        //contain within range
        if(length(p.xyz)>1.){p.xyz=normalize(p.xyz);}
        p.xyz*=.999;
        p.w=-MAX_SIZE/(((MAX_SIZE/MIN_SIZE)-1.)*pow(length(p.xyz),2.)+1.);
        //ball tests
        for(int i=1;i<=NUM_BALLS;i++){
            vec4 s=texelFetch(iChannel0,ivec2(i%irx,i/irx),0);
            //if sign is negative don't care about collision
            //negative sign means the ball is not added to the structure yet
            //also handles self check with the 0
            d=min(d*sign(s.w),length(p.xyz-s.xyz)-abs(s.w))*sign(s.w);
        }
        
        //did it collide?
        if(d<abs(p.w)){
            fragColor=vec4(p.xyz,abs(p.w));
        }else{
            fragColor=p;
        }
    }else if(md==0){
        fragColor=vec4(iResolution.xy,0.,0.);
    }else{
    	fragColor=vec4(-1.);
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
const int NUM_BALLS=1000;//change this to whatever your computer can handle
const float SPHERE_RANGE=5.;

const float MIN_SIZE=.01;
const float MAX_SIZE=.1;

//Hashes from David Hoskins at https://www.shadertoy.com/view/4djSRW
float hash13(vec3 p3){
	p3  = fract(p3 * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

vec3 hash33(vec3 p3){
	p3 = fract(p3 * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yxz+33.33);
    return fract((p3.xxy + p3.yxx)*p3.zyx);

}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//Camera Rotation, Click And Drag With Velocity

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
    if(iFrame==0){
        fragColor=vec4(0.,0.,0.,1.);
        //initialize velocity
        if(fragCoord.x>1.&&fragCoord.x<3.&&fragCoord.y<1.){
        	fragColor=vec4(.08,.02,0.,0.);
        }
    }else{
        if(fragCoord.x<1.&&fragCoord.y<1.){
            vec4 p=texelFetch(iChannel0,ivec2(0),0);
            vec4 v=texelFetch(iChannel0,ivec2(1,0),0)/3.;
            vec2 rx = vec2( sin(v.x/2.),cos(v.x/2.));
   			vec2 ry = vec2(-sin(v.y/2.),cos(v.y/2.));
            //Quaternion multiplication simplification for basis elements
            vec4 d=vec4(-rx.y*ry.y,rx.x*ry.x,-rx.x*ry.y,rx.y*ry.x);
            //Full Quaternion multiplication
            fragColor = normalize(vec4(d.x*p.x-d.y*p.y-d.z*p.z-d.w*p.w,
                                       d.x*p.y+d.y*p.x+d.z*p.w-d.w*p.z,
                                       d.x*p.z-d.y*p.w+d.z*p.x+d.w*p.y,
                                       d.x*p.w+d.y*p.z-d.z*p.y+d.w*p.x));
        }else if(fragCoord.x<2.&&fragCoord.y<1.){
            vec4 v=texelFetch(iChannel0,ivec2(1,0),0);
            vec4 m=texelFetch(iChannel0,ivec2(2,0),0);
            v=.98*v;
            if(m.z>.5){
            	v.xy+=(iMouse.xy-m.xy)/iResolution.xy;
            }
            fragColor=v;
        }else if(fragCoord.x<3.&&fragCoord.y<1.){
            fragColor = iMouse;
        }else{
            fragColor = vec4(0.);
        }
    }
}