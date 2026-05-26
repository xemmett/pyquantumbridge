// -------------------------------------------------------------------------------------------------
//! \file PPIWindow.h
//! Creates and manages PPI Window for rendering spoke data

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// -----------------------------------------------------------------------------------------------------------
// Include Files

#include "stdafx.h"

#include "../../QuantumLib/QuantumLib/Include/QuantumLibTypes.h"
#include "QuantumClientPPI.h"
#include "PPIWindow.h"
#include "Palettes.h"

#include "d3d9.h"
// -----------------------------------------------------------------------------------------------------------
// clear alpha as the colour is copied
#define WRITE_DST()       { *pDst++ = *pSrc & 0x00ffffff; }

// simple clear
#define CLEAR_SRC()       *pSrc++ = 0;

#define SAFE_RELEASE(p)   { if (p) { (p)->Release(); p = 0; } }

extern HINSTANCE hInst;

// -----------------------------------------------------------------------------------------------------------

BOOL CALLBACK PPIProc(HWND hWnd,        // Handle of dialog box
	UINT uMsg,							// Message identifier
	WPARAM wParam,						// First message parameter
	LPARAM lParam);						// Second message parameter

// -----------------------------------------------------------------------------------------------------------
//! \fn PPIWindow
//! Initialise all settings 
//! Create a thread and Mutex for the PPI Window
PPIWindow::PPIWindow(HWND hParent) :
m_pDirect3D9(0),
m_pd3dDevice(0),
m_pSystemRectTexture(0),
m_pVideoRectTexture(0),
m_pR2PVertexBuffer(0),
m_pPolarTexture(0),
m_pProjVerticesBuffer(0),
m_pRectVerticesBuffer(0),
m_pBlendVerticesBuffer(0),
m_PixelsPerSpoke(0),       // pixel radius of PPI
m_cx(0),
m_cy(0),
m_Width(0),                // window size
m_Height(0),
m_ClearRequired(false),
m_hParent(hParent),
m_hWnd(0),
m_hInstance(hInst),
m_hEvent(INVALID_HANDLE_VALUE),
m_hThread(INVALID_HANDLE_VALUE),
m_ThreadID(0),
m_bFirstSpoke(true),
m_FirstBlockToDraw(0),
m_bVRMDirty(true),
m_VRM(1024 * 2 / 3),
m_bPaused(false),
m_PaletteIndex(0)
{
	memset(m_SystemSpokeCache, 0, sizeof(m_SystemSpokeCache));

	m_hEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	m_hThread = CreateThread(
		0,						// Security attributes
		(DWORD)0,				// Initial stack size
		WindowMessageLoop,		// Thread start address
		(LPVOID) this,			// Thread parameter
		(DWORD)0,				// Creation flags
		&m_ThreadID);			// Thread identifier

	memset(&m_d3dpp, 0, sizeof(m_d3dpp));

	// Wait until the window has been initialised
	WaitForSingleObject(m_hEvent, INFINITE);
}
// -----------------------------------------------------------------------------------------------------------

PPIWindow::~PPIWindow()
{
	SendMessage(m_hParent, WM_USER_PPI_WINDOW_CLOSED, NULL, NULL);
}
// -----------------------------------------------------------------------------------------------------------
void PPIWindow::Close()
{
	SendMessage(m_hWnd, WM_CLOSE, NULL, NULL);
}
// -----------------------------------------------------------------------------------------------------------
//! \fn PPIRegisterClass
//! Register class for PPI window
const wchar_t CLASS_NAME[] = L"PPI Window";

ATOM PPIWindow::PPIRegisterClass(HINSTANCE hInstance)
{
	WNDCLASSEX wcex;

	wcex.cbSize = sizeof(WNDCLASSEX);

	wcex.style = CS_HREDRAW | CS_VREDRAW;
	wcex.lpfnWndProc = (WNDPROC)PPIProc;
	wcex.cbClsExtra = 0;
	wcex.cbWndExtra = 0;
	wcex.hInstance = hInstance;
	wcex.hIcon = LoadIcon(hInstance, IDI_APPLICATION);
	wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wcex.lpszMenuName = "PPI";
	wcex.lpszClassName = (LPCSTR)CLASS_NAME;
	wcex.hIconSm = LoadIcon(wcex.hInstance, IDI_APPLICATION);

	return RegisterClassEx(&wcex);
}

