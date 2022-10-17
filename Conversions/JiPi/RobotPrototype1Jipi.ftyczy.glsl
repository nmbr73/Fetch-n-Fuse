

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define PI 3.14159265359

vec2 robotAn;
float robotH;
vec3 robotPos;
float legLength;

mat2 rmatrix(float a)    //Rotation matrix;
{
	float c = cos(a);
	float s = sin(a);

	return mat2(c, -s, s, c);
}

vec3 getRayDir(vec3 cameraDir, vec2 coord, float cameraAngle)
{
	coord.y /= iResolution.x / iResolution.y;
	vec3 xAxis = normalize(vec3(-cameraDir.z, 0, cameraDir.x)) * tan(cameraAngle / 2.0);
	vec3 yAxis = normalize(cross(cameraDir, xAxis)) * tan(cameraAngle / 2.0) * -1.0;
	vec3 result = normalize(cameraDir + xAxis * coord.x + yAxis * coord.y);

	return (result);
}

float sdCapsule( vec3 p, vec3 a, vec3 b, float r )
{
  vec3 pa = p - a, ba = b - a;
  float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
  return length( pa - ba*h ) - r;
}

float sdfStruts(vec3 p) //sdf for strut parts
{
    vec3 p1;
    
    p1 = vec3(p.xy, abs(p.z)) - vec3(0.3, -0.2, 0.16);
    float strut = length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.40), p1.z)) - 0.02;
    
    //--forehead--
    
    p1 = vec3(p.xy, abs(p.z)) - vec3(0.03, 0.34, 0.16);
    
    p1.xy *= rmatrix(PI / 7.0 + PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.30), p1.z)) - 0.02);
    
    //--upper horizontal---
    
    p1 = vec3(p) - vec3(0.03, 0.34, 0.16);
    
    p1.zy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
    //--lower horizontal---
    
    p1 = vec3(p) - vec3(0.31, 0.21, 0.16);
    
    p1.zy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
    //--chin horizontal---
    
    p1 = vec3(p) - vec3(0.31, -0.21, 0.16);
    
    p1.zy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
    //--chin - spine---
    
    p1 = vec3(vec3(p.xy, abs(p.z))) - vec3(0.31, -0.21, 0.16);
    
    p1.xy *= rmatrix(-PI / 1.9);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.42), p1.z)) - 0.02);
    
    //--spine - back---
    
    p1 = vec3(vec3(p.xy, abs(p.z))) - vec3(-0.1086, -0.2446, 0.16);
    
    p1.xy *= rmatrix(-PI / 4.9);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
    //--back - up---
    
    p1 = vec3(vec3(p.xy, abs(p.z))) - vec3(-0.3, 0.0118, 0.16);
    
    p1.xy *= rmatrix(PI / 10.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.34), p1.z)) - 0.02);
    
    //--up - front---
    
    p1 = vec3(p.xy, abs(p.z)) - vec3(0.03, 0.34, 0.16);
    
    p1.xy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.22), p1.z)) - 0.02);
    
    //--up - back horizontal---
    
    p1 = vec3(p) - vec3(0.03 - 0.22, 0.34, 0.16);
    
    p1.zy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
    //--back horizontal---
    
    p1 = vec3(p) - vec3(-0.3, 0.0118, 0.16);
    
    p1.zy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
     //--spine horizontal---
    
    p1 = vec3(p) - vec3(-0.1086, -0.2446, 0.16);
    
    p1.zy *= rmatrix(-PI / 2.0);
    
    strut = min(strut, length(vec3(p1.x, p1.y - clamp(p1.y, 0.0, 0.32), p1.z)) - 0.02);
    
    
    return strut;
}

