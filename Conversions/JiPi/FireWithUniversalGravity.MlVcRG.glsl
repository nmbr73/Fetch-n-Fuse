

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define rMode 0
//#define rMode 1
//#define rMode 2
//#define rMode 3
//#define rMode 4

#define shadow false
//#define shadow true

//IMPORTANT. IF IT DOESNT WORK THEN GO TO BUFFER A AND SET THE BUFFER TO CLAMP AND THEN REPEAT
float numPart(vec2 p)
{
    float t=0.;
    float t1=0.;
    for(float i=-R2;i<=R2;i++){
    	for(float j=-R2;j<=R2;j++){
            float w=pow(3.0,-length(vec2(i,j)/R2));
            if(length(vec2(i,j))>R2){
            	continue;
            }
            if(texture(iChannel0,p+(vec2(j,i)/iResolution.xy)).x!=-1.){
            //if(texture(iChannel1,p+(vec2(j,i)/iResolution.xy)).x>.1){
                t+=w;
           	}
            t1+=w;
        }
    }
    return t/t1;
}

bool hasPart(vec2 p)
{
    for(float i=-R2;i<=R2;i++){
    	for(float j=-R2;j<=R2;j++){
            if(length(vec2(i,j))>R2){
            	continue;
            }
            if(texture(iChannel0,p+(vec2(j,i)/iResolution.xy))!=vec4(-1.)){
            	return true;
           	}
        }
    }
    return false;
}

