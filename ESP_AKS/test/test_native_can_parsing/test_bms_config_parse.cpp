#include <unity.h>

#include "CanParse.h"

// =========================================================================
// Lithium Balance c-BMS — CAN ID 0xE000 ve 0xE001 Doğrulanmış Testleri
// =========================================================================

namespace {

twai_message_t makeE000Msg(uint8_t dlc,
                           uint8_t b0, uint8_t b1,
                           uint8_t pv_hi, uint8_t pv_lo,
                           uint8_t soc_hi, uint8_t soc_lo,
                           uint8_t b6, uint8_t b7) {
    twai_message_t m{};
    m.identifier = 0x0000E000;
    m.data_length_code = dlc;
    m.data[0] = b0;
    m.data[1] = b1;
    m.data[2] = pv_hi;
    m.data[3] = pv_lo;
    m.data[4] = soc_hi;
    m.data[5] = soc_lo;
    if (dlc > 6) m.data[6] = b6;
    if (dlc > 7) m.data[7] = b7;
    return m;
}

twai_message_t makeE001Msg(uint8_t dlc, uint8_t t1, uint8_t t2) {
    twai_message_t m{};
    m.identifier = 0x0000E001;
    m.data_length_code = dlc;
    if (dlc > 6) m.data[6] = t1;
    if (dlc > 7) m.data[7] = t2;
    return m;
}

}  // namespace

