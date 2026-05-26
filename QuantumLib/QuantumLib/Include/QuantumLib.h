//! \file QuantumLib.h
//! API for Quantum Library

//! \mainpage Introduction to the Quantum SDK and API
//! The Quantum SDK is a library of functions providing an interface between a Client Application and the Quantum Radar.\n 
//! The API provides callback functions giving the current status of the Radar, Spoke Data, Marpa Target Data and Alarm information.\n
//! The API also allow messages to be sent to the Quantum Radar to control the Radars Parameters and Settings.\n 
//! The SDK provides example programs that are compiled and ready to run.\n
//! The example programs have been built using Visual Studio 2017 and the solution files and source code are supplied with the SDK.\n
//! The example source code may be re-used and/or modified for third party applications.\n
//! \section Copyright Copyright Notice
//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n
//! \section Network Network Settings
//! \subsection Address IP Address
//! The Quantum Radar works with an IP address in the range 10.0.x.x to 10.31.x.x with a subnet mask opf 255.224.0.0 \n
//! \subsection Server DHCP Server
//! The IP address is assigned to the Quantum Radar by a DHCP server, the host system must therefore run a DHCP server.\n
//! \section Protocol Quantum Protocol
//! The protocol between the host system and the Quantum Radar allows for multiple displays to control and display data from the Radar.\n
//! When a host sends a command to the Radar to change a parameter, it must receive the appropriate notification message to validate that the parameter has been set correctly. \n
//! This validation process will ensure the integrity of settings between the Quantum Radar and all connected host systems.\n
//! \page page1 Example Programs
//! \section build Building the examples
//! There are three example programs supplied with the Quantum SDK.\n
//! The applications have been built using Visual Studio 2017 for win32 and x64.\n
//! QuantumClient - windows console based application showing notification messages from the scanner and allowing user modification of settings from a command line interface.\n
//! QuantumClientDialog - windows application showing notification messages from the scanner, alarm messages, marpa data and allowing user modification of settings from a command line interface.\n
//! QuantumClientPPI - windows based application that can display a PPI image using DirectX 9.\n
//! \n
//! 32 bit versions of the Compiled library files and compiled versions of the example programs are installed at : -\n
//! {InstallDir}/Ouput/Debug_Win32\n
//! {InstallDir}/Ouput/Release_Win32\n
//! \n
//! 64 bit versions of the Compiled library files and compiled versions of the example programs are installed at : -\n
//! {InstallDir}/Ouput/Debug_x64\n
//! {InstallDir}/Ouput/Release_x64\n
//! \n
//! NOTE:\n
//! Installation of the x86 version of the Microsoft VC redistribution libraries may be required to run the Win32 example programs.\n
//! Installation of the x64 version of the Microsoft VC redistribution libraries may be required to run the x64 example programs.\n
//! Installation of the DirectX redistribution libraries may be required to run the Win32 PPI example programs.\n
//! There is no 64 bit version of the PPI example program.\n
//! \subsection cli Command Line 
//! The command line interpreter is a basic command line allowing the user to set radar parameters\n
//! \subsection syntax Syntax
//! <ScannerIndex>, <Setting>, <param1> [,param2, .... ,<paramn>]\n
//! All values entered are numerical where \n
//! <ScannerIndex> is the index of the scanner that is shown in the Scanner List window\n
//! <Setting> is the enumerted value of the setting from e_Setting_t\n
//! <param1 .. n> value(s) to sent with the command\n
//! \subsection examples Examples
//! 0,1,1           ; Set Radar Mode to 1 (transmitting)\n
//! 0,1,0           ; Set Radar Mode to 0 (standby)\n
//! 0,20,5          ; set range index 5 (1/2 nm range)\n
//! 0,22,0          ; set Gain Mode Manual\n
//! 0,23,90         ; Set Gain value 90%\n
//! 0,62,750,175    ; acquire marpa target at 750 metres, bearing 175 degrees\n
//! \page page2 Building a Client Application
//! The Quantum Library is wrapped inside the QuantumLib namespace which includes an QuantumLib::INotify and QuantumLib::API classes.\n
//! The Client Application class should be derived from the base class QuantumLib::INotify.\n
//! The Application should first call QuantumLib::API::Open() to initialise the DLL and Ethernet Interface.\n
//! The Application should then call QuantumLib::API::RegisterForScannerNotifications() to receive callbacks from the library.\n
//! The methods defined in QuantumLib::INotify are virtual functions with an empty function body defined. Therefore, these methods may be overrideen by the Client application , but do not need to be implemented.\n
//! When the application and scanner are first connected, the QuantumLib::INotify::SettingChanged() method will be called for each Setting in the Radar, giving the Client Appplication a full list of the current scanner settings.\n
//! The Client Application can alter the Scanner Settings by making a call to QuantumLib::API::NewScannerSetting(). Each change of scanner setting will generate a callback to QuantumLib::INotify::SettingChanged() method.\n
//! Some settings may cause multiple callbacks to be generated e.g. changing the range index.\n
//! \page page3 Settings Data
//! \section SettingData SettingData_t
//! \subsection Control Using SettingData_t to control the scanner
//! To send a command to the scanner, the SettingData_t structure should be created by calling the SettingData_t constructor with the setting value and the serial number of the scanner.\n
//! The SetValue() method of the SettingData_t structure should then be called with the appropriate type.The SetValue() method contains a number of overloaded functions and verifies the parameter type against the setting supplied in the constructor.\n
//! The IsValid() method of the SettingData_t structure will return false if the Setting and Value do not match.\n
//! For Example, to set the Radar(with SerialNumber 0x12345678) to Transmit Mode\n
//! SettingData_t Setting(eRadarMode, 0x12345678);\n
//! Setting.SetValue(eRadarModeTransmitting);\n
//! If(Setting.IsValid() == true)\n
//! {\n
//! 	bool ret = QuantumLib::API::NewScannerSetting(Setting);\n
//! }\n
//! \subsection Notify Using SettingData_t to receive notifications from the scanner
//! The SettingData_t structure is by the application program to receive setting change notification messages from the library.\n
//! The GetSetting() method of the SettingData_t structure can be used to determine the setting.\n
//! The GetValue() method of the SettingData_t structure should then be called with the appropriate type to read the new setting value.The GetValue() method contains a number of overloaded functions and verifies the parameter type against the setting.\n
//! The IsValid() method of the SettingData_t structure will return false if the GetValue() method has been called with a type that does not match the setting.\n
//! For example\n
//! void QuantumApp::SettingChanged(QuantumLib::SettingData_t& rSetting)\n
//! {\n
//! If(rSetting.GetSetting() == QuantumLib::eSettingRadarMode)\n
//! {\n
//! 		QuantumLib::eRadarMode_t RadarMode;\n
//! 		rSetting.GetValue(RadarMode);\n
//! 		If(rSetting.IsValid() == true)\n
//! 		{\n
//! 			// update application with new RadarMode value\n
//! 		}\n
//!			else\n
//!			{\n
//! 			// log error\n
//!			}\n
//! 	}\n
//! }\n
//! \page page4 Debugging the Quantum Library
//! \section Debug Debug Options
//! \subsection File Debug Configuration File
//! The Debug information can be configured from a configuration file QuantumLibDebug.cfg which should be placed in the directory with the QuantumLib.DLL and the application executable file.\n
//! The configuration file allows output to be directed to the console or a file.\n
//! The level of debug output may also be specified.\n
//! Example QuantumLibDebug.cfg File.\n
//! Filename: QDebug.Txt\n
//!	Output: 3\n
//!	Level: 2\n
//!	\subsection Output Debug Output
//!	Debug output can be directed to one of three possible output devices.\n
//!	0 - No Output\n
//!	1 - Output to console window in a Win32 console application\n
//!	2 - Output to console tab in visual studio\n
//!	3 - Output to file\n
//!	\subsection Level Debug Level
//!	Debug output can be configured to provide different levels of configuration information. 6 levels of debug output are provided, Level 1 giving the most concise debug information and level 6 the most verbose.\n
//!	0 - No debug messages are output.\n
//!	1 - Failure and Error messages, can't create class, sockets ...\n
//!	2 - Ethernet Messages, creating threads, opening sockets, binding ports ...\n
//!	3 - Control parameter messages sent to radar\n
//!	4 - Marpa and Alarm Notification messages\n
//!	5 - Received System Packets Unit Info, Service Info\n
//!	6 - Received Radar Packets, logs the serial number and message Id for all messages received from the Radar\n

