// -------------------------------------------------------------------------------------------------
//! \file ClientMain.cpp
//! Defines the entry point for the console application.

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

// ------------------------------------------------------------------------------------------
// include files

#include "stdafx.h"
#include <stdio.h>

#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include "QuantumLib.h"
#include "QuantumClient.h"

#include <Windows.h>
#include <new>

// ------------------------------------------------------------------------------------------
//! \fn main
//! create QuantumClient class and run QuantumClient 
int main()
{
	int ret = -1;

	QuantumClient* pQuantumClient = new(std::nothrow) QuantumClient();

	if (pQuantumClient != 0)
	{
		ret = pQuantumClient->Run();
	}

	_CrtDumpMemoryLeaks();

	return ret;
}
// ------------------------------------------------------------------------------------------