// -----------------------------------------------------------------------------------------------------------
//! \fn WindowMessageLoop
//! Windows message loop associated to PPIWindow thread
//! Register class and create window
DWORD __stdcall PPIWindow::WindowMessageLoop(LPVOID lpvThreadParm)
{
	PPIWindow *pPPIWindow;     // The owner renderer object

	// Cast the thread parameter to be our owner object
	pPPIWindow = (PPIWindow *)lpvThreadParm;
	ATOM PPIClass = pPPIWindow->PPIRegisterClass(hInst);
	if (PPIClass == 0)
	{
		return FALSE;
	}

	HWND hWnd = CreateWindowEx(0, (LPCSTR)CLASS_NAME, "Radar PPI", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,500, 500, pPPIWindow->m_hParent, NULL, hInst, pPPIWindow);
	pPPIWindow->m_hWnd = hWnd;

	if (pPPIWindow->m_hWnd != 0)
	{
		// Initialise the window, then signal the constructor that it can
		// continue and then unlock the object's critical section and process messages
		pPPIWindow->InitialiseWindow();
	}

	SetEvent(pPPIWindow->m_hEvent);

	if (hWnd != 0)
	{
		ShowWindow(hWnd, SW_SHOWNORMAL);
		UpdateWindow(hWnd);
	}

	// Run the message loop.
		MSG msg = {};
	while (GetMessage(&msg, 0, (UINT) 0, (UINT) 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	CloseHandle(pPPIWindow->m_hEvent);
	//ExitThread(0);
	return 0;
}
// -----------------------------------------------------------------------------------------------------------
//! \fn PPIProc
//! Message handler for PPI window
//! Allows resizing and recentering of PPI
BOOL CALLBACK PPIProc(HWND hWnd,			// Handle of dialog box
	UINT uMsg,								// Message identifier
	WPARAM wParam,							// First message parameter
	LPARAM lParam)							// Second message parameter
{
	PPIWindow *pPPIWindow;					// Pointer to the owning object

	if (uMsg == WM_CREATE)
	{
		pPPIWindow = (PPIWindow *)((LPCREATESTRUCT)lParam)->lpCreateParams;
		SetWindowLongPtr(hWnd, (DWORD)GWLP_USERDATA, (LONG)pPPIWindow);
	}

	// Get the window long that holds our owner pointer
	pPPIWindow = (PPIWindow *)GetWindowLongPtr(hWnd, GWLP_USERDATA);

	int wmId, wmEvent;

	switch (uMsg)
	{
	case WM_COMMAND:
		wmId = LOWORD(wParam);
		wmEvent = HIWORD(wParam);
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
		break;
	case WM_LBUTTONDOWN:
		if (pPPIWindow)
		{
			pPPIWindow->Recentre(lParam);
		}

		break;

	case WM_ENTERSIZEMOVE:
		break;

	case WM_SIZE:
		if (pPPIWindow)
		{
			pPPIWindow->Resize();
		}
		break;


	case WM_EXITSIZEMOVE:
		if (pPPIWindow)
		{
			pPPIWindow->Resize();
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hWnd);
		UnregisterClass((LPCSTR)CLASS_NAME, hInst);
		return (LRESULT)0;

	case WM_PAINT:
		break;
	case WM_DESTROY:
		PostQuitMessage(0);
		break;
	default:
		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}
	return 0;
}
// -----------------------------------------------------------------------------------------------------------
//! \fn InitialiseWindow
//! Create and initialise DirectX Device
HRESULT PPIWindow::InitialiseWindow(void)
{
	HRESULT               hr;
	D3DDISPLAYMODE        d3ddm;

	if ((m_pDirect3D9 = Direct3DCreate9(D3D_SDK_VERSION)) == 0)
	{
		return E_FAIL;
	}

	if (FAILED(m_pDirect3D9->GetAdapterDisplayMode(D3DADAPTER_DEFAULT, &d3ddm)))
	{
		return E_FAIL;
	}
	ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));
	m_d3dpp.Windowed = TRUE;
	m_d3dpp.SwapEffect = D3DSWAPEFFECT_DISCARD;
	m_d3dpp.hDeviceWindow = m_hWnd;
	m_d3dpp.BackBufferFormat = d3ddm.Format;

	hr = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT,
		D3DDEVTYPE_HAL,
		m_hWnd,
		D3DCREATE_HARDWARE_VERTEXPROCESSING,
		&m_d3dpp,
		&m_pd3dDevice);


	if (FAILED(hr))
	{
		// failed??? was it because we asked for hardware vertex processing?

		hr = m_pDirect3D9->CreateDevice(D3DADAPTER_DEFAULT,
			D3DDEVTYPE_HAL,
			m_hWnd,
			D3DCREATE_SOFTWARE_VERTEXPROCESSING,
			&m_d3dpp,
			&m_pd3dDevice);


		if (FAILED(hr))
		{
			m_pDirect3D9->Release();
			return hr;
		}
	}

	if (m_d3dpp.BackBufferFormat != D3DFMT_X8R8G8B8)
	{
		m_pd3dDevice->Release();
		m_pDirect3D9->Release();

		MessageBox(m_hWnd, TEXT("Invalid Back Buffer Format"), TEXT("Error"), MB_OK);

		return E_FAIL;
	}

	hr = InitDeviceObjects();

	if (FAILED(hr))
	{
		//DbgLog((LOG_TRACE, 3, TEXT("Fatal : Failed to InitDeviceObjects")));
		return hr;
	}

	hr = RestoreDeviceObjects();

	if (FAILED(hr))
	{
		//DbgLog((LOG_TRACE, 3, TEXT("Fatal : Failed to RestoreDeviceObjects")));
		return hr;
	}

	return S_OK;


}

//-----------------------------------------------------------------------------------
//! \fn UninitialiseWindow
//! This is called by the worker window thread when it receives a WM_GOODBYE
//! message from the window object destructor to delete all the resources we
//! allocated during initialisation
HRESULT PPIWindow::UninitialiseWindow()
{
	//Pause(TRUE);

	InvalidateDeviceObjects();
	DeleteDeviceObjects();

	SAFE_RELEASE(m_pBlendVerticesBuffer);
	SAFE_RELEASE(m_pProjVerticesBuffer);
	SAFE_RELEASE(m_pRectVerticesBuffer);
	SAFE_RELEASE(m_pR2PVertexBuffer);
	SAFE_RELEASE(m_pVideoRectTexture);
	SAFE_RELEASE(m_pSystemRectTexture);
	SAFE_RELEASE(m_pd3dDevice);
	SAFE_RELEASE(m_pDirect3D9);

	return NOERROR;
}

