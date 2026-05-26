//! \file QuantumInterface.cpp
//! Main Client Application file
//! Receives notification messages from QuantumLib
//! Sends Setting messages to API

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

#include "stdafx.h"

#include "QuantumClientPPI.h"
#include "QuantumInterface.h"
#include "PPIWindow.h"
#include "resource.h"

#include <windows.h>
#include <new>
#include <cstdio>
#include <cassert>

// ------------------------------------------------------------------------------------------------
QuantumInterface::QuantumInterface(HWND hDlg) :
m_hDlg(hDlg),
m_pPPIWindow(0),
m_ScannerBufferSize(0),
m_SpokeDataBufferWriteIndex(0),
m_SpokeDataBufferReadIndex(0),
m_ScannerCount(0),
m_SelectedScannerSerialNumber(0),
m_ScannerIndex(0),
m_PaletteIndex(0)
{
	// Text buffers
	memset(&m_ScannerBuffer[0], 0, sizeof(m_ScannerBuffer));
	memset(m_SpokeDataBuffer, 0, sizeof(m_SpokeDataBuffer));

}
// ------------------------------------------------------------------------------------------------
QuantumInterface::~QuantumInterface()
{
	QuantumLib::API::DeRegisterForScannerNotifications(this);

	Sleep(100);
	QuantumLib::API::Close();

	if (m_pPPIWindow)
	{
		m_pPPIWindow->Close();
		delete m_pPPIWindow;
		m_pPPIWindow = 0;
	}
}
// ------------------------------------------------------------------------------------------------
//! \fn Open
//! Opens the DLL and registers this class for notification messages
//! Opens PPI window
QuantumLib::eErrorCode_t QuantumInterface::Open()
{
	QuantumLib::eErrorCode_t ErrorCode = QuantumLib::API::Open();
	if (ErrorCode == QuantumLib::eNoError)
	{
		ErrorCode = QuantumLib::API::RegisterForScannerNotifications(this);
		if (ErrorCode == QuantumLib::eNoError)
		{
			m_pPPIWindow = new PPIWindow(m_hDlg);
			if (m_pPPIWindow == 0)
			{
				ErrorCode = QuantumLib::eErrorOutOfMemory;
			}
		}
	}

	SetDlgItemInt(m_hDlg, IDC_SCANNER_INDEX, m_ScannerIndex, FALSE);
	SetDlgItemInt(m_hDlg, IDC_PALETTE_INDEX, m_PaletteIndex, FALSE);

	return ErrorCode;
}
// ------------------------------------------------------------------------------------------------
//! \fn UpdateScannerListWindow
//! Reads scanner details and re-write list to dialog window
void QuantumInterface::UpdateScannerListWindow(uint32_t ScannerCount)
{
	QuantumLib::ScannerDetails_t ScannerDetails;

	m_ScannerCount = ScannerCount;

	m_ScannerBufferSize = 0;
	for (uint32_t index = 0; index < ScannerCount; index++)
	{
		QuantumLib::eErrorCode_t ErrorCode = QuantumLib::API::GetScannerDetails(index, ScannerDetails);
		if (ErrorCode == QuantumLib::eNoError)
		{

			m_ScannerBufferSize += sprintf(m_ScannerBuffer + m_ScannerBufferSize, "%u, %s SerNo 0x%08x\r\n", index, ScannerDetails.Description, ScannerDetails.SerialNumber);
		}
		if (index == m_ScannerIndex)
		{
			m_SelectedScannerSerialNumber = ScannerDetails.SerialNumber;
		}
	}
	m_ScannerBuffer[m_ScannerBufferSize] = 0;
	SetDlgItemText(m_hDlg, IDC_SCANNER_LIST, m_ScannerBuffer);
}
// ------------------------------------------------------------------------------------------------
//! \fn UpdateSpokeDataWindow
//! Retrieves spoke buffer and calls Render function of PPI
void QuantumInterface::UpdateSpokeDataWindow()
{

	QuantumLib::SpokeData_t* pSpokeData = &m_SpokeDataBuffer[m_SpokeDataBufferReadIndex++];
	if (m_SpokeDataBufferReadIndex >= cSpokeDataBufferSize)
	{
		m_SpokeDataBufferReadIndex = 0;
	}

	if (m_pPPIWindow)
	{
		m_pPPIWindow->Render(pSpokeData->Bearing, pSpokeData->SpokeData);
	}

}
// ------------------------------------------------------------------------------------------------
//! \fn ScannerListChanged
//! Notifcation callback from QuantumLib
//! Sends user defined windows message to update dialog
void QuantumInterface::ScannerListChanged(uint32_t count)
{
	LPARAM lParam = static_cast<LPARAM>(count);
	SendNotifyMessage(m_hDlg, WM_USER_SCANNER_LIST_MSG, NULL, lParam);
}

