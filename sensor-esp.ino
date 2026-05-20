#include <HardwareSerial.h>

#include "LD2420.h"
#include "WifiManagerHelper.h"
#include "WebServerHelper.h"

HardwareSerial ld2420Serial(1);

static const int RX_PIN = 21;
static const int TX_PIN = 20;

LD2420 radar(ld2420Serial, RX_PIN, TX_PIN);
WifiManagerHelper wifi;
WebServerHelper webServer(radar);

void setup() {
  Serial.begin(115200);
  delay(1000);

  radar.begin();

  Serial.println("Connecting to WiFi...");

  if (!wifi.connect("ESP32-LD2420-Setup")) {
    Serial.println("WiFi connection failed. Restarting...");
    delay(3000);
    ESP.restart();
  }

  Serial.println("WiFi connected!");
  Serial.print("Chosen IP: ");
  Serial.println(wifi.getIpAddress());

  int rangeOnBoot = radar.readRange();
  Serial.print("Range on boot: ");
  Serial.println(rangeOnBoot);

  webServer.begin();

  Serial.println("Web server started.");
}

void loop() {
  webServer.handleClient();
}