void test_e000_dlc_too_short(void) {
    // DLC < 8 → false, hiçbir alan yazılmamalı
    twai_message_t m = makeE000Msg(7, 0x00, 0x00, 0x02, 0x0E, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_FALSE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_FALSE(out.TEL_bmsDataValid);
}

void test_e000_parsing_nominal(void) {
    // Akım: 0x00 0x0A (10) -> 10 * 0.1A = 1A -> 100 CentiMa
    // Voltaj: 0x02 0x0E (526) -> 52.6V -> 526 deciV
    // SoC: 0x27 0x10 (10000) -> 100.00% -> 10000 hundredths
    twai_message_t m = makeE000Msg(8, 0x00, 0x0A, 0x02, 0x0E, 0x27, 0x10, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_INT32(100, out.TEL_bmsCurrentCentiMa);
    TEST_ASSERT_EQUAL_UINT16(526, out.TEL_bmsPackVoltageDeciV);
    TEST_ASSERT_EQUAL_UINT16(10000, out.TEL_bmsSocHundredths);
    TEST_ASSERT_TRUE(out.TEL_bmsDataValid);
}

void test_e000_parsing_negative_current(void) {
    // Akım: 0xFF 0xF6 (-10) -> -1A -> -100 CentiMa
    twai_message_t m = makeE000Msg(8, 0xFF, 0xF6, 0x02, 0x0E, 0x27, 0x10, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_INT32(-100, out.TEL_bmsCurrentCentiMa);
}

void test_e000_preserves_other_fields(void) {
    twai_message_t m = makeE000Msg(8, 0x00, 0x00, 0x03, 0x10, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    out.TEL_motorRpm = 1234;
    out.TEL_bmsCellVoltageMaxDeciMv = 40000;
    out.TEL_bmsTempHighestC = 25;
    out.TEL_bmsSystemState = 2;

    CanParse::parseLbBmsE000(m, out);

    TEST_ASSERT_EQUAL_UINT16(1234, out.TEL_motorRpm);
    TEST_ASSERT_EQUAL_UINT16(40000, out.TEL_bmsCellVoltageMaxDeciMv);
    TEST_ASSERT_EQUAL_INT8(25, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_UINT8(2, out.TEL_bmsSystemState);
}

void test_e001_dlc_too_short(void) {
    twai_message_t m = makeE001Msg(7, 20, 25);
    TelemetryData out{};
    TEST_ASSERT_FALSE(CanParse::parseLbBmsE001(m, out));
}

void test_e001_parsing_temps(void) {
    // b6=40 (0x28), b7=-5 (0xFB): max=40 -> Highest, min=-5 -> Lowest
    twai_message_t m = makeE001Msg(8, 0x28, 0xFB);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE001(m, out));
    TEST_ASSERT_EQUAL_INT8(40, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_INT8(-5, out.TEL_bmsTempLowestC);
}

// --- E001 max() semantiği (B3 §9.2.c.ii "en yüksek olan") ---
// Byte sırasına körü körüne güvenilmez: b7 > b6 olduğunda Highest = b7 olmalı.

void test_e001_max_picks_higher_when_b7_greater(void) {
    // b6=10, b7=30 -> Highest=30 (b7), Lowest=10 (b6)
    twai_message_t m = makeE001Msg(8, 0x0A, 0x1E);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE001(m, out));
    TEST_ASSERT_EQUAL_INT8(30, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_INT8(10, out.TEL_bmsTempLowestC);
}

void test_e001_max_both_negative(void) {
    // b6=-20 (0xEC), b7=-5 (0xFB) -> Highest=-5, Lowest=-20
    twai_message_t m = makeE001Msg(8, 0xEC, 0xFB);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE001(m, out));
    TEST_ASSERT_EQUAL_INT8(-5, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_INT8(-20, out.TEL_bmsTempLowestC);
}

void test_e001_dlc_boundary_8_ok(void) {
    // Sınır: DLC=8 tam kabul edilir (min geçerli)
    twai_message_t m = makeE001Msg(8, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE001(m, out));
    TEST_ASSERT_EQUAL_INT8(0, out.TEL_bmsTempHighestC);
    TEST_ASSERT_EQUAL_INT8(0, out.TEL_bmsTempLowestC);
}

// --- E000 akım (HIPOTEZ ölçek raw*10) — nominal/sınır/bozuk ---

void test_e000_current_hypothesis_scale_positive(void) {
    // b0:b1 = 0x00 0x64 (100) -> raw*10 = 1000 CentiMa (ölçek HIPOTEZ)
    twai_message_t m = makeE000Msg(8, 0x00, 0x64, 0x02, 0x0E, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_INT32(1000, out.TEL_bmsCurrentCentiMa);
}

void test_e000_current_boundary_int16_min(void) {
    // b0:b1 = 0x80 0x00 (INT16_MIN = -32768) -> -327680 CentiMa
    twai_message_t m = makeE000Msg(8, 0x80, 0x00, 0x02, 0x0E, 0x00, 0x00, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_INT32(-327680, out.TEL_bmsCurrentCentiMa);
}

// --- E000 SoC (HIPOTEZ anlam) — nominal/sınır/bozuk ---

void test_e000_soc_hypothesis_nominal(void) {
    // b4:b5 = 0x13 0x88 (5000) -> %50.00 (ölçek HIPOTEZ)
    twai_message_t m = makeE000Msg(8, 0x00, 0x00, 0x02, 0x0E, 0x13, 0x88, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_UINT16(5000, out.TEL_bmsSocHundredths);
}

void test_e000_soc_boundary_max_uint16(void) {
    // b4:b5 = 0xFF 0xFF (65535) -> parse ham değeri döndürür (clamp sanitize'da)
    twai_message_t m = makeE000Msg(8, 0x00, 0x00, 0x02, 0x0E, 0xFF, 0xFF, 0x00, 0x00);
    TelemetryData out{};
    TEST_ASSERT_TRUE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_UINT16(65535, out.TEL_bmsSocHundredths);
}

void test_e000_dlc6_rejects_current_and_soc_untouched(void) {
    // Bozuk frame: DLC=6 (< 8) -> false, hiçbir alan yazılmamalı
    twai_message_t m = makeE000Msg(6, 0x00, 0x64, 0x02, 0x0E, 0x13, 0x88, 0x00, 0x00);
    TelemetryData out{};
    out.TEL_bmsCurrentCentiMa = 42;
    out.TEL_bmsSocHundredths = 7;
    TEST_ASSERT_FALSE(CanParse::parseLbBmsE000(m, out));
    TEST_ASSERT_EQUAL_INT32(42, out.TEL_bmsCurrentCentiMa);
    TEST_ASSERT_EQUAL_UINT16(7, out.TEL_bmsSocHundredths);
    TEST_ASSERT_FALSE(out.TEL_bmsDataValid);
}

int main(int argc, char** argv) {
    UNITY_BEGIN();
    RUN_TEST(test_e000_dlc_too_short);
    RUN_TEST(test_e000_parsing_nominal);
    RUN_TEST(test_e000_parsing_negative_current);
    RUN_TEST(test_e000_preserves_other_fields);
    RUN_TEST(test_e001_dlc_too_short);
    RUN_TEST(test_e001_parsing_temps);
    // Yeni: E001 max() semantiği
    RUN_TEST(test_e001_max_picks_higher_when_b7_greater);
    RUN_TEST(test_e001_max_both_negative);
    RUN_TEST(test_e001_dlc_boundary_8_ok);
    // Yeni: E000 akım (HIPOTEZ)
    RUN_TEST(test_e000_current_hypothesis_scale_positive);
    RUN_TEST(test_e000_current_boundary_int16_min);
    // Yeni: E000 SoC (HIPOTEZ)
    RUN_TEST(test_e000_soc_hypothesis_nominal);
    RUN_TEST(test_e000_soc_boundary_max_uint16);
    RUN_TEST(test_e000_dlc6_rejects_current_and_soc_untouched);
    return UNITY_END();
}
