

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// Created by sebastien durand /2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

// TODO debug this for new chrome !!

//#define NO_LEAF
//#define BENJAMIN_BUTTON

#define ZERO min(0,iFrame) 

const float 
	PI = 3.14159265358979323846,
	PI2 = 2.*PI,
    AU = 149597870.61, // Astronomical Unit = Mean distance of the earth from the sun - Km
	TORAD = PI/180.,
    TODEG = 180./PI;
const float 
    LATITUDE = 42., 
    LONGITUDE = 1.,
    UTC_HOUR = 7.8,
	OLD_TREE = 1.3,
	VERY_OLD_TREE = 3.;

const vec3
    Julia=vec3(-.1825,-.905,-.2085),
    ba=vec3(0.,0.7,0.),
	ba2=vec3(0.,2.1,0.),
    SUN_COLOR=vec3(1., .75, .6);

float 
    oobdb = 1./dot(ba,ba),
    oobd2 = 1./dot(ba2,ba2);

float jGlobalTime,life, season, treeStep, frc, f2, thinBranch, rBranch, clpOld, clpVeryOld;  
int bEnd, fStart, fEnd;
vec3 sunLight, sunColour;


bool intersectSphere(in vec3 ro, in vec3 rd, in float r) {
	float b = dot(rd,ro), d = b*b - dot(ro,ro) + r*r;
	return (d>0. && -sqrt(d)-b > 0.);
}





// - Tools ---------------------------------------------------------------------------

float hash(in float n) { return fract(sin(n) * 43758.5453123); }

// polynomial smooth min (k = 0.1) - iq;
float smin(in float a, in float b, in float k ){
    float h = clamp(.5+.5*(b-a)/k, .0, 1.);
    return mix(b,a,h) - k*h*(1.-h);
}

float Noise(in vec2 x) {
    vec2 p=floor(x), f=fract(x);
    f *= f*(3.-2.*f);
    float n = p.x + p.y*57.;
    return mix(mix(hash(n+ 0.), hash(n+ 1.),f.x),
               mix(hash(n+57.), hash(n+58.),f.x),f.y);

}

mat3 rotAA(in vec3 v, in float angle){//axis angle rotation
	float c=cos(angle);
	vec3  s=v*sin(angle);
	return mat3(v.xxx*v,v.yyy*v,v.zzz*v)*(1.0-c)+
		   mat3(c,-s.z,s.y,s.z,c,-s.x,-s.y,s.x,c);
}

// -----------------------------------------------------------------------------------

mat3 rmx,mrot;

// - Shapes & distances --------------------------------------------------------------

vec2 opU(vec2 d1, vec2 d2 ) {
	return (d1.x<d2.x) ? d1 : d2;
}
const float MAX_DIST = 18.;



float sdSubTree0(in vec3 p0) {
	float obj = 0.;
	//p.xz = mod(p.xz, 4.) - 2.;
	float text, dr=0.74074, dd, d0 = MAX_DIST, d = d0;
	
	vec3 p = p0; //, pBest;
	vec2 pt;
	float brBest;
	float k=1.;
    int ffEnd = fEnd;
    int ffStart = fStart;
    
	for (int n=ZERO; n<10; n++) {
		if (n>=bEnd) break;
		dd = (length(p-ba*clamp(dot(p,ba)*oobdb,0.,1.05))-rBranch+p.y*thinBranch)*dr;
        d = (dd<d) ? smin(d,dd,.00155) : d;
		p.x = abs(p.x);
		p = p*rmx + k*Julia;
		dr *= 0.74074;
        ffEnd--;
        ffStart--;
    }
	dd = (length(p-ba*clamp(dot(p,ba)*oobdb,-0.1,frc))-rBranch+p.y*thinBranch)*dr;
	d = (dd<d) ? smin(d,dd,dr*.055) : d;
	k = .5;
   
    for (int n=ZERO; n<6; n++) {
        if (n>ffEnd) break;
		if (n>=ffStart) { // Leafs
			p += (n==ffStart) ? (.08+sin(p.yzx*15.0)*.37) : vec3(0.);
			dd = (length(p- ba2*f2*clamp(dot(p.y,ba2.y)*oobd2,.0,3.))-0.2)*dr;
			d = (dd<d) ? dd+.0001 : d;
		}
		p.x = abs(p.x);
		p = p*rmx + k*Julia;
		dr *= 0.74074;
    }
    return d;
}
	
