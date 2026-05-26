// -------------------------------------------------------------------------------------------------
//! \file QuantumClientDialog.h
//! User defined Windows messages for commuinicating between the QuantumLib API and QuantumClientDialog App

//! Copyright (C) 2018, 2019 Raymarine - All Rights Reserved.\n
//! You may use, distribute and modify this code under the terms of the Raymarine Radar SDK License Agreement.\n
//! You should have received a copy of the Raymarine Radar SDK License Agreement with this file. If not, please contact Raymarine.\n

#pragma once

// ------------------------------------------------------------------------------------------
// include files

#include "resource.h"

// ------------------------------------------------------------------------------------------
// Windows user defined message values

#define WM_USER_SCANNER_LIST_MSG		WM_USER + 0x100
#define WM_USER_SPOKE_DATA_MSG			WM_USER + 0x101
#define WM_USER_NOTIFICATION_MSG		WM_USER + 0x102
#define WM_USER_ALARM_MSG				WM_USER + 0x103
#define WM_USER_MARPA_MSG				WM_USER + 0x104

// ------------------------------------------------------------------------------------------
