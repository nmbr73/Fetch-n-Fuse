

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
#define DTR 0.01745329
#define rot(a) mat2(cos(a),sin(a),-sin(a),cos(a))

vec2 uv;
vec3 cp,cn,cr,ro,rd,ss,oc,cc,gl,vb;
vec4 fc;
float tt,cd,sd,io,oa,td;
int es=0,ec;

float bx(vec3 p,vec3 s){vec3 q=abs(p)-s;return min(max(q.x,max(q.y,q.z)),0.)+length(max(q,0.));}
float cy(vec3 p, vec2 s){p.y+=s.x/2.;p.y-=clamp(p.y,0.,s.x);return length(p)-s.y;}


vec3 lattice(vec3 p, int iter, float an)
{
		for(int i = 0; i < iter; i++)
		{
			p.xz *= rot(an*DTR);
			p.xy *= rot(an*DTR);
			p=abs(p)-1.;
			p.yz *= rot(-an*DTR);
		}
		return p;
}


float mp(vec3 p)
{
//now with mouse control
if(iMouse.z>0.){
    p.yz*=rot(2.0*(iMouse.y/iResolution.y-0.5));
    p.zx*=rot(-7.0*(iMouse.x/iResolution.x-0.5));
}
		vec3 pp=p;
		
		p.xz*=rot(tt*0.1);
		p.xy*=rot(tt*0.1);
	
		vec3 pl1 = lattice(p, 4, 25.);
		vec3 pl2 = lattice(p, 4, 45.);
		vec3 pl3 = lattice(p, 4, 70.);
	
		float b1=bx(pl1, vec3(3.,0.15,0.15));
		float b2=cy(pl2, vec2(5.,0.5));
		float b3=bx(pl3, vec3(3.,0.15,0.15));
		
		float an = pow(sin(tt*3.)*0.5+0.5,10.);
		float an2 = 1.-pow(sin(tt*3.+3.14)*0.5+0.5,10.);
		sd = mix(b1,b3,cos(tt)*0.5+0.5);
		sd = mix(b2,sd, an+an2);
	
		if(sd>0.01) gl += exp(-sd*0.01) * normalize(p*p) * 0.015;
	
		sd=abs(sd)-0.001;

		if(sd<0.001)
		{
			oc=vec3(0.);
			io=1.05;
			oa=0.;
			ss=vec3(0);
		  vb=vec3(0.,8,2.5);
			ec=2;	
		}
		return sd;
}

void tr(){vb.x=0.;cd=0.;for(float i=0.;i<512.;i++){mp(ro+rd*cd);cd+=sd;td+=sd;if(sd<0.0001||cd>128.)break;}}
void nm(){mat3 k=mat3(cp,cp,cp)-mat3(.001);cn=normalize(mp(cp)-vec3(mp(k[0]),mp(k[1]),mp(k[2])));}

void px()
{
  cc=vec3(0.,0.,0.)+length(pow(rd+vec3(0,0.,0),vec3(3)))*0.1+gl;
  vec3 l=vec3(0.9,0.7,0.5);
  if(cd>128.){oa=1.;return;}
  float df=clamp(length(cn*l),0.,1.);
  vec3 fr=pow(1.-df,3.)*mix(cc,vec3(0.4),0.5);
	float sp=(1.-length(cross(cr,cn*l)))*0.2;
	float ao=min(mp(cp+cn*0.3)-0.3,0.3)*0.5;
  cc=mix((oc*(df+fr+ss)+fr+sp+ao+gl),oc,vb.x);
}

void render(vec2 frag, vec2 res, float time, out vec4 col)
{
	tt=mod(time, 260.);
  uv=vec2(frag.x/res.x,frag.y/res.y);
  uv-=0.5;uv/=vec2(res.y/res.x,1);
  ro=vec3(0,0,-15);rd=normalize(vec3(uv,1));
  
	for(int i=0;i<20;i++)
  {
		tr();cp=ro+rd*cd;
    nm();ro=cp-cn*0.01;
    cr=refract(rd,cn,i%2==0?1./io:io);
    if(length(cr)==0.&&es<=0){cr=reflect(rd,cn);es=ec;}
    if(max(es,0)%3==0&&cd<128.)rd=cr;es--;
		if(vb.x>0.&&i%2==1)oa=pow(clamp(cd/vb.y,0.,1.),vb.z);
		px();fc=fc+vec4(cc*oa,oa)*(1.-fc.a);	
		if((fc.a>=1.||cd>128.))break;
  }
  col = fc/fc.a;
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    render(fragCoord.xy,iResolution.xy,iTime,fragColor);
}