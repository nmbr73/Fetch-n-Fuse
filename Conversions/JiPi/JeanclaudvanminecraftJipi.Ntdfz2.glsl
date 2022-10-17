

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by Andrew Wild - akohdr/2016
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

//#define ROTATE
//#define TECHN_IQ_COLOR
    
// Lower sampling res
//#define LOOP 199
//#define RES 1.

// Higher sampling res.
#define LOOP 899
#define RES 6.0

bool isVoxel(out vec4 k, const in vec4 P ) 
{
    vec3 a = abs(P.xyz);
    if(a.z<1.) {
        
        vec2 uv = P.xy/iChannelResolution[1].xy;
        if(uv.x < -.02 || uv.y>.03) return false;
        uv -= vec2(.05,.03);
        uv *= 15.;
        
        k = texture(iChannel1, 1.+uv);

#ifdef TECHN_IQ_COLOR
    float maxrb = max( k.r, k.b );
    float dg = k.g; 
    k.g = min( k.g, maxrb*0.8 ); 
    k += dg - k.g;
#endif
        
        return ((k.g<.3) && ((k.r>.0)||(k.b>.0))) ||
               ((k.g<.6) && ((k.r>.3)||(k.b>.4))) ||
               ((k.g<.9) && ((k.r>.7)||(k.b>.7)));
    }
    return false;
}

void mainImage(out vec4 k, vec2 P) 
{
     vec2 R = iResolution.xy,
          h = vec2(0,.5),
          u = (P - h*R.y)/R.x - h.yx;
#ifdef ROTATE
    float T = 3.*cos(iTime);
#else
    float T = 2.3;
#endif
    
    vec3 v = vec3(cos(T), 1, sin(T)),
         r = mat3(u.x,    0,   .8,
                    0,  u.y,    0,
                  -.8,    0,  u.x) * v,
         o = vec3(50,2.,-50) * v.zzx,
         f = floor(o),
         q = sign(r),
         d = abs(length(r)/r),
         s = d * ( q*(f-o + .5) +.5), 
         m;

    for(int i=0; i<LOOP; i++) {
        float a = s.x, b = s.y, c = s.z;
        s += d*(m = vec3(a<b&&a<=c, b<c&&b<=a, c<a&&c<=b));
        f += m/RES*q;
        
        if(isVoxel(k, vec4(f.xyz, T)) || isVoxel(k, vec4(f.zyx, T))) {
            k += m.x>.0 ? vec4(0) : m.y>.0 ? vec4(.6) : vec4(.3); return; }//early exit
    }
    k = texture(iChannel0, P/R.xy)/3.;
}