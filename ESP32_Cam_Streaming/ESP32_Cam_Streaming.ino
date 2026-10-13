#include "esp_camera.h"
#include <WiFi.h>
#include <WebServer.h>

// ==========================
// Wi-Fi credentials
// ==========================
const char *ssid = "Genzonix";
const char *password = "asdfghjkl1";

// ==========================
// AI Thinker ESP32-CAM pins
// ==========================
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27

#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5

#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

#define LED_GPIO_NUM   4

WebServer server(80);

// ==========================
// Web page
// ==========================
const char MAIN_page[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <title>ESP32-CAM Live Stream</title>

  <style>
    body {
      margin: 0;
      background: #111;
      color: white;
      font-family: Arial;
      text-align: center;
    }

    h2 {
      margin: 15px;
    }

    img {
      border-radius: 10px;
    }
  </style>
</head>

<body>

<h2>AI Thinker ESP32-CAM</h2>
<p>Live OV2640 Camera Stream</p>

<img src="/stream">

</body>
</html>
)rawliteral";

// ==========================
// Camera stream
// ==========================
void handleStream() {

  WiFiClient client = server.client();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
  client.println("Access-Control-Allow-Origin: *");
  client.println();

  while (client.connected()) {

    camera_fb_t *fb = esp_camera_fb_get();

    if (!fb) {
      Serial.println("Camera capture failed");
      break;
    }

    client.println("--frame");
    client.println("Content-Type: image/jpeg");
    client.print("Content-Length: ");
    client.println(fb->len);
    client.println();

    client.write(fb->buf, fb->len);
    client.println();

    esp_camera_fb_return(fb);

    delay(30);
  }
}

// ==========================
// Home page
// ==========================
void handleRoot() {
  server.send_P(200, "text/html", MAIN_page);
}

// ==========================
// Setup
// ==========================
void setup() {

  Serial.begin(115200);
  Serial.println();
  Serial.println("Starting ESP32-CAM...");

  // ==========================
  // Camera configuration
  // ==========================
  camera_config_t config;

  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer   = LEDC_TIMER_0;

  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;

  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;

  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;

  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;

  config.xclk_freq_hz = 20000000;

  config.pixel_format = PIXFORMAT_JPEG;
  
  // ==========================
  // Resolution
  config.frame_size = FRAMESIZE_QQVGA;  // 640x480
  config.jpeg_quality = 8;
  config.fb_count = 2;


  // ==========================

  // ==========================
  // Initialize camera
  // ==========================
  esp_err_t err = esp_camera_init(&config);

  if (err != ESP_OK) {

    Serial.printf(
      "Camera initialization failed: 0x%x\n",
      err
    );

    while (true) {
      delay(1000);
    }
  }

  // Horizontal flip / mirror
  sensor_t *s = esp_camera_sensor_get();
  s->set_hmirror(s, 1);

  Serial.println("Camera initialized successfully.");

  // ==========================
  // Connect Wi-Fi
  // ==========================
  WiFi.begin(ssid, password);

  Serial.print("Connecting to Wi-Fi");

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("Wi-Fi connected!");

  Serial.print("ESP32-CAM IP address: ");
  Serial.println(WiFi.localIP());

  // ==========================
  // Web server
  // ==========================
  server.on("/", HTTP_GET, handleRoot);
  server.on("/stream", HTTP_GET, handleStream);

  server.begin();

  Serial.println("Web server started.");
  Serial.println();
  Serial.println("Open this address in your browser:");
  Serial.print("http://");
  Serial.println(WiFi.localIP());
}

// ==========================
// Loop
// ==========================
void loop() {
  server.handleClient();
}