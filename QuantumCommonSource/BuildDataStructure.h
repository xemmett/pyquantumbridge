
#pragma once

#include <stdint.h>
#include "../QuantumLib/QuantumLib/Include/QuantumLib.h"



// ------------------------------------------------------------------------------------------
//! \fn bool _BuildDataStructure(QuantumLib::SettingData_t& S, uint32_t ParamCount, uint32_t* pParams)
//! Extracts the command value (or values) from the command line parameters and enters them into the correct fields of the SettingData_t structure
//! SerialNumber, Setting and Channel are already configured, so this function just fills in the appropriate value field for the Setting
bool _BuildDataStructure(QuantumLib::SettingData_t& S, uint32_t ParamCount, uint32_t* pParams);

// ------------------------------------------------------------------------------------------------
// end 


