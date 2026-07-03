#include <unity.h>
#include "LoraRxHandler.h"

// ---------------------------------------------------------------------------
// 0xB0 (UKS_HEARTBEAT_BYTE) HEARTBEAT olarak siniflandirilir — bit degismedi
// ---------------------------------------------------------------------------
void test_heartbeat_byte_is_classified_as_heartbeat(void) {
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0xB0) == LoraRxByteKind::HEARTBEAT);
}

// ---------------------------------------------------------------------------
// 9.2.a: eski komut byte'lari (0xA1-0xA4) artik ozel islenmiyor, UNKNOWN
// ---------------------------------------------------------------------------
void test_former_command_bytes_are_classified_as_unknown(void) {
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0xA1) == LoraRxByteKind::UNKNOWN);
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0xA2) == LoraRxByteKind::UNKNOWN);
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0xA3) == LoraRxByteKind::UNKNOWN);
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0xA4) == LoraRxByteKind::UNKNOWN);
}

// ---------------------------------------------------------------------------
// Rastgele diger byte'lar da UNKNOWN (RF gurultusu)
// ---------------------------------------------------------------------------
void test_random_bytes_are_classified_as_unknown(void) {
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0x00) == LoraRxByteKind::UNKNOWN);
    TEST_ASSERT_TRUE(lora_classify_rx_byte(0xFF) == LoraRxByteKind::UNKNOWN);
}

// ---------------------------------------------------------------------------
// Bilinmeyen byte geldiginde sayac her zaman artar
// ---------------------------------------------------------------------------
void test_unknown_byte_always_increments_count(void) {
    uint32_t count = 0u;
    uint64_t lastWarn = 0u;

    lora_note_unknown_byte(1000u, &count, &lastWarn, 10000u);
    TEST_ASSERT_EQUAL_UINT32(1u, count);

    lora_note_unknown_byte(2000u, &count, &lastWarn, 10000u);
    TEST_ASSERT_EQUAL_UINT32(2u, count);
}

// ---------------------------------------------------------------------------
// Ilk cagri: now_ms - last_warn_ms(=0) >= interval ise WARN verilir
// ---------------------------------------------------------------------------
void test_unknown_byte_warns_when_interval_elapsed_since_zero(void) {
    uint32_t count = 0u;
    uint64_t lastWarn = 0u;

    TEST_ASSERT_TRUE(lora_note_unknown_byte(10000u, &count, &lastWarn, 10000u));
    TEST_ASSERT_EQUAL_UINT64(10000u, lastWarn);
}

// ---------------------------------------------------------------------------
// Throttle: interval icinde ikinci bilinmeyen byte WARN vermez (log spam yok)
// ---------------------------------------------------------------------------
void test_unknown_byte_within_interval_does_not_warn_again(void) {
    uint32_t count = 0u;
    uint64_t lastWarn = 10000u;

    TEST_ASSERT_FALSE(lora_note_unknown_byte(15000u, &count, &lastWarn, 10000u));
    // lastWarn throttle nedeniyle guncellenmedi
    TEST_ASSERT_EQUAL_UINT64(10000u, lastWarn);
}

// ---------------------------------------------------------------------------
// Interval tekrar dolunca WARN yeniden verilir
// ---------------------------------------------------------------------------
void test_unknown_byte_warns_again_after_interval(void) {
    uint32_t count = 0u;
    uint64_t lastWarn = 10000u;

    TEST_ASSERT_TRUE(lora_note_unknown_byte(20000u, &count, &lastWarn, 10000u));
    TEST_ASSERT_EQUAL_UINT64(20000u, lastWarn);
}
