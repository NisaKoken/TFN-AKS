# CAN Message Table

This table documents the CAN frames currently implemented or reserved by the AKS firmware.

## Motor Driver Frames

### `0x100` Torque Command

Direction: `AKS -> Motor Driver`

| Byte | Field | Type | Scale | Description |
| --- | --- | --- | --- | --- |
| 0 | Torque MSB | `uint8_t` | raw | High byte of torque command |
| 1 | Torque LSB | `uint8_t` | raw | Low byte of torque command |

Notes:

- The CAN task transmits torque at the control loop rate.
- When VCU state is not `DRIVE`, torque is forced to `0` for safety.

### `0x200` Motor Status

Direction: `Motor Driver -> AKS`

| Byte | Field | Type | Scale | Description |
| --- | --- | --- | --- | --- |
| 0 | RPM MSB | `uint8_t` | raw | High byte of motor RPM |
| 1 | RPM LSB | `uint8_t` | raw | Low byte of motor RPM |
| 2 | Torque Feedback MSB | `uint8_t` | raw | High byte of signed torque feedback |
| 3 | Torque Feedback LSB | `uint8_t` | raw | Low byte of signed torque feedback |
| 4 | Error Flags | `uint8_t` | bitfield | Motor driver fault / warning flags |

Freshness rule:

- If no valid `0x200` frame is received for `500 ms`, AKS marks motor data invalid.
- A post-reception motor timeout is treated as a critical safety condition outside `IDLE`.

## Solion SK Serisi BMS Frames

Source: Solion SK Serisi Akü Kontrol Kartı (BMS) LB-LT Can 2.0b Haberleşme Föyü V1.4

Protocol parameters:
- ID Format: **29-bit Extended** (`CAN 2.0b`)
- Byte Order: **Big Endian**
- Standard CAN speed: **125 kbps** (configurable: 125 / 250 / 500 / 1000 kbps)
- 120 Ω termination resistor: **not included** in BMS

### `0x111` BMS-A — Cell Voltages, Temperatures, System State

Direction: `BMS -> AKS` | Period: 250 ms (LT) / 100 ms (LB)

| Byte | Field | TelemetryData | Type | Scale | Range | Description |
| --- | --- | --- | --- | --- | --- | --- |
| 0–1 | Cell Voltage Max | `TEL_bmsCellVoltageMaxDeciMv` | `uint16_t` | × 0.1 mV | 0–65535 | Maximum cell voltage |
| 2–3 | Cell Voltage Min | `TEL_bmsCellVoltageMinDeciMv` | `uint16_t` | × 0.1 mV | 0–65535 | Minimum cell voltage |
| 4 | Highest Cell Temp | `TEL_bmsTempHighestC` | `int8_t` | 1 °C | −128–127 | Hottest cell temperature |
| 5 | Lowest Cell Temp | `TEL_bmsTempLowestC` | `int8_t` | 1 °C | −128–127 | Coldest cell temperature |
| 6 | System State | `TEL_bmsSystemState` | `uint8_t` | state code | 1–4 | 1=Discharge, 2=IDLE, 3=Charge, 4=FAULT |

Doc examples: CellVMax `0x9366` = 3773.4 mV · CellVMin `0x922E` = 3742.2 mV · TempH `0x20` = 32 °C · State `0x03` = Charge

Safety notes:

- `TEL_bmsSystemState == 4` (FAULT) is treated as a critical condition and fires `FAULT_DETECTED` via the CAN event callback.
- `TEL_bmsTempHighestC` is used for over-temperature threshold checks in VcuLogic.

### `0x112` BMS-B — Pack Voltage, Pack Current, SOC

Direction: `BMS -> AKS` | Period: 250 ms (LT) / 100 ms (LB)

| Byte | Field | TelemetryData | Type | Scale | Range | Description |
| --- | --- | --- | --- | --- | --- | --- |
| 0–1 | Pack Voltage | `TEL_bmsPackVoltageDeciV` | `uint16_t` | × 0.1 V | 0–65535 | Total pack voltage |
| 2–5 | Pack Current | `TEL_bmsCurrentCentiMa` | `int32_t` | × 0.01 mA | ±2 147 483 647 | Pack current (+charge / −discharge) |
| 6–7 | SOC | `TEL_bmsSocHundredths` | `uint16_t` | × 0.01 % | 0–65535 | State of Charge |

Doc examples: PackV `0x020E` = 52.6 V · Current `0xFFFD3A96` = −1816.1 mA · SOC `0x188B` = 62.83 %

## Legacy / Reserved

### `0x300` Legacy BMS Status

Direction: `BMS -> AKS`

Status: reserved for backward compatibility logging only. The current firmware does not parse payload fields from this frame.
