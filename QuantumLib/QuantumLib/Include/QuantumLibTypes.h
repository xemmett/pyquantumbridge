//! \file QuantumLibTypes.h
//! \brief	Structure definitions and enumerated types for parameters and data passed between the API and Client application
//! \details Definitions of enumerated types and values for setting values in the scanner and notification messages from the scanner
//! \details Structures used for Spoke Data and Marpa Data sent from the scanner
//! \details Structures and enumerated types for Alarm Messages and Alarm Handling

//! \copyright Copyright (C) 2018, 2019 Raymarine - All Rights Reserved
//! \copyright You may use, distribute and modify this code under the
//! \copyright terms of the Raymarine Radar SDK License Agreement.
//! \copyright You should have received a copy of the Raymarine Radar SDK License Agreement with
//! \copyright this file. If not, please contact Raymarine.


#pragma once

#include <stdint.h>
#include <cstring>

namespace QuantumLib
{
	static const uint32_t cQuantumUnitType = 76;					//!< Unit Type for Quantum Radar Q24C
	static const uint32_t cQuantumW3UnitType = 77;					//!< Unit Type for Quantum Wired adaptor
	static const uint32_t cQuantumDopplerUnitType = 102;			//!< Unit Type for Quantum Doppler Radar Q24D

	static const uint8_t cQuantumDescription[32] = "Quantum Radome";					//!< Textual Description for Quantum Radar
	static const uint8_t cQuantumDopplerDescription[32] = "Quantum Doppler Radome";		//!< Textual Description for Quantum Doppler Radar
	static const uint8_t cUnknownDescription[32] = "Unknown";

	static const uint32_t cSamplesPerSpoke = 1024;					//!< Number of Samples Per Spoke 
	static const uint32_t cSpokesPerScan = 2048;					//!< Number of Spoke Per Scan

	static const uint32_t cMaxCustomRanges = 20;					//!< Maximum number of Custom Ranges

	//! \struct CustomRanges_t
	//! Structure containing Serial Number and Textual description of the scanner
	struct CustomRanges_t
	{
		float fRange[cMaxCustomRanges];
	};


	//! \struct ScannerDetails_t
	//! Structure containing Serial Number and Textual description of the scanner
	//! \sa ScannerListChanged
	//! \sa GetScannerDetails
	struct ScannerDetails_t
	{
		uint8_t		Description[32];		//!< Text Description of Scanner
		uint32_t	SerialNumber;			//!< Scanner Serial Number encrypted to 32 bit value
	};

	//! \struct SpokeData_t
	//! Spoke Data and header information
	struct SpokeData_t
	{
		uint16_t	SamplesPerSpoke;    //!< count of samples in each spoke
		uint16_t	SpokesPerScan;		//!< count of spokes in each scan
		uint8_t     Channel;			//!< 0-Default or 1st Range in Dual Range mode 1- 2nd Range in Dual Range Mode
		uint16_t	InstrumentedRange;  //!< sample offset representing instrumented range
		uint16_t	Bearing;            //!< 0-2047
		uint16_t	DataLength;			//!< initially used by app layer, but may be modified by transport layer (e.g. if data is compressed)
		uint8_t		SpokeData[cSamplesPerSpoke];	    //<! dummy place holder for variable length data field
	};

	//! \enum eFeatures_t
	//! Feature flags indicate functions that are supported by the scanner.\n
	//! Notification messages will be sent to FeatureChanged callback function.
	//! \sa FeatureChanged
	enum eFeatures_t
	{
		eFeatureDualRange = 1,			//!< Dual Range 
		eFeature48RPM = 2,				//!< 48RPM
		eFeatureDoppler = 3,			//!< Doppler 
		eFeatureAutoAcquire = 4			//!< Auto Target Acquisition
	};

	//! \enum eParameters_t
	//! Parameter settings to indicate the limits of functionality in the scanner.\n
	//! Notification messages will be sent to the ParameterChanged callback function.
	//! \sa ParameterChanged
	enum eParameters_t
	{
		eParamMinRange = 0,				//!< Minimum Range Index
		eParamMaxRange = 1,				//!< Maximum Range Index
		eParamMarpaTargets = 2			//!< Maximum Marpa Targets available
	};

	//! \enum eSettings_t 
	//! These can be used to send Commands to the scanner or listen for Notifications of parameters changing in the scaner.\n
	//! Some parameters can only be used for commands and some for notification only as detailed.
	//! Notification messages will be sent to the SettingChanged callback function
	//! \sa SettingChanged
	enum eSettings_t
	{
		eSettingUndefined = 0,
		eSettingRadarMode = 1,					//!< Set/Notify Standby, Transmit, Power Down \sa eRadarMode_t
		eSettingInterferenceRejection = 2,		//!< Level of Interference Rejection applied \sa eInterferenceRejectionMode_t
		eSettingBearingAlignment = 3,			//!< Bearing alignment -179.5 to +180.0 
		eSettingTimedTransmit = 4,				//!< Timed Transmit Parameters, Standby period and transmit scan count \sa TimedTransmit_t
		eSettingTxFrequency = 5,				//!< Tx Frequency. Nominal, High or Low \sa eTransmitFrequency_t
		eSettingGuardZoneSensitivity = 6,		//!< Gaurd Zone sensitiivty 0-100%
		eSettingGuardZone1 = 7,					//!< Guard Zone 1 start/end range/bearing \sa Zone_t 
		eSettingGuardZone2 = 8,					//!< Guard Zone 2 start/end range/bearing \sa Zone_t 
		eSettingGuardZoneEnable = 9,			//!< Enable/Disable Guard Zone 1 or 2 (Command only) \sa ZoneState_t
		eSettingAutoAccquireZone1 = 10,			//!< Auto Acquire Zone 1 start/end range/bearing \sa Zone_t 
		eSettingAutoAccquireZone2 = 11,			//!< Auto Acquire Zone 2 start/end range/bearing \sa Zone_t
		eSettingAutoAcquireMode = 12,			//!< 0- Off (Feature currently not supported) \sa eAutoAcquireMode_t
		eSettingAutoAcquireZoneEnable = 13,		//!< Enable/Disable Auto Acquire Zone 1 or 2 (Command only) \sa ZoneState_t
		eSettingCustomRanges = 14,				//!< List of Instrumented Ranges corresponding to Range Index (Notification Only)

