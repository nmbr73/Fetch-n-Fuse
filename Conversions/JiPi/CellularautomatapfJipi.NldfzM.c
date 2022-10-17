
// ----------------------------------------------------------------------------------
// - Buffer A                                                                       -
// ----------------------------------------------------------------------------------
// Connect Buffer A 'Previsualization: Buffer A' to iChannel0

#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

//sandbox -> shadertoy wrapper
#define mouse          swi2(iMouse,x,y)/iResolution
#define renderbuffer   iChannel0
#define resolution     iResolution



                                  
//clamps particle angle above 0.0f
__DEVICE__ float bound(float angle)
{
  return _fmaxf(angle,0.00392156f);
}

//returns the sequence of offsets for the moore neighborhood
//8 offsets corrosponding to the ring of moore neighborhood positions ((0.0f, -1.0f), (1.0f, -1.0f), (1.0f, 0.0f), (1.0f, 1.0f)... etc)
__DEVICE__ float2 neighbor_offset(float i)
{
  float c = _fabs(i-2.0f);
  float s = _fabs(i-4.0f);
  return to_float2(c > 1.0f ? c > 2.0f ? 1.0f : 0.0f : -1.0f, s > 1.0f ? s > 2.0f ? -1.0f : 0.0f : 1.0f);
}

//mixes two angles - i think its bugged
__DEVICE__ float mix_angle( float angle, float target, float rate )
{    
  angle = _fabs( angle - target - 1.0f ) < _fabs( angle - target ) ? angle - 1.0f : angle;
  angle = _fabs( angle - target + 1.0f ) < _fabs( angle - target ) ? angle + 1.0f : angle;
  angle = fract(_mix(angle, target, rate));     
  return bound(angle);
}

//probability distribution curve
__DEVICE__ float witch(float x)
{
  x  = 1.0f-x;
  float w = 0.0625f/(x*x+0.0625f);
  return   w*w;
}

//adds a new cell at the position every frame
__DEVICE__ float4 add_new_cell(inout float4 *cell, in float2 position, in float2 coordinates, in bool polarity, float2 iResolution, __TEXTURE2D__ renderbuffer, float MIN_FLOAT)
{
  float2 uv       = coordinates/resolution; 
  bool is_pixel   = _fabs(length(_floor(swi2(coordinates,x,y)-position))) < 1.0f;
  float prior_angle  = polarity ? texture(renderbuffer, uv).y : _tex2DVecN(renderbuffer,uv.x,uv.y,15).w; 
  float initial_angle  = _fmaxf(fract(prior_angle + MIN_FLOAT * 55.0f), MIN_FLOAT);
  float2 angle    = polarity ? initial_angle * to_float2(1.0f, 0.0f) : initial_angle * to_float2(0.0f, 1.0f);
      
  *cell       = is_pixel ? to_float4(0.0f, angle.x, 0.0f, angle.y) : *cell;  
  return *cell;
}


//clears the cell if it reaches the screen border
__DEVICE__ float4 clear_at_screen_edge(inout float4 *cell, in float2 coordinates, float2 iResolution)
{
  return *cell * (float)(coordinates.x > 1.0f && coordinates.y > 1.0f && coordinates.x < resolution.x-1.0f && coordinates.y < resolution.y-1.0f);
}


