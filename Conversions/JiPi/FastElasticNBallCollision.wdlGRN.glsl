

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//main rendering

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor=texture(iChannel0,fragCoord/iResolution.xy);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//controls moving the particles
vec4 getParticle(vec2 p)
{
    float R=ceil(ballRadius);
    vec4 np=vec4(-1.);
    float s=.5;
    for(float i=-R;i<=R;i++){
    	for(float j=-R;j<=R;j++){
            vec4 prt = texture(iChannel0,(p+vec2(j,i))/iResolution.xy);
            if(prt!=vec4(-1.)){
                if( abs(prt.x+prt.z+j)<=s && abs(prt.y+prt.w+i)<=s ){
                    np=vec4(prt.x+prt.z+j,prt.y+prt.w+i,prt.z,prt.w);
            	}
            }
            
    	}
    }
    return np;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    if(map(fragCoord)<=0.){
        fragColor = vec4(-1.);
    }else if(iFrame<=5){
        float A=10.*ceil(ballRadius);
        if(int(fragCoord.x/A)!=int((1.+fragCoord.x)/A)&&
           int(fragCoord.y/A)!=int((1.+fragCoord.y)/A)){
            fragColor = 1.*vec4(0.,0.,2.*(hash22(fragCoord.xy+vec2(iDate.w))-.5));
        }else{
            fragColor = vec4(-1.);
        }
    }else if(int(fragCoord.x)==int(iResolution.x/2.)&&
             int(fragCoord.y)==int(iResolution.y/2.)&&
             texelFetch(iChannel1, ivec2(32,1),0 ).x==1.){
        fragColor = 1.*vec4(0.,0.,2.*(hash22(fragCoord.xy+vec2(iDate.w))-.5));
    }else{
        vec4 p=getParticle(fragCoord);
        if(p.x!=-1.){
            //handles ball with too much speed (rarely happens)
            if(length(p.zw)>ceil(ballRadius)){
				p.zw*=ceil(ballRadius)/length(p.zw);
            }
            //friction goes here
            //p.zw*=.99;
            fragColor = p;
        }else{
            fragColor = vec4(-1.);
        }
    }
}
// >>> ___ GLSL:[Buf B] ____________________________________________________________________ <<<
//controls particle collisions
//there is no restitution in this sim
//sometimes clumps form as a result with multiple balls colliding at the same time

vec4 getNewVelocity(vec4 mp, vec2 p)
{
    float R=ceil(ballRadius*2.);
    vec2 nv=vec2(0.);
    float collisionCount=0.;
    vec4 nMP=mp;
    //check collisions with other balls
    for(float i=-R;i<=R;i++){
    	for(float j=-R;j<=R;j++){
            if(i!=0.&&j!=0.){
                vec4 prt = texture(iChannel0,(p+vec2(j,i))/iResolution.xy);
            	if(prt.x!=-1.){
                    vec2 collision=mp.xy-(prt.xy+vec2(j,i));
                    //vec2 collision1=mp.xy+mp.zw-(prt.xy+vec2(j,i)-prt.zw);
                    float d=length(collision);
                    float r=ballRadius*2.;
                	if(d<=r){
                        collision = collision/d;
                        float a = dot(mp.zw,collision);
                        float b = dot(prt.zw,collision);
                        
                        //possible to add a fake restitution by applying collisions based on overlap
                        /*nv+=(mp.zw+((b-a)*collision))*((r+1.)-d);
                        collisionCount+=((r+1.)-d);*/
                        
                        //basic 2D elastic collision equation
                        nv+=(mp.zw+((b-a)*collision));
                        collisionCount+=1.;
                        
                        //here's basic 1D math which creates a lot of clumps
                        //Might be useful if that's what you want
                        /*nv+=prt.zw;
                        collisionCount+=1.;*/   
            		}
            	}
            }     
    	}
    }
    //check collisions with non-kinematic obstacles
    float d=map(p+mp.xy);
    if(d<=ballRadius){
        vec2 normal=mapNormal(p+mp.xy);
        nv+=reflect(mp.zw,normal);
        collisionCount+=1.;
    }
    //apply all collisions
    if(collisionCount>0.){
    	nMP.zw=nv/collisionCount;
    }
    return nMP;
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    if(iFrame<=5){
        fragColor=texture(iChannel0,uv);
    }else{
		vec4 mp=texture(iChannel0,uv);
        if(mp.x!=-1.){
            fragColor = getNewVelocity(mp,fragCoord);
        }else{
            fragColor = vec4(-1.);
        }
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
#define HASHSCALE3 vec3(.1031, .1030, .0973)

float ballRadius=2.*sqrt(2.);//min of sqrt(2.), max is how big your computer can handle

//utility functions

//can be used for solid, non-kinematic obstacles
//unless you use a higher than 1. bouse value then the balls get stuck
float map(vec2 p){
    //float d=length(p-vec2(100.,50.))-30.;
    /*d=min(d,p.x-5.);
    d=min(d,165.-p.x);
    d=min(d,p.y-5.);
    d=min(d,90.-p.y);*/
	//return d;
    return 100.;
}
vec2 mapNormal(vec2 p)
{
	vec2 eps = vec2(0.0, 0.001);
    vec2 normal = normalize(vec2(
        map(p + eps.yx) - map(p - eps.yx),
        map(p + eps.xy) - map(p - eps.xy)));
    return normal;
}
//from https://www.shadertoy.com/view/4djSRW
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * HASHSCALE3);
    p3 += dot(p3, p3.yzx+19.19);
    return fract((p3.xx+p3.yz)*p3.zy);

}
// >>> ___ GLSL:[Buf C] ____________________________________________________________________ <<<
//main rendering with trails for balls

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 p = fragCoord;
    float d=100.;
    float md=2.;
    if(max(fract(fragCoord.x/10.),fract(fragCoord.y/10.))-.9>0.){
    	md/=1.5;
    }
    if(length(fragCoord-iMouse.xy)<200.&&iMouse.z > 0.){
    	p = iMouse.xy+(fragCoord-iMouse.xy)/3.;
        d=abs(length(fragCoord-iMouse.xy)-200.)-3.;
        md=1.5;
        if(max(fract(fragCoord.x/30.),fract(fragCoord.y/30.))-.9>0.){
            md/=1.5;
        }
    }
    float R=ceil(ballRadius);
    for(float i=-R;i<=R;i++){
        for(float j=-R;j<=R;j++){
            vec4 prt = texture(iChannel0,(floor(p)+vec2(j,i))/iResolution.xy);
            if(prt!=vec4(-1.)){
                d=min(d,length(p-(floor(p)+vec2(j,i)+prt.xy))-ballRadius);
            }

        }
    }
    d=min(d,map(p));
    float trail=.9;
    float nc=trail*texture(iChannel1,fragCoord/iResolution.xy).x+(1.-trail)*step(-d,0.)/md;
    fragColor=vec4(min(nc,step(-d,0.)/md));
}