		eSettingRange = 20,						//!< Set Range Index 
		eSettingPresetMode = 21,				//!< Set Preset Mode Harbour, Coastal, Offshore, Weather \sa ePreset_t
		eSettingGainMode = 22,					//!< Gain Mode, Auto or Manual \sa eGainMode_t
		eSettingGainValue = 23,					//!< Gain Value 0-100%
		eSettingColourGainMode = 24,			//!< Colour Gain Mode, Auto or Manual \sa eColourGainMode_t
		eSettingColourGainValue = 25,			//!< Colour Gain Value, 0-100%
		eSettingSeaMode = 26,					//!< Sea Mode, Auto or Manual	\sa eSeaMode_t
		eSettingSeaValue = 27,					//!< Sea Value 0-100%
		eSettingRainMode = 28,					//!< Rain Mode, Auto or Manual \sa eRainMode_t
		eSettingRainValue = 29,					//!< Rain Value 0-100%
		eSettingTargetExpansion = 30,			//!< Target Expansion on/off \sa eTargetExpansion_t
		eSettingSeaClutterCurve = 31,			//!< Sea clutter curve 1 or 2 - Manual Sea mode only \sa eSeaCurve_t
		eSettingMainBang = 32,					//!< Turn Main Bang Suppression ON/OFF \sa eMainBang_t

		eSettingDopplerMode = 40,				//!< Turn doppler mode ON/OFF \sa eDopplerMode_t
		eSettingDopplerActive = 41,				//!< Indication if Dopler is active (Notification only) \sa eDopplerActive_t

		eSettingTimedTransmitRemaining = 50,	//!< Time remaining in Transmit mode before standby period (Notification only)

		eSettingNavData = 60,					//!< Send Heading, SOG and COG data to scanner for Marpa/Doppler functionality (Command only) \sa NavData_t
		eSettingMarpaSetup = 61,				//!< Send Marpa Alarm setup data to scanner (Command only) \sa MarpaSetup_t
		eSettingMarpaDesignate = 62,			//!< Designate Marpa Target (Command only) \sa MarpaDesignate_t
		eSettingMarpaClear = 63,				//!< Remove Marpa Target (Command Only)
		eSettingAlarmAck = 64,					//!< Acknowledge alarm (Command only) \sa AlarmAck_t

		eSettingMainSoftwareVersion = 70,		//!< Main App Software version (Major * 100) + Minor (Notification Only)
		eSettingPsuSoftwareVersion = 71,		//!< PSU App Software version (Major * 100) + Minor (Notification Only)
		eSettingFpgaSoftwareVersion = 72,		//!< FPGA Firmware version (Major * 100) + Minor (Notification Only)
		eSettingWiFiSoftwareVersion = 73,		//!< WiFi Firmware version (Major * 100) + Minor (Notification Only)
		eSettingW3SoftwareVersion = 74			//!< Wired Adaptor Software version (Major * 100) + Minor (Notification Only)

	};

	//! \enum eRadarMode_t
	//! Defines the current Radar operating mode.\n
	//! Radar can be commanded to Standby, Transmitting, TimedTx or PowerDown Modes.\n
	//! Sleep, Stalled and SelfTest Fail modes are status only values sent from the Scanner.\n
	//! From Sleep mode, the display can set the scanner to Standby Mode.\n
	//! From Stalled and Self Test Failed modes, the display must power cycle the scanner.
	//! The scanner can be power cycled by setting PowerDown waiting for the Sleep status and then selecting Standby Mode.
	//! \sa eSettingRadarMode
	enum eRadarMode_t
	{
		eRadarModeStandby = 0,				//!< Standby Mode
		eRadarModeTransmitting = 1,			//!< Transmitting Mode
		eRadarModePowerDown = 3,			//!< Powering Down Mode
		eRadarModeTimedTx = 4,				//!< Timed Transmit Mode
		eRadarModeSleep = 5,				//!< Radar has Powered down and is sleeping. (Notify Only). Set Standby mode to wake scanner from sllep mode
		eRadarModeStalled = 7,				//!< Scanner Rotation has stalled (Notify Only)
		eRadarModeSelfTestFailed = 10,		//!< Scanner has failed Self Test (Notify only)
		eRadarModeInvalid = -1
	};

	//! \enum eInterferenceRejectionMode_t
	//! Level of Intereference Rejection to be applied.\n
	//! Default level is 3. Deafult level for weather mode is 1.
	//! \sa eSettingInterferenceRejection
	enum eInterferenceRejectionMode_t
	{
		eInterferenceRejectionModeOff = 0,	//!< Inetrference Rejection is turned OFF
		eInterferenceRejectionMode1 = 1,	//!< Lowest level of Interference Rejection
		eInterferenceRejectionMode2 = 2,
		eInterferenceRejectionMode3 = 3,
		eInterferenceRejectionMode4 = 4,
		eInterferenceRejectionMode5 = 5,		//!< Highest level of Interference Rejection
		eInterferenceRejectionModeInvalid = -1
	};

	//! \struct TimedTransmit_t
	//! Parameters to specify the Standby and Transmit periods for Timed Transmit Mode.
	//! \sa eSettingTimedTransmit
	struct TimedTransmit_t
	{
		uint8_t StandbyTime_Mins;			//!< Min 3, Max 15 
		uint8_t TransmitCount_Scans;		//!< Min 10 Max 30
	};

	//! \struct TimedTransmitRemaining_t 
	//! (Notification only) 
	//! Parameters specify the amount of time remaining in Standby when using Timed Transmit Mode.
	//! \sa eSettingTimedTransmitRemaining
	struct TimedTransmitRemaining_t
	{
		uint8_t Mins;				//!< minutes remaining in standby mode
		uint8_t Secs;				//!< seconds remaining in standby mode (0-59)
	};

	//! \enum eTransmitFrequency_t
	//! Transmit Frequency can be altered to improve interfernce rejection.\n
	//! The transmit frequency can be set above or below the nominal 9.4GHz frequency.\n
	//! \sa eSettingTxFrequency
	enum eTransmitFrequency_t
	{
		eTransmitFrequencyNominal = 0,		//!< Nominal Frequency
		eTransmitFrequencyLow = 1,			//!< Below Nominal Frequency
		eTransmitFrequencyHigh = 2			//!< Above nominal Frequency
	};

	//! \enum eChannel_t
	//! The channel is used to select which data stream a parameter should be applied to when Dual Range data is available.\n
	//! If Dual Range data is not available the channel should be set to eChannelDefault.\n
	//! If a parameter applies to all channels eChannelIndependent should be selected.\n
	enum eChannel_t
	{
		eChannelDefault = 0,						//!< Default Channel, Channel 0
		eChannelSecond = 1,							//!< Second Channel, Channel 1
		eChannelIndependent = 0xff					//!< Used if parameter is applied to whole scanner, channel independent
	};

