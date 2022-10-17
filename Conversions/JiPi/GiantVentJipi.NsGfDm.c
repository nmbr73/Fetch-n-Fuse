
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image 'Texture: Organic 1' to iChannel1
// Connect Image 'Texture: Rusty Metal' to iChannel2
// Connect Image 'Texture: Organic 2' to iChannel0


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)


// It's time for another episode of your favorite show :
// -----------------------------------------------------
// E P I C         S P A C E         A D V E N T U R E S
//         ...with Rangiroa and the Commander !
//
// EPISODE 457 :  "Giant Ventifacts Of Calientis V"
//
// -----------------------------------------------------
//
// In the last episode, the Commander has finally found the
// Lost City Of Sandara ! But the place is dead. It's
// been abandonned for centuries. Crushed by the discovery,
// with no hope left, our hero repairs a rocket-powered
// ground-effect vehicle to cross the Great Desert of
// Calientis V : 5000 miles of scorching hot sand mixed with
// salts and sulfur. Meanwhile, our favorite robot girl
// has been captured by the Consortium ! Rangiroa, Queen
// of the Space Pirates, Lunar Lady of Tycho, is being
// brought before the evil Dr Zen for interrogation...
//
// Will our hero save her ? Time is running out. And
// the Great Desert is a very old place, full of weird
// ruins, unspeakable madness, floating temples, giant
// ventifacts, fractal mandeltraps, ghosts, and
// blood-thirsty horrors from a billion years ago...
//
// [Opening] starts !

// Technical notes :
//
// Wanted to make "mesas". Ended up doing strange looking
// ventifacts. Then added space art items. It all started
// coming together.
// "I like it when no plan comes together..."
// I honestly don't now what to say except that this is
// more "painting" than real actual "coding", and that I
// enjoy this process tremendously.
// The more time goes, the more I understand why Iq opened
// that can of worms a long time ago. So thank you for that.
//
// Read the real adventures of Rangiroa and the Commander
// here :

// https://baselunaire.fr/?page_id=554

// 18 episodes already. The concept : a fake lunar radio show
// around 2035 that presents and promotes real Demoscene musics.
// Why ? Well because the Scene is great, and Scene musicians
// are the best, and we should talk about it more often,
// THAT'S WHY !

// The music for this shader is "A Defender Rises" by Skaven252.
// You can download or listen to it on Soundcloud here :
// https://soundcloud.com/skaven252/a-defender-rises
// This guy rules. Seriously.

// Feel free to use this shader to illustrate your latest scifi story
// online, make a video for a music you just composed, or anything.
// Just give proper credit, and provide a link to this page.
// Creative Commons 3.0f, etc...

__DEVICE__ mat2 r2d( float a ){ float c = _cosf(a), s = _sinf(a); return to_mat2( c, s, -s, c ); }
__DEVICE__ float noise(float2 st) { return fract( _sinf( dot( swi2(st,x,y), to_float2(12.9898f,78.9f)))*43758.5453123f ); }

#define TimeVar 1.5f*iTime // Let you easily tweak the global speed

// Basic Geometry Functions.

__DEVICE__ float sdCircle(in float2 p, float radius, float2 pos, float prec)
{
      return smoothstep(0.0f,prec,radius - length(pos-p));
}

// This belongs to Iq...
__DEVICE__ float sdTriangle( in float2 p, in float2 p0, in float2 p1, in float2 p2 )
{
      float2 e0 = p1-p0, e1 = p2-p1, e2 = p0-p2;
      float2 v0 = p -p0, v1 = p -p1, v2 = p -p2;
      float2 pq0 = v0 - e0*clamp( dot(v0,e0)/dot(e0,e0), 0.0f, 1.0f );
      float2 pq1 = v1 - e1*clamp( dot(v1,e1)/dot(e1,e1), 0.0f, 1.0f );
      float2 pq2 = v2 - e2*clamp( dot(v2,e2)/dot(e2,e2), 0.0f, 1.0f );
      float s = sign_f( e0.x*e2.y - e0.y*e2.x );
      float2 d = _fminf(_fminf(to_float2(dot(pq0,pq0), s*(v0.x*e0.y-v0.y*e0.x)),
                               to_float2(dot(pq1,pq1), s*(v1.x*e1.y-v1.y*e1.x))),
                               to_float2(dot(pq2,pq2), s*(v2.x*e2.y-v2.y*e2.x)));
      return -_sqrtf(d.x)*sign_f(d.y);
}