//-----------------------------------------------------------------------------
//! \fn HandlePossibleSizeChange()
//! Handle change in size of PPI window
HRESULT PPIWindow::HandlePossibleSizeChange()
{
	HRESULT   hr = S_OK;
	RECT      rcClientOld;

	rcClientOld = m_Rect;

	// Update window properties
	GetClientRect(m_hWnd, &m_Rect);

	if (rcClientOld.right - rcClientOld.left != m_Rect.right - m_Rect.left ||
		rcClientOld.bottom - rcClientOld.top != m_Rect.bottom - m_Rect.top)
	{
		// A new window size will require a new backbuffer
		// size, so the 3D structures must be changed accordingly.

		Pause(true);

		m_d3dpp.BackBufferWidth = m_Rect.right - m_Rect.left;
		m_d3dpp.BackBufferHeight = m_Rect.bottom - m_Rect.top;

		if (m_pd3dDevice != NULL)
		{
			hr = Reset3DEnvironment();      // Reset the 3D environment
		}

		m_ClearRequired = TRUE;

		Pause(false);
	}

	return hr;
}
//-----------------------------------------------------------------------------
//! \fn SetPaletteIndex()
//! Sets palette index which will start to be used when the next spoke is rendered
bool PPIWindow::SetPaletteIndex(uint32_t Index)
{
	bool ret = false;
	if (Index < cNumberOfPalettes)
	{
		m_PaletteIndex = Index;
		ret = true;
	}
	return ret;
}