	//! \enum ePreset_t
	//! Each Preset Mode contains individual settings for Gain, Colour Gain, Sea and Rain modes and values.\n
	//! The user is able to set and save their preferred settings for each mode and simply switch between modes to restore these settings.
	//! \sa eSettingPresetMode
	enum ePreset_t
	{
		ePresetHarbour = 0,							//!< Apply Harbour Mode Settings
		ePresetCoastal = 1,							//!< Apply Coastal Mode Settings
		ePresetOffshore = 2,						//!< Apply Offshore Mode Settings
		ePresetWeather = 3							//!< Apply Weather Mode Settings
	};

	//! \enum eGainMode_t
	//! Set Gain control to Automatic or Manual Setting.
	//! \sa eSettingGainMode
	enum eGainMode_t
	{
		eGainModeManual = 0,		//!< Manual Control of Gain 0 -100%
		eGainModeAuto = 1			//! Auto control of Gain Setting
	};

	//! \enum eColourGainMode_t
	//! Set Colour Gain control to Automatic or Manual Setting.
	//! \sa eSettingColourGainMode
	enum eColourGainMode_t
	{
		eColourGainModeManual = 0, //!< Manual Control of Colour Gain 0 -100%
		eColourGainModeAuto = 1	   //! Auto control of Colour Gain setting
	};

	//! \enum eSeaMode_t
	//! Set Sea control to Manual or Automatic.\n
	//! In Offshore and Coastal Preset Modes Auto Sea Clutter will dynamically adjust the Sea Clutter level.\n
	//! to suit the current sea state and conditions.
	//! \sa eSettingSeaMode
	enum eSeaMode_t
	{
		eSeaModeManual = 0,		//!< Manual Control of Sea clutter 0 -100%
		eSeaModeAuto = 1		//! Auto control of sea clutter setting
	};

	//! \enum eRainMode_t
	//! Set Rain control OFF (equivalent to Manual 0%) or Manual Setting
	//! \sa eSettingRainMode
	enum eRainMode_t
	{
		eRainModeOff = 0,		//!< Rain Mode OFF
		eRainModeManual = 1		//!< Rain Mode Manual
	};

	//! \enum eTargetExpansion_t
	//! when target expansion is turned on a longer pulse wiidth is used to increase the size of returns from targets.
	//! \sa eSettingTargetExpansion
	enum eTargetExpansion_t
	{
		eTargetExpansionOff = 0,		//!< Target Expansion OFF
		eTargetExpansionOn = 1			//!< Target Expansion ON
	};

	//! \enum eSeaCurve_t
	//!
	//! When manual Sea Mode is selected, the shape of the Sea curve can be altered.\n
	//! Selecting R^5.5 produces a steeper sea curve which removes more targets close to the centre of the PPI.\n
	//! Swicthcing sea curves may be useful depending on the antenna height and/or sea consitions.
	//! \sa eSettingSeaClutterCurve
	enum eSeaCurve_t
	{
		eSeaCurve_R4 = 0,		//!< Use R^4 sea curve
		eSeaCurve_R5_5 = 1		//!< Use R^5.5 sea curve 
	};

	//! \enum eMainBang_t
	//!
	//! Main Bang Suppression is turned on by default 
	enum eMainBang_t
	{
		eMainBangOff = 0,	//!< Main Bang Suppression OFF
		eMainBangOn = 1		//!< Main Bang Suppression ON
	};

	//! \struct Zone_t
	//!
	//! This structure specifies the parameters for a Guard Zone or an Auto Acquire Zone.\n
	//! A zone is specified as an inner and outer range and a start and end bearing.\n
	//! The start and end bearing are defined in degrees from the Own Ships Heading in a clockwise direction 0 to 360 degrees.\n
	//! When setting a Guard Zone or Auto Acquire Zone the 'Enable' parameter is always set to 'true'.\n
	//! When setting a Guard Zone or Auto Acquire Zone the 'Id' parameter is defined by the value of eSetting_t. (eSettingGuardZone1, eSettingGuardZone2, eSettingAutoAccquireZone1, eSettingAutoAccquireZone2).
	//! Guard Zones and Auto Acquire Zones should be enabled and disabled using the GuardZoneEnable and AutoAcquireZoneEnable messages.\n
	//! \sa eSettingGuardZone1 
	//! \sa eSettingGuardZone2 
	//! \sa eSettingAutoAccquireZone1 
	//! \sa eSettingAutoAccquireZone2
	struct Zone_t
	{
		uint32_t StartRange_nm_x1000;		//!< Start Range (inner radius) in Nautical Miles x 1000 (e.g. 1.5nm = 1500)
		uint32_t EndRange_nm_x1000;			//!< End Range in (outer Radius) Nautical Miles x 1000 (e.g. 1.5nm = 1500)
		uint32_t StartAngle_degs_x10;		//!< Start Angle in degrees x 10 (e.g 359.4 degrees = 3954)
		uint32_t EndAngle_degs_x10;			//!< Start Angle in degrees x 10 (e.g 359.4 degrees = 3954)
		bool     Enable;					//!< Always set to 'true' when setting a Guard Zone or AutoAcquireZone. 
		uint8_t  Id;						//!< 0 or 1. Supports 2 Guard Zones and/or 2 Auto Acquire Zones
	};

	//! \struct ZoneState_t
	//! Used to enable/disable Guard Zones and AutoAcquire Zones.
	//! \sa eSettingGuardZoneEnable
	//! \sa eSettingAutoAcquireZoneEnable
	struct ZoneState_t
	{
        uint8_t  Id;
        bool     Enable;
	};

	//! \enum eDopplerMode_t
	//! Turn Doppler processing ON or OFF.\n
	//! When Doppler processing is ON, spoke data contains special values to identify approacing or receeding targets.\n
	//! A value of 255 in the spoke data indicates that this sample is an approaching target.\n
	//! A value of 254 in the spoke data indicates that this sample is an receeding target.
	//! \sa eSettingDopplerMode
	//! \sa SpokeData_t
	enum eDopplerMode_t
	{
		eDopplerOff = 0,
		eDopplerOn = 1
	};

	//! \enum eAutoAcquireMode_t
	//! Specify Auto Acquire Mode (This feature is currently not supported) 
	//! \sa eSettingAutoAcquireMode
	enum eAutoAcquireMode_t
	{
		eAutoAcquireOff = 0,						//!< Auto Acquire is turned off
		eAutoAcquireZone = 1,						//!< Auto Acquire Targets within the specified auto acquire zones (not currently supported)
		eAutoAcquireDoppler = 2						//!< Auto Acquire Targets that have approaching doppler speed (not currently supported)
	};

