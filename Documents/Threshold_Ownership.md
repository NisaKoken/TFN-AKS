# Batarya Eşik Sahipliği (Threshold Ownership)

Repoda birbirinden habersiz **iki ayrı batarya eşik seti** var:

1. `include/SystemConfig.h` — **pack-bazlı** (deciV / centi-mA / °C), `src/VcuLogic.h`
   (`hasWarningCondition` / `hasCriticalCondition`) ve `lib/CanManager/CanManager.cpp`
   (`checkPackVoltageFault` üzerinden) tarafından tüketiliyor. Bu set VCU durum
   makinesini (FAULT / kontaktör) besler.
2. `lib/BmsAlgo/BmsAlgo.h` — **hücre-bazlı** (mV / °C), `computePack()` tarafından
   tüketiliyor. Çıktısı (`BmsComputed`) şu an yalnızca Nextion HMI'ye
   (`BmsNextionPacket.cpp`) gidiyor; VCU kararına bağlı DEĞİL.

Bu doküman her iki setin güncel değerlerini, tüketicilerini, bağlı oldukları
sinyalleri ve hangilerinin fiilen ölü/canlı olduğunu kaydeder. Değerler
koddan (aşağıdaki satır referanslarıyla) okunmuştur, ezberden yazılmamıştır.

---

## 1. Otorite Kuralı

**Araç güvenliği kararları (FAULT / kontaktör açma) için `SystemConfig.h`
OTORİTERDİR.** `BmsAlgo.h` eşikleri yalnızca gösterim/uyarı katmanıdır
(Nextion ekranındaki renk/uyarı seviyesi). İki set çelişirse **`SystemConfig.h`
kazanır** — `BmsAlgo.h` tarafındaki bir eşik aşılsa bile VCU FAULT'a geçmez;
tersine `SystemConfig.h` eşiği aşıldığında VCU FAULT'a geçer, ekran o anda
farklı bir renk gösteriyor olsa bile.

---

## 2. `SystemConfig.h` — Pack-Bazlı Eşikler (VCU Güvenlik Kararı OTORİTESİ)

Kaynak: `include/SystemConfig.h`, "Phase 2 Safety Thresholds" bölümü (satır 156 vd.).

