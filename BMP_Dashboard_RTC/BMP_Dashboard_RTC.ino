#include <ESP8266WiFi.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP085_U.h>
#include <RTClib.h>

// Create instances for the BMP180 and RTC
Adafruit_BMP085_Unified bmp = Adafruit_BMP085_Unified(10085);
RTC_DS1307 rtc;

const char* ssid = "FunBox2-A303";
const char* password = "C7C3E66A631A2E4A69491C61AE";

WiFiServer server(80);

void setup() {
    Serial.begin(115200);
    delay(10);

    // Connect to Wi-Fi
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, password);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    // Initialize the BMP180 sensor
    if (!bmp.begin()) {
        Serial.println("Could not find a valid BMP085 sensor, check wiring!");
        while (1);
    }

    // Initialize the RTC
    if (!rtc.begin()) {
        Serial.println("Couldn't find RTC");
        while (1);
    }

    if (!rtc.isrunning()) {
        Serial.println("RTC is NOT running, let's set the time!");
        rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }

    server.begin();
    Serial.println("HTTP server started");
}

void loop() {
    // Check if a client has connected
    WiFiClient client = server.available();
    if (!client) {
        return;
    }

    // Wait until the client sends some data
    Serial.println("new client");
    while (!client.available()) {
        delay(1);
    }

    // Read the first line of the request
    String request = client.readStringUntil('\r');
    Serial.println(request);
    client.flush();

    // Check if the request is for the root page or for the data endpoint
    if (request.indexOf("GET /data") != -1) {
        // Read data from the BMP180 sensor
        sensors_event_t event;
        bmp.getEvent(&event);

        float pressure = 0;
        float temperature = 0;
        if (event.pressure) {
            pressure = event.pressure;
            bmp.getTemperature(&temperature);
        } else {
            Serial.println("Sensor error!");
        }

        // Get date and time from RTC
        DateTime now = rtc.now();
        String date = String(now.year()) + "/" + String(now.month()) + "/" + String(now.day());
        String time = String(now.hour()) + ":" + String(now.minute()) + ":" + String(now.second());

        // Prepare the JSON response
        String jsonResponse = "{\"pressure\":" + String(pressure) + ",\"temperature\":" + String(temperature) + ",\"date\":\"" + date + "\",\"time\":\"" + time + "\"}";

        // Send the JSON response to the client
        client.print("HTTP/1.1 200 OK\r\nContent-Type: application/json\r\n\r\n");
        client.print(jsonResponse);
    } else {
        // Prepare the HTML page
        String response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
        response += "<!DOCTYPE HTML>\r\n<html>\r\n";
        response += "<h1>ESP8266 Sensor Data</h1>\r\n";
        response += "<p>Pressure: <span id='pressure'></span> hPa</p>\r\n";
        response += "<p>Temperature: <span id='temperature'></span> C</p>\r\n";
        response += "<p>Date: <span id='date'></span></p>\r\n";
        response += "<p>Time: <span id='time'></span></p>\r\n";
        response += "<script>\r\n";
        response += "setInterval(function() {\r\n";
        response += "  fetch('/data').then(response => response.json()).then(data => {\r\n";
        response += "    document.getElementById('pressure').innerText = data.pressure;\r\n";
        response += "    document.getElementById('temperature').innerText = data.temperature;\r\n";
        response += "    document.getElementById('date').innerText = data.date;\r\n";
        response += "    document.getElementById('time').innerText = data.time;\r\n";
        response += "  });\r\n";
        response += "}, 1000);\r\n";
        response += "</script>\r\n";
        response += "</html>\r\n";

        // Send the HTML response to the client
        client.print(response);
    }

    delay(1);
    Serial.println("Client disconnected");
    Serial.println("");
}
