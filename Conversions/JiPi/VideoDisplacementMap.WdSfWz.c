
// ----------------------------------------------------------------------------------
// - Image                                                                          -
// ----------------------------------------------------------------------------------
// Connect Image '/media/a/3405e48f74815c7baa49133bdc835142948381fbe003ad2f12f5087715731153.ogv' to iChannel0
// Connect Image '/media/a/3405e48f74815c7baa49133bdc835142948381fbe003ad2f12f5087715731153.ogv' to iChannel1


#define texture(ch,uv) _tex2DVecN(ch, (uv).x, (uv).y, 15)

// TomF's Displacement map on rast mesh with video for displacement map
// (https://www.shadertoy.com/view/wsSBzw) 

#define BACKFACE_CULL false
#define TEXTURED true
#define PHASES false


struct Vertex
{
    float4 Pos;
    float3 Uvh;
};
    

// Test a position against a triangle and return
// the perspective-correct barycentric coordinates in the triangle
// Note the z value in the vertex is ignored, it's the w that matters.
__DEVICE__ float2 BaryTri3D ( float2 pos, Vertex v1, Vertex v2, Vertex v3 )
{
    float2 posv1 = pos - swi2(v1.Pos,x,y);

    float2 v21 = swi2(v2.Pos,x,y) - swi2(v1.Pos,x,y);
    float2 v31 = swi2(v3.Pos,x,y) - swi2(v1.Pos,x,y);
    
    float scale = v21.x * v31.y - v21.y * v31.x;
    if ( BACKFACE_CULL && ( scale < 0.0f ) )
    {
        return to_float2( -1.0f, -1.0f );
    }

    float rscale = 1.0f / scale;
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
    float3 recipw = to_float3 ( 1.0f/v1.Pos.w, 1.0f/v2.Pos.w, 1.0f/v3.Pos.w );
    
    float baryk = 1.0f - baryi - baryj;
    float newi = recipw.y * baryi;
    float newj = recipw.z * baryj;
    //float neww = recipw.x * baryk + recipw.y * baryi + recipw.z * baryj;
    float neww = recipw.x * baryk + newi + newj;
    
    // ...and project back.
    float rneww = 1.0f/neww;
    float perspi = newi * rneww;
    float perspj = newj * rneww;
        
    return to_float2 ( perspi, perspj );
}



