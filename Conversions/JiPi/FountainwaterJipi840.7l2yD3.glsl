

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<

void sinF(vec3 p, float freq, float amp, float phase, out float f, out vec3 df) {
    float m = length(p.xz);
    f = p.y-cos(freq*(m-phase))*amp*min(1.0, 6./m);
    df.y = 1.0;
    df.x = p.x/m;
    df.z = p.z/m;
    df.xz *= freq*sin(freq*(m-phase))*amp*min(1.0, 20./m);
    
}

float map(vec3 p) {
    vec3 p1 = p-vec3(-2.,0.0,0.0);
    float f;
    vec3 df;
    sinF(p1, 1.0, 2.0, 10.0*iTime, f, df);
    return (f)/length(df);
}

vec3 CalcNormal(vec3 p) {
    vec2 e = vec2(0.0, 0.001);
    vec3 n;
    n.x = map(p+e.yxx) - map(p-e.yxx);
    n.y = map(p+e.xyx) - map(p-e.xyx);
    n.z = map(p+e.xxy) - map(p-e.xxy);
    return normalize(n);
}

float raymarch(vec3 ro, vec3 rd) {
    float t = 0.001;
    for (int i=0; i<1000; i++) {
        float h = map(ro+t*rd);
        if (abs(h)<0.0001*t)
            return t;
        t += 1.0*h;
        if (t>80.0) return -1.0;
    }
}

float softShadow(vec3 ro, vec3 rd) {
    float sha = 1.0;
    float t = 0.01;
    for (int i=0; i<256; i++) {
    	float h = map(ro+t*rd);
        sha = min(sha, 2.0*h/t);
        t += 1.0*clamp(h,0.02,0.20);
        if (t>16.0) break;
    }
    sha = clamp(sha, 0.0, 1.0);
    return sha*sha*(3.0-2.0*sha);
}

mat2 rotMat(float ang) {
    return mat2(cos(ang), sin(ang), -sin(ang), cos(ang));
}

float fresnel(vec3 rd, vec3 normal) {
    float cosf = clamp(dot(rd, normal), -1., 1.);
    float n1, n2;
    if (cosf < 0.0) {
        n1 = 1.;
        n2 = 1.25;
        cosf = -cosf;
    }
    else {
        n1 = 1.25;
        n2 = 1.;
    }
    float sinf = n1/n2 * sqrt(max(0., 1.-cosf*cosf));
    if (sinf>=1.0) {
        return 1.0;
    }
    else {
        float cost = sqrt(max(0., 1.-sinf*sinf));
        float Rs = ((n1 * cosf) - (n2 * cost)) / ((n1 * cosf) + (n2 * cost));
        float Rp = ((n2 * cosf) - (n1 * cost)) / ((n2 * cosf) + (n1 * cost));
        return (Rs * Rs + Rp * Rp) /3.;
    }
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    vec2 uv = (2.*fragCoord - iResolution.xy) / iResolution.y;
    vec3 ro = vec3(0.0,15.0,-20.0);
    vec3 rd = normalize(vec3(uv, 1.));
    rd.zy = rotMat(-0.9) * rd.zy;
    
    vec3 lig = normalize(vec3(0.5, 1.0, -0.5));
    
    float r = raymarch(ro,rd);
    
    vec3 backCol = vec3(.9,.6, .2);
    float waterDepth = 2.;
    vec3 col;
    if (r>=0.0) {
    	vec3 pos = ro + r*rd;
    	vec3 normal = CalcNormal(pos);
     
        float glow = max(0.0, dot(reflect(rd, normal), lig));
        glow = pow(glow, 50.0);
        float fres =fresnel(rd, normal);
        float fresOp = 1.-fres;
        vec3 refVec = refract(rd, normal, 1.0/1.5);
        vec2 floorPos = refVec.xz / refVec.y * (pos.y+waterDepth) + pos.xz;
        
        if (pos.y+waterDepth<0.0) floorPos = vec2(0.0);
        
        
      
        col += texture(iChannel0, floorPos*0.04).xyz*1.2;
        col += vec3(glow);
        col *= fresOp;
        col += fres*backCol;
    }
    else {
        col = backCol;
    }
    
    col = pow(col,vec3(0.8));
    
    fragColor = vec4(col, 1.0);
}