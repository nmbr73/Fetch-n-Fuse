

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// visualization

#define KEY_F 70
#define KEY_V 86

void mainImage(out vec4 fragColor,in vec2 fragCoord)
{
    ivec2 idx = ivec2(fragCoord * 0.5);
    //float rho = f0(iChannel0, idx);
    float rho = RHO(iChannel1, idx);
    float F_len = rho>0.001? F_Len(iChannel0, idx)*min(rho*5.,1.)*5.:0.;
    float v_len = rho>0.001? V_Len(iChannel0, idx)*min(rho*10.,1.)*2.:0.;
    float wall = Wall(iChannel2, idx);
    
    float flag_v = texelFetch( iChannel3, ivec2(KEY_V,2), 0 ).x;
    float flag_f = texelFetch( iChannel3, ivec2(KEY_F,2), 0 ).x>0.5?0.:1.;

    vec4 color = vec4(0,0,rho,1.);            // display fluid
    color += vec4(0,F_len*flag_f,0,0);        // display force
    color += vec4(wall,wall,wall,1.)*0.3;     // display wall
    color += vec4(v_len*flag_v,0,0,0);        // display velocity 

    ivec2 size = ivec2(iResolution*0.5);
    ivec2 center = ivec2(size.x/2,size.y/3*2);
    vec4 tube = vec4(0.25) *(step(length(vec2(center-idx)),9.5) - step(length(vec2(center-idx)),5.6));
    
    color += color.b<0.5?tube:vec4(0.);
    
    fragColor=color;
}
// >>> ___ GLSL:[Common] ___________________________________________________________________ <<<
//from @ndel https://www.shadertoy.com/view/4dK3zG

#define R iResolution.xy
//#define ORG
#ifdef ORG
    #define f0(tex,idx) (texelFetch(tex, idx*2, 0).r)
    #define f1(tex,idx) (texelFetch(tex, idx*2, 0).g)
    #define f2(tex,idx) (texelFetch(tex, idx*2, 0).b)
    #define f3(tex,idx) (texelFetch(tex, idx*2+ivec2(1,0),0).r)
    #define f4(tex,idx) (texelFetch(tex, idx*2+ivec2(1,0),0).g)
    #define f5(tex,idx) (texelFetch(tex, idx*2+ivec2(1,0),0).b)
    #define f6(tex,idx) (texelFetch(tex, idx*2+ivec2(0,1),0).r)
    #define f7(tex,idx) (texelFetch(tex, idx*2+ivec2(0,1),0).g)
    #define f8(tex,idx) (texelFetch(tex, idx*2+ivec2(0,1),0).b)
    #define RHO(tex,idx) (texelFetch(tex, idx*2+ivec2(1,1),0).r)
    #define F_Len(tex,idx) (texelFetch(tex, idx*2+ivec2(1,1),0).g)
    #define V_Len(tex,idx) (texelFetch(tex, idx*2+ivec2(1,1),0).b)
    #define PSI(tex,idx) (texelFetch(tex, idx*2+ivec2(1,1),0).a)
    #define Wall(tex, idx) (texelFetch(tex, idx*2,0).r)
#else
    #define f0(tex,idx)    (texture(tex, (vec2(idx*2)+0.5)/R).r)
    #define f1(tex,idx)    (texture(tex, (vec2(idx*2)+0.5)/R).g)
    #define f2(tex,idx)    (texture(tex, (vec2(idx*2)+0.5)/R).b)
    #define f3(tex,idx)    (texture(tex, (vec2(idx*2+ivec2(1,0))+0.5)/R).r)
    #define f4(tex,idx)    (texture(tex, (vec2(idx*2+ivec2(1,0))+0.5)/R).g)
    #define f5(tex,idx)    (texture(tex, (vec2(idx*2+ivec2(1,0))+0.5)/R).b)
    #define f6(tex,idx)    (texture(tex, (vec2(idx*2+ivec2(0,1))+0.5)/R).r)
    #define f7(tex,idx)    (texture(tex, (vec2(idx*2+ivec2(0,1))+0.5)/R).g)
    #define f8(tex,idx)    (texture(tex, (vec2(idx*2+ivec2(0,1))+0.5)/R).b)
    #define RHO(tex,idx)   (texture(tex, (vec2(idx*2+ivec2(1,1))+0.5)/R).r)
    #define F_Len(tex,idx) (texture(tex, (vec2(idx*2+ivec2(1,1))+0.5)/R).g)
    #define V_Len(tex,idx) (texture(tex, (vec2(idx*2+ivec2(1,1))+0.5)/R).b)
    #define PSI(tex,idx)   (texture(tex, (vec2(idx*2+ivec2(1,1))+0.5)/R).a)
    #define Wall(tex, idx) (texture(tex, (vec2(idx*2)+0.5)/R).r)
