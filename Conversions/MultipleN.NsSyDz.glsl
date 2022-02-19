

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
//	----    ----    ----    ----    ----    ----    ----    ----
//  Shader developed by Slackermanz
//
//  Info/Code:
//  ﻿ - Website: https://slackermanz.com
//  ﻿ - Github: https://github.com/Slackermanz
//  ﻿ - Shadertoy: https://www.shadertoy.com/user/SlackermanzCA
//  ﻿ - Discord: https://discord.gg/hqRzg74kKT
//  
//  Socials:
//  ﻿ - Discord DM: Slackermanz#3405
//  ﻿ - Reddit DM: https://old.reddit.com/user/slackermanz
//  ﻿ - Twitter: https://twitter.com/slackermanz
//  ﻿ - YouTube: https://www.youtube.com/c/slackermanz
//  ﻿ - Older YT: https://www.youtube.com/channel/UCZD4RoffXIDoEARW5aGkEbg
//  ﻿ - TikTok: https://www.tiktok.com/@slackermanz
//  
//  Communities:
//  ﻿ - Reddit: https://old.reddit.com/r/cellular_automata
//  ﻿ - Artificial Life: https://discord.gg/7qvBBVca7u
//  ﻿ - Emergence: https://discord.com/invite/J3phjtD
//  ﻿ - ConwayLifeLounge: https://discord.gg/BCuYCEn
//	----    ----    ----    ----    ----    ----    ----    ----

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
    fragColor = texelFetch( iChannel0, ivec2(gl_FragCoord[0], gl_FragCoord[1]), 0); }
// >>> ___ GLSL:[Buffer A] _________________________________________________________________ <<<
//	----    ----    ----    ----    ----    ----    ----    ----
//  Shader developed by Slackermanz
//
//  Info/Code:
//  ﻿ - Website: https://slackermanz.com
//  ﻿ - Github: https://github.com/Slackermanz
//  ﻿ - Shadertoy: https://www.shadertoy.com/user/SlackermanzCA
//  ﻿ - Discord: https://discord.gg/hqRzg74kKT
//  
//  Socials:
//  ﻿ - Discord DM: Slackermanz#3405
//  ﻿ - Reddit DM: https://old.reddit.com/user/slackermanz
//  ﻿ - Twitter: https://twitter.com/slackermanz
//  ﻿ - YouTube: https://www.youtube.com/c/slackermanz
//  ﻿ - Older YT: https://www.youtube.com/channel/UCZD4RoffXIDoEARW5aGkEbg
//  ﻿ - TikTok: https://www.tiktok.com/@slackermanz
//  
//  Communities:
//  ﻿ - Reddit: https://old.reddit.com/r/cellular_automata
//  ﻿ - Artificial Life: https://discord.gg/7qvBBVca7u
//  ﻿ - Emergence: https://discord.com/invite/J3phjtD
//  ﻿ - ConwayLifeLounge: https://discord.gg/BCuYCEn
//	----    ----    ----    ----    ----    ----    ----    ----

#define txdata (iChannel0)
#define PI 3.14159265359
#define LN 2.71828182846

const uint MAX_RADIUS = 12u;

uint[64] ub =  uint[64]
(   4086443839u, 803560891u, 3521573439u, 155586747u, 
    2355529581u, 3804082561u, 1278181521u, 1198599219u, 
    790900093u, 2043079403u, 72510135u, 329989440u, 
    1205735441u, 1165390975u, 1863025477u, 1552315409u, 
    1460749058u, 1961704519u, 1063988442u, 304586942u, 
    1969173229u, 623707175u, 2719649363u, 533620173u, 
    734380903u, 1866742626u, 69847740u, 779938642u, 
    3928012151u, 1597352029u, 1939485308u, 1599391651u, 
    824038858u, 498247516u, 609160531u, 634145519u, 
    2022645937u, 4285315796u, 87513080u, 246410766u, 
    1160189374u, 688725303u, 1266836767u, 670912482u, 
    2162941226u, 1742659144u, 481786434u, 3618106514u, 
    0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 
    0u, 1067687164u, 1122344419u, 0u );

uint u32_upk(uint u32, uint bts, uint off) { return (u32 >> off) & ((1u << bts)-1u); }

float lmap() { return (gl_FragCoord[0] / float(textureSize(txdata,0)[0])); }
float vmap() { return (gl_FragCoord[1] / float(textureSize(txdata,0)[1])); }
float cmap() { return sqrt	( ((gl_FragCoord[0] - float(textureSize(txdata,0)[0])*0.5) / float(textureSize(txdata,0)[0])*0.5)
							* ((gl_FragCoord[0] - float(textureSize(txdata,0)[0])*0.5) / float(textureSize(txdata,0)[0])*0.5)
							+ ((gl_FragCoord[1] - float(textureSize(txdata,0)[1])*0.5) / float(textureSize(txdata,0)[1])*0.5)
							* ((gl_FragCoord[1] - float(textureSize(txdata,0)[1])*0.5) / float(textureSize(txdata,0)[1])*0.5) ); }
