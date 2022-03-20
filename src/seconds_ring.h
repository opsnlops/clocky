
#pragma once

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>

#include <limits.h>

extern "C"
{
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
}

#include "logging/logging.h"

#define LED_RING_PIN 13

// This is 0.618033988749895 * (2**16)
#define GOLDEN_RATIO_CONJUGATE 40503

uint16_t getRandomHue();
uint16_t interpolateHue(uint16_t oldHue, uint16_t newHue, uint8_t totalSteps, uint8_t currentStep);

portTASK_FUNCTION_PROTO(secondRingTask, pvParameters);
