
#include <Arduino.h>
#include <ArduinoJson.h>

#include "logging/logging.h"

using namespace creatures;

// Make sure a weird message off the wire doesn't set the display to an
// invalid value
#define BRIGHTNESS_MIN 0
#define BRIGHTNESS_MAX 15

#define LED_RING_BRIGHTNESS_MIN 0
#define LED_RING_BRIGHTNESS_MAX 254

#define LED_RING_SATURATION_MIN 0
#define LED_RING_SATURATION_MAX 254

// Defined in main.cpp
extern uint8_t gScreenBrightness;
extern boolean gBlinkColon;
extern boolean gDisplayOn;
extern uint8_t gPixelRingSaturation;
extern uint8_t gPixelBrightness;

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

        // Don't look at an empty string
        if (brightness != NULL && !brightness.isEmpty() && !brightness.equals("null"))
        {

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
        }
        else
        {
            l.warning("'brightness' was missing from the config");
        }

        /*
            Blinking Colon

            Parameter: blinkingColon

        */

        String blinkingColon = json["blinkingColon"];
        l.debug("'blinkingColon' was %s", blinkingColon);

        // Don't do a dumb thing :)
        if (blinkingColon != NULL && !blinkingColon.isEmpty() && !blinkingColon.equals("null"))
        {
            const char *blinkValue = blinkingColon.c_str();

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
        else
        {
            l.warning("'blinkingColon' was missing from the config");
        }

        /*

            Display On?

            Parameter: displayOn

        */

        String displayOn = json["displayOn"];
        l.debug("'displayOn' was %s", displayOn);

        // Don't make an adjustment on an missing value
        if (displayOn != NULL && !displayOn.isEmpty() && !displayOn.equals("null"))
        {
            const char *displayOnValue = displayOn.c_str();

            // Make sure that the brightness is in range
            if (strncmp("on", displayOnValue, strlen(displayOnValue)) == 0)
            {
                gDisplayOn = true;
            }
            else
            {
                gDisplayOn = false;
            }
            l.info("set 'gDisplayOn' to %d", gDisplayOn);
        }
        else
        {
            l.warning("'displayOn' was missing in the config object, not changing anything");
        }

        /*
            LED Ring Brightness

            Parameter: ledRingBrightness
        */

        String ledRingBrightness = json["ledRingBrightness"];
        l.debug("'ledRingBrightness' was %s", brightness.c_str());

        // Don't look at an empty string
        if (ledRingBrightness != NULL && !ledRingBrightness.isEmpty() && !ledRingBrightness.equals("null"))
        {

            uint8_t decodedLedRingBrightness = atoi(ledRingBrightness.c_str());

            // Make sure that the brightness is in range
            if (decodedLedRingBrightness >= LED_RING_BRIGHTNESS_MIN && decodedLedRingBrightness <= LED_RING_BRIGHTNESS_MAX)
            {
                l.debug("setting LED ring brightness to %d", decodedLedRingBrightness);
                gPixelBrightness = decodedLedRingBrightness;
            }
            else
            {
                l.error("Got an out-of-range LED ring brightness request: %d", decodedLedRingBrightness);
            }
        }
        else
        {
            l.warning("'ledRingBrightness' was missing from the config");
        }

        /*
            LED Ring Saturation

            Parameter: ledRingSaturation
        */

        String saturation = json["ledRingSaturation"];
        l.debug("'ledRingSaturation' was %s", saturation.c_str());

        // Don't look at an empty string
        if (saturation != NULL && !saturation.isEmpty() && !saturation.equals("null"))
        {

            uint8_t decodedSaturation = atoi(saturation.c_str());

            // Make sure that the brightness is in range
            if (decodedSaturation >= LED_RING_SATURATION_MIN && decodedSaturation <= LED_RING_SATURATION_MAX)
            {
                l.debug("setting LED saturation to %d", decodedSaturation);
                gPixelRingSaturation = decodedSaturation;
            }
            else
            {
                l.error("Got an out-of-range LED saturation request: %d", decodedSaturation);
            }
        }
        else
        {
            l.warning("'ledRingSaturation' was missing from the config");
        }
    }
}