vec4 getHead(vec3 p, vec2 a) // sdf for head
{
    vec2 uv = vec2(0.0);
    float mat = 0.0;
    float t = 1000000.0;
    
    vec3 p1;
    
    p.xz *= rmatrix(a.x);
    p.xy *= rmatrix(a.y);
    
    if (length(p) > 0.7)  // This "if" is an optimization, which increases fps 3 times
        return vec4(length(p) - 0.6, uv, mat);
    
    //--FOREHEAD--
    
    p1 = p;
    
    p1.xy *= rmatrix(PI / 7.0);
    p1 -= vec3(0.055, 0.25, 0.0);
    
    float forehead = length(max(abs(p1) - vec3(0.145, 0.03, 0.16), vec3(0))) - 0.01;
    
    t = min(t, forehead);
    
    if (t == forehead)
    {
        if (p1.y > 0.0 && abs(p1.x) <= 0.145 && abs(p1.z) <= 0.16)
        {
            uv = p1.zx / vec2(-0.145, -0.16) * vec2(1.0, 1.5);
            mat = 9.0;
        }
    }
    
    p1 = p;
    p1.y -= 0.1;
    
    forehead = length(max(abs(p1 - vec3(-0.08, 0.16, 0.0)) - vec3(0.1, 0.03, 0.16), vec3(0))) - 0.01;
   
    t = min(t, forehead);
    
    
    if (t == forehead)
        mat = 5.0;
    
    //--BASE--
    
    float cube = length(max(abs(p - vec3(0.12, -0.05, 0.0)) - vec3(0.18, 0.2, 0.16), vec3(0))) - 0.01;
       
    p1 = p;
    
    vec2 d = abs(vec2(length(p1.xy), p1.z)) - vec2(0.24, 0.20);
    
    float cilinder = min(max(d.x, d.y), 0.0) + length(max(d, vec2(0.0))) - 0.01;
    
    t = min(t, cube);
    t = min(t, cilinder);
    
    if (t == cube)
    {
        mat = 4.0;
    }
    else if (t == cilinder)
    {
        mat = 3.0;
        if (abs(p.z) < 0.16)
            mat = 4.0;
    }
    
    //--STRUTS--
    
    float strut = sdfStruts(p + vec3(0.0, 0.05, 0.0));
    
    t = min(t, strut);
    
    if (t == strut)
        mat = 6.0;
    
    //--Display--
    
    float display = length(max(abs(p - vec3(0.32, -0.05, 0.0)) - vec3(0.01, 0.16, 0.16), vec3(0))) - 0.01;
   
    t = min(t, display);
    
    if (t == display)
    {
        p1 = p - vec3(0.32, -0.05, 0.0)
        ;
        if (p1.x > 0.0 && abs(p1.y) <= 0.16 && abs(p1.z) <= 0.16)
        {
            mat = 7.0;
            uv = p1.zy / vec2(0.16);
        }
        else
        {
            mat = 8.0;
        }
    }
    
    //--Torus--
    
    p1 = vec3(p.xy, abs(p.z)) - vec3(0.0, 0.00, 0.18);
    
    vec3 c = normalize(vec3(p1.xy, 0)) * 0.25;
    float torus = length(p1 - c) - 0.02;
    
    t = min(t, torus);
    
    
    if (t == torus)
        mat = 8.0;
    
    //--axels----
    
    
    p1 = p;
    
    d = abs(vec2(length(p1.xy), p1.z)) - vec2(0.15, 0.22);
    float cilinder1 = min(max(d.x, d.y), 0.0) + length(max(d, vec2(0.0))) - 0.01;
    
    t = min(t, cilinder1);
    
    if (t == cilinder1)
    {
        mat = 4.0;
    }
    
    d = abs(vec2(length(p1.xy), p1.z)) - vec2(0.05, 0.3);
    float cilinder2 = min(max(d.x, d.y), 0.0) + length(max(d, vec2(0.0))) - 0.01;
    
    t = min(t, cilinder2);
    
    if (t == cilinder2)
    {
        mat = 8.0;
    }
    
    float num = 6.0;
    
    float sector = round(atan(p1.x, p1.y) / (PI * 2.0 / num));
    
    p1.xy *= rmatrix(sector * (PI * 2.0 / num));
    
    p1.z = abs(p1.z);
    
    float tooths = length(max(abs(p1 - vec3(-0.0, 0.16, 0.18)) - vec3(0.04, 0.03, 0.03), vec3(0))) - 0.01;
   
    t = min(t, tooths);
    
    
    if (t == tooths)
        mat = 4.0;
    
    
	return vec4(t, uv, mat);
}