float sdTree0(in vec3 p) {
	// Mix 2 tree to brake symetrie
	float scale =  mix(life, 1.5, clpOld);
	//p.z *= .8;
	p.xyz /= scale;//mix(life, 1.5, clpOld);
	p += vec3(0.,.06,0.)+ sin(p.yzx*vec3(5.,6.,3.))*0.017;
	// first part
    float d1 = sdSubTree0(p);	
    // second part
	p.y /= 1.3;
	p.xyz *= mrot;
	float d2 = sdSubTree0(p);	
	
	d2 *= scale;
	d1 *= scale;
	return min(d1,d2);
}


// first based on Orchard@Night by eiffie (https://www.shadertoy.com/view/4s23Rh)
vec2 sdSubTree(in vec3 p0) {
	float obj = 0.;
	//p.xz = mod(p.xz, 4.) - 2.;
	float text, dr=0.74074, dd, d0 = MAX_DIST, d = d0;
	
	vec3 p = p0, pBest;
	vec2 pt;
	float brBest;
	float k=1.;
	for (int n=ZERO; n<14; n++) {
        if (n>fEnd) break;
		if (n<bEnd) {
			dd = (length(p-ba*clamp(dot(p,ba)*oobdb,0.,1.05))-rBranch+p.y*thinBranch)*dr;
			if (dd < d) {
				d = smin(d,dd,.00155);
				pBest = p;
			}
		}
		else if(n==bEnd) {
			dd = (length(p-ba*clamp(dot(p,ba)*oobdb,-0.1,frc))-rBranch+p.y*thinBranch)*dr;
			if (dd < d) {
				d = smin(d,dd,dr*.055);
				pBest = p;
			}
			if (d<d0) {
				// TODO find best shade
				text = cos(pBest.y*50.+2.5*hash(pBest.x*pBest.y));
				obj = 51. + text;
				//d+=text;//.005*Noise(pBest.xy*30.);
				//d0=d;
			}
			k = .5;
		}	
		else if (n>=fStart && n<=fEnd) { // Leafs
			if(n==fStart) { // Debut feuille
				p +=.08+sin(p.yzx*15.0)*.37;
			//	p.z*=.6 + .4*sin(p.y*.1);
			} 
			dd = (length(p- ba2*f2*clamp(dot(p.y,ba2.y)*oobd2,.0,3.))-0.2)*dr;
			if (dd<d) {
				d = dd+.0001;
				obj = float(n-fStart+2);
			}
		}
		p.x = abs(p.x);
		p = p*rmx + k*Julia;
		dr *= 0.74074;
    }
    return vec2(d, obj);
}
	
vec2 sdTree(in vec3 p) {
	// Mix 2 tree to brake symetrie
	float scale =  mix(life, 1.5, clpOld);
	//p.z *= .8;
	p.xyz /= scale;//mix(life, 1.5, clpOld);
	p += vec3(0.,.06,0.)+ sin(p.yzx*vec3(5.,6.,3.))*0.017;
	// first part
    vec2 d1 = sdSubTree(p);	
    // second part
	p.y /= 1.3;
	p.xyz *= mrot;
	vec2 d2 = sdSubTree(p);	
	
	d2.x *= scale;
	d1.x *= scale;
	float d = smin(d1.x,d2.x, .05*clamp(3.-p.y,0.,10.));
    return vec2(d, (d1.x<d2.x) ? d1.y : d2.y);
}

	
float sdLandscape(in vec3 p) {
    // Artificials hills
	float h=(.6-.6*cos(p.x/1.5)*cos(p.z/1.5))+(season>.75?.002:.006)*Noise(p.zx*35.); // very regular patern + a little bit of noise
	return p.y+h-(season>.75?.08:.0);
}

//- Scene --------------------------------------------------------------------


vec2 mapLandscape(in vec3 p) {
	return vec2(sdLandscape(p), 1.);
}

