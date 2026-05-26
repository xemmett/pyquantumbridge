//! \file QuantumLibDebug.h
//! Enables Debug Output from QuantumLib DLL.\n
//! Debug Output can be directed to a console window (using printf), Visual Studio Output Window (using OutputDebugString) or File (using fprintf).\n
//! Debug Output is directed using the enumerted type eDebugOutput_t.\n
//! The level of output can also be configured enumerated type eDebugLevel_t.\n
//! By default Debug Output is turned off .\n
//! Configuration File QuantumLibDebug.Cfg has the following format.\n
//! Filename: QuantumLibDebugOutput.Txt\n
//! Output :  0\n
//! Level :   0\n

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n


#pragma once

#include <stdint.h>
#include <cstdio>

//! \enum eDebugLevel_t
//! sets the level of debug output required. Setting a value output all messages of that level and lower levels.\n
//! Setting level = eDebugLevelFailure produces the minimum amount of debug output.\n
//! Setting level = eDebugLevelRadarPkts outputs all debug messages.\n
enum eDebugLevel_t
{
	eDebugLevelNone = 0,				//!< No debug messages are output
	eDebugLevelFailure = 1,				//!< Failure messages, can't create class, sockets ...
	eDebugLevelEthernet = 2,			//!< Ethernet Messages, creating threads, opening sockets, binding ports ...
	eDebugLevelControl = 3,				//!< Control parameter messages sent to radar
	eDebugLevelMarpa = 4,				//!< Marpa and Alarm Notification messages
	eDebugLevelSystemPkts = 5,			//!< Received System Packets UnitInfo, Service Info
	eDebugLevelRadarPkts = 6,			//!< Received Radar Packets
	eDebugLevelMax = eDebugLevelRadarPkts
};

extern void DEBUG_TRACE(eDebugLevel_t level, const char *fmt, ...);

//! \class QuantumLibDebug QuantumLibDebug.h 
//! \details Class to handle debug output from QuantumLib.DLL.\n
//! The level of debug output and the output device can be specified from the configuration file QuantumLibDebug.Cfg\n
//! \sa eDebugOutput_t
//! \sa eDebugLevel_t
class QuantumLibDebug
{
public:
	//! \fn QuantumLibDebug();
	//! Initialise parameters for no output.\n
	QuantumLibDebug();
	//! \fn ~QuantumLibDebug()
	//! Close the debug output file.
	~QuantumLibDebug();
	//! \fn void Initialise()
	//! Reads the configuration file QuantumLibDebug.Cfg and sets the appropriate debug options opening the output file if required.\n
	//! If no configuration file is found, no output is produced.\n
	void Initialise();
	//! \fn void OutputString(eDebugLevel_t level, char* DebugStr)
	//! Writes debug to output stream
	void OutputString(eDebugLevel_t level, char* DebugStr);

	//! \enum eDebugOutput_t
	//! Directs the output to the specified stream
	enum eDebugOutput_t
	{
		eDebugOutputNone = 0,			//!< No Debug output
		eDebugOutputConsole = 1,		//!< Debug oputput uses printf to send to a console window
		eDebugOutputVisualStudio = 2,	//!< Debug Output is directed to the Output window in Visual Studio
		eDebugOutputFile = 3			//!< debug output is directed to the file specified in "QuantumLibDebug.Cfg"
	};

private:


	void _SetDebugLevel(eDebugLevel_t DebugLevel);
	void _SetDebugOutput(eDebugOutput_t DebugOutput);
	void _SetDebugFilename(char* pFilename);

	const static uint32_t cMaxLineSize = 128;
	const static eDebugLevel_t cMaxDebugLevel = eDebugLevelMax;

	FILE *m_fp;
	char m_DebugFilename[cMaxLineSize];
	eDebugOutput_t m_DebugOutput;
};

