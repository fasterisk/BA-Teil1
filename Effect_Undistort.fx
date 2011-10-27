cbuffer cbChangesEveryFrame
{
	float g_scale;				// scale factor 
	float2 g_pan;				// panning 
	float g_diffX;				// x size of diffusion texture
	float g_diffY;				// y size of diffusion texture
	float g_polySize;			// size of the curve polygons (2 is screen width/height)
};

Texture2D g_inTex[3];	// input texture (containing lines with north east south west colors)
Texture2D g_diffTex;	// diffusion texture 


SamplerState PointSampler
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};


SamplerState LinearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};




struct VS_INPUT
{
    float3 Pos		: POSITION;
    float2 Tex		: TEXTURE0;
};

struct PS_INPUT
{
	float4 oPosition : SV_POSITION;
	float4 worldPos  : TEXTURE0;
    float2 Tex		 : TEXTURE1;
};

struct PS_OUTPUT
{
	float4 oColor	: SV_Target0;
};


struct PS_OUTPUT_MRT_3
{
	float4 oColor1	: SV_Target0;
	float4 oColor2	: SV_Target1;
	float4 oColor3	: SV_Target2;
};


struct VS_CURVE_INPUT
{
    float2 pos		: POSITION;
    float4 col0		: TEXTURE0;
    float4 col1		: TEXTURE1;
    float2 nb		: TEXTURE2;
};


struct PS_CURVE_INPUT
{
	float4 oPosition : SV_POSITION;
    float4 col		 : TEXTURE0;	//contains blurr in w component
    float4 distdir	 : TEXTURE1;    //distance(x) and direction(yz) to the closest border, will be used for diffusion and anti-aliasing
    float4 oCol		 : TEXTURE2;	//color on the other side of of the curve (used for antialiasing)
};



// ================================================================================
//
// Vertex Shader
//
// ================================================================================

PS_INPUT TetraVS(VS_INPUT In)
{
	PS_INPUT Out;
    Out.oPosition = float4(In.Pos.x, -In.Pos.y,+0.5,1.0);
    Out.Tex = In.Tex;
    return Out;
}


VS_CURVE_INPUT CurveVS(VS_CURVE_INPUT In)
{
	VS_CURVE_INPUT Out;
    Out.pos = In.pos*g_scale+g_pan;
    Out.col0 = In.col0;
    Out.col1 = In.col1;
    if (In.nb.x < 9999.0)
	    Out.nb = In.nb*g_scale+g_pan;
	else
		Out.nb = float2(10000.0,10000.0);
    return Out;
}


// ================================================================================
//
// Geometry Shader
//
// ================================================================================



const float zMax = 1.0;

[maxvertexcount(4)]
void CurveGS1( line VS_CURVE_INPUT input[2], inout TriangleStream<PS_CURVE_INPUT> tStream )
{
	PS_CURVE_INPUT p0;
	float2 nL = input[1].pos.xy-input[0].pos.xy;
	nL = normalize(float2(-nL.y, nL.x));

	// left side
	p0.oPosition = float4( input[0].pos.xy+g_polySize*nL, zMax, 1.0);
	p0.col = input[0].col0;
	p0.oCol = input[0].col1;
	p0.distdir = float4(0.5*g_polySize,-nL,0);
	tStream.Append( p0 );
	p0.oPosition = float4( input[1].pos.xy+g_polySize*nL, zMax, 1.0);
	p0.col = input[1].col0;
	p0.oCol = input[1].col1;
	p0.distdir = float4(0.5*g_polySize,-nL,0);
	tStream.Append( p0 );
	p0.oPosition = float4( input[0].pos.xy, 0.0, 1.0);
	p0.col = input[0].col0;
	p0.oCol = input[0].col1;
	p0.distdir = float4(0,-nL,0);
	tStream.Append( p0 );
	p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
	p0.col = input[1].col0;
	p0.oCol = input[1].col1;
	p0.distdir = float4(0,-nL,0);
	tStream.Append( p0 );
	tStream.RestartStrip();
}