vec2 mapTree(in vec3 p) {
	float 
        a = mix(.045,.001,clpOld)*p.y*cos(jGlobalTime*10.), // (life < OLD_TREE ? 22. : mix(22.,1.,clamp(life- OLD_TREE,0.,1.)))); 
	    c = cos(a),
        s = sin(a);
    p.xz *= mat2(c,-s,s,c);
	p.y *= (1.05+ mix(.04*cos(jGlobalTime/4.*6.28+.8), 0., clpOld));
	return sdTree(p);
}
float mapTree0(in vec3 p) {
	float 
        a = mix(.045,.001,clpOld)*p.y*cos(jGlobalTime*10.), // (life < OLD_TREE ? 22. : mix(22.,1.,clamp(life- OLD_TREE,0.,1.)))); 
	    c = cos(a),
        s = sin(a);
    p.xz *= mat2(c,-s,s,c);
	p.y *= (1.05+ mix(.04*cos(jGlobalTime/4.*6.28+.8), 0., clpOld));
	return sdTree0(p);
}

float map0(in vec3 p, bool tree, bool landscape) {
    float res = 1000.;
    if (tree)      res = min(res, mapTree0(p));
    if (landscape) res = min(res, sdLandscape(p));
	return res;
}

vec2 map(in vec3 p, bool tree, bool landscape) {
    vec2 res = vec2(1000,-1);
    if (tree)      res = opU(res, mapTree(p));
    if (landscape) res = opU(res, mapLandscape(p));
	return res;
}

//- Render --------------------------------------------------------------------------

vec2 castRay(in vec3 ro, in vec3 rd, in float maxd) {
    bool withTree = intersectSphere(ro-vec3(0.,2.6,0.), rd, 2.7);
    // TODO intersect plane for landscape
    
	float precis=.004, h=precis*2., t=5.;
	float res;
    for( int i=ZERO; i<60; i++ ) {		
		if (abs(h) < precis && t > maxd) break; 
        t += h;
        res = map0(ro+rd*t, withTree, true);
        h = res;
    }
    return vec2(t, (t<maxd) ? map(ro+rd*t, withTree, true).y : -1.);
}


//- Lighting -----------------------------------------------------------------------


// Rolling hills by Dave Hoskins (https://www.shadertoy.com/view/Xsf3zX)
// Grab all sky information for a given ray from camera
vec3 GetSky(in vec3 rd, in bool withSun) {
    float sunAmount = withSun ? max( dot( rd, sunLight), 0.0 ) : 0.0;
	return clamp( 
			mix(vec3(.1, .2, .3), vec3(.32, .32, .32), pow(1.0-max(rd.y,0.0),6.)) +
			sunColour * sunAmount * sunAmount * .25 +
			SUN_COLOR * min(pow(sunAmount, 800.0)*1.5, .3)
		, 0.0, 1.0);
}


// Merge grass into the sky background for correct fog colouring...
vec3 ApplyFog(in vec3  rgb, in float dis, in vec3 sky){
	return mix(rgb, sky, clamp(dis*dis*0.003, 0.0, 1.0));
}

// iq
float softshadow(in vec3 ro, in vec3 rd) {
#ifdef FAST
	return 1.;
#else

	float res = 1.0, h, t = .02;
    for( int i=ZERO; i<16; i++ ) {
	//	if (t < maxt) {
		h = map0(ro + rd*t, true, true);
		res = min( res, 1.*h/t );
		t += 0.3;
	//	}
    }
    return clamp(res, 0., 1.);
#endif	
}

