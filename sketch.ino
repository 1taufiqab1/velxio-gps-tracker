#include <WiFi.h>
#include <PubSubClient.h>
#include <TinyGPSPlus.h>
#include <ArduinoJson.h>

#define ESP32_RX2_PIN 16
#define ESP32_TX2_PIN 17
#define GPS_BAUD 9600
#define LED_A_PIN 32
#define LED_B_PIN 33
#define DEVICE_ID "esp32-kel2iot"

const char* wifi_ssid          = "Velxio-GUEST";
const char* mqtt_server        = "test.mosquitto.org";
const int   mqtt_port          = 1883;
const char* mqtt_topic         = "tracker/devices/" DEVICE_ID "/telemetry";
const char* mqtt_command_topic = "tracker/devices/" DEVICE_ID "/command";
const char* mqtt_status_topic  = "tracker/devices/" DEVICE_ID "/status";
const int   mqtt_keep_alive    = 60;
const int   msg_size           = 160;
const int   cmd_size           = 128;
const int   blink_fast_ms      = 150;
const int   blink_slow_ms      = 500;
const int   blip_ms            = 100;

HardwareSerial gps_serial(2);
TinyGPSPlus gps;
WiFiClient wifi_client;
PubSubClient mqtt(wifi_client);

unsigned long last_send = 0;
unsigned long last_publish_ok = 0;
bool gps_enabled = true;
float interval_seconds = 0.666;

unsigned long gps_epoch() {
  int y = gps.date.year();
  int m = gps.date.month();
  int d = gps.date.day();

  if (m <= 2) y--;
  int era = y / 400;
  int yoe = y - era * 400;
  int doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
  int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
  unsigned long days = era * 146097UL + doe - 719468UL;

  return days * 86400UL + gps.time.hour() * 3600UL +
         gps.time.minute() * 60UL + gps.time.second();
}

bool blink_on(unsigned long period_ms) {
  return (millis() / period_ms) % 2;
}

void update_leds() {
  bool led_a;
  if (WiFi.status() != WL_CONNECTED) led_a = blink_on(blink_fast_ms);
  else if (!mqtt.connected())        led_a = blink_on(blink_slow_ms);
  else                               led_a = true;

  bool led_b;
  if (!gps_enabled)                  led_b = false;
  else if (!gps.location.isValid())  led_b = blink_on(blink_slow_ms);

  else led_b = !(last_publish_ok != 0 && millis() - last_publish_ok < blip_ms);

  digitalWrite(LED_A_PIN, led_a);
  digitalWrite(LED_B_PIN, led_b);
}

void send_status(const char* error) {
  char status[msg_size];

  int len = snprintf(status, sizeof(status),
                     "{\"device_id\":\"%s\",\"active\":%s,\"interval_seconds\":%.3f",
                     DEVICE_ID, gps_enabled ? "true" : "false", interval_seconds);

  if (error) {
    len += snprintf(status + len, sizeof(status) - len,
                    ",\"error\":\"%s\"", error);
  }

  snprintf(status + len, sizeof(status) - len, "}");
  mqtt.publish(mqtt_status_topic, status);
}

void publish_status() {
  send_status(nullptr);
}

void set_active(bool on) {
  if (on == gps_enabled) return;
  if (on) gps = TinyGPSPlus();
  gps_enabled = on;
  Serial.println(on ? "GPS ON" : "GPS OFF");
}

void on_message(char* topic, byte* payload, unsigned int length) {
  char cmd[cmd_size];

  unsigned int n = length < sizeof(cmd) - 1 ? length : sizeof(cmd) - 1;
  memcpy(cmd, payload, n);

  cmd[n] = '\0';

  if (strcmp(cmd, "1") == 0 || strcasecmp(cmd, "true") == 0) {
    set_active(true);
    publish_status();
    return;
  }
  if (strcmp(cmd, "0") == 0 || strcasecmp(cmd, "false") == 0) {
    set_active(false);
    publish_status();
    return;
  }

  StaticJsonDocument<256> doc;
  DeserializationError err = deserializeJson(doc, cmd);

  if (err || !doc.is<JsonObject>()) {
    Serial.println("Invalid command");
    send_status("Invalid command");
    return;
  }

  if (!doc["device_id"].isNull()) {
    const char* id = doc["device_id"] | "";
    if (strcmp(id, DEVICE_ID) != 0) {
      Serial.println("Command ignored: device_id mismatch");
      send_status("Command ignored: device_id mismatch");
      return;
    }
  }

  const char* error = nullptr;

  if (doc["interval_seconds"].is<float>()) {
    float value = doc["interval_seconds"];
    if (value > 0) {
      interval_seconds = value;
      Serial.print("Interval set to ");
      Serial.print(interval_seconds);
      Serial.println(" s");
    } else {
      Serial.println("Invalid interval, ignored");
      error = "Invalid interval, ignored";
    }
  }

  if (doc["active"].is<bool>()) {
    set_active(doc["active"]);
  }

  send_status(error);
}

void setup() {
  Serial.begin(115200);
  pinMode(LED_A_PIN, OUTPUT);
  pinMode(LED_B_PIN, OUTPUT);
  gps_serial.begin(GPS_BAUD, SERIAL_8N1, ESP32_RX2_PIN, ESP32_TX2_PIN);

  WiFi.begin(wifi_ssid);
  Serial.print("Connecting to WiFi");
  unsigned long last_dot = 0;

  while (WiFi.status() != WL_CONNECTED) {
    update_leds();
    if (millis() - last_dot >= 500) {
      last_dot = millis();
      Serial.print(".");
    }
    delay(10);
  }
  Serial.println("\nWiFi connected");

  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setKeepAlive(mqtt_keep_alive);
  mqtt.setCallback(on_message);
}

void loop() {
  if (!mqtt.connected()) {
    if (mqtt.connect(DEVICE_ID)) {
      mqtt.subscribe(mqtt_command_topic);
      Serial.println("MQTT connected, subscribed to command topic");
      publish_status();
    }
  }

  mqtt.loop();
  update_leds();

  if (!gps_enabled) {
    while (gps_serial.available()) gps_serial.read();
    return;
  }

  while (gps_serial.available()) gps.encode(gps_serial.read());

  if (millis() - last_send >= (unsigned long)(interval_seconds * 1000)) {
    last_send = millis();

    if (gps.location.isValid()) {
      char msg[msg_size];

      int len = snprintf(msg, sizeof(msg),
                "{\"device_id\":\"%s\",\"lat\":%.6f,\"lng\":%.6f,\"alt_m\":%.1f,\"speed_kn\":%.1f",
                DEVICE_ID, gps.location.lat(), gps.location.lng(),
                gps.altitude.meters(), gps.speed.knots());

      if (gps.date.isValid() && gps.time.isValid()) {
        len += snprintf(msg + len, sizeof(msg) - len,
                        ",\"timestamp\":%lu", gps_epoch());
      }

      snprintf(msg + len, sizeof(msg) - len, "}");

      char time_str[12] = "--:--:--";
      if (gps.time.isValid()) {

        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                (gps.time.hour() + 7) % 24, gps.time.minute(), gps.time.second());
      }

      Serial.print("[");
      Serial.print(time_str);
      Serial.print("] ");
      Serial.println(msg);

      if (mqtt.publish(mqtt_topic, msg)) last_publish_ok = millis();
    } else {
      Serial.println("Waiting for GPS fix...");
    }
  }
}
