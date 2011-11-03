//--------------------------------------------------------------------------------------
// File: VolSurfaces10.cpp
//
// This sample shows a simple example of the Microsoft Direct3D's High-Level 
// Shader Language (HLSL) using the Effect interface. 
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//--------------------------------------------------------------------------------------
#define D3D_DEBUG_INFO

#include "Globals.h"
#include "DXUT.h"
#include "DXUTcamera.h"
#include "DXUTgui.h"
#include "DXUTsettingsDlg.h"
#include "SDKmisc.h"
#include "SDKMesh.h"
#include "VSCombinedObject.h"
#include "resource.h"
#include <string>


//--------------------------------------------------------------------------------------
// Global variables
//--------------------------------------------------------------------------------------
CDXUTDialogResourceManager g_DialogResourceManager; // manager for shared resources of dialogs
CModelViewerCamera      g_Camera;               // A model viewing camera
CDXUTDirectionWidget    g_LightControl;
CD3DSettingsDlg         g_D3DSettingsDlg;       // Device settings dialog
CDXUTDialog             g_HUD;                  // manages the 3D   
CDXUTDialog             g_SampleUI;             // dialog for sample specific controls
float					g_showMenue = true;
bool					g_mouseLButtonDown = false;
bool					g_blackAndWhite = false;
bool					g_isoVisible = false;

// Direct3D9 resources
CDXUTTextHelper*        g_pTxtHelper = NULL;

// Direct3D10 resources
ID3D10Device*			g_device = NULL;
ID3DX10Font*            g_pFont10 = NULL;       
ID3DX10Sprite*          g_pSprite10 = NULL;
VSCombinedObject*		g_vsCombinedObj;
ID3D10RasterizerState*	g_pRasterState;
ID3D10Effect*           g_pEffect10 = NULL;


//--------------------------------------------------------------------------------------
// UI control IDs
//--------------------------------------------------------------------------------------
#define IDC_TOGGLEFULLSCREEN     1
#define IDC_TOGGLEREF            3
#define IDC_CHANGEDEVICE         4
#define IDC_DIFF_STEPS           6
#define IDC_DIFF_STEPS_STATIC    7
#define IDC_CHANGE_IMAGECONTROL  8
#define IDC_CHANGE_BW			 9
#define IDC_SHOW_ISO			10
#define IDC_ISO_VALUE			11
#define IDC_ISO_VALUE_STATIC	12

//--------------------------------------------------------------------------------------
// Forward declarations 
//--------------------------------------------------------------------------------------
bool    CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext );
void    CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext );
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext );
void    CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext );
void    CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext );
bool    CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext );
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext );
void    CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext );
void    CALLBACK OnD3D10DestroyDevice( void* pUserContext );
void    CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext );
void    InitApp();
void    RenderText();

//--------------------------------------------------------------------------------------
// Entry point to the program. Initializes everything and goes into a message processing 
// loop. Idle time is used to render the scene.
//--------------------------------------------------------------------------------------
int WINAPI wWinMain( HINSTANCE hInstance, HINSTANCE hPrevInstance, LPWSTR lpCmdLine, int nCmdShow )
{
    // Enable run-time memory check for debug builds.
#if defined(DEBUG) | defined(_DEBUG)
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // DXUT will create and use the best device (either D3D9 or D3D10) 
    // that is available on the system depending on which D3D callbacks are set below

    // Set DXUT callbacks
    DXUTSetCallbackDeviceChanging( ModifyDeviceSettings );
    DXUTSetCallbackMsgProc( MsgProc );
    DXUTSetCallbackKeyboard( OnKeyboard );
    DXUTSetCallbackFrameMove( OnFrameMove );

    DXUTSetCallbackD3D10DeviceAcceptable( IsD3D10DeviceAcceptable );
    DXUTSetCallbackD3D10DeviceCreated( OnD3D10CreateDevice );
    DXUTSetCallbackD3D10SwapChainResized( OnD3D10ResizedSwapChain );
    DXUTSetCallbackD3D10FrameRender( OnD3D10FrameRender );
    DXUTSetCallbackD3D10SwapChainReleasing( OnD3D10ReleasingSwapChain );
    DXUTSetCallbackD3D10DeviceDestroyed( OnD3D10DestroyDevice );

    InitApp();
    DXUTInit( true, true, NULL ); // Parse the command line, show msgboxes on error, no extra command line params
    DXUTSetCursorSettings( true, true ); // Show the cursor and clip it when in full screen
    DXUTCreateWindow( L"Diffusion Curve Viewer" );
    DXUTCreateDevice( true, 800, 800 );
    DXUTMainLoop(); // Enter into the DXUT render loop

    return DXUTGetExitCode();
}


