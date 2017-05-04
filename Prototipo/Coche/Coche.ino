#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>

#define E1 3
#define E2 6
#define A1 5
#define B_1 4 
#define A2 8
#define B2 7
const int pinCE = 9;
const int pinCSN = 10;
RF24 radio(pinCE, pinCSN);
 
// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;
// recibimos una señal del 0 al 14 para cada motor, el 0 es parado, hasta el 7 positivo y del 8 al 14 negativo
void motorMov(int comando){
  if (comando==0){
     analogWrite(E1, 0);
    digitalWrite(A1, HIGH);
    digitalWrite(B_1, LOW);
    Serial.println("motor trasero parado");
  }
  if (comando<7 && comando!=0){ //del 1 al 6 para sentido positivo
    int velocidad =42*comando ; // 255/6 = 42 aprox 
     analogWrite(E1, velocidad);
    digitalWrite(A1, HIGH);
    digitalWrite(B_1, LOW);
     Serial.print("motor trasero moviendose en sentido positivo a velocidad: ");
     Serial.println(velocidad);
  }
   if (comando>6 && comando!=0){ //del 7 al 12 para sentido positivo
    int velocidad =42*(comando-7) ; // 255/6 = 42 aprox 
     analogWrite(E1, velocidad);
    digitalWrite(A1, LOW);
    digitalWrite(B_1, HIGH);
      Serial.print("motor trasero moviendose en sentido negativo a velocidad: "); 
       Serial.println(velocidad);
  }
}
void motorGir(int comando){
  if (comando==0){
     analogWrite(E2, 0);
    digitalWrite(A2, HIGH);
    digitalWrite(B2, LOW);
     Serial.println("motor delantero parado, (recto)");
     
  }
  if (comando==1){ //1 para sentido positivo
     analogWrite(E2, 255);
    digitalWrite(A2, HIGH);
    digitalWrite(B2, LOW);
    Serial.print("motor delantero moviendose en sentido positivo (derecha)");
  }
   if (comando==2){ //2 para sentido negativo
     analogWrite(E2, 255);
    digitalWrite(A2, LOW);
    digitalWrite(B2, HIGH);
    Serial.print("motor delantero moviendose en sentido negativo (izquierda)");
  }
}
int data[2];
 
void setup()
{
  pinMode(E1, OUTPUT);
  pinMode(A1, OUTPUT);
  pinMode(B_1, OUTPUT);
  pinMode(E2, OUTPUT);
  pinMode(A2, OUTPUT);
  pinMode(B2, OUTPUT);
 radio.begin();
 Serial.begin(9600); 
 radio.openReadingPipe(1, pipe);
 radio.startListening();
}
 
void loop()
{
 if (radio.available())
 {    
 radio.read(data,sizeof(data));
 Serial.print("Dato0= " );
 Serial.print(data[0]);
  Serial.print(" " );
 Serial.print("Dato1= " );
 Serial.println(data[1]);
 motorMov(data[0]);
 motorGir(data[1]);
 }
 //delay(50);
}
