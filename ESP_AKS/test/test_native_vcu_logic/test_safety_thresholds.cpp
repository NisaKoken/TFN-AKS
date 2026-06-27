#include <unity.h>

#include "VcuLogic.h"
#include "test_helpers.h"

using test_helpers::makeTelemetryDataValid;
using VcuLogic::hasCriticalCondition;
using VcuLogic::hasWarningCondition;
using VcuLogic::isCurrentCritical;
using VcuLogic::isCurrentWarning;
using VcuLogic::VcuState;

// ---------------------------------------------------------------------------
// isCurrentCritical — şarj tarafı (eşik: BMS_CRITICAL_MAX_CHARGE_CURRENT_CENTI_MA = 100 000)
// ---------------------------------------------------------------------------
void test_isCurrentCritical_charge_below_threshold(void) {
    TEST_ASSERT_FALSE(isCurrentCritical(90000));   // 0.9 A — altında
}

void test_isCurrentCritical_charge_at_threshold(void) {
    TEST_ASSERT_TRUE(isCurrentCritical(100000));   // 1.0 A — eşikte
}

void test_isCurrentCritical_charge_above_threshold(void) {
    TEST_ASSERT_TRUE(isCurrentCritical(110000));   // 1.1 A — üstünde
}

void test_isCurrentCritical_zero_is_safe(void) {
    TEST_ASSERT_FALSE(isCurrentCritical(0));
}

// ---------------------------------------------------------------------------
// isCurrentCritical — deşarj tarafı (eşik: -1 500 000)
// ---------------------------------------------------------------------------
void test_isCurrentCritical_discharge_below_threshold(void) {
    TEST_ASSERT_FALSE(isCurrentCritical(-1490000));  // 14.9 A — altında
}

void test_isCurrentCritical_discharge_at_threshold(void) {
    TEST_ASSERT_TRUE(isCurrentCritical(-1500000));   // 15.0 A — eşikte
}

void test_isCurrentCritical_discharge_above_threshold(void) {
    TEST_ASSERT_TRUE(isCurrentCritical(-2000000));   // 20.0 A — üstünde
}

// ---------------------------------------------------------------------------
// isCurrentWarning — şarj / deşarj (eşikler: 90 000 / -900 000)
// ---------------------------------------------------------------------------
void test_isCurrentWarning_charge_below_threshold(void) {
    TEST_ASSERT_FALSE(isCurrentWarning(80000));   // 0.8 A
}

void test_isCurrentWarning_charge_at_threshold(void) {
    TEST_ASSERT_TRUE(isCurrentWarning(90000));    // 0.9 A — eşikte
}

void test_isCurrentWarning_discharge_below_threshold(void) {
    TEST_ASSERT_FALSE(isCurrentWarning(-890000)); // 8.9 A
}

void test_isCurrentWarning_discharge_at_threshold(void) {
    TEST_ASSERT_TRUE(isCurrentWarning(-900000));  // 9.0 A — eşikte
}

// ---------------------------------------------------------------------------
// hasWarningCondition / hasCriticalCondition — sıcaklık (warn 55, crit 70)
// ---------------------------------------------------------------------------
void test_warning_temp_below_threshold(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsTempHighestC = 54;
    TEST_ASSERT_FALSE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_warning_temp_at_warn_threshold(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsTempHighestC = 55;
    TEST_ASSERT_TRUE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_critical_temp_at_critical_threshold(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsTempHighestC = 70;
    TEST_ASSERT_TRUE(hasWarningCondition(d));
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

// ---------------------------------------------------------------------------
// Pack voltajı alt sınır (warn ≤740, crit ≤700 deci-V)
// ---------------------------------------------------------------------------
void test_warning_voltage_above_warn_low(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsPackVoltageDeciV = 750;
    TEST_ASSERT_FALSE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_warning_voltage_at_warn_low(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsPackVoltageDeciV = 740;
    TEST_ASSERT_TRUE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_critical_voltage_at_crit_low(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsPackVoltageDeciV = 700;
    TEST_ASSERT_TRUE(hasWarningCondition(d));
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

// ---------------------------------------------------------------------------
// Pack voltajı üst sınır (warn ≥850, crit ≥870)
// ---------------------------------------------------------------------------
void test_warning_voltage_below_warn_high(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsPackVoltageDeciV = 840;
    TEST_ASSERT_FALSE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_warning_voltage_at_warn_high(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsPackVoltageDeciV = 850;
    TEST_ASSERT_TRUE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_critical_voltage_at_crit_high(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsPackVoltageDeciV = 870;
    TEST_ASSERT_TRUE(hasWarningCondition(d));
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

// ---------------------------------------------------------------------------
// BMS FAULT state her zaman critical tetikler.
// ---------------------------------------------------------------------------
void test_critical_motor_error_flag_set(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_motorErrorFlags = 0x01;
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

void test_critical_bms_error_flag_set(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsSystemState = 4;   // FAULT
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

// ---------------------------------------------------------------------------
// Motor timeout — IDLE'de yok sayılır, diğer durumlarda critical.
// ---------------------------------------------------------------------------
void test_motor_timeout_in_idle_is_safe(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_motorTimeoutActive = true;
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::IDLE));
}

void test_motor_timeout_in_ready_is_critical(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_motorTimeoutActive = true;
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

void test_motor_timeout_in_drive_is_critical(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_motorTimeoutActive = true;
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::DRIVE));
}

// ---------------------------------------------------------------------------
// BMS data invalid — eşik kontrolü yapılmaz, motor error hâlâ critical.
// ---------------------------------------------------------------------------
void test_warning_bms_invalid_skips_thresholds(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsDataValid = false;
    d.TEL_bmsTempHighestC = 99;
    d.TEL_bmsPackVoltageDeciV = 500;
    d.TEL_bmsCurrentCentiMa = -2500000;
    TEST_ASSERT_FALSE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
}

void test_critical_bms_invalid_with_motor_error_still_critical(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsDataValid = false;
    d.TEL_motorErrorFlags = 0x04;
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

// ---------------------------------------------------------------------------
// Baseline — temiz veri hiçbir koşul tetiklemez.
// ---------------------------------------------------------------------------
void test_baseline_clean_data_no_conditions(void) {
    TelemetryData d = makeTelemetryDataValid();
    TEST_ASSERT_FALSE(hasWarningCondition(d));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::IDLE));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::READY));
    TEST_ASSERT_FALSE(hasCriticalCondition(d, VcuState::DRIVE));
}

// ---------------------------------------------------------------------------
// Akım eşikleri uçtan uca BMS verisi içinden de doğrulansın.
// ---------------------------------------------------------------------------
void test_critical_via_charge_current(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsCurrentCentiMa = 120000;   // 1.2 A — kritik eşiğin üstünde
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}

void test_critical_via_discharge_current(void) {
    TelemetryData d = makeTelemetryDataValid();
    d.TEL_bmsCurrentCentiMa = -1600000;  // 16 A — kritik eşiğin üstünde
    TEST_ASSERT_TRUE(hasCriticalCondition(d, VcuState::READY));
}
