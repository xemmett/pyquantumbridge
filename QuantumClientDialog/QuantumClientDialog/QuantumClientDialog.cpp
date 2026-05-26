// -------------------------------------------------------------------------------------------------
//! \file QuantumClientDialog.cpp
//! Main entry point for client application

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// -----------------------------------------------------------------------------
// include files


#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "stdafx.h"

#include "QuantumClientDialog.h"
#include "QuantumInterface.h"

#include <new>

// ------------------------------------------------------------------

INT_PTR CALLBACK	QuantumDialog(HWND, UINT, WPARAM, LPARAM);

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

	// create Dialog Box
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_QUANTUM_INTERFACE), NULL, QuantumDialog);

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
//! \fn QuantumDialog
//! Message handler for Quantum Interface Dialog

static QuantumInterface* pQuantumInterface = 0;

INT_PTR CALLBACK QuantumDialog(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	UNREFERENCED_PARAMETER(lParam);
	INT_PTR retVal = FALSE;

	switch (message)
	{
	case WM_INITDIALOG:
		// create Quantum Interface class
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
		uint32_t ScannerCount = static_cast<uint32_t>(lParam);
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
	case WM_USER_NOTIFICATION_MSG:
		if (pQuantumInterface)
		{
			pQuantumInterface->UpdateNotificationWindow();
		}
		retVal = TRUE;
		break;
	case WM_USER_ALARM_MSG:
		if (pQuantumInterface)
		{
			pQuantumInterface->UpdateAlarmWindow();
		}
		retVal = TRUE;
		break;
	case WM_USER_MARPA_MSG:
		if (pQuantumInterface)
		{
			pQuantumInterface->UpdateMarpaWindow();
		}
		retVal = TRUE;
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

		case IDC_SEND_COMMAND:
			if (HIWORD(wParam) == BN_CLICKED)
			{
				if (pQuantumInterface)
				{
					pQuantumInterface->OnSendCommand();
				}
				retVal = TRUE;
			}
			break;

		default:
			break;
		}
	}
	return retVal;
}
// -----------------------------------------------------------------
