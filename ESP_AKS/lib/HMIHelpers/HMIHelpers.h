#pragma once
//
// Saf HMI helper'ları + Nextion UART komut yardımcıları.
// 4 saf metin dönüşümü inline; donanım bağımlılıkları yoktur.
// 3 UART helper'ı (sendEndBytes, sendNumericIfChanged, sendTextIfChanged)
// uart_write_bytes çağırdığı için CPP tarafında tanımlıdır.
// DisplayHMI sınıfı bu fonksiyonları çağırır; native testler aynı
// fonksiyonları doğrudan çağırarak metin formatlama ve cache mantığını
// izole eder.
//
#include <cstddef>
#include <cstdint>
#include <cstdio>

enum class HMI_VcuState : uint8_t {
    INIT = 0,
    IDLE = 1,
    READY = 2,
    DRIVE = 3,
    EMERGENCY_STOP = 4,
    FAULT = 5
};

inline const char* HMI_getStateText(HMI_VcuState HMI_state) {
    switch (HMI_state) {
        case HMI_VcuState::INIT:           return "INIT";
        case HMI_VcuState::IDLE:           return "IDLE";
        case HMI_VcuState::READY:          return "READY";
        case HMI_VcuState::DRIVE:          return "DRIVE";
        case HMI_VcuState::EMERGENCY_STOP: return "ESTOP";
        case HMI_VcuState::FAULT:          return "FAULT";
        default:                           return "UNK";
    }
}

inline void HMI_formatErrorText(uint8_t HMI_errorFlags, char* HMI_output,
                                size_t HMI_outputSize) {
    if (HMI_outputSize == 0)
        return;
    snprintf(HMI_output, HMI_outputSize, "0x%02X", HMI_errorFlags);
}

inline const char* HMI_getValidityText(bool HMI_dataValid,
                                       bool HMI_timeoutActive) {
    if (HMI_timeoutActive)
        return "TIMEOUT";
    return HMI_dataValid ? "VALID" : "INVALID";
}

inline const char* HMI_getContactorText(bool HMI_contactorClosed) {
    return HMI_contactorClosed ? "CLOSED" : "OPEN";
}

// --- "Veri yok" gösterimi (GEÇİCİ — kaynak sinyal doğrulanana kadar) ---
//
// TEL_bmsSocHundredths ve TEL_bmsTempHighestC kaynak CAN sinyalleri henüz
// DOĞRULANMADI: hiçbir CAN ID'den parse edilmiyorlar, hep 0 kalıyorlar.
// 0'ı doğrudan ekrana basmak sürücüye "%0 batarya / 0°C" gibi SAHTE veri
// gösterir. Bu yüzden kaynak doğrulanana kadar geçerli aralık DIŞI birer
// sentinel gönderilir; Nextion tarafı bu değerleri "--" olarak
// göstermelidir (bat: 0-100 geçerli, 255 = veri yok; temp: -127 = veri yok).
//
// TEL_bmsDataValid=false (BMS hiç görülmedi / timeout) durumunda da aynı
// sentinel geçerlidir — bayat/yok veri asla sayı gibi gösterilmez.
//
// TODO(dogrulama): İlgili kaynak sinyalin ID'si + ölçeği DOĞRULANDIĞINDA
// aşağıdaki *_SOURCE_VERIFIED sabiti true yapılıp bu geçici sentinel yolu
// kaldırılacak (bkz. Documents/CAN_Message_Table.md).
constexpr uint8_t HMI_BATTERY_NO_DATA = 255;
constexpr int16_t HMI_TEMP_NO_DATA = -127;
constexpr uint16_t HMI_CELL_VOLTAGE_NO_DATA = 65535;
// Akım sentinel'i (işaretli deci-amper, 0.1 A). Sözleşme (HMI_Field_Map.md v2,
// Runtime tablosu) INT16_MIN'i "veri yok" olarak sabitler — geçerli akım
// aralığı (-3000..3000 deciA) dışı. Sentinel politikası B (AKS-tarafı txt):
// ekrana asla sayı olarak basılmaz, txt formatlayıcı "--" üretir.
constexpr int16_t HMI_CURRENT_NO_DATA = INT16_MIN;  // -32768 = veri yok

