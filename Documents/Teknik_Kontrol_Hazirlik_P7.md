# Teknik Kontrol Hazırlık — CAN Doğrulama + §8.2 / §9.4 Prova (Prompt 7)

Bu belge, sahada teknik kontrol öncesi yapılacak **CAN doğrulama prosedürünü**,
doğrulama geçtiğinde uygulanacak **tek-satır bayrak değişikliklerini** ve
şartname §8.2 / §9.4 için **prova senaryosunu** toplar. Önceki 6 aşamanın devir
notlarının kapanışıdır.

Durum özeti (bu belge yazıldığında): **yalnız pack gerilimi (0xE000 b[2:3])
DOĞRULANDI.** SoC, akım, sıcaklık, per-cell veriler HİPOTEZ → ekranda sentinel
("--" / cellcan=0) gösteriliyor, VCU kararına bağlı değil (Yol A, Prompt 6).

---

## 1. Saha CAN Doğrulama Prosedürü

Her sinyal için: **sniffer logu → beklenen ölçek → bağımsız ölçüm → ekran değeri**
üçlü karşılaştırması yapılır. Ölçek/anlam iki bağımsız gözlemde tutarlıysa
DOĞRULANDI sayılır (pack voltajında uygulanan kanıt seviyesi — bkz.
`Documents/CAN_Message_Table.md`).

### 1.1 Karşılaştırma Tablosu (doldurulacak)

| Sinyal | CAN ID · byte | Ham (sniffer) | Beklenen ölçek/formül | Beklenen fiziksel | Bağımsız referans (ölçüm) | Ekran değeri | GEÇME kriteri |
|---|---|---|---|---|---|---|---|
| **SoC** | `0xE000` b[4:5] | `0x____` | `raw × 0.01 = %` | %__ | Kalibre SoC / coulomb sayacı | `bat.txt` | Ekran = referans ± %5 **VE** yük altında birlikte değişiyor |
| **Sıcaklık** | `0xE001` b[6:7] | `0x__ 0x__` | `int8 = °C` (max(b6,b7)) | __ °C | Termokupl/termometre | `temp.txt` | Ekran = ölçüm ± 2 °C, 3 sıcaklıkta (buz/oda/ılık) |
| **Akım** | `0xE000` b[0:1] | `0x____` | **ölçek DOĞRULANMADI** (aday `int16 × ?`) | __ A | Clamp ampermetre (bilinen yük, ör. 5 A) | `packi.txt` (deciA) | Ekran işareti+değeri = clamp ± %5; **birim zinciri sabitlendi** |
| **Per-cell V** | `0xE002-E005` / `E032-E033` | çözülmedi | **anlam DOĞRULANMADI** | __ mV/hücre | Tek hücreyi değiştir, hangi ID/byte oynuyor gözle | `cellN` + `cellcan` | 24 hücre ↔ ID/byte eşlemesi netleşti |

### 1.2 Adım adım (sinyal başına)

1. **Sniffer logu al:** PeakCAN + LiBAL c-BMS CREATOR (ya da mevcut TWAI sniffer)
   ile ilgili ID'yi kaydet.
2. **Kontrollü uyaran uygula:** SoC için bilinen SoC'den şarj/deşarj; sıcaklık
   için sensörü bilinen sıcaklığa maruz bırak; akım için clamp ampermetreli bilinen
   yük; per-cell için tek hücreyi ayrı besle.
3. **Ham byte ↔ fiziksel değer oranını çıkar**, `CAN_Message_Table.md`'deki aday
   ölçekle karşılaştır. İki bağımsız gözlemde tutarlıysa DOĞRULANDI yaz.
4. **Akım birim zincirini sabitle:** `CanParse.cpp` yorumu "×0.1 A", alan adı
   "CentiMa", `SystemConfig.h` eşiği "centi-mA" — 1000× belirsizlik. Doğrulanan
   ölçeğe göre TEK birim seç, `main.cpp`'deki `TEL_bmsCurrentCentiMa / 100`
   dönüşümünü buna göre düzelt.
5. **Ekran değerini karşılaştır:** Tablodaki GEÇME kriterini uygula.

### 1.3 Doğrulama Geçince — Tek-Satır Bayrak Değişiklikleri

`ESP_AKS/lib/HMIHelpers/HMIHelpers.h` (satır 86-89) — ilgili sinyal geçince:

```
86: constexpr bool HMI_SOC_SOURCE_VERIFIED        = false;  →  = true;
87: constexpr bool HMI_TEMP_SOURCE_VERIFIED       = false;  →  = true;
88: constexpr bool HMI_CURRENT_SOURCE_VERIFIED    = false;  →  = true;
89: constexpr bool HMI_CELL_VOLTAGE_SOURCE_VERIFIED = false; →  = true;
```

- `HMI_CELL_VOLTAGE_SOURCE_VERIFIED=true` ayrıca `main.cpp`'de gerçek per-cell
  yolunu ve `cellcan=1`'i otomatik devreye sokar (tek bayrak, Prompt 5).

**Bayrakla BİRLİKTE yapılması gereken, tek-satır OLMAYAN işler (aksi halde
yanlış/eksik olur):**
- **Akım:** `main.cpp` deciA dönüşümü doğrulanan ölçeğe çekilmeli (§1.2/4).
- **Sıcaklık/akım FAULT'a bağlama:** `BMS_*_MAX_TEMP_C` / akım eşikleri şu an
  VCU kararına BAĞLI DEĞİL; istenirse `VcuLogic::hasWarning/CriticalCondition`'a
  bağlanmalı (E001 freshness takibiyle birlikte). Bu, otorite kuralını
  (SystemConfig = FAULT) korur.
