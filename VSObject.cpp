// Include the OS headers
//-----------------------
#include <windows.h>
#include <atlbase.h>
#pragma warning( disable: 4996 )
#include <strsafe.h>
#pragma warning( default: 4996 )
// Include the D3D10 headers
//--------------------------
#include <d3d10.h>
#include <d3dx10.h>
#include <d3dx9.h>

#include <string>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include "DXUT.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "VSObject.h"
#include "Globals.h"


D3D10_INPUT_ELEMENT_DESC VSObject::InputElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
};

int VSObject::InputElementCount = 2;


D3D10_INPUT_ELEMENT_DESC VSObject::InputCurveElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 2, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
};

int VSObject::InputCurveElementCount = 4;



VSObject::VSObject(ID3D10Device *pd3dDevice)
{
	m_pVertexBuffer = NULL;
	m_pVertexLayout = NULL;
	m_pDepthStencil = NULL;
	m_diffuseTexture[0] = NULL;
	m_diffuseTexture[1] = NULL;
	m_distDirTexture = NULL;
	m_otherTexture = NULL;
	diffTex = 0;
	diff2Tex = 0;
	diffSteps = 8;
	m_pMeshDiff = NULL;
	m_pMeshCurves = NULL;
	m_cNum = 0;
	m_curve = NULL;
	m_scale = 1;
	m_pan = D3DXVECTOR2(0,0);
	m_polySize = 1.0;
	// create output quad
	m_sizeX = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Width : DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	m_sizeY = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height : DXUTGetDXGIBackBufferSurfaceDesc()->Height;
}




VSObject::~VSObject(void)
{
//	SAFE_RELEASE( pMesh );
}