	//! \enum eDopplerActive_t
	//! Notification message to identify if Doppler processing is active.\n
	//! Doppler Mode can be suspended if no SOG information is available in the scanner or if the range is set to 12nm or higher (8nm or higher, with Target Expansion ON or in Weather mode).
	//! \sa eSettingDopplerActive
	enum eDopplerActive_t
	{
		eDopplerSuspended = 0,						//!< Doppler mode has been suspend
		eDopplerActive = 1							//!< Doppler mode is enabled and doppler approaching/receeding information is provided in spoke data
	};

	//! \enum eTargetState_t
	//! Indicates state of currently tracked target, Acquiring, Safe, Dangerous or Lost
	//! \sa MarpaData_t
	enum eTargetState_t
	{
		eAcquiring = 0,								//!< Initial state, remains in this state until target is validated (10  hits)
		eSafe = 1,									//!< Normal state for target tracking
		eDangerous = 2,								//!< Target Dangerous will be within safe zone within the time to safe zone period
		eLost = 3									//!< Target Lost
	};

	//! \struct MarpaData_t
	//! Structure defining information about a tracked target.\n
	//! Each tracked marpa target maintains one of these structures.\n
	//! SOG and COG are required to Maintain True data.\n
	//! If SOG and COG not available then TargetTrueDataValid is set to false.\n
	//! TrueTargetSped_mps and TrueTargetCourse_degs are only valid if SOG and COG are present.\n
	//! Notifications are sent to MarpaDataChanged callback function.
	//! \sa MarpaDataChanged
	//! \sa eTargetState_t
	struct MarpaData_t
	{
		uint32_t Id;								//!< Target Id 0-24
		bool Valid;									//!< Valid falg true/false
		bool AutoAcquired;							//!< Auto Acquired Target true/false
		eTargetState_t State;						//!< Target state, acquiring, safe, dangerous, lost
		float TargetRange_m;						//!< Target Range from own vessel in metres
		float TargetBearing_degs;					//!< Target Relative Bearing from own vessel in degrees
		float TargetTrueBearing_degs;				//!< Target True Bearing from own vessel in degrees - relative to true north
		float RelTargetSpeed_mps;					//!< Target speed in metres per second, relative to own vessel
		float RelTargetCourse_degs;					//!< Target course in degrees, relative to own vessel
		float TrueTargetSpeed_mps;					//!< Target true speed in metres per second, ground stabilised (SOG)
		float TrueTargetCourse_degs;				//!< Target true course in degrees, ground stabilised (COG)
		float ClosestPointOfApproach_m;				//!< Closest Point of approach in metres
		uint32_t TimeToClosetPoint_secs;			//!< Time to closest point of approach in seconds
		float HeadingAtLastUpdate_degs;				//!< Own vessel heading at time of last contact in degree
		bool TargetGoingTowardsClosetPoint;			//!< True if Closest point of approach has not yet been reached, false if relative target motion away from own vessel
		bool TargetTrueDataValid;					//!< Set true/false if SOG/COG available
	};

	//! \struct MarpaSetup_t
	//! Parameters for generating dangerous target alarms.\n
	//! A target is marked as dangerous for the following conditions.\n
	//! If Targets relative speed is towards own vessel and Target is currently in safe zone.\n
	//! If Targets relative speed is towards own vessel and CPA is within in the 'SafeZoneDistance' and TCPA is less than the 'TimeToSafeZone'.\n
	//! If Targets relative speed is towards own vessel and Target is within the 'SafeZoneDistance' after 'TimeToSafeZone' seconds. (Target will not its CPA until after TimeToSafeZone seconds)
	//! \sa eSettingMarpaSetup
	struct MarpaSetup_t
	{
		uint32_t SafeZoneDistance_metres;			//!< Safe Zone Distance in metres
		uint32_t TimeToSafeZone_secs;				//!< Time to Safe Zone in seconds
	};

	//! \struct MarpaDesignate_t
	//! Start Acquiring Target information at the specified range and bearing.\n
	//! Bearing is relative to True North 
	//! \sa eSettingMarpaDesignate
	struct MarpaDesignate_t
	{
		float Range_metres;							//!< Target Range im metres
		float Bearing_degs;							//!< Target Bearing in degrees
	};

	//! \enum eAlarmType_t
	//! Defines the source of the Alarm e.g Guard Zone, Marpa Target.\n
	//! This type is used for notification of alarms and acknowledgment of alarms.
	//! \sa AlarmData_t
	//! \sa AlarmAck_t
	//! \sa eSettingAlarmAck
	enum eAlarmType_t
	{
		eAlarmTypeAll = 0xFFFFFFFF,					//!< Used to Acknowledge all alarm types (0xFFFFFFFF)
		eAlarmTypeGuardZoneTargetEntering = 0,		//!< Target Entering Guard Zone (0)
		eAlarmTypeMarpaDangerousTarget = 1,			//!< Dangerous Marpa Target (1)
		eAlarmTypeMarpaLostTarget = 2,				//!< Lost Marpa Target (2)
		eAlarmTypeAutoAcquireListFull = 3,			//!< Attempting to Auto Acquire Target when list is full (3)
		eAlarmTypeUndefined = 0xFFFFFFFE
	};

	//! \struct AlarmData_t
	//! AlarmDataChanged function is called when the Alarm List changes.\n
	//! The Alarms List contains a list of these structures defining the current Alarms.
	//! \sa AlarmDataChanged
	//! \sa eAlarmType_t
	struct AlarmData_t
	{
		uint32_t		Id;
		eAlarmType_t	Type;
		uint32_t		Value;
	};

	//! \struct AlarmAck_t
	//! Speicfy the alarm Id and/or type to cancel.\n
	//! Alarms may be cancelled individually, by alarms type or all alarms.\n
	//! For Example - \n
	//! Using the Id and Type from the alarm list, cancels an individual alarm.\n
	//! Setting Id = 0xFFFFFFFF and Type = eAlarmTypeMarpaDangerousTarget cancels all dangerous Target alarms.\n
	//! Setting Id = 0xFFFFFFFF and Type = eAlarmTypeAll cancels all alarms.\n
	//! \sa eAlarmType_t
	//! \sa eSettingAlarmAck
	struct AlarmAck_t
	{
		uint32_t		Id;			//!< Id of the alarm, incrementing sequence number
		eAlarmType_t	Type;		//!< Alarm Type
	};

