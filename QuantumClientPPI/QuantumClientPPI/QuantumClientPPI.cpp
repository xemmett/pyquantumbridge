// -------------------------------------------------------------------------------------------------
//! \file QuantumClientPPI.cpp
//! Main entry point for client application

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// -----------------------------------------------------------------------------
// include files

#include "stdafx.h"

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "QuantumClientPPI.h"
#include "QuantumInterface.h"
#include "resource.h"

#include <cstdio>
#include <new>

// ------------------------------------------------------------------

HINSTANCE hInst;								// current instance
INT_PTR CALLBACK	QuantumPPIControl(HWND, UINT, WPARAM, LPARAM);

// -----------------------------------------------------------------
//! \fn tWinMain
//! Main windows entry point
int APIENTRY _tWinMain(_In_ HINSTANCE hInstance,
	_In_opt_ HINSTANCE hPrevInstance,
	_In_ LPTSTR    lpCmdLine,
	_In_ int       nCmdShow)
{
	MSG msg;

	UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(lpCmdLine);

	hInst = hInstance;

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_PPI_CONTROL), NULL, QuantumPPIControl);

	// Main message loop:
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	_CrtDumpMemoryLeaks();

	return (int)msg.wParam;
}

// -----------------------------------------------------------------
//! \fn QuantumPPIControl
//! Message handler for Quantum PPI Dialog

static QuantumInterface* pQuantumInterface = 0;

// Message handler for Quantum Interface Dialog
INT_PTR CALLBACK QuantumPPIControl(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	INT_PTR retVal = FALSE;

	switch (message)
	{
	case WM_INITDIALOG:
		pQuantumInterface = new(std::nothrow) QuantumInterface(hDlg);
		if (pQuantumInterface)
		{
			QuantumLib::eErrorCode_t ErrorCode = pQuantumInterface->Open();
			if (QuantumLib::eNoError == ErrorCode)
			{
				retVal = TRUE;
			}
		}
		if (retVal == FALSE)
		{
			PostMessage(hDlg, WM_DESTROY, NULL, NULL);
		}

		break;

	case WM_USER_SCANNER_LIST_MSG:
	{
		uint32_t ScannerCount = lParam;
		if (pQuantumInterface)
		{
			pQuantumInterface->UpdateScannerListWindow(ScannerCount);
		}
		retVal = TRUE;
		break;
	}
	case WM_USER_SPOKE_DATA_MSG:
		if (pQuantumInterface)
		{
			pQuantumInterface->UpdateSpokeDataWindow();

		}
		retVal = TRUE;
		break;

	case WM_USER_PPI_WINDOW_CLOSED:
		if (pQuantumInterface)
		{
			pQuantumInterface->PPIWindowClosed();
		}
		break;

	case WM_CLOSE:
	case WM_DESTROY:
		EndDialog(hDlg, LOWORD(wParam));
		delete pQuantumInterface;
		pQuantumInterface = 0;
		PostQuitMessage(0);
		retVal = 0;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, LOWORD(wParam));
			delete pQuantumInterface;
			pQuantumInterface = 0;
			PostQuitMessage(0);
			retVal = 0;
			break;

		case IDC_SET_SCANNER_INDEX:
			if (pQuantumInterface)
			{
				pQuantumInterface->SetScannerIndex();
			}
			break;
		
		case IDC_SET_PALETTE_INDEX:
			if (pQuantumInterface)
			{
				pQuantumInterface->SetPaletteIndex();
			}
			break;
		default:
			break;
		}
	}
	return retVal;
}

// --------------------------------------------------------------------------------------------------------
