#include "DisplayHMI.h"
#include "SystemConfig.h"
#include "driver/uart.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstdio>
#include <cstring>

static constexpr const char *TAG = "DisplayHMI";

DisplayHMI::DisplayHMI()
    : HMI_isInitialized(false),
      HMI_hasCachedScreen(false),
      HMI_lastScreenData({}),
      HMI_rxState({}),
      HMI_nextionResetPending(false) {}

bool DisplayHMI::begin() {
    if (HMI_isInitialized) return true;

    uart_config_t HMI_uartConfig = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .rx_flow_ctrl_thresh = 0,
        .source_clk = UART_SCLK_DEFAULT
    };

    if (uart_param_config(HMI_UART_NUM, &HMI_uartConfig) != ESP_OK) {
        ESP_LOGE(TAG, "UART param config failed");
        return false;
    }

    if (uart_set_pin(HMI_UART_NUM, HMI_TX_PIN, HMI_RX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE) != ESP_OK) {
        ESP_LOGE(TAG, "UART set pin failed (TX=%d, RX=%d)", HMI_TX_PIN, HMI_RX_PIN);
        return false;
    }

    if (uart_driver_install(HMI_UART_NUM, 256, 256, 0, nullptr, 0) != ESP_OK) {
        ESP_LOGE(TAG, "UART driver install failed");
        return false;
    }

    HMI_isInitialized = true;

    // §9.4.a.vii (ESP power-cycle): ESP resetlendiğinde bu nesne yeniden
    // kurulur (cache boş) ama begin() tekrar çağrılırsa da cache'i açıkça
    // yok say — sonraki updateScreen TÜM alanları force-refresh ile basar.
    HMI_hasCachedScreen = false;
    HMI_rxState = HMI_RxState{};
    HMI_nextionResetPending = false;

    // Nextion açılış mesajlarını temizle
    HMI_drainRxBuffer();

    // Nextion acknowledge yanıtlarını kapat (bkcmd=0)
    // Aksi halde her komut sonrası gelen 0x01/0x02/0x03 yanıtları
    // readTouchCommand tarafından sahte komut olarak yorumlanır
    const char *HMI_bkcmd = "bkcmd=0";
    uart_write_bytes(HMI_UART_NUM, HMI_bkcmd, 7);
    HMI_sendEndBytes();
    vTaskDelay(pdMS_TO_TICKS(50));  // Nextion'ın işlemesi için bekle
    HMI_drainRxBuffer();            // bkcmd komutunun kendi acknowledge'ını temizle

    ESP_LOGI(TAG, "Initialized on UART%d (TX=IO%d, RX=IO%d)", HMI_UART_NUM, HMI_TX_PIN, HMI_RX_PIN);
    return true;
}

void DisplayHMI::HMI_drainRxBuffer() {
    uint8_t HMI_drainBuf[32];
    while (uart_read_bytes(HMI_UART_NUM, HMI_drainBuf, sizeof(HMI_drainBuf), 0) > 0) {
        // Nextion acknowledge/error yanıtlarını temizle
    }
}

