#include "CanParse.h"

namespace CanParse {

bool parseMotorStatus(const twai_message_t& msg, MotorStatus& out) {
    if (msg.data_length_code < 4)
        return false;

    out.rpm = static_cast<uint16_t>((msg.data[0] << 8) | msg.data[1]);
    out.torqueFeedback =
        static_cast<int16_t>((msg.data[2] << 8) | msg.data[3]);
    out.errorFlags = (msg.data_length_code >= 5) ? msg.data[4] : 0;
    out.isValid = true;
    return true;
}

// =========================================================================
// Lithium Balance c-BMS — 29-bit Extended CAN ID'ler
// =========================================================================

// 0xE000 — alan bazında güven seviyeleri için bkz. CanParse.h ve
// Documents/CAN_Message_Table.md.
bool parseLbBmsE000(const twai_message_t& msg, TelemetryData& out) {
    if (msg.data_length_code < 8)
        return false;

    // HIPOTEZ (ölçek/işaret DOĞRULANMADI — Prompt 7 sniffer teyidi bekliyor):
    // byte[0:1] = Pack Current adayı, int16_t. Ölçek burada raw*10 (aday) olarak
    // uygulanıyor ama CAN_Message_Table.md bu ölçeği UNVERIFIED işaretliyor. Değer
    // TelemetryData'ya yazılır (telemetri logu için) ANCAK sürücü ekranına gerçek
    // veri gibi BASILMAZ (HMI akım alanı yok) ve VCU kararına BAĞLI DEĞİL.
    int16_t raw_current = static_cast<int16_t>((msg.data[0] << 8) | msg.data[1]);
    out.TEL_bmsCurrentCentiMa = static_cast<int32_t>(raw_current) * 10;

    // DOĞRULANDI: byte[2:3] = Pack Voltage, uint16_t, Çarpan 0.1V (2 sniffer
    // oturumunda bağımsız teyit — bu alan VCU güvenlik kararına bağlıdır).
    out.TEL_bmsPackVoltageDeciV = static_cast<uint16_t>((msg.data[2] << 8) | msg.data[3]);

    // HIPOTEZ (alan ANLAMI DOĞRULANMADI — Prompt 7 bekliyor): byte[4:5] SoC olarak
    // yorumlanıyor (raw*0.01%), fakat CAN_Message_Table.md bu byte'ları "kapasite
    // sayacı adayı" olarak işaretliyor (idle'da AZALIYORDU — SoC aleyhine kanıt).
    // Yazılır ama HMI_SOC_SOURCE_VERIFIED=false olduğundan ekranda sentinel gösterilir.
    out.TEL_bmsSocHundredths = static_cast<uint16_t>((msg.data[4] << 8) | msg.data[5]);

    out.TEL_bmsDataValid = true;
    return true;
}

// 0x1806E5F4 — Charger komut frame'i (BMS -> Charger; AKS yalnızca dinler).
bool parseCharger1806E5F4(const twai_message_t& msg, ChargerCommand& out) {
    if (msg.data_length_code < 4)
        return false;

    // DOĞRULANDI: byte[0:1] = şarj voltaj hedefi, big-endian uint16, ×0.1 V
    out.chargeVoltageSetpointDeciV =
        static_cast<uint16_t>((msg.data[0] << 8) | msg.data[1]);

    // DOĞRULANDI: byte[2:3] = şarj akım hedefi, big-endian uint16, ×0.1 A
    out.chargeCurrentSetpointDeciA =
        static_cast<uint16_t>((msg.data[2] << 8) | msg.data[3]);

    return true;
}

// --- Aşağıdaki fonksiyonlar, CAN sniffer loglarında görülen ancak alan
// --- anlamı henüz DOĞRULANMAMIŞ ID'ler içindir. Ham byte'ları kabul eder
// --- ama TelemetryData'ya anlam yüklenmez. İleride gerçek anlam çözüldükçe
// --- bu fonksiyonların içi doldurulacaktır.

// 0xE001 — Sıcaklık (HIPOTEZ — ölçek/anlam DOĞRULANMADI, Prompt 7 bekliyor).
// byte[6] ve byte[7] iki sıcaklık sensörü adayı (int8_t, °C aday). Şartname
// B3 §9.2.c.ii telemetride "en yüksek olanının sıcaklığı" istediğinden
// TEL_bmsTempHighestC = max(t1,t2), TEL_bmsTempLowestC = min(t1,t2) olarak
// hesaplanır (byte sırasına körü körüne güvenilmez; hangi sensörün sıcak
// olduğu frame'e göre değişebilir). CAN_Message_Table.md bu byte'ları
// HIPOTEZ-orta işaretliyor — HMI_TEMP_SOURCE_VERIFIED=false olduğundan ekranda
// sentinel gösterilir.
bool parseLbBmsE001(const twai_message_t& msg, TelemetryData& out) {
    if (msg.data_length_code < 8)
        return false;

    const int8_t t1 = static_cast<int8_t>(msg.data[6]);
    const int8_t t2 = static_cast<int8_t>(msg.data[7]);
    out.TEL_bmsTempHighestC = (t1 >= t2) ? t1 : t2;  // en yüksek olan (9.2.c.ii)
    out.TEL_bmsTempLowestC = (t1 <= t2) ? t1 : t2;   // en düşük olan

    return true;
}

// 0xE002 — TODO: alan anlamı doğrulanmadı
bool parseLbBmsE002(const twai_message_t& msg, TelemetryData& out) {
    (void)out;
    // TODO: alan anlamı doğrulanmadı, ham byte'lar loglanıyor
    return msg.data_length_code > 0;
}

// 0xE003 — TODO: alan anlamı doğrulanmadı
bool parseLbBmsE003(const twai_message_t& msg, TelemetryData& out) {
    (void)out;
    // TODO: alan anlamı doğrulanmadı, ham byte'lar loglanıyor
    return msg.data_length_code > 0;
}

// 0xE004 — TODO: alan anlamı doğrulanmadı
bool parseLbBmsE004(const twai_message_t& msg, TelemetryData& out) {
    (void)out;
    // TODO: alan anlamı doğrulanmadı, ham byte'lar loglanıyor
    return msg.data_length_code > 0;
}

// 0xE005 — TODO: alan anlamı doğrulanmadı
bool parseLbBmsE005(const twai_message_t& msg, TelemetryData& out) {
    (void)out;
    // TODO: alan anlamı doğrulanmadı, ham byte'lar loglanıyor
    return msg.data_length_code > 0;
}

// 0xE032 — TODO: alan anlamı doğrulanmadı
bool parseLbBmsE032(const twai_message_t& msg, TelemetryData& out) {
    (void)out;
    // TODO: alan anlamı doğrulanmadı, ham byte'lar loglanıyor
    return msg.data_length_code > 0;
}

// 0xE033 — TODO: alan anlamı doğrulanmadı
bool parseLbBmsE033(const twai_message_t& msg, TelemetryData& out) {
    (void)out;
    // TODO: alan anlamı doğrulanmadı, ham byte'lar loglanıyor
    return msg.data_length_code > 0;
}

BmsPackVoltageFault checkPackVoltageFault(uint16_t packVoltageDeciV,
                                          uint16_t criticalMinDeciV,
                                          uint16_t criticalMaxDeciV) {
    if (packVoltageDeciV <= criticalMinDeciV)
        return BmsPackVoltageFault::UNDERVOLTAGE;
    if (packVoltageDeciV >= criticalMaxDeciV)
        return BmsPackVoltageFault::OVERVOLTAGE;
    return BmsPackVoltageFault::NONE;
}

bool isMotorStatusTimedOut(bool hasSeen,
                           bool lastValid,
                           TickType_t now,
                           TickType_t lastTick,
                           TickType_t timeoutTicks) {
    if (!hasSeen || !lastValid)
        return false;
    return static_cast<TickType_t>(now - lastTick) >= timeoutTicks;
}

bool isBmsStatusTimedOut(bool hasSeen,
                         bool lastValid,
                         TickType_t now,
                         TickType_t lastTick,
                         TickType_t timeoutTicks) {
    if (!hasSeen || !lastValid)
        return false;
    return static_cast<TickType_t>(now - lastTick) >= timeoutTicks;
}

}  // namespace CanParse