// HIPOTEZ: Bu iki kaynak (0xE000 b[4:5] SoC, 0xE001 b[6:7] sıcaklık)
// firmware'de PARSE ediliyor ama byte anlamı/ölçeği CAN_Message_Table.md'de
// hâlâ HIPOTEZ seviyesinde (SoC için byte[4:5] "kapasite sayacı adayı" ile
// çelişki, sıcaklık için HIPOTEZ-orta). Sniffer ile bağımsız doğrulama
// Prompt 7 (donanım) aşamasına ait — o teyit YAPILANA KADAR bu bayraklar
// false kalır ve ekran gerçek sayı yerine sentinel ("--") gösterir. Teyit
// sonrası ilgili bayrak true yapılıp CAN_Message_Table.md DOĞRULANDI'ya
// çekilecek.
constexpr bool HMI_SOC_SOURCE_VERIFIED = false;          // TEL_bmsSocHundredths (0xE000 b[4:5]) — HIPOTEZ, Prompt 7 bekliyor
constexpr bool HMI_TEMP_SOURCE_VERIFIED = false;         // TEL_bmsTempHighestC (0xE001 b[6:7]) — HIPOTEZ, Prompt 7 bekliyor
constexpr bool HMI_CURRENT_SOURCE_VERIFIED = false;      // TEL_bmsCurrentCentiMa (0xE000 b[0:1]) — HIPOTEZ (ölçek/işaret/birim DOĞRULANMADI), Prompt 7 bekliyor
constexpr bool HMI_CELL_VOLTAGE_SOURCE_VERIFIED = false; // TEL_bmsCellVoltageMaxDeciMv/MinDeciMv — DOĞRULANMADI (kaynak ID yok)

inline uint8_t HMI_batteryDisplayValue(bool HMI_sourceVerified,
                                       bool HMI_bmsDataValid,
                                       uint16_t HMI_socHundredths) {
    if (!HMI_sourceVerified || !HMI_bmsDataValid)
        return HMI_BATTERY_NO_DATA;
    const uint16_t HMI_percent = HMI_socHundredths / 100U;
    return (HMI_percent > 100U) ? 100U : static_cast<uint8_t>(HMI_percent);
}

inline int16_t HMI_temperatureDisplayValue(bool HMI_sourceVerified,
                                           bool HMI_bmsDataValid,
                                           int16_t HMI_temperatureC) {
    if (!HMI_sourceVerified || !HMI_bmsDataValid)
        return HMI_TEMP_NO_DATA;
    return HMI_temperatureC;
}

// Akım gösterim değeri (işaretli deci-amper). SoC/sıcaklık ile aynı gating
// mantığı: kaynak DOĞRULANMADIYSA (HMI_CURRENT_SOURCE_VERIFIED=false) veya
// BMS verisi taze değilse sentinel döner — doğrulanmamış akım sürücüye sayı
// gibi gösterilmez (bkz. Prompt 6 Yol A / §8.2.a.iv).
inline int16_t HMI_currentDisplayValue(bool HMI_sourceVerified,
                                       bool HMI_bmsDataValid,
                                       int16_t HMI_packCurrentDeciA) {
    if (!HMI_sourceVerified || !HMI_bmsDataValid)
        return HMI_CURRENT_NO_DATA;
    return HMI_packCurrentDeciA;
}

// --- Sentinel politikası B: AKS-tarafı txt formatlama (HMI_Field_Map.md v2) ---
// bat / temp / packi Text objesidir; sihirli sentinel sayıları (255/-127/
// INT16_MIN) ekrana asla sızmaz — bu formatlayıcılar sentinel'i "--" yapar,
// aksi halde sözleşmedeki metni üretir. Saf (donanımsız) — native testlenir.

