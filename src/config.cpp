
#include <Arduino.h>
#include <ArduinoJson.h>

#include "logging/logging.h"

using namespace creatures;

// Make sure a weird message off the wire doesn't set the display to an
// invalid value
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 15

// Defined in main.cpp
extern uint8_t gScreenBrightness;
extern boolean gBlinkColon;

static Logger l;

/**
 * @brief Update the configuration of the device from MQTT
 *
 * The config isn't persisted anywhere on the MCU. It's config it's kept in a retained MQTT topic
 * which will be read when it boots.
 *
 * @param incomingJson a String with JSON from MQTT
 */
void updateConfig(String incomingJson)
{
    l.debug("Incoming config message: %s", incomingJson.c_str());

    // Let's put this on the stack so it goes poof when we leave
    StaticJsonDocument<512> json;

    DeserializationError error = deserializeJson(json, incomingJson);

    if (error)
    {
        l.error("Unable to deserialize config from MQTT: %s", error.c_str());
    }
    else
    {
        l.debug("decode was good!");

        /*
            Brightness
        */

        String brightness = json["brightness"];
        l.debug("'brightness' was %s", brightness.c_str());

        uint8_t decodedBrightness = atoi(brightness.c_str());

        // Make sure that the brightness is in range
        if (decodedBrightness >= BRIGHTNESS_MIN && decodedBrightness <= BRIGHTNESS_MAX)
        {
            l.debug("setting brightness to %d", decodedBrightness);
            gScreenBrightness = decodedBrightness;
        }
        else
        {
            l.error("Got an out-of-range brightness request: %d", decodedBrightness);
        }

        /*
            Blinking Colon

            Parameter: blinkingColon

        */

        String blinkingColon = json["blinkingColon"];
        l.debug("'blinkingColon' was %s", blinkingColon);

        const char* blinkValue = blinkingColon.c_str();

        // Make sure that the brightness is in range
        if (strncmp("on", blinkValue, strlen(blinkValue)) == 0)
        {
            gBlinkColon = true;
        }
        else
        {
            gBlinkColon = false;
        }
        l.info("set 'gBlinkColon' to %d", gBlinkColon);
    }
}