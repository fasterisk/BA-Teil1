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
#include "Globals.h"
#include "VSCombinedObject.h"


D3D10_INPUT_ELEMENT_DESC VSCombinedObject::InputElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 12, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
};

int VSCombinedObject::InputElementCount = 2;


D3D10_INPUT_ELEMENT_DESC VSCombinedObject::InputCurveElements[] =
{
    { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 8, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D10_INPUT_PER_VERTEX_DATA, 0 },
    { "TEXTURE", 2, DXGI_FORMAT_R32G32_FLOAT, 0, 40, D3D10_INPUT_PER_VERTEX_DATA, 0 },
	{ NULL, 0, DXGI_FORMAT_UNKNOWN, 0, 0, D3D10_INPUT_PER_VERTEX_DATA, 0 }
};

int VSCombinedObject::InputCurveElementCount = 4;



VSCombinedObject::VSCombinedObject(ID3D10Device *pd3dDevice)
{
	g_vsObj1 = new VSObject(pd3dDevice);
	g_vsObj2 = new VSObject(pd3dDevice);
	g_fileObj1 = "Media\\zephyr.xml";
	g_fileObj2 = "Media\\behindthecurtain.xml";
	g_controlledObj = g_vsObj1;
	g_Obj1IsControlled = true;
	g_showIso = false;
	m_pVertexBuffer = NULL;
	m_pVertexLayout = NULL;
	m_pDepthStencil = NULL;
	m_diffuseTexture[0] = NULL;
	m_diffuseTexture[1] = NULL;
	m_distDirTexture = NULL;
	m_otherTexture = NULL;
	diffTex = 0;
	diff2Tex = 0;
	m_greyValue = 0.5;
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

VSCombinedObject::~VSCombinedObject(void)
{

}


void VSCombinedObject::RenderDiffusion(ID3D10Device *pd3dDevice)
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

	// Draw first object's curves

	// set the shader variables, they are valid through the whole rendering pipeline
	V( g_pScale->SetFloat(g_vsObj1->m_scale) );
	V( g_pPan->SetFloatVector(g_vsObj1->m_pan) );
	V( m_pDiffX->SetFloat( g_vsObj1->m_sizeX ) );
	V( m_pDiffY->SetFloat( g_vsObj1->m_sizeY ) );
	V( m_pPolySize->SetFloat( g_vsObj1->m_polySize ) );
	V( m_pGreyValue->SetFloat(m_greyValue));

	// render the triangles to the highest input texture level (assuming they are already defined!)
	ID3D10InputLayout* pCurveVertexLayout;
	m_pDrawVectorsTechnique->GetPassByIndex( 0 )->GetDesc( &PassDesc );
	if( FAILED( pd3dDevice->CreateInputLayout( InputCurveElements, InputCurveElementCount, PassDesc.pIAInputSignature, PassDesc.IAInputSignatureSize, &pCurveVertexLayout ) ) )
		return;
	pd3dDevice->IASetInputLayout( pCurveVertexLayout );
  	ID3D10Buffer *pVertexBuffer;
	V( g_vsObj1->m_pMeshCurves->GetDeviceVertexBuffer(0, &pVertexBuffer) );
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
		pd3dDevice->Draw( g_vsObj1->m_pMeshCurves->GetVertexCount(), 0 );
	}
	
	// Draw second object's curves
			
	// set the shader variables, they are valid through the whole rendering pipeline
	V( g_pScale->SetFloat(g_vsObj2->m_scale) );
	V( g_pPan->SetFloatVector(g_vsObj2->m_pan) );
	V( m_pDiffX->SetFloat( g_vsObj2->m_sizeX ) );
	V( m_pDiffY->SetFloat( g_vsObj2->m_sizeY ) );
	V( m_pPolySize->SetFloat( g_vsObj2->m_polySize ) );
	V( m_pGreyValue->SetFloat(m_greyValue));

	// render the triangles to the highest input texture level (assuming they are already defined!)
	
	V( g_vsObj2->m_pMeshCurves->GetDeviceVertexBuffer(0, &pVertexBuffer) );
	stride = sizeof(CURVE_Vertex );
	offset = 0;
	pd3dDevice->IASetVertexBuffers( 0, 1, &pVertexBuffer, &stride, &offset );
	pd3dDevice->IASetPrimitiveTopology( D3D10_PRIMITIVE_TOPOLOGY_LINELIST );
	pd3dDevice->RSSetViewports( 1, &m_vp );
	
	//construct the curve triangles in the geometry shader and render them directly

	destTexTV[0] = m_diffuseTextureTV[1-diffTex];
	destTexTV[1] = m_distDirTextureTV;
	destTexTV[2] = m_otherTextureTV;
	pd3dDevice->OMSetRenderTargets( 3, destTexTV, m_pDepthStencilView );
	m_pDrawVectorsTechnique->GetDesc( &techDesc );
	for(UINT p=0; p<techDesc.Passes; ++p)
	{
		m_pDrawVectorsTechnique->GetPassByIndex( p )->Apply(0);
		pd3dDevice->Draw( g_vsObj2->m_pMeshCurves->GetVertexCount(), 0 );
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
		V( g_vsObj1->m_pPolySize->SetFloat( 1.0 -(float)(i)/(float)diffSteps ) );
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

	// draw Isosurfaces
	if(g_showIso)
	{
		pd3dDevice->OMSetRenderTargets( 1, &m_diffuseTextureTV[1-diffTex], NULL );
		V( m_pInTex[0]->SetResource( m_diffuseTextureRV[diffTex] ) );
		V( m_pInTex[1]->SetResource( m_otherTextureRV ) );
		diffTex = 1-diffTex;
		V( m_pDiffTex->SetResource( m_distDirTextureRV ) );
		m_pIsoSurfaceTechnique->GetDesc( &techDesc );
		for(UINT p=0; p<techDesc.Passes; ++p)
		{
			m_pIsoSurfaceTechnique->GetPassByIndex( p )->Apply(0);
			pd3dDevice->Draw( 3*m_pMeshDiff->GetFaceCount(), 0 );
		}
	}

	//restore old render targets
	pd3dDevice->OMSetRenderTargets( 1,  &old_pRTV,  old_pDSV );
	pd3dDevice->RSSetViewports( NumViewports, &pViewports[0]);
}