float vwm() {
	float 	scale_raw 	= uintBitsToFloat(ub[62]);
	float 	zoom 		= uintBitsToFloat(ub[61]);
	float	scale_new	= scale_raw;
	uint 	mode 		= u32_upk(ub[59], 2u, 0u);
	if( mode == 1u ) { //	Linear Parameter Map
		scale_new = ((lmap() + zoom) * (scale_raw / (1.0 + zoom * 2.0))) * 2.0; }
	if( mode == 2u ) { //	Circular Parameter Map
		scale_new = ((sqrt(cmap()) + zoom) * (scale_raw / (1.0 + zoom * 2.0))) * 2.0; }
	return scale_new; }
    
float  tp(uint n, float s) 			{ return (float(n+1u)/256.0) * ((s*0.5)/128.0); }
float utp(uint v, uint  w, uint o) 	{ return tp(u32_upk(v,w,w*o), vwm()); }
float bsn(uint v, uint  o) 			{ return float(u32_upk(v,1u,o)*2u)-1.0; }
    
vec4  sigm(vec4  x, float w) { return 1.0 / ( 1.0 + exp( (-w*2.0 * x * (PI/2.0)) + w * (PI/2.0) ) ); }
float hmp2(float x, float w) { return 3.0*((x-0.5)*(x-0.5))+0.25; }

vec4  gdv( ivec2 of, sampler2D tx ) {
	of 		= ivec2(gl_FragCoord) + of;
	of[0] 	= (of[0] + textureSize(tx,0)[0]) % (textureSize(tx,0)[0]);
	of[1] 	= (of[1] + textureSize(tx,0)[1]) % (textureSize(tx,0)[1]);
	return 	texelFetch( tx, of, 0); }
    
vec4 nbhd( vec2 r, sampler2D tx ) {
//	Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
	uint	chk = 2147483648u /
			(	( 	uint( r[0]*r[0]*PI + r[0]*PI + PI	)
				- 	uint( r[1]*r[1]*PI + r[1]*PI		) ) * 128u );
	float	psn = (chk >= 65536u) ? 65536.0 : float(chk);
	vec4	a = vec4(0.0,0.0,0.0,0.0);
	float	w = 1.0;	// Weighting, unused
	if(r[0] == 0.0) { return vec4( gdv( ivec2(0,0), tx )*w*psn ); }
	else 			{
		for(float i = 0.0; i <= r[0]; i++) {
			for(float j = 1.0; j <= r[0]; j++) {
				float	d = round(sqrt(i*i+j*j));
						w = 1.0;	//	Per-Neighbor Weighting, unused
				if( d <= r[0] && d > r[1] ) {
					vec4 t0  = gdv( ivec2( i, j), tx ) * w * psn; a += t0 - fract(t0);
					vec4 t1  = gdv( ivec2( j,-i), tx ) * w * psn; a += t1 - fract(t1);
					vec4 t2  = gdv( ivec2(-i,-j), tx ) * w * psn; a += t2 - fract(t2);
					vec4 t3  = gdv( ivec2(-j, i), tx ) * w * psn; a += t3 - fract(t3); } } }
		return a; } }

vec4 totl( vec2 r, sampler2D tx ) {
//	Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
	uint	chk = 2147483648u /
			(	( 	uint( r[0]*r[0]*PI + r[0]*PI + PI	)
				- 	uint( r[1]*r[1]*PI + r[1]*PI		) ) * 128u );
	float	psn = (chk >= 65536u) ? 65536.0 : float(chk);
	vec4	b = vec4(0.0,0.0,0.0,0.0);
	float	w = 1.0;	// Weighting, unused
	if(r[0] == 0.0) { return vec4( psn * w, psn * w, psn * w, psn * w ); }
	else 			{
		for(float i = 0.0; i <= r[0]; i++) {
			for(float j = 1.0; j <= r[0]; j++) {
				float	d = round(sqrt(i*i+j*j));
						w = 1.0;	//	Per-Neighbor Weighting, unused
				if( d <= r[0] && d > r[1] ) { b += psn * w * 4.0; } } }
		return b; } }

vec4 bitring(vec4[MAX_RADIUS] rings_a, vec4[MAX_RADIUS] rings_b, uint bits, uint of) {
	vec4 sum = vec4(0.0,0.0,0.0,0.0);
	vec4 tot = vec4(0.0,0.0,0.0,0.0);
	for(uint i = 0u; i < MAX_RADIUS; i++) {
		if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings_a[i]; tot += rings_b[i]; } }
	return sigm( (sum / tot), LN ); } // TODO
    
vec4 conv(vec2 r, sampler2D tx) {
	vec4 nha = nbhd(r, tx);
	vec4 nhb = totl(r, tx);
	return 	nha / nhb; }
    
//	Used to reseed the surface with lumpy noise
float get_xc(float x, float y, float xmod) {
	float sq = sqrt(mod(x*y+y, xmod)) / sqrt(xmod);
	float xc = mod((x*x)+(y*y), xmod) / xmod;
	return clamp((sq+xc)*0.5, 0.0, 1.0); }
float shuffle(float x, float y, float xmod, float val) {
	val = val * mod( x*y + x, xmod );
	return (val-floor(val)); }
float get_xcn(float x, float y, float xm0, float xm1, float ox, float oy) {
	float  xc = get_xc(x+ox, y+oy, xm0);
	return shuffle(x+ox, y+oy, xm1, xc); }