//-----------------------------------------------------------------------------
//! \fn Reset3DEnvironment()
//! Invalidate, Reset and Restore 3D device
HRESULT PPIWindow::Reset3DEnvironment()
{
	HRESULT hr;

	InvalidateDeviceObjects();

	if (FAILED(hr = m_pd3dDevice->Reset(&m_d3dpp)))   // Reset the device
	{
		return hr;
	}

	// Store render target surface desc
	LPDIRECT3DSURFACE9 pBackBuffer;

	m_pd3dDevice->GetBackBuffer(0, 0, D3DBACKBUFFER_TYPE_MONO, &pBackBuffer);
	pBackBuffer->Release();

	//m_bEBLDirty = TRUE;
	m_bVRMDirty = true;

	// Initialize the app's device-dependent objects
	hr = RestoreDeviceObjects();

	if (FAILED(hr))
	{
		InvalidateDeviceObjects();
		return hr;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
//! \fn Cleanup3DEnvironment()
HRESULT PPIWindow::Cleanup3DEnvironment(void)
{
	if (m_pd3dDevice)
	{
		InvalidateDeviceObjects();
		DeleteDeviceObjects();
		m_pd3dDevice->Release();
		m_pd3dDevice = NULL;
	}

	return S_OK;
}

//-----------------------------------------------------------------------------
//!	\fn InitDeviceObjects()
//! paired with DeleteDeviceObjects
//! create Texture. Not lost on a reset
HRESULT PPIWindow::InitDeviceObjects(void)
{
	HRESULT hr;

	// firstly create the buffer for our incomming spokes
	hr = m_pd3dDevice->CreateTexture(QuantumLib::cSamplesPerSpoke, QuantumLib::cSpokesPerScan,
		1,
		0,
		m_d3dpp.BackBufferFormat,
		D3DPOOL_SYSTEMMEM,
		&m_pSystemRectTexture,
		0);

	if (FAILED(hr))
	{
		//DbgLog((LOG_TRACE, 3, TEXT("Fatal : Unable to create system texture")));
		return hr;
	}

	return S_OK;
}
//----------------------------------------------------------------------------
//! DeleteDeviceObjects()
//! paired with InitDeviceObjects
//! delete texture
HRESULT PPIWindow::DeleteDeviceObjects(void)
{
	SAFE_RELEASE(m_pSystemRectTexture);

	return S_OK;
}

//-----------------------------------------------------------------------------
//! \fn RestoreDeviceObjects() 
//! paired with InvalidateDeviceObjectsSaintsForTheCup2019
//! create stuff that could be lost on a reset + state stuff
HRESULT PPIWindow::RestoreDeviceObjects(void)
{
	HRESULT   hr;

	hr = m_pd3dDevice->CreateTexture(QuantumLib::cSamplesPerSpoke, QuantumLib::cSpokesPerScan,
		1,
		0,
		m_d3dpp.BackBufferFormat,
		D3DPOOL_DEFAULT,
		&m_pVideoRectTexture,
		0);

	if (SUCCEEDED(hr))
	{
		// on a resize, we clear the system memory texture and then use this to clear the
		// video memory texture

		ClearSpokes(0, QuantumLib::cSpokesPerScan);
		m_pd3dDevice->UpdateTexture(m_pSystemRectTexture, m_pVideoRectTexture);

		m_ClearRequired = TRUE;
	}

	hr = m_pd3dDevice->CreateTexture(m_d3dpp.BackBufferWidth, m_d3dpp.BackBufferHeight,
		1,
		D3DUSAGE_RENDERTARGET,
		m_d3dpp.BackBufferFormat,
		D3DPOOL_DEFAULT,
		&m_pPolarTexture,
		0);

	// Set up the textures
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLORARG1, D3DTA_TEXTURE);

	m_pd3dDevice->SetSamplerState(0, D3DSAMP_MINFILTER, D3DTEXF_POINT);
	m_pd3dDevice->SetSamplerState(0, D3DSAMP_MAGFILTER, D3DTEXF_POINT);

	m_pd3dDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);

	m_pd3dDevice->SetRenderState(D3DRS_DITHERENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_SPECULARENABLE, FALSE);
	m_pd3dDevice->SetRenderState(D3DRS_ZENABLE, FALSE);

	m_pd3dDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_ZERO);
	m_pd3dDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_SRCALPHA);

	CreateMatrices();

	if (SUCCEEDED(hr))
	{
		GetClientRect(m_hWnd, &m_Rect);

		m_Width = m_Rect.right - m_Rect.left;
		m_Height = m_Rect.bottom - m_Rect.top;

		// bodge the centre to always be in the middle of the window

		m_cx = m_Width / 2;
		m_cy = m_Height / 2;

		// m_PixelsPerSpoke is the radius - fix it to the window size for now

		m_PixelsPerSpoke = (m_Width > m_Height) ? m_Height : m_Width;
		m_PixelsPerSpoke /= 2;
		hr = CreateVertices();
	}

	return S_OK;
}
//----------------------------------------------------------------------------
//! \fn create vertices
//! rectangular to polar mapping
HRESULT PPIWindow::CreateVertices(void)
{
	HRESULT hr;
	if (m_pd3dDevice == 0)
	{
		return E_FAIL;
	}

	SAFE_RELEASE(m_pR2PVertexBuffer);
	SAFE_RELEASE(m_pProjVerticesBuffer);
	SAFE_RELEASE(m_pRectVerticesBuffer);
	SAFE_RELEASE(m_pBlendVerticesBuffer);

	int   radius = m_PixelsPerSpoke;

	//  create the rectangular to polar mapping, use the unit square for vertex coords

	R2PVERTEX * pR2PVertices = m_R2PVertices;

	// create the rectangular to polar mapping vertices

	for (int i = 0; i < BLOCKS_PER_SCAN; i++)
	{
		float   da_s = ((float)(i * M_PIx2) / (float)BLOCKS_PER_SCAN);
		float   da_e = ((float)((i + 1) * M_PIx2) / (float)BLOCKS_PER_SCAN);

		pR2PVertices->p = D3DXVECTOR4((float)m_cx,
			(float)m_cy + 0.5f,
			0.0f,
			1.0f);
		pR2PVertices->tu = 0.0f;
		pR2PVertices->tv = ((float)i / (float)BLOCKS_PER_SCAN);
		pR2PVertices++;

		pR2PVertices->p = D3DXVECTOR4((radius * sinf(da_s)) + m_cx,
			m_cy - (radius * cosf(da_s)),
			0.0f,
			1.0f);
		pR2PVertices->tu = 1.0f;
		pR2PVertices->tv = ((float)i / (float)BLOCKS_PER_SCAN);
		pR2PVertices++;

		pR2PVertices->p = D3DXVECTOR4((radius * sinf(da_e)) + m_cx,
			m_cy - (radius * cosf(da_e)),
			0.0f,
			1.0f);
		pR2PVertices->tu = 1.0f;
		pR2PVertices->tv = ((float)(i + 1) / (float)BLOCKS_PER_SCAN);
		pR2PVertices++;
	}

	hr = m_pd3dDevice->CreateVertexBuffer(sizeof(R2PVERTEX)* BLOCKS_PER_SCAN * 3,
		0,
		D3DFVF_R2PVERTEX,
		D3DPOOL_MANAGED,
		&m_pR2PVertexBuffer,
		0);

	if (SUCCEEDED(hr))
	{
		R2PVERTEX * pPolarVertices;

		hr = m_pR2PVertexBuffer->Lock(0, 0, (VOID**)&pPolarVertices, 0);

		memcpy(pPolarVertices, m_R2PVertices, sizeof(m_R2PVertices));

		m_pR2PVertexBuffer->Unlock();
	}
	else
	{
		//DbgLog((LOG_TRACE, 3, TEXT("Fatal : Unable to create vertex buffer")));
	}

	m_pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);
	m_pd3dDevice->Present(NULL, NULL, NULL, NULL);

	// projected vertices

	m_ProjVertices[0].p = D3DXVECTOR3(-1.0f, 1.0f, 0.0f);
	m_ProjVertices[0].tu = 0.0f;
	m_ProjVertices[0].tv = 0.0f;

	m_ProjVertices[1].p = D3DXVECTOR3(1.0f, 1.0f, 0.0f);
	m_ProjVertices[1].tu = 1.0f;
	m_ProjVertices[1].tv = 0.0f;

	m_ProjVertices[2].p = D3DXVECTOR3(-1.0f, -1.0f, 0.0f);
	m_ProjVertices[2].tu = 0.0f;
	m_ProjVertices[2].tv = 1.0f;

	m_ProjVertices[3].p = D3DXVECTOR3(1.0f, -1.0f, 0.0f);
	m_ProjVertices[3].tu = 1.0f;
	m_ProjVertices[3].tv = 1.0f;

	// Create the vertex buffer

	hr = m_pd3dDevice->CreateVertexBuffer(sizeof(m_ProjVertices),
		0,
		D3DFVF_PROVERTEX,
		D3DPOOL_MANAGED,
		&m_pProjVerticesBuffer,
		0);

	if (SUCCEEDED(hr))
	{
		PROVERTEX * pVertices;

		if (SUCCEEDED(m_pProjVerticesBuffer->Lock(0, 0, (VOID**)&pVertices, 0)))
		{
			memcpy(pVertices, m_ProjVertices, sizeof(m_ProjVertices));

			m_pProjVerticesBuffer->Unlock();
		}
	}

	// rect vertices

	m_RectVertices[0].p = D3DXVECTOR4(0.0f - 0.5f, 0.0f - 0.5f, 0.0f, 1.0f);
	m_RectVertices[0].tu = 0.0f;
	m_RectVertices[0].tv = 0.0f;

	m_RectVertices[1].p = D3DXVECTOR4((float)m_Width - 0.5f, 0.0f - 0.5f, 0.0f, 1.0f);
	m_RectVertices[1].tu = 1.0f;
	m_RectVertices[1].tv = 0.0f;

	m_RectVertices[2].p = D3DXVECTOR4(0.0f - 0.5f, (float)m_Height - 0.5f, 0.0f, 1.0f);
	m_RectVertices[2].tu = 0.0f;
	m_RectVertices[2].tv = 1.0f;

	m_RectVertices[3].p = D3DXVECTOR4((float)m_Width - 0.5f, (float)m_Height - 0.5f, 0.0f, 1.0f);
	m_RectVertices[3].tu = 1.0f;
	m_RectVertices[3].tv = 1.0f;

	// Create the vertex buffer

	hr = m_pd3dDevice->CreateVertexBuffer(sizeof(m_RectVertices),
		0,
		D3DFVF_R2PVERTEX,
		D3DPOOL_MANAGED,
		&m_pRectVerticesBuffer,
		0);

	if (SUCCEEDED(hr))
	{
		R2PVERTEX * pVertices;

		if (SUCCEEDED(m_pRectVerticesBuffer->Lock(0, 0, (VOID**)&pVertices, 0)))
		{
			memcpy(pVertices, m_RectVertices, sizeof(m_RectVertices));

			m_pRectVerticesBuffer->Unlock();
		}
	}

	// blend vertices

	m_BlendVertices[0].p = D3DXVECTOR4((float)m_Rect.left, (float)m_Rect.top, 0.0f, 1.0f);
	m_BlendVertices[0].colour = 0xfa00ff00;

	m_BlendVertices[1].p = D3DXVECTOR4((float)m_Rect.right, (float)m_Rect.top, 0.0f, 1.0f);
	m_BlendVertices[1].colour = 0xfa00ff00;

	m_BlendVertices[2].p = D3DXVECTOR4((float)m_Rect.left, (float)m_Rect.bottom, 0.0f, 1.0f);
	m_BlendVertices[2].colour = 0xfa00ff00;

	m_BlendVertices[3].p = D3DXVECTOR4((float)m_Rect.right, (float)m_Rect.bottom, 0.0f, 1.0f);
	m_BlendVertices[3].colour = 0xfa00ff00;

	// Create the vertex buffer

	m_pd3dDevice->CreateVertexBuffer(sizeof(m_BlendVertices),
		0,
		D3DFVF_SYMVERTEX,
		D3DPOOL_MANAGED,
		&m_pBlendVerticesBuffer,
		0);

	R2PVERTEX * pBlendVertices;

	if (SUCCEEDED(m_pBlendVerticesBuffer->Lock(0, 0, (VOID**)&pBlendVertices, 0)))
	{
		memcpy(pBlendVertices, m_BlendVertices, sizeof(m_BlendVertices));

		m_pBlendVerticesBuffer->Unlock();
	}

	return NOERROR;
}