#endif 
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
// collision step

const float G_self = 1.2;  // affect the surface tension of the liquid molecules
const float G_wall = 0.46; // affect the surface tension between the liquid and the wall
const float g = 0.005;    // gravity
const float k = 1.65;      // affect the viscidity, range: 0.1~1.9

bool is_wall(in ivec2 idx, in ivec2 size){
    return (size.x < idx.x + 1) || (size.y < idx.y + 1) || (idx.x < 0) || (idx.y < 0) || (Wall(iChannel2, idx) > 0.1);
}

// kernal
const ivec2 off[25] = ivec2[25](
    ivec2( 0, 0 ),
    ivec2( 1, 0 ),ivec2( 0, -1 ),ivec2( -1, 0 ),ivec2( 0, 1 ),
    ivec2( 1, -1 ), ivec2( -1, -1 ), ivec2( -1, 1 ), ivec2( 1, 1 ),
    ivec2( 2, 0 ), ivec2( 0, -2 ), ivec2( -2, 0 ), ivec2( 0, 2 ),
    ivec2( 1, -2 ), ivec2( -1, -2 ), ivec2( -1, 2 ), ivec2( 1, 2 ),
    ivec2( 2, -1 ), ivec2( -2, -1 ), ivec2( -2, 1 ), ivec2( 2, 1 ),
    ivec2( 2, -2 ), ivec2( -2, -2 ), ivec2( -2, 2 ), ivec2( 2, 2 )
);