void VSObject::RenderDiffusion(ID3D10Device *pd3dDevice)
{
	HRESULT hr;
	D3D10_TECHNIQUE_DESC techDesc;
    D3D10_PASS_DESC PassDesc;
	UINT stride, offset;

	//store the old render targets and viewports
    ID3D10RenderTargetView* old_pRTV = DXUTGetD3D10RenderTargetView();
    ID3D10DepthStencilView* old_pDSV = DXUTGetD3D10DepthStencilView();
	UINT NumViewports = 1;
	D3D10_VIEWPORT pViewports[100];
	pd3dDevice->RSGetViewports( &NumViewports, &pViewports[0]);

	// set the shader variables, they are valid through the whole rendering pipeline
	V( g_pScale->SetFloat(m_scale) );
	V( g_pPan->SetFloatVector(m_pan) );
	V( m_pDiffX->SetFloat( m_sizeX ) );
	V( m_pDiffY->SetFloat( m_sizeY ) );
	V( m_pPolySize->SetFloat( m_polySize ) );

	// render the triangles to the highest input texture level (assuming they are already defined!)
	ID3D10InputLayout* pCurveVertexLayout;
	m_pDrawVectorsTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	if( FAILED( pd3dDevice->CreateInputLayout( InputCurveElements, InputCurveElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pCurveVertexLayout ) ) )
		return;
	pd3dDevice->IASetInputLayout( pCurveVertexLayout );
  	ID3D10Buffer *pVertexBuffer;
	V( m_pMeshCurves->GetDeviceVertexBuffer(0, &pVertexBuffer) );
	stride = sizeof( CURVE_Vertex );
	offset = 0;
	pd3dDevice->IASetVertexBuffers( 0, 1, &pVertexBuffer, &stride, &offset );
	pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_LINELIST );
	pd3dDevice->RSSetViewports( 1, &m_vp );
	pd3dDevice->ClearDepthStencilView( m_pDepthStencilView, D3D10_CLEAR_DEPTH, 1.0f, 0 );
	//construct the curve triangles in the geometry shader and render them directly
	ID3D10RenderTargetView *destTexTV[3];
	destTexTV[0] = m_diffuseTextureTV[1-diffTex];
	destTexTV[1] = m_distDirTextureTV;
	destTexTV[2] = m_otherTextureTV;
	pd3dDevice->OMSetRenderTargets( 3, destTexTV, m_pDepthStencilView );
	m_pDrawVectorsTechnique->GetDesc( &techDesc );
	for(UINT p=0; p<techDesc.Passes; ++p)
	{
		m_pDrawVectorsTechnique->GetPassByIndex( p )->Apply(0);
		pd3dDevice->Draw( m_pMeshCurves->GetVertexCount(), 0 );
	}
	diffTex = 1-diffTex;
	diff2Tex = 1-diff2Tex;

	// setup the pipeline for the following image-space algorithms
	m_pDiffuseTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	if( FAILED( pd3dDevice->CreateInputLayout( InputElements, InputElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pVertexLayout ) ) )
		return;
	pd3dDevice->IASetInputLayout( m_pVertexLayout );
	V( m_pMeshDiff->GetDeviceVertexBuffer(0, &pVertexBuffer) );
	stride = sizeof( VSO_Vertex );
	pd3dDevice->IASetVertexBuffers( 0, 1, &pVertexBuffer, &stride, &offset );
	pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

	// diffuse the texture in both directions
	for (int i=0; i<diffSteps; i++)
	{
		// SA strategy
		V( m_pPolySize->SetFloat( 1.0 -(float)(i)/(float)diffSteps ) );
		// SH strategy
/*		V( m_pPolySize->SetFloat( 1.0 ) );
		if (i>diffSteps-diffSteps/2)
		{
			V( m_pPolySize->SetFloat( (float)(diffSteps-i)/(float)(diffSteps/2) ) );
		}
*/
		pd3dDevice->OMSetRenderTargets( 1, &m_diffuseTextureTV[1-diffTex], NULL );
		V( m_pInTex[0]->SetResource( m_diffuseTextureRV[diffTex] ) );
		V( m_pInTex[1]->SetResource( m_distDirTextureRV ) );
		diffTex = 1-diffTex;
		m_pDiffuseTechnique->GetDesc( &techDesc );
		for(UINT p=0; p<techDesc.Passes; ++p)
		{
			m_pDiffuseTechnique->GetPassByIndex( p )->Apply(0);
			pd3dDevice->Draw( 3*m_pMeshDiff->GetFaceCount(), 0 );
		}
	}

	// anti alias the lines
	pd3dDevice->OMSetRenderTargets( 1, &m_diffuseTextureTV[1-diffTex], NULL );
	V( m_pInTex[0]->SetResource( m_diffuseTextureRV[diffTex] ) );
	V( m_pInTex[1]->SetResource( m_otherTextureRV ) );
	diffTex = 1-diffTex;
	V( m_pDiffTex->SetResource( m_distDirTextureRV ) );
	m_pLineAntiAliasTechnique->GetDesc( &techDesc );
	for(UINT p=0; p<techDesc.Passes; ++p)
	{
		m_pLineAntiAliasTechnique->GetPassByIndex( p )->Apply(0);
		pd3dDevice->Draw( 3*m_pMeshDiff->GetFaceCount(), 0 );
	}

	//restore old render targets
	pd3dDevice->OMSetRenderTargets( 1,  &old_pRTV,  old_pDSV );
	pd3dDevice->RSSetViewports( NumViewports, &pViewports[0]);
}




// this renders the final image to the screen
void VSObject::Render(ID3D10Device *pd3dDevice)
{
	HRESULT hr;
	// Create the input layout
    D3D10_PASS_DESC PassDesc;
    m_pDisplayImage->GetPassByIndex( 0 )->GetDesc( &PassDesc );
    if( FAILED( pd3dDevice->CreateInputLayout( InputElements, InputElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &m_pVertexLayout ) ) )
        return;
    // Set the input layout
    pd3dDevice->IASetInputLayout( m_pVertexLayout );
	if(m_pVertexBuffer == NULL)
		V( m_pMeshDiff->GetDeviceVertexBuffer(0, &m_pVertexBuffer) );

    // Set vertex buffer
    UINT stride = sizeof( VSO_Vertex );
    UINT offset = 0;
    pd3dDevice->IASetVertexBuffers( 0, 1, &m_pVertexBuffer, &stride, &offset );
    pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
	V( m_pDiffTex->SetResource( m_diffuseTextureRV[diffTex] ) );
	D3D10_TECHNIQUE_DESC techDesc;
	m_pDisplayImage->GetDesc( &techDesc );
	for( UINT p = 0; p < techDesc.Passes; ++p )
	{
		m_pDisplayImage->GetPassByIndex( p )->Apply(0);
		pd3dDevice->Draw( 3*m_pMeshDiff->GetFaceCount(), 0 );
	}
}



bool stringStartsWith(const char *s, const char *val)
{
        return !strncmp(s, val, strlen(val));
}