float get_lump(float x, float y, float nhsz, float xm0, float xm1) {
	float 	nhsz_c 	= 0.0;
	float 	xcn 	= 0.0;
	float 	nh_val 	= 0.0;
	for(float i = -nhsz; i <= nhsz; i += 1.0) {
		for(float j = -nhsz; j <= nhsz; j += 1.0) {
			nh_val = round(sqrt(i*i+j*j));
			if(nh_val <= nhsz) {
				xcn = xcn + get_xcn(x, y, xm0, xm1, i, j);
				nhsz_c = nhsz_c + 1.0; } } }
	float 	xcnf 	= ( xcn / nhsz_c );
	float 	xcaf	= xcnf;
	for(float i = 0.0; i <= nhsz; i += 1.0) {
			xcaf 	= clamp((xcnf*xcaf + xcnf*xcaf) * (xcnf+xcnf), 0.0, 1.0); }
	return xcaf; }
float reseed(uint seed, float scl, float amp) {
	float 	fx = gl_FragCoord[0];
	float 	fy = gl_FragCoord[1];
	float 	r0 = get_lump(fx, fy, round( 6.0  * scl), 19.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),17.0), 23.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),43.0));
	float 	r1 = get_lump(fx, fy, round( 22.0 * scl), 13.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),29.0), 17.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),31.0));
	float 	r2 = get_lump(fx, fy, round( 14.0 * scl), 13.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),11.0), 51.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),37.0));
	float 	r3 = get_lump(fx, fy, round( 18.0 * scl), 29.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed), 7.0), 61.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),28.0));
	return clamp( sqrt((r0+r1)*r3*(amp+1.2))-r2*(amp*1.8+0.2) , 0.0, 1.0); }

struct ConvData {
	vec4 	value;
	float 	total;
};


