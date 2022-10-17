

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//#define SHOW_DATA
//#define GREEN_BG

#if HW_PERFORMANCE==1
#define AA 2
#endif

#define PI 3.14159265359
#define PHI 1.618033988749895

// HG_SDF
void pR(inout vec2 p, float a) {
    p = cos(a)*p + sin(a)*vec2(p.y, -p.x);
}

// Rotate on axis
// blackle https://suricrasia.online/demoscene/functions/
vec3 erot(vec3 p, vec3 ax, float ro) {
  return mix(dot(ax,p)*ax, p, cos(ro))+sin(ro)*cross(ax,p);
}

float unlerp(float low, float high, float value) {
    return (value - low) / (high - low);
}


// --------------------------------------------------------
// Icosahedral domain mirroring
// knighty https://www.shadertoy.com/view/MsKGzw
// 
// Also get the face normal, and tangent planes used to
// calculate the uv coordinates later.
// --------------------------------------------------------

#define PI 3.14159265359

int Type=5;
vec3 nc;
vec3 pab;
vec3 pbc;
vec3 pca;

void init() {
    float cospin=cos(PI/float(Type)), scospin=sqrt(0.75-cospin*cospin);
    nc=vec3(-0.5,-cospin,scospin);
    pbc=vec3(scospin,0.,0.5);
    pca=vec3(0.,scospin,cospin);
    pbc=normalize(pbc);
    pca=normalize(pca);
	pab=vec3(0,0,1);
    pca *= 0.794654;
    pab *= 0.850651;
}

void fold(inout vec3 p) {
	for(int i=0;i<Type;i++){
		p.xy = abs(p.xy);
        p -= 2. * min(0., dot(p,nc)) * nc;
	}
}


float vmin(vec3 v) {
    return min(v.x, min(v.y, v.z));
}

float vmax(vec3 v) {
    return max(v.x, max(v.y, v.z));
}

vec2 triTile(vec2 p)
{ 
    vec2 hx = p * mat2(1,-1./1.73, 0,2./1.73);
    vec3 g = vec3(hx, 1.-hx.x-hx.y);
    vec3 id = floor(g);
    g = fract(g); 
    if (length(g) > 1.) g = 1. - g;
    vec3 axis = primaryAxis(g);
    float y = -(1./3. - vmin(g));
    float x = (vmax((1.-axis.yzx) * g) - vmax((1.-axis) * g)) * cos(1. / (PI / 3.));
    return vec2(x,y);
}

// --------------------------------------------------------
// Modelling
// --------------------------------------------------------

struct Model {
    float d;
    vec3 col;
    vec3 emissive;
    int id;
    bool isBound;
};

float smin(float a, float b, float k){
    float f = clamp(0.5 + 0.5 * ((a - b) / k), 0., 1.);
    return (1. - f) * a + f  * b - f * (1. - f) * k;
}

float smax(float a, float b, float k) {
    return -smin(-a, -b, k);
}

float cmin(float a, float b, float r) {
	return min(min(a, b), (a - r + b)*sqrt(0.5));
}

float cmax(float a, float b, float r) {
	return max(max(a, b), (a + r + b)*sqrt(0.5));
}


float pReflect(inout vec3 p, vec3 planeNormal, float offset, float soft) {
    float t = dot(p, planeNormal)+offset;
    float tr = sqrt(t * t + soft);
    p = p + (-t + tr) * planeNormal;
    return sign(t);
}


