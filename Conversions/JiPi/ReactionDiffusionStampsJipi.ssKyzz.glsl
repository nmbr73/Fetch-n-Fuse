

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

void pixel( out vec4 fragColor, in vec2 fragCoord )
{
    setup(fragCoord, iResolution.xy, iTime, iFrame, iMouse);
    
    // SSAA
    vec3 col = c.yyy;
    float bound = sqrt(fsaa)-1.;
    for(float i = -.5*bound; i<=.5*bound; i+=1.)
        for(float j=-.5*bound; j<=.5*bound; j+=1.)
            col += texture(iChannel0, uv+vec2(i,j)*1.5/max(bound, 1.)*unit).xyz;
    col /= fsaa;

    // edge glow
    vec4 col11 = texture(iChannel0, uv - unit),
        col13 = texture(iChannel0, uv + unit*c.xz),
        col31 = texture(iChannel0 , uv + unit*c.zx),
        col33 = texture(iChannel0, uv + unit),
        x = col33 -col11 -3.* texture(iChannel0, uv + unit*c.yz) -col13 + col31 + 3.*texture(iChannel0, uv + unit*c.yx),
        y = col33 -col11 -3.* texture(iChannel0, uv + unit*c.zy) -col31 + col13 + 3.*texture(iChannel0, uv + unit*c.xy);
    fragColor = vec4(mix(col, 1.5*(abs(y.rgb) + abs(x.rgb)).rgb, .3),1.);


    // Vignette
    uv *=  1. - uv.yx;
    fragColor *= pow(uv.x*uv.y * 15., .2);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float ssaa = 1.;
    vec4 col = vec4(0.);
    float bound = sqrt(ssaa)-1.;
        for(float i = -.5*bound; i<=.5*bound; i+=1.)
            for(float j=-.5*bound; j<=.5*bound; j+=1.)
            {
                vec4 c1;
                float r = pi/4.;
                mat2 R = mat2(cos(r),sin(r),-sin(r),cos(r));
                pixel(c1, fragCoord.xy+R*(vec2(i,j)*1./max(bound, 1.)));
                    col += c1;
            }
    col /= ssaa;
    fragColor = col;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

float f = .0545,
    k = .062,
    bpm = 15.,
    spb = 60./15.,
    stepTime,
    nbeats,
    scale,
    pi = 3.14159,
    fsaa = 144.,
    hardBeats,
    time = 0.;
int frame = 0;
vec2 unit,
    r = vec2(1.,.5),
    uv,
    resolution;
vec3 c = vec3(1.,0.,-1.);
vec4 mouse;

// Creative Commons Attribution-ShareAlike 4.0 International Public License
// Created by David Hoskins.
// See https://www.shadertoy.com/view/4djSRW
vec2 hash22(vec2 p)
{
	vec3 p3 = fract(vec3(p.xyx) * vec3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx+33.33);
    return fract((p3.xx+p3.yz)*p3.zy);
}

// Creative Commons Attribution-ShareAlike 4.0 International Public License
// Created by David Hoskins.
// See https://www.shadertoy.com/view/4djSRW
float hash12(vec2 p)
{
	vec3 p3  = fract(p.xyx * .1031);
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

float lfnoise(vec2 t)
{
    vec2 i = floor(t);
    t = smoothstep(c.yy, c.xx, fract(t));
    vec2 v1 = vec2(hash12(i), hash12(i+c.xy)),
    v2 = vec2(hash12(i+c.yx), hash12(i+c.xx));
    v1 = c.zz+2.*mix(v1, v2, t.y);
    return mix(v1.x, v1.y, t.x);
}

float dbox3(vec3 x, vec3 b)
{
  b = abs(x) - b;
  return length(max(b,0.))
         + min(max(b.x,max(b.y,b.z)),0.);
}

// Distance to star
float dstar(vec2 x, float N, vec2 R)
{
    float d = pi/N,
        p0 = acos(x.x/length(x)),
        p = mod(p0, d);
    vec2 a = mix(R,R.yx,mod(round((p-p0)/d),2.)),
    	p1 = a.x*c.xy,
        ff = a.y*vec2(cos(d),sin(d))-p1;
    return dot(length(x)*vec2(cos(p),sin(p))-p1,ff.yx*c.zx)/length(ff);
}

float dhexagonpattern(vec2 p) 
{
    vec2 q = vec2(p.x*1.2, p.y + p.x*.6),
        qi = floor(q),
        pf = fract(q);
    float v = mod(qi.x + qi.y, 3.);
    
    return dot(step(pf.xy,pf.yx), 1.-pf.yx + step(1.,v)*(pf.x+pf.y-1.) + step(2.,v)*(pf.yx-2.*pf.xy));
}

float m(vec2 x)
{
    return max(x.x,x.y);
}

float d210(vec2 x)
{
    return min(max(max(max(max(min(max(max(m(abs(vec2(abs(abs(x.x)-.25)-.25, x.y))-vec2(.2)), -m(abs(vec2(x.x+.5, abs(abs(x.y)-.05)-.05))-vec2(.12,.02))), -m(abs(vec2(abs(x.x+.5)-.1, x.y-.05*sign(x.x+.5)))-vec2(.02,.07))), m(abs(vec2(x.x+.5,x.y+.1))-vec2(.08,.04))), -m(abs(vec2(x.x, x.y-.04))-vec2(.02, .08))), -m(abs(vec2(x.x, x.y+.1))-vec2(.02))), -m(abs(vec2(x.x-.5, x.y))-vec2(.08,.12))), -m(abs(vec2(x.x-.5, x.y-.05))-vec2(.12, .07))), m(abs(vec2(x.x-.5, x.y))-vec2(.02, .08)));
}

// x: material
// y: distance
// z: reflectivity
vec3 add(vec3 a, vec3 b)
{
    if(a.y < b.y) return a;
    return b;
}

vec3 hsv2rgb(vec3 cc)
{
    vec4 K = vec4(1., 2. / 3., 1. / 3., 3.);
    vec3 p = abs(fract(cc.xxx + K.xyz) * 6. - K.www);
    return cc.z * mix(K.xxx, clamp(p - K.xxx, 0., 1.), cc.y);
}

vec2 rgb2sv(vec3 cc)
{
    vec4 K = vec4(0., -1. / 3., 2. / 3., -1.),
        p = mix(vec4(cc.bg, K.wz), vec4(cc.gb, K.xy), step(cc.b, cc.g)),
        q = mix(vec4(p.xyw, cc.r), vec4(cc.r, p.yzx), step(p.x, cc.r));
    return vec2((q.x - min(q.w, q.y)) / (q.x + 1.e-10), q.x);
}

vec3 scene(vec3 x, sampler2D buffer)
{
    vec3 k = texture(buffer, mod(.5*(x.xy+.5*resolution/resolution.y),resolution.xy)).xyz;
    return vec3(3.+8.*(k.x+k.y)+.1*k.z*k.x*k.y+nbeats+.5*x.x*x.y, x.z+.015-.65*sqrt(abs(lfnoise(.1*x.xy+.66)*lfnoise(nbeats*c.xx+.31)))*k.y, .6)*vec3(1.,.25,1.);
}

vec3 palette(float scale)
{
    const int N = 4;
    vec3 colors[N] = vec3[N](
        vec3(0.16,0.22,0.24),
        vec3(0.90,0.29,0.37),
        vec3(1.00,0.51,0.49),
        vec3(1.00,0.80,0.67)
        
    );
    float i = mod(floor(scale), float(N)),
        ip1 = mod(i + 1., float(N));
    return mix(colors[int(i)], colors[int(ip1)], fract(scale));
}

bool ray(inout vec3 col, out vec3 x, float d, vec3 dir, out vec3 s, vec3 o, vec3 l, out vec3 n, sampler2D buffer)
{
    for(int i=0; i<250; ++i)
    {
        x = o + d * dir;
        s = scene(x, buffer);

        if(abs(x.z)>.15) break;
        
        if(s.y < 1.e-4)
        {
            // Blinn-Phong Illumination
            float dx = 5.e-5;
            n = normalize(vec3(
                scene(x+dx*c.xyy, buffer).y, 
                scene(x+dx*c.yxy, buffer).y, 
                scene(x+dx*c.yyx, buffer).y
            )-s.y);

            col = palette(s.x);

            col = .2 * col
                + col*max(dot(normalize(l-x),n),0.)
                + .7 * col*pow(max(dot(reflect(normalize(l-x),n),dir),0.),2.);
            
            if(x.z < -.01)
            {
                float cc = .035;
                vec2 a = mod(x.xy,cc)-.5*cc,
                    ai = x.xy-mod(x.xy+.5*cc,cc), 
                    y = abs(a)-.002;
                col = mix(col, .5*col, smoothstep(1.5/resolution.y, -1.5/resolution.y, min(y.x,y.y)))+.06*hash12(ai*1.e2);
            }

            return true;
        }
        d += abs(s.y);
    }
    return false;
}

void setup(vec2 fragCoord, vec2 res, float tm, int frm, vec4 ms)
{
    time = tm;
    frame = frm;
    resolution = res;
    stepTime = mod(time+.5*spb, spb)-.5*spb;
    nbeats = (11.+((time-stepTime+.5*spb)/spb + smoothstep(-.2*spb, .2*spb, stepTime)))*.33;
    scale = smoothstep(-.3*spb, 0., stepTime)*smoothstep(.3*spb, 0., stepTime);
    hardBeats = round((time-stepTime)/spb);
    uv = fragCoord/resolution;
    unit = c.xx/resolution;
    mouse = ms;
}

void simulate(vec2 fragCoord, sampler2D buffer, out vec4 fragColor)
{
    vec2 
        uv0 = 3.*(fragCoord.xy-.5*resolution.xy)/resolution.y,
    uva = uv0-3.*(hash22(hardBeats*c.xx)-.5);

    vec3 v = texture(buffer, fragCoord.xy*unit).xyz;
    vec2 u = v.xy;
    float s = hash12(hardBeats*c.xx),
        sdf;

    fragColor.z = v.z;

    // Boundary conditions
    k += .01*lfnoise(.1*uv0+ 2.131 + nbeats*c.xx);
    f += .01*lfnoise(.1*uv0 + nbeats*c.xx+1.31);

    // Mouse
    if(mouse.x != 0. && mouse.y != 0.)
    {
        vec2 uv1 = (mouse.zw-.5*resolution.xy)/resolution.y;
        vec3 
            o = c.yzx,
            dir = normalize(uv1.x * c.xyy + uv1.y * cross(c.xyy,normalize(-o))-o);
        vec2 uve = (o - (o.z)/dir.z * dir).xy;
        uve = .5*(uve.xy+.5*resolution/resolution.y);
        float la = length(fragCoord*unit - uve) - .1;
        if(la < 0.)
        {
            sdf = abs(la)-.005;
            u = max(u, .85*smoothstep(35.*unit.x, -35.*unit.x, sdf)*c.xx);
            if(sdf < 0.)
            {
                fragColor.z = time;
            }
        }
    }

    if(scale > .95)
    {
        if(s < .1)
            sdf = abs(dstar(uva, 5., vec2(.2,.5)))-.05;
        else if(s < .25)
            sdf = abs(length(uva)-.3)-.1-.13*sin(atan(uva.y,uva.x)*4.*pi);
        else if(s < .4)
            sdf = abs(length(uva)-.4)-.09;
        else if(s < .5)
            sdf = abs(dbox3(vec3(uva+uva.yx*c.xz,0), .4*c.xxx))-.09;
        else if(s < .55)
            sdf = 2.*d210(.5*uva);
        else if(s < .7)
            sdf = abs(dhexagonpattern(2.*uva)/2.)-.005;
        else if(s < .95)
            sdf = abs(mod(uva.x, .8)-.4+abs(uva.y)-.4)-.005;
        else fragColor.xy = c.yy;

        u = max(u, .85*smoothstep(35.*unit.x, -35.*unit.x, sdf)*c.xx);
        if(sdf < 0.)
        {
            fragColor.z = time;
        }
    }
    
    if(frame < 10)
    {
        sdf = abs(length(uv0+.3)-.3)-.1-.13*sin(atan(uv0.y+.3,uv0.x+.3)*4.*pi);
        fragColor.xy = max(u, .85*smoothstep(35.*unit.x, -35.*unit.x, sdf)*c.xx);
    }
    else
    {
        vec2 l = c.yy;
        vec3 wc = vec3(.05,.2,-1.);
        mat3 w = mat3(wc.xyx, wc.yzy, wc.xyx);

        // Laplace operator
        for(int i=0; i<3; ++i)
            for(int j=0; j<3; ++j)
                l += w[i][j]*texture(buffer, (fragCoord.xy + (1.5+.5*lfnoise(nbeats*c.xx))*vec2(ivec2(i,j)-1))*unit).xy;

        // Reaction-Diffusion system
        fragColor.xy = u + r*l + c.zx*u.x*u.y*u.y + vec2(f*(1.-u.x), -(f+k)*u.y)+.0005*hash22(uv0*1.e2);
        fragColor.xy = clamp(fragColor.xy, -1.,1.);

    }
}

vec3 shifthue(vec3 rgb, float hue)
{
    float cc = cos(hue),
        cs = sin(hue);
    mat3 yiq = mat3(
        .3,.6,.21,
        .59,-.27,-.52,
        .11,-.32,.31
    );
    rgb = yiq * rgb;
    rgb.yz *= mat2(cc,cs,-cs,cc);
    return  inverse(yiq) * rgb;
}

// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    setup(fragCoord, iResolution.xy, iTime, iFrame, iMouse);
    simulate(fragCoord, iChannel0, fragColor);
}

// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

void pixel( out vec4 fragColor, in vec2 fragCoord )
{
    setup(fragCoord, iResolution.xy, iTime, iFrame, iMouse);
    
    vec2 uv1 = (fragCoord-.5*iResolution.xy)/iResolution.y;
    vec3 
        o = c.yzx,
        col,
        c1,
        x,
        x1,
        n,
        dir = normalize(uv1.x * c.xyy + uv1.y * cross(c.xyy,normalize(-o))-o),
        l = c.zzx-.5*c.yyx,
        s,
        s1;

    // Material ray
    if(ray(col, x, -(o.z-.06)/dir.z, dir, s, o, l, n, iChannel0))
    {
        // Reflections
        if(ray(c1, x1, 2.e-3, reflect(dir,n), s1, x, l, n, iChannel0))
            col = mix(col, c1, s.z);

        // Hard Shadow
        if(ray(c1, x1, 1.e-2, normalize(l-x), s1, x, l, n, iChannel0) && length(l-x1) < length(l-x))
            col *= .5;
    }

    // Gamma
    col += col*col + col*col*col;
    col *= .75;
    col = mix(col, shifthue(col, 2.*pi*lfnoise(.1*nbeats*c.xx)), .5);
    col = mix(length(col)/sqrt(3.)*c.xxx, col, clamp(abs(x.z*100.),0.,1.));
    fragColor = mix(texture(iChannel1, uv), vec4(clamp(col,0.,1.),1.), .5);
}


void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    float ssaa = 1.;
    vec4 col = vec4(0.);
    float bound = sqrt(ssaa)-1.;
        for(float i = -.5*bound; i<=.5*bound; i+=1.)
            for(float j=-.5*bound; j<=.5*bound; j+=1.)
            {
                vec4 c1;
                float r = pi/4.;
                mat2 R = mat2(cos(r),sin(r),-sin(r),cos(r));
                pixel(c1, fragCoord.xy+R*(vec2(i,j)* 1./max(bound, 1.)));
                    col += c1;
            }
    col /= ssaa;
    fragColor = col;
}

// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    setup(fragCoord, iResolution.xy, iTime, iFrame, iMouse);
    simulate(fragCoord, iChannel0, fragColor);
}

// >>> ___ GLSL:[Buffer D] _________________________________________________________________ <<<
/*
 * Reaction-Diffusion-Stamps
 * 
 * Copyright (C) 2022  Alexander Kraus <nr4@z10.info>
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    setup(fragCoord, iResolution.xy, iTime, iFrame, iMouse);
    simulate(fragCoord, iChannel0, fragColor);
}
