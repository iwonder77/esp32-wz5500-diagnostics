/**
 *  Interactive: NA
 *  File: esp32-wz5500-diagnostics.ino
 *  Description: establish basic Ethernet connectivity between ESP32-DevKitC-v4/Adafruit WZ5500 module <-> Macbook
 *
 *  Author: Isai Sanchez
 *  Date: 2-5-2026
 *  Hardware:
 *    - ESP32 DevKitC v4
 *    - Adafruit WZ5500 Ethernet Breakout 
 *  Notes: 
 *    - Wiring to WZ5500 is as follows:
 *      External 3v3 Power Supply   -> VIN
 *      External GND and ESP32 GND  -> GND
 *      ESP32 GPIO18                -> SCK
 *      ESP32 GPIO19                -> MISO
 *      ESP32 GPIO23                -> MOSI
 *      ESP32 GPIO5                 -> CS
 *      N.C.                        -> IRQ
 *      ESP32 GPIO4                 -> RST
 *    - Commands for testing from Mac terminal
 *      1. Ping test:     ping 192.168.50.2
 *      2. UDP test:      echo "hello" | nc -u 192.168.50.2 5000
 *
 * (c) Thanksgiving Point Exhibits Electronics Team — 2025
*/

#include <SPI.h>
#include <Ethernet.h>
#include <EthernetUdp.h>

// ===== HARDWARE CONFIG =====
const unsigned int W5500_CS = 5;
const unsigned int W5500_RST = 4;

// ===== NETWORK CONFIG =====
byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01 };  // esp32 MAC address
IPAddress ip(192, 168, 50, 2);                        // esp32 static IP address
const unsigned int UDP_PORT = 5000;

EthernetUDP udp;

// receive buffer
const size_t BUFFER_SIZE = 256;
char packetBuffer[BUFFER_SIZE];

// simple state tracking
bool ethernetInitialized = false;
bool linkUp = false;
unsigned long packetsReceived = 0;
unsigned long packetsSent = 0;
const unsigned long LINK_TIMEOUT = 10000;

void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println();
  Serial.println("==========================================");
  Serial.println("      W5500 ETHERNET FOUNDATION TEST      ");
  Serial.println("==========================================");
  Serial.println();

  // hardware reset
  Serial.print("Step 1: Hardware reset on RST pin (GPIO ");
  Serial.print(W5500_RST);
  Serial.print(")");
  pinMode(W5500_RST, OUTPUT);
  digitalWrite(W5500_RST, LOW);
  delay(100);
  digitalWrite(W5500_RST, HIGH);
  delay(500);
  Serial.println("  Done.\n");

  // initialize ethernet
  Serial.println("Step 2: Initialize Ethernet library...");
  Ethernet.init(W5500_CS);
  Ethernet.begin(mac, ip);
  Serial.println("  Done.\n");

  // check hardware status
  Serial.println("Step 3: Hardware detection...");
  switch (Ethernet.hardwareStatus()) {
    case EthernetNoHardware:
      Serial.println("  FAIL: No hardware found");
      ethernetInitialized = false;
      break;
    case EthernetW5100:
      Serial.println("  Found: W5100 (unexpected)");
      ethernetInitialized = false;
      break;
    case EthernetW5200:
      Serial.println("  Found: W5200 (unexpected)");
      ethernetInitialized = false;
      break;
    case EthernetW5500:
      Serial.println("  Found: W5500 (correct!)");
      ethernetInitialized = true;
      break;
    default:
      Serial.println("  Unknown hardware");
      ethernetInitialized = false;
  }

  // wait for link to come up
  Serial.println("\nStep 4: Waiting for ethernet link...");
  unsigned long startTime = millis();
  while (Ethernet.linkStatus() != LinkON) {
    if (millis() - startTime > LINK_TIMEOUT) {
      Serial.println("  TIMEOUT: link did not come up after 10s timeout");
      Serial.println("  Continuing anyway - link may come up later");
    }
    delay(500);
  }
  Serial.println();
  switch (Ethernet.linkStatus()) {
    case Unknown:
      Serial.println("  UNKNOWN");
      linkUp = false;
      break;
    case LinkON:
      Serial.println("  UP! ← Success!");
      linkUp = true;
      break;
    case LinkOFF:
      Serial.println("  DOWN");
      linkUp = false;
      break;
    default:
      linkUp = false;
      break;
  }

  // start UDP server
  Serial.print("Step 5: Starting UDP server, binding to port: ");
  Serial.println(UDP_PORT);
  udp.begin(UDP_PORT);
}

