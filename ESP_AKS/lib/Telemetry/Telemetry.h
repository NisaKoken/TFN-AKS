#pragma once

#include <cstdint>

struct TelemetryData {
    uint16_t TEL_motorRpm;
    int16_t TEL_motorTorqueFeedback;
    uint8_t TEL_motorErrorFlags;
    bool TEL_motorDataValid;
    bool TEL_motorTimeoutActive;

    // Solion SK BMS — CAN ID 0x111
    uint16_t TEL_bmsCellVoltageMaxDeciMv;  // raw * 0.1 = mV
    uint16_t TEL_bmsCellVoltageMinDeciMv;  // raw * 0.1 = mV
    int8_t TEL_bmsTempHighestC;
    int8_t TEL_bmsTempLowestC;
    uint8_t TEL_bmsSystemState;  // 1=Discharge, 2=IDLE, 3=Charge, 4=FAULT

    // Solion SK BMS — CAN ID 0x112
    uint16_t TEL_bmsPackVoltageDeciV;  // raw * 0.1 = V
    int32_t TEL_bmsCurrentCentiMa;     // raw * 0.01 = mA (+charge, -discharge)
    uint16_t TEL_bmsSocHundredths;     // raw * 0.01 = %

    bool TEL_bmsDataValid;
};

class Telemetry {
   public:
    Telemetry();
    bool begin();
    void sendStatus(const TelemetryData& TEL_data);

   private:
    bool TEL_isInitialized;
    uint32_t TEL_sequenceCounter;
};
