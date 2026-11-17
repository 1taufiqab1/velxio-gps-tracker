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
| Broker | `test.mosquitto.org` |
| Port | `1884` |
| Username | `rw` |
| Password | `readwrite` |
| Keep Alive | 60 detik |
| Device ID | `esp32:001` |

### MQTT Topics

| Topic | Fungsi |
|---|---|
| `tracker/devices/esp32:001/telemetry` | Mengirim data GPS |
| `tracker/devices/esp32:001/command` | Menerima perintah |

### Konfigurasi Secrets

Kredensial WiFi dan MQTT dipisahkan ke file `secrets.h`, sehingga tidak bercampur dengan logika utama program.

Konfigurasi yang digunakan saat simulasi:

| Variabel | Nilai |
|---|---|
| WiFi SSID | `Velxio-GUEST` |
| WiFi Password | `""` |
| MQTT Server | `test.mosquitto.org` |
| MQTT Port | `1884` |
| MQTT Username | `rw` |
| MQTT Password | `readwrite` |

## 4. Fitur Utama

### A. GPS Tracking

ESP32 membaca data dari modul GPS menggunakan library TinyGPSPlus.

- Latitude: garis lintang.
- Longitude: garis bujur.
- Speed: kecepatan dalam knot.
- Timestamp: waktu Unix dalam detik.

### B. Pengiriman Data MQTT

Perangkat mengirimkan data GPS secara berkala ke topic telemetry.

Interval awal pengiriman adalah:

```cpp
bool gps_enabled = false;
int interval_seconds = 10;
```

GPS secara default dalam kondisi OFF, sehingga belum mengirimkan telemetry sampai diaktifkan melalui MQTT.

Interval pengiriman yang diperbolehkan adalah:

- 2 detik
- 5 detik
- 10 detik
- 30 detik

Contoh payload telemetry:

```json
{
  "device_id": "esp32:001",
  "lat": -7.257500,
  "lng": 112.752100,
  "speed": 2.1,
  "timestamp": 1794700000
}
```

Field `timestamp` dikirim apabila tanggal dan waktu GPS valid.

### C. Pengujian Koneksi MQTT

Pengujian dapat dilakukan menggunakan HiveMQ WebSocket Client:

```url
https://www.hivemq.com/demos/websocket-client/
```

Konfigurasi koneksi:

| Konfigurasi | Nilai |
|---|---|
| Host | `test.mosquitto.org` |
| Port | `8091` |
| Username | `rw` |
| Password | `readwrite` |

Untuk koneksi ESP32 menggunakan MQTT TCP, tetap gunakan port `1884`.

Untuk melihat komunikasi perangkat, lakukan subscribe ke topic:

```text
tracker/devices/esp32:001/#
```

Untuk mengirim command ke ESP32, gunakan topic:

```text
tracker/devices/esp32:001/command
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
  "device_id": "esp32:001",
  "active": true
}
```

Contoh menonaktifkan GPS:

```json
{
  "device_id": "esp32:001",
  "active": false
}
```

Interval pengiriman juga dapat diubah melalui JSON.

Contoh mengubah interval menjadi 5 detik:

```json
{
  "device_id": "esp32:001",
  "active": true,
  "interval_seconds": 5
}
```

Nilai `interval_seconds` hanya menerima integer `2`, `5`, `10`, dan `30`.

Perintah JSON dapat menyertakan `device_id`. Apabila `device_id` tidak sesuai dengan identitas perangkat, perintah akan diabaikan.

## 5. Indikator LED

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

## 6. Alur Kerja Sistem

1. ESP32 melakukan inisialisasi hardware.
2. ESP32 menghubungkan ke WiFi.
3. ESP32 menginisialisasi MQTT dan menghubungkan ke broker.
4. Setelah terhubung, ESP32 melakukan subscribe ke topic command.
5. ESP32 membaca dan memproses data GPS ketika GPS diaktifkan.
6. Ketika lokasi valid dan interval terpenuhi, data dikirim ke MQTT.
7. ESP32 menerima perintah untuk mengubah status GPS atau interval pengiriman.
8. LED diperbarui sesuai kondisi WiFi, MQTT, dan GPS.

## 7. Catatan

- Menggunkaan broker public jadi tidak privacy.
- Field data GPS yang dikirim melalui telemetry adalah latitude, longitude, speed, dan timestamp (opsional), tanpa satellites karena batasan simulasi Velxio.
- Di Velxio nggak ada Wemos D1 Mini jadi belum nyoba pakai MCU itu.
- Tidak menggunakan library Geolinker seperti di artikel kemarin karena kita tidak pakai API mereka.
- Interval default pengiriman adalah 10 detik dan dapat diubah menjadi 2, 5, 10, atau 30 detik.
- Ketika OFF, ESP32 berhenti memproses dan mengirim data GPS ke MQTT, tetapi modul GPS NEO-6M tetap menyala dan mengirimkan data.

## 8. Update

- Ubah Device ID menjadi `esp32:001`.
- Menyesuaikan field telemetry agar sesuai dengan schema backend.
- Hapus topik status MQTT.
- Sesuaikan nilai interval dengan ketentuan backend dengan default 10 detik.
- Tambah password untuk MQTT dan WiFi.
- Refaktor dikit kode agar mudah dibaca.