	//! \struct NavDataItem_t
	//! Generic structure for Navigation Data.\n
	//! Heading, SOG, COG
	//! \sa NavData_t
	struct NavDataItem_t
	{
		bool Valid;			//!< true/false if parameter is valid
		float Value;		//!< speed, bearing ...
	};

	//! \struct NavData_t
	//! Structure sent to scanner containing navigation data.\n
	//! Scanner requires heading to track Marpa Target.\n
	//! Scanner requires SOG/COG to calculate True Heading/Course vectors for Marpa Targets.\n
	//! Scanner requires SOG to highlight moving targets based on their doppler returns.
	//! \sa eSettingNavData
	struct NavData_t
	{
		NavDataItem_t Heading_degs;
		NavDataItem_t Cog_degs;
		NavDataItem_t Sog_mps;
	};

	//! \enum eErrorCode_t
	//! Return codes from Quantum Library
	//! eNoError indicates success
	//! neagative values describe the error 
	enum eErrorCode_t
	{
		eNoError = 0,								//!< 0 - No Error, success
		eErrorGeneralFailure = -1,					//!< -1 OS Failure, Failed to create Mutex, Thread, Event or Semaphore 
		eErrorOutOfMemory = -2,						//!< -2 Out of memory, failed to instantiate class

		eErrorInvalidScannerIndex = -10,			//!< -10 Invalid Scanner Index supplied. Can't find the scanner is current list of scanners
		eErrorInvalidSerialNumber = -11,			//!< -11 Invalid Serial Number supplied. Can't find the scanner is current list of scanners

		eErrorFeatureNotSupported = -20,			//!< -20 Feature requested, in command to scanner, is not avialble in this scanner.
		eErrorInvalidValue = -21,					//!< -21 Invalid value supplied in command to scanner 
		eErrorInvalidDataSize = -22,				//!< -22 Invalid size of data supplied in command to scanner 
		eErrorInvalidSetting = -23,					//!< -23 Invalid setting supplied in command to scanner 
		eErrorInvalidNumberOfParameters = -24,		//!< -24 Invalid Number of Parameters supplied in scanner command

		eErrorEthernetTxMessageLength = -30,		//!< -30 Message length is greater than ethernet packet size
		eErrorEthernetInterfaceFailure = -31,		//!< -31 General failure of ethernet interface
		eErrorEthernetFailedToGetIpaddress = -32,	//!< -32 Failed to get Ip address, port may not be configured for 10.x.x.x network
		eErrorEthernetFailedToCreateSysSockets = -33,	//!< -33 Failed to create system sockets

		eErrorInvalidNotifyFunction = -100,			//!< -100 supplied notify handler is NULL
		eErrorNotifyAlreadyRegistered = -101,		//!< -101 Notify handler already installed
	};


	//! \struct SettingData_t 
	//! The SettingData_t structure is used as the interface between the Application and Quantum Library.\n
	//! It is used to pass settings to the Quantum Radar and to receive notifications of parameter changes from the Quantum Radar.\n
	//! \n
	//! To change a value in the scanner, a 'SettingData_t' structure should be created by calling the constructor with the scanner serial number and the enumerated setting.
	//! The 'SetValue()' function can then be called with the appropriate enumerated type or structure.
	//! A call to 'NewSacnnerSetting()' should be made with a reference to the 'SettingData_t' structure to generate the appropriate message to the scanner. 
	//! If the message sent to the scanner causes the value to be changed, the appropriate Notification message will be received.\n
	//! \n
	//! The Notification callback function, SettingChanged(), passes a reference to a 'SettingData_t' structure to the application. 
	//! The 'GetSerialNumber()' and 'GetSetting()' methods should be used to read the SerialNumber and Setting.
	//! Once the setting has been read, the 'GetValue()' method can be called with a reference to the appropriate enumerated type to read the new value from the structure.\n
	//! \sa eSettings_t
	struct SettingData_t
	{
	public:
		//! Constructor, Initialises the SerialNumber field to the supplied values.\n
		//! \n
		//! Default value for Channel can be used for all single range systems.\n
		//! The channel only needs to be set for dual range systems.
		//! \param serialNumber Defines the scanner that this Setting is applied too.
		SettingData_t(uint32_t serialNumber) :
			SerialNumber(serialNumber),
			Setting(eSettingUndefined),
			Channel(eChannelIndependent),
			Valid(false)
		{
			memset(&Value, 0, sizeof(Value));
		}

		//! Constructor, Initialises the Setting and SerialNumber fields to the supplied values.\n
		//! Sets the Valid flag to false. The Valid flag is set true after a call to SetValue() with a valid value.\n
		//! \param setting setting that this structure is being created for.
		//! \param serialNumber Defines the scanner that this Setting is applied too.
		//! \sa eSettings_t
		SettingData_t(eSettings_t setting, uint32_t serialNumber) :
			SerialNumber(serialNumber),
			Channel(eChannelIndependent),
			Valid(false)
		{
			_SetSetting(setting);
			memset(&Value, 0, sizeof(Value));
		}

		//! \fn GetSerialNumber
		//! \return uint32_t Serial Number of scanner
		uint32_t GetSerialNumber()
		{
			return SerialNumber;
		}

		//! \fn GetSetting
		//! \return eSetting_t Setting
		//! \sa eSetting_t
		eSettings_t GetSetting()
		{
			return Setting;
		}

		//! \fn GetChannel
		//! \sa eChannel_t
		eChannel_t GetChannel()
		{
			return Channel;
		}

		//! \fn IsValid
		//! Verifies that the setting and value have been set and match 
		//! \return true Structure is valid
		//! \return bool true/false false if SetValue() has not been called with a parameter that matches the current setting
		bool IsValid()
		{
			return Valid;
		}

		//! Set Radar Mode
		void SetValue(eSettings_t setting, eRadarMode_t RadarMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingRadarMode)
			{
				Value.RadarMode = RadarMode;
				Valid = true;
			}
		}

		//! Get Radar Mode
		void GetValue(eRadarMode_t& RadarMode)
		{
			Valid = false;
			if (Setting == eSettingRadarMode)
			{
				RadarMode = Value.RadarMode;
				Valid = true;
			}
		}