//--------------------------------------------------------------------------------------
// Initialize the app 
//--------------------------------------------------------------------------------------
void InitApp()
{
    // Initialize dialogs
    g_D3DSettingsDlg.Init( &g_DialogResourceManager );
    g_HUD.Init( &g_DialogResourceManager );
    g_SampleUI.Init( &g_DialogResourceManager );

	g_HUD.SetCallback( OnGUIEvent ); 
	int iY = 10; 

	g_SampleUI.SetCallback( OnGUIEvent ); 
	iY = 0; 
    
	WCHAR btn[100];
	StringCchPrintf( btn, 100, L"Change controlling image");
	g_SampleUI.AddButton( IDC_CHANGE_IMAGECONTROL, btn, 10, iY, 150, 20);
	
	WCHAR sz[100];
	StringCchPrintf( sz, 100, L"Diffusion steps: %d", 8 ); 
    g_SampleUI.AddStatic( IDC_DIFF_STEPS_STATIC, sz, 25, iY += 48, 125, 22 );
    g_SampleUI.AddSlider( IDC_DIFF_STEPS, 40, iY += 24, 100, 22, 0, 400, 4 );
	
	WCHAR chbxbw[100];
	StringCchPrintf( chbxbw, 100, L"Black/White images");
	g_SampleUI.AddCheckBox(IDC_CHANGE_BW, chbxbw, 10, iY += 48, 150, 20);

	WCHAR chbxiso[100];
	StringCchPrintf( chbxiso, 100, L"Show isosurfaces");
	g_SampleUI.AddCheckBox(IDC_SHOW_ISO, chbxiso, 10, iY += 24, 150, 20);
	g_SampleUI.GetCheckBox(IDC_SHOW_ISO)->m_bVisible = false;

	WCHAR siso[100];
	StringCchPrintf( siso, 100, L"Grey value:");
	g_SampleUI.AddStatic(IDC_ISO_VALUE_STATIC, siso, 25, iY += 24, 125, 22);
	g_SampleUI.AddSlider(IDC_ISO_VALUE, 40, iY += 24, 100, 22);
	g_SampleUI.GetStatic(IDC_ISO_VALUE_STATIC)->m_bVisible = false;
	g_SampleUI.GetSlider(IDC_ISO_VALUE)->m_bVisible = false;
}