void VSObject::SetupTextures(ID3D10Device *pd3dDevice, ID3D10Effect* g_pEffect10, int sizeX, int sizeY)
{
	HRESULT hr;

	// create the new mesh
	if (m_pMeshDiff == NULL)
	{
		//create the screen space quad
		D3DXVECTOR3 pos[3*2] = {	D3DXVECTOR3(-1.0f,-1.0f,+0.5f), D3DXVECTOR3( 1.0f,-1.0f,+0.5f), D3DXVECTOR3(-1.0f,1.0f,+0.5f),
									D3DXVECTOR3( 1.0f,-1.0f,+0.5f), D3DXVECTOR3( 1.0f, 1.0f,+0.5f), D3DXVECTOR3(-1.0f,1.0f,+0.5f) };
		D3DXVECTOR2 tex[3*2] = {	D3DXVECTOR2(0.0f, 0.0f), D3DXVECTOR2(1.0f, 0.0f), D3DXVECTOR2(0.0f, 1.0f),
									D3DXVECTOR2(1.0f, 0.0f), D3DXVECTOR2(1.0f, 1.0f), D3DXVECTOR2(0.0f, 1.0f)};
		VSO_Vertex pd[2*3];
		for (int ctri=0; ctri<2; ctri++)
		{
			UINT32 i = 3*ctri;
			pd[i+0].pos = pos[i+0];
			pd[i+1].pos = pos[i+1];
			pd[i+2].pos = pos[i+2];
			pd[i+0].tex = tex[i+0];
			pd[i+1].tex = tex[i+1];
			pd[i+2].tex = tex[i+2];
		}
		V( D3DX10CreateMesh(pd3dDevice, VSObject::InputElements, VSObject::InputElementCount, "POSITION", 2*3, 2, 0, &m_pMeshDiff) );
		V( m_pMeshDiff->SetVertexData(0, pd) );
		V( m_pMeshDiff->CommitToDevice() );
	}

	m_sizeX = sizeX;
	m_sizeY = sizeY;
	m_vp.Width = (int)(m_sizeX);
	m_vp.Height = (int)(m_sizeY);
	m_vp.MinDepth = 0.0f;
	m_vp.MaxDepth = 1.0f;
	m_vp.TopLeftX = 0;
	m_vp.TopLeftY = 0;

	// connect to shader variables
	m_pDrawVectorsTechnique = g_pEffect10->GetTechniqueByName( "DrawCurves" );
	m_pDiffuseTechnique = g_pEffect10->GetTechniqueByName( "Diffuse" );
	m_pLineAntiAliasTechnique = g_pEffect10->GetTechniqueByName( "LineAntiAlias" );
    m_pDisplayImage = g_pEffect10->GetTechniqueByName( "DisplayDiffusionImage" );
	m_pDiffTex = g_pEffect10->GetVariableByName( "g_diffTex" )->AsShaderResource();
	for (int i=0; i<3; i++)
		m_pInTex[i] = (g_pEffect10->GetVariableByName( "g_inTex" ))->GetElement(i)->AsShaderResource();
	m_pDiffX = g_pEffect10->GetVariableByName( "g_diffX")->AsScalar();
	m_pDiffY = g_pEffect10->GetVariableByName( "g_diffY")->AsScalar();
	g_pScale = g_pEffect10->GetVariableByName( "g_scale")->AsScalar();
	m_pPolySize = g_pEffect10->GetVariableByName( "g_polySize")->AsScalar();
	g_pPan = g_pEffect10->GetVariableByName( "g_pan")->AsVector();

	// create the depth buffer (only needed for curve mask rendering)
	DXGI_SAMPLE_DESC samdesc;
	samdesc.Count = 1;
	samdesc.Quality = 0;
	D3D10_TEXTURE2D_DESC texdesc;
	ZeroMemory( &texdesc, sizeof(D3D10_TEXTURE2D_DESC) );
	texdesc.MipLevels = 1;
	texdesc.ArraySize = 1;
	texdesc.SampleDesc = samdesc;
	texdesc.Width = (int)(m_sizeX);
	texdesc.Height = (int)(m_sizeY);
	texdesc.Usage = D3D10_USAGE_DEFAULT;
	texdesc.BindFlags = D3D10_BIND_DEPTH_STENCIL;
	texdesc.Format = DXGI_FORMAT_D32_FLOAT;
	if (m_pDepthStencil != NULL)
		m_pDepthStencil->Release();
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_pDepthStencil );
	hr = pd3dDevice->CreateDepthStencilView( m_pDepthStencil, NULL, &m_pDepthStencilView );

	//create diffusion textures (2 for ping pong rendering)
	if (m_diffuseTexture[0] != NULL)
		m_diffuseTexture[0]->Release();
	if (m_diffuseTexture[1] != NULL)
		m_diffuseTexture[1]->Release();
	if (m_distDirTexture != NULL)
		m_distDirTexture->Release();
	if (m_otherTexture != NULL)
		m_otherTexture->Release();
	texdesc.Width = (int)(m_sizeX);
	texdesc.Height = (int)(m_sizeY);
	texdesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
