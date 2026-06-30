#include <unity.h>

#include "CanParse.h"

// Solion SK BMS — CAN ID 0x111 (parseSolionBmsA)
// Frame layout (Big Endian):
//   [0-1] Cell Voltage Max  uint16  0.1 mV
//   [2-3] Cell Voltage Min  uint16  0.1 mV
//   [4]   Highest Cell Temp int8    1 °C
//   [5]   Lowest Cell Temp  int8    1 °C
//   [6]   System State      uint8   1=Discharge 2=IDLE 3=Charge 4=FAULT

namespace {

twai_message_t makeBmsAMsg(uint8_t dlc,
                            uint8_t vmax_hi, uint8_t vmax_lo,
                            uint8_t vmin_hi, uint8_t vmin_lo,
                            uint8_t temp_high, uint8_t temp_low,
                            uint8_t state) {
    twai_message_t m{};
    m.identifier = 0x111;
    m.data_length_code = dlc;
    m.data[0] = vmax_hi;
    m.data[1] = vmax_lo;
    m.data[2] = vmin_hi;
    m.data[3] = vmin_lo;
    m.data[4] = temp_high;
    m.data[5] = temp_low;
    m.data[6] = state;
    return m;
}

}  // namespace

void test_bms_config_dlc_too_short(void) {
    twai_message_t m = makeBmsAMsg(6, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x02);
    TelemetryData out{};
    TEST_ASSERT_FALSE(CanParse::parseSolionBmsA(m, out));
    TEST_ASSERT_FALSE(out.TEL_bmsDataValid);
}

void test_bms_config_pack_voltage_big_endian(void) {
    // Doc example: 0x9366 = 37734 → 3773.4 mV
    twai_message_t m = makeBmsAMsg(8, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x02);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseSolionBmsA(m, out));
    TEST_ASSERT_EQUAL_UINT16(0x9366, out.TEL_bmsCellVoltageMaxDeciMv);
}

void test_bms_config_cell_voltage_big_endian(void) {
    // Doc example: 0x922E = 37422 → 3742.2 mV
    twai_message_t m = makeBmsAMsg(8, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x02);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseSolionBmsA(m, out));
    TEST_ASSERT_EQUAL_UINT16(0x922E, out.TEL_bmsCellVoltageMinDeciMv);
}

void test_bms_config_temp_highest_signed(void) {
    // 0x20 = 32 °C
    twai_message_t m = makeBmsAMsg(8, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x02);
    TelemetryData out{};
    CanParse::parseSolionBmsA(m, out);
    TEST_ASSERT_EQUAL_INT8(32, out.TEL_bmsTempHighestC);
}

void test_bms_config_temp_lowest_signed(void) {
    // 0x1F = 31 °C
    twai_message_t m = makeBmsAMsg(8, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x02);
    TelemetryData out{};
    CanParse::parseSolionBmsA(m, out);
    TEST_ASSERT_EQUAL_INT8(31, out.TEL_bmsTempLowestC);
}

void test_bms_config_temp_negative(void) {
    // 0xEC = -20 as int8
    twai_message_t m = makeBmsAMsg(8, 0x00, 0x00, 0x00, 0x00, 0xEC, 0xEC, 0x02);
    TelemetryData out{};
    CanParse::parseSolionBmsA(m, out);
    TEST_ASSERT_EQUAL_INT8(-20, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_INT8(-20, out.TEL_bmsTempLowestC);
}

void test_bms_config_system_state(void) {
    // 0x03 = Şarj
    twai_message_t m = makeBmsAMsg(8, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x03);
    TelemetryData out{};
    CanParse::parseSolionBmsA(m, out);
    TEST_ASSERT_EQUAL_UINT8(3, out.TEL_bmsSystemState);
}

void test_bms_config_system_state_fault(void) {
    twai_message_t m = makeBmsAMsg(8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04);
    TelemetryData out{};
    CanParse::parseSolionBmsA(m, out);
    TEST_ASSERT_EQUAL_UINT8(4, out.TEL_bmsSystemState);
}

void test_bms_config_sets_valid_flag(void) {
    twai_message_t m = makeBmsAMsg(8, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02);
    TelemetryData out{};
    out.TEL_bmsDataValid = false;
    TEST_ASSERT_TRUE(CanParse::parseSolionBmsA(m, out));
    TEST_ASSERT_TRUE(out.TEL_bmsDataValid);
}

void test_bms_config_preserves_other_fields(void) {
    twai_message_t m = makeBmsAMsg(8, 0x93, 0x66, 0x92, 0x2E, 0x20, 0x1F, 0x02);
    TelemetryData out{};
    out.TEL_motorRpm = 1234;
    out.TEL_bmsPackVoltageDeciV = 800;
    out.TEL_bmsCurrentCentiMa = -50000;
    out.TEL_bmsSocHundredths = 7500;

    CanParse::parseSolionBmsA(m, out);

    TEST_ASSERT_EQUAL_UINT16(1234, out.TEL_motorRpm);
    TEST_ASSERT_EQUAL_UINT16(800, out.TEL_bmsPackVoltageDeciV);
    TEST_ASSERT_EQUAL_INT32(-50000, out.TEL_bmsCurrentCentiMa);
    TEST_ASSERT_EQUAL_UINT16(7500, out.TEL_bmsSocHundredths);
}
