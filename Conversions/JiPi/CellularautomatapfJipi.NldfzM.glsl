

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    fragColor = texture(iChannel0, fragCoord.xy/iResolution.xy);
}	
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//sandbox -> shadertoy wrapper
#define mouse			iMouse.xy/iResolution.xy
#define renderbuffer 	iChannel0
#define resolution		iResolution.xy

//automata
#define WAKE			.975
#define DECAY			.00125
#define FIELD			.9
#define CHARGE			2.

#define FREQUENCY 		25./32.
#define MAX_FLOAT		pow(2.,  8.)
#define MIN_FLOAT		pow(2., -8.)
#define WRAP 			true

vec2 neighbor_offset(float i);																	//8 offsets corrosponding to the ring of moore neighborhood positions ((0., -1.), (1., -1.), (1., 0.), (1., 1.)... etc)
vec4 reset_button(inout vec4 cell);																//flashing button in the bottom left, mouse over it to reset
vec4 clear_at_screen_edge(inout vec4 cell, in vec2 coordinates);								//clears the cells at the boundary of the screen to prevent wrapping
float mix_angle( float angle, float target, float rate );										//mixes two angles - i think its bugged
vec4 add_new_cell(inout vec4 cell, in vec2 position, in vec2 coordinates, in bool polarity);	//adds a new cell at the "new cell position"
float bound(float angle);																		//clamps particle angle above 0.
float witch(float x);																			//probability distribution curve

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec4 cell		= vec4(0.);

	vec4 prior		= texture(renderbuffer, gl_FragCoord.xy/resolution);
	
	vec2 field		= gl_FragCoord.xy * FREQUENCY;

	vec4 sum			= vec4(0.);
	vec4 neighbor[8];
	for (int i = 0; i < 8; i++)
    {
		vec2 neighbor_uv 	= gl_FragCoord.xy - neighbor_offset(float(i));
		neighbor_uv		= fract(neighbor_uv/resolution);

		neighbor[i] 		= texture(renderbuffer, neighbor_uv);
		sum			+= neighbor[i];
		
		//positive alpha/red particles
		float positive_angle	= neighbor[i].w;
		if (positive_angle != 0.)
		{   
			float sequence		= abs(fract(positive_angle * 2.) - .5) < .25 ? field.x : field.y;
			sequence 		= fract(sequence) * .125;
			sequence			= fract(positive_angle + sequence);
		
			if(floor(sequence * 8.) == float(i)) 
			{
				cell.w 		= positive_angle;
				cell.x 		= neighbor[i].x;
			}	
		}
		
		//negative green/blue particles
		float negative_angle	= neighbor[i].y;
		if (negative_angle != 0.)
		{   
			float sequence		= abs(fract(negative_angle * 2.) - .5) < .25 ? field.x : field.y;
			sequence 		= fract(sequence) * .125;
			sequence			= fract(negative_angle + sequence);
		
			if(floor(sequence * 8.) == float(i)) 
			{
				cell.y 		= negative_angle;
				cell.z 		= neighbor[i].z;
			}	
		}
	}
	
	sum						= sum * .125;
	
	vec4 d_x 				= (neighbor[5] + neighbor[6] + neighbor[7]) - (neighbor[1] + neighbor[2] + neighbor[3]); //left right
	vec4 d_y 				= (neighbor[3] + neighbor[4] + neighbor[6]) - (neighbor[1] + neighbor[0] + neighbor[7]); //top bottom
	
	float positive_normal 	= fract(atan(d_x.x, d_y.x)*.15915494);
	float negative_normal 	= fract(atan(d_x.z, d_y.z)*.15915494);
	
	vec2 f					= normalize(sum.xz);
	float positive_field 	= abs(f.x-cell.x);
	float negative_field 	= abs(f.y-cell.z);
	
	float positive_angle 	= cell.w;
	float negative_angle 	= cell.y;

	float emission			= CHARGE;
	float slope				= FIELD*witch(abs(1.-prior.x-prior.z));
	float decay				= DECAY;	
	
	bool positive_charge	= cell.w > 0.;
	bool negative_charge	= cell.y > 0.;
	

	//fields that curve particle paths - red is positive, blue is negative
	
	//existing particles emit - empty space average charge in local neighborhood
	positive_field 		= positive_charge ? emission : mix(sum.x, mix(sum.x, prior.x, WAKE), -slope)-decay; 
	negative_field 		= negative_charge ? emission : mix(sum.z, mix(sum.z, prior.z, WAKE), -slope)-decay; 
	
	positive_angle 		= positive_charge && positive_field > 0. ? mix_angle(positive_angle, fract(1.-positive_normal), .00625) : cell.w;
	negative_angle 		= negative_charge && negative_field > 0. ? mix_angle(negative_angle, fract(1.-negative_normal), .00625) : cell.y; 
	
    positive_angle 		= positive_charge && negative_field > 0. ? mix_angle(positive_angle, negative_normal, slope) : cell.w;
	negative_angle 		= negative_charge && positive_field > 0. ? mix_angle(negative_angle, positive_normal, slope) : cell.y; 
	
    
    
	positive_angle		= positive_charge ? max(fract(positive_angle), MIN_FLOAT) : 0.;
	negative_angle		= negative_charge ? max(fract(negative_angle), MIN_FLOAT) : 0.;
	
	
	cell				= vec4(positive_field, negative_angle, negative_field, positive_angle);

	cell				= add_new_cell(cell, floor(resolution * vec2(mouse)), fragCoord, false);
	cell				= add_new_cell(cell, floor(resolution * vec2(1.-mouse)), fragCoord, true);
	cell				= add_new_cell(cell, floor(resolution * .75), fragCoord, false);
	cell				= add_new_cell(cell, floor(resolution * .25), fragCoord, true);
    
	cell				= WRAP ? cell : clear_at_screen_edge(cell, fragCoord);
	
	fragColor	    	= clamp(cell, 0., 1.);
}//sphinx


