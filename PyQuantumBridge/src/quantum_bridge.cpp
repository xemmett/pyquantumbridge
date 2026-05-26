// Python bridge for Raymarine Quantum Radar SDK
// Build with CMakeLists.txt (recommended) or setup.py

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <vector>
#include <stdexcept>
#include <string>

#include "QuantumLib.h"
#include "QuantumLibTypes.h"

namespace py = pybind11;
using namespace QuantumLib;

// ---------------------------------------------------------------------------
// SpokeDataPy — owns a copy of the spoke data.
// The DLL passes SpokeData_t by reference only for the duration of the
// callback, so we copy it immediately before Python can hold a reference.
// ---------------------------------------------------------------------------
struct SpokeDataPy {
    uint16_t samples_per_spoke;
    uint16_t spokes_per_scan;
    uint8_t  channel;
    uint16_t instrumented_range;
    uint16_t bearing;
    uint16_t data_length;
    std::vector<uint8_t> data;

    explicit SpokeDataPy(const SpokeData_t& s)
        : samples_per_spoke(s.SamplesPerSpoke)
        , spokes_per_scan(s.SpokesPerScan)
        , channel(s.Channel)
        , instrumented_range(s.InstrumentedRange)
        , bearing(s.Bearing)
        , data_length(s.DataLength)
        , data(s.SpokeData, s.SpokeData + s.DataLength)
    {}

    py::array_t<uint8_t> to_numpy() const {
        return py::array_t<uint8_t>(
            { (py::ssize_t)data.size() },
            { sizeof(uint8_t) },
            data.data()
        );
    }
};

// ---------------------------------------------------------------------------
// PyINotify — trampoline enabling Python subclassing of INotify.
//
// Callbacks arrive from the DLL's internal thread, so we acquire the GIL
// before touching any Python objects.
//
// For callbacks that pass data by reference (SpokeDataReceived, SettingChanged,
// MarpaDataChanged, AlarmDataChanged) we pass Python-owned copies so that
// Python code is free to store them beyond the callback's lifetime.
// ---------------------------------------------------------------------------
class PyINotify : public INotify {
public:
    using INotify::INotify;

    void ScannerListChanged(uint32_t count) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "scanner_list_changed");
        if (fn) fn(count);
    }

    void SpokeDataReceived(uint32_t sn, SpokeData_t& raw) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "spoke_data_received");
        if (fn) fn(sn, py::cast(SpokeDataPy(raw)));
    }

    // SettingData_t(s) copies all fields (including private ones via default
    // copy-ctor), giving Python a fully owned instance.
    void SettingChanged(SettingData_t& s) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "setting_changed");
        if (fn) fn(py::cast(SettingData_t(s)));
    }

    void FeatureChanged(uint32_t sn, eFeatures_t f, bool supported) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "feature_changed");
        if (fn) fn(sn, f, supported);
    }

    void ParameterChanged(uint32_t sn, eParameters_t p, uint32_t v) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "parameter_changed");
        if (fn) fn(sn, p, v);
    }

    void MarpaDataChanged(uint32_t sn, MarpaData_t& raw) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "marpa_data_changed");
        if (fn) fn(sn, py::cast(MarpaData_t(raw)));
    }

    void AlarmDataChanged(uint32_t sn, AlarmData_t& raw) override {
        py::gil_scoped_acquire g;
        py::object fn = py::get_override(this, "alarm_data_changed");
        if (fn) fn(sn, py::cast(AlarmData_t(raw)));
    }
};

// ---------------------------------------------------------------------------
// SettingData factory functions — one per writable setting.
// These hide the C++ overload resolution from Python.
// ---------------------------------------------------------------------------