const float w[9] = float[9](
    0., 1. / 21., 4. / 45., 0., 1. / 60., 2. / 315., 0., 0., 1. / 5040.
);

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 idx = ivec2(fragCoord*0.5);
    ivec2 size = ivec2(iResolution*0.5);
    
    if(idx.x >= size.x || idx.y >= size.y)
    {
        discard;
    }
    
    // density distribution
    float f0,f1,f2,f3,f4,f5,f6,f7,f8;
    // total density
    float rho;

    // get f0~8 from BufferB
    f0 = f0(iChannel1, idx);
    f1 = f1(iChannel1, idx);
    f2 = f2(iChannel1, idx);
    f3 = f3(iChannel1, idx);
    f4 = f4(iChannel1, idx);
    f5 = f5(iChannel1, idx);
    f6 = f6(iChannel1, idx);
    f7 = f7(iChannel1, idx);
    f8 = f8(iChannel1, idx);
    
    // density
    rho = f0 + f1 + f2 + f3 + f4 + f5 + f6 + f7 + f8 + 1e-30;
    // velocity
    vec2 v1 = vec2(1. / rho * (f1 - f2 + f5 - f6 - f7 + f8), 1. / rho * (f3 - f4 + f5 - f6 + f7 - f8));

    // calculate external force (surface tension)
    vec2 F = vec2(0, 0);
    for(int i = 1; i < 25; i++) {
        ivec2 offset = off[i];
        ivec2 idx2 = idx + offset;
        float psi_self = PSI(iChannel1, idx2);
        float psi_wall = is_wall(idx2, size) ? 1. : 0.;
        F += (G_self * psi_self + G_wall * psi_wall) * vec2(offset) * w[offset.x * offset.x + offset.y * offset.y];
    }
    
    vec2 a = F / rho + vec2(0, -g); //gravity
    vec2 v2 = v1 + a;
    
    // speed limit
    float max_speed = 0.57735027;
    if(length(v1) > max_speed-1e-2) {
        v1 = normalize(v1) * (max_speed-1e-2);
    }
    if(length(v2) > max_speed) {
        v2 = normalize(v2) * max_speed;
    }

    // f_new_i = (1-k)*f_i + (k-1)*f_eq_i + f_eq_new_i
    // f_eq_new_i = f_eq(i, v_new)
    float sq_term = - 1.5 * (v1.x * v1.x + v1.y * v1.y);
    float f0eq = 4. / 9. * rho * (1. + sq_term);
    float f1eq = 1. / 9. * rho * (1. + 3. * v1.x + 4.5 * v1.x * v1.x + sq_term);
    float f2eq = 1. / 9. * rho * (1. - 3. * v1.x + 4.5 * v1.x * v1.x + sq_term);
    float f3eq = 1. / 9. * rho * (1. + 3. * v1.y + 4.5 * v1.y * v1.y + sq_term);
    float f4eq = 1. / 9. * rho * (1. - 3. * v1.y + 4.5 * v1.y * v1.y + sq_term);
    float f5eq = 1. / 36. * rho * (1. + 3. * (v1.x + v1.y) + 4.5 * (v1.x + v1.y) * (v1.x + v1.y) + sq_term);
    float f6eq = 1. / 36. * rho * (1. - 3. * (v1.x + v1.y) + 4.5 * (v1.x + v1.y) * (v1.x + v1.y) + sq_term);
    float f7eq = 1. / 36. * rho * (1. + 3. * (- v1.x + v1.y) + 4.5 * (- v1.x + v1.y) * (- v1.x + v1.y) + sq_term);
    float f8eq = 1. / 36. * rho * (1. - 3. * (- v1.x + v1.y) + 4.5 * (- v1.x + v1.y) * (- v1.x + v1.y) + sq_term);

    float sq_term2 = - 1.5 * (v2.x * v2.x + v2.y * v2.y);
    float f0eq2 = 4. / 9. * rho * (1. + sq_term2);
    float f1eq2 = 1. / 9. * rho * (1. + 3. * v2.x + 4.5 * v2.x * v2.x + sq_term2);
    float f2eq2 = 1. / 9. * rho * (1. - 3. * v2.x + 4.5 * v2.x * v2.x + sq_term2);
    float f3eq2 = 1. / 9. * rho * (1. + 3. * v2.y + 4.5 * v2.y * v2.y + sq_term2);
    float f4eq2 = 1. / 9. * rho * (1. - 3. * v2.y + 4.5 * v2.y * v2.y + sq_term2);
    float f5eq2 = 1. / 36. * rho * (1. + 3. * (v2.x + v2.y) + 4.5 * (v2.x + v2.y) * (v2.x + v2.y) + sq_term2);
    float f6eq2 = 1. / 36. * rho * (1. - 3. * (v2.x + v2.y) + 4.5 * (v2.x + v2.y) * (v2.x + v2.y) + sq_term2);
    float f7eq2 = 1. / 36. * rho * (1. + 3. * (- v2.x + v2.y) + 4.5 * (- v2.x + v2.y) * (- v2.x + v2.y) + sq_term2);
    float f8eq2 = 1. / 36. * rho * (1. - 3. * (- v2.x + v2.y) + 4.5 * (- v2.x + v2.y) * (- v2.x + v2.y) + sq_term2);

    f0 = (1. - k) * f0 + (k - 1.) * f0eq + f0eq2;
    f1 = (1. - k) * f1 + (k - 1.) * f1eq + f1eq2;
    f2 = (1. - k) * f2 + (k - 1.) * f2eq + f2eq2;
    f3 = (1. - k) * f3 + (k - 1.) * f3eq + f3eq2;
    f4 = (1. - k) * f4 + (k - 1.) * f4eq + f4eq2;
    f5 = (1. - k) * f5 + (k - 1.) * f5eq + f5eq2;
    f6 = (1. - k) * f6 + (k - 1.) * f6eq + f6eq2;
    f7 = (1. - k) * f7 + (k - 1.) * f7eq + f7eq2;
    f8 = (1. - k) * f8 + (k - 1.) * f8eq + f8eq2;
    
    // store f0~8 and rho,Fx,Fy from @ndel https://www.shadertoy.com/view/4dK3zG
    int itx = int(fragCoord.x) - 2*idx.x;
    int ity = int(fragCoord.y) - 2*idx.y;
    if(itx==0&&ity==0)//stores f0,f1,f2
        fragColor = vec4(f0,f1,f2,1.);
    else if(itx==1&&ity==0)//stores f3,f4,f5
        fragColor = vec4(f3,f4,f5,1.);
    else if(itx==0&&ity==1)//stores f6,f7,f8
        fragColor = vec4(f6,f7,f8,1.);
    else //stores
        fragColor = vec4(rho,length(F),length((v1+v2)*0.5),1.);
}
// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
// streaming step