vec4 getLeg(vec3 p, vec2 an, float k)  // sdf of a leg
{
    float t = 1000000.0;
    vec2 uv = vec2(0.0);
    float mat = 10.0;
    
    vec3 p1;
    
    if (length(p.xz) > legLength * 0.7)  // Another optimization
        return vec4(length(p.xz) - legLength * 0.6, uv, mat);
    
    p.xz *= rmatrix(an.x);
    
    float l = legLength / 2.0;
    
    vec2 a, b, c;
    
    a = vec2(0.0, 0.0);
    
    b = vec2(-l * sqrt(1.0 - k * k), l * k);
    
    c = vec2(0.0, 2.0 * l * k);
    
    vec2 r;
    
    r = normalize((b - a) * mat2(0.0, -1.0, 1.0, 0.0)) * 0.07;
    
    p1 = p;
    p1.z = abs(p1.z);
    
    float leg1 = sdCapsule(p1, vec3(a + r, 0.32), vec3(b + r, 0.2), 0.02);
    
    t = leg1;
    
    float leg2 = sdCapsule(p1, vec3(a - r, 0.32), vec3(b -r, 0.2), 0.02);
    
    t = min(t, leg2);
    
    float leg3 = sdCapsule(p1, vec3(b, 0.1), vec3(c, 0.1), 0.04);
    
    t = min(t, leg3);
    
    p1 = p;
    p1.z = abs(p1.z);
    p1 -= vec3(a, 0.32);
    
    vec2 d = abs(vec2(length(p1.xy), p1.z)) - vec2(0.1, 0.02);
    float joinDown = min(max(d.x, d.y), 0.0) + length(max(d, vec2(0.0))) - 0.01;
    
    t = min(t, joinDown);
    
    p1 = p;
    p1.z = abs(p1.z);
    p1 -= vec3(b, 0.22);
    
    d = abs(vec2(length(p1.xy), p1.z)) - vec2(0.1, 0.02);
    float joinMiddle1 = min(max(d.x, d.y), 0.0) + length(max(d, vec2(0.0))) - 0.01;
    
    t = min(t, joinMiddle1);
    
    p1 = p;
    p1 -= vec3(b, 0.0);
    
    d = abs(vec2(length(p1.xy), p1.z)) - vec2(0.05, 0.2);
    float joinMiddle2 = min(max(d.x, d.y), 0.0) + length(max(d, vec2(0.0))) - 0.01;
    
    t = min(t, joinMiddle2);
    
    
    return vec4(t, uv, mat);
}

vec4 map(vec3 p) 
{
    vec2 uv = vec2(0.0);
    float mat = 0.0;
    float t = 1000000.0;
    
    vec3 p1;
    
    //---Background---
    
    uv = p.xz;
    mat = 0.0;
    
    float background = p.x - -1.0;
    
    t = background;
    
    if (t == background)
    {
        mat = 11.0;
        uv  = p.zy / 1.6;
    }
    
    //---Head---
    
    
    vec4 head = getHead(p - robotPos, robotAn);
    
    t = min(t, head.x);
    if (t == head.x)
    {
        uv = head.yz;
        mat = head.w;
    }
    
    //---Leg---
    
    vec4 leg = getLeg(p - robotPos, robotAn, robotH);
    
    t = min(t, leg.x);
    if (t == leg.x)
    {
        uv = leg.yz;
        mat = leg.w;
    }
    
    return vec4(t, uv, mat);
}