//	texdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;  // use this for higher accuracy diffusion
	texdesc.BindFlags =  D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_diffuseTexture[0]);
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_diffuseTexture[1]);
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_otherTexture);

	// distance map + nearest point map
	texdesc.Usage = D3D10_USAGE_DEFAULT;
	texdesc.CPUAccessFlags = 0;
	texdesc.BindFlags = D3D10_BIND_RENDER_TARGET | D3D10_BIND_SHADER_RESOURCE;
	texdesc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
	hr = pd3dDevice->CreateTexture2D( &texdesc, NULL, &m_distDirTexture);

	//create render target views
	hr = pd3dDevice->CreateShaderResourceView( m_diffuseTexture[0], NULL, &m_diffuseTextureRV[0] );
	hr = pd3dDevice->CreateRenderTargetView(   m_diffuseTexture[0], NULL, &m_diffuseTextureTV[0] );
	hr = pd3dDevice->CreateShaderResourceView( m_diffuseTexture[1], NULL, &m_diffuseTextureRV[1] );
	hr = pd3dDevice->CreateRenderTargetView(   m_diffuseTexture[1], NULL, &m_diffuseTextureTV[1] );
	hr = pd3dDevice->CreateShaderResourceView( m_distDirTexture, NULL, &m_distDirTextureRV );
	hr = pd3dDevice->CreateRenderTargetView(   m_distDirTexture, NULL, &m_distDirTextureTV );
	hr = pd3dDevice->CreateShaderResourceView( m_otherTexture, NULL, &m_otherTextureRV );
	hr = pd3dDevice->CreateRenderTargetView(   m_otherTexture, NULL, &m_otherTextureTV );

	// Orzan diffusion curves
	char s[255] = "Media\\zephyr.xml";

	if (m_pMeshCurves == NULL)
	{
		ReadVectorFile( &s[0] );
		ConstructCurves(pd3dDevice);
	}
}