//--------------------------------------------------------------------------------------
// Called right before creating a D3D9 or D3D10 device, allowing the app to modify the device settings as needed
//--------------------------------------------------------------------------------------
bool CALLBACK ModifyDeviceSettings( DXUTDeviceSettings* pDeviceSettings, void* pUserContext )
{
	pDeviceSettings->d3d10.sd.SampleDesc.Count = 1;
	pDeviceSettings->d3d10.sd.SampleDesc.Quality = 0;

	if( DXUT_D3D9_DEVICE == pDeviceSettings->ver )
    {
        D3DCAPS9 Caps;
        IDirect3D9 *pD3D = DXUTGetD3D9Object();
        pD3D->GetDeviceCaps( pDeviceSettings->d3d9.AdapterOrdinal, pDeviceSettings->d3d9.DeviceType, &Caps );

        // If device doesn't support HW T&L or doesn't support 1.1 vertex shaders in HW 
        // then switch to SWVP.
        if( (Caps.DevCaps & D3DDEVCAPS_HWTRANSFORMANDLIGHT) == 0 ||
            Caps.VertexShaderVersion < D3DVS_VERSION(1,1) )
        {
            pDeviceSettings->d3d9.BehaviorFlags = D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }

        // Debugging vertex shaders requires either REF or software vertex processing 
        // and debugging pixel shaders requires REF.  
#ifdef DEBUG_VS
        if( pDeviceSettings->d3d9.DeviceType != D3DDEVTYPE_REF )
        {
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_HARDWARE_VERTEXPROCESSING;
            pDeviceSettings->d3d9.BehaviorFlags &= ~D3DCREATE_PUREDEVICE;                            
            pDeviceSettings->BehaviorFlags |= D3DCREATE_SOFTWARE_VERTEXPROCESSING;
        }
#endif
#ifdef DEBUG_PS
        pDeviceSettings->d3d9.DeviceType = D3DDEVTYPE_REF;
#endif
    }
    else
    {
        // Uncomment this to get debug information from D3D10
        //pDeviceSettings->d3d10.CreateFlags |= D3D10_CREATE_DEVICE_DEBUG;
    }

    // For the first device created if its a REF device, optionally display a warning dialog box
    static bool s_bFirstTime = true;
    if( s_bFirstTime )
    {
        s_bFirstTime = false;
        if( (DXUT_D3D9_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d9.DeviceType == D3DDEVTYPE_REF) ||
            (DXUT_D3D10_DEVICE == pDeviceSettings->ver && pDeviceSettings->d3d10.DriverType == D3D10_DRIVER_TYPE_REFERENCE) )
            DXUTDisplaySwitchingToREFWarning( pDeviceSettings->ver );
    }

    return true;
}


//--------------------------------------------------------------------------------------
// Handle updates to the scene.  This is called regardless of which D3D API is used
//--------------------------------------------------------------------------------------
void CALLBACK OnFrameMove( double fTime, float fElapsedTime, void* pUserContext )
{
	g_Camera.FrameMove( fElapsedTime );
}


//--------------------------------------------------------------------------------------
// Render the help and statistics text
//--------------------------------------------------------------------------------------
void RenderText()
{
    g_pTxtHelper->Begin();
    g_pTxtHelper->SetInsertionPos( 2, 0 );
    g_pTxtHelper->SetForegroundColor( D3DXCOLOR( 1.0f, 0.0f, 0.0f, 1.0f ) );
    g_pTxtHelper->DrawTextLine( DXUTGetFrameStats( true ) );
//    g_pTxtHelper->DrawTextLine( DXUTGetDeviceStats() );    
//	g_pTxtHelper->DrawFormattedTextLine( L"Pixel fill ratio: %f", g_vsObj->m_fillRatio ); 
    g_pTxtHelper->End();
}


//--------------------------------------------------------------------------------------
// Handle messages to the application
//--------------------------------------------------------------------------------------
LRESULT CALLBACK MsgProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam, bool* pbNoFurtherProcessing, void* pUserContext )
{
    // Pass messages to dialog resource manager calls so GUI state is updated correctly
    *pbNoFurtherProcessing = g_DialogResourceManager.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    // Pass messages to settings dialog if its active
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.MsgProc( hWnd, uMsg, wParam, lParam );
        return 0;
    }

    // Give the dialogs a chance to handle the message first
    *pbNoFurtherProcessing = g_HUD.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;
    *pbNoFurtherProcessing = g_SampleUI.MsgProc( hWnd, uMsg, wParam, lParam );
    if( *pbNoFurtherProcessing )
        return 0;

    g_LightControl.HandleMessages( hWnd, uMsg, wParam, lParam );

    // Pass all remaining windows messages to camera so it can respond to user input
    g_Camera.HandleMessages( hWnd, uMsg, wParam, lParam );

    return 0;
}


//--------------------------------------------------------------------------------------
// Handle key presses
//--------------------------------------------------------------------------------------
void CALLBACK OnKeyboard( UINT nChar, bool bKeyDown, bool bAltDown, void* pUserContext )
{
	static int cVertex = 0;

    if( bKeyDown )
    {
       switch( nChar )
        {
            case 'M': g_showMenue = !g_showMenue; break;
		}
    }
}


