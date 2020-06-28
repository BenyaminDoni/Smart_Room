#include <ESP8266WiFi.h>
#include <MQTT.h>
#include <SPI.h>
#include <MFRC522.h>   //Library RFID
#include <Servo.h>     //Servo

#define ledON 3
#define ledOFF 1
#define pintu 2

//inisialisasi pin pada RFID
#define RST_PIN D1  
#define SS_PIN D2

int pos = 0;
int j=0;
String uidTag = "";

const char ssid[] = "Doni";
const char pass[] = "smartHome";
unsigned long lastMillis = 0;

const char* msgSistemON = "Sistem ON";
const char* msgSistemOFF = "Sistem OFF";
const char* msgLampuON = "Lampu Hidup";
const char* msgLampuOFF = "Lampu Mati";
const char* msgDoorLock = "Pintu terkunci";
const char* msgDoorUnlock = "Pintu terbuka";

MFRC522 mfrc522(SS_PIN, RST_PIN);
MFRC522::MIFARE_Key key;

Servo myservo;

WiFiClient net;
MQTTClient client;

void connect() {
  Serial.print("checking wifi...");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(1000);
  }

  Serial.print("\nconnecting...");
  while (!client.connect("arduino", "smart_room_IoT", "71426ace9dc64a15")) {
    Serial.print(".");
    delay(1000);
  }

  Serial.println("\nconnected!");

  client.subscribe("/SYSTEM");
  client.subscribe("/LAMPU");
  client.subscribe("/PINTU");
}

void messageReceived(String &topic, String &payload) {
  Serial.println("incoming: " + topic + " - " + payload);
  Serial.println();

  if(j==3){j=1;}
  if(j==1){
  //      digitalWrite(ledON, HIGH);
  //      digitalWrite(ledOFF, LOW);
  //      client.publish("SYSTEM",msgSistemON);
  //LAMPU
  if ((String)payload == "lampuON") {
    for (pos = 50; pos <= 100; pos += 1) {
      // in steps of 1 degree
      myservo.write(pos);              
      delay(15);                       
    }
    for (pos = 100; pos <= 50; pos -= 1) { 
      myservo.write(pos);              
      delay(15);                       
    }
    Serial.println(msgLampuON);
    client.publish("LAMPU", msgLampuON);
  }else if((String)payload == "lampuOFF") {
    for (pos = 100; pos >= 50; pos -= 1) { // goes from 0 degrees to 180 degrees
    // in steps of 1 degree
    myservo.write(pos);              // tell servo to go to position in variable 'pos'
    delay(15);                       // waits 15ms for the servo to reach the position
    }
    for (pos = 50; pos >= 100; pos += 1) { // goes from 180 degrees to 0 degrees
      myservo.write(pos);              // tell servo to go to position in variable 'pos'
      delay(15);                       // waits 15ms for the servo to reach the position
    }
    Serial.println(msgLampuOFF);
    client.publish("LAMPU", msgLampuOFF);
    }
    //PINTU
    if ((String)payload == "open") {
      digitalWrite(pintu, LOW);
      Serial.println(msgDoorUnlock);
      client.publish("PINTU", msgDoorUnlock);
    }else if((String)payload == "close") {
      digitalWrite(pintu, HIGH);  // Turn the relay off
      Serial.println(msgDoorLock);
      client.publish("PINTU", msgDoorLock);
    }
  }
  
  if(j==2){
    digitalWrite(ledON, LOW);
    digitalWrite(ledOFF, HIGH);
    client.publish("SYSTEM",msgSistemOFF);
  }
}

void setup() {
  Serial.begin(9600);
  SPI.begin();
  mfrc522.PCD_Init();

  pinMode(ledON, OUTPUT);
  pinMode(ledOFF, OUTPUT);
  pinMode(pintu, OUTPUT);
  myservo.attach(16);
  
  WiFi.begin(ssid, pass);
  client.begin("broker.shiftr.io", net);
  client.onMessage(messageReceived);

  connect();
}

void loop() {
  client.loop();
  delay(10);  // <- fixes some issues with WiFi stability

  if (!client.connected()) {
    connect();
  }

  // Cek untuk kartu yang baru disisipkan 
  if (!mfrc522.PICC_IsNewCardPresent())
    return;

  // Jika nomor tag tidak diperoleh
  if (!mfrc522.PICC_ReadCardSerial())
    return;

  // Peroleh UID pada tag
  uidTag = "";
  for (byte j = 0; j < mfrc522.uid.size; j++) {
    char teks[3];
    sprintf(teks, "%02X", mfrc522.uid.uidByte[j]);
    uidTag += teks;
  }

  // jika tag RFID sesuai dengan yang terdaftar
  if(uidTag.substring(0) == "AA887889"){ 
    Serial.println("Akses Diterima");
    delay(1000);
    Serial.println("System ON");
    Serial.print("card ID: ");
    Serial.println(uidTag);
    client.publish("SYSTEM",msgSistemON);
    
    digitalWrite(ledON, HIGH);
    digitalWrite(ledOFF, LOW);
    j++;
  }
  else{
      Serial.println("Akses Ditolak");
//      Serial.println();
//      Serial.println("System OFF");
//      client.publish("SYSTEM",msgSistemOFF);
//      digitalWrite(ledON, LOW);
//      digitalWrite(ledOFF, HIGH);
  }  
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
}
