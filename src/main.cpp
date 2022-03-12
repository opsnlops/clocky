// This isn't gonna work on anything but an ESP32
#if !defined(ESP32)
#error This code is intended to run only on the ESP32 board
#endif

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_LEDBackpack.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
#include "freertos/queue.h"
}

#include "logging/logging.h"
#include "mqtt/mqtt.h"
#include "network/connection.h"
#include "creatures/creatures.h"
#include "time/time.h"
#include "mdns/creature-mdns.h"
#include "mdns/magicbroker.h"

#include "creature.h"
#include "ota.h"

using namespace creatures;

Logger l = Logger();
Adafruit_7segment display = Adafruit_7segment();

void setup()
{
    // Before we do anything, get the logger going
    l.init();
    l.debug("Logging running!");

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);

    l.info("Helllllo! I'm up and running on on %s!", ARDUINO_VARIANT);

    NetworkConnection network = NetworkConnection();
    network.connectToWiFi();

    // Register ourselves in mDNS
    CreatureMDNS creatureMDNS = CreatureMDNS(CREATURE_NAME, CREATURE_POWER);
    creatureMDNS.registerService(666);
    creatureMDNS.addStandardTags();

    Time time = Time();
    time.init();
    time.obtainTime();

    // Get the location of the magic broker
    MagicBroker magicBroker;
    magicBroker.find();

    // Connect to MQTT
    MQTT mqtt = MQTT(String(CREATURE_NAME));
    mqtt.connect(magicBroker.ipAddress, magicBroker.port);
    mqtt.subscribe(String("cmd"), 0);

    mqtt.startHeartbeat();

    // Enable OTA
    setup_ota(String(CREATURE_NAME));
    start_ota();

    digitalWrite(LED_BUILTIN, LOW);

    display.begin(0x70);
    display.print(0xBEEF, HEX);
    display.writeDisplay();
}

void loop()
{
    l.debug("scanning...");

    for (uint16_t counter = 0; counter < 9999; counter++)
    {
        display.println(counter);
        display.writeDisplay();
        delay(100);
    }
}