//returns the sequence of offsets for the moore neighborhood
vec2 neighbor_offset(float i)
{
	float c = abs(i-2.);
	float s = abs(i-4.);
	return vec2(c > 1. ? c > 2. ? 1. : .0 : -1., s > 1. ? s > 2. ? -1. : .0 : 1.);
}



float mix_angle( float angle, float target, float rate )
{    

   	angle = abs( angle - target - 1. ) < abs( angle - target ) ? angle - 1. : angle;
   	angle = abs( angle - target + 1. ) < abs( angle - target ) ? angle + 1. : angle;
	angle = fract(mix(angle, target, rate));   	
   	return bound(angle);
}

float bound(float angle)
{
	return max(angle,.00392156);
}

float witch(float x)
{
	x	= 1.-x;
	float w = .0625/(x*x+.0625);
	return 	w*w;
}

//adds a new cell at the position every frame
vec4 add_new_cell(inout vec4 cell, in vec2 position, in vec2 coordinates, in bool polarity)
{
	vec2 uv			= coordinates.xy/resolution.xy; 
	bool is_pixel	   	= abs(length(floor(coordinates.xy-position))) < 1.;
	float prior_angle	= polarity ? texture(renderbuffer, uv).y : texture(renderbuffer, uv).w; 
	float initial_angle	= max(fract(prior_angle + MIN_FLOAT * 55.), MIN_FLOAT);
	vec2 angle		= polarity ? initial_angle * vec2(1., 0.) : initial_angle * vec2(0., 1.);
    	
	cell 			= is_pixel ? vec4(0., angle.x, 0., angle.y) : cell;	
	return cell;
}


//clears the cell if it reaches the screen border
vec4 clear_at_screen_edge(inout vec4 cell, in vec2 coordinates)
{
	return cell * float(coordinates.x > 1. && coordinates.y > 1. && coordinates.x < resolution.x-1. && coordinates.y < resolution.y-1.);
}