		//! Set Interference Rejection
		void SetValue(eSettings_t setting, eInterferenceRejectionMode_t InterferenceRejection)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingInterferenceRejection)
			{
				Value.InterferenceRejection = InterferenceRejection;
				Valid = true;
			}
		}

		//! Get Interference Rejection
		void GetValue(eInterferenceRejectionMode_t& InterferenceRejection)
		{
			Valid = false;
			if (Setting == eSettingInterferenceRejection)
			{
				InterferenceRejection = Value.InterferenceRejection;
				Valid = true;
			}
		}

		//! Set unsigned 8 bit value
		//! Range, Gain, Colour Gain, Sea, Rain and GuardZone sensitivity
		void SetValue(eSettings_t setting, uint8_t u8)
		{
			Valid = false;
			_SetSetting(setting);
			switch (Setting)
			{
				case eSettingRange:
				case eSettingGainValue:
				case eSettingColourGainValue:
				case eSettingSeaValue:
				case eSettingRainValue:
				case eSettingGuardZoneSensitivity:
					Value.u8 = u8;
					Valid = true;
					break;
				default:
					break;
			}
		}

		//! Get unsigned 8 bit value
		//! Range, Gain, Colour Gain, Sea, Rain and GuardZone sensitivity
		void GetValue(uint8_t& u8)
		{
			Valid = false;
			switch (Setting)
			{
				case eSettingRange:
				case eSettingGainValue:
				case eSettingColourGainValue:
				case eSettingSeaValue:
				case eSettingRainValue:
				case eSettingGuardZoneSensitivity:
					u8 = Value.u8;
					Valid = true;
					break;
				default:
					break;
			}
		}

		//! Set unsigned 32 bit value
		//! Software Versions and Marpa Target Id
		void SetValue(eSettings_t setting, uint32_t u32)
		{
			Valid = false;
			_SetSetting(setting);
			switch (Setting)
			{
			case eSettingMarpaClear:
			case eSettingMainSoftwareVersion:
			case eSettingPsuSoftwareVersion:
			case eSettingFpgaSoftwareVersion:
			case eSettingWiFiSoftwareVersion:
			case eSettingW3SoftwareVersion:
				Value.u32 = u32;
				Valid = true;
				break;
			default:
				break;
			}
		}

		//! Set unsigned 32 bit value
		//! Software Versions and Marpa Target Id
		void GetValue(uint32_t& u32)
		{
			Valid = false;
			switch (Setting)
			{
				case eSettingMarpaClear:
				case eSettingMainSoftwareVersion:
				case eSettingPsuSoftwareVersion:
				case eSettingFpgaSoftwareVersion:
				case eSettingWiFiSoftwareVersion:
				case eSettingW3SoftwareVersion:
					u32 = Value.u32;
					Valid = true;
					break;
				default:
					break;
			}
		}


		//! Set signed 16 bit value
		//! Bearing Alignment
		void SetValue(eSettings_t setting, int16_t i16)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingBearingAlignment)
			{
				Value.i16 = i16;
				Valid = true;
			}
		}

		//! Get signed 16 bit value
		//! Bearing Alignment
		void GetValue(int16_t& i16)
		{
			Valid = false;
			if (Setting == eSettingBearingAlignment)
			{
				i16 = Value.i16;
				Valid = true;
			}
		}


		//! Set Zone
		//! Guard Zones and AutoAcquire Zones
		void SetValue(eSettings_t setting, Zone_t& Zone)
		{
			Valid = false;
			_SetSetting(setting);
			switch (Setting)
			{
			case eSettingGuardZone1:
			case eSettingGuardZone2:
			case eSettingAutoAccquireZone1:
			case eSettingAutoAccquireZone2:
				memcpy(&Value.Zone, &Zone, sizeof(Value.Zone));
				Valid = true;
				break;
			default:
				break;
			}
		}

		//! Get Zone
		//! Guard Zones and AutoAcquire Zones
		void GetValue(Zone_t& Zone)
		{
			Valid = false;
			if ((Setting == eSettingGuardZone1) || (Setting == eSettingGuardZone2) ||
				(Setting == eSettingAutoAccquireZone1) || (Setting == eSettingAutoAccquireZone2))
			{
				memcpy(&Zone, &Value.Zone, sizeof(Zone));
				Valid = true;
			}
		}


		//! Set Zone Enable/disable State
		//! Guard Zones and AutoAcquire Zones
		void SetValue(eSettings_t setting, ZoneState_t ZoneState)
		{
			Valid = false;
			_SetSetting(setting);
			switch (Setting)
			{
			case eSettingGuardZoneEnable:
			case eSettingAutoAcquireZoneEnable:
				Value.ZoneState = ZoneState;
				Valid = true;
				break;
			default:
				break;
			}
		}

		//! Get Zone Enable/disable State
		//! Guard Zones and AutoAcquire Zones
		void GetValue(ZoneState_t& ZoneState)
		{
			Valid = false;
			switch (Setting)
			{
			case eSettingGuardZoneEnable:
			case eSettingAutoAcquireZoneEnable:
				ZoneState = Value.ZoneState;
				Valid = true;
				break;
			default:
				break;
			}
		}

		//! Set Timed Transmit parameters
		void SetValue(eSettings_t setting, TimedTransmit_t& TimedTransmit)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingTimedTransmit)
			{
				Value.TimedTransmit.StandbyTime_Mins = TimedTransmit.StandbyTime_Mins;
				Value.TimedTransmit.TransmitCount_Scans = TimedTransmit.TransmitCount_Scans;
				Valid = true;
			}
		}
		//! Get Timed Transmit parameters
		void GetValue(TimedTransmit_t& TimedTransmit)
		{
			Valid = false;
			if (Setting == eSettingTimedTransmit)
			{
				TimedTransmit.StandbyTime_Mins = Value.TimedTransmit.StandbyTime_Mins;
				TimedTransmit.TransmitCount_Scans = Value.TimedTransmit.TransmitCount_Scans;
				Valid = true;
			}
		}

		//! Set Transmit frequency
		void SetValue(eSettings_t setting, eTransmitFrequency_t TransmitFrequency)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingTxFrequency)
			{
				Value.TransmitFrequency = TransmitFrequency;
				Valid = true;
			}
		}
		//! Get Transmit frequency
		void GetValue(eTransmitFrequency_t& TransmitFrequency)
		{
			Valid = false;
			if (Setting == eSettingTxFrequency)
			{
				TransmitFrequency = Value.TransmitFrequency;
				Valid = true;
			}
		}

		//! Set Auto Acquire Mode
		void SetValue(eSettings_t setting, eAutoAcquireMode_t AutoAcquireMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingAutoAcquireMode)
			{
				Value.AutoAcquireMode = AutoAcquireMode;
				Valid = true;
			}
		}
		//! Get Auto Acquire Mode
		void GetValue(eAutoAcquireMode_t& AutoAcquireMode)
		{
			Valid = false;
			if (Setting == eSettingAutoAcquireMode)
			{
				AutoAcquireMode = Value.AutoAcquireMode;
				Valid = true;
			}
		}

		//! Set Custom Ranges
		void SetValue(eSettings_t setting, CustomRanges_t& CustomRanges)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingCustomRanges)
			{
				memcpy(&Value.CustomRanges, &CustomRanges, sizeof(CustomRanges_t));
				Valid = true;
			}
		}

		//! Get Custom Ranges
		void GetValue(CustomRanges_t& CustomRanges)
		{
			Valid = false;
			if (Setting == eSettingCustomRanges)
			{
				memcpy(&CustomRanges, &Value.CustomRanges, sizeof(CustomRanges_t));
				Valid = true;
			}
		}

		//! Set Preset Mode
		void SetValue(eSettings_t setting, ePreset_t PresetMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingPresetMode)
			{
				Value.PresetMode = PresetMode;
				Valid = true;
			}
		}
		//! Get Preset Mode
		void GetValue(ePreset_t& PresetMode)
		{
			Valid = false;
			if (Setting == eSettingPresetMode)
			{
				PresetMode = Value.PresetMode;
				Valid = true;
			}
		}

		//! Set Gain Mode
		void SetValue(eSettings_t setting, eGainMode_t GainMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingGainMode)
			{
				Value.GainMode = GainMode;
				Valid = true;
			}
		}
		//! Get Gain Mode
		void GetValue(eGainMode_t& GainMode)
		{
			Valid = false;
			if (Setting == eSettingGainMode)
			{
				GainMode = Value.GainMode;
				Valid = true;
			}
		}

		//! Set Colour Gain Mode
		void SetValue(eSettings_t setting, eColourGainMode_t ColourGainMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingColourGainMode)
			{
				Value.ColourGainMode = ColourGainMode;
				Valid = true;
			}
		}

		//! Get Colour Gain Mode
		void GetValue(eColourGainMode_t& ColourGainMode)
		{
			Valid = false;
			if (Setting == eSettingColourGainMode)
			{
				ColourGainMode = Value.ColourGainMode;
				Valid = true;
			}
		}

		//! Set Sea Mode
		void SetValue(eSettings_t setting, eSeaMode_t SeaMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingSeaMode)
			{
				Value.SeaMode = SeaMode;
				Valid = true;
			}
		}
		//! Get Sea Mode
		void GetValue(eSeaMode_t& SeaMode)
		{
			Valid = false;
			if (Setting == eSettingSeaMode)
			{
				SeaMode = Value.SeaMode;
				Valid = true;
			}
		}

		//! Set Rain Mode
		void SetValue(eSettings_t setting, eRainMode_t RainMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingRainMode)
			{
				Value.RainMode = RainMode;
				Valid = true;
			}
		}
		//! Get Rain Mode
		void GetValue(eRainMode_t& RainMode)
		{
			Valid = false;
			if (Setting == eSettingRainMode)
			{
				RainMode = Value.RainMode;
				Valid = true;
			}
		}

		//! Set TargetExpansion
		void SetValue(eSettings_t setting, eTargetExpansion_t TargetExpansion)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingTargetExpansion)
			{
				Value.TargetExpansionState = TargetExpansion;
				Valid = true;
			}
		}
		//! Get TargetExpansion
		void GetValue(eTargetExpansion_t& TargetExpansion)
		{
			Valid = false;
			if (Setting == eSettingTargetExpansion)
			{
				TargetExpansion = Value.TargetExpansionState;
				Valid = true;
			}
		}

		//! Set Sea Curve
		void SetValue(eSettings_t setting, eSeaCurve_t SeaCurve)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingSeaClutterCurve)
			{
				Value.SeaCurve = SeaCurve;
				Valid = true;
			}
		}
		//! Get Sea Curve
		void GetValue(eSeaCurve_t& SeaCurve)
		{
			Valid = false;
			if (Setting == eSettingSeaClutterCurve)
			{
				SeaCurve = Value.SeaCurve;
				Valid = true;
			}
		}

		//! Set Main Bang Suppression on/off
		void SetValue(eSettings_t setting, eMainBang_t MainBang)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingMainBang)
			{
				Value.MainBangState = MainBang;
				Valid = true;
			}
		}
		//! Get Main Bang Suppression on/off
		void GetValue(eMainBang_t& MainBang)
		{
			Valid = false;
			if (Setting == eSettingMainBang)
			{
				MainBang = Value.MainBangState;
				Valid = true;
			}
		}

		//! SetDopplerMode
		void SetValue(eSettings_t setting, eDopplerMode_t DopplerMode)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingDopplerMode)
			{
				Value.DopplerMode = DopplerMode;
				Valid = true;
			}
		}
		//! GetDopplerMode
		void GetValue(eDopplerMode_t& DopplerMode)
		{
			Valid = false;
			if (Setting == eSettingDopplerMode)
			{
				DopplerMode = Value.DopplerMode;
				Valid = true;
			}
		}

		//! SetDopplerActive
		void SetValue(eSettings_t setting, eDopplerActive_t DopplerActive)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingDopplerActive)
			{
				Value.DopplerActive = DopplerActive;
				Valid = true;
			}
		}
		//! GetDopplerActive
		void GetValue(eDopplerActive_t& DopplerActive)
		{
			Valid = false;
			if (Setting == eSettingDopplerActive)
			{
				DopplerActive = Value.DopplerActive;
				Valid = true;
			}
		}

		//! Set Timed Transmit Remaining Parameters
		void SetValue(eSettings_t setting, TimedTransmitRemaining_t& TimedTransmitRemaining)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingTimedTransmitRemaining)
			{
				Value.TimedTransmitRemaining.Mins = TimedTransmitRemaining.Mins;
				Value.TimedTransmitRemaining.Secs = TimedTransmitRemaining.Secs;
				Valid = true;
			}
		}
		//! Get Timed Transmit Remaining Parameters
		void GetValue(TimedTransmitRemaining_t& TimedTransmitRemaining)
		{
			Valid = false;
			if (Setting == eSettingTimedTransmitRemaining)
			{
				TimedTransmitRemaining.Mins = Value.TimedTransmitRemaining.Mins;
				TimedTransmitRemaining.Secs = Value.TimedTransmitRemaining.Secs;
				Valid = true;
			}
		}

		//! Set Nav Data Values
		void SetValue(eSettings_t setting, NavData_t& NavData)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingNavData)
			{
				memcpy(&Value.NavData, &NavData, sizeof(NavData_t));
				Valid = true;
			}
		}
		//! Get Nav Data Values
		void GetValue(NavData_t& NavData)
		{
			Valid = false;
			if (Setting == eSettingNavData)
			{
				memcpy(&NavData, &Value.NavData, sizeof(NavData));
				Valid = true;
			}
		}

		//! Set Marpa Setup parameters
		void SetValue(eSettings_t setting, MarpaSetup_t& MarpaSetup)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingMarpaSetup)
			{
				Value.MarpaSetup.SafeZoneDistance_metres = MarpaSetup.SafeZoneDistance_metres;
				Value.MarpaSetup.TimeToSafeZone_secs = MarpaSetup.TimeToSafeZone_secs;
				Valid = true;
			}
		}

		//! Get Marpa Setup parameters
		void GetValue(MarpaSetup_t& MarpaSetup)
		{
			Valid = false;
			if (Setting == eSettingMarpaSetup)
			{
				MarpaSetup.SafeZoneDistance_metres = Value.MarpaSetup.SafeZoneDistance_metres;
				MarpaSetup.TimeToSafeZone_secs = Value.MarpaSetup.TimeToSafeZone_secs;
				Valid = true;
			}
		}

		//! Set Marpa Designate Target parameters
		void SetValue(eSettings_t setting, MarpaDesignate_t& MarpaDesignate)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingMarpaDesignate)
			{
				Value.MarpaDesignate.Bearing_degs = MarpaDesignate.Bearing_degs;
				Value.MarpaDesignate.Range_metres = MarpaDesignate.Range_metres;
				Valid = true;
			}
		}
		//! Get Marpa Designate Target parameters
		void GetValue(MarpaDesignate_t& MarpaDesignate)
		{
			Valid = false;
			if (Setting == eSettingMarpaDesignate)
			{
				MarpaDesignate.Bearing_degs = Value.MarpaDesignate.Bearing_degs;
				MarpaDesignate.Range_metres = Value.MarpaDesignate.Range_metres;
				Valid = true;
			}
		}

		//! Set Alarm Acknowledge Parameters
		void SetValue(eSettings_t setting, AlarmAck_t& AlarmAck)
		{
			Valid = false;
			_SetSetting(setting);
			if (Setting == eSettingAlarmAck)
			{
				Value.AlarmAck.Id = AlarmAck.Id;
				Value.AlarmAck.Type = AlarmAck.Type;
				Valid = true;
			}
		}

		//! Get Alarm Acknowledge Parameters
		void GetValue(AlarmAck_t& AlarmAck)
		{
			Valid = false;
			if (Setting == eSettingAlarmAck)
			{
				AlarmAck.Id = Value.AlarmAck.Id;
				AlarmAck.Type = Value.AlarmAck.Type;
				Valid = true;
			}
		}

	private:
		uint32_t	SerialNumber;								//!< serial number of the scanner that this structure is linked too.
		eSettings_t Setting;									//!< enumerated value defining the Setting 
		eChannel_t	Channel;									//!< Channel that this parameter applies to. Only relevent in dual channel systems.
		bool		Valid;
		union Value_t
		{
			uint8_t							u8;
			uint32_t						u32;
			int16_t							i16;
			Zone_t							Zone;
			ZoneState_t						ZoneState;
			eRadarMode_t					RadarMode;					//!< use when 'Setting' = eSettingRadarMode 
			eInterferenceRejectionMode_t	InterferenceRejection;      //!< use when 'Setting' = eSettingInterferenceRejection
			TimedTransmit_t					TimedTransmit;				//!< use when 'Setting' = eSettingTimedTransmit
			eTransmitFrequency_t			TransmitFrequency;			//!< use when 'Setting' = eSettingTxFrequency
			eAutoAcquireMode_t				AutoAcquireMode;			//!< use when 'Setting' = eSettingAutoAcquireMode
			CustomRanges_t					CustomRanges;				//!< use when 'Setting' = eSettingCustomRanges
			ePreset_t						PresetMode;					//!< use when 'Setting' = eSettingPresetMode
			eGainMode_t						GainMode;					//!< use when 'Setting' = eSettingGainMode
			eColourGainMode_t				ColourGainMode;				//!< use when 'Setting' = eSettingColourGainMode
			eSeaMode_t						SeaMode;					//!< use when 'Setting' = eSettingSeaMode
			eRainMode_t						RainMode;					//!< use when 'Setting' = eSettingRainMode
			eTargetExpansion_t				TargetExpansionState;		//!< use when 'Setting' = eSettingTargetExpansion
			eSeaCurve_t						SeaCurve;					//!< use when 'Setting' = eSettingSeaClutterCurve
			eMainBang_t						MainBangState;				//!< use when 'Setting' = eSettingMainBang
			eDopplerMode_t					DopplerMode;				//!< use when 'Setting' = eSettingDopplerMode
			eDopplerActive_t				DopplerActive;				//!< use when 'Setting' = eSettingDopplerActive
			TimedTransmitRemaining_t		TimedTransmitRemaining;		//!< use when 'Setting' = eSettingTimedTransmitRemaining
			NavData_t						NavData;					//!< use when 'Setting' = eSettingNavData
			MarpaSetup_t					MarpaSetup;					//!< use when 'Setting' = eSettingMarpaSetup
			MarpaDesignate_t				MarpaDesignate;				//!< use when 'Setting' = eSettingMarpaDesignate
			AlarmAck_t						AlarmAck;					//!< use when 'Setting' = eSettingAlarmAck
		};
		Value_t Value;													//!< Contains the value relating to the Setting parameter. Union of data values for all settings. 

		void _SetSetting(eSettings_t setting)
		{
			Setting = setting;
			switch (setting)
			{
			case eSettingRange:
			case eSettingPresetMode:
			case eSettingGainMode:
			case eSettingGainValue:
			case eSettingColourGainMode:
			case eSettingColourGainValue:
			case eSettingSeaMode:
			case eSettingSeaValue:
			case eSettingRainMode:
			case eSettingRainValue:
			case eSettingTargetExpansion:
			case eSettingSeaClutterCurve:
			case eSettingMainBang:
			case eSettingDopplerMode:
			case eSettingDopplerActive:
				Channel = eChannelDefault;
				break;
			default:
				Channel = eChannelIndependent;
				break;
			}
		}
	};
}