Model map(vec3 p) {
    p.z*= -1.;
    p = erot(p, normalize(pca), fract(gTime / gDuration / gSpeed + .2) * PI * 2. * (1./3.));

    float r = 2.;

    vec3 col = normalize(p) * .5 + .5;
    
    vec3 face, ab, atob;

    fold(p);
    
    pReflect(p, vec3(1,0,0), 0., .0001);
    pReflect(p, vec3(0,1,0), 0., .0001);
    pReflect(p, normalize(vec3(-1,-1.6,.615)), 0., .0001);
    
    face = pca;
    atob = pbc - pab;
    ab = pab;
    
    vec3 vv = normalize(face - ab);
    vec3 uu = normalize(atob);
    vec3 ww = face;
    mat3 m = mat3(uu,vv,ww);
       
    vec3 pp = p / dot(p, face);
    
    vec2 uv = (pp * m).xy;
    
    col = vec3(uv * vec2(1,-1), 0);
     
    float d = length(p) - 2.;
    
    vec4 data = texture(iChannel0, uv * vec2(1,-1) * vec2(1,2.));
    float border = data.r * r * .66;
    float tile = data.b;
    int id = int(data.g);
    float mask = data.a;

    float o = mix(-.25, .0, pow(tile/2., .5));
   
    float ito = ((linearstep(.2, .6 + o, tFract(gTime)) - pow(linearstep(.8 + o, .9 + o, tFract(gTime)), 4.)));
 
    if (tile == 1.) {
        ito *= -1.;
    }

    float inn = mix(.1, .4, tile/2.);
    
    ito *= .06;
    r += ito;
       
    float ws = 1.;
    float w = mix(.025, .1, mod(tile + 1., 3.) / 2.) * ws;

    
    float d0 = d;
    
    vec2 p2 = vec2(border, length(p) - r);
    d = p2.y;
    d = smin(d, dot(p2 - vec2(.005,0), normalize(vec2(-2.6,1))), .005);
    d = smax(d, p2.y - inn * .4, .005);

    if (id == 0) {
        float ii = mix(.25, .04, tile/3.);
        if (tile == 2.) {
            d = cmax(d, -(abs(border - .2)), .01);
        }
    }
    else
    {
       d = smax(d, dot(p2 - vec2(.1, inn * .4), normalize(vec2(1,1))), .005);
       d = cmin(d, max(d - .01, (abs(border - .1))), .01);
    }
    
    d = max(d, -(border - w / 6.));
        
    border *= 1.5;
    inn *= 1.5;
    
    col = vec3(.6);
    
    if (id == 1 && tile == 1.) {
        col *= .4;
    }
    
    if (id == 1 && tile == 0.) {
        col *= .7;
    }
    
    if (id == 1 && tile == 2.) {
        col *= 1.1;
    }

    col = mix(col, vec3(.15), (1.-smoothstep(.01 * ws*1.5, .04*ws*1.5, abs(border * 6. - inn))));
    
    ito= max(ito, 0.);
    col = mix(col, vec3(0), smoothstep(r + .025 - ito, r - .025 - ito, length(p)) * step(border, w + .01));
        
    col *= clamp(border * 5. + .7, 0., 1.);
    
    
    vec3 emissive = vec3(0);
    if (tile == 1.) {
        float l = (1. - clamp(abs(border - .2) * 100., 0., 1.)) * 2.;
        l += (1. - clamp(abs(border - .2) * 15., 0., 1.)) * .1;
        emissive += vec3(0,.8,.5) * l;
        
        float lt = tFract(gTime - .4);
        float ramp = 1. - linearstep(.025, .07, lt);
        ramp += linearstep(.8, .9, lt);
        ramp *= mix(1., sin(lt * 150.) * .5 + .5, linearstep(1.2, .9, lt));
        
        emissive *= ramp;
    }
    
    col *= .95;
    
    if (id == 1) {
        col *= .4;
    }
    
    return Model(d, col, emissive, id, false);
}

Model mapDebug(vec3 p) {
    Model m = map(p);
    return m;
    
    float d = -p.z;
    m.d = max(m.d, -d);
    d = abs(d) - .01;
    if (d < m.d) {
        return Model(d, vec3(0), vec3(0), 9, false);
    }
    return m;
}


// --------------------------------------------------------
// Rendering
// --------------------------------------------------------

float vmul(vec2 v) {
    return v.x * v.y;
}

// compile speed optim from IQ https://www.shadertoy.com/view/Xds3zN
vec3 calcNormal(vec3 pos){
    vec3 n = vec3(0.0);
    for( int i=0; i<4; i++ )
    {
        vec3 e = 0.5773*(2.0*vec3((((i+3)>>1)&1),((i>>1)&1),(i&1))-1.0);
        n += e*map(pos+0.0005*e).d;
    }
    return normalize(n);
}

mat3 calcLookAtMatrix(vec3 ro, vec3 ta, vec3 up) {
    vec3 ww = normalize(ta - ro);
    vec3 uu = normalize(cross(ww,up));
    vec3 vv = normalize(cross(uu,ww));
    return mat3(uu, vv, ww);
}

