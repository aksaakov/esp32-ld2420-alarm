#pragma once

#include <Arduino.h>
#include <HardwareSerial.h>

class LD2420 {
public:
  LD2420(HardwareSerial& uart, int rxPin, int txPin)
    : serial(uart), rx(rxPin), tx(txPin) {}

  void begin(uint32_t baud = 115200) {
    serial.begin(baud, SERIAL_8N1, rx, tx);
  }

  bool enableConfigModeForUi() {
    if (configModeActive) return true;

    bool ok = enterConfigMode();

    if (ok) {
      configModeActive = true;
      configModeStartedAt = millis();
      Serial.println("Config mode enabled from UI.");
    }

    return ok;
  }

  void disableConfigModeForUi() {
    if (!configModeActive) return;

    exitConfigMode();
    configModeActive = false;
    Serial.println("Config mode disabled.");
  }

  bool isConfigModeEnabled() const {
    return configModeActive;
  }

  void handleConfigModeTimeout() {
    if (configModeActive && millis() - configModeStartedAt > 60000) {
      Serial.println("Config mode timed out after 60 seconds.");
      disableConfigModeForUi();
    }
  }

  bool updatePresence() {
    if (configModeActive) {
      return false;
    }  
    while (serial.available()) {
      uint8_t b = serial.read();

      // Your module previously printed ASCII:
      // ON
      // Range 54
      //
      // So for now we parse text, not binary frames.

      static String line = "";

      if (b == '\n') {
        line.trim();

        if (line.length() > 0) {
          bool detected = lastPresenceState;

          if (line == "ON") {
            detected = true;
          } else if (line == "OFF") {
            detected = false;
          }

          line = "";

          if (detected != lastPresenceState) {
            lastPresenceState = detected;
            return true;
          }
        }
      } else if (b != '\r') {
        line += (char)b;

        if (line.length() > 40) {
          line = "";
        }
      }
    }

  return false;
}

bool isPresenceDetected() const {
  return lastPresenceState;
}

  bool setRange(uint8_t range, uint8_t unmannedDurationSeconds = 5) {
      if (!configModeActive) {
      Serial.println("Cannot set range: config mode is not enabled.");
      return false;
    }

    uint8_t setRangeCmd[] = {
      0xFD, 0xFC, 0xFB, 0xFA,
      0x14, 0x00,
      0x60, 0x00,

      0x00, 0x00,
      range, 0x00, 0x00, 0x00,

      0x01, 0x00,
      range, 0x00, 0x00, 0x00,

      0x02, 0x00,
      unmannedDurationSeconds, 0x00, 0x00, 0x00,

      0x04, 0x03, 0x02, 0x01
    };

    serial.write(setRangeCmd, sizeof(setRangeCmd));
    bool ok = waitForAck(0x60);

    exitConfigMode();

    Serial.println("Sensor updated!");
    disableConfigModeForUi();
    return ok;
  }

  int readRange() {
  if (!configModeActive) {
    Serial.println("Cannot read range: config mode is disabled.");
    return -1;
  }

  uint8_t readParams[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x02, 0x00,
    0x61, 0x00,
    0x04, 0x03, 0x02, 0x01
  };

  serial.write(readParams, sizeof(readParams));

  uint8_t buf[128];
  size_t len = readPacket(buf, sizeof(buf), 3000);

  for (size_t i = 0; i + 5 < len; i++) {
    if (buf[i] == 0x61 &&
        buf[i + 1] == 0x01 &&
        buf[i + 2] == 0x00 &&
        buf[i + 3] == 0x00) {
      return (int)buf[i + 5];
    }
  }

  return -1;
}

private:
  HardwareSerial& serial;
  int rx;
  int tx;

bool configModeActive = false;
unsigned long configModeStartedAt = 0;

void clearBufferFor(unsigned long ms = 300) {
  unsigned long start = millis();

  while (millis() - start < ms) {
    while (serial.available()) {
      serial.read();
    }
  }
}

  bool lastPresenceState = false;

  bool enterConfigMode() {
  clearBufferFor(800);

  uint8_t cmd[] = {
    0xFD, 0xFC, 0xFB, 0xFA,
    0x04, 0x00,
    0xFF, 0x00,
    0x01, 0x00,
    0x04, 0x03, 0x02, 0x01
  };

  serial.write(cmd, sizeof(cmd));

  bool ok = waitForAck(0xFF, 3000);

  Serial.print("Enter config mode: ");
  Serial.println(ok ? "OK" : "FAILED");

  delay(300);

  return ok;
}

  void exitConfigMode() {
    uint8_t cmd[] = {
      0xFD, 0xFC, 0xFB, 0xFA,
      0x02, 0x00,
      0xFE, 0x00,
      0x04, 0x03, 0x02, 0x01
    };

    serial.write(cmd, sizeof(cmd));
    waitForAck(0xFE);
    delay(300);
  }

  bool waitForAck(uint8_t command, unsigned long timeout = 2000) {
    uint8_t buf[64];
    size_t len = readPacket(buf, sizeof(buf), timeout);

    for (size_t i = 0; i + 3 < len; i++) {
      if (buf[i] == command && buf[i + 1] == 0x01 && buf[i + 2] == 0x00 && buf[i + 3] == 0x00) {
        return true;
      }
    }

    return false;
  }

  size_t readPacket(uint8_t* buf, size_t maxLen, unsigned long timeoutMs) {
    size_t len = 0;
    unsigned long start = millis();

    while (millis() - start < timeoutMs) {
      while (serial.available()) {
        uint8_t b = serial.read();

        if (len < maxLen) {
          buf[len++] = b;
        }

        if (len >= 4 && buf[len - 4] == 0x04 && buf[len - 3] == 0x03 && buf[len - 2] == 0x02 && buf[len - 1] == 0x01) {
          return len;
        }
      }
    }

    return len;
  }
};