// SoC: HMI_batteryDisplayValue çıktısını (0..100 veya 255) "0".."100" / "--".
inline void HMI_formatBatteryText(uint8_t HMI_batteryValue, char* HMI_out,
                                  size_t HMI_outSize) {
    if (HMI_outSize == 0) return;
    if (HMI_batteryValue == HMI_BATTERY_NO_DATA)
        snprintf(HMI_out, HMI_outSize, "--");
    else
        snprintf(HMI_out, HMI_outSize, "%u",
                 static_cast<unsigned>(HMI_batteryValue));
}

// Sıcaklık: HMI_temperatureDisplayValue çıktısını (°C veya -127) "-40".."125" / "--".
inline void HMI_formatTempText(int16_t HMI_tempValue, char* HMI_out,
                               size_t HMI_outSize) {
    if (HMI_outSize == 0) return;
    if (HMI_tempValue == HMI_TEMP_NO_DATA)
        snprintf(HMI_out, HMI_outSize, "--");
    else
        snprintf(HMI_out, HMI_outSize, "%d", static_cast<int>(HMI_tempValue));
}

// Akım: HMI_currentDisplayValue çıktısını (deciA veya INT16_MIN) "±d.d" / "--".
// Örn: 125 -> "12.5", -80 -> "-8.0", 5 -> "0.5", -5 -> "-0.5", 0 -> "0.0".
inline void HMI_formatCurrentText(int16_t HMI_currentDeciA, char* HMI_out,
                                  size_t HMI_outSize) {
    if (HMI_outSize == 0) return;
    if (HMI_currentDeciA == HMI_CURRENT_NO_DATA) {
        snprintf(HMI_out, HMI_outSize, "--");
        return;
    }
    const bool HMI_neg = HMI_currentDeciA < 0;
    const int HMI_mag = HMI_neg ? -static_cast<int>(HMI_currentDeciA)
                                : static_cast<int>(HMI_currentDeciA);
    snprintf(HMI_out, HMI_outSize, "%s%d.%d", HMI_neg ? "-" : "",
             HMI_mag / 10, HMI_mag % 10);
}

// --- RX yolu: dokunmatik komut + Nextion sistem (reset) sınıflandırıcısı ---
//
// Saf (donanımsız) byte sınıflandırıcı — DisplayHMI.cpp native derlenmediği
// için touch/reset ayrımı buraya taşındı ve native testlenir (bkz.
// test_native_hmi_helpers). Byte akışında üç şey ayırt edilir:
//   1) Dokunmatik komut çerçevesi: 0x5A [cmd] [~cmd], cmd ∈ {1,2,3,4}.
//      ~cmd ∈ {0xFE,0xFD,0xFC,0xFB} — HİÇBİRİ 0x00/0xFF DEĞİL; ayrıca cmd de
//      0x00/0xFF değil. Yani geçerli bir touch çerçevesi asla 0x00/0xFF içermez.
//   2) Nextion "hazır/reset": 0x88 (power-on/reset event). Tek byte, kesin.
//   3) Nextion açılış dizisi: 0x00 0x00 0x00 0xFF 0xFF 0xFF (tam 6-byte örüntü).
// (1) ile (2)/(3) çakışmaz: sistem byte'ları (0x88/0x00/0xFF) touch çerçevesinin
// parçası olamaz. bkcmd=0 olduğundan komut-ack'leri gelmez; bu sınıflandırıcı
// yine de artık 0x00/0xFF'leri güvenle yutar (yalnız tam açılış örüntüsü reset
// üretir → gürültüde yanlış-pozitif düşük).
enum class HMI_RxEvent : uint8_t {
    NONE = 0,
    TOUCH = 1,           // geçerli 0x5A cmd ~cmd; cmd `outCmd`'e yazılır (1..4)
    NEXTION_RESET = 2,   // 0x88 ya da 00 00 00 FF FF FF
};