// origin sphere intersection
// returns entry and exit distances from ray origin
vec2 iSphere( in vec3 ro, in vec3 rd, float r )
{
	vec3 oc = ro;
	float b = dot( oc, rd );
	float c = dot( oc, oc ) - r*r;
	float h = b*b - c;
	if( h<0.0 ) return vec2(-1.0);
	h = sqrt(h);
	return vec2(-b-h, -b+h );
}

vec3 render( vec2 p )
{

    vec2 tuv = p;
    
    //tuv *= .333;
    //pR(tuv, PI / 12.);
    vec3 col = (vec3(.8) - p.y * .33) * .65;
    float w = fwidth(length(p)) / 2.;

    #ifndef LOOP
    float k = 1. / length(tuv) * 1.25;
    tuv = vec2(k, atan(tuv.x/tuv.y) * 1.101) * 1.;
    #ifdef LOOP2
    tuv += vec2(0,1) * fract(gTime / gDuration / gSpeed + .2) * 1.73;
    #else
    tuv += vec2(-.25,.5) * gTime * .25 * .5;
    #endif
    tuv = triTile(tuv);
    vec4 data = texture(iChannel0, tuv * vec2(1,-1) * vec2(1,2.) * 1.43);
    
    float lp = length(p) - .05;
    
    float ga = smoothstep(.9, 1.1, lp);
    ga *= smoothstep(1.9, 1.1, lp);
    ga = .01 * ga - .005;

    float gb = smoothstep(.6, 1., lp);
    gb *= smoothstep(1.5, 1., lp);
    gb = .01 * gb - .005;
    

    float g = smoothstep(ga + w, ga - w, abs(data.x));
    g += smoothstep(gb + w, gb - w, abs(data.x - .1));
    
    col += vec3(0,.8,.5) * g;

    //col = vec3(.03);
    
    float e = unlerp(1.7, .95, length(p));
    e = mix(.2, .00, e);
    //e = .06;
    
    //data.r /= k;

    float ms = smoothstep(1.8, .9, length(p));
    col += vec3(1) * step(e, data.r) * .15 * ms;
    #else
    col = vec3(.5);
    #endif
    
    #ifdef GREEN_BG
    col *= vec3(0,.8,.5) * 1.8;
    #endif
    
    //col = mix(col, vec3(1,0,0), clamp(unlerp(1., .75, length(p)), 0., 1.));
    
    
    //return vec3(1) * step(data.x, .01);
    
    vec3 camPos = vec3(0,0,9);
    
    vec2 im = iMouse.xy / iResolution.xy - .5;
    
    if (iMouse.x <= 0.)
    {
        im = vec2(0);
    }
    
    im += vec2(.66,.3);
    
    pR(camPos.yz, (.5 - im.y) * PI / 2.);
    pR(camPos.xz, (.5 - im.x) * PI * 1.5);
    
    mat3 camMat = calcLookAtMatrix(camPos, vec3(0), vec3(0,1,0));
    
    float focalLength = 3.;
    vec3 rayDirection = normalize(camMat * vec3(p.xy, focalLength));
    
    vec2 bound = iSphere(camPos, rayDirection, 2.3);
    if (bound.x < 0.) {
    	return col;
    }

    
    vec3 rayPosition = camPos;
    float rayLength = 0.;
    Model model;
    float dist = 0.;
    bool bg = false;
    float closestPass = 1e12;

    for (int i = 0; i < 100; i++) {
        rayLength += dist * .9;
        rayPosition = camPos + rayDirection * rayLength;
        model = mapDebug(rayPosition);
        dist = model.d;
        
        if ( ! model.isBound) {
            closestPass = min(closestPass, dist);
        }
        
        if (abs(dist) < .001) {
        	break;
        }
        
        if (rayLength > 15.) {
            bg = true;
            break;
        }
    }
    

    
    if ( ! bg) {
        col = model.col;
        vec3 nor = calcNormal(rayPosition);
        
        vec3 lin = vec3(0);
        
        vec3 rd = rayDirection;
        vec3 lig = normalize(vec3(-.1,1,-.1));
        vec3 hal = normalize(lig - rd );

        float dif, spe;

        dif = clamp(dot(lig, nor) * .75 + .4, 0., 1.);
        dif += sqrt(clamp( 0.5+0.5*nor.y, 0.0, 1.0 )) * .5;
        spe = pow(clamp(dot(nor, normalize(lig - rd)), 0., 1.), 100.);
        spe *= dif;
        spe *= .04 + .96 * pow(clamp(1. - dot(hal, lig), 0., 1.), 5.);
        lin += 1.3 * col * dif;
        lin += 6. * spe;
        
        lig = normalize(vec3(.5,-1,.5));
        hal = normalize(lig - rd );
        
        dif = clamp(dot(lig, nor), 0., 1.);        
        spe = pow(clamp(dot(nor, normalize(lig - rd)), 0., 1.), 100.);
        spe *= dif;
        spe *= .04 + .96 * pow(clamp(1. - dot(hal, lig), 0., 1.), 5.);       
        float m = clamp(dot(lig, normalize(rayPosition)) + 1., 0., 1.) * 1.5;
        lin += 1. * col * dif * vec3(0,.8,.5) * m;
        lin += 6. * spe * vec3(0,.8,.5) * m;
        
        col = lin;
                
        col += model.emissive;
        
        if (model.id == 9) {
           col = vec3(1,.5,.5) * fract(map(rayPosition).d * 20.);
        }
    }
    else
    {
        col = mix(col, vec3(.1), smoothstep(.015 + w*2., .015 - w*2., closestPass));
    }
    
    return col;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord) {
    initTime(iTime);

    #ifdef SHOW_DATA
        vec4 data = texture(iChannel0, fragCoord.xy / iResolution.xy);
        data.x = fract(data.x * 100.);
        fragColor = data;
        return;
    #endif
    
    init();
    
    vec2 o = vec2(0);
    vec3 col = vec3(0);

    // AA from iq https://www.shadertoy.com/view/3lsSzf
    #ifdef AA
    for( int m=0; m<AA; m++ )
    for( int n=0; n<AA; n++ )
    {
    	o = vec2(float(m),float(n)) / float(AA) - 0.5;
    	float d = 0.5*vmul(sin(mod(fragCoord.xy * vec2(147,131), vec2(PI * 2.))));
    #endif
		
    	vec2 p = (-iResolution.xy + 2. * (fragCoord + o)) / iResolution.y;
    	col += render(p);
        
    #ifdef AA
    }
    col /= float(AA*AA);
    #endif
    
    // colour grading from tropical trevor's scripts
    // https://github.com/trevorvanhoof/ColorGrading
    vec3 uGain = vec3(.0);
    vec3 uLift = vec3(.2);
    vec3 uOffset = vec3(-.225);
    vec3 uGamma = vec3(.3);
    col = pow(max(vec3(0.0), col * (1.0 + uGain - uLift) + uLift + uOffset), max(vec3(0.0), 1.0 - uGamma));
    col = pow( col, vec3(1./2.2) );
    
    fragColor = vec4(col, 0);
}


// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<

#define PI 3.14159265359

mat3 scaleM(float s) {
    return mat3(
        s, 0, 0,
        0, s, 0,
        0, 0, 1
    );
}

mat3 rotM(float a) {
    return mat3(
        cos(a), sin(a), 0,
        -sin(a), cos(a), 0,
        0, 0, 1
    );
}

mat3 transM(vec2 v) {
    return mat3(
        1, 0, v.x,
        0, 1, v.y,
        0, 0, 1
    );
}

vec2 mul(vec2 p, mat3 m) {
   return (vec3(p, 1) * m).xy;
}

void calcAngleOffset(float tf, float ts, out float angle, out vec2 offset) {
    float time = tf + ts;
    angle = time;
    offset = vec2(0, time);
    
    #ifndef LOOP
    #ifndef LOOP2
    return;
    #endif
    #endif
    
    float to = timeOffset / timeGap;

    angle = 0.;
    if (mod(tf, 3.) == 2.) {
        angle += mod(tf, 3.) + ts * -2.;
    } else{
        angle += mod(tf, 3.) + ts;
    }
    angle *= 1.;
    angle += to;

    vec3 ba = max(vec3(0), mod(vec3(tf + 2., tf + 1., tf), 3.) - 1.);
    vec3 bb = max(vec3(0), mod(vec3(tf + 3., tf + 2., tf + 1.), 3.) - 1.);
    vec3 bary = mix(ba, bb, ts);
    offset = bary.x * vec2(0,0) + bary.y * vec2(-1.,-.3) + bary.z * vec2(.3,-.333);

    offset.y += to;
}