[maxvertexcount(4)]
void CurveGS2( line VS_CURVE_INPUT input[2], inout TriangleStream<PS_CURVE_INPUT> tStream )
{
	PS_CURVE_INPUT p0;
	float2 nL = input[1].pos.xy-input[0].pos.xy;
	nL = normalize(float2(-nL.y, nL.x));

	//right side
	p0.oPosition = float4( input[0].pos.xy, 0.0, 1.0);
	p0.col = input[0].col1;
	p0.oCol = input[0].col0;
	p0.distdir = float4(0,nL,0);
	tStream.Append( p0 );
	p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
	p0.col = input[1].col1;
	p0.oCol = input[1].col0;
	p0.distdir = float4(0,nL,0);
	tStream.Append( p0 );
	p0.oPosition = float4( input[0].pos.xy-g_polySize*nL, zMax, 1.0);
	p0.col = input[0].col1;
	p0.oCol = input[0].col0;
	p0.distdir = float4(0.5*g_polySize,nL,0);
	tStream.Append( p0 );
	p0.oPosition = float4( input[1].pos.xy-g_polySize*nL, zMax, 1.0);
	p0.col = input[1].col1;
	p0.oCol = input[1].col0;
	p0.distdir = float4(0.5*g_polySize,nL,0);
	tStream.Append( p0 );
	tStream.RestartStrip();
}



