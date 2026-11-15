# [Velxio GPS Tracker](https://velxio.dev/1taufiqab1/gps-tracker/)

![cover](https://lh3.googleusercontent.com/d/1Gfi19k2lcbCBz2L6g7iS7LXVj-fUL-bP)

## [Link Project Klik Disini](https://velxio.dev/1taufiqab1/gps-tracker/)

## 1. Komponen dan Library

### Hardware
- ESP32 DevKit V1
- GPS NEO-6M
- Breadboard
- 2 LED
- 2 Resistor

### Library Arduino
- `WiFi` — koneksi WiFi.
- `PubSubClient` — komunikasi MQTT.
- `TinyGPSPlus` — membaca dan mengolah data GPS.
- `ArduinoJson` — parsing perintah JSON.

## 2. Konfigurasi Pin

| Komponen | Pin Komponen | Pin ESP32 |
|---|---|---|
| GPS NEO-6M | VCC | 3V3 |
| | GND | GND |
| | TX | GPIO 16 (RX2) |
| | RX | GPIO 17 (TX2) |
| LED A | Anoda (+) | GPIO 32 |
| | Katoda (-) | GND (resistor) |
| LED B | Anoda (+) | GPIO 33 |
| | Katoda (-) | GND (resistor) |

## 3. Konfigurasi MQTT

### MQTT Broker
| Konfigurasi | Nilai |
|---|---|
| Broker | test.mosquitto.org |
| Port | 1883 |
| Keep Alive | 60 detik |
| Device ID | esp32-kel2iot |

### MQTT Topics

| Topic | Fungsi |
|---|---|
| `tracker/devices/esp32-kel2iot/telemetry` | Mengirim data GPS |
| `tracker/devices/esp32-kel2iot/command` | Menerima perintah |
| `tracker/devices/esp32-kel2iot/status` | Mengirim status perangkat |

## 4. Fitur Utama

### A. GPS Tracking

ESP32 membaca data dari modul GPS menggunakan library TinyGPSPlus.

- Latitude: garis lintang.
- Longitude: garis bujur.
- Altitude: ketinggian dalam meter.
- Speed: kecepatan dalam knot.
- Timestamp: waktu Unix dalam detik.

### B. Pengiriman Data MQTT

Perangkat mengirimkan data GPS secara berkala ke topic telemetry.

Interval awal pengiriman adalah:

```cpp
float interval_seconds = 0.666;
```

Artinya, pengiriman dilakukan sekitar setiap 666 milidetik.

Contoh payload telemetry:

```json
{
  "device_id": "esp32-kel2iot",
  "lat": -7.257500,
  "lng": 112.752100,
  "alt_m": 12.5,
  "speed_kn": 2.1,
  "timestamp": 1794700000
}
```

### C. Pengujian Koneksi MQTT

Pengujian dapat dilakukan menggunakan HiveMQ WebSocket Client:

```url
https://www.hivemq.com/demos/websocket-client/
```

Konfigurasi koneksi:

| Konfigurasi | Nilai |
|---|---|
| Host | `test.mosquitto.org` |
| Port | `8081` |

Klik **Connect** untuk menghubungkan ke MQTT Broker.

Untuk melihat seluruh komunikasi perangkat, lakukan subscribe ke topic:

```text
tracker/devices/esp32-kel2iot/#
```

Untuk mengirim command ke ESP32, gunakan topic:

```text
tracker/devices/esp32-kel2iot/command
```

**Mengaktifkan GPS:**

```text
1
```

atau:

```text
true
```

**Menonaktifkan GPS:**

```text
0
```

atau:

```text
false
```

GPS juga dapat dikontrol menggunakan JSON.

Contoh mengaktifkan GPS:

```json
{
  "active": true
}
```

Contoh menonaktifkan GPS:

```json
{
  "active": false
}
```

Perintah JSON dapat menyertakan `device_id`.

Contoh:

```json
{
  "device_id": "esp32-kel2iot",
  "active": true
}
```

Apabila `device_id` tidak sesuai dengan identitas perangkat, perintah akan diabaikan.

## 5. Status Perangkat

Perangkat mengirimkan status melalui topic status.

Contoh payload:

```json
{
  "device_id": "esp32-kel2iot",
  "active": true,
  "interval_seconds": 0.666
}
```

Jika terjadi kesalahan, status dapat menyertakan field `error`.

Contoh:

```json
{
  "device_id": "esp32-kel2iot",
  "active": true,
  "interval_seconds": 0.666,
  "error": "Invalid interval, ignored"
}
```

## 6. Indikator LED

Terdapat dua LED untuk menunjukkan kondisi perangkat.

### LED A — WiFi dan MQTT

| Kondisi | Indikator |
|---|---|
| WiFi terputus | Berkedip cepat |
| WiFi terhubung, MQTT terputus | Berkedip lambat |
| WiFi dan MQTT terhubung | Menyala terus |

### LED B — GPS

| Kondisi | Indikator |
|---|---|
| GPS dinonaktifkan | Mati |
| GPS belum mendapatkan lokasi valid | Berkedip lambat |
| GPS mendapatkan lokasi valid | Menyala |
| Data berhasil dikirim | Mati sesaat sebagai indikator pengiriman |

## 7. Alur Kerja Sistem

1. ESP32 menghubungkan ke WiFi dan MQTT Broker.
2. Setelah terhubung, ESP32 melakukan subscribe ke topic command.
3. ESP32 membaca dan memproses data GPS.
4. Ketika lokasi valid dan interval terpenuhi, data dikirim ke MQTT.
5. ESP32 menerima perintah untuk mengubah status GPS atau interval pengiriman.
6. LED diperbarui sesuai kondisi WiFi, MQTT, dan GPS.

## 8. Catatan 

- Menggunkaan broker public jadi tidak privacy.
- Field data geo GPS yang dikirim hanya latitude, longitude, altitude, dan speed karena batasan simulasi GPS di Velxio hanya ada itu.
- Di Velxio nggak ada Wemos D1 Mini jadi belum nyoba pakai MCU itu.
- Tidak menggunakan library Geolinker seperti di artikel kemarin karena kita tidak pakai API mereka.
- Disini kirim data setiap 666 ms which is terlalu cepat, karena simulasi di Velxio waktunya lebih lambat +-3x lipat dunia nyata, jadi 666 ms velxio ≈ 2 detik dunia nyata. Untuk real iot nanti bisa diubah menjadi 2 detik.
- Ketika OFF, ESP32 berhenti memproses dan mengirim data GPS ke MQTT, tetapi modul GPS NEO-6M tetap menyala dan mengirimkan data.