static SettingData_t make_radar_mode(uint32_t sn, eRadarMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingRadarMode, v); return s;
}
static SettingData_t make_interference_rejection(uint32_t sn, eInterferenceRejectionMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingInterferenceRejection, v); return s;
}
static SettingData_t make_bearing_alignment(uint32_t sn, int16_t v) {
    SettingData_t s(sn); s.SetValue(eSettingBearingAlignment, v); return s;
}
static SettingData_t make_timed_transmit(uint32_t sn, uint8_t standby_mins, uint8_t tx_scans) {
    TimedTransmit_t tt; tt.StandbyTime_Mins = standby_mins; tt.TransmitCount_Scans = tx_scans;
    SettingData_t s(sn); s.SetValue(eSettingTimedTransmit, tt); return s;
}
static SettingData_t make_tx_frequency(uint32_t sn, eTransmitFrequency_t v) {
    SettingData_t s(sn); s.SetValue(eSettingTxFrequency, v); return s;
}
static SettingData_t make_guard_zone_sensitivity(uint32_t sn, uint8_t v) {
    SettingData_t s(sn); s.SetValue(eSettingGuardZoneSensitivity, v); return s;
}
static SettingData_t make_guard_zone1(uint32_t sn, uint32_t sr, uint32_t er,
                                       uint32_t sa, uint32_t ea) {
    Zone_t z; z.StartRange_nm_x1000 = sr; z.EndRange_nm_x1000 = er;
    z.StartAngle_degs_x10 = sa; z.EndAngle_degs_x10 = ea; z.Enable = true; z.Id = 0;
    SettingData_t s(sn); s.SetValue(eSettingGuardZone1, z); return s;
}
static SettingData_t make_guard_zone2(uint32_t sn, uint32_t sr, uint32_t er,
                                       uint32_t sa, uint32_t ea) {
    Zone_t z; z.StartRange_nm_x1000 = sr; z.EndRange_nm_x1000 = er;
    z.StartAngle_degs_x10 = sa; z.EndAngle_degs_x10 = ea; z.Enable = true; z.Id = 1;
    SettingData_t s(sn); s.SetValue(eSettingGuardZone2, z); return s;
}
static SettingData_t make_guard_zone_enable(uint32_t sn, uint8_t id, bool enable) {
    ZoneState_t zs; zs.Id = id; zs.Enable = enable;
    SettingData_t s(sn); s.SetValue(eSettingGuardZoneEnable, zs); return s;
}
static SettingData_t make_auto_acquire_mode(uint32_t sn, eAutoAcquireMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingAutoAcquireMode, v); return s;
}
static SettingData_t make_range_index(uint32_t sn, uint8_t v) {
    SettingData_t s(sn); s.SetValue(eSettingRange, v); return s;
}
static SettingData_t make_preset_mode(uint32_t sn, ePreset_t v) {
    SettingData_t s(sn); s.SetValue(eSettingPresetMode, v); return s;
}
static SettingData_t make_gain_mode(uint32_t sn, eGainMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingGainMode, v); return s;
}
static SettingData_t make_gain_value(uint32_t sn, uint8_t v) {
    SettingData_t s(sn); s.SetValue(eSettingGainValue, v); return s;
}
static SettingData_t make_colour_gain_mode(uint32_t sn, eColourGainMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingColourGainMode, v); return s;
}
static SettingData_t make_colour_gain_value(uint32_t sn, uint8_t v) {
    SettingData_t s(sn); s.SetValue(eSettingColourGainValue, v); return s;
}
static SettingData_t make_sea_mode(uint32_t sn, eSeaMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingSeaMode, v); return s;
}
static SettingData_t make_sea_value(uint32_t sn, uint8_t v) {
    SettingData_t s(sn); s.SetValue(eSettingSeaValue, v); return s;
}
static SettingData_t make_rain_mode(uint32_t sn, eRainMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingRainMode, v); return s;
}
static SettingData_t make_rain_value(uint32_t sn, uint8_t v) {
    SettingData_t s(sn); s.SetValue(eSettingRainValue, v); return s;
}
static SettingData_t make_target_expansion(uint32_t sn, eTargetExpansion_t v) {
    SettingData_t s(sn); s.SetValue(eSettingTargetExpansion, v); return s;
}
static SettingData_t make_sea_clutter_curve(uint32_t sn, eSeaCurve_t v) {
    SettingData_t s(sn); s.SetValue(eSettingSeaClutterCurve, v); return s;
}
static SettingData_t make_main_bang(uint32_t sn, eMainBang_t v) {
    SettingData_t s(sn); s.SetValue(eSettingMainBang, v); return s;
}
static SettingData_t make_doppler_mode(uint32_t sn, eDopplerMode_t v) {
    SettingData_t s(sn); s.SetValue(eSettingDopplerMode, v); return s;
}
static SettingData_t make_nav_data(uint32_t sn,
                                    bool hv, float hd,
                                    bool cv, float cd,
                                    bool sv, float sm) {
    NavData_t nd;
    nd.Heading_degs.Valid = hv; nd.Heading_degs.Value = hd;
    nd.Cog_degs.Valid     = cv; nd.Cog_degs.Value     = cd;
    nd.Sog_mps.Valid      = sv; nd.Sog_mps.Value      = sm;
    SettingData_t s(sn); s.SetValue(eSettingNavData, nd); return s;
}
static SettingData_t make_marpa_setup(uint32_t sn, uint32_t dist_m, uint32_t time_s) {
    MarpaSetup_t ms; ms.SafeZoneDistance_metres = dist_m; ms.TimeToSafeZone_secs = time_s;
    SettingData_t s(sn); s.SetValue(eSettingMarpaSetup, ms); return s;
}
static SettingData_t make_marpa_designate(uint32_t sn, float range_m, float bearing_degs) {
    MarpaDesignate_t md; md.Range_metres = range_m; md.Bearing_degs = bearing_degs;
    SettingData_t s(sn); s.SetValue(eSettingMarpaDesignate, md); return s;
}
static SettingData_t make_marpa_clear(uint32_t sn, uint32_t target_id) {
    SettingData_t s(sn); s.SetValue(eSettingMarpaClear, target_id); return s;
}
static SettingData_t make_alarm_ack(uint32_t sn, uint32_t alarm_id, eAlarmType_t t) {
    AlarmAck_t aa; aa.Id = alarm_id; aa.Type = t;
    SettingData_t s(sn); s.SetValue(eSettingAlarmAck, aa); return s;
}

