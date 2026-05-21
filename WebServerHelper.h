#pragma once

#include <Arduino.h>
#include <WebServer.h>
#include "LD2420.h"

class WebServerHelper {
public:
  WebServerHelper(LD2420& radarRef)
    : server(80), radar(radarRef) {}

  void begin() {
    server.on("/", HTTP_GET, [this]() {
      handleRoot();
    });

    server.on("/enable-config", HTTP_POST, [this]() {
      handleEnableConfig();
    });

    server.on("/set-range", HTTP_POST, [this]() {
      handleSetRange();
    });

    server.begin();
  }

  void handleClient() {
    server.handleClient();
  }

private:
  WebServer server;
  LD2420& radar;

  void handleRoot() {
    bool configEnabled = radar.isConfigModeEnabled();
    int currentRange = configEnabled ? radar.readRange() : -1;

    String disabled = configEnabled ? "" : "disabled";
    String status = configEnabled
      ? "Config mode enabled. You have 60 seconds."
      : "Reporting mode active. Enable config mode to change settings.";

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LD2420 Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 30px; background: #f6f6f6; }
    .card { max-width: 460px; padding: 20px; background: white; border-radius: 12px; box-shadow: 0 2px 8px #ccc; }
    input, button { font-size: 18px; padding: 10px; margin-top: 8px; }
    input { width: 120px; }
    button { cursor: pointer; }
    button:disabled, input:disabled { opacity: 0.45; cursor: not-allowed; }
    .status { padding: 10px; background: #eee; border-radius: 8px; margin: 16px 0; }
  </style>
</head>
<body>
  <div class="card">
    <h2>LD2420 Config</h2>
)rawliteral";

    html += "<div class='status'>";
    html += status;
    html += "</div>";

    html += "<p>Current range: <strong>";
    if (configEnabled && currentRange >= 0) {
      html += currentRange;
    } else if (configEnabled) {
      html += "Unable to read";
    } else {
      html += "Unavailable in reporting mode";
    }
    html += "</strong></p>";

    html += R"rawliteral(
    <form action="/enable-config" method="POST">
      <button type="submit">Enable config mode</button>
    </form>

    <hr>

    <form action="/set-range" method="POST">
      <label for="range">New range:</label><br>
)rawliteral";

    html += "<input id='range' name='range' type='number' min='1' max='255' required ";
    html += disabled;
    html += ">";

    html += "<br><button type='submit' ";
    html += disabled;
    html += ">Set range</button>";

    html += R"rawliteral(
    </form>
  </div>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
  }

 void handleEnableConfig() {
  bool ok = false;

  for (int attempt = 1; attempt <= 3; attempt++) {
    Serial.print("UI enable config attempt ");
    Serial.println(attempt);

    ok = radar.enableConfigModeForUi();

    if (ok) break;

    delay(500);
  }

  if (ok) {
    server.sendHeader("Location", "/");
    server.send(303);
  } else {
    server.send(500, "text/plain", "Failed to enter config mode");
  }
}

  void handleSetRange() {
    if (!radar.isConfigModeEnabled()) {
      server.send(400, "text/plain", "Config mode is not enabled");
      return;
    }

    if (!server.hasArg("range")) {
      server.send(400, "text/plain", "Missing range value");
      return;
    }

    int value = server.arg("range").toInt();

    if (value < 1 || value > 255) {
      server.send(400, "text/plain", "Range must be between 1 and 255");
      return;
    }

    Serial.print("Updating range from UI to: ");
    Serial.println(value);

    bool ok = radar.setRange((uint8_t)value);

    String html = "<html><body>";
    html += ok ? "<h2>Range updated</h2>" : "<h2>Range update failed</h2>";
    html += "<p><a href='/'>Back</a></p>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  }
};