//--------------------------------------------------------------------------------------
// Handles the GUI events
//--------------------------------------------------------------------------------------
void CALLBACK OnGUIEvent( UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext )
{   
    switch( nControlID )
    {
        case IDC_TOGGLEFULLSCREEN: DXUTToggleFullScreen(); break;
        case IDC_TOGGLEREF:        DXUTToggleREF(); break;
        case IDC_CHANGEDEVICE:     g_D3DSettingsDlg.SetActive( !g_D3DSettingsDlg.IsActive() ); break;
 
 		case IDC_DIFF_STEPS: 
			g_vsCombinedObj->diffSteps = g_SampleUI.GetSlider( IDC_DIFF_STEPS )->GetValue();
            WCHAR sz[100];
            StringCchPrintf( sz, 100, L"Diffusion steps: %d", g_vsCombinedObj->diffSteps ); 
            g_SampleUI.GetStatic( IDC_DIFF_STEPS_STATIC )->SetText( sz );
            break;
		case IDC_CHANGE_IMAGECONTROL:
			g_vsCombinedObj->ChangeControl();
			break;
		case IDC_CHANGE_BW:
			g_blackAndWhite = !g_blackAndWhite;
			if(g_blackAndWhite)
			{
				g_vsCombinedObj->g_vsObj1->ReadVectorFile( &g_vsCombinedObj->g_fileObj1[0], 1);
				g_vsCombinedObj->g_vsObj2->ReadVectorFile( &g_vsCombinedObj->g_fileObj2[0], 2);
				g_SampleUI.GetCheckBox(IDC_SHOW_ISO)->m_bVisible = true;
			}
			else
			{
				g_vsCombinedObj->g_vsObj1->ReadVectorFile( &g_vsCombinedObj->g_fileObj1[0], 0);
				g_vsCombinedObj->g_vsObj2->ReadVectorFile( &g_vsCombinedObj->g_fileObj2[0], 0);
				g_SampleUI.GetCheckBox(IDC_SHOW_ISO)->m_bVisible = false;
				g_SampleUI.GetSlider(IDC_ISO_VALUE)->m_bVisible = false;
				g_SampleUI.GetStatic(IDC_ISO_VALUE_STATIC)->m_bVisible = false;
			}
			g_vsCombinedObj->g_vsObj1->ConstructCurves(g_device);
			g_vsCombinedObj->g_vsObj2->ConstructCurves(g_device);
			break;
		case IDC_SHOW_ISO:
			if(g_isoVisible)
			{
				g_SampleUI.GetSlider(IDC_ISO_VALUE)->m_bVisible = false;
				g_SampleUI.GetStatic(IDC_ISO_VALUE_STATIC)->m_bVisible = false;
				g_vsCombinedObj->g_showIso = false;
			}
			else
			{
				g_SampleUI.GetSlider(IDC_ISO_VALUE)->m_bVisible = true;
				g_SampleUI.GetStatic(IDC_ISO_VALUE_STATIC)->m_bVisible = true;
				g_vsCombinedObj->g_showIso = true;
			}
			g_isoVisible = !g_isoVisible;
			break;
		case IDC_ISO_VALUE:
			int value = g_SampleUI.GetSlider( IDC_ISO_VALUE )->GetValue();
			g_vsCombinedObj->m_greyValue = value/100.0;
			break;
	}
}


//--------------------------------------------------------------------------------------
// Reject any D3D10 devices that aren't acceptable by returning false
//--------------------------------------------------------------------------------------
bool CALLBACK IsD3D10DeviceAcceptable( UINT Adapter, UINT Output, D3D10_DRIVER_TYPE DeviceType, DXGI_FORMAT BackBufferFormat, bool bWindowed, void* pUserContext )
{
    return true;
}



