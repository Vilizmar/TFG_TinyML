#define loraSerial Serial1

void setup() {
  Serial.begin(115200);
  loraSerial.begin(9600);
  
  while (!Serial) delay(10);
  
  Serial.println("Iniciando conexión LoRaWAN");
  
  // Se lanza el JOIN a TTN
  loraSerial.println("AT+JOIN");
  
  // Damos 10 segundos de margen para que negocie con la red
  delay(10000); 
}

void loop() {
  Serial.println("\n[!] Enviando paquete");
  
  // AT+MSG envía un mensaje
  loraSerial.println("AT+MSG=\"Hola\"");
  
  // Leemos lo que nos contesta el módulo durante los siguientes 5 segundos
  unsigned long timeout = millis();
  while (millis() - timeout < 5000) {
    while (loraSerial.available()) {
      Serial.write(loraSerial.read());
    }
  }
  
  // En LoRaWAN no se debe enviar datos constantemente. 
  // Se esperan 30 segundos.
  delay(30000); 
}