void loop() {
  // Update link status
  updateLinkStatus();

  // Handle incoming UDP packets
  if (ethernetInitialized && linkUp) {
    handleUdp();
  }

  // Periodic status update
  printPeriodicStatus();

  // Maintain Ethernet (DHCP renewal, etc.)
  Ethernet.maintain();

  delay(10);
}

void updateLinkStatus() {
  static EthernetLinkStatus lastStatus = Unknown;
  static unsigned long lastChange = 0;

  EthernetLinkStatus currentStatus = Ethernet.linkStatus();

  if (currentStatus != lastStatus) {
    lastStatus = currentStatus;
    lastChange = millis();

    Serial.print("[");
    printTimestamp();
    Serial.print("] LINK STATUS: ");

    switch (currentStatus) {
      case LinkON:
        Serial.println("UP ✓ ←── Cable connected!");
        linkUp = true;
        break;
      case LinkOFF:
        Serial.println("DOWN ✗ ←── Check cable");
        linkUp = false;
        break;
      case Unknown:
        Serial.println("UNKNOWN");
        linkUp = false;
        break;
    }
  }
}

void handleUdp() {
  int packetSize = udp.parsePacket();

  if (packetSize > 0) {
    packetsReceived++;

    // Get sender info
    IPAddress senderIP = udp.remoteIP();
    uint16_t senderPort = udp.remotePort();

    // Read packet
    memset(packetBuffer, 0, BUFFER_SIZE);
    int bytesRead = udp.read(packetBuffer, BUFFER_SIZE - 1);
    packetBuffer[bytesRead] = '\0';

    // Trim trailing whitespace/newlines
    while (bytesRead > 0 && (packetBuffer[bytesRead - 1] == '\n' || packetBuffer[bytesRead - 1] == '\r' || packetBuffer[bytesRead - 1] == ' ')) {
      packetBuffer[--bytesRead] = '\0';
    }

    // Log received packet
    Serial.println();
    Serial.print("[");
    printTimestamp();
    Serial.println("] ╔═══ UDP PACKET RECEIVED ═══════════════════════════════");
    Serial.print("        ║ From: ");
    Serial.print(senderIP);
    Serial.print(":");
    Serial.println(senderPort);
    Serial.print("        ║ Size: ");
    Serial.print(bytesRead);
    Serial.println(" bytes");
    Serial.print("        ║ Data: \"");
    Serial.print(packetBuffer);
    Serial.println("\"");

    // Prepare response
    String response;

    if (strcmp(packetBuffer, "PING") == 0 || strcmp(packetBuffer, "ping") == 0) {
      response = "PONG";
    } else if (strcmp(packetBuffer, "STATUS") == 0 || strcmp(packetBuffer, "status") == 0) {
      response = "OK - Uptime: " + String(millis() / 1000) + "s, RX: " + String(packetsReceived) + ", TX: " + String(packetsSent);
    } else {
      // Echo back with prefix
      response = "ECHO: " + String(packetBuffer);
    }

    // Send response
    udp.beginPacket(senderIP, senderPort);
    udp.print(response);
    udp.endPacket();
    packetsSent++;

    Serial.print("        ║ Sent: \"");
    Serial.print(response);
    Serial.println("\"");
    Serial.println("        ╚════════════════════════════════════════════════════════");
    Serial.println();
  }
}

void printPeriodicStatus() {
  static unsigned long lastStatusTime = 0;
  const unsigned long STATUS_INTERVAL = 10000;  // Every 10 seconds

  if (millis() - lastStatusTime >= STATUS_INTERVAL) {
    lastStatusTime = millis();

    Serial.print("[");
    printTimestamp();
    Serial.print("] Status: Link=");
    Serial.print(linkUp ? "UP" : "DOWN");
    Serial.print(", IP=");
    Serial.print(Ethernet.localIP());
    Serial.print(", RX=");
    Serial.print(packetsReceived);
    Serial.print(", TX=");
    Serial.println(packetsSent);
  }
}

void printTimestamp() {
  unsigned long seconds = millis() / 1000;
  unsigned long minutes = seconds / 60;
  seconds = seconds % 60;

  if (minutes < 10) Serial.print("0");
  Serial.print(minutes);
  Serial.print(":");
  if (seconds < 10) Serial.print("0");
  Serial.print(seconds);
}
