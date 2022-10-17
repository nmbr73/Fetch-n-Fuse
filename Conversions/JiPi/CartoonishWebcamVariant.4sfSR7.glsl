

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// variant from okeli4408: https://www.shadertoy.com/view/XdXXzM#

bool keyToggle(int ascii) {
	return (texture(iChannel2,vec2((.5+float(ascii))/256.,0.75)).x > 0.);
}

float showFlag(vec2 p, vec2 uv, float v) {
	float d = length(2.*(uv-p)*iResolution.xy/iResolution.y);
	return 	1.-step(.06*v,d) + smoothstep(0.005,0.,abs(d-.06));
}

float showFlag(vec2 p, vec2 uv, bool flag) {
	return showFlag(p, uv, (flag) ? 1.: 0.);
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 uv = fragCoord.xy / iResolution.xy;

	// --- tunings 
	
	vec2 mouse = iMouse.xy / iResolution.xy;
	
	bool BG_BW, BG_COL, FG_COL,FLIP,GAMMA, VID;
	
	if (iMouse.z<=0.) { // no mouse: autodemo
		float t = iTime/3.;
		float t0 = mod(t,3.); int i = int(t0);
		
		// (!BG_BW) = (i==0)
		BG_BW  = (i==1);
		BG_COL = (i==2);
		FLIP = (mod(t/3.,2.)>1.);
		GAMMA = (mod(t/6.,2.)>1.);
		VID = (iResolution.y<200.) || (iChannelResolution[0].y<=0.) || (mod(t/12.,2.)>1.);
		FG_COL=false; // already there for BG + high gamma
		
		mouse = .5*( 1.+ .5*vec2(cos(3.*t)+cos(t), sin(3.3*t)+cos(.7*t) ) );
	}	
	else 
	{
		BG_BW  = keyToggle(66);
		BG_COL = keyToggle(67);
		FG_COL = keyToggle(70);
		FLIP   = keyToggle(32);
		GAMMA  = keyToggle(71);
		VID    = keyToggle(86);
	}
	
	float panel = showFlag(vec2(.25,.05),uv, bool(BG_BW))
				+ showFlag(vec2(.35,.05),uv, bool(BG_COL))
				+ showFlag(vec2(.45,.05),uv, bool(FG_COL))
				+ showFlag(vec2(.55,.05),uv, bool(GAMMA))
				+ showFlag(vec2(.65,.05),uv, bool(FLIP))
				+ showFlag(mouse,uv, true);
	
    // --- display 
	
	vec3 col = (VID) ? texture(iChannel1, vec2(1.-uv.x,uv.y)).rgb 
					 : texture(iChannel0, vec2(1.-uv.x,uv.y)).rgb;
	
	// edge = norm of luminance derivative.
	float lum = col.x + col.y + col.z;
	vec2 deriv = vec2(dFdx(lum), dFdy(lum));
	float edge = sqrt(dot(deriv,deriv));
	// improve:
	edge = smoothstep(0.,mouse.x,edge);
	if (GAMMA) edge = pow(edge, exp(2.*2.*(mouse.y-.7))); // gamma contrasting
	
	if (FLIP) edge = 1.-edge;
	
	vec3 bg = vec3 ( (BG_BW) ? 1.: 0.);  // black vs white background
	if (BG_COL) bg = 1.-col; 			 // background = reverse video
	if (!FG_COL) col = 1.-bg;			 // forground = rev of background
	
	// key transform: ink + paper
	col = mix(col,bg,edge); 

	if (!GAMMA) col = pow(col, vec3(exp(3.*mouse.y))); // gamma contrasting
	
	col.b = (col.b+.2*(col.r+col.g) < panel) ? panel:  col.b-panel;
	fragColor = vec4(col,1.0);
}