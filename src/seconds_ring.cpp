#include <Arduino.h>
#include <ArduinoJson.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include "logging/logging.h"

#include "seconds_ring.h"

using namespace creatures;

static Logger l;

Adafruit_NeoPixel strip = Adafruit_NeoPixel(12, LED_RING_PIN, NEO_GRB + NEO_KHZ800);

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
    strip.begin();
    strip.clear();
    strip.show();
    l.debug("started up the LED ring on GPIO %d", LED_RING_PIN);

    // How many ticks per step? 60000 ms in a minute / 12 pixels / brightness levels per pixel
    const int brightnessPerPixel = 24;
    const TickType_t xFrequency = pdMS_TO_TICKS(60000 / 12 / brightnessPerPixel);
    l.debug("frequency is %d", xFrequency);

    uint16_t hue = getRandomHue();
    uint32_t ulNotifiedValue;
    TickType_t xLastWakeTime; // Keep track of the last time we woke up so we can be ultra precise
    for (;;)
    {

        /*
          TODO: Rather than use brightness, have it fade between colors. That's less jarring than
                turning off, and makes handling brightness from the configuration super easy.
        */

        // Wait for a our cue to start
        l.debug("waiting for a signal to start");
        xTaskNotifyWait(0x00, ULONG_MAX, &ulNotifiedValue, portMAX_DELAY);
        xLastWakeTime = xTaskGetTickCount();
        l.debug("got the signal, starting!");

        uint8_t currentBrightness = 0;
        for (uint8_t pixel = 0; pixel < 12; pixel++)
        {
            l.debug("now doing pixel %d", pixel);
            for (uint8_t currentBrightness = 1; currentBrightness - 1 < brightnessPerPixel; currentBrightness++)
            {
                // Wait until the right number of ticks
                vTaskDelayUntil(&xLastWakeTime, xFrequency);

                /*
                    The FreeRTOS ticker is pretty good, and this is a real time OS, but one the last one,
                    let's skip a step to allow time for the singaling to get lined up for the top of the
                    minute.
                */
                if (pixel == 11 && currentBrightness == 1)
                    currentBrightness++;

                l.verbose("now doing brightness %d", currentBrightness);

                // Fill in the already used pixels
                for (int j = 0; j < pixel; j++)
                {
                    strip.setPixelColor(j, strip.ColorHSV(hue, 242, brightnessPerPixel));
                }

                strip.setPixelColor(pixel, strip.ColorHSV(hue, 242, currentBrightness));

                // If we're at the top, reset for next time
                if (pixel == 11 && currentBrightness == brightnessPerPixel)
                {
                    hue = getRandomHue();
                }

                strip.show();
            }
        }
    }
}