void DisplayHMI::updateScreen(const HMI_DisplayData& HMI_data) {
    if (!HMI_isInitialized) return;

    const bool HMI_forceRefresh = !HMI_hasCachedScreen;
    char HMI_currentErrorText[16];
    char HMI_lastErrorText[16];

    HMI_formatErrorText(HMI_data.HMI_motorErrorFlags, HMI_currentErrorText,
                        sizeof(HMI_currentErrorText));
    HMI_formatErrorText(HMI_lastScreenData.HMI_motorErrorFlags,
                        HMI_lastErrorText, sizeof(HMI_lastErrorText));

    // §8.2.a.iv sentinel-taşıyan alanlar (bat/temp/packi) Text objesidir
    // (HMI_Field_Map.md v2, sentinel politikası B): sihirli sayı (255/-127/
    // INT16_MIN) ekrana sızmasın diye AKS burada "--"/metin formatlar.
    char HMI_curBatText[8],  HMI_lastBatText[8];
    char HMI_curTempText[8], HMI_lastTempText[8];
    char HMI_curCurrText[10], HMI_lastCurrText[10];
    HMI_formatBatteryText(HMI_data.HMI_currentBattery, HMI_curBatText,
                          sizeof(HMI_curBatText));
    HMI_formatBatteryText(HMI_lastScreenData.HMI_currentBattery, HMI_lastBatText,
                          sizeof(HMI_lastBatText));
    HMI_formatTempText(HMI_data.HMI_bmsTemperatureC, HMI_curTempText,
                       sizeof(HMI_curTempText));
    HMI_formatTempText(HMI_lastScreenData.HMI_bmsTemperatureC, HMI_lastTempText,
                       sizeof(HMI_lastTempText));
    HMI_formatCurrentText(HMI_data.HMI_packCurrentDeciA, HMI_curCurrText,
                          sizeof(HMI_curCurrText));
    HMI_formatCurrentText(HMI_lastScreenData.HMI_packCurrentDeciA,
                          HMI_lastCurrText, sizeof(HMI_lastCurrText));

    HMI_sendNumericIfChanged("speed", HMI_data.HMI_currentSpeed,
                             HMI_lastScreenData.HMI_currentSpeed,
                             HMI_forceRefresh);
    // §8.2.a.iv SoC — Text ("--" / "0".."100")
    HMI_sendTextIfChanged("bat", HMI_curBatText, HMI_lastBatText,
                          HMI_forceRefresh);
    HMI_sendNumericIfChanged("rpm", HMI_data.HMI_motorRpm,
                             HMI_lastScreenData.HMI_motorRpm,
                             HMI_forceRefresh);
    HMI_sendNumericIfChanged("torque", HMI_data.HMI_motorTorqueFeedback,
                             HMI_lastScreenData.HMI_motorTorqueFeedback,
                             HMI_forceRefresh);
    // §8.2.a.iv sıcaklık — Text ("--" / "-40".."125")
    HMI_sendTextIfChanged("temp", HMI_curTempText, HMI_lastTempText,
                          HMI_forceRefresh);
    // §8.2.a.iv gerilim — Number (deciV); DOĞRULANDI, gating yok
    HMI_sendNumericIfChanged("packv", HMI_data.HMI_bmsPackVoltageDeciV,
                             HMI_lastScreenData.HMI_bmsPackVoltageDeciV,
                             HMI_forceRefresh);
    // §8.2.a.iv akım — Text ("--" / "±d.d"). İşaret gösterimi txt ile çözülür
    // (Prompt 1 sözleşmesi B); kaynak HIPOTEZ olduğundan main.cpp sentinel
    // besler ve ekran "--" gösterir.
    HMI_sendTextIfChanged("packi", HMI_curCurrText, HMI_lastCurrText,
                          HMI_forceRefresh);

    HMI_sendTextIfChanged("state", HMI_getStateText(HMI_data.HMI_vcuState),
                          HMI_getStateText(HMI_lastScreenData.HMI_vcuState),
                          HMI_forceRefresh);
    HMI_sendTextIfChanged("motorErr", HMI_currentErrorText, HMI_lastErrorText,
                          HMI_forceRefresh);
    HMI_sendTextIfChanged("valid",
                          HMI_getValidityText(HMI_data.HMI_motorDataValid,
                                              HMI_data.HMI_motorTimeoutActive),
                          HMI_getValidityText(
                              HMI_lastScreenData.HMI_motorDataValid,
                              HMI_lastScreenData.HMI_motorTimeoutActive),
                          HMI_forceRefresh);
    HMI_sendTextIfChanged("contactor",
                          HMI_getContactorText(HMI_data.HMI_contactorClosed),
                          HMI_getContactorText(
                              HMI_lastScreenData.HMI_contactorClosed),
                          HMI_forceRefresh);

    HMI_lastScreenData = HMI_data;
    HMI_hasCachedScreen = true;
}

bool DisplayHMI::readTouchCommand(uint8_t& HMI_command) {
    if (!HMI_isInitialized) return false;

    // --- HMI Command Frame Format ---
    // Dokunmatik komut 3-byte çerçeve: [0x5A] [KOMUT] [~KOMUT] (checksum).
    // Örnek START: 0x5A 0x01 0xFE. Sınıflandırma (touch vs Nextion sistem
    // byte'ları) HMI_rxClassifyByte'ta (saf, native testli) yapılır — böylece
    // 0x88 / 00 00 00 FF FF FF gibi sistem yanıtları touch komutuyla KARIŞMAZ.

    uint8_t HMI_rxByte;
    // pdMS_TO_TICKS(10) ile en az 1 byte bekler; ardından buffer'daki kalanları
    // 0 timeout ile çeker (böylece komut + reset marker aynı taramada yakalanır).
    int HMI_rxBytes =
        uart_read_bytes(HMI_UART_NUM, &HMI_rxByte, 1, pdMS_TO_TICKS(10));
    if (HMI_rxBytes <= 0) return false;

    bool HMI_gotCommand = false;

    do {
        uint8_t HMI_cmd = 0;
        const HMI_RxEvent HMI_ev =
            HMI_rxClassifyByte(HMI_rxState, HMI_rxByte, HMI_cmd);

        if (HMI_ev == HMI_RxEvent::NEXTION_RESET) {
            // §9.4.a.vii: Nextion power-on/reset → tam ekran yeniden basımı.
            // Ana sayfa: cache'i sıfırla (sonraki updateScreen force-refresh).
            // BMS sayfası: main.cpp'nin takeNextionResetFlag() ile göreceği bayrak.
            HMI_hasCachedScreen = false;
            HMI_nextionResetPending = true;
            ESP_LOGW(TAG,
                     "Nextion reset algilandi (0x88/startup) — tam ekran "
                     "yeniden gonderilecek");
        } else if (HMI_ev == HMI_RxEvent::TOUCH && !HMI_gotCommand) {
            HMI_command = HMI_cmd;
            HMI_gotCommand = true;
            // return ETME: aynı buffer'da reset marker da olabilir, taramayı
            // sürdürüp onu da yakala (touch nadir; ikinci touch pratikte gelmez).
        }
    } while (uart_read_bytes(HMI_UART_NUM, &HMI_rxByte, 1, 0) == 1);

    return HMI_gotCommand;
}

bool DisplayHMI::takeNextionResetFlag() {
    const bool HMI_pending = HMI_nextionResetPending;
    HMI_nextionResetPending = false;
    return HMI_pending;
}
