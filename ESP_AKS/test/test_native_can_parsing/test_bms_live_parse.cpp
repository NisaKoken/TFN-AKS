#include <unity.h>

#include "CanParse.h"

// Solion SK BMS — CAN ID 0x112 (parseSolionBmsB)
// Frame layout (Big Endian):
//   [0-1] Pack Voltage     uint16  0.1 V
//   [2-5] Pack Current     int32   0.01 mA  (+charge / -discharge)
//   [6-7] SOC              uint16  0.01 %

namespace {

twai_message_t makeBmsBMsg(uint8_t dlc,
                            uint8_t pv_hi,  uint8_t pv_lo,
                            uint8_t c3,     uint8_t c2,
                            uint8_t c1,     uint8_t c0,
                            uint8_t soc_hi, uint8_t soc_lo) {
    twai_message_t m{};
    m.identifier = 0x112;
    m.data_length_code = dlc;
    m.data[0] = pv_hi;
    m.data[1] = pv_lo;
    m.data[2] = c3;
    m.data[3] = c2;
    m.data[4] = c1;
    m.data[5] = c0;
    m.data[6] = soc_hi;
    m.data[7] = soc_lo;
    return m;
}

}  // namespace

void test_bms_live_dlc_too_short(void) {
    twai_message_t m = makeBmsBMsg(7, 0x02, 0x0E, 0xFF, 0xFD, 0x3A, 0x96, 0x18, 0x8B);
    TelemetryData out{};
    TEST_ASSERT_FALSE(CanParse::parseSolionBmsB(m, out));
    TEST_ASSERT_FALSE(out.TEL_bmsDataValid);
}

void test_bms_live_pack_voltage_big_endian(void) {
    // Doc example: 0x020E = 526 → 52.6 V
    twai_message_t m = makeBmsBMsg(8, 0x02, 0x0E, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseSolionBmsB(m, out));
    TEST_ASSERT_EQUAL_UINT16(0x020E, out.TEL_bmsPackVoltageDeciV);
}

void test_bms_live_current_negative(void) {
    // Doc example: 0xFFFD3A96 = -181610 centi-mA = -1816.10 mA (deşarj)
    twai_message_t m = makeBmsBMsg(8, 0x02, 0x0E, 0xFF, 0xFD, 0x3A, 0x96, 0x18, 0x8B);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_INT32(-181610, out.TEL_bmsCurrentCentiMa);
}

void test_bms_live_current_positive(void) {
    // 0x0002BF20 = 180000 centi-mA = 1800 mA = 1.8 A (şarj)
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0x00, 0x02, 0xBF, 0x20, 0x00, 0x00);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_INT32(180000, out.TEL_bmsCurrentCentiMa);
}

void test_bms_live_current_zero(void) {
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_INT32(0, out.TEL_bmsCurrentCentiMa);
}

void test_bms_live_soc_big_endian(void) {
    // Doc example: 0x188B = 6283 → %62.83
    twai_message_t m = makeBmsBMsg(8, 0x02, 0x0E, 0xFF, 0xFD, 0x3A, 0x96, 0x18, 0x8B);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_UINT16(0x188B, out.TEL_bmsSocHundredths);
}

void test_bms_live_soc_full(void) {
    // %100.00 = 10000 = 0x2710
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x27, 0x10);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_UINT16(10000, out.TEL_bmsSocHundredths);
}

void test_bms_live_sets_valid_flag(void) {
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    out.TEL_bmsDataValid = false;
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_TRUE(out.TEL_bmsDataValid);
}

void test_bms_live_error_flags(void) {
    // parseSolionBmsB does not exist in CAN ID 0x112 — system state is in BMS-A.
    // This test is intentionally absent.
}

void test_bms_live_current_signed_minus_one(void) {
    // 0xFFFFFFFF = -1 centi-mA
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF, 0x00, 0x00);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_INT32(-1, out.TEL_bmsCurrentCentiMa);
}

void test_bms_live_current_signed_min(void) {
    // 0x80000000 = INT32_MIN
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_INT32(INT32_MIN, out.TEL_bmsCurrentCentiMa);
}

void test_bms_live_current_signed_max(void) {
    // 0x7FFFFFFF = INT32_MAX
    twai_message_t m = makeBmsBMsg(8, 0x00, 0x00, 0x7F, 0xFF, 0xFF, 0xFF, 0x00, 0x00);
    TelemetryData out{};
    CanParse::parseSolionBmsB(m, out);
    TEST_ASSERT_EQUAL_INT32(INT32_MAX, out.TEL_bmsCurrentCentiMa);
}

void test_bms_live_preserves_other_fields(void) {
    twai_message_t m = makeBmsBMsg(8, 0x03, 0x10, 0x00, 0x00, 0x00, 0x00, 0x18, 0x8B);
    TelemetryData out{};
    out.TEL_motorRpm = 500;
    out.TEL_bmsCellVoltageMaxDeciMv = 40000;
    out.TEL_bmsTempHighestC = 25;
    out.TEL_bmsSystemState = 2;

    CanParse::parseSolionBmsB(m, out);

    TEST_ASSERT_EQUAL_UINT16(500, out.TEL_motorRpm);
    TEST_ASSERT_EQUAL_UINT16(40000, out.TEL_bmsCellVoltageMaxDeciMv);
    TEST_ASSERT_EQUAL_INT8(25, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_UINT8(2, out.TEL_bmsSystemState);
}
