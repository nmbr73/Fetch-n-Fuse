

          //    Any text before the first marker ist ignored.       //
          //    Do not delete or change these markers!              //
          //    You may want to do some minimal changes in the      //
          //    following code sections to better support the       //
          //    pattern matching that does the preliminaty GLSL     //
          //    to DCTL conversion.                                 //


// >>> ___ GLSL:[Image] ____________________________________________________________________ <<<
// TomF's Displacement map on rast mesh with video for displacement map
// (https://www.shadertoy.com/view/wsSBzw) 

#define BACKFACE_CULL false
#define TEXTURED true
#define PHASES false


struct Vertex
{
    vec4 Pos;
    vec3 Uvh;
};
    

// Test a position against a triangle and return
// the perspective-correct barycentric coordinates in the triangle
// Note the z value in the vertex is ignored, it's the w that matters.
vec2 BaryTri3D ( vec2 pos, Vertex v1, Vertex v2, Vertex v3 )
{
    vec2 posv1 = pos - vec2(v1.Pos);
    
    vec2 v21 = vec2(v2.Pos) - vec2(v1.Pos);
    vec2 v31 = vec2(v3.Pos) - vec2(v1.Pos);
    
    float scale = v21.x * v31.y - v21.y * v31.x;
    if ( BACKFACE_CULL && ( scale < 0.0 ) )
    {
        return vec2 ( -1.0, -1.0 );
    }

    float rscale = 1.0 / scale;
    float baryi = ( posv1.x * v31.y - posv1.y * v31.x ) * rscale;
    float baryj = ( posv1.x * v21.y - posv1.y * v21.x ) * -rscale;
    
    // Now interpolate the canonical coordinates (0,0,1,v1.w), (1,0,1,v2.w) and (0,1,1,v3.w)
    // with perspective correction
    // So we project all three by their respective w:
    // (0,0,v1.w) -> (0,     0,     1/v1.w)
    // (1,0,v2.w) -> (1/v2.w,0,     1/v2.w)
    // (0,1,v3.w) -> (0,     1/v3.w,1/v3.w)
    // Then interpolate those values linearly to produce (nx,ny,nw),
    // then divide by nw again.
    vec3 recipw = vec3 ( 1.0/v1.Pos.w, 1.0/v2.Pos.w, 1.0/v3.Pos.w );
    
    float baryk = 1.0 - baryi - baryj;
    float newi = recipw.y * baryi;
    float newj = recipw.z * baryj;
    //float neww = recipw.x * baryk + recipw.y * baryi + recipw.z * baryj;
    float neww = recipw.x * baryk + newi + newj;
    
    // ...and project back.
    float rneww = 1.0/neww;
    float perspi = newi * rneww;
    float perspj = newj * rneww;
        
    return vec2 ( perspi, perspj );
}