//--------------------------------------------------------------------------------------
// Create any D3D10 resources that aren't dependant on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10CreateDevice( ID3D10Device* pd3dDevice, const DXGI_SURFACE_DESC *pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

	g_device = pd3dDevice;

    V_RETURN( g_DialogResourceManager.OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D10CreateDevice( pd3dDevice ) );
    V_RETURN( D3DX10CreateFont( pd3dDevice, 15, 0, FW_BOLD, 1, FALSE, DEFAULT_CHARSET, 
                                OUT_DEFAULT_PRECIS, DEFAULT_QUALITY, DEFAULT_PITCH | FF_DONTCARE, 
                                L"Arial", &g_pFont10 ) );
    V_RETURN( D3DX10CreateSprite( pd3dDevice, 512, &g_pSprite10 ) );
    g_pTxtHelper = new CDXUTTextHelper( NULL, NULL, g_pFont10, g_pSprite10, 15 );

    V_RETURN( CDXUTDirectionWidget::StaticOnD3D10CreateDevice( pd3dDevice ) );

    // Read the D3DX effect file
    WCHAR str[MAX_PATH];
	ID3D10Blob *pErrBlob = NULL;
    V_RETURN( DXUTFindDXSDKMediaFileCch( str, MAX_PATH, L"Effect_Undistort.fx" ) );
    hr = D3DX10CreateEffectFromFile( str, NULL, NULL, "fx_4_0", D3D10_SHADER_ENABLE_STRICTNESS, 0, pd3dDevice, NULL, NULL, &g_pEffect10, &pErrBlob, NULL );
	if(FAILED ( hr ))
	{
		std::string errStr((LPCSTR)pErrBlob->GetBufferPointer(), pErrBlob->GetBufferSize());
		WCHAR err[256];
		MultiByteToWideChar(CP_ACP, 0, errStr.c_str(), (int)errStr.size(), err, errStr.size());
		MessageBox( NULL, (LPCWSTR)err, L"Error", MB_OK );
		return hr;
	}

    // Setup the vector image and the display
	UINT width = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Width : DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	UINT height = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height : DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	g_vsCombinedObj = new VSCombinedObject(pd3dDevice); 

    D3D10_RASTERIZER_DESC rasterizerState;
    rasterizerState.FillMode = D3D10_FILL_SOLID;
    rasterizerState.CullMode = D3D10_CULL_NONE;
    rasterizerState.FrontCounterClockwise = true;
    rasterizerState.DepthBias = false;
    rasterizerState.DepthBiasClamp = 0;
    rasterizerState.SlopeScaledDepthBias = 0;
    rasterizerState.DepthClipEnable = true;
    rasterizerState.ScissorEnable = false;
    rasterizerState.MultisampleEnable = false;
    rasterizerState.AntialiasedLineEnable = false;
    pd3dDevice->CreateRasterizerState( &rasterizerState, &g_pRasterState );

    return S_OK;
}


//--------------------------------------------------------------------------------------
// Create any D3D10 resources that depend on the back buffer
//--------------------------------------------------------------------------------------
HRESULT CALLBACK OnD3D10ResizedSwapChain( ID3D10Device* pd3dDevice, IDXGISwapChain *pSwapChain, const DXGI_SURFACE_DESC* pBackBufferSurfaceDesc, void* pUserContext )
{
    HRESULT hr;

    V_RETURN( g_DialogResourceManager.OnD3D10ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );
    V_RETURN( g_D3DSettingsDlg.OnD3D10ResizedSwapChain( pd3dDevice, pBackBufferSurfaceDesc ) );

    // Setup the camera's projection parameters
    float fAspectRatio = 1.0;//pBackBufferSurfaceDesc->Width / (FLOAT)pBackBufferSurfaceDesc->Height;
    g_Camera.SetProjParams( D3DX_PI/4, fAspectRatio, 0.1f, 10.0f );
    g_Camera.SetWindow( pBackBufferSurfaceDesc->Width, pBackBufferSurfaceDesc->Height );
    g_Camera.SetButtonMasks( MOUSE_RIGHT_BUTTON, MOUSE_WHEEL, MOUSE_MIDDLE_BUTTON );

    g_HUD.SetLocation( pBackBufferSurfaceDesc->Width-170, 0 );
    g_HUD.SetSize( 170, 170 );
    g_SampleUI.SetLocation( pBackBufferSurfaceDesc->Width-170, pBackBufferSurfaceDesc->Height-300 );
    g_SampleUI.SetSize( 170, 300 );

	// resize the texture so that it fits to the current screen size
	UINT width = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Width : DXUTGetDXGIBackBufferSurfaceDesc()->Width;
	UINT height = ( DXUTIsAppRenderingWithD3D9() ) ? DXUTGetD3D9BackBufferSurfaceDesc()->Height : DXUTGetDXGIBackBufferSurfaceDesc()->Height;
	g_vsCombinedObj->SetupTextures(pd3dDevice, g_pEffect10, width, height, g_blackAndWhite);
	g_vsCombinedObj->g_vsObj1->SetupTextures(pd3dDevice, g_pEffect10, width, height);
	g_vsCombinedObj->g_vsObj2->SetupTextures(pd3dDevice, g_pEffect10, width, height);
    return S_OK;
}


