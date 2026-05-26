// -------------------------------------------------------------------------------------------------
//! \file QuantumInterface.cpp
//! Main Client Application file
//! Receives notification messages from QuantumLib
//! Sends Setting messages to API

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// ------------------------------------------------------------------------------------------------
// Class to interface between Quantum API and dialog app

#include "stdafx.h"

#include "QuantumClientDialog.h"
#include "QuantumInterface.h"
#include "../../QuantumCommonSource/BuildDataStructure.h"
#include "resource.h"

#include <windows.h>
#include <new>
#include <cstdio>
#include <cassert>

// ------------------------------------------------------------------------------------------------
QuantumInterface::QuantumInterface(HWND hDlg) :
m_hDlg(hDlg),
m_pCLI(0),
m_ScannerBufferSize(0),
m_AlarmListIndex(0),
m_AlarmListWrapped(false),
m_AlarmBufferSize(0),
m_MarpaListIndex(0),
m_MarpaListWrapped(false),
m_MarpaBufferSize(0),
m_NotificationListIndex(0),
m_NotificationListWrapped(false),
m_NotificationBufferSize(0),
m_SpokeDataBufferWriteIndex(0),
m_SpokeDataBufferReadIndex(0),
m_SpokeCount(0)
{
	// Text buffers
	memset(&m_ScannerBuffer[0], 0, sizeof(m_ScannerBuffer));
	memset(&m_AlarmList[0], 0, sizeof(m_AlarmList));
	memset(m_AlarmBuffer, 0, sizeof(m_AlarmBuffer));
	memset(&m_MarpaList[0], 0, sizeof(m_MarpaList));
	memset(m_MarpaBuffer, 0, sizeof(m_MarpaBuffer));
	memset(&m_NotificationList[0], 0, sizeof(m_NotificationList));
	memset(m_NotificationBuffer, 0, sizeof(m_NotificationBuffer));
	memset(m_SpokeDataBuffer, 0, sizeof(m_SpokeDataBuffer));

}
// ------------------------------------------------------------------------------------------------
QuantumInterface::~QuantumInterface()
{
	QuantumLib::API::DeRegisterForScannerNotifications(this);

	Sleep(100);
	QuantumLib::API::Close();

	if (m_pCLI)
	{
		delete m_pCLI;
		m_pCLI = 0;
	}

}
// ------------------------------------------------------------------------------------------------
//! \fn Open
//! Opens the DLL and registers this class for notification messages
QuantumLib::eErrorCode_t QuantumInterface::Open()
{
	QuantumLib::eErrorCode_t ErrorCode = QuantumLib::eNoError;

	m_pCLI = new(std::nothrow) CLI;
	if (m_pCLI == 0)
	{
		ErrorCode = QuantumLib::eErrorOutOfMemory;
	}

	if (ErrorCode == QuantumLib::eNoError)
	{
		ErrorCode = QuantumLib::API::Open();
	}
	if (ErrorCode == QuantumLib::eNoError)
	{
		ErrorCode = QuantumLib::API::RegisterForScannerNotifications(this);
	}
	return ErrorCode;
}
// ------------------------------------------------------------------------------------------------
//! \fn UpdateScannerListWindow
//! Reads scanner details and re-write list to dialog window
void QuantumInterface::UpdateScannerListWindow(uint32_t ScannerCount)
{
	QuantumLib::ScannerDetails_t ScannerDetails;

	m_ScannerBufferSize = 0;
	for (uint32_t index = 0; index < ScannerCount; index++)
	{
		QuantumLib::eErrorCode_t ErrorCode = QuantumLib::API::GetScannerDetails(index, ScannerDetails);
		if (ErrorCode == QuantumLib::eNoError)
		{

			m_ScannerBufferSize += sprintf(m_ScannerBuffer + m_ScannerBufferSize, "%u, %s SerNo 0x%08x\r\n", index, ScannerDetails.Description, ScannerDetails.SerialNumber);
		}
	}
	m_ScannerBuffer[m_ScannerBufferSize] = 0;
	SetDlgItemText(m_hDlg, IDC_SCANNER_LIST, m_ScannerBuffer);
}