ConvData ring( float r ) {

	const float psn = 32768.0;

	float tot = 0.0;
	vec4  val = vec4(0.0,0.0,0.0,0.0);

	float sq2	= sqrt(2.0);

	float o_0 = r + 0.5;
	float o_1 = sq2 * o_0;
	float o_2 = o_1 / 2.0;
	float o_3 = sqrt( o_0*o_0 - r*r );
	float o_4 = o_2 - ( floor(o_2) + 0.5 );
	float o_5 = floor( o_2 ) + floor( o_4 );

	float i_0 = r - 0.5;
	float i_1 = sq2 * i_0;
	float i_2 = i_1 / 2.0;
	float i_3 = sqrt( i_0*i_0 - r*r );
	float i_4 = i_2 - ( floor(i_2) + 1.0 );
	float i_5 = floor( i_2 ) + floor( i_4 );

	float d_0 = ( i_5 ) + 1.0 - ( o_5 );

	for(float i = 1.0; i < floor( i_2 ) + 1.0 - d_0; i++) {

		float j_0 = sqrt( o_0*o_0 - (i+0.0)*(i+0.0) );
		float j_1 = sqrt( i_0*i_0 - (i+0.0)*(i+0.0) );
		float j_2 = ( 1.0 - abs( sign ( (floor( i_2 ) + 1.0) - i ) ) );

		for(float j = floor( j_1 ) + j_2; j < floor( j_0 ); j++) {
			val += floor(gdv(ivec2( i, (int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2( i,-(int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2(-i,-(int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2(-i, (int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2( (int(j)+1), i), txdata) * psn);
			val += floor(gdv(ivec2( (int(j)+1),-i), txdata) * psn);
			val += floor(gdv(ivec2(-(int(j)+1),-i), txdata) * psn);
			val += floor(gdv(ivec2(-(int(j)+1), i), txdata) * psn);
			tot += 8.0 * psn; } }

//	Orthagonal
	val += floor(gdv(ivec2( r, 0), txdata) * psn);
	val += floor(gdv(ivec2( 0,-r), txdata) * psn);
	val += floor(gdv(ivec2(-r,-0), txdata) * psn);
	val += floor(gdv(ivec2(-0, r), txdata) * psn);
	tot += 4.0 * psn;

//	Diagonal
//	TODO This is not quite perfect
	float k_0 = r;
	float k_1 = sq2 * k_0;
	float k_2 = k_1 / 2.0;
	float k_3 = sqrt( k_0*k_0 - r*r );
	float k_4 = k_2 - ( floor(k_2) + 1.0 );
	float k_5 = floor( k_2 ) + floor( k_4 );

	float dist = round(k_2);

	if( sign( o_4 ) == -1.0 ) {
	//	val += gdv(ivec2( (floor(o_5)+1), floor(o_5)+1), txdata);
		val += floor(gdv(ivec2( (floor(o_5)+1.0), (floor(o_5)+1.0)), txdata) * psn);
		val += floor(gdv(ivec2( (floor(o_5)+1.0),-(floor(o_5)+1.0)), txdata) * psn);
		val += floor(gdv(ivec2(-(floor(o_5)+1.0),-(floor(o_5)+1.0)), txdata) * psn);
		val += floor(gdv(ivec2(-(floor(o_5)+1.0), (floor(o_5)+1.0)), txdata) * psn);
		tot += 4.0 * psn; }

	return ConvData( val, tot ); }


vec4 conv( float r ) {
	ConvData nh = ring( r );
	return 	nh.value / nh.total; }
    
vec4 bitmake(ConvData[MAX_RADIUS] rings, uint bits, uint of) {
	vec4  sum = vec4(0.0,0.0,0.0,0.0);
	float tot = 0.0;
	for(uint i = 0u; i < MAX_RADIUS; i++) {
		if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings[i].value; tot += rings[i].total; } }
	return sum / tot; }

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

//	----    ----    ----    ----    ----    ----    ----    ----
//	Rule Initilisation
//	----    ----    ----    ----    ----    ----    ----    ----

//	NH Rings
	ConvData[MAX_RADIUS] nh_rings_m;
	for(uint i = 0u; i < MAX_RADIUS; i++) { nh_rings_m[i] = ring(float(i+1u)); }

//	Parameters
	const	float 	mnp 	= 1.0 / 65536.0;			//	Minimum value of a precise step for 16-bit channel
	const	float 	s  		= mnp *  80.0 *  128.0;
	const	float 	n  		= mnp *  80.0 *   2.0;

//	Output Values
	vec4 res_c = gdv( ivec2(0, 0), txdata );

//	Result Values
	vec4 res_v = res_c;

//	----    ----    ----    ----    ----    ----    ----    ----
//	Update Functions
//	----    ----    ----    ----    ----    ----    ----    ----

	uint[12] nb = uint[12] (
		ub[0],  ub[1],  ub[2],  ub[3],
		ub[4],  ub[5],  ub[6],  ub[7],
		ub[8],  ub[9],  ub[10], ub[11] );

	uint[24] ur = uint[24] (
		ub[12], ub[13], ub[14], ub[15], 
		ub[16], ub[17], ub[18], ub[19],	
		ub[20], ub[21], ub[22], ub[23],
		ub[24], ub[25], ub[26], ub[27],	
		ub[28], ub[29], ub[30], ub[31], 
		ub[32], ub[33], ub[34], ub[35]  );

	uint[ 3] ch2 = uint[ 3] ( 2286157824u, 295261525u, 1713547946u );
	uint[ 3] ch  = uint[ 3] ( ub[38], ub[39], ub[40] );
	uint[ 3] ch3 = uint[ 3] ( ub[41], ub[42], ub[43] );

//	Update Sign
	uint[ 2] us = uint[ 2] ( ub[36], ub[37] );

	vec4[12] smnca_res = vec4[12](res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c);

	vec4 conv1 = conv(1.0);

	for(uint i = 0u; i < 24u; i++) {
		uint  	cho = u32_upk( ch[i/8u], 2u, (i*4u+0u) & 31u );
				cho = (cho == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+0u) & 31u ) : cho;
		uint  	chi = u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u );
				chi = (chi == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+2u) & 31u ) : chi;
		uint  	chm = u32_upk( ch3[i/8u], 2u, (i*4u+2u) & 31u );
				chm = (chm == 3u) ? u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u ) : chm;
                
		vec4 nhv = bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u );

		if( nhv[cho] >= utp( ur[i], 8u, 0u) && nhv[cho] <= utp( ur[i], 8u, 1u)) {
			smnca_res[i/4u][chi] += bsn(us[i/16u], ((i*2u+0u) & 31u)) * s * res_c[chm]; }

		if( nhv[cho] >= utp( ur[i], 8u, 2u) && nhv[cho] <= utp( ur[i], 8u, 3u)) {
			smnca_res[i/4u][chi] += bsn(us[i/16u], ((i*2u+1u) & 31u)) * s * res_c[chm]; } }

	uvec4 dev_idx = uvec4(0u,0u,0u,0u);
    
	vec4 dev = vec4(0.0,0.0,0.0,0.0);
	for(uint i = 0u; i < 6u; i++) {
		vec4 smnca_res_temp = abs(res_c - smnca_res[i]);
		if(smnca_res_temp[0] > dev[0]) { dev_idx[0] = i; dev[0] = smnca_res_temp[0]; }
		if(smnca_res_temp[1] > dev[1]) { dev_idx[1] = i; dev[1] = smnca_res_temp[1]; }
		if(smnca_res_temp[2] > dev[2]) { dev_idx[2] = i; dev[2] = smnca_res_temp[2]; }
		if(smnca_res_temp[3] > dev[3]) { dev_idx[3] = i; dev[3] = smnca_res_temp[3]; } }

	res_v[0] = smnca_res[dev_idx[0]][0];
	res_v[1] = smnca_res[dev_idx[1]][1];
	res_v[2] = smnca_res[dev_idx[2]][2];
	res_v[3] = smnca_res[dev_idx[3]][3];

    res_c = ((res_v + (conv1 * (s*2.13333))) / (1.0 + (s*2.13333)))- 0.01 * s;
    
//	----    ----    ----    ----    ----    ----    ----    ----
//	Shader Output
//	----    ----    ----    ----    ----    ----    ----    ----


    if (iMouse.z > 0. && length(iMouse.xy - fragCoord) < 14.0) {
        res_c[0] = round(mod(float(iFrame),2.0));
        res_c[1] = round(mod(float(iFrame),3.0));
        res_c[2] = round(mod(float(iFrame),5.0)); }
    if (iFrame == 0) { res_c[0] = reseed(0u, 1.0, 0.4); res_c[1] = reseed(1u, 1.0, 0.4); res_c[2] = reseed(2u, 1.0, 0.4); }

//	Force alpha to 1.0
	res_c[3] 	= 1.0;
    fragColor=clamp(res_c,0.0,1.0);
}


// >>> ___ GLSL:[Buffer B] _________________________________________________________________ <<<
//	----    ----    ----    ----    ----    ----    ----    ----
//  Shader developed by Slackermanz
//
//  Info/Code:
//  ﻿ - Website: https://slackermanz.com
//  ﻿ - Github: https://github.com/Slackermanz
//  ﻿ - Shadertoy: https://www.shadertoy.com/user/SlackermanzCA
//  ﻿ - Discord: https://discord.gg/hqRzg74kKT
//  
//  Socials:
//  ﻿ - Discord DM: Slackermanz#3405
//  ﻿ - Reddit DM: https://old.reddit.com/user/slackermanz
//  ﻿ - Twitter: https://twitter.com/slackermanz
//  ﻿ - YouTube: https://www.youtube.com/c/slackermanz
//  ﻿ - Older YT: https://www.youtube.com/channel/UCZD4RoffXIDoEARW5aGkEbg
//  ﻿ - TikTok: https://www.tiktok.com/@slackermanz
//  
//  Communities:
//  ﻿ - Reddit: https://old.reddit.com/r/cellular_automata
//  ﻿ - Artificial Life: https://discord.gg/7qvBBVca7u
//  ﻿ - Emergence: https://discord.com/invite/J3phjtD
//  ﻿ - ConwayLifeLounge: https://discord.gg/BCuYCEn
//	----    ----    ----    ----    ----    ----    ----    ----

#define txdata (iChannel0)
#define PI 3.14159265359
#define LN 2.71828182846

const uint MAX_RADIUS = 12u;

uint[64] ub =  uint[64]
(   4086443839u, 803560891u, 3521573439u, 155586747u, 
    2355529581u, 3804082561u, 1278181521u, 1198599219u, 
    790900093u, 2043079403u, 72510135u, 329989440u, 
    1205735441u, 1165390975u, 1863025477u, 1552315409u, 
    1460749058u, 1961704519u, 1063988442u, 304586942u, 
    1969173229u, 623707175u, 2719649363u, 533620173u, 
    734380903u, 1866742626u, 69847740u, 779938642u, 
    3928012151u, 1597352029u, 1939485308u, 1599391651u, 
    824038858u, 498247516u, 609160531u, 634145519u, 
    2022645937u, 4285315796u, 87513080u, 246410766u, 
    1160189374u, 688725303u, 1266836767u, 670912482u, 
    2162941226u, 1742659144u, 481786434u, 3618106514u, 
    0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 
    0u, 0u, 0u, 0u, 
    0u, 1067687164u, 1122344419u, 0u );

uint u32_upk(uint u32, uint bts, uint off) { return (u32 >> off) & ((1u << bts)-1u); }

float lmap() { return (gl_FragCoord[0] / float(textureSize(txdata,0)[0])); }
float vmap() { return (gl_FragCoord[1] / float(textureSize(txdata,0)[1])); }
float cmap() { return sqrt	( ((gl_FragCoord[0] - float(textureSize(txdata,0)[0])*0.5) / float(textureSize(txdata,0)[0])*0.5)
							* ((gl_FragCoord[0] - float(textureSize(txdata,0)[0])*0.5) / float(textureSize(txdata,0)[0])*0.5)
							+ ((gl_FragCoord[1] - float(textureSize(txdata,0)[1])*0.5) / float(textureSize(txdata,0)[1])*0.5)
							* ((gl_FragCoord[1] - float(textureSize(txdata,0)[1])*0.5) / float(textureSize(txdata,0)[1])*0.5) ); }
float vwm() {
	float 	scale_raw 	= uintBitsToFloat(ub[62]);
	float 	zoom 		= uintBitsToFloat(ub[61]);
	float	scale_new	= scale_raw;
	uint 	mode 		= u32_upk(ub[59], 2u, 0u);
	if( mode == 1u ) { //	Linear Parameter Map
		scale_new = ((lmap() + zoom) * (scale_raw / (1.0 + zoom * 2.0))) * 2.0; }
	if( mode == 2u ) { //	Circular Parameter Map
		scale_new = ((sqrt(cmap()) + zoom) * (scale_raw / (1.0 + zoom * 2.0))) * 2.0; }
	return scale_new; }
    
float  tp(uint n, float s) 			{ return (float(n+1u)/256.0) * ((s*0.5)/128.0); }
float utp(uint v, uint  w, uint o) 	{ return tp(u32_upk(v,w,w*o), vwm()); }
float bsn(uint v, uint  o) 			{ return float(u32_upk(v,1u,o)*2u)-1.0; }
    
vec4  sigm(vec4  x, float w) { return 1.0 / ( 1.0 + exp( (-w*2.0 * x * (PI/2.0)) + w * (PI/2.0) ) ); }
float hmp2(float x, float w) { return 3.0*((x-0.5)*(x-0.5))+0.25; }

vec4  gdv( ivec2 of, sampler2D tx ) {
	of 		= ivec2(gl_FragCoord) + of;
	of[0] 	= (of[0] + textureSize(tx,0)[0]) % (textureSize(tx,0)[0]);
	of[1] 	= (of[1] + textureSize(tx,0)[1]) % (textureSize(tx,0)[1]);
	return 	texelFetch( tx, of, 0); }
    
vec4 nbhd( vec2 r, sampler2D tx ) {
//	Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
	uint	chk = 2147483648u /
			(	( 	uint( r[0]*r[0]*PI + r[0]*PI + PI	)
				- 	uint( r[1]*r[1]*PI + r[1]*PI		) ) * 128u );
	float	psn = (chk >= 65536u) ? 65536.0 : float(chk);
	vec4	a = vec4(0.0,0.0,0.0,0.0);
	float	w = 1.0;	// Weighting, unused
	if(r[0] == 0.0) { return vec4( gdv( ivec2(0,0), tx )*w*psn ); }
	else 			{
		for(float i = 0.0; i <= r[0]; i++) {
			for(float j = 1.0; j <= r[0]; j++) {
				float	d = round(sqrt(i*i+j*j));
						w = 1.0;	//	Per-Neighbor Weighting, unused
				if( d <= r[0] && d > r[1] ) {
					vec4 t0  = gdv( ivec2( i, j), tx ) * w * psn; a += t0 - fract(t0);
					vec4 t1  = gdv( ivec2( j,-i), tx ) * w * psn; a += t1 - fract(t1);
					vec4 t2  = gdv( ivec2(-i,-j), tx ) * w * psn; a += t2 - fract(t2);
					vec4 t3  = gdv( ivec2(-j, i), tx ) * w * psn; a += t3 - fract(t3); } } }
		return a; } }

vec4 totl( vec2 r, sampler2D tx ) {
//	Precision limit of signed float32 for [n] neighbors in a 16 bit texture (symmetry preservation)
	uint	chk = 2147483648u /
			(	( 	uint( r[0]*r[0]*PI + r[0]*PI + PI	)
				- 	uint( r[1]*r[1]*PI + r[1]*PI		) ) * 128u );
	float	psn = (chk >= 65536u) ? 65536.0 : float(chk);
	vec4	b = vec4(0.0,0.0,0.0,0.0);
	float	w = 1.0;	// Weighting, unused
	if(r[0] == 0.0) { return vec4( psn * w, psn * w, psn * w, psn * w ); }
	else 			{
		for(float i = 0.0; i <= r[0]; i++) {
			for(float j = 1.0; j <= r[0]; j++) {
				float	d = round(sqrt(i*i+j*j));
						w = 1.0;	//	Per-Neighbor Weighting, unused
				if( d <= r[0] && d > r[1] ) { b += psn * w * 4.0; } } }
		return b; } }

vec4 bitring(vec4[MAX_RADIUS] rings_a, vec4[MAX_RADIUS] rings_b, uint bits, uint of) {
	vec4 sum = vec4(0.0,0.0,0.0,0.0);
	vec4 tot = vec4(0.0,0.0,0.0,0.0);
	for(uint i = 0u; i < MAX_RADIUS; i++) {
		if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings_a[i]; tot += rings_b[i]; } }
	return sigm( (sum / tot), LN ); } // TODO
    