//convert HSV to RGB
vec3 hsv2rgb(vec3 c){
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

vec3 bg(vec2 p)
{
    return vec3(.5*mod(ceil(p.x) + ceil(p.y), 2.));
}

vec3 render(vec2 uv, vec2 p){
    float m=map(p,iTime);
    if(m<0.){
        if(m>-10.){
        	return vec3(.5+.4*dot(mapNormal(p,iTime),normalize(vec2(.5,1.))));
        }else{
        	return vec3(.5);
        }
    }else{
        float mc=1.;
        if(shadow){
        	//mc=1.-(1./(.05*m+1.));
            mc=1.-(1./(.1*m+1.));
        }
        if(rMode==3||rMode==4){
            float h=numPart(uv);
            if(rMode==3){
            	return mc*mix(vec3(0.),hsv2rgb(vec3(h*.5,1.,1.)),clamp(h,0.,1.));
            }
           	//return mc*mix(vec3(0.),hsv2rgb(vec3(h/20.,1.,1.)),clamp(h*10.,0.,1.));
            //return vec3(numPart(uv));
            //return mc*vec3(step(.2,h));
        }else{
            float h=texture(iChannel1,uv).x;
            vec3 sh=vec3(1./iResolution.xy,0.);
            h+=texture(iChannel1,uv+sh.xz).x+texture(iChannel1,uv+sh.zy).x
                +texture(iChannel1,uv-sh.xz).x+texture(iChannel1,uv-sh.zy).x;
            h/=5.;
            float rs=10.; //good values are 10, 20, 30
            if(rMode==0){
                return mc*mix(bg(p*4.),hsv2rgb(vec3(h/20.,1.,1.)),h);
            }else if(rMode==1){
                return mc*mix(vec3(0.),hsv2rgb(vec3(h/20.,1.,1.)),1.-h);
            }else if(rMode==2){
                return mc*mix(vec3(0.),hsv2rgb(vec3((h/20.)+.6,1.,1.)),h);
            }
        }
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec3 c=vec3(render(uv,fragCoord.xy));
    fragColor = vec4(c,1.0);
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//particle handling
vec4 getParticle(vec2 p)
{
    vec4 mass=vec4(0.,0.,0.,0.);
    float s=.8;//modify this value. .5 is default. 1. is big; above 1 is interesting ex. 2, 3
    float nh=.0;//modify this as well. 0. is default.
    float t=0.;
    for(float i=-R;i<=R;i++){
    	for(float j=-R;j<=R;j++){
            vec4 prt = texture(iChannel0,(p+vec2(j,i))/iResolution.xy);
            if(prt!=vec4(-1.)){
                if( abs(prt.x+prt.z-j)<=s && abs(prt.y+prt.w-i)<=s ){
                    mass+=vec4(prt.x+prt.z-j,prt.y+prt.w-i,prt.z,prt.w);
                    t++;
            	}
            }
            
    	}
    }
    if(t>nh){
    	return mass/t;
    }
    return vec4(-1.);
}
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    if(iFrame<=5){
    	fragColor = vec4(0.);
    }else{
        if(length(vec2(fragCoord.x-iMouse.x,fragCoord.y-iMouse.y))<20.&&iMouse.z > 0.){
            vec2 v=-R*normalize(vec2(fragCoord.x-iMouse.x+3.*sin(iTime*100.),fragCoord.y-iMouse.y+3.*sin(iTime*251.)));
            fragColor = vec4(0.0,0.0,v);
        }else{
        	vec4 p=getParticle(fragCoord);
            if(map(fragCoord,iTime)>=-R&&p.x!=-1.){
                vec2 rez=1./iResolution.xy;
                vec2 n=mapNormal(fragCoord+p.xy,iTime);
                //gravity
                vec2 ac=n*.1/(1.+.001*map(p.xy,iTime));
                vec2 nv=vec2(clamp(.999*p.z+ac.x,-R,R),clamp(.999*p.w+ac.y,-R,R));
                if(map(fragCoord+p.xy,iTime)<=0.){
                    //the normal vector can be made more accurate here by getting the exact collision
                    p.zw=1.1*reflect(vec2(p.z,p.w),n);
                    nv=vec2(clamp(p.z,-R,R),clamp(p.w,-R,R));
                }
                fragColor = vec4(p.x,p.y,nv);
            }else{
                fragColor = vec4(-1.);
            }
        }
    }
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//trails
const float str=.8; //good values are .8,.99,-.1
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    fragColor = vec4(texture(iChannel1,uv).xyz*str,1.0);
    if(texture(iChannel0,uv)!=vec4(-1.)){
    	fragColor +=1.;
    }
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//increase R as much as your computer can take. 4-5 is usually sufficient though.
//setting R to 10 is fun, albeit slow
const float R=4.;
const float R2=8.;
mat2 rot2(float a) {
	float c = cos(a);
	float s = sin(a);
	return mat2(c, s,-s, c);
}
float box(vec2 p,vec2 b)
{
    vec2 d = abs(p) - b;
    return min(max(d.x,d.y),0.0) + length(max(d,0.0));
}
float rotatedBox(vec2 p, vec2 b, float time)
{
    vec2 q=p;
    q.xy*=rot2(time);
	return box(q,b);
}
float circle(vec2 p,float r)
{
	return length(p)-r;
}
float edge(vec2 p)
{   
	float d=.5-length(p+vec2(-.5));
    d=max(d,.3-length(p-vec2(0.)));
    return d;
}
float grid(vec2 p)
{
	float gS=130.;
    vec2 q=gS*(fract(p/gS)-.5);
    return length(q)-gS/5.;
}
float items(vec2 p,float time)
{
	float d=box(p-vec2(80.,60.),vec2(40.,20.));
    d=min(d,circle(p-vec2(230.+40.*sin(time),100.+20.*sin(time)),30.));
    d=min(d,circle(p-vec2(400.,180.),80.));
    d=max(d,-circle(p-vec2(400.,180.),40.));
    d=max(d,-rotatedBox(p-vec2(400.,180.),vec2(90.,20.),time));
    d=min(d,rotatedBox(p-vec2(135.,115.),vec2(20.,80.),-.7));
    d=min(d,circle(p-vec2(800.,500.),100.));
    d=min(d,circle(p-vec2(300.,550.),30.));
    d=min(d,box(p-vec2(1000.,400.+300.*sin(time*.1)),vec2(50.,20.)));
    return d;
}
float map(vec2 p, float time)
{
    float d=items(p,time);
    return d;
    
}
vec2 mapNormal(vec2 p, float t)
{
	vec2 eps = vec2(0.0, 0.001);
    vec2 normal = normalize(vec2(
        map(p + eps.yx,t) - map(p - eps.yx,t),
        map(p + eps.xy,t) - map(p - eps.xy,t)));
    return normal;
}