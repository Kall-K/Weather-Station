#include <Arduino.h>
#include <SoftwareSerial.h>
#include <DHT.h>
#include <LiquidCrystal.h>

#include "config.h"
#include "secret.h"

#define DHTPIN 6
#define DHTTYPE DHT22
#define DELAY_TIME 5000

DHT dht(DHTPIN, DHTTYPE);

// Initialize with RS, E, D4, D5, D6, D7
LiquidCrystal lcd(12, 11, 5, 4, 3, 2); 

// ESP8266 serial
SoftwareSerial esp(7, 8); // RX, TX

bool esp_error = false;
bool wifi_connected = false;
bool server_connected = false;


bool waitForResponse(String cmd, int waitTime, String expectedResponse) {

    esp.println(cmd);

    String response = "";
    long int time = millis();
    
    while ((millis() - time) < waitTime) {
        while (esp.available()) {
            char c = esp.read();
            Serial.write(c);
            response += c;
            
            // return true if expected response is found in the response before timeout
            if (response.indexOf(expectedResponse) != -1) {
                return true; 
            }
        }
    }
    return false; // Timeout if expected response not found
}


bool connectWiFi() {

    // esp.println("AT+CWJAP?");

    if (!waitForResponse("AT+CWMODE=1", DELAY_TIME, "OK")) {
        return false;
    }

    String cmd = "AT+CWJAP=\"" + String(WIFI_SSID) + "\",\"" + String(WIFI_PASS) + "\"";

    if (waitForResponse(cmd, DELAY_TIME+10000, "OK")){
        lcd.clear();
        lcd.print("WiFi Connected");
        delay(2000);
        return true;
    } else {
        lcd.clear();
        lcd.print("WiFi Failed");
        delay(2000);
        return false;       
    }

}

void sendData(float values[], int values_size, int idx) {
    String readings;
    // readings.reserve(100);

    readings = "{";

    for (int i = 0; i < values_size; i++) {
        readings += "\"";
        readings += sensors[idx].values[i];
        readings += "\":";
        readings += String(values[i]);

        if (i < values_size - 1) {
            readings += ",";
        }
    }

    readings += "}";

    String json;

    json = "{\"stationId\":";
    json += String(STATION_ID);
    json += ",\"sensorName\":\"";
    json += sensors[idx].name;
    json += "\",\"readings\":";
    json += readings;
    json += "}";

    String startTCP = "AT+CIPSTART=\"TCP\",\"" + SERVER_IP + "\"," + SERVER_PORT;

    if (!waitForResponse(startTCP, DELAY_TIME, "OK")) {
        return;
    }

    String http;

    http = "POST /sensorValues HTTP/1.1\r\n";
    http += "Host: ";
    http += SERVER_IP;
    http += "\r\n";
    http += "Content-Type: application/json\r\n";
    http += "Connection: close\r\n";
    http += "Content-Length: ";
    http += String(json.length());
    http += "\r\n";
    http += "\r\n";
    http += json;


    Serial.print("json length: ");
    Serial.println(json.length());

    Serial.print("http length: ");
    Serial.println(http.length());

    Serial.println(http);

    String sendCmd = "AT+CIPSEND=" + String(http.length());
    esp.println(sendCmd);

    if(esp.find(">")) { 
        Serial.println("Sending.."); 
        esp.print(http);
        if(esp.find("SEND OK")) { 
            Serial.println("Packet sent");
            while (esp.available()) {
                String tmpResp = esp.readString();
                Serial.println(tmpResp);
            }
            esp.println("AT+CIPCLOSE");
        }
    }
    
}

void setup() {

    Serial.begin(9600);

    esp.begin(9600);

    dht.begin();

    lcd.begin(16, 2);

    lcd.clear();
    lcd.print("Starting...");


    // test ESP8266 connection
    if(!waitForResponse("AT", DELAY_TIME, "OK")) {
        esp_error = true;
        lcd.clear();
        lcd.print("ESP Error");
        delay(2000);
    }

    if(!esp_error && connectWiFi()) {
        wifi_connected = true;
        waitForResponse("AT+CIFSR", DELAY_TIME, "OK");
    }

    // test server connection
    if (wifi_connected) {
        lcd.clear();
        lcd.print("server test...");
        delay(2000);
        if (!waitForResponse("AT+CIPSTART=\"TCP\",\"" + String(SERVER_IP) + "\"," + String(SERVER_PORT), DELAY_TIME, "OK")) {
            server_connected = false;
            lcd.clear();
            lcd.print("server not reachable");
        } else {
            server_connected = true;
            lcd.clear();
            lcd.print("server connected");
        }

        waitForResponse("AT+CIPCLOSE", DELAY_TIME, "OK");
    }
}

void loop() {

    float dht22[] = {dht.readTemperature(), dht.readHumidity()};
    float soil_moisture[] = {analogRead(A4)};

    if (isnan(dht22[1]) || isnan(dht22[0])) {

        lcd.clear();
        lcd.print("dht Error");

        delay(2000);

        return;
    }

    if (isnan(soil_moisture[0])) {

        lcd.clear();
        lcd.print("Soil Moisture Error");

        delay(2000);

        return;
    }

    // LCD display
    lcd.clear();

    lcd.setCursor(0, 0);
    lcd.print("T:");
    lcd.print(dht22[0]);
    lcd.print(" H:");
    lcd.print(dht22[1]);

    lcd.setCursor(0, 1);
    lcd.print("Soil:");
    lcd.print(soil_moisture[0]);

    // Send to server
    if (server_connected) {
        sendData(dht22, 2, 0);
        delay(1000);
        sendData(soil_moisture, 1, 1);
    }

    delay(10000);
}