//--------------------------------------------------------------------------------------
// Render the scene using the D3D10 device
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10FrameRender( ID3D10Device* pd3dDevice, double fTime, float fElapsedTime, void* pUserContext )
{
    HRESULT hr;

    // If the settings dialog is being shown, then render it instead of rendering the app's scene
    if( g_D3DSettingsDlg.IsActive() )
    {
        g_D3DSettingsDlg.OnRender( fElapsedTime );
        return;
    }

    if( g_Camera.m_nMouseWheelDelta && g_Camera.m_nZoomButtonMask == MOUSE_WHEEL )
	{
		g_vsCombinedObj->g_controlledObj->m_pan /= g_vsCombinedObj->g_controlledObj->m_scale;
	   	g_vsCombinedObj->g_controlledObj->m_scale += g_vsCombinedObj->g_controlledObj->m_scale*g_Camera.m_nMouseWheelDelta * 0.2;
		g_vsCombinedObj->g_controlledObj->m_pan *= g_vsCombinedObj->g_controlledObj->m_scale;
	    g_Camera.m_nMouseWheelDelta = 0;
		g_vsCombinedObj->g_controlledObj->m_scale = max(0.01, g_vsCombinedObj->g_controlledObj->m_scale);
	}

	if ((!g_Camera.IsMouseRButtonDown()) && (g_mouseLButtonDown == true))
	{
		g_mouseLButtonDown = false;
	}
	else if (g_Camera.IsMouseRButtonDown())
		g_mouseLButtonDown = true;
	else
		g_mouseLButtonDown = false;

    if( g_Camera.IsBeingDragged() )
	{
		float ff = 1.0f;
		if (g_Camera.IsMouseRButtonDown())
			ff = 0.127f; 
		float xFac = ff*4.0f/g_vsCombinedObj->g_controlledObj->m_sizeX;
		float yFac = ff*4.0f/g_vsCombinedObj->g_controlledObj->m_sizeY;
		g_vsCombinedObj->g_controlledObj->m_pan += D3DXVECTOR2(xFac*g_Camera.m_vMouseDelta.x,-yFac*g_Camera.m_vMouseDelta.y);
		g_Camera.m_vMouseDelta.x = 0;
		g_Camera.m_vMouseDelta.y = 0;
	}
	
	g_vsCombinedObj->m_polySize = 2.3; //render each polygon in full screen size
	g_vsCombinedObj->RenderDiffusion(pd3dDevice);
	g_vsCombinedObj->Render(pd3dDevice);

	if (g_showMenue)
	{
		DXUT_BeginPerfEvent( DXUT_PERFEVENTCOLOR, L"HUD / Stats" );
	    g_HUD.OnRender( fElapsedTime ); 
		g_SampleUI.OnRender( fElapsedTime );
 		RenderText();
		DXUT_EndPerfEvent();
	}
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10ResizedSwapChain 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10ReleasingSwapChain( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10ReleasingSwapChain();
}


//--------------------------------------------------------------------------------------
// Release D3D10 resources created in OnD3D10CreateDevice 
//--------------------------------------------------------------------------------------
void CALLBACK OnD3D10DestroyDevice( void* pUserContext )
{
    g_DialogResourceManager.OnD3D10DestroyDevice();
    g_D3DSettingsDlg.OnD3D10DestroyDevice();
    CDXUTDirectionWidget::StaticOnD3D10DestroyDevice();
    DXUTGetGlobalResourceCache().OnDestroyDevice();
    SAFE_DELETE( g_pTxtHelper );
    SAFE_RELEASE( g_pFont10 );
    SAFE_RELEASE( g_pSprite10 );
    SAFE_RELEASE( g_pEffect10 );
	SAFE_DELETE( g_vsCombinedObj );
}



