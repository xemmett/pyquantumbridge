// --------------------------------------------------------------------------------------------------------------------------------
//! \file CLI.h
//! Command Line Interpreter Header file
//! Defines the CLI class

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

#pragma once

// --------------------------------------------------------------------------------------------------------------------------------
// include files

#include <stdint.h>

// --------------------------------------------------------------------------------------------------------------------------------
//! \class CLI
//! The command line interpreter is a basic command line allowing the user to set radar parameters
//! The syntax for commands is
//! <ScannerIndex>, <Setting>, <param1> [,param2, .... ,<paramn>]
//! all values entered are numerical
class CLI
{
public:
	CLI();
	~CLI();

	static const uint32_t cMaxParams = 10;	//!< max number of command line parameters
	struct CLI_Command_t
	{
		uint32_t ScannerIndex;				//!< Scanner Index 0 .. n
		uint32_t CommandIndex;				//!< Setting number /sa eSetting_t
		uint32_t Channel;					//!< Channel Index /sa eChannel_t
		uint32_t Param[cMaxParams];			//!< list of parameters
		uint32_t ParamCount;				//! count of parameters in list
	};

	//! \fn bool ProcessCommandLine(CLI_Command_t* pCommandLineParams);
	//! Processes a command line
	void ProcessCommandLine(char* pCmdLine, CLI_Command_t* pCommandLineParams);

private:
	
	void _GetCommandLine(char* pCmdLine);
	void _ParseCommandLine(CLI_Command_t* pCLI_Command);

	char* _FindNextParameter(void);
	void _FindNextDelimeter(void);

	bool _ConvertUnsignedInt(char* pStr, unsigned int* pValue);

	static const uint32_t cBufferSize = 256;

	char m_CommandLineBuffer[cBufferSize];
	uint32_t m_CommandLineSize;
	uint32_t m_CommandLineOffset;

	bool m_ParsingComplete;

	CLI_Command_t m_CommandLineParams;


};
// --------------------------------------------------------------------------------------------------------------------------------

