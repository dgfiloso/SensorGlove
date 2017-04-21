#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
 
const int pinCE = 9;
const int pinCSN = 10;
RF24 radio(pinCE, pinCSN);
 
// Single radio pipe address for the 2 nodes to communicate.
const uint64_t pipe = 0xE8E8F0F0E1LL;
 
int data[2];
 
void setup()
{
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
 }
 delay(50);
}