// this renders the final image to the screen
void VSCombinedObject::Render(ID3D10Device *pd3dDevice)
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


void VSCombinedObject::SetupTextures(ID3D10Device *pd3dDevice, ID3D10Effect* g_pEffect10, int sizeX, int sizeY, bool bw)
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
	m_pIsoSurfaceTechnique = g_pEffect10->GetTechniqueByName("Isosurface");
	m_pDiffTex = g_pEffect10->GetVariableByName( "g_diffTex" )->AsShaderResource();
	for (int i=0; i<3; i++)
		m_pInTex[i] = (g_pEffect10->GetVariableByName( "g_inTex" ))->GetElement(i)->AsShaderResource();
	m_pDiffX = g_pEffect10->GetVariableByName( "g_diffX")->AsScalar();
	m_pDiffY = g_pEffect10->GetVariableByName( "g_diffY")->AsScalar();
	g_pScale = g_pEffect10->GetVariableByName( "g_scale")->AsScalar();
	m_pPolySize = g_pEffect10->GetVariableByName( "g_polySize")->AsScalar();
	m_pGreyValue = g_pEffect10->GetVariableByName("g_greyValue")->AsScalar();
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

	if (m_pMeshCurves == NULL)
	{
		if(bw)
		{
			g_vsObj1->ReadVectorFile( &g_fileObj1[0], 1);
			g_vsObj2->ReadVectorFile( &g_fileObj2[0], 2);
		}
		else
		{
			g_vsObj1->ReadVectorFile( &g_fileObj1[0], 0);
			g_vsObj2->ReadVectorFile( &g_fileObj2[0], 0);
		}
		g_vsObj1->ConstructCurves(pd3dDevice);
		g_vsObj2->ConstructCurves(pd3dDevice);
	}
}

void VSCombinedObject::ChangeControl()
{
	if(g_Obj1IsControlled)
	{
		g_controlledObj = g_vsObj2;
	}
	else
	{
		g_controlledObj = g_vsObj1;
	}
	g_Obj1IsControlled = !g_Obj1IsControlled;
		
}