const vec2 eps = vec2(.001, 0.);
/*
vec3 calcNormal(in vec3 p, in bool tree, in bool landscape) {
    vec2 e = vec2(eps.x, -eps.x); 
    return normalize(e.xyy * map0(p + e.xyy, tree, landscape) +
                     e.yyx * map0(p + e.yyx, tree, landscape) + 
                     e.yxy * map0(p + e.yxy, tree, landscape) + 
                     e.xxx * map0(p + e.xxx, tree, landscape));
}
*/
vec3 calcNormal( vec3 pos, vec3 ray, float t,  in bool tree, in bool landscape) {
  //  vec2 e = vec2(eps.x, -eps.x); 
  //  return normalize(e.xyy * map(pos + e.xyy).x + e.yyx * map(pos + e.yyx).x + e.yxy * map(pos + e.yxy).x + e.xxx * map(pos + e.xxx).x);;

	float pitch = .2 * t / iResolution.x;
    
//#ifdef FAST
//	// don't sample smaller than the interpolation errors in Noise()
	pitch = max( pitch, .005 );
//#endif
	
	vec2 d = vec2(-1,1) * pitch;

	vec3 p0 = pos+d.xxx; // tetrahedral offsets
	vec3 p1 = pos+d.xyy;
	vec3 p2 = pos+d.yxy;
	vec3 p3 = pos+d.yyx;
	
	float f0 =  map0(p0, tree, landscape);
    float f1 = map0(p1, tree, landscape);
	float f2 = map0(p2, tree, landscape);
	float f3 = map0(p3, tree, landscape);
	
	vec3 grad = p0*f0+p1*f1+p2*f2+p3*f3 - pos*(f0+f1+f2+f3);
	//return normalize(grad);	// prevent normals pointing away from camera (caused by precision errors)
	return normalize(grad - max(.0,dot (grad,ray ))*ray);
}



float calcAO(in vec3 pos, in vec3 nor) {
#ifdef FAST	
	return 1.;
#else
	float dd, hr=.01, totao=.0, sca=1.;	
    for(int aoi=ZERO; aoi<5; aoi++ ) {
        dd = map(nor * hr + pos, true, true).x;
        totao += -(dd-hr)*sca;
        sca *= .75;
        hr += .05;
    }
    return clamp(1.-4.*totao, 0., 1.);
#endif
}

vec3 render(in vec3 ro, in vec3 rd) { 
    vec3 col = vec3(0.0);
    vec2 res = castRay(ro,rd,MAX_DIST);
    float t = res.x, m = res.y;
float ao = 1., sh = 1.;
    if( m>-0.5 ) {
        vec3 nor, pos = ro + t*rd;
        

		if (m >= 2. && m<10.) {
            nor = calcNormal( pos, rd, t, true, false);
			col = mix(vec3(.35*m,3.-.08*m,0.), vec3(3.-.08*m,(m*.35),0.), season*1.3); // Automne
		} else if (m<2.) {
            nor = calcNormal( pos, rd, t, false, true);
			col = season>.75?vec3(1):
				  season>.3?vec3(1.3,0.9,0.3):
				  vec3(.3,0.9,0.3);
		} else {
            nor = calcNormal( pos, rd, t, true, false);
			col = vec3(.7+.3*(m-50.5));
		} 
	
		ao = calcAO(pos, nor );
        float 
		    amb = 1.,//clamp(.5+.5*nor.y, .0, 1.),
            dif = clamp(dot( nor, sunLight ), 0., 1.),
            bac = clamp(dot( nor, normalize(vec3(sunLight))), 0., 1.);//*clamp( 1.0-pos.y,0.0,1.0);

	//	if( dif>.02 ) {
		sh = 4.*softshadow( pos, sunLight); 
		dif *= sh;
	//}

		vec3 brdf = .1*ao*(amb*vec3(.10,.11,.13) + bac*.15) + 1.2*dif;

		float 
            pp = clamp(dot(reflect(rd,nor), sunLight),0.,1.),
		    spe = sh*pow(pp,16.),
		    fre = ao*pow( clamp(1.+dot(nor,rd),0.,1.), 2.);

		col = col*brdf + col*spe + .5*fre;//*(.5+.5*col);
		  //  * exp(-.01*t*t);

		// TODO calculate sun color from position and use it here
		if(spe>0.0){
			col+=.2*sunColour*pow(max(0.0,dot(reflect(rd,nor),sunLight)),8.)*spe;
		}
		#ifdef MIE_RAYLEIGH_SKY
			vec3 gp = vec3(ro.x, earth.rs + 3500.0, ro.z);
			col = ApplyFog(col, res.x, compute_sky_light(gp, rd, earth, sun));
		#else
			col = ApplyFog(col, res.x, GetSky(rd, true));
		#endif
		//		DoLighting(col, pos, nor, rd, res.x);

	} else {
		#ifdef MIE_RAYLEIGH_SKY
			vec3 gp = vec3(ro.x, earth.rs + 3500.0, ro.z);
			col = compute_sky_light(gp, rd, earth, sun);
		#else
			col = GetSky(rd, true);
		#endif
		col = ApplyFog( col, res.x, col);
	}
	
	return vec3(clamp(col,0.,1.) );
}


