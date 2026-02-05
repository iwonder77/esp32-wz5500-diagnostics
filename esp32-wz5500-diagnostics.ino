#include <SPI.h>
#include <Ethernet.h>

#define W5500_CS    5
#define W5500_RST   4

byte mac[] = { 0xDE, 0xAD, 0xBE, 0xEF, 0x00, 0x01 };
IPAddress ip(192, 168, 50, 2);

void setup() {
    Serial.begin(115200);
    delay(2000);
    
    Serial.println("\n========== W5500 PHY DIAGNOSTIC ==========\n");
    
    // Reset
    Serial.println("Step 1: Hardware reset...");
    pinMode(W5500_RST, OUTPUT);
    digitalWrite(W5500_RST, LOW);
    delay(100);
    digitalWrite(W5500_RST, HIGH);
    delay(500);
    Serial.println("  Done.\n");
    
    // Initialize
    Serial.println("Step 2: Initialize Ethernet library...");
    Ethernet.init(W5500_CS);
    Ethernet.begin(mac, ip);
    Serial.println("  Done.\n");
    
    // Check hardware
    Serial.println("Step 3: Hardware detection...");
    switch (Ethernet.hardwareStatus()) {
        case EthernetNoHardware:
            Serial.println("  FAIL: No hardware found");
            break;
        case EthernetW5100:
            Serial.println("  Found: W5100 (unexpected)");
            break;
        case EthernetW5200:
            Serial.println("  Found: W5200 (unexpected)");
            break;
        case EthernetW5500:
            Serial.println("  Found: W5500 (correct!)");
            break;
        default:
            Serial.println("  Unknown hardware");
    }
    
    // Check IP
    Serial.print("\nStep 4: IP address: ");
    Serial.println(Ethernet.localIP());
    
    // Monitor link status
    Serial.println("\nStep 5: Monitoring link status...");
    Serial.println("  Connect Ethernet cable to router now.");
    Serial.println("  (Watching for 30 seconds)\n");
}

void loop() {
    static unsigned long lastPrint = 0;
    static int seconds = 0;
    
    if (millis() - lastPrint >= 1000) {
        lastPrint = millis();
        seconds++;
        
        EthernetLinkStatus link = Ethernet.linkStatus();
        
        Serial.print("  [");
        Serial.print(seconds);
        Serial.print("s] Link: ");
        
        switch (link) {
            case Unknown:
                Serial.println("UNKNOWN");
                break;
            case LinkON:
                Serial.println("UP! ← Success!");
                break;
            case LinkOFF:
                Serial.println("DOWN");
                break;
        }
        
        if (seconds >= 30) {
            Serial.println("\n  30 second timeout reached.");
            if (link != LinkON) {
                Serial.println("  Link never came up.");
                Serial.println("\n  Possible causes:");
                Serial.println("    1. RST pin not connected");
                Serial.println("    2. Cable not connected to active device");
                Serial.println("    3. Power issue (try different USB cable/port)");
                Serial.println("    4. Faulty W5500 module");
            }
            while(true) { delay(1000); }
        }
    }
}
