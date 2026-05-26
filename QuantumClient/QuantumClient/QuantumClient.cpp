// -------------------------------------------------------------------------------------------------
//! \file QuantumClient.cpp
//! Main calss for console app
//! Provides handlers for notification callback functions for interface to Quantum Library
//! Creates a CLI class and waits in a loop processing command line input

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// ------------------------------------------------------------------------------------------
// include files

// ------------------------------------------------------------------------------------------
#include "stdafx.h"
#include "QuantumClient.h"
#include "QuantumLib.h"
#include "QuantumLibTypes.h"
#include "CLI.h"
#include "../../QuantumCommonSource/BuildDataStructure.h"
#include <Windows.h>
#include <new>

// ------------------------------------------------------------------------------------------
//! Constructor
QuantumClient::QuantumClient() :
m_LastSpokeNumber(0)
{
}


// ------------------------------------------------------------------------------------------
//! Destructor
QuantumClient::~QuantumClient()
{
}

// ------------------------------------------------------------------------------------------
//! \fn int QuantumClient::Run()
//! loads the DLL by calling the Open method on the DLL and registers with the DLL
//! Creates a command line interpreter and waits for command line input
int QuantumClient::Run()
{
	QuantumLib::eErrorCode_t ErrorCode = QuantumLib::eNoError;
	// load the QuantumLib DLL
	ErrorCode = QuantumLib::API::Open();
	if (ErrorCode == QuantumLib::eNoError)
	{
		// Register this class for notification callbacks from the DLL
		ErrorCode = QuantumLib::API::RegisterForScannerNotifications(this);
		if (ErrorCode == QuantumLib::eNoError)
		{
			// create a command line interpreter class
			CLI* pCLI = new(std::nothrow) CLI;

			if (pCLI)
			{
				CLI::CLI_Command_t CommandLineParams;
				bool bExit = false;
				bool ret = false;

				while (bExit == false)
				{
					// read and process command line
					if (pCLI->ProcessCommandLine(&CommandLineParams))
					{
						if (CommandLineParams.ParamCount > 0)
						{
							// get serial number from scanner index 
							uint32_t SerNo = _GetSerialNumber(CommandLineParams.ScannerIndex);
							QuantumLib::eSettings_t Setting = static_cast<QuantumLib::eSettings_t>(CommandLineParams.CommandIndex);

							// create the structure to send command to QuantumLib DLL
							QuantumLib::SettingData_t S(Setting, SerNo);
							ret = _BuildDataStructure(S, CommandLineParams.ParamCount, CommandLineParams.Param);

							if (ret == true)
							{
								// send command to DLL to send to Quantum
								// DLL validates the parameters in SettingData_t structure, converts to protocol message and sends to Quantum Radar
								ErrorCode = QuantumLib::API::NewScannerSetting(S);
							}
							else
							{
								ErrorCode = QuantumLib::eErrorInvalidNumberOfParameters;
							}
							if (ErrorCode != QuantumLib::eNoError)
							{
								// display error information
								printf("Client: Command Failed ErrorCode = %d, SerNo=0x%08x, Setting=%d, ParamCount=%u\n", ErrorCode, SerNo, Setting, CommandLineParams.ParamCount);
							}
						}
					}
					else
					{
						bExit = true;
					}

					Sleep(500);
				}
				if (pCLI)
				{
					delete pCLI;
					pCLI = 0;
				}
			}
			QuantumLib::API::DeRegisterForScannerNotifications(this);
		}
		QuantumLib::API::Close();
	}
	return ErrorCode;
}

// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::ScannerListChanged(uint32_t ScannerCount)
//! Notify handler called when scanner are detected or lost
//! print info to console window
void QuantumClient::ScannerListChanged(uint32_t ScannerCount)
{
	printf("Client: Nr of scanners = %u\n", ScannerCount);
	QuantumLib::ScannerDetails_t ScannerDetails;

	for (uint32_t index = 0; index < ScannerCount; index++)
	{
		if (QuantumLib::API::GetScannerDetails(index, ScannerDetails) != QuantumLib::eNoError)
		{
			printf("Client: %s, SerNo 0x%08x\n", ScannerDetails.Description, ScannerDetails.SerialNumber);
		}
	}

}

// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData)
//! Notify handler called when spoke data received
//! print Missing spoke numbers to console window
void QuantumClient::SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData)
{
	uint32_t NewSpokeNumber = rSpokeData.Bearing;

	if (NewSpokeNumber != (m_LastSpokeNumber + 1) % QuantumLib::cSpokesPerScan)
	{
		// need to create a list for all scanners if we want to tracl missing spokes in this manner 
		//printf("Client: Missing Spoke Bearing=%d, Last=%d\n", NewSpokeNumber, m_LastSpokeNumber);
	}
	m_LastSpokeNumber = NewSpokeNumber;
}
// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::SettingChanged(QuantumLib::SettingData_t& rSettingData)
//! Notify handler called when setting changed
//! print details of setting change to console window
void QuantumClient::SettingChanged(QuantumLib::SettingData_t& S)
{
	switch (S.GetSetting())
	{
	case QuantumLib::eSettingRadarMode:
		QuantumLib::eRadarMode_t RadarMode;
		S.GetValue(RadarMode);
		printf("Setting: Sno=0x%08x, RadarMode = %d\r\n", S.GetSerialNumber(), RadarMode);
		break;
	case QuantumLib::eSettingInterferenceRejection:
		QuantumLib::eInterferenceRejectionMode_t IRMode;
		S.GetValue(IRMode);
		printf("Setting:  Sno=0x%08x, IRMode = %d\r\n", S.GetSerialNumber(), IRMode);
		break;
	case QuantumLib::eSettingBearingAlignment:
		int16_t BearingAlignment;
		S.GetValue(BearingAlignment);
		printf("Setting:  Sno=0x%08x, BearingAlignment = %d\r\n", S.GetSerialNumber(), BearingAlignment);
		break;
	case QuantumLib::eSettingTimedTransmit:
		QuantumLib::TimedTransmit_t TimedTransmit;
		S.GetValue(TimedTransmit);
		printf("Setting:  Sno=0x%08x, TimedTx Scans = %d Mins = %d\r\n", S.GetSerialNumber(), TimedTransmit.TransmitCount_Scans, TimedTransmit.StandbyTime_Mins);
		break;
	case QuantumLib::eSettingTimedTransmitRemaining:
		QuantumLib::TimedTransmitRemaining_t TimedTransmitRemaining;
		S.GetValue(TimedTransmitRemaining);
		printf("Setting:  Sno=0x%08x, TimedTx Reamin Mins:Secs = %d:%d\r\n", S.GetSerialNumber(), TimedTransmitRemaining.Mins, TimedTransmitRemaining.Secs);
		break;
	case QuantumLib::eSettingTxFrequency:
		QuantumLib::eTransmitFrequency_t TransmitFrequency;
		S.GetValue(TransmitFrequency);
		printf("Setting:  Sno=0x%08x, TxFreq = %d\r\n", S.GetSerialNumber(), TransmitFrequency);
		break;
	case QuantumLib::eSettingGuardZoneSensitivity:
		uint8_t Sensitivity;
		S.GetValue(Sensitivity);
		printf("Setting:  Sno=0x%08x, GZSensitivity = %d\r\n", S.GetSerialNumber(), Sensitivity);
		break;
	case QuantumLib::eSettingGuardZone1:
	{
		QuantumLib::Zone_t Zone;
		S.GetValue(Zone);
		printf("Setting:  Sno=0x%08x, GZ1 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
		break;
	}
	case QuantumLib::eSettingGuardZone2:
	{
		QuantumLib::Zone_t Zone;
		S.GetValue(Zone);
		printf("Setting:  Sno=0x%08x, GZ2 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
		break;
	}
	case QuantumLib::eSettingAutoAccquireZone1:
	{
		QuantumLib::Zone_t Zone;
		S.GetValue(Zone);
		printf("Setting:  Sno=0x%08x, AAZ1 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
		break;
	}
	case QuantumLib::eSettingAutoAccquireZone2:
	{
		QuantumLib::Zone_t Zone;
		S.GetValue(Zone);
		printf("Setting:  Sno=0x%08x, AAZ2 Range(%u:%u) Angle(%u:%u) Enable(%d)\r\n", S.GetSerialNumber(), Zone.StartRange_nm_x1000, Zone.EndRange_nm_x1000, Zone.StartAngle_degs_x10, Zone.EndAngle_degs_x10, Zone.Enable);
		break;
	}
	case QuantumLib::eSettingCustomRanges:
		QuantumLib::CustomRanges_t CustomRanges;
		S.GetValue(CustomRanges);
		printf("Setting: Sno=0x%08x Ranges[1,8,16] = %f, %f, %f\r\n", S.GetSerialNumber(), CustomRanges.fRange[1], CustomRanges.fRange[8], CustomRanges.fRange[16]);
		break;
	case QuantumLib::eSettingRange:
		uint8_t RangeIndex;
		S.GetValue(RangeIndex);
		printf("Setting:  Sno=0x%08x, RangeIndex = %d\r\n", S.GetSerialNumber(), RangeIndex);
		break;
	case QuantumLib::eSettingPresetMode:
		QuantumLib::ePreset_t PresetMode;
		S.GetValue(PresetMode);
		printf("Setting:  Sno=0x%08x, PresetMode = %d\r\n", S.GetSerialNumber(), PresetMode);
		break;
	case QuantumLib::eSettingGainMode:
		QuantumLib::eGainMode_t GainMode;
		S.GetValue(GainMode);
		printf("Setting:  Sno=0x%08x, GainMode = %d\r\n", S.GetSerialNumber(), GainMode);
		break;
	case QuantumLib::eSettingGainValue:
		uint8_t GainValue;
		S.GetValue(GainValue);
		printf("Setting:  Sno=0x%08x, GainValue = %d\r\n", S.GetSerialNumber(), GainValue);
		break;
	case QuantumLib::eSettingColourGainMode:
		QuantumLib::eColourGainMode_t ColourGainMode;
		S.GetValue(ColourGainMode);
		printf("Setting:  Sno=0x%08x, ColourGainMode = %d\r\n", S.GetSerialNumber(), ColourGainMode);
		break;
	case QuantumLib::eSettingColourGainValue:
		uint8_t ColourGainValue;
		S.GetValue(ColourGainValue);
		printf("Setting:  Sno=0x%08x, ColourGainValue = %d\r\n", S.GetSerialNumber(), ColourGainValue);
		break;
	case QuantumLib::eSettingSeaMode:
		QuantumLib::eSeaMode_t SeaMode;
		S.GetValue(SeaMode);
		printf("Setting:  Sno=0x%08x, SeaMode = %d\r\n", S.GetSerialNumber(), SeaMode);
		break;
	case QuantumLib::eSettingSeaValue:
		uint8_t SeaValue;
		S.GetValue(SeaValue);
		printf("Setting:  Sno=0x%08x, SeaValue = %d\r\n", S.GetSerialNumber(), SeaValue);
		break;
	case QuantumLib::eSettingRainMode:
		QuantumLib::eRainMode_t RainMode;
		S.GetValue(RainMode);
		printf("Setting:  Sno=0x%08x, RainMode = %d\r\n", S.GetSerialNumber(), RainMode);
		break;
	case QuantumLib::eSettingRainValue:
		uint8_t RainValue;
		S.GetValue(RainValue);
		printf("Setting:  Sno=0x%08x, RainValue = %d\r\n", S.GetSerialNumber(), RainValue);
		break;
	case QuantumLib::eSettingTargetExpansion:
		QuantumLib::eTargetExpansion_t TargetExpansion;
		S.GetValue(TargetExpansion);
		printf("Setting:  Sno=0x%08x, TargetExpansion = %d\r\n", S.GetSerialNumber(), TargetExpansion);
		break;
	case QuantumLib::eSettingSeaClutterCurve:
		QuantumLib::eSeaCurve_t SeaCurve;
		S.GetValue(SeaCurve);
		printf("Setting:  Sno=0x%08x, SeaCurve = %d\r\n", S.GetSerialNumber(), SeaCurve);
		break;
	case QuantumLib::eSettingMainBang:
		QuantumLib::eMainBang_t MainBang;
		S.GetValue(MainBang);
		printf("Setting:  Sno=0x%08x, MBS = %d\r\n", S.GetSerialNumber(), MainBang);
		break;
	case QuantumLib::eSettingDopplerMode:
		QuantumLib::eDopplerMode_t DopplerMode;
		S.GetValue(DopplerMode);
		printf("Setting:  Sno=0x%08x, DopplerMode = %d\r\n", S.GetSerialNumber(), DopplerMode);
		break;
	case QuantumLib::eSettingDopplerActive:
		QuantumLib::eDopplerActive_t DopplerActive;
		S.GetValue(DopplerActive);
		printf("Setting:  Sno=0x%08x, DopplerActive = %d\r\n", S.GetSerialNumber(), DopplerActive);
		break;
	case QuantumLib::eSettingAutoAcquireMode:
		QuantumLib::eAutoAcquireMode_t AutoAcquireMode;
		S.GetValue(AutoAcquireMode);
		printf("Setting:  Sno=0x%08x, AutoAcquireMode = %d\r\n", S.GetSerialNumber(), AutoAcquireMode);
		break;
	case QuantumLib::eSettingMainSoftwareVersion:
	{
		uint32_t Version;
		S.GetValue(Version);
		printf("Setting: Sno=0x%08x, Main SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
		break;
	}
	case QuantumLib::eSettingPsuSoftwareVersion:
	{
		uint32_t Version;
		S.GetValue(Version);
		printf("Setting: Sno=0x%08x, PSU SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
		break;
	}
	case QuantumLib::eSettingFpgaSoftwareVersion:
	{
		uint32_t Version;
		S.GetValue(Version);
		printf("Setting: Sno=0x%08x, FPGA Vers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
		break;
	}
	case QuantumLib::eSettingWiFiSoftwareVersion:
	{
		uint32_t Version;
		S.GetValue(Version);
		printf("Setting: Sno=0x%08x, WiFi SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
		break;
	}
	case QuantumLib::eSettingW3SoftwareVersion:
	{
		uint32_t Version;
		S.GetValue(Version);
		printf("Setting: Sno=0x%08x, W3 SwVers = %u.%u\r\n", S.GetSerialNumber(), Version / 100, Version % 100);
		break;
	}
	default:
		break;
	}
}

// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::FeatureChanged(uint32_t SerialNumber, QuantumLib::eFeatures_t Feature, bool Supported)
//! Notify handler called when feature has changed
//! Features are normally fixed so this function is only called on power up to notify the application of the avialable features
//! print details of features to console window
void QuantumClient::FeatureChanged(uint32_t SerialNumber, QuantumLib::eFeatures_t Feature, bool Supported)
{
	switch (Feature)
	{
	case QuantumLib::eFeatureDualRange:
		printf("ClientFeature: Sno=0x%08x, DualRangeSupported = %d\n", SerialNumber, Supported);
		break;
	case QuantumLib::eFeature48RPM:
		printf("ClientFeature: Sno=0x%08x, 48RPMSupported = %d\n", SerialNumber, Supported);
		break;
	case QuantumLib::eFeatureDoppler:
		printf("ClientFeature: Sno=0x%08x, DopplerSupported = %d\n", SerialNumber, Supported);
		break;
	case QuantumLib::eFeatureAutoAcquire:
		printf("ClientFeature: Sno=0x%08x, AutoAcquireSupported = %d\n", SerialNumber, Supported);
		break;
	default:
		break;
	}
}
// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::ParameterChanged(uint32_t SerialNumber, QuantumLib::eParameters_t Parameter, uint32_t Value)
//! Notify handler called when parameter has changed
//! Parameters are normally fixed so this function is only called on power up to notify the application of the parameter values
//! print details of parameters to console window
void QuantumClient::ParameterChanged(uint32_t SerialNumber, QuantumLib::eParameters_t Parameter, uint32_t Value)
{
	switch (Parameter)
	{
	case QuantumLib::eParamMinRange:
		printf("ClientParam: Sno=0x%08x, MinRange=%u\n", SerialNumber, Value);
		break;
	case QuantumLib::eParamMaxRange:
		printf("ClientParam: Sno=0x%08x, MaxRange=%u\n", SerialNumber, Value);
		break;
	case QuantumLib::eParamMarpaTargets:
		printf("ClientParam: Sno=0x%08x, MarpaTargets=%u\n", SerialNumber, Value);
		break;
	default:
		break;
	}

}
// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::MarpaDataChanged(uint32_t SerialNumber, QuantumLib::MarpaData_t& rMarpaData)
//! Notify handler called when marpa data has changed
//! print details of marpa data to console window
void QuantumClient::MarpaDataChanged(uint32_t SerialNumber, QuantumLib::MarpaData_t& rMarpaData)
{
	printf("ClientMarpaData: Sno=0x%08x, Id=%d, Valid=%d Type = %d\n", SerialNumber, rMarpaData.Id, rMarpaData.Valid, rMarpaData.State);

}
// ------------------------------------------------------------------------------------------
//! \fn void QuantumClient::AlarmDataChanged(uint32_t SerialNumber, QuantumLib::AlarmData_t& rAlarmData)
//! Notify handler called when alarm data has changed
//! print details of alarm data to console window
void QuantumClient::AlarmDataChanged(uint32_t SerialNumber, QuantumLib::AlarmData_t& rAlarmData)
{
	printf("ClientAlarm: Sno=0x%08x, Id=%d, Type=%d, Value=%d\n", SerialNumber, rAlarmData.Id, rAlarmData.Type, rAlarmData.Value);
}

// ------------------------------------------------------------------------------------------
//! \fn uint32_t QuantumClient::_GetSerialNumber(uint32_t Index)
//! Converts scanner Index into 32 bit serial number
uint32_t QuantumClient::_GetSerialNumber(uint32_t Index)
{
	QuantumLib::ScannerDetails_t ScannerDetails;
	uint32_t SerNo = 0xFFFFFFFF;

	if (QuantumLib::API::GetScannerDetails(Index, ScannerDetails) == QuantumLib::eNoError)
	{
		SerNo = ScannerDetails.SerialNumber;
	}
	return SerNo;
}

