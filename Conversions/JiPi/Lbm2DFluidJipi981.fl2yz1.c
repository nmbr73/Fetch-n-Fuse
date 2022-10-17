
// ----------------------------------------------------------------------------------
// - Common                                                                         -
// ----------------------------------------------------------------------------------


//from @ndel https://www.shadertoy.com/view/4dK3zG
#define f0(tex,idx) (texelFetch(tex, idx*2, 0).r)
#define f1(tex,idx) (texelFetch(tex, idx*2, 0).g)
#define f2(tex,idx) (texelFetch(tex, idx*2, 0).b)
#define f3(tex,idx) (texelFetch(tex, idx*2+to_int2(1,0),0).r)
#define f4(tex,idx) (texelFetch(tex, idx*2+to_int2(1,0),0).g)
#define f5(tex,idx) (texelFetch(tex, idx*2+to_int2(1,0),0).b)
#define f6(tex,idx) (texelFetch(tex, idx*2+to_int2(0,1),0).r)
#define f7(tex,idx) (texelFetch(tex, idx*2+to_int2(0,1),0).g)
#define f8(tex,idx) (texelFetch(tex, idx*2+to_int2(0,1),0).b)
#define RHO(tex,idx) (texelFetch(tex, idx*2+to_int2(1,1),0).r)
#define F_Len(tex,idx) (texelFetch(tex, idx*2+to_int2(1,1),0).g)
#define V_Len(tex,idx) (texelFetch(tex, idx*2+to_int2(1,1),0).b)
#define PSI(tex,idx) (texelFetch(tex, idx*2+to_int2(1,1),0).a)
#define Wall(tex, idx) (texelFetch(tex, idx*2,0).r)

// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0
// Connect Buffer A 'Previsualization: Buffer B' to iChannel1
// Connect Buffer A 'Previsualization: Buffer C' to iChannel2


// collision step

const float G_self = 1.2f;  // affect the surface tension of the liquid molecules
const float G_wall = 0.46f; // affect the surface tension between the liquid and the wall
const float g = 0.005f;    // gravity
const float k = 1.65f;      // affect the viscidity, range: 0.1~1.9

__DEVICE__ bool is_wall(in int2 idx, in int2 size){
    return (size.x < idx.x + 1) || (size.y < idx.y + 1) || (idx.x < 0) || (idx.y < 0) || (Wall(iChannel2, idx) > 0.1f);
}

// kernal
const int2 off[25] = ivec2[25](
    to_int2( 0, 0 ),
    to_int2( 1, 0 ),to_int2( 0, -1 ),to_int2( -1, 0 ),to_int2( 0, 1 ),
    to_int2( 1, -1 ), to_int2( -1, -1 ), to_int2( -1, 1 ), to_int2( 1, 1 ),
    to_int2( 2, 0 ), to_int2( 0, -2 ), to_int2( -2, 0 ), to_int2( 0, 2 ),
    to_int2( 1, -2 ), to_int2( -1, -2 ), to_int2( -1, 2 ), to_int2( 1, 2 ),
    to_int2( 2, -1 ), to_int2( -2, -1 ), to_int2( -2, 1 ), to_int2( 2, 1 ),
    to_int2( 2, -2 ), to_int2( -2, -2 ), to_int2( -2, 2 ), to_int2( 2, 2 )
);

const float w[9] = float[9](
    0.0f, 1.0f / 21.0f, 4.0f / 45.0f, 0.0f, 1.0f / 60.0f, 2.0f / 315.0f, 0.0f, 0.0f, 1.0f / 5040.
);