#pragma once

//! \def QUANTUMLIB_API 
//! Declares an interface function for the DLL. 
#ifdef QUANTUMLIB_EXPORTS
#define QUANTUMLIB_API __declspec(dllexport)
#else
#define QUANTUMLIB_API __declspec(dllimport)
#endif

#include "QuantumLibTypes.h"
#include <stdint.h>

namespace QuantumLib
{
	//! \class INotify
	//! \brief Base class for notification messages received from the Radar.\n
	//! \details The client application should implement an instance of the INotify class to receive notification messages.
	//! ScannerListChanged is implemented as a pure virtual function and must be implemented by the application to determine the number (if any) of Scanners connected.
	//! Other methods in the INotify class are virtual functions with an empty default implementation so that the application layer only needs to implement the functions of interest.
	class INotify
	{
	public:
		INotify() {}
		virtual ~INotify() {}

		//! \fn ScannerListChanged
		//! Called whenever a scanner is added or removed from the system.
		//! \param count number of scanners detecetd in the system
		virtual void ScannerListChanged(uint32_t count) = 0;

		//! \fn SpokeDataReceived
		//! Called for each spoke of data received.
		//! \param SerialNumber Defines the scanner that Spoke Data was received from.
		//! \param rSpokeData Reference to the SpokeData structure
		//! \sa SpokeData_t
		virtual void SpokeDataReceived(uint32_t SerialNumber, SpokeData_t& rSpokeData) {}