__KERNEL__ void VideoDisplacementMapFuse(float4 fragColor, float2 fragCoord, float iTime, float2 iResolution, float4 iMouse, sampler2D iChannel0, sampler2D iChannel1)
{

    const int GridW = 4;
    const int GridH = 4;
    const int NumVerts = GridH*GridW*2;
    const int NumTris = (GridH-1)*(GridW-1)*2*2 + ((GridH-1)*2+(GridW-1)*2)*2;

    // NDC (-1 to +1)
    float2 uv = -1.0f + 2.0f * (fragCoord/iResolution);
    
    int Phase = 0;
    if(PHASES)
    {
      Phase = ((int)(_floor(0.2f*(iTime-1.0f*fragCoord.x/iResolution.x))))%3;
    }
    

    float wobble = 0.0f;//float(iTime);
    
    // Create the mesh. This would of course be done offline.
    Vertex Verts[NumVerts];
    for ( int w = 0; w < GridW; w++ )
    {
        for ( int h = 0; h < GridH; h++ )
        {
            Vertex Vert0, Vert1;
            Vert0.Pos.x = (float)(w - GridW/2);
            Vert0.Pos.y = (float)(h - GridH/2);
            float wf = -2.0f + (float)(w);
            float hf = -3.0f + (float)(h);
            float d = _sqrtf(wf*wf + hf*hf);
            Vert0.Pos.z = 0.3f * _cosf (d * 0.8f + wobble) - 1.0f;
            Vert0.Pos.w = 1.0f;
            Vert0.Uvh.x = (float)(w) / (float)(GridW-1);
            Vert0.Uvh.y = (float)(h) / (float)(GridH-1);
            Vert0.Uvh.z = 0.0f;
            
            Vert1.Pos = Vert0.Pos;
            Vert1.Pos.z += 1.0f;
            Vert1.Uvh = Vert0.Uvh;
            Vert1.Uvh.z = 1.0f;
            
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
   
    
    float hfov = 0.6f;
    float vfov = hfov * iResolution.y / iResolution.x;
    float zfar = 10.0f;
    float znear = 1.0f;
    float q = zfar/(zfar-znear);
    
    mat4 ProjMat;
    ProjMat.r0 = to_float4 ( 1.0f/hfov, 0.0f, 0.0f, 0.5f );
    ProjMat.r1 = to_float4 ( 0.0f, 1.0f/vfov, 0.0f, 0.5f );
    ProjMat.r2 = to_float4 ( 0.0f, 0.0f, q, 1.0f );
    ProjMat.r3 = to_float4 ( 0.0f, 0.0f, -q*znear, 0.0f );
    
    
    mat4 TotalMat;
    mat4 ObjMat1;
    mat4 ObjMat2;
    
    // Mouse -> eye
    float a1 = iMouse.x * 0.01f + 3.0f;
    float a2 = iMouse.y * 0.01f + 2.8f;
    float zdist = 4.0f;
    
    ObjMat1.r0 = to_float4 ( _cosf(a1),  _sinf(a1), 0.0f, -0.8f );
    ObjMat1.r1 = to_float4 ( _sinf(a1), -_cosf(a1), 0.0f, 0.8f );
    ObjMat1.r2 = to_float4 ( 0.0f,          0.0f, 1.0f, 0.0f );
    ObjMat1.r3 = to_float4 ( 0.0f,          0.0f, 0.0f, 1.0f );
    ObjMat2.r0 = to_float4 ( 1.0f, 0.0f,          0.0f, 0.0f );
    ObjMat2.r1 = to_float4 ( 0.0f, _cosf(a2),  _sinf(a2), 0.0f );
    ObjMat2.r2 = to_float4 ( 0.0f, _sinf(a2), -_cosf(a2), zdist );
    ObjMat2.r3 = to_float4 ( 0.0f, 0.0f,          0.0f, 1.0f );
    
    TotalMat = mul_mat4_mat4(ObjMat1 , ObjMat2);
    TotalMat = mul_mat4_mat4(TotalMat , ProjMat);

    // Background colour
    float3 col;
    //swi2(col,x,y) = swi2(uv,x,y);
    col.x = 0.7f;
    col.y = 0.7f;
    col.z = 0.7f;

    Vertex ScreenVert[NumVerts];
    for ( int VertNum = 0; VertNum < NumVerts; VertNum++ )
    {
        float4 Pos = Verts[VertNum].Pos;
        
        float4 ScrPos = mul_f4_mat4(Pos , TotalMat);
        float rw = 1.0f/ScrPos.w;
        ScrPos.x *= rw;
        ScrPos.y *= rw;
        ScrPos.z *= rw;
        
        ScreenVert[VertNum].Pos = ScrPos;
        ScreenVert[VertNum].Uvh = Verts[VertNum].Uvh;
    }
    
    float NearestZ = 10000000.0f;
    float FarthestZ = -10000000.0f;
    float3 NearUvh;
    float3 FarUvh;
    
    for ( int TriNum = 0; TriNum < NumTris; TriNum++ )
    {
        Vertex v1 = ScreenVert[Indices0[TriNum]];
        Vertex v2 = ScreenVert[Indices1[TriNum]];
        Vertex v3 = ScreenVert[Indices2[TriNum]];
        float3 bary;
        swi2S(bary,x,y, BaryTri3D ( uv, v1, v2, v3 ));
        bary.z = 1.0f - bary.x - bary.y;

        float3 uvh = bary.z * v1.Uvh + bary.x * v2.Uvh + bary.y * v3.Uvh;
        
        if ( ( bary.x >= 0.0f ) &&
             ( bary.y >= 0.0f ) &&
             ( bary.z >= 0.0f ) &&
             ( uvh.x + uvh.y + uvh.z > -1000.0f ) ) // see above
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
                    float maxbary = _fminf ( bary.x, _fminf ( bary.y, bary.z ) );
                    maxbary = maxbary * 100.0f;
                    if ( maxbary < 1.0f )
                    {
                        col.x = 1.0f - maxbary;
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
            float Dist = length(swi2(NearUvh,x,y) - swi2(FarUvh,x,y));
            int NumSteps = (int)(Dist*64.0f+5.0f); // texture size?
            float3 LastUvh = NearUvh;
            float LastTexH = NearUvh.z;
            for ( int StepNum = 0; StepNum <= NumSteps; StepNum++ )
            {
                float Lerp = (float)(StepNum)/(float)(NumSteps);
                float3 uvh = NearUvh + (FarUvh-NearUvh)*Lerp;
                float height = 1.0f - texture( iChannel0, swi2(uvh,x,y) ).x;
                if ( uvh.z >= height )
                {
                    // We crossed from one side to the other,
                    // so interpolate to the intersection.
                    float h0 = LastUvh.z - LastTexH;
                    float h1 = uvh.z - height;
                    float ThisLerp = h0 / (h0-h1);
                    float3 uvh2 = LastUvh + (uvh-LastUvh)*ThisLerp;
                    float4 tex2 = swi4(texture( iChannel0, swi2(uvh2,x,y) ),x,x,x,x) * texture( iChannel1, swi2(uvh2,x,y) );
                    //swi3(col,x,y,z) = swi3(tex2,x,y,z);
                    col.x = tex2.x;
                    col.y = tex2.y;
                    col.z = tex2.z;
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
            //swi3(col,x,y,z) = swi3(NearUvh,x,y,z);
        }
        else if ( Phase == 2 )
        {
            //swi3(col,x,y,z) = texture ( iChannel0, swi2(NearUvh,x,y) ).xxx;
            col.x = texture ( iChannel0, swi2(NearUvh,x,y) ).x;
            col.y = texture ( iChannel0, swi2(NearUvh,x,y) ).x;
            col.z = texture ( iChannel0, swi2(NearUvh,x,y) ).x;
        }
        
    col = to_float3(1.0f,0.0f,0.0f);    
    }
    
    // Output to screen
    fragColor = to_float4_aw(col,1.0f);


  SetFragmentShaderComputedColor(fragColor);
}