vec4 conv(vec2 r, sampler2D tx) {
	vec4 nha = nbhd(r, tx);
	vec4 nhb = totl(r, tx);
	return 	nha / nhb; }
    
//	Used to reseed the surface with lumpy noise
float get_xc(float x, float y, float xmod) {
	float sq = sqrt(mod(x*y+y, xmod)) / sqrt(xmod);
	float xc = mod((x*x)+(y*y), xmod) / xmod;
	return clamp((sq+xc)*0.5, 0.0, 1.0); }
float shuffle(float x, float y, float xmod, float val) {
	val = val * mod( x*y + x, xmod );
	return (val-floor(val)); }
float get_xcn(float x, float y, float xm0, float xm1, float ox, float oy) {
	float  xc = get_xc(x+ox, y+oy, xm0);
	return shuffle(x+ox, y+oy, xm1, xc); }
float get_lump(float x, float y, float nhsz, float xm0, float xm1) {
	float 	nhsz_c 	= 0.0;
	float 	xcn 	= 0.0;
	float 	nh_val 	= 0.0;
	for(float i = -nhsz; i <= nhsz; i += 1.0) {
		for(float j = -nhsz; j <= nhsz; j += 1.0) {
			nh_val = round(sqrt(i*i+j*j));
			if(nh_val <= nhsz) {
				xcn = xcn + get_xcn(x, y, xm0, xm1, i, j);
				nhsz_c = nhsz_c + 1.0; } } }
	float 	xcnf 	= ( xcn / nhsz_c );
	float 	xcaf	= xcnf;
	for(float i = 0.0; i <= nhsz; i += 1.0) {
			xcaf 	= clamp((xcnf*xcaf + xcnf*xcaf) * (xcnf+xcnf), 0.0, 1.0); }
	return xcaf; }