//-----------------------------------------------------------------------------
//! \fn CreateMatrices
//! view mapping
HRESULT PPIWindow::CreateMatrices(void)
{
	if (m_pd3dDevice)
	{
		D3DXMATRIX matIdentity;
		D3DXMatrixIdentity(&matIdentity);
		m_pd3dDevice->SetTransform(D3DTS_WORLD, &matIdentity);

		// Set up our view matrix. A view matrix can be defined given an eye point,
		// a point to lookat, and a direction for which way is up. Here, we set the
		// eye five units back along the z-axis and up three units, look at the
		// origin, and define "up" to be in the y-direction.
		float From_x = 0.0f;
		float From_y = 0.0f;
		float From_z = -2.41f;
		float Look_x = 0.0f;
		float Look_y = 0.0f;
		float Look_z = 0.0f;
		float Up_x = 0.0f;
		float Up_y = 1.0f;
		float Up_z = 0.0f;

		D3DXMATRIX matView;
		D3DXVECTOR3 vFromPt = D3DXVECTOR3(From_x, From_y, From_z);
		D3DXVECTOR3 vLookatPt = D3DXVECTOR3(Look_x, Look_y, Look_z);
		D3DXVECTOR3 vUpVec = D3DXVECTOR3(Up_x, Up_y, Up_z);
		D3DXMatrixLookAtLH(&matView, &vFromPt, &vLookatPt, &vUpVec);
		m_pd3dDevice->SetTransform(D3DTS_VIEW, &matView);

		// Set the projection matrix

		D3DXMATRIX matProj;
		D3DXMatrixPerspectiveFovLH(&matProj, D3DX_PI / 4, 1.0f, -1.0f, 1.0f);
		m_pd3dDevice->SetTransform(D3DTS_PROJECTION, &matProj);
	}

	return NOERROR;
}
//--------------------------------------------------------------------------
//! \fn PeakDetectSpoke
//! Ensures targets near centre of the image are not lost when converting from rectangular to polar display
void PPIWindow::PeakDetectSpoke(int SpokeBearing, unsigned int * pSpoke)
{
	unsigned int *    Ptr = pSpoke;
	unsigned int *    NextPtr = pSpoke;
	unsigned int      Max = 0;
	int               Offset = OFFSET_IN_BLOCK(SpokeBearing);
	unsigned int *    TexturePtr = &m_SystemSpokeCache[SpokeBearing - Offset][0];
	unsigned int      Decm_DecInc = 64 * m_PixelsPerSpoke;
	unsigned int      Decm_Error = 0;

	// bearing 0 : DecInc = 64
	// bearing 1 : DecInc = 128
	// bearing 2 : DecInc = 192
	// bearing 3 : DecInc = 256
	// bearing n : DecInc = 64 * (n + 1)

	unsigned int      Line_DecInc = 64 * (Offset + 1);
	unsigned int      Line_Error = 0;

	int               Iterations = QuantumLib::cSamplesPerSpoke;

	while (Iterations--)
	{
		if (ALPHA(*NextPtr) > ALPHA(Max))
		{
			Max = *NextPtr;
		}

		NextPtr++;
		Decm_Error += Decm_DecInc;

		if (Decm_Error >> 16)
		{
			Decm_Error &= 0xffff;

			// time to 'step' the decimator

			while (Ptr != NextPtr)
			{
				// peak detect
				if (ALPHA(*TexturePtr) < ALPHA(Max))
				{
					*TexturePtr = Max;
				}

				TexturePtr++;
				Line_Error += Line_DecInc;

				if (Line_Error >> 16)
				{
					Line_Error &= 0xffff;
					// 'step' the line
					TexturePtr += QuantumLib::cSamplesPerSpoke;
				}
				Ptr++;
			}
			Max = 0;
		}
	}
}