mat3 gridTransformation(out float scale) {
    float tf = tFloor(gTime);
    float ts = easeSnap(linearstep(.3, .8, tFract(gTime)));

    scale = 2.5;
        
    float angle;
    vec2 offset;
    calcAngleOffset(tf, ts, angle, offset);  

    mat3 m = scaleM(scale);
    m *= rotM(PI * -.08 * angle);
    m *= transM(offset * -.78);
    return m;
}

mat3 gridTransformation2(out float scale) {
    float tf = tFloor(gTime);
    float ts = easeOutBack(linearstep(.6, .9, tFract(gTime)), 1.70158);
    
    scale = 3.5;
   
    float angle;
    vec2 offset;
    calcAngleOffset(tf, ts, angle, offset);  

    mat3 m = scaleM(scale);
    m *= rotM(PI * -.08 * angle * .5);
    m *= rotM(PI * 2./3.);
    m *= transM((offset * -0.78 * .5).yx);
    return m;
}

float effectMask(vec2 uv) {
    return sin(length(uv) * 12. + 1.) * .5 + .5;
}

// --------------------------------------------------------
// Triangle Voronoi
// tdhooper https://www.shadertoy.com/view/ss3fW4
// iq https://shadertoy.com/view/ldl3W8
// --------------------------------------------------------

float vmax(vec3 v) {
    return max(v.x, max(v.y, v.z));
}

const float s3 = sin(PI / 3.);

vec3 sdTriEdges(vec2 p) {
    return vec3(
        dot(p, vec2(0,-1)),
        dot(p, vec2(s3, .5)),
        dot(p, vec2(-s3, .5))
    );
}

float sdTri(vec2 p) {
    vec3 t = sdTriEdges(p);
    return max(t.x, max(t.y, t.z));
}

float sdTri(vec3 t) {
    return max(t.x, max(t.y, t.z));
}

float sdBorder(vec3 tbRel, vec2 pt1, vec2 pt2) {
    
    vec3 axis = primaryAxis(-tbRel);
    bool isEdge = axis.x + axis.y + axis.z < 0.;

    vec2 gA = vec2(0,-1);
    vec2 gB = vec2(s3, .5);
    vec2 gC = vec2(-s3, .5);
    
    vec2 norA = gC * axis.x + gA * axis.y + gB * axis.z;
    vec2 norB = gB * -axis.x + gC * -axis.y + gA * -axis.z;
    
    vec2 dir = gA * axis.x + gB * axis.y + gC * axis.z;
    vec2 corner = dir * dot(dir, pt1 - pt2) * 2./3.;
        
    vec2 ca, cb;
    float side;
    
    if (isEdge) {
        corner = pt2 + corner;
        ca = corner + max(0., dot(corner, -norB)) * norB;
        cb = corner + min(0., dot(corner, -norA)) * norA;
    } else {
        corner = pt1 - corner;
        ca = corner + max(0., dot(corner, -norA)) * norA;
        cb = corner + min(0., dot(corner, -norB)) * norB;
    }
    
    side = step(dot(corner, dir * mat2(0,-1,1,0)), 0.);
    corner = mix(ca, cb, side);
    
    float d = length(corner);

    return d;
}

vec2 hash2( vec2 p )
{
	return textureLod( iChannel0, (p+0.5)/256.0, 0.0 ).xy;
}

vec2 cellPoint(vec2 n, vec2 f, vec2 cell, bool gaps) {
    vec2 coord = n + cell;
    vec2 o = hash2( n + cell );
    if (gaps && hash2(o.yx * 10.).y > .5) {
        return vec2(1e12);
    }
    #ifdef ANIMATE
        o = 0.5 + 0.5*sin( time * PI * 2. + 6.2831*o );
    #endif	
    vec2 point = cell + o - f;
    return point;
}