float reseed(uint seed, float scl, float amp) {
	float 	fx = gl_FragCoord[0];
	float 	fy = gl_FragCoord[1];
	float 	r0 = get_lump(fx, fy, round( 6.0  * scl), 19.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),17.0), 23.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),43.0));
	float 	r1 = get_lump(fx, fy, round( 22.0 * scl), 13.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),29.0), 17.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),31.0));
	float 	r2 = get_lump(fx, fy, round( 14.0 * scl), 13.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),11.0), 51.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),37.0));
	float 	r3 = get_lump(fx, fy, round( 18.0 * scl), 29.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed), 7.0), 61.0 + mod(float(u32_upk(ub[63], 24u, 0u)+seed),28.0));
	return clamp( sqrt((r0+r1)*r3*(amp+1.2))-r2*(amp*1.8+0.2) , 0.0, 1.0); }

struct ConvData {
	vec4 	value;
	float 	total;
};


ConvData ring( float r ) {

	const float psn = 32768.0;

	float tot = 0.0;
	vec4  val = vec4(0.0,0.0,0.0,0.0);

	float sq2	= sqrt(2.0);

	float o_0 = r + 0.5;
	float o_1 = sq2 * o_0;
	float o_2 = o_1 / 2.0;
	float o_3 = sqrt( o_0*o_0 - r*r );
	float o_4 = o_2 - ( floor(o_2) + 0.5 );
	float o_5 = floor( o_2 ) + floor( o_4 );

	float i_0 = r - 0.5;
	float i_1 = sq2 * i_0;
	float i_2 = i_1 / 2.0;
	float i_3 = sqrt( i_0*i_0 - r*r );
	float i_4 = i_2 - ( floor(i_2) + 1.0 );
	float i_5 = floor( i_2 ) + floor( i_4 );

	float d_0 = ( i_5 ) + 1.0 - ( o_5 );

	for(float i = 1.0; i < floor( i_2 ) + 1.0 - d_0; i++) {

		float j_0 = sqrt( o_0*o_0 - (i+0.0)*(i+0.0) );
		float j_1 = sqrt( i_0*i_0 - (i+0.0)*(i+0.0) );
		float j_2 = ( 1.0 - abs( sign ( (floor( i_2 ) + 1.0) - i ) ) );

		for(float j = floor( j_1 ) + j_2; j < floor( j_0 ); j++) {
			val += floor(gdv(ivec2( i, (int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2( i,-(int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2(-i,-(int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2(-i, (int(j)+1)), txdata) * psn);
			val += floor(gdv(ivec2( (int(j)+1), i), txdata) * psn);
			val += floor(gdv(ivec2( (int(j)+1),-i), txdata) * psn);
			val += floor(gdv(ivec2(-(int(j)+1),-i), txdata) * psn);
			val += floor(gdv(ivec2(-(int(j)+1), i), txdata) * psn);
			tot += 8.0 * psn; } }

//	Orthagonal
	val += floor(gdv(ivec2( r, 0), txdata) * psn);
	val += floor(gdv(ivec2( 0,-r), txdata) * psn);
	val += floor(gdv(ivec2(-r,-0), txdata) * psn);
	val += floor(gdv(ivec2(-0, r), txdata) * psn);
	tot += 4.0 * psn;

//	Diagonal
//	TODO This is not quite perfect
	float k_0 = r;
	float k_1 = sq2 * k_0;
	float k_2 = k_1 / 2.0;
	float k_3 = sqrt( k_0*k_0 - r*r );
	float k_4 = k_2 - ( floor(k_2) + 1.0 );
	float k_5 = floor( k_2 ) + floor( k_4 );

	float dist = round(k_2);

	if( sign( o_4 ) == -1.0 ) {
	//	val += gdv(ivec2( (floor(o_5)+1), floor(o_5)+1), txdata);
		val += floor(gdv(ivec2( (floor(o_5)+1.0), (floor(o_5)+1.0)), txdata) * psn);
		val += floor(gdv(ivec2( (floor(o_5)+1.0),-(floor(o_5)+1.0)), txdata) * psn);
		val += floor(gdv(ivec2(-(floor(o_5)+1.0),-(floor(o_5)+1.0)), txdata) * psn);
		val += floor(gdv(ivec2(-(floor(o_5)+1.0), (floor(o_5)+1.0)), txdata) * psn);
		tot += 4.0 * psn; }

	return ConvData( val, tot ); }


vec4 conv( float r ) {
	ConvData nh = ring( r );
	return 	nh.value / nh.total; }
    
vec4 bitmake(ConvData[MAX_RADIUS] rings, uint bits, uint of) {
	vec4  sum = vec4(0.0,0.0,0.0,0.0);
	float tot = 0.0;
	for(uint i = 0u; i < MAX_RADIUS; i++) {
		if(u32_upk(bits, 1u, i+of) == 1u) { sum += rings[i].value; tot += rings[i].total; } }
	return sum / tot; }

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {

//	----    ----    ----    ----    ----    ----    ----    ----
//	Rule Initilisation
//	----    ----    ----    ----    ----    ----    ----    ----

//	NH Rings
	ConvData[MAX_RADIUS] nh_rings_m;
	for(uint i = 0u; i < MAX_RADIUS; i++) { nh_rings_m[i] = ring(float(i+1u)); }

//	Parameters
	const	float 	mnp 	= 1.0 / 65536.0;			//	Minimum value of a precise step for 16-bit channel
	const	float 	s  		= mnp *  80.0 *  128.0;
	const	float 	n  		= mnp *  80.0 *   2.0;

//	Output Values
	vec4 res_c = gdv( ivec2(0, 0), txdata );

//	Result Values
	vec4 res_v = res_c;

//	----    ----    ----    ----    ----    ----    ----    ----
//	Update Functions
//	----    ----    ----    ----    ----    ----    ----    ----

	uint[12] nb = uint[12] (
		ub[0],  ub[1],  ub[2],  ub[3],
		ub[4],  ub[5],  ub[6],  ub[7],
		ub[8],  ub[9],  ub[10], ub[11] );

	uint[24] ur = uint[24] (
		ub[12], ub[13], ub[14], ub[15], 
		ub[16], ub[17], ub[18], ub[19],	
		ub[20], ub[21], ub[22], ub[23],
		ub[24], ub[25], ub[26], ub[27],	
		ub[28], ub[29], ub[30], ub[31], 
		ub[32], ub[33], ub[34], ub[35]  );

	uint[ 3] ch2 = uint[ 3] ( 2286157824u, 295261525u, 1713547946u );
	uint[ 3] ch  = uint[ 3] ( ub[38], ub[39], ub[40] );
	uint[ 3] ch3 = uint[ 3] ( ub[41], ub[42], ub[43] );

//	Update Sign
	uint[ 2] us = uint[ 2] ( ub[36], ub[37] );

	vec4[12] smnca_res = vec4[12](res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c,res_c);

	vec4 conv1 = conv(1.0);

	for(uint i = 0u; i < 24u; i++) {
		uint  	cho = u32_upk( ch[i/8u], 2u, (i*4u+0u) & 31u );
				cho = (cho == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+0u) & 31u ) : cho;
		uint  	chi = u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u );
				chi = (chi == 3u) ? u32_upk( ch2[i/8u], 2u, (i*4u+2u) & 31u ) : chi;
		uint  	chm = u32_upk( ch3[i/8u], 2u, (i*4u+2u) & 31u );
				chm = (chm == 3u) ? u32_upk( ch[i/8u], 2u, (i*4u+2u) & 31u ) : chm;
                
		vec4 nhv = bitmake( nh_rings_m, nb[i/2u], (i & 1u) * 16u );

		if( nhv[cho] >= utp( ur[i], 8u, 0u) && nhv[cho] <= utp( ur[i], 8u, 1u)) {
			smnca_res[i/4u][chi] += bsn(us[i/16u], ((i*2u+0u) & 31u)) * s * res_c[chm]; }

		if( nhv[cho] >= utp( ur[i], 8u, 2u) && nhv[cho] <= utp( ur[i], 8u, 3u)) {
			smnca_res[i/4u][chi] += bsn(us[i/16u], ((i*2u+1u) & 31u)) * s * res_c[chm]; } }

	uvec4 dev_idx = uvec4(0u,0u,0u,0u);
    
	vec4 dev = vec4(0.0,0.0,0.0,0.0);
	for(uint i = 0u; i < 6u; i++) {
		vec4 smnca_res_temp = abs(res_c - smnca_res[i]);
		if(smnca_res_temp[0] > dev[0]) { dev_idx[0] = i; dev[0] = smnca_res_temp[0]; }
		if(smnca_res_temp[1] > dev[1]) { dev_idx[1] = i; dev[1] = smnca_res_temp[1]; }
		if(smnca_res_temp[2] > dev[2]) { dev_idx[2] = i; dev[2] = smnca_res_temp[2]; }
		if(smnca_res_temp[3] > dev[3]) { dev_idx[3] = i; dev[3] = smnca_res_temp[3]; } }

	res_v[0] = smnca_res[dev_idx[0]][0];
	res_v[1] = smnca_res[dev_idx[1]][1];
	res_v[2] = smnca_res[dev_idx[2]][2];
	res_v[3] = smnca_res[dev_idx[3]][3];

    res_c = ((res_v + (conv1 * (s*2.13333))) / (1.0 + (s*2.13333)))- 0.01 * s;
    
//	----    ----    ----    ----    ----    ----    ----    ----
//	Shader Output
//	----    ----    ----    ----    ----    ----    ----    ----


    if (iMouse.z > 0. && length(iMouse.xy - fragCoord) < 14.0) {
        res_c[0] = round(mod(float(iFrame),2.0));
        res_c[1] = round(mod(float(iFrame),3.0));
        res_c[2] = round(mod(float(iFrame),5.0)); }
    if (iFrame == 0) { res_c[0] = reseed(0u, 1.0, 0.4); res_c[1] = reseed(1u, 1.0, 0.4); res_c[2] = reseed(2u, 1.0, 0.4); }

//	Force alpha to 1.0
	res_c[3] 	= 1.0;
    fragColor=clamp(res_c,0.0,1.0);
}