// This belongs to a nice shadertoy coder whose name I lost.
// Please tell me if you read this !
__DEVICE__ float metaDiamond(float2 p, float2 pixel, float r, float s, float iTime)
{
      float2 d = abs_f2(mul_mat2_f2(r2d(s*TimeVar) , (p-pixel)));
      return r / (d.x + d.y);
}

// That's it guys, everything else is mine, as you can
// see by the sudden drop in quality. :D

__DEVICE__ float4 drawAtmoGradient(in float2 v_p)
{
      return _mix( to_float4(0.0f,0.3f,0.7f,1.0f), to_float4(1.0f,0.8f,0.7f,1.0f), 1.0f-v_p.y + 0.7f);
}

// Ultra-super-simplified 1D noise with smooth please ?
// We don't need more, really !
__DEVICE__ float fbm(in float2 v_p)
{
      float VarX1 = 0.0f;
      float VarX2 = 0.0f;
      float VarD0 = 0.0f;
      float VarS1 = 0.0f;
      float Amplitude = 1.0f/2.0f;
      float Periode   = 2.0f;
      VarX1 = Amplitude*_floor( Periode*v_p.x);
      VarX2 = Amplitude*_floor( Periode*v_p.x + 1.0f);
      VarD0 = fract( Periode*v_p.x);
      VarS1 += _mix( noise(to_float2_s(VarX1)), noise(to_float2_s(VarX2)), smoothstep( 0.0f, 1.0f, VarD0));
      return VarS1;
}

__DEVICE__ float GetMesaMaxHeight(in float2 v_p, float iTime)
{
      float MH = 0.98
               + 0.06f*fbm(to_float2_s(5.0f*v_p.x + 0.25f*TimeVar))
               + 0.02f*fbm(to_float2_s(40.0f*v_p.x + 2.0f*TimeVar));
      float Offset = 0.0f;
      if( fbm(to_float2_s(10.0f*v_p.x + 0.5f*TimeVar)) > 0.30f )
          Offset = -0.75f*(fbm(to_float2_s(10.0f*v_p.x + 0.5f*TimeVar)) - 0.30f);
      MH += Offset;
      return MH;
}

