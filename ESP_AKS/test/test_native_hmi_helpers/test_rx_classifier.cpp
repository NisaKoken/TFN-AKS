#include <unity.h>

#include "HMIHelpers.h"

// =========================================================================
// RX sınıflandırıcısı — dokunmatik komut (0x5A cmd ~cmd) ile Nextion sistem
// yanıtları (0x88 reset, 00 00 00 FF FF FF açılış) KARIŞMAMALI (§9.4.a.vii,
// Prompt 3 madde 3). HMI_rxClassifyByte saf/donanımsız olduğundan native
// test edilir.
// =========================================================================

namespace {

// Bir byte dizisini besler; son üretilen olayı ve (touch ise) komutu döner.
// Ara olaylar sayılır (reset/touch adedi) — karışma testleri için.
struct FeedResult {
    HMI_RxEvent lastEvent = HMI_RxEvent::NONE;
    uint8_t lastCmd = 0;
    int touchCount = 0;
    int resetCount = 0;
};

FeedResult feed(const uint8_t* bytes, size_t n) {
    HMI_RxState st{};
    FeedResult r{};
    for (size_t i = 0; i < n; ++i) {
        uint8_t cmd = 0;
        HMI_RxEvent ev = HMI_rxClassifyByte(st, bytes[i], cmd);
        r.lastEvent = ev;
        if (ev == HMI_RxEvent::TOUCH) {
            r.touchCount++;
            r.lastCmd = cmd;
        } else if (ev == HMI_RxEvent::NEXTION_RESET) {
            r.resetCount++;
        }
    }
    return r;
}

}  // namespace

// --- Dokunmatik komutlar 1..4 doğru çözülür ---

void test_rx_touch_start_cmd1(void) {
    const uint8_t f[] = {0x5A, 0x01, 0xFE};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL(HMI_RxEvent::TOUCH, r.lastEvent);
    TEST_ASSERT_EQUAL_UINT8(1, r.lastCmd);
    TEST_ASSERT_EQUAL_INT(1, r.touchCount);
    TEST_ASSERT_EQUAL_INT(0, r.resetCount);
}

void test_rx_touch_all_four_commands(void) {
    const uint8_t c1[] = {0x5A, 0x01, 0xFE};
    const uint8_t c2[] = {0x5A, 0x02, 0xFD};
    const uint8_t c3[] = {0x5A, 0x03, 0xFC};
    const uint8_t c4[] = {0x5A, 0x04, 0xFB};
    TEST_ASSERT_EQUAL_UINT8(1, feed(c1, sizeof(c1)).lastCmd);
    TEST_ASSERT_EQUAL_UINT8(2, feed(c2, sizeof(c2)).lastCmd);
    TEST_ASSERT_EQUAL_UINT8(3, feed(c3, sizeof(c3)).lastCmd);
    TEST_ASSERT_EQUAL_UINT8(4, feed(c4, sizeof(c4)).lastCmd);
}

void test_rx_touch_checksum_mismatch_no_command(void) {
    const uint8_t f[] = {0x5A, 0x01, 0x00};  // ~1 = 0xFE, gelen 0x00 → reddet
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL_INT(0, r.touchCount);
}

// --- Nextion sistem yanıtları TOUCH üretmez, RESET üretir ---

void test_rx_0x88_is_reset_not_touch(void) {
    const uint8_t f[] = {0x88};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL(HMI_RxEvent::NEXTION_RESET, r.lastEvent);
    TEST_ASSERT_EQUAL_INT(1, r.resetCount);
    TEST_ASSERT_EQUAL_INT(0, r.touchCount);
}

void test_rx_startup_sequence_is_reset(void) {
    const uint8_t f[] = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL_INT(1, r.resetCount);
    TEST_ASSERT_EQUAL_INT(0, r.touchCount);
}

void test_rx_startup_with_extra_leading_zeros(void) {
    // Fazladan öndeki sıfırlar tolere edilmeli
    const uint8_t f[] = {0x00, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
    TEST_ASSERT_EQUAL_INT(1, feed(f, sizeof(f)).resetCount);
}

// --- Yanlış-pozitif olmamalı ---

void test_rx_bare_ff_run_no_reset(void) {
    // 3 sıfır gelmeden FF FF FF → açılış değil, reset YOK
    const uint8_t f[] = {0xFF, 0xFF, 0xFF};
    TEST_ASSERT_EQUAL_INT(0, feed(f, sizeof(f)).resetCount);
}

void test_rx_two_zeros_then_ff_no_reset(void) {
    const uint8_t f[] = {0x00, 0x00, 0xFF, 0xFF, 0xFF};  // yalnız 2 sıfır
    TEST_ASSERT_EQUAL_INT(0, feed(f, sizeof(f)).resetCount);
}

void test_rx_broken_startup_pattern_no_reset(void) {
    // 00 00 00 FF 00 FF FF FF → araya 0x00 girince örüntü kırılır
    const uint8_t f[] = {0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF, 0xFF, 0xFF};
    TEST_ASSERT_EQUAL_INT(0, feed(f, sizeof(f)).resetCount);
}

// --- KARIŞMA testleri: sistem byte'ları touch'ı bozmaz, touch reset üretmez ---

void test_rx_reset_then_touch_both_seen(void) {
    // 0x88 (reset) hemen ardından geçerli touch → ikisi de ayrı ayrı görülür
    const uint8_t f[] = {0x88, 0x5A, 0x02, 0xFD};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL_INT(1, r.resetCount);
    TEST_ASSERT_EQUAL_INT(1, r.touchCount);
    TEST_ASSERT_EQUAL_UINT8(2, r.lastCmd);
}

void test_rx_startup_then_touch_both_seen(void) {
    // 00 00 00 FF FF FF (reset) + 5A 03 FC (touch) → reset sonra touch cmd 3
    const uint8_t f[] = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0x5A, 0x03, 0xFC};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL_INT(1, r.resetCount);
    TEST_ASSERT_EQUAL_INT(1, r.touchCount);
    TEST_ASSERT_EQUAL_UINT8(3, r.lastCmd);
}

void test_rx_stray_ff_before_touch_does_not_corrupt(void) {
    // Nextion artık FF, ardından geçerli touch → touch yine çözülür
    const uint8_t f[] = {0xFF, 0x5A, 0x04, 0xFB};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL_INT(1, r.touchCount);
    TEST_ASSERT_EQUAL_UINT8(4, r.lastCmd);
    TEST_ASSERT_EQUAL_INT(0, r.resetCount);
}

void test_rx_touch_bytes_never_trigger_reset(void) {
    // Arka arkaya tüm touch komutları hiç reset üretmemeli
    const uint8_t f[] = {0x5A, 0x01, 0xFE, 0x5A, 0x02, 0xFD,
                         0x5A, 0x03, 0xFC, 0x5A, 0x04, 0xFB};
    FeedResult r = feed(f, sizeof(f));
    TEST_ASSERT_EQUAL_INT(0, r.resetCount);
    TEST_ASSERT_EQUAL_INT(4, r.touchCount);
}