bool is_wall(in ivec2 idx, in ivec2 size){
    return (size.x < idx.x + 1) || (size.y < idx.y + 1) || (idx.x < 0) || (idx.y < 0) || (Wall(iChannel2, idx) > 0.1);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ){

    ivec2 idx = ivec2(fragCoord*0.5);
    ivec2 size = ivec2(iResolution*0.5);

    float f0, f1, f2, f3, f4, f5, f6, f7, f8;

    if(!is_wall(idx,size))
    {
        ivec2 idx2;
        ivec2 center = ivec2(size.x/2,size.y/3*2);
        if( iFrame%5==1 && (f0 < 0.1) && length(vec2(center-idx))<6.) // water source
        {
            f4 = 1.; //↓
            //f1 = 1.; //→
        }
        else // streaming and handling boundary condition 
        {
            f0 = f0(iChannel0, idx);
            idx2 = idx + ivec2(-1, 0);
            f1 = is_wall(idx2, size) ? f2(iChannel0, idx) : f1(iChannel0, idx2);
            idx2 = idx + ivec2(1, 0);
            f2 = is_wall(idx2, size) ? f1(iChannel0, idx) : f2(iChannel0, idx2);
            idx2 = idx + ivec2(0, -1);
            f3 = is_wall(idx2, size) ? f4(iChannel0, idx) : f3(iChannel0, idx2);
            idx2 = idx + ivec2(0, 1);
            f4 = is_wall(idx2, size) ? f3(iChannel0, idx) : f4(iChannel0, idx2);
            idx2 = idx + ivec2(-1, -1);
            f5 = is_wall(idx2, size) ? f6(iChannel0, idx) : f5(iChannel0, idx2);
            idx2 = idx + ivec2(1, 1);
            f6 = is_wall(idx2, size) ? f5(iChannel0, idx) : f6(iChannel0, idx2);
            idx2 = idx + ivec2(1, -1);
            f7 = is_wall(idx2, size) ? f8(iChannel0, idx) : f7(iChannel0, idx2);
            idx2 = idx + ivec2(-1, 1);
            f8 = is_wall(idx2, size) ? f7(iChannel0, idx) : f8(iChannel0, idx2);
        }
    }
    
    const float rho0 = 1.;
    float rho = f0+f1+f2+f3+f4+f5+f6+f7+f8+1e-30;
    float psi = rho0 * (1.f - exp(-abs(rho) / rho0));

    // store f0~8, rho, psi from @ndel https://www.shadertoy.com/view/4dK3zG
    int itx = int(fragCoord.x) - 2*idx.x;
    int ity = int(fragCoord.y) - 2*idx.y;
    if(itx==0&&ity==0)//stores f0,f1,f2
        fragColor = vec4(f0,f1,f2,1.);
    else if(itx==1&&ity==0)//stores f3,f4,f5
        fragColor = vec4(f3,f4,f5,1.);
    else if(itx==0&&ity==1)//stores f6,f7,f8
        fragColor = vec4(f6,f7,f8,1.);
    else //stores rho psi
        fragColor = vec4(rho,0,0,psi);
}
// >>> ___ GLSL:[Buffer C] _________________________________________________________________ <<<
// draw wall

#define IsInBox(box, idx) (idx.x > box.x && idx.x < box.y && idx.y > box.z && idx.y < box.w)
#define IsInCircle(o,r,idx) (length(o-vec2(idx))<r)
#define KEY_SPACE 32
#define KEY_E 69

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    ivec2 idx = ivec2(fragCoord*0.5);
    ivec2 size = ivec2(iResolution*0.5);
    ivec4 box1 = ivec4(size.x/2-10,size.x/3*2,size.y/3-20,size.y/3-10);
    ivec4 box2 = box1-size.y/8;
    ivec4 box3 = box2+ivec4(size.y/4,size.y/4,-15,-15);
    ivec4 box4 = ivec4(size.x/3-20,size.x/3-10,5,60);
    ivec4 box5 = box4 - ivec4(12,12,0,0);
    ivec4 box6 = box4 + ivec4(200,200,-20,20);
    ivec4 box7 = box4 - ivec4(27,27,10,20);
    ivec4 box8 = box4 + ivec4(11,11,0,10);
    // init
    if(iFrame==0)
    {
        if(IsInBox(box1,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }
        else if(IsInBox(box2,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }
        else if(IsInBox(box3,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }        
        else if(IsInBox(box4,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }        
        else if(IsInBox(box5,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }      
        else if(IsInBox(box6,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        } 
        else if(IsInBox(box7,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }
        else if(IsInBox(box8,idx)) {
           fragColor = vec4(1.,1.,1.,1.);
        }
    }
    else
    {
        float wall = Wall(iChannel2,idx);
        //...
        // mouse and keyboard
        float pressSpace = texelFetch( iChannel3, ivec2(KEY_SPACE,0.0), 0 ).x;
        float pressE = texelFetch( iChannel3, ivec2(KEY_E,0.0), 0 ).x;
        if(iMouse.z>0.5 && IsInCircle(iMouse.xy/2.,5.,idx))
        {
            wall = (pressSpace>0.5 || pressE > 0.5) ? 0. : 1.;
        }
        
        fragColor = vec4(wall,wall,wall,1.);
    }
}