// ---------------------------------------------------------------------------
// SettingData getter helpers — return the value or None if type doesn't match.
// ---------------------------------------------------------------------------

#define SD_GET_ENUM(fn_name, CppType, getter_call) \
    static py::object fn_name(SettingData_t& s) { \
        CppType v; s.getter_call(v); \
        return s.IsValid() ? py::cast(v) : py::none(); \
    }

SD_GET_ENUM(sd_get_radar_mode,             eRadarMode_t,              GetValue)
SD_GET_ENUM(sd_get_interference_rejection, eInterferenceRejectionMode_t, GetValue)
SD_GET_ENUM(sd_get_tx_frequency,           eTransmitFrequency_t,      GetValue)
SD_GET_ENUM(sd_get_gain_mode,              eGainMode_t,               GetValue)
SD_GET_ENUM(sd_get_colour_gain_mode,       eColourGainMode_t,         GetValue)
SD_GET_ENUM(sd_get_sea_mode,               eSeaMode_t,                GetValue)
SD_GET_ENUM(sd_get_rain_mode,              eRainMode_t,               GetValue)
SD_GET_ENUM(sd_get_preset_mode,            ePreset_t,                 GetValue)
SD_GET_ENUM(sd_get_target_expansion,       eTargetExpansion_t,        GetValue)
SD_GET_ENUM(sd_get_sea_clutter_curve,      eSeaCurve_t,               GetValue)
SD_GET_ENUM(sd_get_main_bang,              eMainBang_t,               GetValue)
SD_GET_ENUM(sd_get_doppler_mode,           eDopplerMode_t,            GetValue)
SD_GET_ENUM(sd_get_doppler_active,         eDopplerActive_t,          GetValue)
SD_GET_ENUM(sd_get_auto_acquire_mode,      eAutoAcquireMode_t,        GetValue)

static py::object sd_get_bearing_alignment(SettingData_t& s) {
    int16_t v; s.GetValue(v);
    return s.IsValid() ? py::cast(v) : py::none();
}
static py::object sd_get_u8(SettingData_t& s) {
    uint8_t v; s.GetValue(v);
    return s.IsValid() ? py::cast(v) : py::none();
}
static py::object sd_get_u32(SettingData_t& s) {
    uint32_t v; s.GetValue(v);
    return s.IsValid() ? py::cast(v) : py::none();
}
static py::object sd_get_timed_transmit(SettingData_t& s) {
    TimedTransmit_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d;
    d["standby_mins"]    = v.StandbyTime_Mins;
    d["transmit_scans"]  = v.TransmitCount_Scans;
    return d;
}
static py::object sd_get_timed_transmit_remaining(SettingData_t& s) {
    TimedTransmitRemaining_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d;
    d["mins"] = v.Mins; d["secs"] = v.Secs;
    return d;
}
static py::object sd_get_zone(SettingData_t& s) {
    Zone_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d;
    d["start_range_nm_x1000"]  = v.StartRange_nm_x1000;
    d["end_range_nm_x1000"]    = v.EndRange_nm_x1000;
    d["start_angle_degs_x10"]  = v.StartAngle_degs_x10;
    d["end_angle_degs_x10"]    = v.EndAngle_degs_x10;
    d["enable"] = v.Enable; d["id"] = v.Id;
    return d;
}
static py::object sd_get_zone_state(SettingData_t& s) {
    ZoneState_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d; d["id"] = v.Id; d["enable"] = v.Enable;
    return d;
}
static py::object sd_get_nav_data(SettingData_t& s) {
    NavData_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d;
    d["heading_valid"] = v.Heading_degs.Valid; d["heading_degs"] = v.Heading_degs.Value;
    d["cog_valid"]     = v.Cog_degs.Valid;     d["cog_degs"]     = v.Cog_degs.Value;
    d["sog_valid"]     = v.Sog_mps.Valid;      d["sog_mps"]      = v.Sog_mps.Value;
    return d;
}
static py::object sd_get_marpa_setup(SettingData_t& s) {
    MarpaSetup_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d;
    d["safe_zone_distance_m"]    = v.SafeZoneDistance_metres;
    d["time_to_safe_zone_secs"]  = v.TimeToSafeZone_secs;
    return d;
}
static py::object sd_get_marpa_designate(SettingData_t& s) {
    MarpaDesignate_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::dict d; d["range_m"] = v.Range_metres; d["bearing_degs"] = v.Bearing_degs;
    return d;
}
static py::object sd_get_custom_ranges(SettingData_t& s) {
    CustomRanges_t v; s.GetValue(v);
    if (!s.IsValid()) return py::none();
    py::list lst;
    for (uint32_t i = 0; i < cMaxCustomRanges; ++i) lst.append(v.fRange[i]);
    return lst;
}

