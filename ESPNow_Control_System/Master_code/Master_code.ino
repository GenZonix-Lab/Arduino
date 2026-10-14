//ESP Now Master code with LED feedback
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN 12
#define NUM_LEDS 1

Adafruit_NeoPixel strip(NUM_LEDS, LED_PIN, NEO_GRB + NEO_KHZ800);

#define ESPNOW_WIFI_CHANNEL 6
#define r1 5
#define r2 18

bool r1_state = false;
bool r2_state = false;
String cmd = "";

// ---------------- ESP-NOW Broadcast Peer ----------------
class ESP_NOW_Broadcast_Peer : public ESP_NOW_Peer {
public:
  ESP_NOW_Broadcast_Peer(uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
      : ESP_NOW_Peer(ESP_NOW.BROADCAST_ADDR, channel, iface, lmk) {}

  ~ESP_NOW_Broadcast_Peer() {
    remove();
  }

  bool begin() {
    if (!ESP_NOW.begin() || !add()) {
      log_e("Failed to initialize ESP-NOW or register broadcast peer");
      return false;
    }
    return true;
  }

  bool send_message(const uint8_t *data, size_t len) {
    if (!send(data, len)) {
      log_e("Failed to broadcast message");
      return false;
    }
    return true;
  }
};

ESP_NOW_Broadcast_Peer broadcast_peer(ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);

// ---------------- LED Control Functions ----------------
void setColor(uint8_t r, uint8_t g, uint8_t b) {
  strip.setPixelColor(0, strip.Color(r, g, b));
  strip.show();
}

// ---------------- Receive Callback ----------------
void onReceiveDataBack(const uint8_t *mac, const uint8_t *data, int len, bool broadcast) {
  String incoming = "";
  for (int i = 0; i < len; i++) incoming += (char)data[i];

  Serial.printf("Received: %s\n", incoming.c_str());

  // When slave sends "signal"
  if (incoming == "signal") {
    Serial.println("Signal received from slave - Turning LED Green");
    setColor(0, 255, 0);   // Green
    delay(2000);
    setColor(0, 0, 0);     // Off
  }
}
// ---------------- Setup ----------------
void setup() {
  Serial.begin(115200);
  pinMode(r1, INPUT_PULLUP);
  pinMode(r2, INPUT_PULLUP);

  strip.begin();
  strip.show(); // Initialize all LEDs to off

  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) delay(100);

  Serial.println("ESP-NOW Master");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  if (!broadcast_peer.begin()) {
    Serial.println("Failed to initialize broadcast peer");
    delay(5000);
    ESP.restart();
  }

  // Register receive callback
  ESP_NOW.onReceive(onReceiveDataBack);
}

// ---------------- Main Loop ----------------
void loop() {
  bool r1_val = digitalRead(r1);
  bool r2_val = digitalRead(r2);
  char data[32];

  if (!r1_val) {
    r1_state = !r1_state;
    cmd = r1_state ? "r1on" : "r1off";

    snprintf(data, sizeof(data), "%s", cmd.c_str());
    Serial.printf("Broadcasting message: %s\n", data);

    // LED shows red during transmission
    setColor(255, 0, 0); // Red
    if (!broadcast_peer.send_message((uint8_t *)data, strlen(data))) {
      Serial.println("Failed to broadcast message");
    }
    delay(800);
    setColor(0, 0, 0); // Turn off after sending
  }

  else if (!r2_val) {
    r2_state = !r2_state;
    cmd = r2_state ? "r2on" : "r2off";

    snprintf(data, sizeof(data), "%s", cmd.c_str());
    Serial.printf("Broadcasting message: %s\n", data);

    setColor(255, 0, 0); // Red (sending)
    if (!broadcast_peer.send_message((uint8_t *)data, strlen(data))) {
      Serial.println("Failed to broadcast message");
    }
    delay(800);
    setColor(0, 0, 0); // Turn off after sending
  }
}