vec4 marchRay(vec3 rayOrigin, vec3 rayDir)  //almost classic raymarching
{
	float t;
	float d = 0.0;
	float e = 0.00001;
	float maxRange = 10.0;
	vec3 pos;
    vec4 info;

	for (t = 0.0; t <= maxRange; t += d)
	{
		pos = rayOrigin + rayDir * t;
        
        info = map(pos);
       
		
        d = info.x;

		if (d < e)
			break;
	}
	if (t > maxRange)
		return vec4(-1.0);
    
    info.x = t;
	return info;
}

vec3 getNorm(vec3 pos)
{
	vec2 e = vec2(0.001, 0);
	float tp = map(pos).x;

	vec3 norm = -normalize(vec3(map(pos - e.xyy).x - tp,
							   map(pos - e.yxy).x - tp,
							   map(pos - e.yyx).x - tp));
	return (norm);
}

float getEyesAnim(float t)
{
    return 1.0 - step(0.02, pow(cos(0.3 * t) * cos(0.5 * t), 16.0));
}

vec3 getFaceTex(vec2 uv0, float t)
{
    vec3 color = vec3(0.2);
    
    uv0.x += sin(t * 1.0) * sin (t * 1.5) * 0.016;
    vec2 uv = round(uv0 * 15.0) / 15.0;
    //uv = uv0;
    
    vec2 eyesSize = vec2(0.3, 0.05 + 0.25 * getEyesAnim(t));
    float eyes = length((vec2(abs(uv.x), uv.y) - vec2(0.4, 0.2)) / eyesSize);
    
    color = mix(vec3(1.0, 0.0, 0.0), color, step(1.0, eyes));
    
    vec2 d = abs(uv + vec2(0.0, 0.4)) - vec2(0.3, 0.05);
    float mouth = length(max(vec2(0.0), d)) + min(max(d.x,d.y), 0.0);
    
    color = mix(vec3(1.0, 0.0, 0.0), color, step(0.0, mouth));
    
    float b = length(uv0.y - uv.y);
    
    color *= vec3(1.0 - b * 20.0);
    
    return color;
}

vec3 getForeheadTex(vec2 uv)
{
    vec3 col = vec3(.8);
    
    vec2 uv1 = uv - vec2(-0.4, 0.0);
    
    vec2 q = uv1 / vec2(0.3, 0.5);
    
    float zero = pow(abs(q.x), 3.0) + pow(abs(q.y), 3.0);
    
    float zeroSlash = abs(uv1.y - uv1.x * 2.4);
    
    col = mix(col, vec3(1.0, 0.8, 0.0), 1.0 - step(0.4, abs(1.0 - zero)));
    col = mix(col, vec3(1.0, 0.8, 0.0), (1.0 - step(0.1, zeroSlash)) * (1.0 - step(1.0, zero)));
    
    uv1 = uv - vec2(0.4, 0.0);
    
    float one = length(max(abs(uv1) - vec2(0.01, 0.5), vec2(0.0))) - 0.05;
    
    uv1 = uv;
    
    uv1 -= vec2(0.28, 0.33);
    uv1 *= rmatrix(PI / 5.0);
    
    one = min(one, length(max(abs(uv1) - vec2(0.01, 0.2), vec2(0.0))) - 0.05);
    
    uv1 = uv - vec2(0.4, -0.5);
    
    one = min(one, length(max(abs(uv1) - vec2(0.25, 0.01), vec2(0.0))) - 0.05);
    
    col = mix(col, vec3(1.0, 0.8, 0.0), 1.0 - step(0.00, one));
    
    return (col);
}

