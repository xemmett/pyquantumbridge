// -------------------------------------------------------------------------------------------------
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
#include "CLI.h"

// -----------------------------------------------------------------------------------------------------
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
	void UpdateNotificationWindow();
	void UpdateMarpaWindow();
	void UpdateAlarmWindow();

	void ScannerListChanged(uint32_t ScannerCount);
	void SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData);
	void SettingChanged(QuantumLib::SettingData_t& S);
	void FeatureChanged(uint32_t SerialNumber, QuantumLib::eFeatures_t Feature, bool Supported);
	void ParameterChanged(uint32_t SerialNumber, QuantumLib::eParameters_t Parameter, uint32_t Value);
	void MarpaDataChanged(uint32_t SerialNumber, QuantumLib::MarpaData_t& MarpaData);
	void AlarmDataChanged(uint32_t SerialNumber, QuantumLib::AlarmData_t& AlarmData);

	// control messages
	void OnSendCommand();

private:
	uint32_t _GetSerialNumber(uint32_t Index);

	HWND m_hDlg;
	CLI* m_pCLI;
	
	// Text buffers
	static const uint32_t cLineBufferSize =128;
	static const uint32_t cScanerListSize = 6;
	char m_ScannerBuffer[cScanerListSize * cLineBufferSize];
	uint32_t m_ScannerBufferSize;

	static const uint32_t cAlarmListSize = 6;
	char m_AlarmList[cAlarmListSize][cLineBufferSize];
	uint32_t m_AlarmListIndex;
	bool m_AlarmListWrapped;

	char m_AlarmBuffer[cAlarmListSize * cLineBufferSize];
	uint32_t m_AlarmBufferSize;

	static const uint32_t cMarpaListSize = 25;
	char m_MarpaList[cMarpaListSize][cLineBufferSize];
	uint32_t m_MarpaListIndex;
	bool m_MarpaListWrapped;

	char m_MarpaBuffer[cMarpaListSize * cLineBufferSize];
	uint32_t m_MarpaBufferSize;

	static const uint32_t cNotificationListSize = 25;
	char m_NotificationList[cNotificationListSize][cLineBufferSize];
	uint32_t m_NotificationListIndex;
	bool m_NotificationListWrapped;

	char m_NotificationBuffer[cNotificationListSize * cLineBufferSize];
	uint32_t m_NotificationBufferSize;

	static const uint32_t cSpokeDataBufferSize = 32;
	QuantumLib::SpokeData_t m_SpokeDataBuffer[cSpokeDataBufferSize];
	uint32_t m_SpokeDataBufferWriteIndex;
	uint32_t m_SpokeDataBufferReadIndex;

	uint32_t m_SpokeCount;
};

