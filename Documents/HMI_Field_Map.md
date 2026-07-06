# HMI Field Map — AKS ↔ Nextion Sözleşmesi (v2)

Bu doküman AKS firmware'i ile Nextion ekran projesi (`EV_Dashboard_v01.HMI`)
arasındaki TEK sözleşmedir. Nextion tarafındaki `objname`'ler ve firmware'in
gönderdiği komutlar birebir bu tabloya uyar. Değişiklik önce burada yapılır,
sonra iki tarafa uygulanır.

- Şartname dayanağı: Bölüm 3 §8.2.a.iv — "batarya gerilimi, akımı ve sıcaklığı,
  SoC bilgisinin sürücü ekranında gösterilmesi gerekir" + Bölüm 2 §9.18 (hız).
- Sürüm: v2 (packi eklendi, sentinel politikası AKS-tarafı txt olarak sabitlendi,
  BMS sayfası objeleri haritaya alındı).

## Sayfa / Kapsam Mimarisi

| Sayfa | Nextion adı | İçerik |
| --- | --- | --- |
| page0 | `main` | Sürücü gösterge paneli (hız, SoC, akım, gerilim, sıcaklık, durum) |
| page1 | `bms` | 24 hücre grid'i + özetler (cell/j/bal, cellmax/cellmin/warn) |

Kurallar:
- Veri taşıyan TÜM objeler `vscope = global` olmalıdır (sayfa değişse de değer korunur).
- Firmware komutları **sayfa önekli** gönderir: `main.speed.val=...`,
  `bms.cell0.val=...`. (Prompt 3 firmware işi; öneksiz komut yalnız aktif
  sayfadaki objeye ulaşır, iki sayfalı tasarımda güvenilmez.)
- Öneklerin byte maliyeti: `buildBmsNextionCommands` çağrısındaki `maxBytes`
  bütçesi (şu an 90) `"bms."` öneki (+4 byte/komut) hesaba katılarak artırılmalıdır.
- Nextion baud: **9600** (firmware `DisplayHMI::begin` sabiti). Değiştirilmez.
- Firmware açılışta `bkcmd=0` gönderir; Nextion projesi bunu geçersiz kılmamalıdır.

## Runtime Veri Modeli (HMI_DisplayData)

| Firmware Alanı | Kaynak | Açıklama |
| --- | --- | --- |
| `HMI_currentSpeed` | `TEL_speedKmhX10 / 10` | Araç hızı, tam sayı km/h |
| `HMI_currentBattery` | `TEL_bmsSocHundredths` (E000 b[4:5]) | SoC %0..100; veri yok = 255 |
| `HMI_packCurrentDeciA` **(YENİ)** | `TEL_bmsCurrentCentiMa` → deciAmp | Paket akımı 0.1 A çözünürlük, işaretli; veri yok = INT16_MIN |
| `HMI_motorRpm` | `TEL_motorRpm` (0x200) | Ham motor RPM |
| `HMI_motorTorqueFeedback` | `TEL_motorTorqueFeedback` | İşaretli tork (şu an beslenmiyor — Faz 1.2) |
| `HMI_motorErrorFlags` | `TEL_motorErrorFlags` | Motor sürücü hata bayrakları |
| `HMI_motorDataValid` | `TEL_motorDataValid` | Tazelik göstergesi |
| `HMI_motorTimeoutActive` | `TEL_motorTimeoutActive` | Timeout göstergesi |
| `HMI_bmsTemperatureC` | `TEL_bmsTempHighestC` (E001 b[6:7]) | En yüksek hücre sıcaklığı °C; veri yok = -127 |
| `HMI_bmsPackVoltageDeciV` | `TEL_bmsPackVoltageDeciV` (E000 b[2:3]) | Paket gerilimi deciV (1004 = 100.4 V) |
| `HMI_contactorClosed` | `RelayManager::getRelayState()` | Tüm pozitif kontaktörler kapalıysa true |
| `HMI_vcuState` | `VcuLogic::getState()` | Güncel VCU durumu |

> AKIM BİRİM NOTU (P1): `TEL_bmsCurrentCentiMa` zincirindeki 1000× ölçek
> belirsizliği (CanParse yorumu "raw ×0.1 A" ↔ alan adı "centiMa") bench'te
> çözülmeden `packi` alanı YANLIŞ değer gösterebilir. Ekrana bağlamadan önce
> tek birim sabitlenecek; bu haritanın kabul ettiği ekran birimi **deciAmp**tir.

