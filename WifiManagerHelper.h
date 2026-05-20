#pragma once

#include <WiFi.h>
#include <WiFiManager.h>

class WifiManagerHelper {
public:
  bool connect(const char* apName = "ESP32-Setup") {
    WiFiManager wifiManager;

    IPAddress staticIp(192, 168, 0, 151);
    IPAddress gateway(192, 168, 0, 1);
    IPAddress subnet(255, 255, 255, 0);

    wifiManager.setSTAStaticIPConfig(staticIp, gateway, subnet);

    return wifiManager.autoConnect(apName);
  }

  String getIpAddress() {
    return WiFi.localIP().toString();
  }
};