struct HMI_RxState {
    uint8_t touchState = 0;  // 0 boşta, 1: 0x5A alındı, 2: cmd alındı (~cmd bekleniyor)
    uint8_t pendingCmd = 0;
    uint8_t zeroRun = 0;     // ardışık 0x00 (açılış örüntüsü 00 00 00 için)
    uint8_t ffRun = 0;       // 3 sıfırdan sonra ardışık 0xFF
};

inline HMI_RxEvent HMI_rxClassifyByte(HMI_RxState& HMI_st, uint8_t HMI_b,
                                      uint8_t& HMI_outCmd) {
    // 0x88 — Nextion hazır/reset (en yüksek öncelik, tek byte kesin).
    if (HMI_b == 0x88) {
        HMI_st = HMI_RxState{};
        return HMI_RxEvent::NEXTION_RESET;
    }

    // 0x00 / 0xFF — touch çerçevesinin parçası olamaz; açılış izleyicisine ver
    // ve varsa yarım touch çerçevesini iptal et.
    if (HMI_b == 0x00 || HMI_b == 0xFF) {
        HMI_st.touchState = 0;
        HMI_st.pendingCmd = 0;
        if (HMI_b == 0x00) {
            if (HMI_st.ffRun > 0) {      // FF'lerden sonra 0x00 → örüntü kırıldı, sıfırdan
                HMI_st.ffRun = 0;
                HMI_st.zeroRun = 1;
            } else if (HMI_st.zeroRun < 3) {
                HMI_st.zeroRun++;
            }  // zeroRun==3'te fazladan sıfır tolere edilir
        } else {  // 0xFF
            if (HMI_st.zeroRun >= 3) {
                HMI_st.ffRun++;
                if (HMI_st.ffRun >= 3) {
                    HMI_st = HMI_RxState{};
                    return HMI_RxEvent::NEXTION_RESET;
                }
            } else {                     // 3 sıfır gelmeden FF → açılış değil
                HMI_st.ffRun = 0;
                HMI_st.zeroRun = 0;
            }
        }
        return HMI_RxEvent::NONE;
    }

    // Diğer her byte açılış izleyicisini sıfırlar.
    HMI_st.zeroRun = 0;
    HMI_st.ffRun = 0;

    // Dokunmatik çerçeve: 0x5A [cmd] [~cmd]
    switch (HMI_st.touchState) {
        case 0:
            if (HMI_b == 0x5A) HMI_st.touchState = 1;
            return HMI_RxEvent::NONE;
        case 1:
            HMI_st.pendingCmd = HMI_b;
            HMI_st.touchState = 2;
            return HMI_RxEvent::NONE;
        case 2: {
            const uint8_t HMI_expected = static_cast<uint8_t>(~HMI_st.pendingCmd);
            HMI_st.touchState = 0;
            if (HMI_b == HMI_expected) {
                HMI_outCmd = HMI_st.pendingCmd;
                return HMI_RxEvent::TOUCH;
            }
            return HMI_RxEvent::NONE;  // checksum uyuşmadı
        }
        default:
            HMI_st.touchState = 0;
            return HMI_RxEvent::NONE;
    }
}

// Nextion UART tarafı — implementasyonu HMIHelpers.cpp içindedir.
void HMI_sendEndBytes(void);

// Yalnızca yeni değer önceki cache'ten farklıysa (veya force=true) UART'a
// "{component}.val={value}\xFF\xFF\xFF" yazar.
void HMI_sendNumericIfChanged(const char* HMI_component, int32_t HMI_value,
                              int32_t HMI_lastValue, bool HMI_force);

// Aynı şekilde text varyantı: "{component}.txt=\"{value}\"\xFF\xFF\xFF".
void HMI_sendTextIfChanged(const char* HMI_component, const char* HMI_value,
                           const char* HMI_lastValue, bool HMI_force);