## Nextion Obje Sözleşmesi — `main` sayfası (page0)

| Obje | Tip | Komut | Değer aralığı / format | Sentinel |
| --- | --- | --- | --- | --- |
| `speed` | Number | `main.speed.val=<int>` | 0..300 km/h (UKS clamp x10=3000 ile uyumlu) | yok (motor CAN kesilirse son değer; bkz. P3-2) |
| `bat` | Text | `main.bat.txt="<0..100>"` | "%%" işareti Nextion'da statik etiket | `"--"` (SoC kaynak geçersiz / BMS timeout) |
| `packi` | Text **(YENİ)** | `main.packi.txt="<±d.d>"` | -300.0..300.0 A, 1 ondalık; işaret: + deşarj / − şarj (bench teyidi bekliyor) | `"--"` |
| `temp` | Text | `main.temp.txt="<int>"` | -40..125 °C | `"--"` |
| `packv` | Number | `main.packv.val=<deciV>` | 0..1500 (0..150.0 V, B2 §9.2.k limiti) — Nextion gösterimi xfloat önerilir (bkz. not) | yok (v2 kapsamı dışı; v3 adayı) |
| `rpm` | Number | `main.rpm.val=<int>` | 0..6000 | yok |
| `torque` | Number | `main.torque.val=<int>` | işaretli; şu an daima 0 (Faz 1.2) | yok |
| `state` | Text | `main.state.txt="..."` | `INIT/IDLE/READY/DRIVE/ESTOP/FAULT` | — |
| `motorErr` | Text | `main.motorErr.txt="0xNN"` | hex bayrak | — |
| `valid` | Text | `main.valid.txt="..."` | `VALID/INVALID/TIMEOUT` | — |
| `contactor` | Text | `main.contactor.txt="..."` | `CLOSED/OPEN` | — |
| `jbat` | Progress bar (opsiyonel) | `main.jbat.val=<0..100>` | SoC bar; sentinel durumunda 0 | 0 (boş bar) |

`packv` notu: Number objesi ham deciV gösterir (1004). Sürücü dostu gösterim
için obje **xfloat** yapılıp `vvs1=1` ayarlanırsa aynı `packv.val=<deciV>`
komutu "100.4" olarak render edilir — firmware değişikliği GEREKMEZ. Tercih: xfloat.

## Nextion Obje Sözleşmesi — `bms` sayfası (page1)

| Obje | Tip | Komut | Değer / format | Sentinel |
| --- | --- | --- | --- | --- |
| `cell0`..`cell23` | Number | `bms.cellN.val=<mV>` | hücre gerilimi mV | 65535 (`HMI_CELL_VOLTAGE_NO_DATA`) — ekranda "65535" görünür; v3'te txt'e geçiş adayı |
| `j0`..`j23` | Progress bar | `bms.jN.val=<0..100>` | hücre bar doluluğu (BMS_SOC_EMPTY_MV..FULL_MV aralığından) | 0 (sentinel mV → boş bar) |
| `bal0`..`bal23` | Number/Checkbox | `bms.balN.val=<0|1>` | dengeleme bayrağı | 0 |
| `cellmax` | Number | `bms.cellmax.val=<mV>` | en yüksek hücre mV | 65535 |
| `cellmin` | Number | `bms.cellmin.val=<mV>` | en düşük hücre mV | 65535 |
| `warn` | Number | `bms.warn.val=<0|1|2>` | 0=OK, 1=WARNING, 2=CRITICAL (BmsAlgo GÖSTERİM eşiği; VCU FAULT DEĞİL) | 2 (bayat veri CRITICAL'e çekilir) |
| `cellcan` **(YENİ)** | Number | `bms.cellcan.val=<0\|1>` | 0=per-cell CAN **doğrulanmadı** (veri yok, tüm bar 0), 1=doğrulandı (gerçek per-cell veri). warn'dan BAĞIMSIZ ayrı gösterge (madde 2) | 0 (boot: doğrulanmadı) |

> `cellcan` notu: Per-cell CAN ID'leri (0xE002-E005, E032-E033) DOĞRULANANA kadar
> `cellcan=0` kalır; Nextion projesi bunu net bir "HÜCRE CAN: DOĞRULANMADI"
> banner'ı ile göstermeli (warn rengiyle KARIŞTIRMADAN). Tek bayrak
> (`HMI_CELL_VOLTAGE_SOURCE_VERIFIED`) true olunca gerçek veri + `cellcan=1`.

## "Veri Yok" (Sentinel) Politikası — KARAR

Sentinel taşıyan alanlar (`bat`=255, `temp`=-127, `packi`=INT16_MIN) ekranda
**"--"** gösterilmelidir. İki seçenek değerlendirildi:

| | A) Nextion tarafında (timer + vis/övrl) | B) AKS tarafında txt formatlama |
| --- | --- | --- |
| Mekanizma | UART `.val` yazımı Nextion'da event TETİKLEMEZ → tm0 timer'ı (~200 ms) değerleri poll edip 255/-127'de sayı objesini gizler, "--" text'ini gösterir | Firmware alanı Text objesine `"87"` / `"--"` olarak basar |
| Artı | Firmware komut tipi değişmez; sayı objesi korunur | Tek doğruluk kaynağı firmware'de; native testlenebilir (HMIHelpers zaten 4 alanda txt üretiyor); sihirli sayılar (255/-127) ekrana asla sızmaz; format tam kontrol |
| Eksi | Sihirli sayılar iki kod tabanında da yaşar; HMI-içi mantık CI'da test edilemez; timer/görünürlük yönetimi sayfa başına elle çoğaltılır; poll gecikmesi | Biraz daha fazla UART byte'ı; sayısal widget bağlaması kaybolur (çözüm: opsiyonel `jbat` bar) |