//*********************************************************************************
//    +----------------------------------------------------------------+
//    |   Position of the sun in the sky at a given location and time  |
// +----------------------------------------------------------------+
// Based on LOW-Precision formulae for planetary positions by T.C. Van Flandern and H-F. Pulkkinen
// http://articles.adsabs.harvard.edu/cgi-bin/nph-iarticle_query?1979ApJS...41..391V&defaultprint=YES&filetype=.pdf

float julianDay2000(in int yr, in int mn, in int day, in int hr, in int m, in int s) {
	int im = (mn-14)/12, 
		ijulian = day - 32075 + 1461*(yr+4800+im)/4 + 367*(mn-2-im*12)/12 - 3*((yr+4900+im)/100)/4;
	float f = float(ijulian)-2451545.;
	return f - 0.5 + float(hr)/24. + float(m)/1440. + float(s)/86400.;
}

float julianDay2000(in float unixTimeMs) {
	return (unixTimeMs/86400.) - 10957.5;// = + 2440587.5-2451545; 
}

vec2 SunAtTime(in float julianDay2000, in float latitude, in float longitude) {
	float zs,rightAscention, declination, sundist,
		t  = julianDay2000,	//= jd - 2451545., // nb julian days since 01/01/2000 (1 January 2000 = 2451545 Julian Days)
		t0 = t/36525.,    		 // nb julian centuries since 2000      
		t1 = t0+1.,   		 	 // nb julian centuries since 1900
		Ls = fract(.779072+.00273790931*t)*PI2, // mean longitude of sun
		Ms = fract(.993126+.0027377785 *t)*PI2, // mean anomaly of sun
		GMST = 280.46061837 + 360.98564736629*t + (0.000387933 - t0/38710000.)*t0*t0, // Greenwich Mean Sidereal Time   
// position of sun
		v = (.39785-.00021*t1)*sin(Ls)-.01*sin(Ls-Ms)+.00333*sin(Ls+Ms),
		u = 1.-.03349*cos(Ms)-.00014*cos(2.*Ls)+.00008*cos(Ls),
		w = -.0001-.04129 * sin(2.*Ls)+(.03211-.00008*t1)*sin(Ms)
			+.00104*sin(2.*Ls-Ms)-.00035*sin(2.*Ls+Ms);
// calcul distance of sun
	sundist = 1.00021*sqrt(u)*AU;
// calcul right ascention
	zs = w / sqrt(u-v*v);
	rightAscention = Ls + atan(zs/sqrt(1.-zs*zs));
// calcul declination
	zs = v / sqrt(u);
	declination = atan(zs/sqrt(1.-zs*zs));
// position relative to geographic location
	float
		sin_dec = sin(declination),   cos_dec = cos(declination),
		sin_lat = sin(TORAD*latitude),cos_lat = cos(TORAD*latitude),
		lmst = mod((GMST + longitude)/15., 24.);
	if (lmst<0.) lmst += 24.;
	lmst = TORAD*lmst*15.;
	float
		ha = lmst - rightAscention,       
		elevation = asin(sin_lat * sin_dec + cos_lat * cos_dec * cos(ha)),
		azimuth   = acos((sin_dec - (sin_lat*sin(elevation))) / (cos_lat*cos(elevation)));
	return vec2(sin(ha)>0.? azimuth:PI2-azimuth, elevation);
}

vec3 getSunVector(in float jd, in float latitude, in float longitude) {
	vec2 ae = SunAtTime(jd, latitude, longitude);
	// X = north, Y = top
	return normalize(vec3(-cos(ae.x)*cos(ae.y), sin(ae.y), sin(ae.x)*cos(ae.y)));
}

