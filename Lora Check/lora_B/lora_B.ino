#include <SoftwareSerial.h>

SoftwareSerial LoRa(D5, D6);   // Lora Tx to EspD5, Lora Rx to ESPD6

void send(String c){
  LoRa.print(c + "\r\n");
  delay(300);
}

void setup(){
  Serial.begin(115200);
  LoRa.begin(115200);

  delay(1500);

  send("AT");
  send("AT+BAND=865000000");     // MUST MATCH ESP32-C6
  send("AT+NETWORKID=6");
  send("AT+PARAMETER=12,4,1,7");
  send("AT+ADDRESS=2");

  Serial.println("Configured B");
}

void loop(){
  while(LoRa.available()){
    Serial.write(LoRa.read());
  }
}