		//! \fn SettingChanged
		//! Called each time a setting in the scanner has been altered.
		//! \param rSettingData reference to a SettingData_t structure defining the Setting changed in the Radar and its new value.
		//! \sa SettingData_t
		//! \sa eSettings_t
		//! \sa eChannel_t
		virtual void SettingChanged(QuantumLib::SettingData_t& rSettingData) {}

		//! \fn FeatureChanged
		//! Typically this function will only be called when the scanner is initially detected as the available Features do not change.\n 
		//! The function will be called for each Feature available.
		//! \param SerialNumber Defines the scanner that this Feature applies too.
		//! \param Feature defines the available feature
		//! \param Supported defines if the feature is supported
		//! \sa eFeatures_t
		virtual void FeatureChanged(uint32_t SerialNumber, eFeatures_t Feature, bool Supported) {}

		//! \fn ParameterChanged
		//! This function is called on initialisation.\n
		//! Parameters specify the Minimum and Maximum Range Index available on the scanner and the Maximum number of Marpa Targets that can be acquired by the scanner.\n
		//! \param SerialNumber Defines the scanner that this Parameter applies too.
		//! \param Parameter Index of the new parameter
		//! \param Value Value of the new parameter
		// \sa eParameters_t
		virtual void ParameterChanged(uint32_t SerialNumber, QuantumLib::eParameters_t Parameter, uint32_t Value) {}

		//! \fn MarpaDataChanged
		//! \param SerialNumber Defines the scanner that the Marpa Data applies too.
		//! \param rMarpaData reference to marpa data structure
		//! \sa MarpaData_t
		virtual void MarpaDataChanged(uint32_t SerialNumber, QuantumLib::MarpaData_t& rMarpaData) {}

		//! \fn AlarmDataChanged
		//! This function is called for each new alarm that is genertaed by the system
		//! If multiple new alarms occur, the function is called multiple times
		//! \param SerialNumber Defines the scanner that the Alarm list applies too.
		//! \param rAlarmData reference to alarm data structure defining the current Alarm
		//! \sa AlarmData_t
		virtual void AlarmDataChanged(uint32_t SerialNumber, QuantumLib::AlarmData_t& rAlarmData) {}

	};

	//! \class API 
	//! \brief Defines the interface between the Client application and the Quantum SDK Library
	//! This class is exported from the library
	class QUANTUMLIB_API API
	{
	public:
		//! \fn Open
		//! Initialises the DLL, creates and initialises the ethernet interface and sockets.\n
		//! Enable communications between the DLL and the scanner.\n
		//! Creates classes for encoding and decoding messages.\n
		//! \return eNoError
		//! \return eErrorOutOfMemory
		//! \return eErrorGeneralFailure
		//! \return eErrorEthernetInterfaceFailure
		//! \return eErrorEthernetFailedToGetIpaddress
		//! \return eErrorEthernetFailedToCreateSysSockets
		//! \sa eErrorCode_t
		static eErrorCode_t Open();
		//! \fn Close
		//! Close ethernet interface and destroys classes created by Open.
		static void Close();

		//! \fn RegisterForScannerNotifications
		//! Defines the class that should be called for Scanner Notification Messages
		//! \return eNoError 
		//! \return eErrorNotifyAlreadyRegistered
		//! \return eErrorInvalidNotifyFunction
		//! \sa eErrorCode_t
		static eErrorCode_t RegisterForScannerNotifications(INotify* pINotify);

		//! \fn DeRegisterForScannerNotifications
		//! Removes the class interface for Scanner Notification Messages
		static void DeRegisterForScannerNotifications(INotify* pINotify);

		//! \fn static QUANTUMLIB_API eErrorCode_t NewScannerSetting(SettingData_t& rSettingData)
		//! Function to call to control scanner
		//! \param rSettingData Reference to SettingData_t structure. This gives details of the Setting that has cahnged and its new value.
		//! \return eNoError 
		//! \return eErrorInvalidValue
		//! \return eErrorFeatureNotSupported
		//! \return eErrorInvalidSetting
		//! \return eErrorEthernetTxMessageLength
		//! \return eErrorGeneralFailure
		//! \sa eErrorCode_t
		//! \sa SettingData_t
		//! \sa eSettings_t
		//! \sa eChannel_t
		static eErrorCode_t NewScannerSetting(SettingData_t& rSettingData);

		//! \fn GetScannerDetails
		//! Returns a reference to the ScannerDetails structure, containing the Serial Number and Textual description of the scanner.
		//! \param ScannerIndex Index returned by QuantumLib::INotify::ScannerListChanged() function.
		//! \param rScannerDetails Reference to scanner details structure
		//! \return eNoError 
		//! \return eErrorInvalidScannerIndex
		//! \sa eErrorCode_t
		//! \sa ScannerDetails_t
		//! \sa QuantumLib::INotify::ScannerListChanged()
		static eErrorCode_t GetScannerDetails(uint32_t ScannerIndex, ScannerDetails_t& rScannerDetails);

	private:
	};

}


