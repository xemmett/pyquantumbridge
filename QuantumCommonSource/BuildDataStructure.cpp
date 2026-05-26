
#include "BuildDataStructure.h"

static uint32_t SequenceNumber = 0;

// ------------------------------------------------------------------------------------------
//! \fn bool _BuildDataStructure(QuantumLib::SettingData_t& S, uint32_t ParamCount, uint32_t* pParams)
//! Extracts the command value (or values) from the command line parameters and enters them into the correct fields of the SettingData_t structure
//! SerialNumber, Setting and Channel are already configured, so this function just fills in the appropriate value field for the Setting
bool _BuildDataStructure(QuantumLib::SettingData_t& S, uint32_t ParamCount, uint32_t* pParams)
{
    bool ret = false;

    QuantumLib::eSettings_t Setting = S.GetSetting();
    switch (Setting)
    {
    case QuantumLib::eSettingRadarMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eRadarMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingInterferenceRejection:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eInterferenceRejectionMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingBearingAlignment:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<int16_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingTimedTransmit:
        if (ParamCount == 2)
        {
            QuantumLib::TimedTransmit_t TimedTransmit;
            TimedTransmit.StandbyTime_Mins = *pParams++;
            TimedTransmit.TransmitCount_Scans = *pParams++;
            S.SetValue(Setting, TimedTransmit);
            ret = true;
        }
        break;

    case QuantumLib::eSettingTxFrequency:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eTransmitFrequency_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingPresetMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::ePreset_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingGainMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eGainMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingColourGainMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eColourGainMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingSeaMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eSeaMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingRainMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eRainMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingTargetExpansion:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eTargetExpansion_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingSeaClutterCurve:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eSeaCurve_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingMainBang:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eMainBang_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingDopplerMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eDopplerMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingAutoAcquireMode:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<QuantumLib::eAutoAcquireMode_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingGuardZone1:
    case QuantumLib::eSettingGuardZone2:
        if (ParamCount == 4)
        {
            QuantumLib::Zone_t Zone;
            Zone.Id = 0;
            if (Setting == QuantumLib::eSettingGuardZone2)
            {
                Zone.Id = 1;
            }
            Zone.StartRange_nm_x1000 = *pParams++;
            Zone.EndRange_nm_x1000 = *pParams++;
            Zone.StartAngle_degs_x10 = *pParams++;
            Zone.EndAngle_degs_x10 = *pParams++;
            Zone.Enable = true;
            S.SetValue(Setting, Zone);
            ret = true;
        }
        break;
    case QuantumLib::eSettingAutoAccquireZone1:
    case QuantumLib::eSettingAutoAccquireZone2:
        if (ParamCount == 4)
        {
            QuantumLib::Zone_t Zone;
            Zone.Id = 0;
            if (Setting == QuantumLib::eSettingAutoAccquireZone2)
            {
                Zone.Id = 1;
            }
            Zone.StartRange_nm_x1000 = *pParams++;
            Zone.EndRange_nm_x1000 = *pParams++;
            Zone.StartAngle_degs_x10 = *pParams++;
            Zone.EndAngle_degs_x10 = *pParams++;
            Zone.Enable = true;
            S.SetValue(Setting, Zone);
            ret = true;
        }
        break;
    case QuantumLib::eSettingAutoAcquireZoneEnable:
    case QuantumLib::eSettingGuardZoneEnable:
        if (ParamCount == 2)
        {
            QuantumLib::ZoneState_t ZoneState;
            ZoneState.Id = *pParams++;
            ZoneState.Enable = false;
            uint32_t val = *pParams++;
            if (val == 1)
            {
                ZoneState.Enable = true;
            }
            S.SetValue(Setting, ZoneState);
            ret = true;
        }
        break;
    case QuantumLib::eSettingRange:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<uint8_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingGuardZoneSensitivity:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<uint8_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingGainValue:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<uint8_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingColourGainValue:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<uint8_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingSeaValue:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<uint8_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingRainValue:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, static_cast<uint8_t>(*pParams));
            ret = true;
        }
        break;
    case QuantumLib::eSettingNavData:
        if (ParamCount == 6)
        {
            QuantumLib::NavData_t NavData;
            uint32_t valid = *pParams++;
            uint32_t value = *pParams++;
            if (valid != 0)
            {
                NavData.Heading_degs.Valid = true;
                NavData.Heading_degs.Value = static_cast<float>(value);
            }
            else
            {
                NavData.Heading_degs.Valid = false;
                NavData.Heading_degs.Value = 0;
            }
            valid = *pParams++;
            value = *pParams++;
            if (valid != 0)
            {
                NavData.Cog_degs.Valid = true;
                NavData.Cog_degs.Value = static_cast<float>(value);
            }
            else
            {
                NavData.Cog_degs.Valid = false;
                NavData.Cog_degs.Value = 0;
            }
            valid = *pParams++;
            value = *pParams++;
            if (valid != 0)
            {
                NavData.Sog_mps.Valid = true;
                NavData.Sog_mps.Value = static_cast<float>(value);
            }
            else
            {
                NavData.Sog_mps.Valid = false;
                NavData.Sog_mps.Value = 0;
            }
            S.SetValue(Setting, NavData);
            ret = true;
        }
        break;
    case QuantumLib::eSettingMarpaSetup:
        if (ParamCount == 2)
        {
            QuantumLib::MarpaSetup_t MarpaSetup;
            MarpaSetup.SafeZoneDistance_metres = *pParams++;
            MarpaSetup.TimeToSafeZone_secs = *pParams++;
            S.SetValue(Setting, MarpaSetup);
            ret = true;
        }
        break;

    case QuantumLib::eSettingMarpaDesignate:
        if (ParamCount == 2)
        {
            QuantumLib::MarpaDesignate_t MarpaDesignate;
            MarpaDesignate.Range_metres = static_cast<float>(*pParams++);
            MarpaDesignate.Bearing_degs = static_cast<float>(*pParams++);
            S.SetValue(Setting, MarpaDesignate);
            ret = true;
        }
        break;
    case QuantumLib::eSettingMarpaClear:
        if (ParamCount == 1)
        {
            S.SetValue(Setting, *pParams++);
            ret = true;
        }
        break;

    case QuantumLib::eSettingAlarmAck:
        if (ParamCount == 2)
        {
            QuantumLib::AlarmAck_t AlarmAck;
            AlarmAck.Id = *pParams++;
            AlarmAck.Type = static_cast<QuantumLib::eAlarmType_t>(*pParams++);
            S.SetValue(Setting, AlarmAck);
            ret = true;
        }
        break;

    default:
        break;
    }

    return ret;
}
// ------------------------------------------------------------------------------------------------
// end 


