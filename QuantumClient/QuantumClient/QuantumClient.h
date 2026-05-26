// ------------------------------------------------------------------------------------------------
//! \file QuantumClient.h
//! Defines the QuantumClient class
//! Main interface between the console application and the DLL

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// ------------------------------------------------------------------------------------------------
#pragma once

// ------------------------------------------------------------------------------------------------
// include files

#include "QuantumLib.h"

// ------------------------------------------------------------------------------------------------
//! \class QuantumClient
//! Derived from INotify base class
//! Provides example callback routines for all notify handlers
class QuantumClient : QuantumLib::INotify
{
public:
	QuantumClient();
	~QuantumClient();

	int Run();

	void ScannerListChanged(uint32_t ScannerCount);
	void SpokeDataReceived(uint32_t SerialNumber, QuantumLib::SpokeData_t& rSpokeData) ;
	void SettingChanged(QuantumLib::SettingData_t& rSettingData);
	void FeatureChanged(uint32_t SerialNumber, QuantumLib::eFeatures_t Feature, bool Supported) ;
	void ParameterChanged(uint32_t SerialNumber, QuantumLib::eParameters_t Parameter, uint32_t Value) ;
	void MarpaDataChanged(uint32_t SerialNumber, QuantumLib::MarpaData_t& rMarpaData) ;
	void AlarmDataChanged(uint32_t SerialNumber, QuantumLib::AlarmData_t& rAlarmData) ;

private:
	uint32_t _GetSerialNumber(uint32_t Index);

	uint32_t m_LastSpokeNumber;
};

// ------------------------------------------------------------------------------------------------