void VSObject::ReadVectorFile( char *s )
{
	char buff[256];
	WCHAR wcFileInfo[512];
	char *token;

	FILE *F = fopen( s, "rb");

	while (fgets(buff, 255, F))
		if (stringStartsWith(buff, "<!DOCTYPE CurveSetXML"))
		{
			StringCchPrintf( wcFileInfo, 512, L"(INFO) : This seems to be a diffusion curves file.\n");
			OutputDebugString( wcFileInfo );
			break;
		}
	fgets(buff, 255, F);
	token = strtok(buff, " \"\t");
	while (!stringStartsWith(token, "image_width="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_fWidth = atof(token);
	while (!stringStartsWith(token, "image_height="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_fHeight = atof(token);
	while (!stringStartsWith(token, "nb_curves="))
		token = strtok(NULL, " \"\t");
	token = strtok(NULL, " \"\t");
	m_cNum = atof(token);
	StringCchPrintf( wcFileInfo, 512, L"(INFO) : %d curves found in file.\n", m_cNum);
	OutputDebugString( wcFileInfo );
	m_curve = new CURVE[m_cNum];
	m_cSegNum = 0;
	D3DXVECTOR2 maxBound = D3DXVECTOR2(-1000000,-1000000);
	D3DXVECTOR2 minBound = D3DXVECTOR2(1000000,1000000);
	for (int i1=0; i1<m_cNum; i1++)
	{
		while (!stringStartsWith(buff, " <curve nb_control_points"))
			fgets(buff, 255, F);
		token = strtok(buff, " \"\t");
		while (!stringStartsWith(token, "nb_control_points="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_curve[i1].pNum = atof(token);
		m_cSegNum += ((m_curve[i1].pNum-1)/3);

		while (!stringStartsWith(token, "nb_left_colors="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_curve[i1].clNum = atof(token);

		while (!stringStartsWith(token, "nb_right_colors="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_curve[i1].crNum = atof(token);

		while (!stringStartsWith(token, "nb_blur_points="))
			token = strtok(NULL, " \"\t");
		token = strtok(NULL, " \"\t");
		m_curve[i1].bNum = atof(token);

		// read in individual curve data
		m_curve[i1].p = new D3DXVECTOR2[m_curve[i1].pNum];
		for (int i2=0; i2<m_curve[i1].pNum; i2++)
		{
			while (!stringStartsWith(buff, "   <control_point "))
				fgets(buff, 255, F);
			token = strtok(buff, " \"\t");
			while (!stringStartsWith(token, "x="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].p[i2].y = atof(token);
			while (!stringStartsWith(token, "y="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].p[i2].x = atof(token);
			fgets(buff, 255, F);
			//extend the bounds if necessary
			if (m_curve[i1].p[i2].y < minBound.y)
				minBound.y = m_curve[i1].p[i2].y;
			if (m_curve[i1].p[i2].y > maxBound.y)
				maxBound.y = m_curve[i1].p[i2].y;
			if (m_curve[i1].p[i2].x < minBound.x)
				minBound.x = m_curve[i1].p[i2].x;
			if (m_curve[i1].p[i2].x > maxBound.x)
				maxBound.x = m_curve[i1].p[i2].x;
		}
		m_curve[i1].cl = new COLORPOINT[m_curve[i1].clNum];
		for (int i2=0; i2<m_curve[i1].clNum; i2++)
		{
			while (!stringStartsWith(buff, "   <left_color "))
				fgets(buff, 255, F);
			token = strtok(buff, " \"\t");
			while (!stringStartsWith(token, "G="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cl[i2].col.y = atof(token)/256.0;

			while (!stringStartsWith(token, "R="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cl[i2].col.z  = atof(token)/256.0;

			while (!stringStartsWith(token, "globalID="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cl[i2].off = atof(token);

			while (!stringStartsWith(token, "B="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cl[i2].col.x  = atof(token)/256.0;
			fgets(buff, 255, F);
		}

		m_curve[i1].cr = new COLORPOINT[m_curve[i1].crNum];
		for (int i2=0; i2<m_curve[i1].crNum; i2++)
		{
			while (!stringStartsWith(buff, "   <right_color "))
				fgets(buff, 255, F);
			token = strtok(buff, " \"\t");
			while (!stringStartsWith(token, "G="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cr[i2].col.y = atof(token)/256.0;

			while (!stringStartsWith(token, "R="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cr[i2].col.z  = atof(token)/256.0;

			while (!stringStartsWith(token, "globalID="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cr[i2].off = atof(token);

			while (!stringStartsWith(token, "B="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].cr[i2].col.x  = atof(token)/256.0;
			fgets(buff, 255, F);
		}

		m_curve[i1].b = new BLURRPOINT[m_curve[i1].bNum];
		for (int i2=0; i2<m_curve[i1].bNum; i2++)
		{
			while (!stringStartsWith(buff, "   <best_scale"))
				fgets(buff, 255, F);
			token = strtok(buff, " \"\t");
			while (!stringStartsWith(token, "value="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].b[i2].blurr = atof(token);

			while (!stringStartsWith(token, "globalID="))
				token = strtok(NULL, " \"\t");
			token = strtok(NULL, " \"\t");
			m_curve[i1].b[i2].off = atof(token);
			fgets(buff, 255, F);
		}
	}
	fclose(F);
	//scale the whole image between -1 and 1
	D3DXVECTOR2 middlePan = D3DXVECTOR2( 0.5*(maxBound.x+minBound.x), 0.5*(maxBound.y+minBound.y));
	for (int i1=0; i1<m_cNum; i1++)
		for (int i2=0; i2<m_curve[i1].pNum; i2++)
		{
			m_curve[i1].p[i2].x = 2.0f*(m_curve[i1].p[i2].x-middlePan.x)/m_fWidth;
			m_curve[i1].p[i2].y = 2.0f*(m_curve[i1].p[i2].y-middlePan.y)/m_fHeight;
		}
	StringCchPrintf( wcFileInfo, 512, L"(INFO) : %d curve segments found in file.\n", m_cSegNum);
	OutputDebugString( wcFileInfo );

}




// convert the vectors into triangle strips and draw these to the finest pyramid level
void VSObject::ConstructCurves(ID3D10Device *pd3dDevice)
{
	D3DXVECTOR2	pLoop;
	int	iLoopStart;
	HRESULT	hr;
	float subSegNum = 1.0f;
	CURVE_Vertex *pd = new CURVE_Vertex[m_cSegNum*(int)(subSegNum)*10*2];
	int	*used = new int[m_cNum];
	ZeroMemory(used,m_cNum*sizeof(int));
	int cPos = 0;
	for (int iX=0; iX<m_cNum; iX++)
	{
		int i1=(int)(rand())*(m_cNum-1)/RAND_MAX;
		while (used[i1] > 0)
		{
			i1++;
			if (i1 == m_cNum)
				i1 = 0;
		}
		used[i1] = 1;

		int cSeg = 0;
		int lID = 0;
		while (m_curve[i1].cl[lID+1].off <= 0)
			lID++;
		int lS = m_curve[i1].cl[lID].off;
		int lN = m_curve[i1].cl[lID+1].off;
		int rID = 0;
		while (m_curve[i1].cr[rID+1].off <= 0)
			rID++;
		int rS = m_curve[i1].cr[rID].off;
		int rN = m_curve[i1].cr[rID+1].off;
		int bID = 0;
		while (m_curve[i1].b[bID+1].off <= 0)
			bID++;
		int bS = m_curve[i1].b[rID].off;
		int bN2 = m_curve[i1].b[rID+1].off;
		for (int i2=0; i2<m_curve[i1].pNum-1; i2+=3)
		{
			for (float t=0; t<1.0f; t+=0.1f)
			{
				float t1 = t;
				float t2 = t+0.1f;

				float f = (float)(cSeg-lS)/(float)(lN-lS);
				float fN = (float)(cSeg+1-lS)/(float)(lN-lS);
				D3DXVECTOR4 cL = D3DXVECTOR4((1.0f-f)*m_curve[i1].cl[lID].col + f*m_curve[i1].cl[lID+1].col, 1.0f);
				D3DXVECTOR4 cNL = D3DXVECTOR4((1.0f-fN)*m_curve[i1].cl[lID].col + fN*m_curve[i1].cl[lID+1].col, 1.0f);

				f = (float)(cSeg-rS)/(float)(rN-rS);
				fN = (float)(cSeg+1-rS)/(float)(rN-rS);
				D3DXVECTOR4 cR = D3DXVECTOR4((1.0f-f)*m_curve[i1].cr[rID].col + f*m_curve[i1].cr[rID+1].col, 1.0f);
				D3DXVECTOR4 cNR = D3DXVECTOR4((1.0f-fN)*m_curve[i1].cr[rID].col + fN*m_curve[i1].cr[rID+1].col, 1.0f);

				f = ((float)(cSeg-bS))/((float)(bN2-bS));
				fN = (float)(cSeg+1-bS)/(float)(bN2-bS);
				float b = (1.0f-f)*m_curve[i1].b[bID].blurr + f*m_curve[i1].b[bID+1].blurr;
				float bN = (1.0f-fN)*m_curve[i1].b[bID].blurr + fN*m_curve[i1].b[bID+1].blurr;

				// it is not entirely clear how [Orzan et al.08] encode the blurr in the files, this has been found to work ok..
				b = b*1; //pow(1.5f,b);
				bN = bN*1; //pow(1.5f,bN);

				for (float sI=0.0f; sI<1.0f; sI+=1.0f/subSegNum)
				{
					float sN = sI+1.0f/subSegNum;
					float s1  = (1.0f-sI)*t1 + sI*t2;
					float s2 =  (1.0f-sN)*t1 + sN*t2;

					D3DXVECTOR2 p0 = (1.0f-s1)*(1.0f-s1)*(1.0f-s1)*m_curve[i1].p[i2] + 3*s1*(1.0f-s1)*(1.0f-s1)*m_curve[i1].p[i2+1] + 3*s1*s1*(1.0f-s1)*m_curve[i1].p[i2+2] + s1*s1*s1*m_curve[i1].p[i2+3];
					D3DXVECTOR2 p1 = (1.0f-s2)*(1.0f-s2)*(1.0f-s2)*m_curve[i1].p[i2] + 3*s2*(1.0f-s2)*(1.0f-s2)*m_curve[i1].p[i2+1] + 3*s2*s2*(1.0f-s2)*m_curve[i1].p[i2+2] + s2*s2*s2*m_curve[i1].p[i2+3];

					pd[cPos+0].col[0] = (1.0f-sI)*cL + sI*cNL;
					pd[cPos+0].col[0].w = (1.0f-sI)*b + sI*bN;
					pd[cPos+1].col[0] = (1.0f-sN)*cL + sN*cNL;
					pd[cPos+1].col[0].w = (1.0f-sN)*b + sN*bN;
					pd[cPos+0].col[1] = (1.0f-sI)*cR + sI*cNR;
					pd[cPos+0].col[1].w = (1.0f-sI)*b + sI*bN;
					pd[cPos+1].col[1] = (1.0f-sN)*cR + sN*cNR;
					pd[cPos+1].col[1].w = (1.0f-sN)*b + sN*bN;
					pd[cPos+0].pos = D3DXVECTOR2(p0.x,-p0.y);
					pd[cPos+1].pos = D3DXVECTOR2(p1.x,-p1.y);

					if ((i2 == 0) && (t == 0.0f) && (sI < 0.5f/subSegNum))
					{
						pd[cPos+0].nb = D3DXVECTOR2(10000.0f, 10000.0f);
						// if we are at the begin of a loop, do not declare endpoints
						if ((m_curve[i1].p[0] == m_curve[i1].p[m_curve[i1].pNum-1]) && (sI < 0.5f/subSegNum))
						{
							pLoop = pd[cPos+1].pos;
							iLoopStart = cPos;
						}
					}
					else
					{
						pd[cPos+0].nb = pd[cPos-1].pos;
						pd[cPos-1].nb = pd[cPos+1].pos;
					}
					pd[cPos+1].nb = D3DXVECTOR2(10000.0f, 10000.0f);
					// if we are at the end of a loop, do not declare endpoints
					if ((m_curve[i1].p[0] == m_curve[i1].p[m_curve[i1].pNum-1]) && (i2==m_curve[i1].pNum-1-3) && (t>=0.89f) && (sI+1.1f/subSegNum>=1.0))
					{
						pd[iLoopStart].nb = pd[cPos+0].pos;
						pd[cPos+1].nb = pLoop;
					}
					cPos += 2;
				}
				cSeg++;
				while ((cSeg >= lN) && (lID < m_curve[i1].clNum-2))
				{
					lID++;
					lS = m_curve[i1].cl[lID].off;
					lN = m_curve[i1].cl[lID+1].off;
				}
				while ((cSeg >= rN) && (rID < m_curve[i1].crNum-2))
				{
					rID++;
					rS = m_curve[i1].cr[rID].off;
					rN = m_curve[i1].cr[rID+1].off;
				}
				while ((cSeg >= bN2)  && (bID < m_curve[i1].bNum-2))
				{
					bID++;
					bS = m_curve[i1].b[bID].off;
					bN2 = m_curve[i1].b[bID+1].off;
				}
			}
		}
	}
	if (m_pMeshCurves)
	{
		m_pMeshCurves->Discard( D3DX10_MESH_DISCARD_ATTRIBUTE_BUFFER );
		m_pMeshCurves->Discard( D3DX10_MESH_DISCARD_ATTRIBUTE_TABLE );
		m_pMeshCurves->Discard( D3DX10_MESH_DISCARD_POINTREPS );
		m_pMeshCurves->Discard( D3DX10_MESH_DISCARD_ADJACENCY );
		m_pMeshCurves->Discard( D3DX10_MESH_DISCARD_DEVICE_BUFFERS );
	}
	m_pMeshCurves = NULL;
	V( D3DX10CreateMesh(pd3dDevice, VSObject::InputCurveElements, VSObject::InputCurveElementCount, "POSITION", m_cSegNum*(int)(subSegNum)*10*2, m_cSegNum*(int)(subSegNum)*10, 0, &m_pMeshCurves) );
	V( m_pMeshCurves->SetVertexData(0, pd) );
	V( m_pMeshCurves->CommitToDevice() );
	delete[]pd;
	delete[]used;

	WCHAR wcFileInfo[512];
	StringCchPrintf( wcFileInfo, 512, L"(INFO) : Number of curve segments: %d \n", m_cSegNum*(int)(subSegNum)*10);
	OutputDebugString( wcFileInfo );

}
