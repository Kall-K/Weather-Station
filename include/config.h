#ifndef CONFIG_H
#define CONFIG_H

const int STATION_ID = 1;

struct SENSOR {
    String name;
    String values[10];
};

struct SENSOR sensors[] = {
    {"DHT22", {"temperature", "humidity"}},
    {"SOIL_MOISTURE", {"soil_moisture"}}
};

#endif