[maxvertexcount(8)]
void CurveGS3( line VS_CURVE_INPUT input[2], inout TriangleStream<PS_CURVE_INPUT> tStream )
{
	// close the gaps to the following line
	if (input[1].nb.x < 9999.0)
	{
		PS_CURVE_INPUT p0;

		float2 nL = input[1].pos.xy-input[0].pos.xy;
		nL = normalize(float2(-nL.y, nL.x));
		float2 nLN = input[1].nb.xy-input[1].pos.xy;
		nLN = normalize(float2(-nLN.y, nLN.x));

		//larger than 90 degree angle -> form 4 polygons
		if (dot(input[0].pos.xy-input[1].pos.xy,input[1].nb.xy-input[1].pos.xy) >= 0)
		{
			float2 lP = -normalize(normalize(input[0].pos.xy-input[1].pos.xy) + normalize(input[1].nb.xy-input[1].pos.xy));
			float4 col = input[1].col0;
			float4 oCol = input[1].col1;
			float2 lH1,lH2;
			if (dot(nL, input[1].nb.xy-input[1].pos.xy) > 0)
			{
				col = input[1].col1;
				oCol = input[1].col0;
				nL = -nL;
				nLN = -nLN;
				lH1 = normalize(nL+lP);
				lH2 = normalize(nLN+lP);
			}
			else
			{
				lH1 = normalize(nL+lP);
				lH2 = normalize(nLN+lP);
			}
			p0.col = col;
			p0.oCol = oCol;
			
			p0.oPosition = float4( input[1].pos+g_polySize*nL, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*nL,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
			p0.distdir = float4(0,float2(0,0),1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos+g_polySize*lH1, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*lH1,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy+g_polySize*lP, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*lP,1);
			tStream.Append( p0 );
			tStream.RestartStrip();

			p0.oPosition = float4( input[1].pos.xy+g_polySize*lP, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*lP,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
			p0.distdir = float4(0,float2(0,0),1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos+g_polySize*lH2, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*lH2,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy+g_polySize*nLN, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*nLN,1);
			tStream.Append( p0 );
			tStream.RestartStrip();
		}
		else if (dot(nL,nLN) <= 0.7071)	//between 45 and 90 degree angle -> form 2 polygons
		{
			float4 col = input[1].col0;
			float4 oCol = input[1].col1;
			if ( dot(input[1].nb.xy-input[1].pos.xy,nL) >= 0 )
			{
				col = input[1].col1;
				oCol = input[1].col0;
				nL = - nL;
				nLN = -nLN;
			}
			float2 lH = normalize(nL+nLN);
			p0.col = col;
			p0.oCol = oCol;
			p0.oPosition = float4( input[1].pos.xy+g_polySize*nL, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*nL,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
			p0.distdir = float4(0,float2(0,0),1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy+g_polySize*lH, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*lH,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy+g_polySize*nLN, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*nLN,1);
			tStream.Append( p0 );
			tStream.RestartStrip();
		}
		else //smaller than 45 degrees angle -> form 1 polygon
		{
			float4 col = input[1].col0;
			float4 oCol = input[1].col1;
			if ( dot(input[1].nb.xy-input[1].pos.xy,nL) >= 0 )
			{
				col = input[1].col1;
				oCol = input[1].col0;
				nL = - nL;
				nLN = -nLN;
			}
			p0.col = col;
			p0.oCol = oCol;
			p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
			p0.distdir = float4(0,float2(0,0),1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy+g_polySize*nL, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*nL,1);
			tStream.Append( p0 );
			p0.oPosition = float4( input[1].pos.xy+g_polySize*nLN, zMax, 1.0);
			p0.distdir = float4(0,-g_polySize*nLN,1);
			tStream.Append( p0 );
			tStream.RestartStrip();
		}
	}		
}



[maxvertexcount(4)]
void CurveGS4( line VS_CURVE_INPUT input[2], inout TriangleStream<PS_CURVE_INPUT> tStream )
{
	if (input[0].nb.x > 9999.0)
	{
		// fill the beginning of the line if we are at a starting point
		PS_CURVE_INPUT p0;
		float2 nL = input[1].pos.xy-input[0].pos.xy;
		nL = normalize(float2(-nL.y, nL.x));
		float2 lP = -normalize(input[1].pos.xy-input[0].pos.xy);
		float2 lH = normalize(nL+lP);
	
		p0.col = input[0].col0;
		p0.oCol = input[0].col1;
		p0.oPosition = float4( input[0].pos.xy+g_polySize*nL, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*nL,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[0].pos.xy, 0.0, 1.0);
		p0.distdir = float4(0,float2(0,0),1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[0].pos.xy+g_polySize*lH, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lH,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[0].pos.xy+g_polySize*lP, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lP,1);
		tStream.Append( p0 );
		tStream.RestartStrip();
	}
	else if (input[1].nb.x > 9999.0)
	{
		// fill the end of the line if we are at an end point
		PS_CURVE_INPUT p0;

		float2 nL = input[1].pos.xy-input[0].pos.xy;
		nL = normalize(float2(-nL.y, nL.x));
		float2 lP = normalize(input[1].pos.xy-input[0].pos.xy);
		float2 lH = normalize(nL+lP);

		p0.col = input[1].col0;
		p0.oCol = input[1].col1;
		p0.oPosition = float4( input[1].pos.xy+g_polySize*nL, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*nL,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
		p0.distdir = float4(0,float2(0,0),1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[1].pos.xy+g_polySize*lH, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lH,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[1].pos.xy+g_polySize*lP, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lP,1);
		tStream.Append( p0 );
		tStream.RestartStrip();
	}
}




[maxvertexcount(4)]
void CurveGS5( line VS_CURVE_INPUT input[2], inout TriangleStream<PS_CURVE_INPUT> tStream )
{
	if (input[0].nb.x > 9999.0)
	{
		// fill the beginning of the line if we are at a starting point
		PS_CURVE_INPUT p0;
		float2 nL = input[1].pos.xy-input[0].pos.xy;
		nL = normalize(float2(-nL.y, nL.x));
		float2 lP = -normalize(input[1].pos.xy-input[0].pos.xy);
		float2 lH = normalize(nL+lP);
	
		p0.col = input[0].col1;
		p0.oCol = input[0].col0;
		p0.oPosition = float4( input[0].pos.xy+g_polySize*lP, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lP,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[0].pos.xy, 0.0, 1.0);
		p0.distdir = float4(0,float2(0,0),1);
		tStream.Append( p0 );
		lH = normalize(-nL+lP);
		p0.oPosition = float4( input[0].pos.xy+g_polySize*lH, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lH,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[0].pos.xy-g_polySize*nL, zMax, 1.0);
		p0.distdir = float4(0,g_polySize*nL,1);
		tStream.Append( p0 );
		tStream.RestartStrip();
	}
	else if (input[1].nb.x > 9999.0)
	{
		// fill the end of the line if we are at an end point
		PS_CURVE_INPUT p0;

		float2 nL = input[1].pos.xy-input[0].pos.xy;
		nL = normalize(float2(-nL.y, nL.x));
		float2 lP = normalize(input[1].pos.xy-input[0].pos.xy);
		float2 lH = normalize(nL+lP);

		p0.col = input[1].col1;
		p0.oCol = input[1].col0;
		p0.oPosition = float4( input[1].pos.xy+g_polySize*lP, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lP,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[1].pos.xy, 0.0, 1.0);
		p0.distdir = float4(0,float2(0,0),1);
		tStream.Append( p0 );
		lH = normalize(-nL+lP);
		p0.oPosition = float4( input[1].pos.xy+g_polySize*lH, zMax, 1.0);
		p0.distdir = float4(0,-g_polySize*lH,1);
		tStream.Append( p0 );
		p0.oPosition = float4( input[1].pos.xy-g_polySize*nL, zMax, 1.0);
		p0.distdir = float4(0,g_polySize*nL,1);
		tStream.Append( p0 );
		tStream.RestartStrip();
	}
}



// ================================================================================
//
// The Pixel Shader.
//
// ================================================================================


PS_OUTPUT TetraUpPS(PS_INPUT In)
{
	PS_OUTPUT Out; 
	Out.oColor = g_diffTex.Sample(LinearSampler, In.Tex.xy).xyzw;
	return Out;
}



PS_OUTPUT_MRT_3 CurvePS(PS_CURVE_INPUT In)
{
	PS_OUTPUT_MRT_3 Out; 
	Out.oColor1 = In.col;
	if (In.distdir.w < 0.5)
		Out.oColor2 = float4( In.distdir.x, float2(In.distdir.y,-In.distdir.z), 0);
	else
		Out.oColor2 = float4( 0.5*length(In.distdir.yz), normalize(float2(In.distdir.y,-In.distdir.z)), 1);
	Out.oColor3 = In.oCol;
	return Out;
}




PS_OUTPUT DiffusePS(PS_INPUT In)
{
	PS_OUTPUT Out; 
	float rawKernel = 0.92387*g_inTex[1].SampleLevel(PointSampler, In.Tex.xy, 0).x;
	float kernel = rawKernel*g_diffX;
	kernel *= g_polySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	Out.oColor = g_inTex[0].SampleLevel(PointSampler, In.Tex.xy+float2(-kernel/g_diffX,0), 0);
	Out.oColor += g_inTex[0].SampleLevel(PointSampler, In.Tex.xy+float2( kernel/g_diffX,0), 0);
	kernel = rawKernel*g_diffY;
	kernel *= g_polySize;
	kernel -= 0.5;
	kernel = max(0,kernel);
	Out.oColor += g_inTex[0].SampleLevel(PointSampler, In.Tex.xy+float2(0,-kernel/g_diffY), 0);
	Out.oColor += g_inTex[0].SampleLevel(PointSampler, In.Tex.xy+float2(0, kernel/g_diffY), 0);
	Out.oColor /= 4;
	return Out;
}


PS_OUTPUT LineAntiAliasPS(PS_INPUT In)
{
	PS_OUTPUT Out; 
	float3 dd = g_diffTex.SampleLevel(PointSampler, In.Tex.xy, 0).xyz;
	dd.yz = normalize(dd.yz);
	dd.z = -dd.z;
	float4 thisColor = g_inTex[0].SampleLevel(PointSampler, In.Tex.xy, 0).xyzw;
	if (dd.x*g_diffX > 0.7)
		Out.oColor = thisColor;
	else
	{
		float4 otherColor = g_inTex[1].SampleLevel(PointSampler, In.Tex.xy, 0).xyzw;
		Out.oColor = (1.0-(dd.x*g_diffX/0.7))*0.5*(otherColor+thisColor) + (dd.x*g_diffX/0.7)*thisColor;
	}
	return Out;
}



//--------------------------------------------------------------------------------------
// Techniques/states
//--------------------------------------------------------------------------------------

RasterizerState state
{
	FillMode = Solid;
	CullMode = none;
	MultisampleEnable = FALSE;
};


DepthStencilState soliddepth
{
	DepthEnable = FALSE;
};


DepthStencilState depthEnable
{
	DepthEnable = TRUE;
	Depthfunc = LESS_EQUAL;
	DepthWriteMask = ALL;
	StencilEnable = FALSE;
};


BlendState NoBlending
{
    AlphaToCoverageEnable = FALSE;
    BlendEnable[0] = FALSE;
};



technique10 DisplayDiffusionImage
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, TetraVS( ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, TetraUpPS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( soliddepth, 0 );
    }
}



technique10 DrawCurves
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, CurveVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, CurveGS1( ) ) );
        SetPixelShader( CompileShader( ps_4_0, CurvePS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( depthEnable, 0 );
    }
	pass P2
	{
        SetVertexShader( CompileShader( vs_4_0, CurveVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, CurveGS2( ) ) );
        SetPixelShader( CompileShader( ps_4_0, CurvePS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( depthEnable, 0 );
    }
	pass P3
	{
        SetVertexShader( CompileShader( vs_4_0, CurveVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, CurveGS3( ) ) );
        SetPixelShader( CompileShader( ps_4_0, CurvePS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( depthEnable, 0 );
    }
	pass P4
	{
        SetVertexShader( CompileShader( vs_4_0, CurveVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, CurveGS4( ) ) );
        SetPixelShader( CompileShader( ps_4_0, CurvePS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( depthEnable, 0 );
    }         
	pass P5
	{
        SetVertexShader( CompileShader( vs_4_0, CurveVS( ) ) );
        SetGeometryShader( CompileShader( gs_4_0, CurveGS5( ) ) );
        SetPixelShader( CompileShader( ps_4_0, CurvePS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( depthEnable, 0 );
    }         
}



technique10 Diffuse
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, TetraVS( ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, DiffusePS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( soliddepth, 0 );
    }
}


technique10 LineAntiAlias
{
	pass P1
	{
        SetVertexShader( CompileShader( vs_4_0, TetraVS( ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, LineAntiAliasPS( ) ) );
        SetRasterizerState( state );
        SetBlendState( NoBlending, float4( 1.0f, 1.0f, 1.0f, 1.0f ), 0xFFFFFFFF );
        SetDepthStencilState( soliddepth, 0 );
    }
}


