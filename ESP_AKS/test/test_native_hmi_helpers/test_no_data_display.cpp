#include <unity.h>

#include "HMIHelpers.h"

// =========================================================================
// "Veri yok" gösterimi — HMI_batteryDisplayValue / HMI_temperatureDisplayValue
//
// Kaynak sinyaller (TEL_bmsSocHundredths, TEL_bmsTempHighestC) DOĞRULANMADI;
// doğrulanana kadar ekrana sahte %0/0°C yerine sentinel (255 / -127)
// gönderilir. Bu testler hem bugünkü "kaynak doğrulanmadı" yolunu hem de
// ileride kaynak doğrulandığında devreye girecek dönüşüm/clamp yolunu
// kilitler.
// =========================================================================

// --- Bugünkü üretim durumu: kaynak DOĞRULANMADI -> her zaman sentinel ---

void test_battery_unverified_source_returns_no_data(void) {
    // Değer ve bmsValid ne olursa olsun sentinel dönmeli
    TEST_ASSERT_EQUAL_UINT8(HMI_BATTERY_NO_DATA,
                            HMI_batteryDisplayValue(false, true, 8000));
    TEST_ASSERT_EQUAL_UINT8(HMI_BATTERY_NO_DATA,
                            HMI_batteryDisplayValue(false, false, 0));
}

void test_temp_unverified_source_returns_no_data(void) {
    TEST_ASSERT_EQUAL_INT16(HMI_TEMP_NO_DATA,
                            HMI_temperatureDisplayValue(false, true, 25));
    TEST_ASSERT_EQUAL_INT16(HMI_TEMP_NO_DATA,
                            HMI_temperatureDisplayValue(false, false, 0));
}

void test_production_source_verified_flags_are_false_pending_hw_verify(void) {
    // SoC (0xE000 b[4:5]) ve sıcaklık (0xE001 b[6:7]) PARSE ediliyor ama byte
    // anlamı/ölçeği HIPOTEZ (bkz. CAN_Message_Table.md) — Prompt 7 donanım
    // sniffer teyidine kadar bayraklar false; ekran sentinel ("--") gösterir.
    // Teyit yapıldığında bu test true bekleyecek şekilde güncellenecek.
    TEST_ASSERT_FALSE(HMI_SOC_SOURCE_VERIFIED);
    TEST_ASSERT_FALSE(HMI_TEMP_SOURCE_VERIFIED);
}

// --- bmsDataValid=false -> kaynak doğrulanmış olsa bile sentinel ---

void test_battery_invalid_bms_returns_no_data(void) {
    TEST_ASSERT_EQUAL_UINT8(HMI_BATTERY_NO_DATA,
                            HMI_batteryDisplayValue(true, false, 8000));
}

void test_temp_invalid_bms_returns_no_data(void) {
    TEST_ASSERT_EQUAL_INT16(HMI_TEMP_NO_DATA,
                            HMI_temperatureDisplayValue(true, false, 25));
}

// --- Gelecek yolu: kaynak doğrulanmış + veri taze -> gerçek dönüşüm ---

void test_battery_verified_valid_converts_hundredths_to_percent(void) {
    // 8000 hundredths = %80.00 -> 80
    TEST_ASSERT_EQUAL_UINT8(80, HMI_batteryDisplayValue(true, true, 8000));
    TEST_ASSERT_EQUAL_UINT8(0, HMI_batteryDisplayValue(true, true, 99));
    TEST_ASSERT_EQUAL_UINT8(100, HMI_batteryDisplayValue(true, true, 10000));
}

void test_battery_verified_valid_clamps_above_100(void) {
    // Bozuk/aralık dışı SOC sentinelle (255) çakışmamalı — 100'e clamp
    TEST_ASSERT_EQUAL_UINT8(100, HMI_batteryDisplayValue(true, true, 65535));
}

void test_temp_verified_valid_passes_through(void) {
    TEST_ASSERT_EQUAL_INT16(25, HMI_temperatureDisplayValue(true, true, 25));
    TEST_ASSERT_EQUAL_INT16(-10, HMI_temperatureDisplayValue(true, true, -10));
}

// =========================================================================
// §8.2.a.iv akım (packi) — gating + Prompt 1 sözleşmesi B (txt "--"/"±d.d")
// =========================================================================

// --- Akım gating (HMI_currentDisplayValue) ---

void test_current_unverified_source_returns_no_data(void) {
    TEST_ASSERT_EQUAL_INT16(HMI_CURRENT_NO_DATA,
                            HMI_currentDisplayValue(false, true, 125));
    TEST_ASSERT_EQUAL_INT16(HMI_CURRENT_NO_DATA,
                            HMI_currentDisplayValue(false, false, 0));
}

void test_current_invalid_bms_returns_no_data(void) {
    TEST_ASSERT_EQUAL_INT16(HMI_CURRENT_NO_DATA,
                            HMI_currentDisplayValue(true, false, 125));
}

void test_current_verified_valid_passes_through(void) {
    TEST_ASSERT_EQUAL_INT16(125, HMI_currentDisplayValue(true, true, 125));
    TEST_ASSERT_EQUAL_INT16(-80, HMI_currentDisplayValue(true, true, -80));
}

// --- Sentinel txt politikası B: sihirli sayı ekrana sızmamalı ---

void test_battery_text_sentinel_is_dashes(void) {
    char buf[8];
    HMI_formatBatteryText(HMI_BATTERY_NO_DATA, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("--", buf);
}

void test_battery_text_value(void) {
    char buf[8];
    HMI_formatBatteryText(87, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("87", buf);
    HMI_formatBatteryText(0, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("0", buf);
    HMI_formatBatteryText(100, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("100", buf);
}

void test_temp_text_sentinel_is_dashes(void) {
    char buf[8];
    HMI_formatTempText(HMI_TEMP_NO_DATA, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("--", buf);
}

void test_temp_text_value(void) {
    char buf[8];
    HMI_formatTempText(25, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("25", buf);
    HMI_formatTempText(-40, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("-40", buf);
}

void test_current_text_sentinel_is_dashes(void) {
    char buf[10];
    HMI_formatCurrentText(HMI_CURRENT_NO_DATA, buf, sizeof(buf));
    TEST_ASSERT_EQUAL_STRING("--", buf);
}

void test_current_text_positive_one_decimal(void) {
    char buf[10];
    HMI_formatCurrentText(125, buf, sizeof(buf));  // 12.5 A
    TEST_ASSERT_EQUAL_STRING("12.5", buf);
}

void test_current_text_negative_one_decimal(void) {
    char buf[10];
    HMI_formatCurrentText(-80, buf, sizeof(buf));  // -8.0 A
    TEST_ASSERT_EQUAL_STRING("-8.0", buf);
}

void test_current_text_sub_amp_and_zero(void) {
    char buf[10];
    HMI_formatCurrentText(5, buf, sizeof(buf));    // 0.5 A
    TEST_ASSERT_EQUAL_STRING("0.5", buf);
    HMI_formatCurrentText(-5, buf, sizeof(buf));   // -0.5 A
    TEST_ASSERT_EQUAL_STRING("-0.5", buf);
    HMI_formatCurrentText(0, buf, sizeof(buf));    // 0.0 A
    TEST_ASSERT_EQUAL_STRING("0.0", buf);
}
