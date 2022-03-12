

/**
 * @file ota.c
 * @author Bunny (bunny@bunnynet.org)
 * @brief Provides a way to update over the air
 * @version 0.1
 * @date 2022-02-12
 *
 * @copyright Copyright (c) 2022
 *
 */

#include <Arduino.h>
#include <ArduinoOTA.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"
}

#include "ota.h"
#include "logging/logging.h"

static creatures::Logger l;

void setup_ota(String hostname)
{
    ESP_LOGV(OTA_TAG, "Prepping for OTA setup");

    ArduinoOTA.setHostname(hostname.c_str());

    l.info("OTA configured for %s.local", hostname);
}

void start_ota()
{
    ArduinoOTA.begin();

    TaskHandle_t creatureOTATaskHandle;
    xTaskCreate(creatureOTATask,
                "creatureOTATask",
                4096,
                NULL,
                1,
                &creatureOTATaskHandle);

    l.info("OTA ready");
}

/**
 * @brief A task that looks for updates every few seconds
 *
 * Creates a simple task that checks to see if there's a pending OTA
 * update waiting for us.
 */
portTASK_FUNCTION(creatureOTATask, pvParameters)
{
    for (;;)
    {
        l.verbose("Checking for pending updates");

        ArduinoOTA.handle();

        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}