// ------------------------------------------------------------------------------------------------
//! \fn UpdateSpokeDataWindow
//! Increment spoke count and write to window
void QuantumInterface::UpdateSpokeDataWindow()
{
	m_SpokeCount++;
	SetDlgItemInt(m_hDlg, IDC_SPOKE_COUNT, m_SpokeCount, FALSE);
}
// ------------------------------------------------------------------------------------------------
//! \fn UpdateNotificationWindow
//! Writes the entire rpa Data circular buffer to the Marpa Window
//! Function is called when WM_USER_notification_MSG is received. 
//! This detaches the Dialog Box from the Recieve Thread inside QuantumLib preventing deadlock
void QuantumInterface::UpdateNotificationWindow()
{
	m_NotificationBufferSize = 0;

	if (m_NotificationListWrapped == true)
	{
		for (uint32_t x = m_NotificationListIndex; x < cNotificationListSize; x++)
		{
			m_NotificationBufferSize += sprintf(m_NotificationBuffer + m_NotificationBufferSize, "%s", m_NotificationList[x]);
		}
	}
	for (uint32_t x = 0; x < m_NotificationListIndex; x++)
	{
		m_NotificationBufferSize += sprintf(m_NotificationBuffer + m_NotificationBufferSize, "%s", m_NotificationList[x]);
	}

	SetDlgItemText(m_hDlg, IDC_NOTIFICATIONS_LIST, m_NotificationBuffer);
}
// ------------------------------------------------------------------------------------------------
//! \fn UpdateMarpaWindow
//! Writes the entire Marpa Data circular buffer to the Marpa Window
//! Function is called when WM_USER_MARPA_MSG is received. 
//! This detaches the Dialog Box from the Recieve Thread inside QuantumLib preventing deadlock
void QuantumInterface::UpdateMarpaWindow()
{
	m_MarpaBufferSize = 0;
	if (m_MarpaListWrapped == true)
	{
		for (uint32_t x = m_MarpaListIndex; x < cMarpaListSize; x++)
		{
			m_MarpaBufferSize += sprintf(m_MarpaBuffer + m_MarpaBufferSize, "%s", m_MarpaList[x]);
		}
	}
	for (uint32_t x = 0; x < m_MarpaListIndex; x++)
	{
		m_MarpaBufferSize += sprintf(m_MarpaBuffer + m_MarpaBufferSize, "%s", m_MarpaList[x]);
	}

	SetDlgItemText(m_hDlg, IDC_MARPA_TARGET_LIST, m_MarpaBuffer);
}
// ------------------------------------------------------------------------------------------------
//! \fn UpdateAlarmWindow
//! Writes the entire Alarm Data circular buffer to the Alarm Window
//! Function is called when WM_USER_ALARM_MSG is received. 
//! This detaches the Dialog Box from the Recieve Thread inside QuantumLib preventing deadlock
void QuantumInterface::UpdateAlarmWindow()
{
	m_AlarmBufferSize = 0;
	if (m_AlarmListWrapped == true)
	{
		for (uint32_t x = m_AlarmListIndex; x < cAlarmListSize; x++)
		{
			m_AlarmBufferSize += sprintf(m_AlarmBuffer + m_AlarmBufferSize, "%s", m_AlarmList[x]);
		}
	}
	for (uint32_t x = 0; x < m_AlarmListIndex; x++)
	{
		m_AlarmBufferSize += sprintf(m_AlarmBuffer + m_AlarmBufferSize, "%s", m_AlarmList[x]);
	}
	SetDlgItemText(m_hDlg, IDC_ALARM_LIST, m_AlarmBuffer);
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
//! Buffers spoke data into circular buffer and sends user defined windows message to update dialog
void QuantumInterface::SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData)
{
	memcpy(&m_SpokeDataBuffer[m_SpokeDataBufferWriteIndex++], &rSpokeData, sizeof(QuantumLib::SpokeData_t));
	if (m_SpokeDataBufferWriteIndex >= cSpokeDataBufferSize)
	{
		m_SpokeDataBufferWriteIndex = 0;
	}
	SendNotifyMessage(m_hDlg, WM_USER_SPOKE_DATA_MSG, NULL, NULL);
}
// ------------------------------------------------------------------------------------------------
//! \fn SettingChanged
//! Notifcation callback from QuantumLib
//! Decodes SettingData_t and adds appropriate message to Notification buffer
//! Sends user defined windows message to update dialog
void QuantumInterface::SettingChanged(QuantumLib::SettingData_t& S)
{

	switch (S.GetSetting())
	{
		case QuantumLib::eSettingRadarMode:
			QuantumLib::eRadarMode_t RadarMode;
			S.GetValue(RadarMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x, RadarMode = %d\r\n", S.GetSerialNumber(), RadarMode);
			break;
		case QuantumLib::eSettingInterferenceRejection:
			QuantumLib::eInterferenceRejectionMode_t IRMode;
			S.GetValue(IRMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, IRMode = %d\r\n", S.GetSerialNumber(), IRMode);
			break;
		case QuantumLib::eSettingBearingAlignment:
			int16_t BearingAlignment;
			S.GetValue(BearingAlignment);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, BearingAlignment = %d\r\n", S.GetSerialNumber(), BearingAlignment);
			break;
		case QuantumLib::eSettingTimedTransmit:
			QuantumLib::TimedTransmit_t TimedTransmit;
			S.GetValue(TimedTransmit);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, TimedTx Scans = %d Mins = %d\r\n", S.GetSerialNumber(), TimedTransmit.TransmitCount_Scans, TimedTransmit.StandbyTime_Mins);
			break;
		case QuantumLib::eSettingTimedTransmitRemaining:
			QuantumLib::TimedTransmitRemaining_t TimedTransmitRemaining;
			S.GetValue(TimedTransmitRemaining);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, TimedTx Reamin Mins:Secs = %d:%d\r\n", S.GetSerialNumber(), TimedTransmitRemaining.Mins, TimedTransmitRemaining.Secs);
			break;
		case QuantumLib::eSettingTxFrequency:
			QuantumLib::eTransmitFrequency_t TransmitFrequency;
			S.GetValue(TransmitFrequency);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, TxFreq = %d\r\n", S.GetSerialNumber(), TransmitFrequency);
			break;
		case QuantumLib::eSettingGuardZoneSensitivity:
			uint8_t Sensitivity;
			S.GetValue(Sensitivity);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, GZSensitivity = %d\r\n", S.GetSerialNumber(), Sensitivity);
			break;
		case QuantumLib::eSettingGuardZone1:
		{
			QuantumLib::Zone_t Zone;
			S.GetValue(Zone);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, GZ1 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
			break;
		}
		case QuantumLib::eSettingGuardZone2:
		{
			QuantumLib::Zone_t Zone;
			S.GetValue(Zone);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, GZ2 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
			break;
		}
		case QuantumLib::eSettingAutoAccquireZone1:
		{
			QuantumLib::Zone_t Zone;
			S.GetValue(Zone);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, AAZ1 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
			break;
		}
		case QuantumLib::eSettingAutoAccquireZone2:
		{
			QuantumLib::Zone_t Zone;
			S.GetValue(Zone);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, AAZ2 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
			break;
		}
		case QuantumLib::eSettingCustomRanges:
			QuantumLib::CustomRanges_t CustomRanges;
			S.GetValue(CustomRanges);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x Ranges[1,8,16] = %f, %f, %f\r\n", S.GetSerialNumber(), CustomRanges.fRange[1], CustomRanges.fRange[8], CustomRanges.fRange[16]);
			break;
		case QuantumLib::eSettingRange:
			uint8_t RangeIndex;
			S.GetValue(RangeIndex);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, RangeIndex = %d\r\n", S.GetSerialNumber(), RangeIndex);
			break;
		case QuantumLib::eSettingPresetMode:
			QuantumLib::ePreset_t PresetMode;
			S.GetValue(PresetMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, PresetMode = %d\r\n", S.GetSerialNumber(), PresetMode);
			break;
		case QuantumLib::eSettingGainMode:
			QuantumLib::eGainMode_t GainMode;
			S.GetValue(GainMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, GainMode = %d\r\n", S.GetSerialNumber(), GainMode);
			break;
		case QuantumLib::eSettingGainValue:
			uint8_t GainValue;
			S.GetValue(GainValue);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, GainValue = %d\r\n", S.GetSerialNumber(), GainValue);
			break;
		case QuantumLib::eSettingColourGainMode:
			QuantumLib::eColourGainMode_t ColourGainMode;
			S.GetValue(ColourGainMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, ColourGainMode = %d\r\n", S.GetSerialNumber(), ColourGainMode);
			break;
		case QuantumLib::eSettingColourGainValue:
			uint8_t ColourGainValue;
			S.GetValue(ColourGainValue);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, ColourGainValue = %d\r\n", S.GetSerialNumber(), ColourGainValue);
			break;
		case QuantumLib::eSettingSeaMode:
			QuantumLib::eSeaMode_t SeaMode;
			S.GetValue(SeaMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, SeaMode = %d\r\n", S.GetSerialNumber(), SeaMode);
			break;
		case QuantumLib::eSettingSeaValue:
			uint8_t SeaValue;
			S.GetValue(SeaValue);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, SeaValue = %d\r\n", S.GetSerialNumber(), SeaValue);
			break;
		case QuantumLib::eSettingRainMode:
			QuantumLib::eRainMode_t RainMode;
			S.GetValue(RainMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, RainMode = %d\r\n", S.GetSerialNumber(), RainMode);
			break;
		case QuantumLib::eSettingRainValue:
			uint8_t RainValue;
			S.GetValue(RainValue);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, RainValue = %d\r\n", S.GetSerialNumber(), RainValue);
			break;
		case QuantumLib::eSettingTargetExpansion:
			QuantumLib::eTargetExpansion_t TargetExpansion;
			S.GetValue(TargetExpansion);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, TargetExpansion = %d\r\n", S.GetSerialNumber(), TargetExpansion);
			break;
		case QuantumLib::eSettingSeaClutterCurve:
			QuantumLib::eSeaCurve_t SeaCurve;
			S.GetValue(SeaCurve);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, SeaCurve = %d\r\n", S.GetSerialNumber(), SeaCurve);
			break;
		case QuantumLib::eSettingMainBang:
			QuantumLib::eMainBang_t MainBang;
			S.GetValue(MainBang);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, MBS = %d\r\n", S.GetSerialNumber(), MainBang);
			break;
		case QuantumLib::eSettingDopplerMode:
			QuantumLib::eDopplerMode_t DopplerMode;
			S.GetValue(DopplerMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, DopplerMode = %d\r\n", S.GetSerialNumber(), DopplerMode);
			break;
		case QuantumLib::eSettingDopplerActive:
			QuantumLib::eDopplerActive_t DopplerActive;
			S.GetValue(DopplerActive);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, DopplerActive = %d\r\n", S.GetSerialNumber(), DopplerActive);
			break;
		case QuantumLib::eSettingAutoAcquireMode:
			QuantumLib::eAutoAcquireMode_t AutoAcquireMode;
			S.GetValue(AutoAcquireMode);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting:  Sno=0x%08x, AutoAcquireMode = %d\r\n", S.GetSerialNumber(), AutoAcquireMode);
			break;
		case QuantumLib::eSettingMainSoftwareVersion:
		{
			uint32_t Version;
			S.GetValue(Version);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x, Main SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
			break;
		}
		case QuantumLib::eSettingPsuSoftwareVersion:
		{
			uint32_t Version;
			S.GetValue(Version);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x, PSU SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
			break;
		}
		case QuantumLib::eSettingFpgaSoftwareVersion:
		{
			uint32_t Version;
			S.GetValue(Version);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x, FPGA Vers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
			break;
		}
		case QuantumLib::eSettingWiFiSoftwareVersion:
		{
			uint32_t Version;
			S.GetValue(Version);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x, WiFi SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
			break;
		}
		case QuantumLib::eSettingW3SoftwareVersion:
		{
			uint32_t Version;
			S.GetValue(Version);
			sprintf(m_NotificationList[m_NotificationListIndex], "Setting: Sno=0x%08x, W3 SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
			break;
		}
		default:
			break;
	}

	m_NotificationListIndex++;
	if (m_NotificationListIndex >= cNotificationListSize)
	{
		m_NotificationListWrapped = true;
		m_NotificationListIndex = 0;
	}

	SendNotifyMessage(m_hDlg, WM_USER_NOTIFICATION_MSG, NULL, NULL);
}
// ------------------------------------------------------------------------------------------------
//! \fn FeatureChanged
//! Notifcation callback from QuantumLib
//! Decodes Features and adds appropriate message to Notification buffer
//! Sends user defined windows message to update dialog
void QuantumInterface::FeatureChanged(uint32_t SerialNumber, QuantumLib::eFeatures_t Feature, bool Supported)
{
	switch (Feature)
	{
	case QuantumLib::eFeatureDualRange:
		sprintf(m_NotificationList[m_NotificationListIndex], "Feature: Sno=0x%08x, DualRangeSupported = %d\r\n", SerialNumber, Supported);
		break;
	case QuantumLib::eFeature48RPM:
		sprintf(m_NotificationList[m_NotificationListIndex], "Feature: Sno=0x%08x, 48RPMSupported = %d\r\n", SerialNumber, Supported);
		break;
	case QuantumLib::eFeatureDoppler:
		sprintf(m_NotificationList[m_NotificationListIndex], "Feature: Sno=0x%08x, DopplerSupported = %d\r\n", SerialNumber, Supported);
		break;
	case QuantumLib::eFeatureAutoAcquire:
		sprintf(m_NotificationList[m_NotificationListIndex], "Feature: Sno=0x%08x, AutoAcquireSupported = %d\r\n", SerialNumber, Supported);
		break;
	default:
		break;
	}
	
	m_NotificationListIndex++;
	if (m_NotificationListIndex >= cNotificationListSize)
	{
		m_NotificationListWrapped = true;
		m_NotificationListIndex = 0;
	}
	
	SendNotifyMessage(m_hDlg, WM_USER_NOTIFICATION_MSG, NULL, NULL);
}
// ------------------------------------------------------------------------------------------------
//! \fn ParameterChanged
//! Notifcation callback from QuantumLib
//! Decodes Parameter value and adds appropriate message to Notification buffer
//! Sends user defined windows message to update dialog
void QuantumInterface::ParameterChanged(uint32_t SerialNumber, QuantumLib::eParameters_t Parameter, uint32_t Value)
{
	switch (Parameter)
	{
	case QuantumLib::eParamMinRange:
		sprintf(m_NotificationList[m_NotificationListIndex], "Param: Sno=0x%08x, MinRange=%u\r\n", SerialNumber, Value);
		break;
	case QuantumLib::eParamMaxRange:
		sprintf(m_NotificationList[m_NotificationListIndex], "Param: Sno=0x%08x, MaxRange=%u\r\n", SerialNumber, Value);
		break;
	case QuantumLib::eParamMarpaTargets:
		sprintf(m_NotificationList[m_NotificationListIndex], "Param: Sno=0x%08x, MarpaTargets=%u\r\n", SerialNumber, Value);
		break;
	default:
		break;
	}

	m_NotificationListIndex++;
	if (m_NotificationListIndex >= cNotificationListSize)
	{
		m_NotificationListWrapped = true;
		m_NotificationListIndex = 0;
	}

	SendNotifyMessage(m_hDlg, WM_USER_NOTIFICATION_MSG, NULL, NULL);
}
// ------------------------------------------------------------------------------------------------
//! \fn MarpaDataChanged
//! Notifcation callback from QuantumLib
//! Adds MarpaData to circular buffer
//! Sends user defined windows message to update dialog
void QuantumInterface::MarpaDataChanged(uint32_t SerialNumber, QuantumLib::MarpaData_t& MarpaData)
{
	sprintf(m_MarpaList[m_MarpaListIndex], "Marpa: Sno=0x%08x, Id=%u, Valid=%d Type = %d\r\n", SerialNumber, MarpaData.Id, MarpaData.Valid, MarpaData.State);
	m_MarpaListIndex++;
	if (m_MarpaListIndex >= cMarpaListSize)
	{
		m_MarpaListWrapped = true;
		m_MarpaListIndex = 0;
	}
	SendNotifyMessage(m_hDlg, WM_USER_MARPA_MSG, NULL, NULL);
}
// ------------------------------------------------------------------------------------------------
//! \fn AlarmDataChanged
//! Notifcation callback from QuantumLib
//! Adds AlarmData to circular buffer
//! Sends user defined windows message to update dialog
void QuantumInterface::AlarmDataChanged(uint32_t SerialNumber, QuantumLib::AlarmData_t& AlarmData)
{
	sprintf(m_AlarmList[m_AlarmListIndex], "Alarm: Sno=0x%08x, Id=%u, Type=%d, Value=%u\r\n", SerialNumber, AlarmData.Id, AlarmData.Type, AlarmData.Value);
	m_AlarmListIndex++;
	if (m_AlarmListIndex >= cAlarmListSize)
	{
		m_AlarmListWrapped = true;
		m_AlarmListIndex = 0;
	}
	SendNotifyMessage(m_hDlg, WM_USER_ALARM_MSG, NULL, NULL);
}
// ------------------------------------------------------------------------------------------------
//! \fn OnSendCommand
//! Reads and decodes the command line
//! Creates a SettingData_t structure with appropriate Serial Number, Setting and Channel
//! Completes SettingData_t using appropriate Value for the Setting specified
//! calls API::NewScannerSetting with the completed SettingData_t structure to be sent to Quantum Radar
void QuantumInterface::OnSendCommand()
{
	bool ret = false;
	static const uint32_t cTempBufferSize = 256;
	char TempBuffer[cTempBufferSize];

	TempBuffer[0] = 0;
	SetDlgItemText(m_hDlg, IDC_COMMAND_RETURN_STATUS, TempBuffer);

	int retval = GetDlgItemText(m_hDlg, IDC_COMMAND_LINE, TempBuffer, cTempBufferSize);
	if (retval != 0)
	{
		CLI::CLI_Command_t CommandLineParams;
		m_pCLI->ProcessCommandLine(TempBuffer, &CommandLineParams);
		if (CommandLineParams.ParamCount > 0)
		{
			QuantumLib::eErrorCode_t ErrorCode = QuantumLib::eNoError;
			uint32_t SerNo = _GetSerialNumber(CommandLineParams.ScannerIndex);
			QuantumLib::eSettings_t Setting = static_cast<QuantumLib::eSettings_t>(CommandLineParams.CommandIndex);
		
			QuantumLib::SettingData_t S(Setting, SerNo);
			ret = _BuildDataStructure(S, CommandLineParams.ParamCount, CommandLineParams.Param);

			if (ret == true)
			{
				ErrorCode = QuantumLib::API::NewScannerSetting(S);
			}
			else
			{
				ErrorCode = QuantumLib::eErrorInvalidNumberOfParameters;
			}
			if (ErrorCode != QuantumLib::eNoError)
			{
				sprintf_s(TempBuffer, cTempBufferSize, "Command Failed ErrCode= %d, SerNo=0x%08x, Setting=%d, ParamCount=%u", ErrorCode, SerNo, Setting, CommandLineParams.ParamCount);
			}
			else
			{
				sprintf_s(TempBuffer, cTempBufferSize, "Command Sent SerNo=0x%08x, Setting=%d, ParamCount=%u", SerNo, Setting, CommandLineParams.ParamCount);
			}
			SetDlgItemText(m_hDlg, IDC_COMMAND_RETURN_STATUS, TempBuffer);
		}
	}
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



