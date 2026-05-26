// -------------------------------------------------------------------------------------------------
//! \file CLI.cpp
//! Implementation of Command Line Interpreter Class

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

#include "stdafx.h"
#include "CLI.h"

#include <memory>
#include <cctype>

// ------------------------------------------------------------------------------------------
//! \fn CLI()
//! Constructor, initialisation of parameters
CLI::CLI() :
m_CommandLineSize(0),
m_CommandLineOffset(0),
m_ParsingComplete(false)
{
	memset(m_CommandLineBuffer, 0, cBufferSize);
	memset(&m_CommandLineParams, 0, sizeof(m_CommandLineParams));

}

// ------------------------------------------------------------------------------------------
//! \fn CLI()
//! Destructor nothing to do
CLI::~CLI()
{
}

// ------------------------------------------------------------------------------------------
//! \fn ProcessCommandLine
//! Reads and Parses command line 
void CLI::ProcessCommandLine(char* pCmdLine, CLI_Command_t* pCommandLineParams)
{
	m_CommandLineSize = 0;
	m_CommandLineOffset = 0;

	memset(pCommandLineParams, 0, sizeof(CLI_Command_t));

	_GetCommandLine(pCmdLine);
	_ParseCommandLine(pCommandLineParams);

}

// ------------------------------------------------------------------------------------------
//! \fn _GetCommandLine
//! Reads command line
void CLI::_GetCommandLine(char* pCmdLine)
{
	bool done = false;
	m_CommandLineSize = 0;
	strcpy(m_CommandLineBuffer, pCmdLine);
	m_CommandLineSize = static_cast<uint32_t>(strlen(m_CommandLineBuffer));
}
// ------------------------------------------------------------------------------------------
//! \fn _ParseCommandLine
//! Parses a command line looking for comma seperated numerical values
void CLI::_ParseCommandLine(CLI_Command_t* pCLI_Command)
{
	m_ParsingComplete = false;
	char *pNextParam = nullptr;
	uint32_t Value;
	uint32_t ParamIndex = 0;
	bool EndOfCommandLine = false;

	while (EndOfCommandLine == false)
	{
		pNextParam = _FindNextParameter();
		_FindNextDelimeter();

		if (pNextParam == NULL)
		{
			EndOfCommandLine = true;
		}
		else
		{
			if (_ConvertUnsignedInt(pNextParam, &Value) == true)
			{
				switch (ParamIndex)
				{
				case 0:
					pCLI_Command->ScannerIndex = Value;
					break;
				case 1:
					pCLI_Command->CommandIndex = Value;
					break;
				default:
					if (ParamIndex < (cMaxParams + 2))
					{
						pCLI_Command->Param[ParamIndex - 2] = Value;
						pCLI_Command->ParamCount = ParamIndex - 1;
					}
					break;
				}

				ParamIndex++;
			}
		}
	}
}

// ------------------------------------------------------------------------------------------
//! \fn _FindNextParameter
//! Searches for next parameter on command line, skipping whitespace
char* CLI::_FindNextParameter(void)
{
	char* pRet = NULL;
	if (m_ParsingComplete == false)
	{
		while (isspace(m_CommandLineBuffer[m_CommandLineOffset]) && (m_CommandLineBuffer[m_CommandLineOffset] != 0) && (m_CommandLineOffset < m_CommandLineSize))
		{
			m_CommandLineOffset++;
		}

		if (isascii(m_CommandLineBuffer[m_CommandLineOffset]))
		{
			pRet = m_CommandLineBuffer + m_CommandLineOffset;
		}
		else
		{
			// if we haven't found an ascii character, we must have reached the end of the buffer
			m_ParsingComplete = true;
		}
	}
	return pRet;
}
// ------------------------------------------------------------------------------------------
//! \fn _FindNextDelimeter
//! Searches for next delimeter (',') on command line,
void CLI::_FindNextDelimeter(void)
{
	if (m_ParsingComplete == false)
	{
		while ((m_CommandLineBuffer[m_CommandLineOffset] != ',') && (m_CommandLineBuffer[m_CommandLineOffset] != 0) && (m_CommandLineOffset < m_CommandLineSize))
		{
			m_CommandLineOffset++;
		}

		if (m_CommandLineBuffer[m_CommandLineOffset] == ',')
		{
			// replace delimeter with null
			m_CommandLineBuffer[m_CommandLineOffset++] = 0;
		}
		else
		{
			// if we haven't found the delimeter, we must have reached the end of the buffer
			m_ParsingComplete = true;
		}
	}
}
// -----------------------------------------------------------------------------
//! \fn _ConvertUnsignedInt
//! Converts string to unsigned integer, string can be in decimal or hex format (0x123abc)
bool CLI::_ConvertUnsignedInt(char* pStr, unsigned int* pValue)
{
	bool ret = false;
	char* pTemp;
	*pValue = 0;
	pTemp = strstr(pStr, "0x");
	if (pTemp == NULL)
	{
		// search for normal integer
		int count = sscanf_s(pStr, "%u", pValue);
		if (count == 1)
		{
			ret = true;
		}
	}
	else
	{
		// search for hex value 0x...
		int count = sscanf_s(pStr, "0x%x", pValue);
		if (count == 1)
		{
			ret = true;
		}
	}
	return ret;
}
// -----------------------------------------------------------------------------


