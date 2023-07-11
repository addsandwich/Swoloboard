/*
 * Copyright (c) 2023 Christopher Watson
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files
 * (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify,
 * merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * Purpose:
 * This firmware code is designed for a Simple Arduino controlled NRF24L01 to provide radio connection to a Swoloboard. This
 * particular set of firmware was based on an Arduino Pro Mini.  
 *
 * Library: TMRh20/RF24, https://github.com/tmrh20/RF24/
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR
 * IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include <SPI.h>
#include <nRF24L01.h>
#include <RF24.h>
RF24 radio(2, 10); // CE, CSN
const byte address[6] = "00585";
const uint8_t  channel = 185;
//Low End 0
//High End 1024
const int LOW_END = 0;
const int HIGH_END = 1023;




// Define the digital inputs
#define lBTN 8  // Joystick button 1
#define rBTN 3  // Joystick button 2
#define BBYMode 9 // Switch 1

// Max size 32 bytes because of buffer limit
struct CMD_Packet {
  byte LButton;
  byte RButton;
  byte LStickXL;
  byte LStickXH;
  byte LStickYL;
  byte LStickYH;
  
  byte RStickXL;
  byte RStickXH;
  byte RStickYL;
  byte RStickYH;
  byte LTrimL;
  byte LTrimH;
  byte RTrimL;
  byte RTrimH;
  byte BbyMode;
};

//Make command packet
CMD_Packet packet;

void setup() {
  Serial.begin(9600);
  radio.begin();
  radio.openWritingPipe(address);
  radio.setPALevel(RF24_PA_LOW,0);
  radio.setChannel(channel);
  radio.stopListening();
  Serial.println("Sending");
  
  //Set Up Controls
  pinMode(lBTN, INPUT_PULLUP);
  pinMode(rBTN, INPUT_PULLUP);
  pinMode(BBYMode, INPUT_PULLUP);
}



void loop() {

  //temp vals
  int tempx = 0;
  int tempy = 0;

  
  // Read all analog inputs and map them to one Byte value
  //Gather Left Stick Values
//  tempx = map(analogRead(A3),0,1023,LOW_END,HIGH_END);
//  tempy = (map(analogRead(A2),0,1023,HIGH_END,LOW_END));
  tempx = analogRead(A1);
  tempy = ~analogRead(A0);
  packet.LStickXL = (byte)tempx;
  packet.LStickXH = (byte)(tempx>>8);
  packet.LStickYL = (byte)tempy;
  packet.LStickYH = (byte)(tempy>>8);
  //Gather Right Stick Values
//  tempx = map(analogRead(A0),0,1023,LOW_END,HIGH_END);
//  tempy = (map(analogRead(A1),0,1023,HIGH_END,LOW_END));
  tempx = analogRead(A3);
  tempy = ~analogRead(A2);
  packet.RStickXL = (byte)tempx;
  packet.RStickXH = (byte)(tempx>>8);
  packet.RStickYL = (byte)tempy;
  packet.RStickYH = (byte)(tempy>>8); 
  //Gather Trim Values
//  tempx = map(analogRead(A7),0,1023,LOW_END,HIGH_END);
//  tempy = map(analogRead(A6),0,1023,LOW_END,HIGH_END);
  tempx = analogRead(A7);
  tempy = analogRead(A6);
  packet.LTrimL = (byte)tempx;
  packet.LTrimH = (byte)(tempx>>8);
  packet.RTrimL = (byte)tempy;
  packet.RTrimH = (byte)(tempy>>8);
  // Read all digital inputs
  packet.LButton = digitalRead(lBTN);
  packet.RButton = digitalRead(rBTN);

  //If baby mode, go easy
  packet.BbyMode = digitalRead(BBYMode);

  // Send the whole data from the structure to the receiver
  radio.write(&packet, sizeof(CMD_Packet));
  
  delay(2);
}