// ---------------------------------------------------------------------------
// API wrappers
// ---------------------------------------------------------------------------

static eErrorCode_t api_open()           { return API::Open(); }
static void         api_close()          { API::Close(); }
static eErrorCode_t api_register(INotify* n)   { return API::RegisterForScannerNotifications(n); }
static void         api_deregister(INotify* n) { API::DeRegisterForScannerNotifications(n); }
static eErrorCode_t api_new_setting(SettingData_t& s) { return API::NewScannerSetting(s); }

static py::object api_get_scanner_details(uint32_t index) {
    ScannerDetails_t det;
    eErrorCode_t err = API::GetScannerDetails(index, det);
    if (err != eNoError)
        throw std::runtime_error("GetScannerDetails failed: error code " +
                                 std::to_string(static_cast<int>(err)));
    return py::cast(det);
}

// ---------------------------------------------------------------------------
// Module definition
// ---------------------------------------------------------------------------

PYBIND11_MODULE(quantum_bridge, m) {
    m.doc() = "Raymarine Quantum Radar Python bridge (pybind11)";

    // ---- Enums ----

    py::enum_<eRadarMode_t>(m, "RadarMode")
        .value("STANDBY",          eRadarModeStandby)
        .value("TRANSMITTING",     eRadarModeTransmitting)
        .value("POWER_DOWN",       eRadarModePowerDown)
        .value("TIMED_TX",         eRadarModeTimedTx)
        .value("SLEEP",            eRadarModeSleep)
        .value("STALLED",          eRadarModeStalled)
        .value("SELF_TEST_FAILED", eRadarModeSelfTestFailed)
        .value("INVALID",          eRadarModeInvalid)
        .export_values();

    py::enum_<eInterferenceRejectionMode_t>(m, "InterferenceRejection")
        .value("OFF",     eInterferenceRejectionModeOff)
        .value("LEVEL_1", eInterferenceRejectionMode1)
        .value("LEVEL_2", eInterferenceRejectionMode2)
        .value("LEVEL_3", eInterferenceRejectionMode3)
        .value("LEVEL_4", eInterferenceRejectionMode4)
        .value("LEVEL_5", eInterferenceRejectionMode5)
        .value("INVALID", eInterferenceRejectionModeInvalid)
        .export_values();

    py::enum_<eTransmitFrequency_t>(m, "TransmitFrequency")
        .value("NOMINAL", eTransmitFrequencyNominal)
        .value("LOW",     eTransmitFrequencyLow)
        .value("HIGH",    eTransmitFrequencyHigh)
        .export_values();

    py::enum_<eChannel_t>(m, "Channel")
        .value("DEFAULT",     eChannelDefault)
        .value("SECOND",      eChannelSecond)
        .value("INDEPENDENT", eChannelIndependent)
        .export_values();

    py::enum_<ePreset_t>(m, "Preset")
        .value("HARBOUR",  ePresetHarbour)
        .value("COASTAL",  ePresetCoastal)
        .value("OFFSHORE", ePresetOffshore)
        .value("WEATHER",  ePresetWeather)
        .export_values();

    py::enum_<eGainMode_t>(m, "GainMode")
        .value("MANUAL", eGainModeManual)
        .value("AUTO",   eGainModeAuto)
        .export_values();

    py::enum_<eColourGainMode_t>(m, "ColourGainMode")
        .value("MANUAL", eColourGainModeManual)
        .value("AUTO",   eColourGainModeAuto)
        .export_values();

    py::enum_<eSeaMode_t>(m, "SeaMode")
        .value("MANUAL", eSeaModeManual)
        .value("AUTO",   eSeaModeAuto)
        .export_values();

    py::enum_<eRainMode_t>(m, "RainMode")
        .value("OFF",    eRainModeOff)
        .value("MANUAL", eRainModeManual)
        .export_values();

    py::enum_<eTargetExpansion_t>(m, "TargetExpansion")
        .value("OFF", eTargetExpansionOff)
        .value("ON",  eTargetExpansionOn)
        .export_values();

    py::enum_<eSeaCurve_t>(m, "SeaCurve")
        .value("R4",  eSeaCurve_R4)
        .value("R5_5", eSeaCurve_R5_5)
        .export_values();

    py::enum_<eMainBang_t>(m, "MainBang")
        .value("OFF", eMainBangOff)
        .value("ON",  eMainBangOn)
        .export_values();

    py::enum_<eDopplerMode_t>(m, "DopplerMode")
        .value("OFF", eDopplerOff)
        .value("ON",  eDopplerOn)
        .export_values();

    py::enum_<eAutoAcquireMode_t>(m, "AutoAcquireMode")
        .value("OFF",     eAutoAcquireOff)
        .value("ZONE",    eAutoAcquireZone)
        .value("DOPPLER", eAutoAcquireDoppler)
        .export_values();

    py::enum_<eDopplerActive_t>(m, "DopplerActive")
        .value("SUSPENDED", eDopplerSuspended)
        .value("ACTIVE",    eDopplerActive)
        .export_values();

    py::enum_<eTargetState_t>(m, "TargetState")
        .value("ACQUIRING", eAcquiring)
        .value("SAFE",      eSafe)
        .value("DANGEROUS", eDangerous)
        .value("LOST",      eLost)
        .export_values();

    py::enum_<eAlarmType_t>(m, "AlarmType")
        .value("ALL",                        eAlarmTypeAll)
        .value("GUARD_ZONE_TARGET_ENTERING", eAlarmTypeGuardZoneTargetEntering)
        .value("MARPA_DANGEROUS_TARGET",     eAlarmTypeMarpaDangerousTarget)
        .value("MARPA_LOST_TARGET",          eAlarmTypeMarpaLostTarget)
        .value("AUTO_ACQUIRE_LIST_FULL",     eAlarmTypeAutoAcquireListFull)
        .value("UNDEFINED",                  eAlarmTypeUndefined)
        .export_values();

    py::enum_<eFeatures_t>(m, "Feature")
        .value("DUAL_RANGE",   eFeatureDualRange)
        .value("RPM_48",       eFeature48RPM)
        .value("DOPPLER",      eFeatureDoppler)
        .value("AUTO_ACQUIRE", eFeatureAutoAcquire)
        .export_values();

    py::enum_<eParameters_t>(m, "Parameter")
        .value("MIN_RANGE",     eParamMinRange)
        .value("MAX_RANGE",     eParamMaxRange)
        .value("MARPA_TARGETS", eParamMarpaTargets)
        .export_values();

    py::enum_<eSettings_t>(m, "Setting")
        .value("UNDEFINED",                eSettingUndefined)
        .value("RADAR_MODE",               eSettingRadarMode)
        .value("INTERFERENCE_REJECTION",   eSettingInterferenceRejection)
        .value("BEARING_ALIGNMENT",        eSettingBearingAlignment)
        .value("TIMED_TRANSMIT",           eSettingTimedTransmit)
        .value("TX_FREQUENCY",             eSettingTxFrequency)
        .value("GUARD_ZONE_SENSITIVITY",   eSettingGuardZoneSensitivity)
        .value("GUARD_ZONE_1",             eSettingGuardZone1)
        .value("GUARD_ZONE_2",             eSettingGuardZone2)
        .value("GUARD_ZONE_ENABLE",        eSettingGuardZoneEnable)
        .value("AUTO_ACQUIRE_ZONE_1",      eSettingAutoAccquireZone1)
        .value("AUTO_ACQUIRE_ZONE_2",      eSettingAutoAccquireZone2)
        .value("AUTO_ACQUIRE_MODE",        eSettingAutoAcquireMode)
        .value("AUTO_ACQUIRE_ZONE_ENABLE", eSettingAutoAcquireZoneEnable)
        .value("CUSTOM_RANGES",            eSettingCustomRanges)
        .value("RANGE",                    eSettingRange)
        .value("PRESET_MODE",              eSettingPresetMode)
        .value("GAIN_MODE",                eSettingGainMode)
        .value("GAIN_VALUE",               eSettingGainValue)
        .value("COLOUR_GAIN_MODE",         eSettingColourGainMode)
        .value("COLOUR_GAIN_VALUE",        eSettingColourGainValue)
        .value("SEA_MODE",                 eSettingSeaMode)
        .value("SEA_VALUE",                eSettingSeaValue)
        .value("RAIN_MODE",                eSettingRainMode)
        .value("RAIN_VALUE",               eSettingRainValue)
        .value("TARGET_EXPANSION",         eSettingTargetExpansion)
        .value("SEA_CLUTTER_CURVE",        eSettingSeaClutterCurve)
        .value("MAIN_BANG",                eSettingMainBang)
        .value("DOPPLER_MODE",             eSettingDopplerMode)
        .value("DOPPLER_ACTIVE",           eSettingDopplerActive)
        .value("TIMED_TRANSMIT_REMAINING", eSettingTimedTransmitRemaining)
        .value("NAV_DATA",                 eSettingNavData)
        .value("MARPA_SETUP",              eSettingMarpaSetup)
        .value("MARPA_DESIGNATE",          eSettingMarpaDesignate)
        .value("MARPA_CLEAR",              eSettingMarpaClear)
        .value("ALARM_ACK",                eSettingAlarmAck)
        .value("MAIN_SOFTWARE_VERSION",    eSettingMainSoftwareVersion)
        .value("PSU_SOFTWARE_VERSION",     eSettingPsuSoftwareVersion)
        .value("FPGA_SOFTWARE_VERSION",    eSettingFpgaSoftwareVersion)
        .value("WIFI_SOFTWARE_VERSION",    eSettingWiFiSoftwareVersion)
        .value("W3_SOFTWARE_VERSION",      eSettingW3SoftwareVersion)
        .export_values();

    py::enum_<eErrorCode_t>(m, "ErrorCode")
        .value("NO_ERROR",                       eNoError)
        .value("GENERAL_FAILURE",                eErrorGeneralFailure)
        .value("OUT_OF_MEMORY",                  eErrorOutOfMemory)
        .value("INVALID_SCANNER_INDEX",          eErrorInvalidScannerIndex)
        .value("INVALID_SERIAL_NUMBER",          eErrorInvalidSerialNumber)
        .value("FEATURE_NOT_SUPPORTED",          eErrorFeatureNotSupported)
        .value("INVALID_VALUE",                  eErrorInvalidValue)
        .value("INVALID_DATA_SIZE",              eErrorInvalidDataSize)
        .value("INVALID_SETTING",                eErrorInvalidSetting)
        .value("INVALID_NUMBER_OF_PARAMETERS",   eErrorInvalidNumberOfParameters)
        .value("ETHERNET_TX_MESSAGE_LENGTH",     eErrorEthernetTxMessageLength)
        .value("ETHERNET_INTERFACE_FAILURE",     eErrorEthernetInterfaceFailure)
        .value("ETHERNET_FAILED_TO_GET_IP",      eErrorEthernetFailedToGetIpaddress)
        .value("ETHERNET_FAILED_TO_CREATE_SOCKETS", eErrorEthernetFailedToCreateSysSockets)
        .value("INVALID_NOTIFY_FUNCTION",        eErrorInvalidNotifyFunction)
        .value("NOTIFY_ALREADY_REGISTERED",      eErrorNotifyAlreadyRegistered)
        .export_values();

    // ---- SpokeData ----

    py::class_<SpokeDataPy>(m, "SpokeData")
        .def_readonly("samples_per_spoke",  &SpokeDataPy::samples_per_spoke)
        .def_readonly("spokes_per_scan",    &SpokeDataPy::spokes_per_scan)
        .def_readonly("channel",            &SpokeDataPy::channel)
        .def_readonly("instrumented_range", &SpokeDataPy::instrumented_range)
        .def_readonly("bearing",            &SpokeDataPy::bearing,
                      "Spoke bearing index 0-2047 (multiply by 360/2048 for degrees)")
        .def_readonly("data_length",        &SpokeDataPy::data_length)
        .def("to_numpy", &SpokeDataPy::to_numpy,
             "Return spoke intensity samples as a 1-D uint8 numpy array of length data_length.\n"
             "Value 254 = Doppler receding target, 255 = Doppler approaching target.");

    // ---- ScannerDetails ----

    py::class_<ScannerDetails_t>(m, "ScannerDetails")
        .def_property_readonly("description", [](const ScannerDetails_t& s) {
            return std::string(reinterpret_cast<const char*>(s.Description));
        })
        .def_readonly("serial_number", &ScannerDetails_t::SerialNumber)
        .def("__repr__", [](const ScannerDetails_t& s) {
            return "<ScannerDetails description='" +
                   std::string(reinterpret_cast<const char*>(s.Description)) +
                   "' serial=0x" + [&]{ char buf[16]; snprintf(buf,16,"%08X",s.SerialNumber); return std::string(buf); }() + ">";
        });

    // ---- MarpaData ----

    py::class_<MarpaData_t>(m, "MarpaData")
        .def_readonly("id",                              &MarpaData_t::Id)
        .def_readonly("valid",                           &MarpaData_t::Valid)
        .def_readonly("auto_acquired",                   &MarpaData_t::AutoAcquired)
        .def_readonly("state",                           &MarpaData_t::State)
        .def_readonly("target_range_m",                  &MarpaData_t::TargetRange_m)
        .def_readonly("target_bearing_degs",             &MarpaData_t::TargetBearing_degs)
        .def_readonly("target_true_bearing_degs",        &MarpaData_t::TargetTrueBearing_degs)
        .def_readonly("rel_target_speed_mps",            &MarpaData_t::RelTargetSpeed_mps)
        .def_readonly("rel_target_course_degs",          &MarpaData_t::RelTargetCourse_degs)
        .def_readonly("true_target_speed_mps",           &MarpaData_t::TrueTargetSpeed_mps)
        .def_readonly("true_target_course_degs",         &MarpaData_t::TrueTargetCourse_degs)
        .def_readonly("closest_point_of_approach_m",     &MarpaData_t::ClosestPointOfApproach_m)
        .def_readonly("time_to_closest_point_secs",      &MarpaData_t::TimeToClosetPoint_secs)
        .def_readonly("heading_at_last_update_degs",     &MarpaData_t::HeadingAtLastUpdate_degs)
        .def_readonly("target_going_towards_closest",    &MarpaData_t::TargetGoingTowardsClosetPoint)
        .def_readonly("target_true_data_valid",          &MarpaData_t::TargetTrueDataValid);

    // ---- AlarmData ----

    py::class_<AlarmData_t>(m, "AlarmData")
        .def_readonly("id",    &AlarmData_t::Id)
        .def_readonly("type",  &AlarmData_t::Type)
        .def_readonly("value", &AlarmData_t::Value);

    // ---- INotify (abstract base with trampoline) ----

    py::class_<INotify, PyINotify>(m, "INotify")
        .def(py::init<>())
        .def("scanner_list_changed", &INotify::ScannerListChanged,
             py::arg("count"),
             "MUST override. Called when a scanner is added or removed.")
        .def("spoke_data_received", &INotify::SpokeDataReceived,
             py::arg("serial_number"), py::arg("spoke"),
             "Called for each radar spoke. spoke is a SpokeData instance (Python-owned copy).")
        .def("setting_changed", &INotify::SettingChanged,
             py::arg("setting_data"),
             "Called when a scanner setting changes. setting_data is a Python-owned SettingData.")
        .def("feature_changed", &INotify::FeatureChanged,
             py::arg("serial_number"), py::arg("feature"), py::arg("supported"))
        .def("parameter_changed", &INotify::ParameterChanged,
             py::arg("serial_number"), py::arg("parameter"), py::arg("value"))
        .def("marpa_data_changed", &INotify::MarpaDataChanged,
             py::arg("serial_number"), py::arg("marpa_data"),
             "Called when a tracked MARPA target is updated. marpa_data is a Python-owned copy.")
        .def("alarm_data_changed", &INotify::AlarmDataChanged,
             py::arg("serial_number"), py::arg("alarm_data"),
             "Called for each new alarm. alarm_data is a Python-owned copy.");

    // ---- SettingData ----

    py::class_<SettingData_t>(m, "SettingData")
        // --- Factories (commands to scanner) ---
        .def_static("radar_mode",             &make_radar_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("interference_rejection", &make_interference_rejection,
                    py::arg("serial_number"), py::arg("level"))
        .def_static("bearing_alignment",      &make_bearing_alignment,
                    py::arg("serial_number"), py::arg("value_degs_x2"),
                    "Bearing offset in 0.5-degree steps: -359 to +360 (i.e. -179.5 to +180.0 deg)")
        .def_static("timed_transmit",         &make_timed_transmit,
                    py::arg("serial_number"), py::arg("standby_mins"), py::arg("transmit_scans"))
        .def_static("tx_frequency",           &make_tx_frequency,
                    py::arg("serial_number"), py::arg("frequency"))
        .def_static("guard_zone_sensitivity", &make_guard_zone_sensitivity,
                    py::arg("serial_number"), py::arg("value_0_100"))
        .def_static("guard_zone1",            &make_guard_zone1,
                    py::arg("serial_number"),
                    py::arg("start_range_nm_x1000"), py::arg("end_range_nm_x1000"),
                    py::arg("start_angle_degs_x10"), py::arg("end_angle_degs_x10"))
        .def_static("guard_zone2",            &make_guard_zone2,
                    py::arg("serial_number"),
                    py::arg("start_range_nm_x1000"), py::arg("end_range_nm_x1000"),
                    py::arg("start_angle_degs_x10"), py::arg("end_angle_degs_x10"))
        .def_static("guard_zone_enable",      &make_guard_zone_enable,
                    py::arg("serial_number"), py::arg("zone_id_0_or_1"), py::arg("enable"))
        .def_static("auto_acquire_mode",      &make_auto_acquire_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("range_index",            &make_range_index,
                    py::arg("serial_number"), py::arg("index"),
                    "Range index 0..max (use parameter_changed to get max range)")
        .def_static("preset_mode",            &make_preset_mode,
                    py::arg("serial_number"), py::arg("preset"))
        .def_static("gain_mode",              &make_gain_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("gain_value",             &make_gain_value,
                    py::arg("serial_number"), py::arg("value_0_100"))
        .def_static("colour_gain_mode",       &make_colour_gain_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("colour_gain_value",      &make_colour_gain_value,
                    py::arg("serial_number"), py::arg("value_0_100"))
        .def_static("sea_mode",               &make_sea_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("sea_value",              &make_sea_value,
                    py::arg("serial_number"), py::arg("value_0_100"))
        .def_static("rain_mode",              &make_rain_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("rain_value",             &make_rain_value,
                    py::arg("serial_number"), py::arg("value_0_100"))
        .def_static("target_expansion",       &make_target_expansion,
                    py::arg("serial_number"), py::arg("state"))
        .def_static("sea_clutter_curve",      &make_sea_clutter_curve,
                    py::arg("serial_number"), py::arg("curve"))
        .def_static("main_bang",              &make_main_bang,
                    py::arg("serial_number"), py::arg("state"))
        .def_static("doppler_mode",           &make_doppler_mode,
                    py::arg("serial_number"), py::arg("mode"))
        .def_static("nav_data",               &make_nav_data,
                    py::arg("serial_number"),
                    py::arg("heading_valid"), py::arg("heading_degs"),
                    py::arg("cog_valid"),     py::arg("cog_degs"),
                    py::arg("sog_valid"),     py::arg("sog_mps"))
        .def_static("marpa_setup",            &make_marpa_setup,
                    py::arg("serial_number"),
                    py::arg("safe_zone_distance_m"), py::arg("time_to_safe_zone_secs"))
        .def_static("marpa_designate",        &make_marpa_designate,
                    py::arg("serial_number"), py::arg("range_m"), py::arg("bearing_degs"),
                    "Acquire MARPA target. bearing_degs is relative to true north.")
        .def_static("marpa_clear",            &make_marpa_clear,
                    py::arg("serial_number"), py::arg("target_id"))
        .def_static("alarm_ack",              &make_alarm_ack,
                    py::arg("serial_number"), py::arg("alarm_id"), py::arg("alarm_type"))
        // --- Getters (reading from setting_changed notifications) ---
        .def("get_setting",                   &SettingData_t::GetSetting)
        .def("get_serial_number",             &SettingData_t::GetSerialNumber)
        .def("get_channel",                   &SettingData_t::GetChannel)
        .def("is_valid",                      &SettingData_t::IsValid)
        .def("get_radar_mode",                &sd_get_radar_mode)
        .def("get_interference_rejection",    &sd_get_interference_rejection)
        .def("get_bearing_alignment",         &sd_get_bearing_alignment)
        .def("get_timed_transmit",            &sd_get_timed_transmit,
             "Returns dict {standby_mins, transmit_scans} or None")
        .def("get_tx_frequency",              &sd_get_tx_frequency)
        .def("get_range_index",               &sd_get_u8)
        .def("get_gain_value",                &sd_get_u8)
        .def("get_colour_gain_value",         &sd_get_u8)
        .def("get_sea_value",                 &sd_get_u8)
        .def("get_rain_value",                &sd_get_u8)
        .def("get_guard_zone_sensitivity",    &sd_get_u8)
        .def("get_gain_mode",                 &sd_get_gain_mode)
        .def("get_colour_gain_mode",          &sd_get_colour_gain_mode)
        .def("get_sea_mode",                  &sd_get_sea_mode)
        .def("get_rain_mode",                 &sd_get_rain_mode)
        .def("get_preset_mode",               &sd_get_preset_mode)
        .def("get_target_expansion",          &sd_get_target_expansion)
        .def("get_sea_clutter_curve",         &sd_get_sea_clutter_curve)
        .def("get_main_bang",                 &sd_get_main_bang)
        .def("get_doppler_mode",              &sd_get_doppler_mode)
        .def("get_doppler_active",            &sd_get_doppler_active)
        .def("get_auto_acquire_mode",         &sd_get_auto_acquire_mode)
        .def("get_software_version",          &sd_get_u32,
             "Returns (Major * 100) + Minor for software version settings")
        .def("get_marpa_clear_target_id",     &sd_get_u32)
        .def("get_timed_transmit_remaining",  &sd_get_timed_transmit_remaining,
             "Returns dict {mins, secs} or None")
        .def("get_zone",                      &sd_get_zone,
             "Returns dict with zone geometry or None")
        .def("get_zone_state",                &sd_get_zone_state,
             "Returns dict {id, enable} or None")
        .def("get_nav_data",                  &sd_get_nav_data,
             "Returns dict {heading_valid, heading_degs, cog_valid, cog_degs, sog_valid, sog_mps} or None")
        .def("get_marpa_setup",               &sd_get_marpa_setup,
             "Returns dict {safe_zone_distance_m, time_to_safe_zone_secs} or None")
        .def("get_marpa_designate",           &sd_get_marpa_designate,
             "Returns dict {range_m, bearing_degs} or None")
        .def("get_custom_ranges",             &sd_get_custom_ranges,
             "Returns list of 20 float range values (nm) or None");

    // ---- API module-level functions ----

    m.def("open",  &api_open,
          "Initialise the DLL and Ethernet interface. Returns ErrorCode.");
    m.def("close", &api_close,
          "Close the Ethernet interface and clean up.");
    m.def("register_notifications", &api_register,
          py::arg("notifier"),
          "Register an INotify subclass to receive radar callbacks. Returns ErrorCode.");
    m.def("deregister_notifications", &api_deregister,
          py::arg("notifier"),
          "Remove a previously registered notifier.");
    m.def("new_setting", &api_new_setting,
          py::arg("setting_data"),
          "Send a command to the scanner. Returns ErrorCode.");
    m.def("get_scanner_details", &api_get_scanner_details,
          py::arg("scanner_index"),
          "Return ScannerDetails for the given index. Raises RuntimeError on failure.");

    // ---- Module-level constants ----

    m.attr("SAMPLES_PER_SPOKE")   = cSamplesPerSpoke;   // 1024
    m.attr("SPOKES_PER_SCAN")     = cSpokesPerScan;     // 2048
    m.attr("MAX_CUSTOM_RANGES")   = cMaxCustomRanges;   // 20
    m.attr("DOPPLER_APPROACHING") = (uint8_t)255;
    m.attr("DOPPLER_RECEDING")    = (uint8_t)254;
}
