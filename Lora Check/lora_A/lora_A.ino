HardwareSerial LoRa(1);

void send(String c){
  LoRa.print(c + "\r\n");
  delay(300);
}

void setup(){
  Serial.begin(115200);
  LoRa.begin(115200, SERIAL_8N1, 4, 5);   // LorA Tx to SDA (GPIO4), Lora Ex to SCL (GPIO5)

  delay(1500);

  send("AT");
  send("AT+BAND=865000000");        // India band (use 915000000 if elsewhere)
  send("AT+NETWORKID=6");
  send("AT+PARAMETER=12,4,1,7");
  send("AT+ADDRESS=1");

  Serial.println("Configured A");
}

void loop(){
  send("AT+SEND=2,5,HELLO");
  Serial.println("Sent HELLO");
  delay(3000);
}