- **Per-cell:** gerçek 24-hücre kaynağı `main.cpp` verified yolundaki placeholder
  (pack/24) yerine çözülmüş ID'lerden doldurulmalı.
- **Kozmetik:** `CanParse.cpp/.h`, `Telemetry.h`, `SystemConfig.h`, `main.cpp`
  boot logu "HIPOTEZ" → "DOĞRULANDI"; `CAN_Message_Table.md`/`Threshold_Ownership.md`
  senkron.

---

## 2. Teknik Kontrol Prova Senaryosu (§9.4, §8.2)

### 2.a — Dört değerin anlık gösterimi (§8.2.a.iv)
- **Hazırlık:** araç açık, BMS CAN yayında, panel `main` sayfasında.
- **Beklenen:** `packv` (Number, gerçek deciV — DOĞRULANDI), `bat`/`temp`/`packi`
  (Text). **Doğrulama ÖNCESİ**: SoC/temp/current = **"--"** (dürüst sentinel),
  gerilim gerçek. **Doğrulama SONRASI** (bayraklar true): dördü de canlı sayı,
  10 Hz güncelleme.
- **Kanıt:** dördü de `xQueuePeek(TEL_sensorDataQueue)`'dan VCU state'ten BAĞIMSIZ
  doldurulur (`main.cpp vTask_HMI_Display`) → durum ne olursa olsun ekranda kalır.

### 2.b — Güç kes-aç → kendiliğinden toparlanma (§9.4.a.vii)
- **ESP reset:** nesne yeniden kurulur → `HMI_hasCachedScreen=false` + `BMS_firstRun=true`
  → sonraki tik TÜM alanları force-refresh basar. `RelayManager.allOff` + INIT→IDLE
  güvenli başlar. Watchdog tüm task'larda.
- **Nextion reset:** `HMI_rxClassifyByte` 0x88 / `00 00 00 FF FF FF` algılar →
  ana sayfa cache sıfırlanır + `takeNextionResetFlag()` BMS'i force-refresh eder →
  ~1 tik (~100 ms) içinde tam repaint.
- **60 sn kriteri:** tam repaint 9600 baud'da throttle-limitli ~1.3-1.5 s (≪ 60 sn).
  LoRa 60 sn kesinti + replay ayrıca `TEKNIK_KONTROL_PROVASI.md` §3'te (offline buffer).

### 2.c — Şarj modunda aynı dört veri
- **Kanıt:** HMI dört değeri VCU state'e bakmadan gösterir; şarj (S1 kapalı) veya
  sürüş (S2 kapalı) fark etmez — aynı `main` sayfası, aynı alanlar. Şarj modu
  anahtarlama (S1/S2) BYS/AKS sorumluluğunda (bkz. Bölüm 3 §8.2.a.iii/vii —
  sahiplik netleştirilmeli, açık iş).

### 2.d — Hız ↔ telemetri bilgisayarı tutarlılığı (§9.18.b / §9.4.b.iii)
- **Kanıt:** `speed.val = TEL_speedKmhX10/10`; `rpmToSpeedKmhX10` VehicleParams.h
  (D=0.56 m, GR=1, teker=motor rpm, `VEHICLE_PARAMS_CONFIRMED=1`).
- **Adım:** aracı bilinen sabit hızda sür → UKS CSV alan 19 (`spdX10`)/10 =
  Monitor GUI HIZ göstergesi = Nextion `speed` = referans (± ölçüm toleransı).

---

## 3. Final Native Test Koşu Raporu Formatı

Komut: `pio test -e native` (tüm suite'ler). Rapor:

```
TUFAN-AKS — Native Test Final Koşu
Tarih: ____   Commit: ____   Ortam: native (host)

Suite                        | Dosya | Test | Sonuç
-----------------------------|-------|------|-------
test_native_vcu_logic        |   9   |  55  | PASS
test_native_can_parsing      |   ?   |  ??  | ???   ← iki-runner tutarsızlığı çözülmeli
test_native_relay            |   7   |  18  | PASS
test_native_telemetry        |   5   |  13  | PASS
test_native_hmi_helpers      |   8   |  55  | PASS  ← +26 (Prompt 3/4: sentinel-txt, RX classifier)
test_native_bms_algo         |  10   |  25  | PASS  ← +6 (Prompt 5: cellcan)
-----------------------------|-------|------|-------
TOPLAM                       |       | 167+ | 

Kabul: TÜM suite'ler PASS + toplam ≥ 167.
```

**Açık iş (rapor öncesi kapatılmalı):** `test_native_can_parsing` iki `main()`
(test_main.cpp + test_bms_config_parse.cpp) + tanımsız extern'ler içeriyor →
`pio test -e native -f test_native_can_parsing` şu an build etmeyebilir. Bu
suite reconcile edilmeden "167+ PASS" iddia EDİLEMEZ (bkz. Prompt 6 devir notu).

---

## 4. Not

Bu belgedeki tüm iddialar kaynak kodda izi olan maddelerdir; doldurulacak
tablo hücreleri (`____`) saha ölçümüyle ekip tarafından tamamlanır —
UYDURULMAMALIDIR.
