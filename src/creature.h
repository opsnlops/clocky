#pragma once

#define CREATURE_NAME "clocky-yellow"
#define CREATURE_POWER "mains"


// Toss some things into the global namespace so that the libs can read it
const char* gCreatureName = CREATURE_NAME;

// Is the WiFi connected?
boolean gWifiConnected = false;
