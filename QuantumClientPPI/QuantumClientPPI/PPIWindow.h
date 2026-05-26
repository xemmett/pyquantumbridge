// -------------------------------------------------------------------------------------------------
//! \file PPIWindow.h
//! PPI Window for rendering spoke data

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

#pragma once

// -------------------------------------------------------------------------------------------------
// Include Files

#include "../../QuantumLib/QuantumLib/Include/QuantumLib.h"

#include "d3d9.h"
#include "d3dx9.h"

#include <list>
#include <stdint.h>

// -------------------------------------------------------------------------------------------------
// Defines / macros

#define M_PIx2                  (float)6.283185307179586476925

#define SPOKES_PER_SCAN_MASK    (QuantumLib::cSpokesPerScan - 1)       // not me

#define SPOKES_PER_BLOCK_N      5                           // change me!!
#define SPOKES_PER_BLOCK        (1 << SPOKES_PER_BLOCK_N)   // not me
#define SPOKES_PER_BLOCK_MASK   (SPOKES_PER_BLOCK - 1)      // not me

#define BLOCKS_PER_SCAN         (QuantumLib::cSpokesPerScan / SPOKES_PER_BLOCK)
#define BLOCKS_PER_SCAN_MASK    (BLOCKS_PER_SCAN - 1)

#define BEARING_TO_BLOCK(x)     ((x) >> SPOKES_PER_BLOCK_N)
#define BLOCK_TO_BEARING(x)     ((x) << SPOKES_PER_BLOCK_N)

#define OFFSET_IN_BLOCK(x)      (x & SPOKES_PER_BLOCK_MASK)

#define FIRST_IN_BLOCK(x)       BLOCK_TO_BEARING(x)
#define LAST_IN_BLOCK(x)        (BLOCK_TO_BEARING(x) + SPOKES_PER_BLOCK_MASK)

#define NEXT_BLOCK(x)           (((x) + 1) & BLOCKS_PER_SCAN_MASK)
#define PREV_BLOCK(x)           (((x) == 0) ? BLOCKS_PER_SCAN_MASK : ((x) - 1))

#define NEXT_BEARING(x)         (((x) + 1) & SPOKES_PER_SCAN_MASK)
#define PREV_BEARING(x)         (((x) == 0) ? SPOKES_PER_SCAN_MASK : ((x) - 1))

// macros for 32bpp work
#define ALPHA(x)                (x >> 24)
#define RED(x)                  ((x >> 16) & 0xff)
#define GREEN(x)                ((x >> 8) & 0xff)
#define BLUE(x)                 (x & 0xff)

// our vertex format
struct R2PVERTEX
{
	D3DXVECTOR4     p;
	FLOAT           tu, tv;
};

#define D3DFVF_R2PVERTEX        (D3DFVF_XYZRHW | D3DFVF_TEX1)

// the vertices for our symbology
struct SYMVERTEX
{
	D3DXVECTOR4     p;
	DWORD           colour;
};

#define D3DFVF_SYMVERTEX        (D3DFVF_XYZRHW | D3DFVF_DIFFUSE)

// the vertices for the final projection of the intermediate texture to the back buffer
struct PROVERTEX
{
	D3DXVECTOR3     p;
	FLOAT           tu, tv;
};

#define D3DFVF_PROVERTEX        (D3DFVF_XYZ | D3DFVF_TEX1)

// and the rest ...
#define MAX_LOADSTRING          100
// -------------------------------------------------------------------------------------------------
//! \class PPIWindow
//! class to draw Render spoke data onto a PPI Image
//! Uses DirectX 
class PPIWindow
{
public:
	PPIWindow(HWND hParent);
	~PPIWindow();

	void Close();		// called to close PPI window from Quantum Interface window

	static DWORD __stdcall WindowMessageLoop(LPVOID lpvThreadParm);

	HRESULT InitialiseWindow(void);
	HRESULT UninitialiseWindow();

	bool Render(int SpokeBearing, uint8_t* pSpokeData);

	void Resize();
	void Recentre(uint32_t CentrePos);
	void Pause(bool Pause);

	bool SetPaletteIndex(uint32_t Index);

private:

	ATOM    PPIRegisterClass(HINSTANCE hInstance);
	HRESULT HandlePossibleSizeChange();
	HRESULT Reset3DEnvironment();
	HRESULT Cleanup3DEnvironment(void);
	HRESULT InitDeviceObjects(void);
	HRESULT DeleteDeviceObjects(void);

	HRESULT RestoreDeviceObjects(void);
	HRESULT InvalidateDeviceObjects(void);

	HRESULT CreateVertices(void);
	HRESULT CreateMatrices(void);

	void PeakDetectSpoke(int SpokeBearing, unsigned int * pSpoke);
	void ClearSpokes(int Start, int End);
	
	HWND					m_hParent;
	HWND                    m_hWnd;               // window handle
	HINSTANCE               m_hInstance;          // Global module instance handle

	HANDLE					m_hEvent;			  // Synch Eveent for initialisation of d3d
	HANDLE                  m_hThread;            // Our worker thread
	DWORD                   m_ThreadID;           // Worker thread ID

	// direct 3D interfaces
	IDirect3D9 *            m_pDirect3D9;
	IDirect3DDevice9 *      m_pd3dDevice;

	// spokes are peak detected one by one into ...
	unsigned int            m_SystemSpokeCache[QuantumLib::cSpokesPerScan][QuantumLib::cSamplesPerSpoke];

	// every SPOKES_PER_BLOCK spokes, we copy (removing the alpha channel) to ...
	LPDIRECT3DTEXTURE9      m_pSystemRectTexture;

	// every SPOKES_PER_BLOCK spokes, we copy the block to ...
	LPDIRECT3DTEXTURE9      m_pVideoRectTexture;

	// then we use a triangle list with these vertices to ...
	R2PVERTEX               m_R2PVertices[BLOCKS_PER_SCAN * 3];
	LPDIRECT3DVERTEXBUFFER9 m_pR2PVertexBuffer;

	// draw the PPI to ... (same size as window)

	LPDIRECT3DTEXTURE9      m_pPolarTexture;

	// then using these vertices, it's drawn to the back buffer

	PROVERTEX               m_ProjVertices[4];
	LPDIRECT3DVERTEXBUFFER9 m_pProjVerticesBuffer;

	// or these (non projected)

	R2PVERTEX               m_RectVertices[4];
	LPDIRECT3DVERTEXBUFFER9 m_pRectVerticesBuffer;

	// vertices used to alpha blend black to decay previous values

	SYMVERTEX               m_BlendVertices[4];
	LPDIRECT3DVERTEXBUFFER9 m_pBlendVerticesBuffer;

	int                     m_PixelsPerSpoke;       // pixel radius of PPI

	int                     m_cx;                   // ppi centre
	int                     m_cy;

	int                     m_Width;                // window size
	int                     m_Height;

	RECT                    m_Rect;

	bool                    m_ClearRequired;

	D3DPRESENT_PARAMETERS   m_d3dpp;

	bool			m_bFirstSpoke;
	int				m_FirstBlockToDraw;

	// VRM parameters
	bool			m_bVRMDirty;
	uint32_t		m_VRM;
	SYMVERTEX       m_VRMVertices[129];  // quite a pleasing circle

	bool			m_bPaused;
	bool			m_bStateRunning;

	uint32_t        m_PaletteIndex;
};