//--------------------------------------------------------------------------
// called by the windows message thread to process received spokes
//
bool PPIWindow::Render(int SpokeBearing, uint8_t* pSpokeData)
{
	bool            bRenderNow = false;
	uint32_t RenderBuffer[QuantumLib::cSamplesPerSpoke];

	if ((m_pVideoRectTexture == 0) || (m_bPaused == true))
	{
		return false;
	}

	m_bStateRunning = true;

	// correct bearing so that spoke 0 is at the bottom of the screen
	SpokeBearing += QuantumLib::cSpokesPerScan / 2;
	SpokeBearing %= QuantumLib::cSpokesPerScan;

	if (m_bVRMDirty == true)
	{
		for (uint32_t i = 0; i < 128; i++)
		{
			float   da = (float)(i * M_PIx2) / 128.0f;
			float   dr = ((float)m_VRM * (float)m_PixelsPerSpoke) / 1024.0f;

			m_VRMVertices[i].p = D3DXVECTOR4((dr * sinf(da)) + m_cx,
				m_cy - (dr * cosf(da)),
				0.0f,
				1.0f);
			m_VRMVertices[i].colour = 0xffffff00;
		}
		m_VRMVertices[128] = m_VRMVertices[0];
	}

	// we have the sample, draw it into the texture
	bRenderNow = true;

	// check for first time through

	if (m_bFirstSpoke)
	{
		m_FirstBlockToDraw = BEARING_TO_BLOCK(SpokeBearing);
		m_bFirstSpoke = false;
	}

	// convert sample data to RGB palette value and merge sample value into top 32 bits for peak detection algorithm
	// bits 24-31 original sample value
	// bits 16-23 blue
	// bits 8-15 green
	// bits 0-7 red

	uint32_t* pPalette = pPalettes[m_PaletteIndex];

	for (uint32_t x = 0; x < QuantumLib::cSamplesPerSpoke; x++)
	{
		uint8_t sample = *pSpokeData++;
		RenderBuffer[x] = *(pPalette + sample) | ((static_cast<uint32_t>(sample)) << 24);
	}

	// now 'draw' the new spoke into the system memory texture
	PeakDetectSpoke(SpokeBearing, RenderBuffer);

	if (bRenderNow)
	{
		bRenderNow = false;

		// now check if it's time to draw anything by comparing m_FirstBlockToDraw
		// with the last bearing received, work out how many blocks are ready to be drawn
		// note however, that we don't draw the block before the one containing the latest
		// received spoke, this means that we can use texture filtering and we won't get any
		// strange artifacts on the first scan

		int Count = SpokeBearing - BLOCK_TO_BEARING(m_FirstBlockToDraw);

		if (Count < 0)
		{
			Count += QuantumLib::cSpokesPerScan;
		}

		Count = BEARING_TO_BLOCK(Count) - 1;

		if (Count > 0)
		{
			RECT                UpdateRect;
			D3DLOCKED_RECT      LockedRect;
			LPDIRECT3DSURFACE9  pSavedSurface;
			LPDIRECT3DSURFACE9  pSurface;

			// setup the d3d destination to be the intermediate texture

			m_pd3dDevice->GetRenderTarget(0, &pSavedSurface);                  // AddRef(pSavedSurface)

			m_pPolarTexture->GetSurfaceLevel(0, &pSurface);                    // AddRef(pSurface)
			m_pd3dDevice->SetRenderTarget(0, pSurface);                        // AddRef(pSurface), Release(pSavedSurface)

			// all draws now go to m_pPolarTexture

			if (m_ClearRequired)
			{
				m_pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0L);
				m_ClearRequired = false;
			}

			UpdateRect.left = 0;
			UpdateRect.top = BLOCK_TO_BEARING(m_FirstBlockToDraw);
			UpdateRect.right = QuantumLib::cSamplesPerSpoke;

			// draw from m_FirstBlockToDraw for Count - 1 blocks

			if (m_FirstBlockToDraw + Count < BLOCKS_PER_SCAN)
			{
				UpdateRect.bottom = LAST_IN_BLOCK(m_FirstBlockToDraw + Count - 1) + 1;

				// copy from m_SystemSpokeCache to m_pSystemRectTexture removing alpha channel info
				// as we go

				if (SUCCEEDED(m_pSystemRectTexture->LockRect(0, &LockedRect, &UpdateRect, 0)))
				{
					unsigned char * pBuffer = (unsigned char *)LockedRect.pBits;
					unsigned int *  pSrc;
					unsigned int *  pDst;

					for (int y = UpdateRect.top; y < UpdateRect.bottom; y++)
					{
						pDst = (unsigned int *)pBuffer;
						pSrc = &m_SystemSpokeCache[y][0];

						for (int x = 0; x < QuantumLib::cSamplesPerSpoke; x++)
						{
							WRITE_DST();
							CLEAR_SRC();
						}

						pBuffer += LockedRect.Pitch;
					}

					m_pSystemRectTexture->UnlockRect(0);
				}

				m_pd3dDevice->UpdateTexture(m_pSystemRectTexture, m_pVideoRectTexture);
				bRenderNow = true;

				// draw the changed blocks to the intermediate texture

				if (SUCCEEDED(m_pd3dDevice->BeginScene()))              // Begin the scene
				{
					m_pd3dDevice->SetFVF(D3DFVF_R2PVERTEX);
					m_pd3dDevice->SetStreamSource(0, m_pR2PVertexBuffer, 0, sizeof(R2PVERTEX));
					m_pd3dDevice->SetTexture(0, m_pVideoRectTexture);
					m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, m_FirstBlockToDraw * 3, Count);
					m_pd3dDevice->SetTexture(0, 0);

					m_pd3dDevice->EndScene();                              // End the scene.
				}

				m_FirstBlockToDraw += Count;
			}
			else
			{
				// we are going to wrap

				UpdateRect.bottom = QuantumLib::cSpokesPerScan;

				if (SUCCEEDED(m_pSystemRectTexture->LockRect(0, &LockedRect, &UpdateRect, 0)))
				{
					unsigned char * pBuffer = (unsigned char *)LockedRect.pBits;
					unsigned int *  pSrc;
					unsigned int *  pDst;

					for (int y = UpdateRect.top; y < UpdateRect.bottom; y++)
					{
						pDst = (unsigned int *)pBuffer;
						pSrc = &m_SystemSpokeCache[y][0];

						for (int x = 0; x < QuantumLib::cSamplesPerSpoke; x++)
						{
							WRITE_DST();
							CLEAR_SRC();
						}

						pBuffer += LockedRect.Pitch;
					}

					m_pSystemRectTexture->UnlockRect(0);
				}

				m_pd3dDevice->UpdateTexture(m_pSystemRectTexture, m_pVideoRectTexture);
				bRenderNow = TRUE;

				// draw the changed blocks to the intermediate texture

				if (SUCCEEDED(m_pd3dDevice->BeginScene()))              // Begin the scene
				{
					m_pd3dDevice->SetFVF(D3DFVF_R2PVERTEX);
					m_pd3dDevice->SetStreamSource(0, m_pR2PVertexBuffer, 0, sizeof(R2PVERTEX));
					m_pd3dDevice->SetTexture(0, m_pVideoRectTexture);
					m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, m_FirstBlockToDraw * 3, BLOCKS_PER_SCAN - m_FirstBlockToDraw);
					m_pd3dDevice->SetTexture(0, 0);

					m_pd3dDevice->EndScene();                              // End the scene.
				}

				Count -= BLOCKS_PER_SCAN - m_FirstBlockToDraw;
				m_FirstBlockToDraw = 0;

				if (Count)
				{
					UpdateRect.top = 0;
					UpdateRect.bottom = LAST_IN_BLOCK(Count) + 1;

					if (SUCCEEDED(m_pSystemRectTexture->LockRect(0, &LockedRect, &UpdateRect, 0)))
					{
						unsigned char * pBuffer = (unsigned char *)LockedRect.pBits;
						unsigned int *  pSrc;
						unsigned int *  pDst;

						for (int y = UpdateRect.top; y < UpdateRect.bottom; y++)
						{
							pDst = (unsigned int *)pBuffer;
							pSrc = &m_SystemSpokeCache[y][0];

							for (int x = 0; x < QuantumLib::cSamplesPerSpoke; x++)
							{
								WRITE_DST();
								CLEAR_SRC();
							}

							pBuffer += LockedRect.Pitch;
						}

						m_pSystemRectTexture->UnlockRect(0);
					}

					m_pd3dDevice->UpdateTexture(m_pSystemRectTexture, m_pVideoRectTexture);

					// draw the changed blocks to the intermediate texture

					if (SUCCEEDED(m_pd3dDevice->BeginScene()))              // Begin the scene
					{
						m_pd3dDevice->SetFVF(D3DFVF_R2PVERTEX);
						m_pd3dDevice->SetStreamSource(0, m_pR2PVertexBuffer, 0, sizeof(R2PVERTEX));
						m_pd3dDevice->SetTexture(0, m_pVideoRectTexture);
						m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, Count);
						m_pd3dDevice->SetTexture(0, 0);

						m_pd3dDevice->EndScene();                              // End the scene.
					}

					m_FirstBlockToDraw = Count;
				}
			}

			if (SUCCEEDED(m_pd3dDevice->BeginScene()))              // Begin the scene
			{
				// stop 'trails' as the ebl and vrm change

				if (m_bVRMDirty)
				{
					m_pd3dDevice->SetFVF(D3DFVF_R2PVERTEX);
					m_pd3dDevice->SetStreamSource(0, m_pR2PVertexBuffer, 0, sizeof(R2PVERTEX));
					m_pd3dDevice->SetTexture(0, m_pVideoRectTexture);
					m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLELIST, 0, BLOCKS_PER_SCAN);
					m_pd3dDevice->SetTexture(0, 0);

					m_bVRMDirty = false;
				}

				m_pd3dDevice->SetFVF(D3DFVF_SYMVERTEX);
				m_pd3dDevice->DrawPrimitiveUP(D3DPT_LINESTRIP, 128, m_VRMVertices, sizeof(SYMVERTEX));

				bRenderNow = TRUE;
				m_pd3dDevice->EndScene();                              // End the scene.
			}

			// return the render target to be the back buffer
			SAFE_RELEASE(pSurface);

			m_pd3dDevice->SetRenderTarget(0, pSavedSurface);                  // AddRef(pSavedSurface), Release(pSurface)

			// draws now go to the back buffer
			SAFE_RELEASE(pSavedSurface);                                      // Release(pSavedSurface
		}
	}

	if (bRenderNow)
	{
		if (m_pd3dDevice)
		{
			m_pd3dDevice->Clear(0, 0, D3DCLEAR_TARGET, 0x00000000, 1.0f, 0);

			if (SUCCEEDED(m_pd3dDevice->BeginScene()))
			{
				m_pd3dDevice->SetFVF(D3DFVF_R2PVERTEX);
				m_pd3dDevice->SetTexture(0, m_pPolarTexture);
				m_pd3dDevice->SetStreamSource(0, m_pRectVerticesBuffer, 0, sizeof(R2PVERTEX));
				m_pd3dDevice->DrawPrimitive(D3DPT_TRIANGLESTRIP, 0, 2);
				m_pd3dDevice->SetTexture(0, 0);

				m_pd3dDevice->EndScene();
			}
			m_pd3dDevice->Present(0, 0, 0, 0);
		}
	}
	m_bStateRunning = false;
	return true;
}