__KERNEL__ void GiantVentJipiFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1, sampler2D iChannel2)
{

     float2 p = to_float2( (iResolution.x/iResolution.y)*(fragCoord.x - iResolution.x/2.0f) / iResolution.x,
                           fragCoord.y / iResolution.y);

     float ratio = iResolution.y/iResolution.x;

     // Making the mouse interactive for absolutely no reason
     float TiltX = -0.001f*(iMouse.x - iResolution.x/2.0f);
     float AltiY =  0.005f*(iMouse.y - iResolution.y/2.0f);

     // Propagating user-induced chaos...
     p = p * (1.2f - 0.1f*AltiY);
     p = mul_f2_mat2(p,r2d(TiltX));

     // This, gentlemen, is our World : a single vector.
     // Don't tell Elon, he's gonna freak out.
     float4 col = to_float4(0.0f,0.0f,0.0f,1.0f);

     // Here's an atmosphere so you can choke...
     col = drawAtmoGradient(p + to_float2(0.0f,0.75f));

     // For 25 TimeVars, make the screen ondulate
     // Like it's hot in the desert or something...
     // Use iTime instead of TimeVar because whatever
     // the speed of the desert, heat distorsion should stay realtime.
     if( mod_f(TimeVar,50.0f) < 25.0f ) p += to_float2((0.0005f + 0.0005f*fbm(to_float2_s(0.2f*iTime)))*_sinf(50.0f*p.y - 25.0f*iTime),0.0f);

     // Classic French cuisine : how to make croissants.
     float FD1 = sdCircle(p,0.50f,to_float2(-0.70f,1.0f),0.01f);
     float DS1 = sdCircle(p,0.57f,to_float2(-0.75f,1.0f),0.07f);
     float Croissant1 = FD1 - DS1;
     col += clamp(Croissant1,0.0f,1.0f);

     // I'm a friendly guy : I offer you another croissant !
     float FD2 = sdCircle(p,0.20f,to_float2(-0.75f,0.75f),0.01f);
     float DS2 = sdCircle(p,0.27f,to_float2(-0.79f,0.74f),0.07f);
     float Croissant2 = FD2 - DS2;
     col += 0.3f*FD2*texture(iChannel2,2.0f*mul_mat2_f2(r2d(3.5f),to_float2(p.x*ratio, p.y)) + to_float2(0.003f*TimeVar,0.0f));
     col += clamp(2.0f*Croissant2,0.0f,1.0f);

     // Okay you get a third one.
     float FD3 = sdCircle(p,0.10f,to_float2( 0.80f, 0.77f),0.01f);
     float DS3 = sdCircle(p,0.16f,to_float2( 0.83f, 0.76f),0.07f);
     float Croissant3 = FD3 - DS3;
     col += 0.3f*FD3*texture(iChannel1,1.5f*mul_mat2_f2(r2d(3.5f),p) + to_float2(0.001f*TimeVar,0.0f));
     col += clamp(2.0f*Croissant3,0.0f,1.0f);

     // Trinary Star System + Some Modulation
     float BV1 = 0.7f + 0.3f*fbm(to_float2_s(0.3f*TimeVar         ));
     float BV2 = 0.7f + 0.3f*fbm(to_float2_s(0.3f*TimeVar + 250.0f ));
     float BV3 = 0.5f + 0.5f*fbm(to_float2_s(0.3f*TimeVar + 350.0f ));

     // Star Cross (with gimbal-assist)
     p += to_float2(-0.5f,-0.9f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += metaDiamond( p, to_float2( 0.0f, 0.0f), BV1*0.020f, 0.0f, iTime);
     col += 0.5f*smoothstep(0.08f,0.0f,_fabs(p.y))*smoothstep(0.0015f,0.0f,_fabs(p.x));
     col += 0.5f*smoothstep(0.08f,0.0f,_fabs(p.x))*smoothstep(0.0015f,0.0f,_fabs(p.y));
     p = mul_f2_mat2(p,r2d(3.14159f/4.0f));
     col += 0.5f*smoothstep(0.05f,0.0f,_fabs(p.y))*smoothstep(0.0015f,0.0f,_fabs(p.x));
     col += 0.5f*smoothstep(0.05f,0.0f,_fabs(p.x))*smoothstep(0.0015f,0.0f,_fabs(p.y));
     p = mul_f2_mat2(p,r2d(-3.14159f/4.0f));
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2(-0.5f,-0.9f);

     // Medium Star
     p += to_float2(-0.30f,-1.05f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += metaDiamond( p, to_float2( 0.0f, 0.0f), BV2*0.005f, 0.0f, iTime);
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2(-0.30f,-1.05f);

     // Small Star
     p += to_float2(-0.25f,-1.08f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += metaDiamond( p, to_float2( 0.0f, 0.0f), BV3*0.002f, 0.0f, iTime);
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2(-0.25f,-1.08f);

     if( p.y < 0.5f )
     {
         // Beneath 0.5f : The Salt Flats
         col  = to_float4(0.7f,0.7f,0.6f,1.0f);
         col += 0.5f*(texture(iChannel2,to_float2( 0.50f*(p.x*ratio)/((0.50f-p.y)) + 4.0f*TimeVar, _logf(0.50f-p.y))));
     }else{
         // Above 0.5f : The mountains of "New New Mexico" (aka "Calientis V")
         col = _mix(col,
                   to_float4(0.74f,0.74f,0.9f,1.0f)*(0.5f+0.2f*texture(iChannel0,2.0f*(p + to_float2(0.02f*TimeVar,0.0f)))),
                   smoothstep(0.005f,0.0f,p.y + 0.05f*fbm(to_float2_s(2.5f*p.x + 0.05f*TimeVar)) - 0.57f));
     };

     // Moebius-like floating temple right in the middle of the desert.
     // Because existential horror can strike anytime, anywhere. :p
     // Dedicated to Arzak fans...
     // Alternative title : "Easy fake lame DIY 3D in your 2D scene : an introduction"

     float4 Color1 = to_float4(0.9f,0.9f,1.0f,1.0f) - 0.15f*to_float4_s(texture(iChannel0,to_float2(0.01f*p.x,p.y - 0.01f*_sinf(0.4f*TimeVar))).x);
     float4 Color2 = to_float4(0.7f,0.7f,1.0f,1.0f) - 0.15f*to_float4_s(texture(iChannel0,to_float2(0.01f*p.x,p.y - 0.01f*_sinf(0.4f*TimeVar))).x);
     float4 Face1;
     float4 Face2;

     // BOOM ! You didn't see anything... ... Oh shut up, Gandalf !
     if( mod_f(0.005f*TimeVar,0.08f) < 0.04f)
     {
         Face1 = Color1;
         Face2 = Color2;
     }else{
         Face1 = Color2;
         Face2 = Color1;
     };

     // The Moebius Rock floats. And sings.
     // I can hear it. My dog can hear it.
     // Why can't you ?!
     float AltitudeMoebius = 0.550f + 0.01f*_sinf(0.4f*TimeVar);

     p += to_float2(mod_f(0.03f*TimeVar,4.0f) - 2.0f,0.0f);

     // Top Pylon
     col = _mix(Face1,col,smoothstep(0.0f,0.002f,
     sdTriangle(p,
                 to_float2(-0.0200f, AltitudeMoebius + 0.04f),
                 to_float2( 0.0200f, AltitudeMoebius + 0.04f),
                 to_float2( 0.0000f, AltitudeMoebius + 0.48f) )));
     col = _mix(Face2,col,smoothstep(0.0f,0.002f,
     sdTriangle(p,
                 to_float2(-0.0200f                          , AltitudeMoebius + 0.04f),
                 to_float2( 0.0200f - mod_f(0.005f*TimeVar,0.04f), AltitudeMoebius + 0.04f),
                 to_float2( 0.0000f                          , AltitudeMoebius + 0.48f) )));

     // Bottom Tetrahedron
     col = _mix(0.9f*Face1,col,smoothstep(0.0f,0.002f,
     sdTriangle(p,
                 to_float2(-0.0200f, AltitudeMoebius + 0.03f),
                 to_float2( 0.0200f, AltitudeMoebius + 0.03f),
                 to_float2( 0.0000f, AltitudeMoebius - 0.02f) )));
     col = _mix(0.9f*Face2,col,smoothstep(0.0f,0.002f,
     sdTriangle(p,
                 to_float2(-0.0200f                          , AltitudeMoebius + 0.03f),
                 to_float2( 0.0200f - mod_f(0.005f*TimeVar,0.04f), AltitudeMoebius + 0.03f),
                 to_float2( 0.0000f                          , AltitudeMoebius - 0.02f) )));

     // Ghostly Beacons
     col += metaDiamond( p, to_float2( 0.0f,AltitudeMoebius + 0.50f), 0.001f, 0.0f, iTime);
     col += to_float4(1.0f,0.0f,0.0f,1.0f)*metaDiamond( p, to_float2( 0.0f,AltitudeMoebius + 0.52f), 0.001f, 0.0f, iTime);

     p -= to_float2(mod_f(0.03f*TimeVar,4.0f) - 2.0f,0.0f);

     if( p.y > 0.5f )
     {
         // Very strange method to make 2D "mesas". Not sure it actually makes sense.
         // The final shapes are a bit pointy, which is fine for an extraterrestrial
         // desert, I suppose. Less so for martian mesas... Ah, well. Next time, in
         // another shader (incidentally I just figured out how to do it properly).
         // Anyway, let's pretend these are "giant ventifacts".
         float Inc = 1.0f*p.x + 0.05f*TimeVar; // Unit speed of Ventifacts relative to p.x (20:1 ratio)
         float MesaMaxHeight = GetMesaMaxHeight(p, iTime);
         float MesaLine = clamp( fbm(to_float2_s(2.0f*Inc + 0.005f*fbm(to_float2_s(80.0f*p.y)))), 0.0f, MesaMaxHeight);

         // Make the Sand follow a curve that is (more or less) realistic
         // Adding octaves, usual fbm impro stuff, you know the drill...
         float SandLine = 0.480f + 0.100f*fbm(to_float2_s( 2.0f*Inc))
                                 + 0.008f*fbm(to_float2_s(20.0f*Inc))
                                 + 0.002f*fbm(to_float2_s(60.0f*Inc));

         // Basic Color + Vertically-stretched Texture + Horizontally-stretched Texture
         float4 MesaColors = to_float4(1.0f,0.8f,0.7f,1.0f);
         MesaColors += 0.5f*texture(iChannel1,to_float2(     Inc, 0.2f*p.y));
         MesaColors += 0.5f*texture(iChannel1,to_float2( 0.1f*Inc,     p.y));

         // Basic random shadows + slanted highlights...
         MesaColors -= 0.35f*smoothstep( 0.0f, 1.0f, fbm(to_float2_s(40.0f*Inc)) + fbm(to_float2_s(15.0f*p.y - 30.0f*Inc)));

         // More Shadows !
         MesaColors = MesaColors*( 0.2f + 0.8f*smoothstep( 0.0f, 0.4f, (MesaLine - SandLine)));

         // Additional shadows at mesa's base.
         float VerticalWeathering = 1.0f;
         VerticalWeathering *= (0.8f+0.2f*smoothstep(0.0f,0.02f,(p.y - 0.6f + 0.25f*fbm(to_float2_s(80.0f*Inc)))));
         MesaColors *= VerticalWeathering;

         // Outputing mesas like big giant rotten teeth on a dead dragon's jaw...
         col = _mix( col, MesaColors, smoothstep(0.007f,0.0f,p.y - MesaLine));

         // Adding highlights, because "secondary reflections", "ambient occlusion", etc
         // (haha, yeah right)
         col *= clamp(smoothstep(-0.15f,0.0f,p.y - MesaLine + 0.01f*fbm(to_float2_s(10.0f*Inc))),0.5f,1.0f);

         // Mesas shadows on the sand...
         float SandShadows = 0.0f;
         // If we're in the shadow of a mesa, SandLine altitude should decrease (...feeling of volume)
         if( SandLine < MesaLine ) SandLine = SandLine - 0.2f*(MesaLine - SandLine);
         // Defining SandColors. Adding some y-stretched texture to simulate local sandslides.
         float4 SandColors = 0.80f*to_float4(0.3f,0.2f,0.2f,1.0f)
                           + 0.20f*texture(iChannel0,to_float2(2.0f*Inc,0.1f*p.y + 0.0f));

         // If we are in the shadow of a mesa
         if( SandLine < MesaLine)
         {
             // on-the-fly logic, probably false, but
             // just right enough to be useful.
             // "Paint-coding", guys...
             if( p.y > SandLine - (MesaLine - SandLine) )
             {
                 SandShadows = 0.7f;
             }else{
                 SandShadows = 1.0f;
             };
         }else{
             SandShadows = 1.0f;
         };

         // Outputing shaded sand dune, "MY DUNE !" haha
         col = _mix(col,SandShadows*SandColors,smoothstep(0.0025f,0.0f,p.y - SandLine));
    };

     float2  ConsortiumShipPos = to_float2( 2.0f-mod_f(0.01f*TimeVar + 1.0f,4.0f), -1.2f);
     float ConsortiumShipPrec = 0.0035f;
     float4  HullColorFix = to_float4(0.5f,0.8f,1.0f,1.0f);
     float4  HullColorTop;
     float4  HullColorBottom;


     // Move ship to position !
     p += ConsortiumShipPos;
     // Zoom Zoom Zoom !
     p *= 0.75f;

     // Tweaking Ship Colors to make them just right (i.e. blend into the sky).
     HullColorTop    = HullColorFix*to_float4(0.6f,0.6f,1.0f,1.0f) + 0.2f*to_float4_s(texture(iChannel0,to_float2(2.0f*p.x,0.1f*p.y)).x);
     HullColorTop *= 1.2f;
     // Tweaking Ship Colors.
     HullColorBottom = HullColorFix*to_float4(0.8f,0.8f,1.0f,1.0f) + 0.4f*to_float4_s(texture(iChannel0,to_float2(0.5f*p.x,0.1f*p.y)).x);
     HullColorBottom *= 0.6f;

     // Fusion-Drive Tail visible due to reaction mass impurities (grey water from comets).
     if(p.x < 0.0f) col += smoothstep(0.12f,0.0f,_fabs(0.2f*p.x))*smoothstep(0.01f,0.0f,_fabs(p.y));

     // How to draw a spaceship in six triangles : a tutorial.

     // Forward part
     col = _mix(HullColorTop,col,smoothstep(0.0f,ConsortiumShipPrec,
     sdTriangle(p,
                 to_float2( 0.145f, 0.00f),
                 to_float2( 0.200f, 0.01f),
                 to_float2( 0.355f, 0.00f) )));
     col = _mix(HullColorBottom,col,smoothstep(0.0f,ConsortiumShipPrec,
     sdTriangle(p,
                 to_float2( 0.145f, 0.00f),
                 to_float2( 0.200f,-0.015f),
                 to_float2( 0.355f, 0.00f) )));

     // Middle Part
     col = _mix(HullColorTop,col,smoothstep(0.0f,ConsortiumShipPrec,
     sdTriangle(p,
                 to_float2( 0.000f, 0.00f),
                 to_float2( 0.005f, 0.01f),
                 to_float2( 0.150f, 0.00f) )));
     col = _mix(HullColorBottom,col,smoothstep(0.0f,ConsortiumShipPrec,
     sdTriangle(p,
                 to_float2( 0.000f, 0.00f),
                 to_float2( 0.005f,-0.01f),
                 to_float2( 0.150f, 0.00f) )));

     // Back Part
     col = _mix(HullColorTop,col,smoothstep(0.0f,ConsortiumShipPrec,
     sdTriangle(p,
                 to_float2(-0.005f, 0.00f),
                 to_float2( 0.010f, 0.02f),
                 to_float2( 0.070f, 0.00f) )));
     col = _mix(HullColorBottom,col,smoothstep(0.0f,ConsortiumShipPrec,
     sdTriangle(p,
                 to_float2(-0.005f, 0.00f),
                 to_float2( 0.010f,-0.02f),
                 to_float2( 0.070f, 0.00f) )));

     // End tutorial. You're welcome. :D

     // Fusion-Drive Glow (...keep this end at a distance)
     p += to_float2( 0.005f,-0.002f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += metaDiamond(p,to_float2(0.0f,0.0f), 0.010f, 0.0f, iTime);
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2( 0.005f,-0.002f);
     // De-Zoom
     p *= 1.0f/0.75f;
     // Back to normal p.
     p -= ConsortiumShipPos;

     // Le Hovercraft

     float2  HovercraftPos   = to_float2(0.05f  - 0.3f*fbm(to_float2_s(0.1f*TimeVar)),-0.35f);
     float HovercraftTrail = 0.335f;
     float HovercraftBoost = 0.0f;

     // Shadow
     col = _mix(to_float4_s(0.5f),col,smoothstep(0.0f,0.001f,
                sdTriangle(p + HovercraftPos + to_float2( 0.0f, 0.01f ),
                           to_float2(-0.01f+ 0.001f*_sinf(2.0f*TimeVar), 0.0050f),
                           to_float2(-0.01f+ 0.001f*_sinf(2.0f*TimeVar),-0.0050f),
                           to_float2( 0.04f- 0.001f*_sinf(2.0f*TimeVar), 0.000f) )));

     // Lifting Body
     col = _mix(to_float4_s(0.5f),col,smoothstep(0.0f,0.001f,
                sdTriangle(p + HovercraftPos + to_float2( 0.0f,- 0.001f*_sinf(2.0f*TimeVar) ),
                           to_float2(-0.01f, 0.0050f),
                           to_float2(-0.01f,-0.0050f),
                           to_float2( 0.04f, 0.000f) )));

     // Vertical Tail
     col = _mix(to_float4_s(0.4f),col,smoothstep(0.0f,0.001f,
                sdTriangle(p + HovercraftPos + to_float2( 0.0f,- 0.001f*_sinf(2.0f*TimeVar) ),
                           to_float2(-0.010f, 0.0050f),
                           to_float2(-0.015f, 0.015f),
                           to_float2( 0.000f, 0.0050f) )));

     // Cockpit Canopy
     col = _mix(to_float4_s(0.2f),col,smoothstep(0.0f,0.001f,
                sdTriangle(p + HovercraftPos + to_float2( 0.0f,- 0.001f*_sinf(2.0f*TimeVar) ),
                           to_float2( 0.000f, 0.0050f),
                           to_float2( 0.005f, 0.0000f),
                           to_float2( 0.025f, 0.0010f) )));

     // Dust Trail
     if( p.x < -0.05f  + 0.3f*fbm(to_float2_s(0.1f*TimeVar)) - 0.01f )
         col += 0.1f*smoothstep(0.0f,0.01f,p.y - HovercraftTrail)
                    *smoothstep(0.035f,0.0f, p.y -0.015f*_fabs(5.0f*(p.x + HovercraftPos.x))
                                                        *fbm(to_float2_s(10.0f*(p.x + HovercraftPos.x) + 10.0f*TimeVar)) - 0.98f*HovercraftTrail);
     // Very lame yet mostly accurate thruster simulation.
     // This shader is a disgrace to mathematics, exhibit 41 :
     if( fbm(to_float2_s(0.1f*(TimeVar + 0.1f))) - fbm(to_float2_s(0.1f*(TimeVar))) > 0.005f)
     {
        // Haha rocket goes BRRRRRR !
        HovercraftBoost = 0.005f;
     }else{
        // Puff Puff Puff Puff Puff
        HovercraftBoost = _fabs(0.003f*_sinf(20.0f*TimeVar));
     };

     // Rocket Blast
     col += to_float4(1.0f,0.5f,0.5f,1.0f)*metaDiamond(p + HovercraftPos + to_float2(  0.015f,- 0.0015f*_sinf(2.0f*TimeVar)),to_float2( 0.0f,0.0f), HovercraftBoost, 10.0f, iTime);

     // A bit of dust in the air...
     if( p.y > 0.5f) col += 0.25f*smoothstep(0.25f,0.0f,p.y - 0.1f*fbm(to_float2_s(2.0f*p.x + 1.0f*TimeVar)) - 0.5f);

     // Make a haze just above the ground in the distance.
     col += 0.2f*smoothstep(0.01f,0.0f,_fabs(p.y-0.5f));

     // For the last 25 TimeVars of a 50 TimeVars cycle...
     if( mod_f(TimeVar,50.0f) > 25.0f )
     {
         // Draw some Nasa camera crosses to look cool and realistic (hahahahaha)
         if(mod_f(fragCoord.y + 200.0f,400.0f) > 399.0f) if(mod_f(fragCoord.x + 50.0f + 200.0f,400.0f) < 100.0f) col = to_float4(0.2f,0.2f,0.2f,1.0f);
         if(mod_f(fragCoord.x + 200.0f,400.0f) > 399.0f) if(mod_f(fragCoord.y + 50.0f + 200.0f,400.0f) < 100.0f) col = to_float4(0.2f,0.2f,0.2f,1.0f);
     };

     // Lensflares ! Lensflares everywhere !
     
     // Big Star
     // Let's compute Mesa's height at the Big Star's x-coordinate.
     float NewMesaLine = clamp( fbm(to_float2_s(2.0f*(0.5f + 0.05f*TimeVar)) + 0.005f*fbm(to_float2_s(80.0f*0.9f))),0.0f,GetMesaMaxHeight(to_float2(0.5f,0.0f), iTime));

     p += to_float2(-0.5f,-0.9f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += 0.2
               // If the mesa's top is above the Big Star, remove lensflare.
               *smoothstep(0.0f,0.01f,0.9f-NewMesaLine)
               // If the Moebius Rock clips the Big Star, remove lensflare.
               *smoothstep(0.0f,0.01f,_fabs(mod_f(0.03f*TimeVar,4.0f) - 2.0f + 0.5f))
               // Basic Hand-Made Linear 2D Lensflare
               // the best kind, like granma used to...
               *smoothstep(0.03f,0.0f,_fabs(p.y))
               *smoothstep(2.00f,0.0f,_fabs(p.x));
     // Circle around the Big Star. Not exactly JWST-worthy, I know.
     // Look, I'm just doing my best, okay ?! :D
     col += 0.1f*smoothstep(0.0125f,0.0f,_fabs(sdCircle(p,0.05f,to_float2_s(0.0f),0.07f) - 0.0125f))
               *smoothstep(0.0f,0.01f,0.9f-NewMesaLine)
               *smoothstep(0.0f,0.01f,_fabs(mod_f(0.03f*TimeVar,4.0f) - 2.0f + 0.5f));
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2(-0.5f,-0.9f);

     // Medium Star
     p += to_float2(-0.30f,-1.05f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += 0.1f*smoothstep(0.01f,0.0f,_fabs(p.y))
               *smoothstep(0.50f,0.0f,_fabs(p.x));
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2(-0.30f,-1.05f);

     // Small Star
     p += to_float2(-0.25f,-1.08f);
     p = mul_f2_mat2(p,r2d(-TiltX));
     col += 0.1f*smoothstep(0.01f,0.0f,_fabs(p.y))
               *smoothstep(0.25f,0.0f,_fabs(p.x));
     p = mul_f2_mat2(p,r2d( TiltX));
     p -= to_float2(-0.25f,-1.08f);

     // Every 25 TimeVars, pretend like you're watching through an Active SunShade
     // that cancels heat distorsion through some kind of adaptative optics magic.
     // Hey, it's the future after all.
     if(mod_f(TimeVar,50.0f) > 25.0f) col *= to_float4(1.0f,0.8f,0.7f,1.0f);

     // HO MY GOD !
     fragColor = col;

  SetFragmentShaderComputedColor(fragColor);
}