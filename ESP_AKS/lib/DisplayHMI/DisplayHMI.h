#pragma once
#include <cstddef>
#include <cstdint>

#include "HMIHelpers.h"

// HMI_VcuState ve saf metin/komut helper'ları HMIHelpers.h'ye taşındı; bu
// header geriye uyumluluk için onu yeniden expose eder. DisplayHMI sınıfı
// yalnızca init / cache / RX state'ini yönetir.
struct HMI_DisplayData {
    uint16_t HMI_currentSpeed;
    // --- §8.2.a.iv: BMS'in dört zorunlu sürücü-yüzü değeri ---
    // Dördü de VCU state'ten BAĞIMSIZ doldurulur (bkz. main.cpp vTask_HMI_Display),
    // yani hem DRIVE hem şarj modunda ekranda kalır.
    // bat/temp/packi Text objesidir (HMI_Field_Map.md v2, sentinel politikası B):
    // struct'ta sayısal-veya-sentinel tutulur, updateScreen bunu "--"/metin yapar.
    uint8_t HMI_currentBattery;         // SoC (%0-100, sentinel HMI_BATTERY_NO_DATA=255)
    int16_t HMI_bmsTemperatureC;        // En yüksek sıcaklık (°C, sentinel HMI_TEMP_NO_DATA=-127)
    uint16_t HMI_bmsPackVoltageDeciV;   // Pack gerilimi (0.1 V) — DOĞRULANDI (Number obje)
    int16_t HMI_packCurrentDeciA;       // Pack akımı (işaretli 0.1 A, sentinel HMI_CURRENT_NO_DATA=INT16_MIN)
    // --- Motor / durum alanları ---
    uint16_t HMI_motorRpm;
    int16_t HMI_motorTorqueFeedback;
    uint8_t HMI_motorErrorFlags;
    bool HMI_motorDataValid;
    bool HMI_motorTimeoutActive;
    bool HMI_contactorClosed;
    HMI_VcuState HMI_vcuState;
};

class DisplayHMI {
   private:
    bool HMI_isInitialized;
    bool HMI_hasCachedScreen;
    HMI_DisplayData HMI_lastScreenData;

    void HMI_drainRxBuffer();

   public:
    DisplayHMI();
    bool begin();
    void updateScreen(const HMI_DisplayData& HMI_data);
    bool readTouchCommand(uint8_t& HMI_command);
};