//-----------------------------------------------------------------------------
//! \fn ClearSpokes
//! the end will always be greater than the start
void PPIWindow::ClearSpokes(int Start, int End)
{
	D3DLOCKED_RECT  LockedRect;
	RECT            m_Rect;

	m_Rect.left = 0;
	m_Rect.top = Start;
	m_Rect.right = QuantumLib::cSamplesPerSpoke;
	m_Rect.bottom = End;

	if (SUCCEEDED(m_pSystemRectTexture->LockRect(0, &LockedRect, &m_Rect, 0)))
	{
		BYTE *    pRGB = (BYTE *)LockedRect.pBits;
		DWORD *   pTemp;
		int       i;

		while (Start < End)
		{
			pTemp = (DWORD *)pRGB;

			for (i = 0; i < QuantumLib::cSamplesPerSpoke; i++)
			{
				*pTemp++ = 0x00000000;
			}

			pRGB += LockedRect.Pitch;
			Start++;
		}

		m_pSystemRectTexture->UnlockRect(0);
	}
}

//-----------------------------------------------------------------------------
//! \fnInvalidateDeviceObjects() 
//! paired with RestoreDeviceObjects
//! delete stuff created in RestoreDeviceObjects
//
HRESULT PPIWindow::InvalidateDeviceObjects(void)
{
	SAFE_RELEASE(m_pR2PVertexBuffer);
	SAFE_RELEASE(m_pVideoRectTexture);
	SAFE_RELEASE(m_pPolarTexture);

	return S_OK;
}
//-----------------------------------------------------------------------------
//! \fn Resize
//! called when window size changes
void PPIWindow::Resize()
{
	//Pause(true);
	HandlePossibleSizeChange();
	//Pause(false);
	//GetClientRect(m_hWnd, &m_Rect);
}

//-----------------------------------------------------------------------------
//! \fn Recentre
//! called when window centre position changes
void PPIWindow::Recentre(uint32_t CentrePos)
{
	int   xPos;
	int   yPos;

	xPos = CentrePos & 0xffff;
	yPos = CentrePos >> 16;
	m_cx = xPos;
	m_cy = yPos;

	// force ebl & vrm to recalculate

	m_bVRMDirty = TRUE;
	m_ClearRequired = TRUE;

	Pause(true);
	CreateVertices();
	Pause(false);
}
//-----------------------------------------------------------------------------
//! \fn Pause
//! temporary suspend of spoke drawing
void PPIWindow::Pause(bool Pause)
{
	while (m_bStateRunning == true)
	{
		Sleep(5);
	}
	m_bPaused = Pause;
}
//-----------------------------------------------------------------------------