__KERNEL__ void CellularautomatapfJipiFuse__Buffer_A(float4 fragColor, float2 fragCoord, float2 iResolution, int iFrame, float4 iMouse, sampler2D iChannel0)
{
  
  
  //automata
//#define WAKE       0.975f
//#define DECAY      0.00125f
//#define FIELD      0.9f
//#define CHARGE     2.0f

//#define FREQUENCY     25.0f/32.0f
//#define MAX_FLOAT    _powf(2.0f,  8.0f)
//#define MIN_FLOAT    _powf(2.0f, -8.0f)
//#define WRAP         true
  
  
  CONNECT_SLIDER0(WAKE, -1.0f, 3.0f, 0.975f);
  CONNECT_SLIDER1(DECAY, -1.0f, 0.1f, 0.00125f);
  CONNECT_SLIDER2(FIELD, -1.0f, 3.0f, 0.9f);
  CONNECT_SLIDER3(CHARGE, -1.0f, 5.0f, 2.0f);
  CONNECT_SLIDER4(FREQUENCY, -1.0f, 1.0f, 0.675f);
  CONNECT_SLIDER5(Max_float, -1.0f, 3.0f, 0.0);
  CONNECT_SLIDER6(Min_float, -1.0f, 3.0f, 0.0);
  
  
  CONNECT_CHECKBOX0(Reset, 0);
  CONNECT_CHECKBOX1(WRAP, 1);
  
  float MAX_FLOAT = _powf(2.0f,  8.0f+Max_float);
  float MIN_FLOAT = _powf(2.0f,  -8.0f+Min_float);
  

  fragCoord+=0.5f;

  float4 cell     = to_float4_s(0.0f);

  float4 prior    = texture(renderbuffer, fragCoord/resolution);
  
  float2 field    = fragCoord * FREQUENCY;

  float4 sum      = to_float4_s(0.0f);
  float4 neighbor[8];
  for (int i = 0; i < 8; i++)
    {
    float2 neighbor_uv   = fragCoord - neighbor_offset((float)(i));
    neighbor_uv     = fract_f2(neighbor_uv/resolution);

    neighbor[i]     = _tex2DVecN(renderbuffer,neighbor_uv.x,neighbor_uv.y,15);
    sum            += neighbor[i];
    
    //positive alpha/red particles
    float positive_angle  = neighbor[i].w;
    if (positive_angle != 0.0f)
    {   
      float sequence = _fabs(fract(positive_angle * 2.0f) - 0.5f) < 0.25f ? field.x : field.y;
      sequence       = fract(sequence) * 0.125f;
      sequence       = fract(positive_angle + sequence);
    
      if(_floor(sequence * 8.0f) == (float)(i)) 
      {
        cell.w     = positive_angle;
        cell.x     = neighbor[i].x;
      }  
    }
    
    //negative green/blue particles
    float negative_angle  = neighbor[i].y;
    if (negative_angle != 0.0f)
    {   
      float sequence = _fabs(fract(negative_angle * 2.0f) - 0.5f) < 0.25f ? field.x : field.y;
      sequence       = fract(sequence) * 0.125f;
      sequence       = fract(negative_angle + sequence);
    
      if(_floor(sequence * 8.0f) == (float)(i)) 
      {
        cell.y     = negative_angle;
        cell.z     = neighbor[i].z;
      }  
    }
  }
  
  sum                = sum * 0.125f;
  
  float4 d_x         = (neighbor[5] + neighbor[6] + neighbor[7]) - (neighbor[1] + neighbor[2] + neighbor[3]); //left right
  float4 d_y         = (neighbor[3] + neighbor[4] + neighbor[6]) - (neighbor[1] + neighbor[0] + neighbor[7]); //top bottom
  
  float positive_normal   = fract(_atan2f(d_x.x, d_y.x)*0.15915494f);
  float negative_normal   = fract(_atan2f(d_x.z, d_y.z)*0.15915494f);
  
  float2 f               = normalize(swi2(sum,x,z));
  float positive_field   = _fabs(f.x-cell.x);
  float negative_field   = _fabs(f.y-cell.z);
  
  float positive_angle   = cell.w;
  float negative_angle   = cell.y;

  float emission     = CHARGE;
  float slope        = FIELD*witch(_fabs(1.0f-prior.x-prior.z));
  float decay        = DECAY;  
  
  bool positive_charge  = cell.w > 0.0f;
  bool negative_charge  = cell.y > 0.0f;
  

  //fields that curve particle paths - red is positive, blue is negative
  
  //existing particles emit - empty space average charge in local neighborhood
  positive_field     = positive_charge ? emission : _mix(sum.x, _mix(sum.x, prior.x, WAKE), -slope)-decay; 
  negative_field     = negative_charge ? emission : _mix(sum.z, _mix(sum.z, prior.z, WAKE), -slope)-decay; 
  
  positive_angle     = positive_charge && positive_field > 0.0f ? mix_angle(positive_angle, fract(1.0f-positive_normal), 0.00625f) : cell.w;
  negative_angle     = negative_charge && negative_field > 0.0f ? mix_angle(negative_angle, fract(1.0f-negative_normal), 0.00625f) : cell.y; 
  
  positive_angle     = positive_charge && negative_field > 0.0f ? mix_angle(positive_angle, negative_normal, slope) : cell.w;
  negative_angle     = negative_charge && positive_field > 0.0f ? mix_angle(negative_angle, positive_normal, slope) : cell.y; 
  
    
    
  positive_angle    = positive_charge ? _fmaxf(fract(positive_angle), MIN_FLOAT) : 0.0f;
  negative_angle    = negative_charge ? _fmaxf(fract(negative_angle), MIN_FLOAT) : 0.0f;
  
  
  cell        = to_float4(positive_field, negative_angle, negative_field, positive_angle);

  cell        = add_new_cell(&cell, _floor(resolution * (mouse)), fragCoord, false, iResolution, iChannel0, MIN_FLOAT);
  cell        = add_new_cell(&cell, _floor(resolution * (1.0f-mouse)), fragCoord, true, iResolution, iChannel0, MIN_FLOAT);
  cell        = add_new_cell(&cell, _floor(resolution * 0.75f), fragCoord, false, iResolution, iChannel0, MIN_FLOAT);
  cell        = add_new_cell(&cell, _floor(resolution * 0.25f), fragCoord, true, iResolution, iChannel0, MIN_FLOAT);
    
  cell        = WRAP ? cell : clear_at_screen_edge(&cell, fragCoord, iResolution);
  
  fragColor   = clamp(cell, 0.0f, 1.0f);
  
  if ( iFrame == 0 || Reset ) 
    fragColor = to_float4_s(0.0f);

  SetFragmentShaderComputedColor(fragColor);  
  
}//sphinx



// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Previsualization: Buffer A' to iChannel0


__KERNEL__ void CellularautomatapfJipiFuse(float4 fragColor, float2 fragCoord, float2 iResolution, sampler2D iChannel0)
{
  
  fragCoord+=0.5f;
  
  CONNECT_CHECKBOX2(Original, 0);
  
  CONNECT_COLOR0(Color1, 1.0f, 0.0f, 0.0f, 1.0f);
  CONNECT_COLOR1(Color2, 0.0f, 0.0f, 1.0f, 1.0f);
  CONNECT_COLOR2(Color3, 0.0f, 1.0f, 0.0f, 1.0f);
  CONNECT_SLIDER7(Brightness, -1.0f, 5.0f, 4.0f);

  float4 tex = texture(iChannel0, fragCoord/iResolution);
  
  //if(tex.x > 0.0f) fragColor = fragColor.x * Color1;
  //if(tex.z > 0.0f) fragColor = fragColor.z * Color2;
  
  fragColor = _mix(_mix(tex.x * Color1, tex.z * Color2, 0.5f),tex.y * Color3, 0.5) * Brightness;
   

  if (Original) fragColor = tex;

  fragColor.w = Color1.w;

  SetFragmentShaderComputedColor(fragColor);
}