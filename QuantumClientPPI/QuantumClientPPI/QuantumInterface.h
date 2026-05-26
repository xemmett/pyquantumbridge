//! \file QuantumInterface.h
//! Main Client Application file
//! Receives notification messages from QuantumLib
//! Sends Setting messages to API

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

#pragma once

// -----------------------------------------------------------------------------------------------------
// Includes

#include "../../QuantumLib/QuantumLib/Include/QuantumLib.h"

// -----------------------------------------------------------------------------------------------------

class PPIWindow;

// ------------------------------------------------------------------------------------------------
//! \class QuantumInterface
//! Interface between Quantum API and Client PPI app
//! Derived from INotify base class
class QuantumInterface : public QuantumLib::INotify
{
public:
	QuantumInterface(HWND hDlg);
	~QuantumInterface();

	QuantumLib::eErrorCode_t Open();

	// notification messages from QuantumLib::INotify
	void UpdateScannerListWindow(uint32_t ScannerCount);
	void UpdateSpokeDataWindow();

	void ScannerListChanged(uint32_t ScannerCount);
	void SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData);

	void PPIWindowClosed();

	// control messages
	void SetScannerIndex();
	void SetPaletteIndex();

private:
	uint32_t _GetSerialNumber(uint32_t Index);

	HWND m_hDlg;
	PPIWindow* m_pPPIWindow;

	// Text buffers
	static const uint32_t cLineBufferSize =128;
	static const uint32_t cScanerListSize = 10;
	char m_ScannerBuffer[cScanerListSize * cLineBufferSize];
	uint32_t m_ScannerBufferSize;

	static const uint32_t cSpokeDataBufferSize = 32;
	QuantumLib::SpokeData_t m_SpokeDataBuffer[cSpokeDataBufferSize];
	uint32_t m_SpokeDataBufferWriteIndex;
	uint32_t m_SpokeDataBufferReadIndex;

	uint32_t m_ScannerCount;
	uint32_t m_SelectedScannerSerialNumber;
	uint32_t m_ScannerIndex;
	uint32_t m_PaletteIndex;

};

// -----------------------------------------------------------------------------------------------------