__KERNEL__ void Lbm2DFluidJipi981Fuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel1, sampler2D iChannel2)
{

    int2 idx = to_int2(fragCoord*0.5f);
    int2 size = to_int2(iResolution*0.5f);
    
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
    float2 v1 = to_float2(1.0f / rho * (f1 - f2 + f5 - f6 - f7 + f8), 1.0f / rho * (f3 - f4 + f5 - f6 + f7 - f8));

    // calculate external force (surface tension)
    float2 F = to_float2(0, 0);
    for(int i = 1; i < 25; i++) {
        int2 offset = off[i];
        int2 idx2 = idx + offset;
        float psi_self = PSI(iChannel1, idx2);
        float psi_wall = is_wall(idx2, size) ? 1.0f : 0.0f;
        F += (G_self * psi_self + G_wall * psi_wall) * to_float2(offset) * w[offset.x * offset.x + offset.y * offset.y];
    }
    
    float2 a = F / rho + to_float2(0, -g); //gravity
    float2 v2 = v1 + a;
    
    // speed limit
    float max_speed = 0.57735027f;
    if(length(v1) > max_speed-1e-2) {
        v1 = normalize(v1) * (max_speed-1e-2);
    }
    if(length(v2) > max_speed) {
        v2 = normalize(v2) * max_speed;
    }

    // f_new_i = (1-k)*f_i + (k-1)*f_eq_i + f_eq_new_i
    // f_eq_new_i = f_eq(i, v_new)
    float sq_term = - 1.5f * (v1.x * v1.x + v1.y * v1.y);
    float f0eq = 4.0f / 9.0f * rho * (1.0f + sq_term);
    float f1eq = 1.0f / 9.0f * rho * (1.0f + 3.0f * v1.x + 4.5f * v1.x * v1.x + sq_term);
    float f2eq = 1.0f / 9.0f * rho * (1.0f - 3.0f * v1.x + 4.5f * v1.x * v1.x + sq_term);
    float f3eq = 1.0f / 9.0f * rho * (1.0f + 3.0f * v1.y + 4.5f * v1.y * v1.y + sq_term);
    float f4eq = 1.0f / 9.0f * rho * (1.0f - 3.0f * v1.y + 4.5f * v1.y * v1.y + sq_term);
    float f5eq = 1.0f / 36.0f * rho * (1.0f + 3.0f * (v1.x + v1.y) + 4.5f * (v1.x + v1.y) * (v1.x + v1.y) + sq_term);
    float f6eq = 1.0f / 36.0f * rho * (1.0f - 3.0f * (v1.x + v1.y) + 4.5f * (v1.x + v1.y) * (v1.x + v1.y) + sq_term);
    float f7eq = 1.0f / 36.0f * rho * (1.0f + 3.0f * (- v1.x + v1.y) + 4.5f * (- v1.x + v1.y) * (- v1.x + v1.y) + sq_term);
    float f8eq = 1.0f / 36.0f * rho * (1.0f - 3.0f * (- v1.x + v1.y) + 4.5f * (- v1.x + v1.y) * (- v1.x + v1.y) + sq_term);

    float sq_term2 = - 1.5f * (v2.x * v2.x + v2.y * v2.y);
    float f0eq2 = 4.0f / 9.0f * rho * (1.0f + sq_term2);
    float f1eq2 = 1.0f / 9.0f * rho * (1.0f + 3.0f * v2.x + 4.5f * v2.x * v2.x + sq_term2);
    float f2eq2 = 1.0f / 9.0f * rho * (1.0f - 3.0f * v2.x + 4.5f * v2.x * v2.x + sq_term2);
    float f3eq2 = 1.0f / 9.0f * rho * (1.0f + 3.0f * v2.y + 4.5f * v2.y * v2.y + sq_term2);
    float f4eq2 = 1.0f / 9.0f * rho * (1.0f - 3.0f * v2.y + 4.5f * v2.y * v2.y + sq_term2);
    float f5eq2 = 1.0f / 36.0f * rho * (1.0f + 3.0f * (v2.x + v2.y) + 4.5f * (v2.x + v2.y) * (v2.x + v2.y) + sq_term2);
    float f6eq2 = 1.0f / 36.0f * rho * (1.0f - 3.0f * (v2.x + v2.y) + 4.5f * (v2.x + v2.y) * (v2.x + v2.y) + sq_term2);
    float f7eq2 = 1.0f / 36.0f * rho * (1.0f + 3.0f * (- v2.x + v2.y) + 4.5f * (- v2.x + v2.y) * (- v2.x + v2.y) + sq_term2);
    float f8eq2 = 1.0f / 36.0f * rho * (1.0f - 3.0f * (- v2.x + v2.y) + 4.5f * (- v2.x + v2.y) * (- v2.x + v2.y) + sq_term2);

    f0 = (1.0f - k) * f0 + (k - 1.0f) * f0eq + f0eq2;
    f1 = (1.0f - k) * f1 + (k - 1.0f) * f1eq + f1eq2;
    f2 = (1.0f - k) * f2 + (k - 1.0f) * f2eq + f2eq2;
    f3 = (1.0f - k) * f3 + (k - 1.0f) * f3eq + f3eq2;
    f4 = (1.0f - k) * f4 + (k - 1.0f) * f4eq + f4eq2;
    f5 = (1.0f - k) * f5 + (k - 1.0f) * f5eq + f5eq2;
    f6 = (1.0f - k) * f6 + (k - 1.0f) * f6eq + f6eq2;
    f7 = (1.0f - k) * f7 + (k - 1.0f) * f7eq + f7eq2;
    f8 = (1.0f - k) * f8 + (k - 1.0f) * f8eq + f8eq2;
    
    // store f0~8 and rho,Fx,Fy from @ndel https://www.shadertoy.com/view/4dK3zG
    int itx = int(fragCoord.x) - 2*idx.x;
    int ity = int(fragCoord.y) - 2*idx.y;
    if(itx==0&&ity==0)//stores f0,f1,f2
        fragColor = to_float4(f0,f1,f2,1.0f);
    else if(itx==1&&ity==0)//stores f3,f4,f5
        fragColor = to_float4(f3,f4,f5,1.0f);
    else if(itx==0&&ity==1)//stores f6,f7,f8
        fragColor = to_float4(f6,f7,f8,1.0f);
    else //stores
        fragColor = to_float4_aw(rho,length(F),length((v1+v2)*0.5f),1.0f);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer B                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer B 'Previsualization: Buffer A' to iChannel0
// Connect Buffer B 'Previsualization: Buffer B' to iChannel1
// Connect Buffer B 'Previsualization: Buffer C' to iChannel2


// streaming step

__DEVICE__ bool is_wall(in int2 idx, in int2 size){
    return (size.x < idx.x + 1) || (size.y < idx.y + 1) || (idx.x < 0) || (idx.y < 0) || (Wall(iChannel2, idx) > 0.1f);
}

__KERNEL__ void Lbm2DFluidJipi981Fuse__Buffer_B(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, sampler2D iChannel0, sampler2D iChannel2)
{


    int2 idx = to_int2(fragCoord*0.5f);
    int2 size = to_int2(iResolution*0.5f);

    float f0, f1, f2, f3, f4, f5, f6, f7, f8;

    if(!is_wall(idx,size))
    {
        int2 idx2;
        int2 center = to_int2(size.x/2,size.y/3*2);
        if( iFrame%5==1 && (f0 < 0.1f) && length(to_float2(center-idx))<6.0f) // water source
        {
            f4 = 1.0f; //↓
            //f1 = 1.0f; //→
        }
        else // streaming and handling boundary condition 
        {
            f0 = f0(iChannel0, idx);
            idx2 = idx + to_int2(-1, 0);
            f1 = is_wall(idx2, size) ? f2(iChannel0, idx) : f1(iChannel0, idx2);
            idx2 = idx + to_int2(1, 0);
            f2 = is_wall(idx2, size) ? f1(iChannel0, idx) : f2(iChannel0, idx2);
            idx2 = idx + to_int2(0, -1);
            f3 = is_wall(idx2, size) ? f4(iChannel0, idx) : f3(iChannel0, idx2);
            idx2 = idx + to_int2(0, 1);
            f4 = is_wall(idx2, size) ? f3(iChannel0, idx) : f4(iChannel0, idx2);
            idx2 = idx + to_int2(-1, -1);
            f5 = is_wall(idx2, size) ? f6(iChannel0, idx) : f5(iChannel0, idx2);
            idx2 = idx + to_int2(1, 1);
            f6 = is_wall(idx2, size) ? f5(iChannel0, idx) : f6(iChannel0, idx2);
            idx2 = idx + to_int2(1, -1);
            f7 = is_wall(idx2, size) ? f8(iChannel0, idx) : f7(iChannel0, idx2);
            idx2 = idx + to_int2(-1, 1);
            f8 = is_wall(idx2, size) ? f7(iChannel0, idx) : f8(iChannel0, idx2);
        }
    }
    
    const float rho0 = 1.0f;
    float rho = f0+f1+f2+f3+f4+f5+f6+f7+f8+1e-30;
    float psi = rho0 * (1.f - _expf(-_fabs(rho) / rho0));

    // store f0~8, rho, psi from @ndel https://www.shadertoy.com/view/4dK3zG
    int itx = int(fragCoord.x) - 2*idx.x;
    int ity = int(fragCoord.y) - 2*idx.y;
    if(itx==0&&ity==0)//stores f0,f1,f2
        fragColor = to_float4(f0,f1,f2,1.0f);
    else if(itx==1&&ity==0)//stores f3,f4,f5
        fragColor = to_float4(f3,f4,f5,1.0f);
    else if(itx==0&&ity==1)//stores f6,f7,f8
        fragColor = to_float4(f6,f7,f8,1.0f);
    else //stores rho psi
        fragColor = to_float4(rho,0,0,psi);


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Buffer C                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer C 'Preset: Keyboard' to iChannel3
// Connect Buffer C 'Previsualization: Buffer A' to iChannel0
// Connect Buffer C 'Previsualization: Buffer B' to iChannel1
// Connect Buffer C 'Previsualization: Buffer C' to iChannel2


// draw wall

#define IsInBox(box, idx) (idx.x > box.x && idx.x < box.y && idx.y > box.z && idx.y < box.w)
#define IsInCircle(o,r,idx) (length(o-to_float2(idx))<r)
#define KEY_SPACE 32
#define KEY_E 69

__KERNEL__ void Lbm2DFluidJipi981Fuse__Buffer_C(float4 fragColor, float2 fragCoord, float2 iResolution, float4 iMouse, int iFrame, sampler2D iChannel2, sampler2D iChannel3)
{

    int2 idx = to_int2(fragCoord*0.5f);
    int2 size = to_int2(iResolution*0.5f);
    int4 box1 = to_int4(size.x/2-10,size.x/3*2,size.y/3-20,size.y/3-10);
    int4 box2 = box1-size.y/8;
    int4 box3 = box2+to_int4(size.y/4,size.y/4,-15,-15);
    int4 box4 = to_int4(size.x/3-20,size.x/3-10,5,60);
    int4 box5 = box4 - to_int4(12,12,0,0);
    int4 box6 = box4 + to_int4(200,200,-20,20);
    int4 box7 = box4 - to_int4(27,27,10,20);
    int4 box8 = box4 + to_int4(11,11,0,10);
    // init
    if(iFrame==0)
    {
        if(IsInBox(box1,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }
        else if(IsInBox(box2,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }
        else if(IsInBox(box3,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }        
        else if(IsInBox(box4,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }        
        else if(IsInBox(box5,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }      
        else if(IsInBox(box6,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        } 
        else if(IsInBox(box7,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }
        else if(IsInBox(box8,idx)) {
           fragColor = to_float4(1.0f,1.0f,1.0f,1.0f);
        }
    }
    else
    {
        float wall = Wall(iChannel2,idx);
        //...
        // mouse and keyboard
        float pressSpace = texelFetch( iChannel3, to_int2(KEY_SPACE,0.0f), 0 ).x;
        float pressE = texelFetch( iChannel3, to_int2(KEY_E,0.0f), 0 ).x;
        if(iMouse.z>0.5f && IsInCircle(iMouse.xy/2.0f,5.0f,idx))
        {
            wall = (pressSpace>0.5f || pressE > 0.5f) ? 0.0f : 1.0f;
        }
        
        fragColor = to_float4(wall,wall,wall,1.0f);
    }


  SetFragmentShaderComputedColor(fragColor);
}
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Preset: Keyboard' to iChannel3
// Connect Image 'Previsualization: Buffer A' to iChannel0
// Connect Image 'Previsualization: Buffer B' to iChannel1
// Connect Image 'Previsualization: Buffer C' to iChannel2


// visualization

#define KEY_F 70
#define KEY_V 86

__KERNEL__ void Lbm2DFluidJipi981Fuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2, sampler2D iChannel3)
{

    int2 idx = to_int2(fragCoord * 0.5f);
    //float rho = f0(iChannel0, idx);
    float rho = RHO(iChannel1, idx);
    float F_len = rho>0.001? F_Len(iChannel0, idx)*_fminf(rho*5.0f,1.0f)*5.:0.;
    float v_len = rho>0.001? V_Len(iChannel0, idx)*_fminf(rho*10.0f,1.0f)*2.:0.;
    float wall = Wall(iChannel2, idx);
    
    float flag_v = texelFetch( iChannel3, to_int2(KEY_V,2), 0 ).x;
    float flag_f = texelFetch( iChannel3, to_int2(KEY_F,2), 0 ).x>0.5?0.:1.;

    float4 color = to_float4(0,0,rho,1.0f);            // display fluid
    color += to_float4(0,F_len*flag_f,0,0);        // display force
    color += to_float4(wall,wall,wall,1.0f)*0.3f;     // display wall
    color += to_float4(v_len*flag_v,0,0,0);        // display velocity 

    int2 size = to_int2(iResolution*0.5f);
    int2 center = to_int2(size.x/2,size.y/3*2);
    float4 tube = to_float4_s(0.25f) *(step(length(to_float2(center-idx)),9.5f) - step(length(to_float2(center-idx)),5.6f));
    
    color += color.b<0.5?tube:to_float4_s(0.0f);
    
    fragColor=color;


  SetFragmentShaderComputedColor(fragColor);
}