#include <unity.h>

#include <cstring>

#include "BmsNextionPacket.h"
#include "fake_nextion_emit.h"

// =========================================================================
// cellcan göstergesi (madde 2) — per-cell CAN doğrulama durumu.
//   - cellDataVerified=false → cellcan.val=0 ("CAN doğrulanmadı")
//   - cellDataVerified=true  → cellcan.val=1
//   - warn'dan BAĞIMSIZ (madde 3 otorite kuralı)
//   - doğrulanmamışken sentinel hücreler → tüm bar (j*) 0
// =========================================================================

namespace {

// Tüm hücreleri verilen mV ile dolduran yardımcı.
void fillCells(BmsPackData& raw, uint16_t mv) {
    raw.isValid = true;
    for (int i = 0; i < BMS_CELL_COUNT; ++i) raw.cellVoltageMv[i] = mv;
}

}  // namespace

void test_cellcan_zero_when_unverified(void) {
    fake_nextion_reset();
    BmsPackData raw{};
    fillCells(raw, 65535);  // sentinel — doğrulanmamış
    BmsComputed comp{};
    comp.cellMaxMv = 65535;
    comp.cellMinMv = 65535;
    comp.warningLevel = 0;
    BmsNextionCache cache{};

    // 9. arg cellDataVerified = false
    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            true, true, 2000, false);

    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("cellcan.val=0"),
                                 "cellcan.val=0 bekleniyordu (doğrulanmadı)");
}

void test_cellcan_one_when_verified(void) {
    fake_nextion_reset();
    BmsPackData raw{};
    fillCells(raw, 3300);
    BmsComputed comp{};
    comp.cellMaxMv = 3300;
    comp.cellMinMv = 3300;
    comp.warningLevel = 0;
    BmsNextionCache cache{};

    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            true, true, 2000, true);

    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("cellcan.val=1"),
                                 "cellcan.val=1 bekleniyordu (doğrulandı)");
}

void test_cellcan_default_arg_is_zero(void) {
    // 9. argüman verilmezse varsayılan false → cellcan=0 (ŞU ANKİ durum)
    fake_nextion_reset();
    BmsPackData raw{};
    fillCells(raw, 3300);
    BmsComputed comp{};
    comp.cellMaxMv = 3300;
    comp.cellMinMv = 3300;
    comp.warningLevel = 0;
    BmsNextionCache cache{};

    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            true, true, 2000);  // cellDataVerified defaultu

    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("cellcan.val=0"),
                                 "default cellDataVerified=false → cellcan.val=0");
}

void test_cellcan_change_cached(void) {
    BmsPackData raw{};
    fillCells(raw, 3300);
    BmsComputed comp{};
    comp.cellMaxMv = 3300;
    comp.cellMinMv = 3300;
    comp.warningLevel = 0;
    BmsNextionCache cache{};

    // Tick 1: force, unverified → cellcan=0, cache ısınır
    fake_nextion_reset();
    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            true, true, 2000, false);
    TEST_ASSERT_TRUE(cache.isWarm);

    // Tick 2: force yok, updateCells yok, hâlâ unverified → cellcan DEĞİŞMEDİ,
    // tekrar emit EDİLMEMELİ.
    fake_nextion_reset();
    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            false, false, 2000, false);
    TEST_ASSERT_NULL_MESSAGE(fake_nextion_find("cellcan.val="),
                             "cellcan değişmediyse tekrar emit edilmemeli");

    // Tick 3: doğrulama açıldı (false→true) → cellcan=1 emit edilmeli.
    fake_nextion_reset();
    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            false, false, 2000, true);
    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("cellcan.val=1"),
                                 "false→true geçişinde cellcan.val=1 emit edilmeli");
}

void test_unverified_sentinel_cells_all_bars_zero(void) {
    // Doğrulanmamış durumda hücreler sentinel → tüm j*.val=0 (boş bar)
    fake_nextion_reset();
    BmsPackData raw{};
    fillCells(raw, 65535);
    BmsComputed comp{};
    comp.cellMaxMv = 65535;
    comp.cellMinMv = 65535;
    comp.warningLevel = 0;
    BmsNextionCache cache{};

    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            true, true, 2000, false);

    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("j0.val=0"), "j0 boş bar olmalı");
    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("j23.val=0"), "j23 boş bar olmalı");
    // Dolu bar sızmamalı
    TEST_ASSERT_NULL_MESSAGE(fake_nextion_find("j0.val=100"),
                             "sentinel hücrede dolu bar OLMAMALI");
}

void test_cellcan_independent_of_warn(void) {
    // warn=CRITICAL (gerçek hücre uyarısı) ile cellcan=0 (veri yok) AYNI anda
    // ve BİRBİRİNDEN BAĞIMSIZ görünebilmeli (otorite kuralı, madde 3).
    fake_nextion_reset();
    BmsPackData raw{};
    fillCells(raw, 3300);
    BmsComputed comp{};
    comp.cellMaxMv = 3300;
    comp.cellMinMv = 3300;
    comp.warningLevel = 2;  // CRITICAL (gösterim)
    BmsNextionCache cache{};

    buildBmsNextionCommands(comp, raw, fake_nextion_capture, nullptr, cache,
                            true, true, 2000, false);

    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("warn.val=2"),
                                 "warn=2 comp'tan olduğu gibi basılmalı");
    TEST_ASSERT_NOT_NULL_MESSAGE(fake_nextion_find("cellcan.val=0"),
                                 "cellcan=0 warn'dan bağımsız basılmalı");
}