vec3 getColor(vec4 info)
{
   float mat = info.w;
   vec2 uv = info.yz;
   
   if (mat == 1.0)
   {
       vec3 c1 = vec3(0, 98, 255) / 255.0;
       vec3 c2 = vec3(255, 21, 0) / 255.0;
       
       float k = sin(fract(uv.y + uv.x * 1.0) * PI);
       
       vec3 color = mix(c1, c2, k);
       
       color *= pow(abs(sin((uv.y + uv.x * 1.0) * PI / 2.0 * 32.0)), 10.0);
       
       return (color);
   }
   if (mat == 0.0)
   {
       vec2 id = floor(uv);
       if (mod((id.x + id.y), 2.0) == 0.0)
           return (vec3(0.5));
       return vec3(1.0);
   }
   if (mat == 2.0)
   {
       vec2 id = floor(uv);
       if (mod((id.x + id.y), 2.0) == 0.0)
           return (vec3(0.5));
       return vec3(1.0);
   }
   if (mat == 3.0)
       return vec3(245.0, 133.0, 54.0) / vec3(255.0);
   if (mat == 4.0)
       return vec3(0.3);
   if (mat == 5.0)
       return vec3(0.95);
   if (mat == 6.0)
       return vec3(0.2);
   if (mat == 7.0)
       return (getFaceTex(uv, iTime * 10.0));
   if (mat == 8.0)
       return vec3(0.6);
   if (mat == 9.0)
       return getForeheadTex(uv);
   if (mat == 10.0)
       return vec3(0.5);
   if (mat == 11.0)
   {
       if (max(abs(uv).x, abs(uv).y) > 1.0)
           return vec3(1.0);
       if (max(abs(uv).x, abs(uv).y) > 0.9)
       {
           vec3 c1 = vec3(0.1);
           vec3 c2 = vec3(1.0, 1.0, 0.0);
           
           float k = uv.y - uv.x;
           
           return mix(c1, c2, step(0.5, fract(k * 10.0)));
       }
       return vec3(0.4);
   }
   return (vec3(0));
}

float getShadow(vec3 rayOrigin, vec3 rayDir)
{
	float t;
	float d = 0.0;
	float e = 0.00001;
	float maxRange = 10.0;
    vec4 info;
	vec3 pos;
    
    float res = 1.0;

	for (t = 0.0; t <= maxRange; t += d)
	{
		pos = rayOrigin + rayDir * t;
        info = map(pos);
		d = info.x;
        res = min(res, d / t * 64.0);
		if (d < e)
			break;
	}
        
	return res;
}

void mainImage(out vec4 fragColor, in vec2 fragCoord)
{
    float d1 = 2.0;
    float d2 = 1.5;
    
    vec3 cameraPos;
    vec3 cameraDir;
    
    vec2 mouseuv = (iMouse.xy - iResolution.xy / 2.0) / iResolution.y * 2.0;
    
    cameraPos = vec3(d1, 0.0, 0.0);
    cameraDir = vec3(-1.0, 0.0, 0.0);
    
    vec3 rayDir = getRayDir(cameraDir, (fragCoord.xy / iResolution.xy) * 2.0 - 1.0, PI / 2.0);
	vec3 rayOrigin = cameraPos;
    
    legLength = 2.0;
    robotPos = vec3(0.0, mouseuv.y * -0.4, 0.0);
    robotH = (1.4 - robotPos.y) / legLength;
    
    vec3 mousePos = getRayDir(cameraDir, (iMouse.xy / iResolution.xy) * 2.0 - 1.0, PI / 2.0);;
    
    vec3 d = (mousePos - robotPos);
    
    robotAn = vec2(atan(d.z, d.x) + PI, atan(d.y, d.x) + PI);
    
    
    vec4 info = marchRay(rayOrigin, rayDir);
	float t = info.x;

	vec3 color = vec3(0);

	if (t != -1.0)
	{
		vec3 pos = rayOrigin + rayDir * t;
		vec3 lightDir = normalize(vec3(2.0, 0.6, -3.0) - pos);
		vec3 norm = getNorm(pos);
		float l = 1.0;
        float mat = info.w;
        
        color = getColor(info);
        
        if (mat != 7.0)
        {
            l = 0.3;
            float st = getShadow(pos + norm * 0.001, lightDir);

            l += max(0.0, dot(lightDir, norm)) * 0.7 * st;
        }

        color *= l;
	}

	fragColor = vec4(color, 1.0);
}