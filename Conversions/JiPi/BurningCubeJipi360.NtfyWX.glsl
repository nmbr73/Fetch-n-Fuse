

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = fragCoord/iResolution.xy;
    vec4 col;
    vec4 col2;
    vec4 raw = texture(iChannel0, uv)*0.5+0.5;
    vec4 raw2 = raw;

    
        raw2*=-1.;
                col2 = raw2*0.5+0.5;

        col2 = vec4(col2.y*1.5+abs(col2.z)*0.9-0.9,col2.y*0.5+abs(col2.z)*0.3-0.5 ,col2.y*0.3+abs(col2.z)*0.3-0.5,0);

    if(raw.z < 0.9){
        raw *= -1.;
        
    }
        col = raw*0.5+0.5;
    
    fragColor = mix(col,col2, clamp(sin(iTime)+0.5,0.,1.));
    //fragColor =  texture(iChannel1, uv);
    
}
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
vec2 R;

vec4 tf(ivec2 p, int i, int j){
    //return texelFetch(iChannel0,p+ivec2(i,j),0);
    return texture(iChannel0,(vec2(p+ivec2(i,j))+0.5f)/R);
}
vec4 state(ivec2 p){
    //vec4 colNow = vec4(0,0,0,0);
    vec4 r = tf(p,0,0);
    vec4 colNow;
    float d0 = 0.05;
    int i0 = int(r.x * d0 + 1.)-1;
    int i1 = i0 + 1;
    int j0 = int(r.y*d0 + 1.)-1;
    int j1 = j0 + 1;
    float s1 = r.x*d0 - float(i0);
    float s0 = 1.-s1;
    float t1 = r.y*d0 - float(j0);
    float t0 = 1. - t1;
    colNow = t0*(s0 * tf(p,i0,j0) + s1 * tf(p,i1,j0)) +
        t1 *(s0 * tf(p,i0,j1) + s1 * tf(p,i1,j1));
    
    
    
    //for(int i = 0; i < 2; i++){
      //  for(int j = 0; j < 2; j++){
        //    colNow += tf(p, tx + i, ty + j) * 0.1 * abs((r.x*0.1 - float(tx - i)) - 1.) * abs((r.y*0.1- float(ty - j)) - 1.);
       // }
   // }
    
    
    
    for(int i = -1; i < 2; i++){
        for(int j = -1; j < 2; j++){
            //colNow += texelFetch(iChannel0, p + ivec2(i,j),0).rgba;
            //colNow += texture(iChannel0, (vec2( p + ivec2(i,j))+0.5f)/R).rgba;
            
            
            
                //int tx = floor()
            if(abs(i) + abs(j) < 3){
                vec4 l = tf(p,i,j);
                float infl = -float(i) * l.x + -float(j) * l.y;
                colNow += infl * l * -0.005f;
                float d = (l.z - r.z)*0.1;
                colNow += vec4(d*float(i),d*float(j),-d*0.02,0);
            }
            
        }
    }
    if(r.z > 0.){
        colNow+= vec4(0,-0.5,-0.05,0);
         
    }
    colNow+= vec4(sin(colNow.y*60.)*0.15,0,0,0);
    
    
        if(p.x < 1 || p.y < 1 || p.x > int(iResolution.x) - 2 || p.y > int(iResolution.y) -2){
    return vec4(0,0,0.3,0);
    }
    return clamp(colNow*1.,-10.,10.);
    
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{

    R=iResolution.xy;  
    // Amount, XV, YV, Tempurature;
    fragColor = vec4(0.1,0.1,1.0,1.0);
    
    vec4 col = state(ivec2(fragCoord));
    
    vec2 uv = fragCoord/iResolution.xy;
    if(iFrame < 5){
      col = texture(iChannel1, uv)*2.-1.;
    }
    
    col+= texture(iChannel2, uv);
    
    
    if(length((iMouse.xy)-fragCoord)<3.){
        col = vec4(sin(iTime*15.)*3.,-2.9,cos(iTime*10.)*3.5+5.5,0);
    }
    fragColor = col;
    
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
float sdBoxFrame( vec3 p, vec3 b, float e )
{
       p = abs(p  )-b;
  vec3 q = abs(p+e)-e;
  return min(min(
      length(max(vec3(p.x,q.y,q.z),0.0))+min(max(p.x,max(q.y,q.z)),0.0),
      length(max(vec3(q.x,p.y,q.z),0.0))+min(max(q.x,max(p.y,q.z)),0.0)),
      length(max(vec3(q.x,q.y,p.z),0.0))+min(max(q.x,max(q.y,p.z)),0.0));
}

float sdBox( vec3 p, vec3 b )
{
  vec3 q = abs(p) - b;
  return length(max(q,0.0)) + min(max(q.x,max(q.y,q.z)),0.0);
}

vec3 rotx(vec3 p, float a){
    float s = sin(a);
    float c = cos(a);
    return(vec3(p.x*c+p.y*s,-p.x*s+p.y*c,p.z));
}
float sdSph(vec3 p, float r){
    return length(p) - r;
}
vec4 getSDF(vec3 p, float time){
    vec3 pos = rotx(p,time);
    pos = rotx(pos.zyx,time*0.7);
    return vec4(0,-0.5,1.0,sdBoxFrame(pos,vec3(0.1,0.1,0.1),0.0015));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float time = iTime;
    vec2 uv = (fragCoord-0.5f*iResolution.xy)/iResolution.x;
     //vec2 pos = 0.25*vec2(sin(time),cos(time));
    //float d = length(uv-pos);
    
    
    vec3 pos = vec3(0.03*sin(time*1.0f),cos(time*0.5f),-sin(time*0.5f));
    vec3 dir = normalize(rotx(vec3(uv.x,uv.y,-1.).zxy,time*0.5f).zxy);
    
    vec4 col = vec4(0,0,0,0);
    int ma = 100;
    vec4 newcol;
    bool hit = false;
    for(int i = 0; i < ma; i ++){
        vec4 oc = getSDF(pos,time);
        newcol = oc;
        float dis = oc.w;
        
        col += vec4(.01/dis);
        if(dis < 0.0001){
            i = ma;
            hit = true;
        }
        if(dis > 10.){
            i = ma;
        }
        pos += dir * dis;
    }
    if(hit){
        col = newcol;
    }else{
        col = vec4(0,0,0,0);
    }
    
    
    
    
    //if(d<0.05){
    //    col = vec4(0,0,1.0,0);
    //}
    fragColor = col;
}