void mainImage( out vec4 fragColor, in vec2 fragCoord ) {
     rmx =  rotAA(normalize(vec3(.2174,1,.02174)),1.62729)*1.35;
	 mrot = rotAA(normalize(vec3(.1,.95,.02)), 2.);

// Picture params -------------------------------------------------------------------------
	vec2 q = fragCoord.xy/iResolution.xy;
    vec2 p = -1.+2.*q;
	p.x *= iResolution.x/iResolution.y;
    vec2 mo = iMouse.xy/iResolution.xy;

// - Camera	-------------------------------------------------------------------------------
    float a0=1.5+iTime/12., r=2.5;
    vec3 
        ro = vec3(-9.+r*cos(a0+PI2*mo.x), 
				  max(.75,1.5+.6*mo.y),
				  5.5+r*sin(a0+PI2*mo.x)),
	    ta = vec3(1.6, 1.9, -3.5 ),
        // Camera tx
        cw = normalize(ta-ro),
	    cu = cross(cw,vec3(0.,1.,0.)),
	    cv = cross(cu,cw),
	    rd = normalize(p.x*cu + p.y*cv + 2.5*cw);
    
// - Time / Season / Position of sun according to location and time ----------------------- 
#ifdef BENJAMIN_BUTTON
    jGlobalTime = 20000.-iTime; // For Benjamin button tree, inverse the sign !
#else
	jGlobalTime = 1.+iTime*.75;		
#endif
	life =.5+mod(jGlobalTime,68.)/24.;
	season = mod(jGlobalTime, 4.)/4.;
	float 
        day = floor(season*365.)+100.,
	    jd2000 = julianDay2000((day*24.+UTC_HOUR)*3600.);
	
	vec2 ae = SunAtTime(jd2000, LATITUDE, LONGITUDE);
	// X = north, Y = top
	sunLight = normalize(vec3(-cos(ae.x)*cos(ae.y), sin(ae.y), sin(ae.x)*cos(ae.y)));

#ifdef MIE_RAYLEIGH_SKY
	sun.azi = ae.x;
	sun.alt = ae.y; 
#endif
	
	if (sunLight.y < .12) {
    	sunColour = mix(vec3(2.,.4,.2), SUN_COLOR, smoothstep(0.,1., sunLight.y*8.));
	} else {
		sunColour = SUN_COLOR;
	}
	
// - Scene parameters according to season --------------------------------------------------
	treeStep = clamp(life*life,0.,1.);
	frc = fract(treeStep*10.);
	bEnd = int(treeStep*10.);
	f2 = sin(season*(3.14/.8)); /*- .33*cos(a*3.) + .2*cos(a*5.)*/;
	f2 = treeStep*treeStep*clamp(2.*f2,0.,1.);
	f2 = mix(f2, 0., clamp(2.*(life-OLD_TREE),0.,1.));
#ifdef NO_LEAF
	fEnd = fStart = 20;
#else
	fStart = (season>.8||life<.6||life>1.5*OLD_TREE) ? 20 : bEnd+1;//bEnd>6? bEnd+2:8; //1+int(3.*(sin(season*(3.14/.8))));
	fEnd =  fStart + (bEnd<8 ? 1:3);
#endif
	thinBranch=.018;
	rBranch = mix(.01,.07,treeStep);
	clpOld = clamp(life-OLD_TREE,0.,1.);
	clpVeryOld = clamp(life-VERY_OLD_TREE,0.,1.);
    
// - Render ------------------------------------------------------------------------------
    vec3 col = render(ro, rd);
    
// - Post prod ---------------------------------------------------------------------------    
	col = (sqrt(col)
			    + sunColour*mix(0., .051, sunLight.y<.05 ? 1.-sunLight.y*20. : 0.) // extra light at sunshine
		)
	    * mix(1.2, 5., 3.*clpVeryOld) // I see the light !
	    * pow(abs(16.*q.x*q.y*(1.-q.x)*(1.-q.y)), mix(.2, .25, clamp(life-VERY_OLD_TREE-.3,0.,1.))); // vigneting
        
    fragColor=vec4( col, 1.);
}