// ------------------------------------------------------------------------------------------------
//! \fn SpokeDataReceived
//! Notifcation callback from QuantumLib
//! Buffers spoke data into circular buffer and sends user defined windows message to render to PPI window
void QuantumInterface::SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData)
{
	if (SerialNumber == m_SelectedScannerSerialNumber)
	{
		memcpy(&m_SpokeDataBuffer[m_SpokeDataBufferWriteIndex++], &rSpokeData, sizeof(QuantumLib::SpokeData_t));
		if (m_SpokeDataBufferWriteIndex >= cSpokeDataBufferSize)
		{
			m_SpokeDataBufferWriteIndex = 0;
		}
		SendNotifyMessage(m_hDlg, WM_USER_SPOKE_DATA_MSG, NULL, NULL);
	}
}
// ------------------------------------------------------------------------------------------------
//! \fn SetScannerIndex
//! Called to update Client app with selected scanner index
void QuantumInterface::SetScannerIndex()
{
	BOOL retStatus = FALSE;
	unsigned int index = GetDlgItemInt(m_hDlg, IDC_SCANNER_INDEX, &retStatus, FALSE);
	if (retStatus == TRUE)
	{
		if (index < m_ScannerCount)
		{
			m_ScannerIndex = index;
		}
	}
	QuantumLib::ScannerDetails_t ScannerDetails;
	QuantumLib::eErrorCode_t ErrorCode = QuantumLib::API::GetScannerDetails(m_ScannerIndex, ScannerDetails);
	if (ErrorCode == QuantumLib::eNoError)
	{
		m_SelectedScannerSerialNumber = ScannerDetails.SerialNumber;
	}
	SetDlgItemInt(m_hDlg, IDC_SCANNER_INDEX, m_ScannerIndex, FALSE);
}
// ------------------------------------------------------------------------------------------------
//! \fn SetPaletteIndex
//! Called to update Client app with selected palette index
//! calls PPI window to update with selected palette
void QuantumInterface::SetPaletteIndex()
{
	BOOL retStatus = FALSE;
	unsigned int index = GetDlgItemInt(m_hDlg, IDC_PALETTE_INDEX, &retStatus, FALSE);
	if (retStatus == TRUE)
	{
		if (m_pPPIWindow)
		{
			if (m_pPPIWindow->SetPaletteIndex(index))
			{
				m_PaletteIndex = index;
			}
		}
	}
	SetDlgItemInt(m_hDlg, IDC_PALETTE_INDEX, m_PaletteIndex, FALSE);

}
// ------------------------------------------------------------------------------------------------
//! \fn _GetSerialNumber(uint32_t Index)
//! Converts scanner Index into 32 bit serial number
uint32_t QuantumInterface::_GetSerialNumber(uint32_t Index)
{
	QuantumLib::ScannerDetails_t ScannerDetails;
	uint32_t SerNo = 0xFFFFFFFF;

	QuantumLib::eErrorCode_t ErrorCode = QuantumLib::API::GetScannerDetails(Index, ScannerDetails);
	if (ErrorCode == QuantumLib::eNoError)
	{
		SerNo = ScannerDetails.SerialNumber;
	}
	return SerNo;
}
// ------------------------------------------------------------------------------------------------
//! \fn PPIWindowClosed
//! Synchronise PPI window and dialog 
void QuantumInterface::PPIWindowClosed()
{
	m_pPPIWindow = 0;
}
// ------------------------------------------------------------------------------------------------






