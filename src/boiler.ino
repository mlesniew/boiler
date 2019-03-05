#include <OneWire.h>
#include <DallasTemperature.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>
#include <ArduinoJson.h>

#include "digital_output.h"
#include "blinker.h"
#include "timer.h"

#define HOSTNAME "Boiler"
#define PASSWORD "abracadabra"
#define RESOLUTION 10

struct Sensor {
    const DeviceAddress address;
    const char * name;
    bool connected, got_reading;
    float last_reading;
};

Sensor sensor_info[] = {
    { { 0x28, 0x11, 0xed, 0x05, 0x17, 0x13, 0x01, 0x12 }, "first" },
    { { 0x28, 0x1d, 0xb3, 0xbc, 0x16, 0x13, 0x01, 0xf1 }, "second" },
    { { 0x28, 0xff, 0x3e, 0x28, 0x41, 0x18, 0x02, 0xbe }, "third" },
    { { 0 }, nullptr }
};

ESP8266WebServer server(80);

Led led_sensor(D2);
Led led(D4, true);

Blinker blinker(led, 200, 0b1 << 30);
Blinker blinker_sensor(led_sensor, 150, 0);

OneWire onewire(D1);
DallasTemperature sensors(&onewire);

void reboot() {
    Serial.println("Reboot...");
    while (true) {
        ESP.restart();
        delay(10 * 1000);
    }
}

String addr2str(const DeviceAddress addr) {
    static constexpr char digit[] = "0123456789abcdef";
    char result[3 * 8];
    unsigned int pos = 0;
    for (int i = 0; i < 8; ++i) {
        const auto val = addr[i];
        result[pos++] = digit[(val >> 4) & 0xf];
        result[pos++] = digit[val & 0xf];
        result[pos++] = '-';
    }
    result[--pos] = '\0';
    return String(result);
}

Sensor * get_sensor(const DeviceAddress addr) {
    for (Sensor * s = sensor_info; s->name; ++s) {
        if (memcmp(addr, s->address, 8) == 0)
            return s;
    }
    return nullptr;
}

void setup_wifi() {
    BlinkerSettingGuard bg(blinker, 0b101 << 10);
    WiFi.hostname(HOSTNAME);
    WiFiManager wifiManager;

    wifiManager.setAPCallback([](WiFiManager *) {
            blinker.set_pattern(0b10101 << 10); 
        });

    if (!wifiManager.autoConnect(HOSTNAME, PASSWORD)) {
        Serial.println("AutoConnect failed, reboot.");
        reboot();
    }
}

void setup_endpoints() {
    Serial.println("Setting up endpoints...");

    server.on("/alive", []{
            server.send(200, "text/plain", HOSTNAME " is alive");
            });

    server.on("/version", []{
            server.send(200, "text/plain", __DATE__ " " __TIME__);
            });

    server.on("/", []{
            StaticJsonBuffer<1024> buf;
            JsonArray & doc = buf.createArray();

            for (const Sensor * s = sensor_info; s->name; ++s) {
                JsonObject & e = buf.createObject();
                e["addr"] = addr2str(s->address);
                e["name"] = s->name;
                if (s->got_reading) {
                    e["temperature"] = s->last_reading;
                } else {
                    e["temperature"] = (char*)0;
                }
                doc.add(e);
            }
            doc.prettyPrintTo(Serial);
            String ret;
            doc.printTo(ret);
            server.send(200, "application/json", ret);
        });
}

void setup_sensors() {
    Serial.println("Setting up sensors...");

    for (Sensor * s = sensor_info; s->name; ++s) {
        s->connected = false;
        s->got_reading = false;
        s->last_reading = -127;
    }

    sensors.begin();

    const auto device_count = sensors.getDeviceCount();

    Serial.println("Number of One Wire devices: " + String(device_count));
    for (auto idx = 0; idx < device_count; ++idx) {
        DeviceAddress addr;
        if (!sensors.getAddress(addr, idx)) {
            Serial.println("Error reading address of device at index " + String(idx));
            continue;
        }

        Serial.print("    " + addr2str(addr));
        if (!sensors.validFamily(addr)) {
            Serial.println(" - unsupported device");
            continue;
        }

        Sensor * s = get_sensor(addr);
        if (!s) {
            Serial.println(" - unrecognized device");
            continue;
        }

        const auto resolution = sensors.getResolution(addr);
        if (resolution != RESOLUTION) {
            Serial.print(" - updating resolution to " + String(RESOLUTION));
            sensors.setResolution(RESOLUTION);
        }

        Serial.println(" - recognized as '" + String(s->name) + "'");
        s->connected = true;
    }

    Serial.println("Sensor setup complete.");
}

void setup() {
    Serial.begin(9600);

    // blink the diode really fast until setup() exits
    BlinkerSettingGuard bg(blinker, 0b10);

    setup_sensors();
    setup_wifi();
    setup_endpoints();

    Serial.println("Starting up server...");
    server.begin();
    Serial.println("Setup complete");
}

unsigned int update_readings() {
    Serial.println("Reading sensors...");
    unsigned int count = 0;
    sensors.requestTemperatures();
    for (Sensor * s = sensor_info; s->name; ++s) {
        s->last_reading = sensors.getTempC(s->address);
        Serial.print("    " + String(s->name));
        if (s->last_reading != -127) {
            if (!s->connected) {
                Serial.println(" -- just connected");
                s->got_reading = false;
            } else {
                Serial.println(": " + String(s->last_reading));
                s->got_reading = true;
                ++count;
            }
            s->connected = true;
        } else {
            Serial.println(" -- not connected");
            s->got_reading = false;
            s->connected = false;
        }
    }
    Serial.println("Done, got " + String(count) + " readings.");
    return count;
}

void loop() {
    static Periodic sensor_reader(60 * 1000, []{
            unsigned int c = update_readings();
            unsigned long pattern = 0;
            while (c-- > 0) {
                pattern <<= 2;
                pattern |= 0b10;
            }
            pattern <<= 8;
            blinker_sensor.set_pattern(pattern);
        });

    sensor_reader.tick();
    server.handleClient();
}