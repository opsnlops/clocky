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
#include "config.h"
#include "ota.h"

using namespace creatures;

TaskHandle_t showTimeTaskHandler;
TaskHandle_t timeSyncTaskhandler;
TaskHandle_t messageReaderTaskHandle;
portTASK_FUNCTION_PROTO(showTimeTask, pvParameters);
portTASK_FUNCTION_PROTO(timeSyncTask, pvParameters);
portTASK_FUNCTION_PROTO(messageQueueReaderTask, pvParameters);

static Logger l = Logger();
static MQTT mqtt = MQTT(String(CREATURE_NAME));
Adafruit_7segment display = Adafruit_7segment();

// Configuration
uint8_t gScreenBrightness = 1; // 0 - 15, 15 is brightest
boolean gBlinkColon = true;    // Should the colon blink?

void setup()
{
    /*
        Since this Creature has it's own display, let's bring it up _before_ we
        start even logging, and use the display to show what phase of the boot
        process we're in.
    */

    int bootPhase = 0;
    display.begin(0x70);
    display.setBrightness(gScreenBrightness);

    display.print(bootPhase++);
    display.writeDisplay();

    l.init();
    l.debug("Logging running!");
    display.print(bootPhase++);
    display.writeDisplay();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    display.print(bootPhase++);
    display.writeDisplay();

    l.info("Helllllo! I'm up and running on on %s!", ARDUINO_VARIANT);

    NetworkConnection network = NetworkConnection();
    network.connectToWiFi();
    display.print(bootPhase++);
    display.writeDisplay();

    // Register ourselves in mDNS
    CreatureMDNS creatureMDNS = CreatureMDNS(CREATURE_NAME, CREATURE_POWER);
    creatureMDNS.registerService(666);
    creatureMDNS.addStandardTags();
    display.print(bootPhase++);
    display.writeDisplay();

    // Set the initial time. Important for a clock! :)
    Time time = Time();
    time.init();
    time.obtainTime();
    display.print(bootPhase++);
    display.writeDisplay();

    // Get the location of the magic broker
    MagicBroker magicBroker;
    magicBroker.find();
    display.print(bootPhase++);
    display.writeDisplay();

    // Connect to MQTT
    mqtt.connect(magicBroker.ipAddress, magicBroker.port);
    mqtt.subscribe(String("cmd"), 0);
    mqtt.subscribe(String("config"), 0);

    mqtt.startHeartbeat();
    display.print(bootPhase++);
    display.writeDisplay();

    // Enable OTA
    setup_ota(String(CREATURE_NAME));
    start_ota();
    display.print(bootPhase++);
    display.writeDisplay();

    digitalWrite(LED_BUILTIN, LOW);

    l.debug("starting the show time task");
    xTaskCreatePinnedToCore(showTimeTask,
                            "showTimeTask",
                            4096,
                            NULL,
                            1,
                            &showTimeTaskHandler,
                            1);

    // Start the task to read the queue
    l.debug("starting the message reader task");
    xTaskCreate(messageQueueReaderTask,
                "messageQueueReaderTask",
                20480,
                NULL,
                1,
                &messageReaderTaskHandle);

    /*
    l.debug("starting the show time task");
    xTaskCreate(timeSyncTask,
                "showTimeTask",
                8192,
                NULL,
                1,
                &showTimeTaskHandler);
    */

    l.info("All booted!");
}

void loop()
{
    vTaskDelete(NULL);
}

portTASK_FUNCTION(showTimeTask, pvParameters)
{

    l.info("Show Time Task started");

    int refreshRate = 100;   // Refresh rate in ms
    int colonBlinkRate = 10; // How many ticks to flip the colons?

    boolean showColon = true;

    int cycle = 0;
    for (;;)
    {
        Time time = Time();
        int currentTime = atoi(time.getCurrentTime("%H%M").c_str());

        // If the time is less than 1200, it's AM.
        boolean isAm = true;

        if (currentTime > 1200)
        {
            isAm = false;
        }

        // I like the time in 12 hour time, so let's convert
        if (isAm)
        {
            if (currentTime < 100)
            {
                // Normalize 00:xx -> 12:xx
                currentTime += 1200;
            }
        }
        else
        {
            // And normalize the PM time
            if (currentTime > 1259)
            {
                currentTime -= 1200;
            }
        }

        uint8_t amPmMarker = 0x00;
        // Which light for AM/PM?
        if (isAm)
        {
            amPmMarker = 0x04;
        }
        else
        {
            amPmMarker = 0x08;
        }

        // Flip the colon on the right interval
        if (gBlinkColon)
        {
            if (cycle++ > colonBlinkRate)
            {
                showColon = !showColon;
                cycle = 0;
            }
        }
        else
        {
            showColon = true;
        }

        // Bit 0x02 is the colon, and position 2 is the colons
        if (showColon)
        {
            display.writeDigitRaw(2, amPmMarker | 0x02);
        }
        else
        {
            display.writeDigitRaw(2, amPmMarker);
        }

        display.print(currentTime);
        display.setBrightness(gScreenBrightness);
        display.writeDisplay();

        vTaskDelay(pdMS_TO_TICKS(refreshRate));
    }
}

/**
 * @brief Handle incoming messages from MQTT
 *
 * It wouldn't be a network connected clock if I can't update the config on the
 * fly from Home Assistant! :)
 */
portTASK_FUNCTION(messageQueueReaderTask, pvParameters)
{

    QueueHandle_t incomingQueue = mqtt.getIncomingMessageQueue();
    for (;;)
    {
        struct MqttMessage message;
        if (xQueueReceive(incomingQueue, &message, (TickType_t)5000) == pdPASS)
        {
            l.debug("Incoming message! local topic: %s, global topic: %s, payload: %s",
                    message.topic,
                    message.topicGlobalNamespace,
                    message.payload);

            // Is this a config message?
            if (strncmp("config", message.topic, strlen(message.topic)) == 0)
            {
                l.info("Got a config message from MQTT: %s", message.payload);
                updateConfig(String(message.payload));
            }
            else
            {
                l.warning("unexpected MQTT message! topic %s", message.topic);
            }
        }
    }
}

/**
 * @brief Ensures the time stays in sync
 *
 * Tells the time class to re-sync itself every now and then to make
 * sure we're set to the exact right time.
 *
 */
portTASK_FUNCTION(timeSyncTask, pvParameters)
{
    l.debug("starting time sync task");

    uint16_t resyncTimeInMs = 15 * 1000;

    Time time = Time();
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(resyncTimeInMs));

        l.debug("Refreshing time from SNTP");
        time.obtainTime();
        l.info("Refreshed the time from SNTP");
    }
}