| Sabit | Satır | Değer | Birim | Tüketici (fonksiyon) | Bağlı Sinyal | Sinyal Durumu | Karar Yolunda CANLI mı? |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `BMS_WARN_MIN_PACK_VOLTAGE_DECI_V` | 176 | 720 (72.0 V) | deciV | `VcuLogic::hasWarningCondition` | `TEL_bmsPackVoltageDeciV` | ✅ DOĞRULANDI | ✅ CANLI |
| `BMS_CRITICAL_MIN_PACK_VOLTAGE_DECI_V` | 177 | 600 (60.0 V) | deciV | `VcuLogic::hasCriticalCondition` + `CanManager::handleLbBmsE000` → `CanParse::checkPackVoltageFault` | `TEL_bmsPackVoltageDeciV` | ✅ DOĞRULANDI | ✅ CANLI (iki bağımsız yol) |
| `BMS_WARN_MAX_PACK_VOLTAGE_DECI_V` | 178 | 852 (85.2 V) | deciV | `VcuLogic::hasWarningCondition` | `TEL_bmsPackVoltageDeciV` | ✅ DOĞRULANDI | ✅ CANLI |
| `BMS_CRITICAL_MAX_PACK_VOLTAGE_DECI_V` | 179 | 876 (87.6 V) | deciV | `VcuLogic::hasCriticalCondition` + `CanManager::handleLbBmsE000` → `CanParse::checkPackVoltageFault` | `TEL_bmsPackVoltageDeciV` | ✅ DOĞRULANDI | ✅ CANLI (iki bağımsız yol) |
| `BMS_WARN_MAX_TEMP_C` | 185 | 55 | °C | *(yok — VCU'ya bağlı değil)* | `TEL_bmsTempHighestC` | ⚠️ HIPOTEZ — 0xE001 b[6:7]'den PARSE ediliyor (max), ama byte anlamı DOĞRULANMADI (Prompt 7) | ❌ ÖLÜ (karar yolunda; sinyal artık canlı ama eşik bağlanmadı) |
| `BMS_CRITICAL_MAX_TEMP_C` | 186 | 70 | °C | *(yok — VCU'ya bağlı değil)* | `TEL_bmsTempHighestC` | ⚠️ HIPOTEZ — parse ediliyor, DOĞRULANMADI | ❌ ÖLÜ (karar yolunda) |
| `BMS_WARN_MAX_CHARGE_CURRENT_CENTI_MA` | 192 | 90 000 (0.9 A) | centi-mA | `VcuLogic::isCurrentWarning` (saf yardımcı, birim testli — ama çağrılmıyor) | `TEL_bmsCurrentCentiMa` | ⚠️ HIPOTEZ — 0xE000 b[0:1]'den PARSE ediliyor (raw*10), ölçek/işaret DOĞRULANMADI + birim adı belirsiz (Prompt 7) | ❌ ÖLÜ (bağlanmamış; birim tutarsızlığı da var — bkz. not) |
| `BMS_CRITICAL_MAX_CHARGE_CURRENT_CENTI_MA` | 193 | 100 000 (1.0 A) | centi-mA | `VcuLogic::isCurrentCritical` (aynı durum) | `TEL_bmsCurrentCentiMa` | ⚠️ HIPOTEZ — parse ediliyor, DOĞRULANMADI | ❌ ÖLÜ (bağlanmamış) |
| `BMS_WARN_MAX_DISCHARGE_CURRENT_CENTI_MA` | 194 | 900 000 (9.0 A) | centi-mA | `VcuLogic::isCurrentWarning` | `TEL_bmsCurrentCentiMa` | ⚠️ HIPOTEZ — parse ediliyor, DOĞRULANMADI | ❌ ÖLÜ (bağlanmamış) |
| `BMS_CRITICAL_MAX_DISCHARGE_CURRENT_CENTI_MA` | 195 | 1 500 000 (15.0 A) | centi-mA | `VcuLogic::isCurrentCritical` | `TEL_bmsCurrentCentiMa` | ⚠️ HIPOTEZ — parse ediliyor, DOĞRULANMADI | ❌ ÖLÜ (bağlanmamış) |
| `BMS_CRITICAL_MIN_CELL_VOLTAGE_MV` | 200 | 2500 | mV | *(yok)* | `TEL_bmsCellVoltageMinDeciMv` | ❌ UNVERIFIED — hep 0 | ❌ ÖLÜ |
| `BMS_CRITICAL_MAX_CELL_VOLTAGE_MV` | 201 | 3650 | mV | *(yok)* | `TEL_bmsCellVoltageMaxDeciMv` | ❌ UNVERIFIED — hep 0 | ❌ ÖLÜ |

Freshness/timeout eşikleri (aynı dosya, "CAN Freshness Thresholds" bölümü, satır 207 vd.)
da pack/paket seviyesinde VCU kararını besler:

| Sabit | Satır | Değer | Tüketici | Sinyal Durumu | Karar Yolunda CANLI mı? |
| --- | --- | --- | --- | --- | --- |
| `CAN_MOTOR_STATUS_TIMEOUT_MS` | 208 | 500 ms | `CanManager::updateMotorStatusValidity` → `TEL_motorTimeoutActive` → `VcuLogic::hasCriticalCondition` (IDLE dışında critical) | ✅ DOĞRULANDI (frame varlığı, ölçeğe bağlı değil) | ✅ CANLI |
| `CAN_BMS_STATUS_TIMEOUT_MS` | 209 | 500 ms | `CanManager::updateBmsValidity` → `TEL_bmsTimeoutActive` → `VcuLogic::hasCriticalCondition` (IDLE dışında critical) | ✅ DOĞRULANDI (E000 frame varlığı) | ✅ CANLI |
| `CAN_CHARGER_TIMEOUT_MS` | 213 | 2000 ms | Yalnızca charger setpoint'lerini "bayat" işaretler | ✅ DOĞRULANDI | ⚠️ KISMİ — `CAN_Event`/FAULT ÜRETMEZ (bilinçli tasarım, opsiyonel akış) |

---

## 3. `lib/BmsAlgo/BmsAlgo.h` — Hücre-Bazlı Eşikler (Gösterim/Uyarı Katmanı)

Kaynak: `lib/BmsAlgo/BmsAlgo.h`. Tüketici tek fonksiyon: `computePack()`
(`lib/BmsAlgo/BmsAlgo.cpp:30-108`). Çıktı (`BmsComputed.warningLevel`) yalnızca
`buildBmsNextionCommands()` üzerinden Nextion ekranına (`warn` alanı) gidiyor —
VCU karar mantığına hiç girmiyor.

| Sabit | Satır | Değer | Birim | Rol |
| --- | --- | --- | --- | --- |
| `BMS_BALANCE_THRESHOLD_MV` | 25 | 50 | mV | Pasif dengeleme tetik bandı (delta eşiği) — kimyadan bağımsız, DEĞİŞMEDİ |
| `BMS_BALANCE_TOP_MARGIN_MV` | 30 | 5 | mV | Dengeleme marjı (en yüksek hücreye yakın diğerleri) — kimyadan bağımsız, DEĞİŞMEDİ |
| `BMS_SOC_EMPTY_MV` | 51 | 2500 | mV | SoC %0 referansı — LiFePO4 spec min (2.50 V) |
| `BMS_SOC_FULL_MV` | 52 | 3650 | mV | SoC %100 referansı — LiFePO4 spec maks (3.65 V) |
| `BMS_CELL_UNDERVOLT_CRIT_MV` | 66 | 2500 | mV | Hücre CRITICAL alt sınır — LiFePO4 spec min |
| `BMS_CELL_OVERVOLT_CRIT_MV` | 67 | 3650 | mV | Hücre CRITICAL üst sınır — LiFePO4 spec maks |
| `BMS_TEMP_OVERTEMP_CRIT_C` | 68 | 60 | °C | Hücre CRITICAL sıcaklık — kimyadan bağımsız, DEĞİŞMEDİ |
| `BMS_CELL_UNDERVOLT_WARN_MV` | 75 | 2800 | mV | Hücre WARNING alt sınır — CRIT'e 300 mV marj |
| `BMS_CELL_OVERVOLT_WARN_MV` | 76 | 3550 | mV | Hücre WARNING üst sınır — CRIT'e 100 mV marj |
| `BMS_TEMP_OVERTEMP_WARN_C` | 77 | 50 | °C | Hücre WARNING sıcaklık — kimyadan bağımsız, DEĞİŞMEDİ |

Değerler 24S LiFePO4 spec'ine (2.50–3.65 V/hücre) uyarlanmıştır — bkz. bölüm 5.
CRITICAL uçları (2500/3650 mV) SystemConfig.h pack CRITICAL eşikleriyle
(600/876 deciV) hücre×24 ilişkisiyle birebir örtüşür; WARN bandı bu katmana
özgüdür (SystemConfig.h WARN eşikleriyle 1:1 eşleşmesi gerekmez).

> **Güncelleme (Prompt 5 — Yol A ile uyumlu):** `src/main.cpp` artık per-cell
> veriyi ORTALAMAYLA doldurmuyor. Per-cell kaynak (0xE002-E005, E032-E033)
> DOĞRULANMADIĞI için (tek bayrak `HMI_CELL_VOLTAGE_SOURCE_VERIFIED=false`)
> hücreler sentinel (65535) ile doldurulur → tüm bar 0, `cellmax/cellmin`
> sentinel, ve ayrı bir **`cellcan=0`** göstergesi ("CAN doğrulanmadı") basılır.
> Bu durumda `computePack()` sentinel hücrelerle ÇAĞRILMAZ (65535 > OVERVOLT_CRIT
> sahte CRITICAL üretirdi); `warningLevel` NÖTR (OK) — bayat/timeout durumunda
> CRITICAL. Doğrulama sonrası bayrak `true` yapılınca gerçek 24-hücre yolu +
> gerçek `computePack` warn'ı devreye girer.
>
> **`cellcan` vs `warn` ayrımı (otorite kuralı ile uyumlu):** `warn` (0/1/2)
> BmsAlgo GÖSTERİM eşiğidir; `cellcan` (0/1) veri-KAYNAĞI doğrulama durumudur.
> "CAN doğrulanmadı" durumu `warn` rengine yüklenMEZ — ayrı `cellcan` alanıyla
> gösterilir. Böylece ileride gerçek bir hücre uyarısı (warn=2) ile veri-yok
> durumu (cellcan=0) ekranda birbirine karışmadan görünebilir.

---

## 4. Fiilen Ölü Eşikler ve Neden Önemli

**Karar yolunda ÖLÜ** (eşik VCU `hasWarning/hasCriticalCondition`'a
BAĞLANMAMIŞ olduğu için gerçek olayda FAULT üretmeyen): sıcaklık eşikleri
(`BMS_WARN_MAX_TEMP_C`, `BMS_CRITICAL_MAX_TEMP_C`), akım eşikleri
(`BMS_WARN_/CRITICAL_MAX_CHARGE_/DISCHARGE_CURRENT_CENTI_MA`) ve hücre voltajı
eşikleri (`BMS_CRITICAL_MIN_/MAX_CELL_VOLTAGE_MV`).

> **Güncelleme (Prompt 6 — Yol A):** Sıcaklık (`TEL_bmsTempHighestC`, 0xE001
> b[6:7] max), akım (`TEL_bmsCurrentCentiMa`, 0xE000 b[0:1]) ve SoC
> (`TEL_bmsSocHundredths`, 0xE000 b[4:5]) kaynak sinyalleri artık **PARSE
> ediliyor** (yani "hep 0" DEĞİL). ANCAK byte anlamı/ölçeği CAN_Message_Table.md'de
> **HIPOTEZ** — Prompt 7 donanım sniffer teyidi bekliyor. Bu yüzden:
> - **VCU kararına hâlâ BAĞLI DEĞİL** (yukarıdaki eşikler bağlanmadı) — ÖLÜ.
> - **HMI ekranı** SoC/sıcaklık için `HMI_*_SOURCE_VERIFIED=false` ile sentinel
>   ("--") gösterir (doğrulanmamış değeri sürücüye sayı gibi göstermez). Akımın
>   HMI alanı hiç yok.
> - **Telemetri CSV** (UKS izleme merkezi) bu HIPOTEZ değerleri sanitize'dan
>   geçirerek yine de gönderir (alanlar CSV sözleşmesinde sabit). Açık iş:
>   izleme merkezi tarafında da doğrulanana kadar "güvenilir" gösterilmemeli.

Bu eşiklerin kodda tanımlı, derlenen ve (akım için) birim testli olması, ekip
üyelerinde "bu korumalar aktif" izlenimi yaratabilir; oysa gerçek bir aşırı
sıcaklık/akım/hücre-voltajı olayında VCU bunu FARK ETMEZ ve FAULT'a geçmez —
bu, sahada **yanlış güvenlik hissi** riski taşır.

**Canlı (karar yolunda)**: pack voltajı eşikleri (`TEL_bmsPackVoltageDeciV`,
DOĞRULANDI) ve motor/BMS freshness timeout'ları.

---

## 5. Bilinen Çelişki: `BmsAlgo.h` NMC Varsayımları vs LiFePO4 Paket Kimyası

> ✅ **ÇÖZÜLDÜ** (`AKS-bmsalgo-lifepo4-thresholds`, bkz. `lib/BmsAlgo/BmsAlgo.h`
> ve `lib/BmsAlgo/BmsNextionPacket.cpp`'nin güncel hali). Aşağıdaki tarihçe,
> çelişkinin neden var olduğunu ve nasıl giderildiğini kaydetmek için
> bilerek SİLİNMEDİ.

`BmsAlgo.h`'nin hücre eşikleri, bu PR'dan önce **4.2 V Li-ion (NMC) kimyası**
varsayımıyla yazılmıştı:

| BmsAlgo.h sabiti | Eski (NMC) değer | NMC'de tipik anlamı | 24S LiFePO4 gerçeği (SystemConfig.h pack spec'i) | Yeni (LiFePO4) değer |
| --- | --- | --- | --- | --- |
| `BMS_SOC_FULL_MV` | 4200 mV | Dolu hücre (NMC) | LiFePO4 hücre maksimumu **3650 mV** (3.65 V) | **3650 mV** |
| `BMS_CELL_OVERVOLT_CRIT_MV` | 4250 mV | NMC aşırı şarj | LiFePO4 spec maks zaten 3650 mV — bu eşik hiç aşılamaz aralıktaydı | **3650 mV** |
| `BMS_CELL_UNDERVOLT_CRIT_MV` | 3000 mV | NMC aşırı deşarj | LiFePO4 spec min **2500 mV** (2.50 V) — bu eşik LiFePO4 için ERKEN tetikleniyordu | **2500 mV** |
| `BMS_SOC_EMPTY_MV` | 3000 mV | Boş hücre (NMC) | LiFePO4 spec min 2500 mV | **2500 mV** |
| `BMS_CELL_UNDERVOLT_WARN_MV` | 3200 mV | NMC WARN alt | CRIT'e (yeni: 2500) 300 mV marj hedefleniyor | **2800 mV** |
| `BMS_CELL_OVERVOLT_WARN_MV` | 4150 mV | NMC WARN üst | CRIT'e (yeni: 3650) 100 mV marj hedefleniyor | **3550 mV** |

Sıcaklık (`BMS_TEMP_OVERTEMP_WARN_C`/`CRIT_C`, 50/60°C) ve dengeleme
(`BMS_BALANCE_THRESHOLD_MV`/`TOP_MARGIN_MV`, 50/5 mV) eşikleri kimyadan
bağımsız olduğu için DEĞİŞTİRİLMEDİ.

Aynı NMC varsayımı üçüncü bir yerde daha vardı: `lib/BmsAlgo/BmsNextionPacket.cpp`
içindeki `cellBarFill()` fonksiyonunun **lokal** `kBarEmptyMv = 3000` /
`kBarFullMv = 4200` sabitleri — hücre bar doluluk yüzdesini aynı yanlış (NMC)
referans aralığına göre hesaplıyordu. Bu lokal kopya silindi; `cellBarFill()`
artık doğrudan `BmsAlgo.h`'deki `BMS_SOC_EMPTY_MV`/`BMS_SOC_FULL_MV`'yi
kullanıyor (tek kaynak).

Regresyon kilidi: `test/test_native_bms_algo/` (SoC haritalaması, WARN/CRITICAL
sınır semantiği, dengeleme, `cellBarFill` — bkz. `Documents/Test_Guide.md`).

---

## 6. Yeni Eşik Eklerken Karar Kuralı

- Eşik **pack-bazlı bir VCU güvenlik kararı** ise (FAULT/kontaktör tetikleyecekse)
  → `include/SystemConfig.h`'ye eklenir, `VcuLogic.h`'deki
  `hasWarningCondition`/`hasCriticalCondition`'a bağlanır.
- Eşik **hücre-bazlı bir gösterim/algoritma** ise (HMI uyarı rengi, dengeleme,
  SoC tahmini) → `lib/BmsAlgo/BmsAlgo.h`'ye eklenir, `computePack()` içinde
  kullanılır.
- Aynı fiziksel limitin iki dosyada da tutulması gerekiyorsa (ör.
  hücre_maks_mV × 24 = pack_maks_deciV), bu tutarlılık ilişkisi **her iki
  dosyada da yorumla belgelenmelidir** — sabit değerleri birbirinden bağımsız
  güncellenip sessizce ayrışmamalıdır.
