#include <Dhcp.h>
#include <Dns.h>
#include <Ethernet.h>
#include <EthernetClient.h>
#include <EthernetServer.h>
#include <EthernetUdp.h>
#include <util.h>
#include <SPI.h>
#include <Servo.h>
#include <TimedAction.h>

int pinTrigger = 30;  //Der Trigger-Pin des Distanzsensors
int pinEcho = 31;     //Der Echo-Pin des Distanzsensors
int pinLenkung = 2;   //Der Pin für die Lenkung
int pinSpeed = 3;     //Die PWM-Pins der Servos

int currentSpeed = 100; //Die derzeitige Geschwindigkeit
int currentAngle = 90;  //Der Aktuelle Winkel des Lenkservos 90=neutral >90 = rechts <90=links; Maximalwerte 50 und 130

Servo servoLenkung,servoSpeed; //Die Objekte zur Steuerung der Servos
            
int packetSize =0;             //Die Größe des angekommenen Packets
long distance = 0;            //Die Ditanz des Abstandssensor

char currentOrder;            //Der derzeitige Befehl

TimedAction sensortimer = TimedAction(1000,readSensor);      //Der Timer für die Sensorabrufe
TimedAction distancetimer = TimedAction(1000,sendDistance);  //Der Timer für das Senden der Distanz
TimedAction statustimer = TimedAction(1000,printStatus);     //Der Timer für die Ausgabe des Status
  
byte mac[] = { 
  0xDE, 0xAD, 0xBE, 0xEF, 0xFE, 0xED }; // Die MAC-Adresse des Arduino
IPAddress ip(10,0,0,99);                // IP des Arduino
IPAddress remote(10,0,0,101);           // IP des Client (Smartphone)
IPAddress gateway(10,0,0,1);            // Gateway
IPAddress subnet(255,255,255,0);        // Subnetadresse

int remoteport=8888;                      //Der Port unseres Clienten(Smartphone)
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];  //Der Buffer in dem unsere empfangenen Plakete gelagert werden
EthernetUDP Udp;                            //Das Objekt fÃ¼r unsere UDP-Verbindung

void setup(){
  servoLenkung.attach(pinLenkung);        //WeiÃŸt dem Lenkservo den Pin 2 zu
  servoSpeed.attach(pinSpeed);            //WeiÃŸt dem Speedervo den Pin 3 zu

  pinMode(pinEcho, INPUT);                //Setzt den PinMode für den Echo-Pin
  pinMode(pinTrigger, OUTPUT);            //Setzt den PinMode für den Trigger-Pin

  Ethernet.begin(mac,ip,gateway,subnet);  //Ethernet wird initialisiert
  Udp.begin(8888);                        //Startet den UDP-Server auf Port 8888

  Serial.begin(9600);
  Serial.println("Server-IP: ");
  Serial.println(Ethernet.localIP());     // Ausgabe der eigenen IP
  Serial.println("Waiting 3 Seconds for Car to boot");  //Startprozess des Autos abwarten
  delay(1000);
  Serial.println("1");
  delay(1000);
  Serial.println("2");
  delay(1000);
  Serial.println("3");
}

void loop(){
  receiveOrder(); //liefert den Befehl vom Ethernetshield
  processOrder(currentOrder,servoLenkung, servoSpeed); //verarbeitet den erhaltenen Befehl
  sensortimer.check();      //checkt jede Sekunde die Sensordaten
  //distancetimer.check();    //schickt jede Sekunde die derzeitige Distanz zum Clienten
  statustimer.check();      //schickt jede Sekunde den derzeitigen Status an die Console
}

void receiveOrder(){ // Diese Methode dient dazu Befehle aus dem WLAN-Netz auszulesen
  packetSize = Udp.parsePacket();  // Schaut nach ob ein Paket gekommen ist
  if(packetSize){                      // Ist  ein Paket gekommen, wird dieses verarbeitet 
    Udp.read(packetBuffer,UDP_TX_PACKET_MAX_SIZE);  //liest das Paket in den Buffer ein
    currentOrder = packetBuffer[0];  //holt sich den ersten Wert aus dem Buffer (unser Befehl)
  }
  else {
    currentOrder = 'x';
  }
}

void processOrder(char order,Servo lenkung, Servo speeds){ //Diese Methode dient dazu den erhaltenen Wert aus receiveOrder zu verarbeiten
  currentAngle = lenkung.read(); //liest den derzeitigen Wert des Lenkservos aus 
  switch (currentOrder){
    case 'f': //v ... vorwÃ¤rts
      if(currentSpeed < 150){
         currentSpeed=currentSpeed+1;  //ErhÃ¶ht die derzeitige Geschwindigkeit
         speeds.write(currentSpeed);   //Schreibt den Wert in den Servo     
      }
      break;
    case 'b': //z ... zurÃ¼ck
      if(currentSpeed > 70){
        currentSpeed=currentSpeed-1;  //Senkt die derzeitige Geschwindigkeit 
        speeds.write(currentSpeed);   //siehe case 'f'
      }  
      break;
    case 'l': //l ... links 
      if(currentAngle >= 70 && currentAngle <= 110){
        currentAngle = currentAngle - 5; //Ã„ndert den derzeitigen Lenkwinkel         
      }
      else{   // ist der Wert des Servos ausserhalb des Wertebereichs, wird er hier entsprechend korrigiert
        if(currentAngle>110){
            currentAngle=109;
        }
            
        if(currentAngle<70){
           currentAngle=71;
        }       
      }
      lenkung.write(currentAngle);
      break;
    case 'r': //r ... rechts
      if(currentAngle >= 70 && currentAngle <= 110){
        currentAngle = currentAngle + 5;  //Siehe case 'l'         
      }
      else{   // ist der Wert des Servos ausserhalb des Wertebereichs, wird er hier entsprechend korrigiert
        if(currentAngle>110){
            currentAngle=109;
        }       
        if(currentAngle<70){
           currentAngle=71;
        }       
      }
      lenkung.write(currentAngle);  //Schreibt den Wert in den Servo
      break;
    case 's': //s ... stop
      speeds.write(90);
      break;
    }    
} 
void printStatus(){     //Ausgabe der Statusdaten
    Serial.println();
    Serial.print("Geschwindigkeit: ");
    Serial.print(currentSpeed);
    Serial.print(" Lenkwinkel: ");
    Serial.print(currentAngle);
    Serial.print(" Abstand: ");
    Serial.print(distance); 
    Serial.print(" Befehl: ");
    Serial.print(currentOrder); 
}

void sendDistance(){      //Leitet die derzeitige Distanz an den Clienten weiter
//if(packetSize){                      // Ist  ein Paket gekommen, wird dieses verarbeitet 
    //remote =  Udp.remoteIP();    //Holt sich die IP des Clienten
    //remoteport = Udp.remotePort();   //Holt sich den Port des Clienten
    Udp.beginPacket(remote,remoteport);  //Startet ein UDP-Packet
    Udp.write(distance);                 //Schreibt Daten in dieses
    Udp.endPacket();                 // Beendet das UDP Paket
    
  //}
}

void readSensor(){
  long duration;
  digitalWrite(pinTrigger, LOW); 
  delayMicroseconds(2);
  digitalWrite(pinTrigger,HIGH);
  delayMicroseconds(10);
  digitalWrite(pinTrigger,LOW);
  
  duration = pulseIn(pinEcho, HIGH);  //Wartet auf einen Pulse vom Ultraschallsensor
  distance = (duration/2) /29;  
  if(distance > 300){
    distance = 300;
    if(distance < 4){
      distance =4;
    }
  }
}