**Seçim: B — AKS tarafında txt.** `bat`, `temp`, `packi` Text objesi olur;
firmware Prompt 3'te `HMI_sendTextIfChanged` ile besler. Geçiş notu: .HMI ve
firmware güncellemesi AYNI sürümde sahaya alınmalıdır; aksi halde eski firmware
`bat.val=...` gönderir ve yeni Text objesinde karşılıksız kalır (Nextion
`bkcmd=0` olduğundan sessizce yutulur — ekran "--"da kalır, güvenli taraf).

## Metin Formatlama Kuralları

| UI Alanı | Çıktı |
| --- | --- |
| `state` | `INIT`, `IDLE`, `READY`, `DRIVE`, `ESTOP`, `FAULT` |
| `motorErr` | `0x00`, `0x04`, `0xFF` gibi hex |
| `valid` | `VALID`, `INVALID`, `TIMEOUT` |
| `contactor` | `CLOSED`, `OPEN` |
| `bat` | `"0"`..`"100"` veya `"--"` |
| `temp` | `"-40"`..`"125"` veya `"--"` |
| `packi` | `"12.5"`, `"-8.0"` (ASCII eksi) veya `"--"` |

## Tazeleme Davranışı

- HMI refresh görevi **10 Hz** çalışır.
- Sürücü, son gönderilen snapshot'ı cache'ler; alan yalnız değiştiyse veya ekran
  ilk kez doluyorsa gönderilir.
- AÇIK İŞ (denetim P2-4): yalnız Nextion resetlenirse cache yüzünden ekran boş
  kalır — periyodik force-refresh (örn. 5 sn) veya Nextion startup-event tespiti
  Prompt 3 kapsamında eklenmelidir.

## Dokunmatik Komut Girişi (HMI → AKS)

Butonlar 3 byte'lık çerçeve gönderir: `[0x5A] [KOMUT] [~KOMUT]`
(Nextion buton Touch Release Event içinde `printh`):

| Komut ID | Anlam | printh |
| --- | --- | --- |
| `1` | `START` | `printh 5A 01 FE` |
| `2` | `RESET` | `printh 5A 02 FD` |
| `3` | `EMERGENCY_STOP` | `printh 5A 03 FC` |
| `4` | `DRIVE_ENABLE` | `printh 5A 04 FB` |

## Sürüm Geçmişi

| Sürüm | Değişiklik |
| --- | --- |
| v1 | İlk harita (10 obje, sentinel yok, BMS sayfası haritasız) |
| v2 | §8.2.a.iv uyumu: `packi` eklendi; `bat`/`temp` Text'e geçti; sentinel politikası (B) sabitlendi; `bms` sayfası objeleri (cell/j/bal ×24 + cellmax/cellmin/warn) haritaya alındı; sayfa öneki + vscope=global kuralı; 0x5A dokunmatik çerçevesi belgelendi |
