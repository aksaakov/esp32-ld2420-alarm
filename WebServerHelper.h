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

    server.on("/set", HTTP_POST, [this]() {
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
    int currentRange = radar.readRange();

    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <title>LD2420 Config</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <style>
    body { font-family: Arial, sans-serif; margin: 30px; }
    input, button { font-size: 18px; padding: 8px; }
    .card { max-width: 420px; padding: 20px; border: 1px solid #ccc; border-radius: 12px; }
  </style>
</head>
<body>
  <div class="card">
    <h2>LD2420 Range Config</h2>
)rawliteral";

    html += "<p>Current range: <strong>";
    html += currentRange;
    html += "</strong></p>";

    html += R"rawliteral(
    <form action="/set" method="POST">
      <label for="range">New range:</label><br><br>
      <input id="range" name="range" type="number" min="1" max="255" required>
      <button type="submit">Set range</button>
    </form>
  </div>
</body>
</html>
)rawliteral";

    server.send(200, "text/html", html);
  }

  void handleSetRange() {
    if (!server.hasArg("range")) {
      server.send(400, "text/plain", "Missing range value");
      return;
    }

    int value = server.arg("range").toInt();

    if (value < 1 || value > 255) {
      server.send(400, "text/plain", "Range must be between 1 and 255");
      return;
    }

    Serial.print("Updating range to: ");
    Serial.println(value);

    bool ok = radar.setRange((uint8_t)value);

    int currentRange = radar.readRange();

    if (ok) {
      Serial.print("Range successfully updated. Current range: ");
      Serial.println(currentRange);
    } else {
      Serial.println("Range update failed.");
    }

    String html = "<html><body>";
    html += ok ? "<h2>Range updated</h2>" : "<h2>Range update failed</h2>";
    html += "<p>Current range: <strong>";
    html += currentRange;
    html += "</strong></p>";
    html += "<p><a href='/'>Back</a></p>";
    html += "</body></html>";

    server.send(200, "text/html", html);
  }
};