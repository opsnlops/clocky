#include <Arduino.h>
#include <ArduinoJson.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include "logging/logging.h"
#include "mdns/creature-mdns.h"

#include "seconds_ring.h"

using namespace creatures;

static Logger l;

#define NUMBER_OF_PIXELS 60

Adafruit_NeoPixel strip = Adafruit_NeoPixel(NUMBER_OF_PIXELS, LED_RING_PIN, NEO_GRB + NEO_KHZ800);


extern CreatureMDNS* creatureMDNS;

// Config vars
uint8_t gPixelRingSaturation;
uint8_t gPixelBrightness;

TaskHandle_t secondRingTaskHandle;

// Seed this with a random number
uint16_t colorNumber = random(1, USHRT_MAX);
uint16_t getRandomHue()
{

    // https://martin.ankerl.com/2009/12/09/how-to-create-random-colors-programmatically/

    uint32_t tempColor = colorNumber + GOLDEN_RATIO_CONJUGATE;
    colorNumber = tempColor %= USHRT_MAX;

    l.debug("hue is now: %d", colorNumber);
    return colorNumber;
}

/**
 * @brief A task to update the LED second hand
 *
 * This is kinda fun experiment on how to show seconds with a digital
 * clock in a non-annoying way.
 *
 */
portTASK_FUNCTION(secondRingTask, pvParameters)
{

    // Default to this for now, make this a config var later
    gPixelRingSaturation = 242;
    gPixelBrightness = 10;
    strip.begin();
    strip.clear();
    strip.show();
    l.debug("started up the LED ring on GPIO %d", LED_RING_PIN);

    creatureMDNS->addServiceText(String("number_of_pixels"), String(NUMBER_OF_PIXELS));

    // How many ticks per step? 60000 ms in a minute / NUMBER_OF_PIXELS pixels / how many steps per pixel
    const int stepsPerPixel = 40; // One second per pixel  (25 = 25Hz)
    const TickType_t xFrequency = pdMS_TO_TICKS(60000 / NUMBER_OF_PIXELS / stepsPerPixel);
    l.debug("frequency is %d", xFrequency);

    uint16_t oldHue = getRandomHue();

    // Fill the strip with the oldHue
    l.debug("filling the ring with the 'old' hue");
    for(uint8_t i = 0; i < NUMBER_OF_PIXELS; i++)
    {
        strip.setPixelColor(i, strip.ColorHSV(oldHue, gPixelRingSaturation, gPixelBrightness));
    }
    strip.show();

    uint16_t newHue = oldHue;
    uint32_t ulNotifiedValue;
    TickType_t xLastWakeTime; // Keep track of the last time we woke up so we can be ultra precise
    for (;;)
    {

        // Wait for a our cue to start
        l.debug("waiting for a signal to start");
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, portMAX_DELAY);
        xLastWakeTime = xTaskGetTickCount();
        l.debug("got the signal, starting!");

        // Save the old color and get the new one
        oldHue = newHue;
        newHue = getRandomHue();
        l.debug("oldHue: %d, newHue: %d", oldHue, newHue);

        uint8_t currentStep = 0;
        for (uint8_t pixel = 0; pixel < NUMBER_OF_PIXELS; pixel++)
        {
            l.debug("now doing pixel %d", pixel);
            for (uint8_t currentStep = 1; currentStep - 1 < stepsPerPixel; currentStep++)
            {
                // Wait until the right number of ticks
                vTaskDelayUntil(&xLastWakeTime, xFrequency);

                /*
                    The FreeRTOS ticker is pretty good, and this is a real time OS, but one the last one,
                    let's skip a few steps to allow time for the singaling to get lined up for the top of the
                    minute.
                */
                if (pixel == NUMBER_OF_PIXELS - 1 && currentStep == 1)
                    currentStep += 2;

                l.verbose("now doing step %d", currentStep);

                strip.setPixelColor(pixel, strip.ColorHSV(interpolateHue(oldHue, newHue, stepsPerPixel, currentStep),
                                                          gPixelRingSaturation,
                                                          gPixelBrightness));


                strip.show();
            }
        }
    }
}

/**
 * @brief Returns the requested step in a fade between two hues
 *
 * @param oldColor Starting hue
 * @param newColor Finishing hue
 * @param totalSteps How many steps are we fading
 * @param currentStep Which one to get
 * @return uint16_t The hue requested
 */
uint16_t interpolateHue(uint16_t oldHue, uint16_t newHue, uint8_t totalSteps, uint8_t currentStep)
{
    // How much is each step?
    uint16_t differentialStep = (newHue - oldHue) / totalSteps;

    uint16_t stepHue = oldHue + (differentialStep * currentStep);
    l.verbose("old: %d, new: %d, differential: %d, current: %d", oldHue, newHue, differentialStep, stepHue);

    return stepHue;
}