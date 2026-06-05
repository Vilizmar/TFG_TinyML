#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "ARPf-556";
const char* password = "Arduino1234";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient portentaClient;
PubSubClient mqttClient(portentaClient);

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println("\n--- INICIANDO PRUEBA MQTT ---");
  Serial.print("Conectando a ");
  Serial.println(ssid);
  
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\n¡WiFi Conectado!");
  Serial.print("IP Asignada: ");
  Serial.println(WiFi.localIP());


  mqttClient.setBufferSize(128);

  mqttClient.setServer(mqtt_server, 1883);
}

void loop() {
  // Si se desconecta volver a conectarla
  if (!mqttClient.connected()) {
    Serial.println("Intentando conectar al broker HiveMQ...");
    // El nombre debe ser único
    if (mqttClient.connect("Portenta_TFG_Test_999998")) {
      Serial.println("¡Conectado a HiveMQ con éxito!");
    } else {
      Serial.print("Fallo MQTT, código de error = ");
      Serial.println(mqttClient.state());
      delay(5000);
      return;
    }
  }
  
  mqttClient.loop();
  
  // Publicar un mensaje de prueba cada 5 segundos
  Serial.println("Enviando mensaje de prueba...");
  mqttClient.publish("tfgsenda/vlizmar/nodo1", "{\"estado\":\"vivo\", \"mensaje\":\"mamsmasdafb\"}");
  
  delay(5000);
}