vec4 voronoi(vec2 x, bool gaps)
{
    vec2 n = floor(x);
    vec2 f = fract(x);

    //----------------------------------
    // first pass: regular voronoi
    //----------------------------------
	vec2 closestCell, closestPoint;

    const int reach = 3;

    float closestDist = 8.0;
    for( int j = -reach; j <= reach; j++ )
    for( int i = -reach; i <= reach; i++ )
    {
        vec2 cell = vec2(i, j);
        vec2 point = cellPoint(n, f, cell, gaps);
        float dist = vmax(sdTriEdges(point));

        if( dist < closestDist )
        {
            closestDist = dist;
            closestPoint = point;
            closestCell = cell;
        }
    }


    //----------------------------------
    // second pass: distance to borders
    //----------------------------------
    closestDist = 8.0;
    for( int j = -reach-1; j <= reach+1; j++ )
    for( int i = -reach-1; i <= reach+1; i++ )
    {
        vec2 cell = closestCell + vec2(i, j);
        vec2 point = cellPoint(n, f, cell, gaps);

        vec3 triEdges = sdTriEdges(closestPoint - point);
        float dist = vmax(triEdges);

        if( dist > 0.00001 ) {
            closestDist = min(closestDist, sdBorder(triEdges, closestPoint, point));
        }
    }

    return vec4(closestDist, closestCell + n, 0.);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    initTime(iTime);

    vec2 uv = fragCoord.xy / iResolution.xy;
    uv *= vec2(1,-1);
    uv.y /= 2.;
    
    if (uv.x > uv.y * -2.) {
        fragColor = vec4(0);
        return;
    }

    float scl;
    mat3 m = gridTransformation(scl);

    vec4 v = voronoi(mul(uv, m), false);
    float d = v.x / scl;
    vec2 localPt = v.yz;
    vec2 worldPt = mul(localPt, inverse(m));
    vec2 seed = hash2(localPt);
    float id = 0.;
    float tile = mod(localPt.x + localPt.y, 3.);

    if (tile == 0.)
    {
        m = gridTransformation2(scl);
        v = voronoi(mul(uv, m), false);
        d = min(d, v.x / scl);
        localPt = v.yz;
        worldPt = mul(localPt, inverse(m));
        seed = hash2(localPt + seed * 10.);
        id = 1.;
        tile = mod(localPt.x + localPt.y, 3.);
    }

    float mask = effectMask(worldPt);
    
    fragColor = vec4(d, id, tile, mask);
}


// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//#define LOOP
//#define LOOP2

// skaplun https://www.shadertoy.com/view/7tf3Ws
float easeOutBack(float x, float t) {
    float c1 = t;
    float c3 = c1 + 1.;

    return 1. + c3 * pow(x - 1., 3.) + c1 * pow(x - 1., 2.);
}

float easeInOutBack(float x) {
    float c1 = 1.70158;
    float c2 = c1 * 1.525;

    return x < .5
      ? (pow(2. * x, 2.) * ((c2 + 1.) * 2. * x - c2)) / 2.
      : (pow(2. * x - 2., 2.) * ((c2 + 1.) * (x * 2. - 2.) + c2) + 2.) / 2.;
}

float easeSnap(float x) {
    x = pow(x, .75);
    x = easeInOutBack(x);
    return x;
}

float linearstep(float a, float b, float t) {
    return clamp((t - a) / (b - a), 0., 1.);
}

#ifdef LOOP
float timeOffset = (92. + 100.) * (3./4.);
float timeGap = 3.;
#else
#ifdef LOOP2
    float timeOffset = (92. + 100.);
    float timeGap = 4.;
#else
    float timeOffset = (92. + 100.);
    float timeGap = 4.;
#endif
#endif

float gTime;
float gDuration;
float gSpeed;

void initTime(float time) {
    gTime = time;
    gSpeed = 1.;
    gDuration = 14.;
    
    #ifdef LOOP
    gSpeed = 1.5;
    gDuration = (3. * timeGap) / gSpeed; // 6
    gTime /= gDuration;
    gTime = fract(gTime);
    gTime *= gDuration;
    gTime *= gSpeed;
    gTime += .25;
    #endif
    
    #ifdef LOOP2
    gSpeed = 1.;
    gDuration = (3. * timeGap) / gSpeed; // 12
    gTime /= gDuration;
    gTime = fract(gTime);
    gTime *= gDuration;
    gTime *= gSpeed;
    //gTime += .25;
    #endif
}

float tFloor(float time) {
    time += timeOffset;
    time -= timeGap / 3.;
    return floor(time / timeGap);
}

float tFract(float time) {
    time += timeOffset;
    time -= timeGap / 3.;
    return fract(time / timeGap) * timeGap * .5;
}

vec3 primaryAxis(vec3 p) {
    vec3 a = abs(p);
    return (1.-step(a.xyz, a.yzx))*step(a.zxy, a.xyz)*sign(p);
}