const int GridW = 4;
const int GridH = 4;
const int NumVerts = GridH*GridW*2;
const int NumTris = (GridH-1)*(GridW-1)*2*2 + ((GridH-1)*2+(GridW-1)*2)*2;

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // NDC (-1 to +1)
    vec2 uv = -1.0 + 2.0f * (fragCoord/iResolution.xy);
    
    int Phase = 0;
    if(PHASES)
    {
    	Phase = (int(floor(0.2*(iTime-1.0*fragCoord.x/iResolution.x))))%3;
    }
    
    
    float wobble = 0.0;//float(iTime);
    
    // Create the mesh. This would of course be done offline.
    Vertex Verts[NumVerts];
    for ( int w = 0; w < GridW; w++ )
    {
        for ( int h = 0; h < GridH; h++ )
        {
            Vertex Vert0, Vert1;
            Vert0.Pos.x = float(w - GridW/2);
            Vert0.Pos.y = float(h - GridH/2);
            float wf = -2.0 + float(w);
            float hf = -3.0 + float(h);
            float d = sqrt(wf*wf + hf*hf);
            Vert0.Pos.z = 0.3 * cos (d * 0.8 + wobble) - 1.0;
            Vert0.Pos.w = 1.0f;
            Vert0.Uvh.x = float(w) / float(GridW-1);
            Vert0.Uvh.y = float(h) / float(GridH-1);
            Vert0.Uvh.z = 0.0;
            
            Vert1.Pos = Vert0.Pos;
            Vert1.Pos.z += 1.0;
            Vert1.Uvh = Vert0.Uvh;
            Vert1.Uvh.z = 1.0;
            
            int Index = (w*GridH+h)*2;
            Verts[Index+0] = Vert0;
            Verts[Index+1] = Vert1;
        }
    }

    int Indices0[NumTris];
    int Indices1[NumTris];
    int Indices2[NumTris];
    for ( int w = 0; w < GridW-1; w++ )
    {
        for ( int h = 0; h < GridH-1; h++ )
        {
            int VertIndex = (w*GridH+h)*2;
            int TriIndex = (w + (GridH-1)*h) * 2 * 2;
            Indices0[TriIndex+0] = VertIndex  ;
            Indices1[TriIndex+0] = VertIndex  +GridH*2;
			Indices2[TriIndex+0] = VertIndex  +2;
            Indices0[TriIndex+1] = VertIndex+1;
            Indices2[TriIndex+1] = VertIndex+1+GridH*2;
			Indices1[TriIndex+1] = VertIndex+1+2;
            
            Indices2[TriIndex+2] = VertIndex  +GridH*2;
            Indices0[TriIndex+2] = VertIndex  +GridH*2+2;
			Indices1[TriIndex+2] = VertIndex  +2;
            Indices1[TriIndex+3] = VertIndex+1+GridH*2;
            Indices0[TriIndex+3] = VertIndex+1+GridH*2+2;
			Indices2[TriIndex+3] = VertIndex+1+2;
        }
    }
    // Now do the edges.
    int CurTri  = (GridH-1)*(GridW-1)*2*2;
    for ( int w = 0; w < GridW-1; w++ )
    {
        int VertIndex = (w*GridH)*2;
        Indices0[CurTri+0] = VertIndex;
        Indices1[CurTri+0] = VertIndex+1;
		Indices2[CurTri+0] = VertIndex+GridH*2;
        Indices0[CurTri+1] = VertIndex+1;
        Indices1[CurTri+1] = VertIndex+GridH*2;
		Indices2[CurTri+1] = VertIndex+GridH*2+1;

        VertIndex = (w*GridH+GridH-1)*2;
        Indices0[CurTri+2] = VertIndex;
        Indices1[CurTri+2] = VertIndex+1;
		Indices2[CurTri+2] = VertIndex+GridH*2;
        Indices0[CurTri+3] = VertIndex+1;
        Indices1[CurTri+3] = VertIndex+GridH*2;
		Indices2[CurTri+3] = VertIndex+GridH*2+1;
        
        CurTri += 4;
    }
    for ( int h = 0; h < GridH-1; h++ )
    {
        int VertIndex = (h)*2;
        Indices0[CurTri+0] = VertIndex;
        Indices1[CurTri+0] = VertIndex+1;
		Indices2[CurTri+0] = VertIndex+2;
        Indices0[CurTri+1] = VertIndex+1;
        Indices1[CurTri+1] = VertIndex+2;
		Indices2[CurTri+1] = VertIndex+2+1;

        VertIndex = (h+GridH*(GridW-1))*2;
        Indices0[CurTri+2] = VertIndex;
        Indices1[CurTri+2] = VertIndex+1;
		Indices2[CurTri+2] = VertIndex+2;
        Indices0[CurTri+3] = VertIndex+1;
        Indices1[CurTri+3] = VertIndex+2;
		Indices2[CurTri+3] = VertIndex+2+1;
        
        CurTri += 4;
    }
   
    
    float hfov = 0.6;
    float vfov = hfov * iResolution.y / iResolution.x;
    float zfar = 10.0f;
    float znear = 1.0f;
    float q = zfar/(zfar-znear);
    
    mat4 ProjMat;
    ProjMat[0] = vec4 ( 1.0/hfov, 0.0f, 0.0f, 0.5f );
    ProjMat[1] = vec4 ( 0.0f, 1.0/vfov, 0.0f, 0.5f );
    ProjMat[2] = vec4 ( 0.0f, 0.0f, q, 1.0f );
    ProjMat[3] = vec4 ( 0.0f, 0.0f, -q*znear, 0.0f );
    
    
    mat4 TotalMat;
    mat4 ObjMat1;
    mat4 ObjMat2;
    
    // Mouse -> eye
    float a1 = iMouse.x * 0.01 + 3.0;
    float a2 = iMouse.y * 0.01 + 2.8;
    float zdist = 4.0;
    
    ObjMat1[0] = vec4 ( cos(a1),  sin(a1), 0.0, -0.8 );
    ObjMat1[1] = vec4 ( sin(a1), -cos(a1), 0.0, 0.8 );
    ObjMat1[2] = vec4 ( 0.0,          0.0, 1.0, 0.0 );
    ObjMat1[3] = vec4 ( 0.0,          0.0, 0.0, 1.0 );
    ObjMat2[0] = vec4 ( 1.0, 0.0,          0.0, 0.0 );
    ObjMat2[1] = vec4 ( 0.0, cos(a2),  sin(a2), 0.0 );
    ObjMat2[2] = vec4 ( 0.0, sin(a2), -cos(a2), zdist );
    ObjMat2[3] = vec4 ( 0.0, 0.0,          0.0, 1.0 );
    
    TotalMat = ObjMat1 * ObjMat2;
    TotalMat = TotalMat * ProjMat;

    // Background colour
    vec3 col;
    //col.xy = uv.xy;
    col.x = 0.7;
    col.y = 0.7;
    col.z = 0.7;

    Vertex ScreenVert[NumVerts];
    for ( int VertNum = 0; VertNum < NumVerts; VertNum++ )
    {
        vec4 Pos = Verts[VertNum].Pos;
        
        vec4 ScrPos = Pos * TotalMat;
        float rw = 1.0/ScrPos.w;
        ScrPos.x *= rw;
        ScrPos.y *= rw;
        ScrPos.z *= rw;
        
        ScreenVert[VertNum].Pos = ScrPos;
        ScreenVert[VertNum].Uvh = Verts[VertNum].Uvh;
    }
    
    float NearestZ = 10000000.0f;
    float FarthestZ = -10000000.0f;
    vec3 NearUvh;
    vec3 FarUvh;
    
    for ( int TriNum = 0; TriNum < NumTris; TriNum++ )
    {
        Vertex v1 = ScreenVert[Indices0[TriNum]];
        Vertex v2 = ScreenVert[Indices1[TriNum]];
        Vertex v3 = ScreenVert[Indices2[TriNum]];
        vec3 bary;
        bary.xy = BaryTri3D ( uv, v1, v2, v3 );
        bary.z = 1.0 - bary.x - bary.y;

        vec3 uvh = bary.z * v1.Uvh + bary.x * v2.Uvh + bary.y * v3.Uvh;
        
        if ( ( bary.x >= 0.0 ) &&
             ( bary.y >= 0.0 ) &&
             ( bary.z >= 0.0 ) &&
             ( uvh.x + uvh.y + uvh.z > -1000.0 ) ) // see above
        {
            // Interpolate Z
            // Note this is linear Z, not the strange Z that most rasteriser use
            // In this case, that's fine.
            float Z = bary.z * v1.Pos.z + bary.x * v2.Pos.z + bary.y * v3.Pos.z;
            if ( NearestZ >= Z )
            {
                NearestZ = Z;
                NearUvh = uvh;
                
                if ( Phase == 1 )
                {
                    // Wireframe!
                    float maxbary = min ( bary.x, min ( bary.y, bary.z ) );
                    maxbary = maxbary * 100.0;
                    if ( maxbary < 1.0 )
                    {
                        col.x = 1.0 - maxbary;
                    }
                }  
            }
            
            if ( FarthestZ <= Z )
            {
                FarthestZ = Z;
                FarUvh = uvh;
            }
            
        }        
    }
    
    if ( NearestZ < FarthestZ )
    {
        if ( Phase == 0 )
        {
            // Now "trace" from NearUV to FarUV and
            // see where it intersects the heightfield.
            // Ideally the heightfield would store an SDF rather than a simple height,
            // so you could step through it quickly, but I don't have that, so this is
            // just a brute-force step, although it is at leats proportional to the
            // number of texels to traverse.
            float Dist = length(NearUvh.xy - FarUvh.xy);
            int NumSteps = int(Dist*64.0+5.0); // texture size?
            vec3 LastUvh = NearUvh;
            float LastTexH = NearUvh.z;
            for ( int StepNum = 0; StepNum <= NumSteps; StepNum++ )
            {
                float Lerp = float(StepNum)/float(NumSteps);
                vec3 uvh = NearUvh + (FarUvh-NearUvh)*Lerp;
                float height = 1.0 - texture ( iChannel0, uvh.xy ).x;
                if ( uvh.z >= height )
                {
                    // We crossed from one side to the other,
                    // so interpolate to the intersection.
                    float h0 = LastUvh.z - LastTexH;
                    float h1 = uvh.z - height;
                    float ThisLerp = h0 / (h0-h1);
                    vec3 uvh2 = LastUvh + (uvh-LastUvh)*ThisLerp;
                    vec4 tex2 = texture ( iChannel0, uvh2.xy ).xxxx * texture ( iChannel1, uvh2.xy );
                    col.xyz = tex2.xyz;
                    break;
                }
                else
                {
                    LastUvh = uvh;
                    LastTexH = height;
                }
            }
        }
        else if ( Phase == 1 )
        {
            //col.xyz = NearUvh.xyz;
        }
        else if ( Phase == 2 )
        {
            col.xyz = texture ( iChannel0, NearUvh.xy ).xxx;
        }
    }
    
    // Output to screen
    fragColor = vec4(col,1.0);
}
