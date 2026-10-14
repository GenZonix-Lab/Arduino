// ESP-NOW Slave code with "signal" response
#include "ESP32_NOW.h"
#include "WiFi.h"
#include <esp_mac.h>
#include <vector>

#define ESPNOW_WIFI_CHANNEL 6
#define l1 5
#define l2 18

class ESP_NOW_Peer_Class : public ESP_NOW_Peer {
public:
  ESP_NOW_Peer_Class(const uint8_t *mac_addr, uint8_t channel, wifi_interface_t iface, const uint8_t *lmk)
      : ESP_NOW_Peer(mac_addr, channel, iface, lmk) {}

  ~ESP_NOW_Peer_Class() {}

  bool add_peer() {
    if (!add()) {
      log_e("Failed to register the broadcast peer");
      return false;
    }
    return true;
  }

  // Function to handle received messages
  void onReceive(const uint8_t *data, size_t len, bool broadcast) {
    String msg = String((char *)data);
    msg.trim();
    Serial.printf("  Message Received: %s\n", msg.c_str());

    // Perform actions based on command
    if (msg == "r1on") {
      digitalWrite(l1, LOW);
    } else if (msg == "r1off") {
      digitalWrite(l1, HIGH);
    } else if (msg == "r2on") {
      digitalWrite(l2, LOW);
    } else if (msg == "r2off") {
      digitalWrite(l2, HIGH);
    }

    // Send response "signal" back to master
    char reply[] = "signal";
    if (!send(info->src_addr, (uint8_t *)reply, strlen(reply))) {
      Serial.println("Failed to send 'signal' back to master");
    } else {
      Serial.println("Sent 'signal' back to master");
    }
  }
};

std::vector<ESP_NOW_Peer_Class *> masters;

// Callback when a new master is discovered
void register_new_master(const esp_now_recv_info_t *info, const uint8_t *data, int len, void *arg) {
  if (memcmp(info->des_addr, ESP_NOW.BROADCAST_ADDR, 6) == 0) {
    Serial.printf("Unknown peer " MACSTR " sent a broadcast message\n", MAC2STR(info->src_addr));
    Serial.println("Registering the peer as a master");

    ESP_NOW_Peer_Class *new_master = new ESP_NOW_Peer_Class(info->src_addr, ESPNOW_WIFI_CHANNEL, WIFI_IF_STA, nullptr);
    if (!new_master->add_peer()) {
      Serial.println("Failed to register the new master");
      delete new_master;
      return;
    }

    // Assign onReceive callback dynamically
    new_master->onReceiveFunc([=](const uint8_t *data, size_t len, bool broadcast) {
      String msg = String((char *)data);
      msg.trim();
      Serial.printf("Message from Master: %s\n", msg.c_str());

      if (msg == "r1on") digitalWrite(l1, LOW);
      else if (msg == "r1off") digitalWrite(l1, HIGH);
      else if (msg == "r2on") digitalWrite(l2, LOW);
      else if (msg == "r2off") digitalWrite(l2, HIGH);

      // Send confirmation "signal" back
      const char reply[] = "signal";
      if (!new_master->send((uint8_t *)reply, strlen(reply))) {
        Serial.println("Failed to send 'signal' back");
      } else {
        Serial.println("Sent 'signal' back to master");
      }
    });

    masters.push_back(new_master);
    Serial.printf("Successfully registered master " MACSTR " (total masters: %zu)\n", MAC2STR(new_master->addr()), masters.size());
  }
}

/* -------------------- Main -------------------- */
void setup() {
  Serial.begin(115200);
  pinMode(l1, OUTPUT);
  pinMode(l2, OUTPUT);
  digitalWrite(l1, HIGH);
  digitalWrite(l2, HIGH);

  // Initialize Wi-Fi
  WiFi.mode(WIFI_STA);
  WiFi.setChannel(ESPNOW_WIFI_CHANNEL);
  while (!WiFi.STA.started()) delay(100);

  Serial.println("ESP-NOW Slave with Signal Response");
  Serial.println("Wi-Fi parameters:");
  Serial.println("  Mode: STA");
  Serial.println("  MAC Address: " + WiFi.macAddress());
  Serial.printf("  Channel: %d\n", ESPNOW_WIFI_CHANNEL);

  // Initialize ESP-NOW
  if (!ESP_NOW.begin()) {
    Serial.println("Failed to initialize ESP-NOW");
    delay(5000);
    ESP.restart();
  }

  Serial.printf("ESP-NOW version: %d, max data length: %d\n", ESP_NOW.getVersion(), ESP_NOW.getMaxDataLen());

  // Register callback for new master
  ESP_NOW.onNewPeer(register_new_master, nullptr);

  Serial.println("Setup complete. Waiting for master broadcast...");
}

void loop() {
  delay(100);
}
