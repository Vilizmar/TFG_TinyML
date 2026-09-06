#include <WiFi.h>

const char* ssid = "ARPf-556";
const char* password = "Arduino1234";

void setup() {

    Serial.begin(115200);
    delay(2000);

    Serial.println("\n--- PRUEBA WIFI ---");

    Serial.print("Iniciando WiFi en: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    unsigned long startTime = millis();

    while (WiFi.status() != WL_CONNECTED &&
           millis() - startTime < 30000) {

        delay(500);

        Serial.print(".");
        Serial.print(" status=");
        Serial.println(WiFi.status());
    }

    Serial.println();

    if (WiFi.status() == WL_CONNECTED) {

        Serial.println("***** WIFI CONECTADO *****");

        Serial.print("IP: ");
        Serial.println(WiFi.localIP());

    } else {

        Serial.println("***** WIFI NO CONECTADO *****");

        Serial.print("Estado final: ");
        Serial.println(WiFi.status());
    }
}

void loop() {

    Serial.print("Estado WiFi: ");
    Serial.println(WiFi.status());

    delay(2000);
}