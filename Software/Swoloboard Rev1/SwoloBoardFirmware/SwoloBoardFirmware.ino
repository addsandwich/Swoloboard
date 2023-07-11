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
 * This firmware code is designed for a motor controller board(Swoloboard) and provides essential functionality for controlling and managing
 * motors, ensuring precise and reliable movement. It includes features such as speed control, direction control, and feedback
 * mechanisms. The firmware is intended to be used in various applications that require motor control, such as robotics, 
 * automation, and mechatronics projects. The motor controller board in question named Swoloboard has basic functions such as battery read,
 * radio control through an NRF24L01 chip, and is designed around a mounted Arduino Pro Micro.  
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

// Radio
RF24 radio(18,4); // CE, CSN
const byte address[6] = "00585";
const uint8_t  channel = 185;

//Radio Bounds
//Low End 0
//High End 1024
const int LOW_END = 0;
const int HIGH_END = 1023;
const int MID = (HIGH_END - LOW_END + 1) / 2;
const int DEAD = 25;

// Give the motor control pins names:
#define pwmL1 6
#define pwmL2 5
#define pwmR1 10
#define pwmR2 9
#define dirLA 7
#define dirLB 8
#define dirRA 1
#define dirRB 0
#define kicker 3

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
  byte Switch1;
};

struct CMD_Tran{
  int LButton;
  int RButton;
  int LStickY;
  int LStickX;
  int RStickX;
  int RStickY;
  int LTrim;
  int RTrim;
  int Switch1;
};

CMD_Packet packet;
CMD_Tran cmd;

int tempL;
int tempR;

void setup() {
  Serial.begin(9600);
  pinMode(pwmL1, OUTPUT);
  pinMode(pwmL2, OUTPUT);
  pinMode(pwmR1, OUTPUT);
  pinMode(pwmR2, OUTPUT);
  pinMode(dirLA, OUTPUT);
  pinMode(dirLB, OUTPUT);
  pinMode(dirRA, OUTPUT);
  pinMode(dirRB, OUTPUT);
  pinMode(kicker, OUTPUT);

  //Set Up Serial Comms - Debug
  Serial.begin(9600);

  //Set up Radio
  radio.begin();
  radio.openReadingPipe(1, address);
  radio.setPALevel(RF24_PA_MIN,0);
  radio.setChannel(channel);
  Serial.println("Starting Radio");
  resetData(); 
  resetCMD(); 

  //to recieve
  radio.startListening();

  // Set speed to zero
  tempL = 0;
  tempR = 0;
  BrakeNow();
}

void loop() {

  if (radio.available()) {
    radio.read(&packet, sizeof(CMD_Packet));

    cmd.LStickY = packet.LStickYL | ((packet.LStickYH&0x03)<<8);
    cmd.LButton = packet.LButton;
    cmd.RStickY = packet.RStickYL | ((packet.RStickYH&0x03)<<8);
    cmd.RButton = packet.RButton;
  }
  else{
    resetData(); 
    resetCMD(); 
  }

  if (cmd.LStickY > (MID + DEAD)) {
    tempL = map(cmd.LStickY,MID,HIGH_END,0,255);
    LeftForward(tempL);
  }
  else if (cmd.LStickY < (MID - DEAD)){
    tempL = map(cmd.LStickY,MID,LOW_END,0,255);
    LeftBackwards(tempL);
  } 
  else {
    BrakeLeft();
  }

  if (cmd.RStickY > (MID + DEAD)) {
    tempR = map(cmd.RStickY,MID,HIGH_END,0,255);
    RightForward(tempR);
  }
  else if (cmd.RStickY < (MID - DEAD)){
    tempR = map(cmd.RStickY,MID,LOW_END,0,255);
    RightBackwards(tempR);
  } 
  else {
    BrakeRight();
  }

  if (cmd.LButton == 0 || cmd.RButton == 0){
    Kick(1);
  }else{
    Kick(0);
  }


  delay(5);

}

void LeftForward(int speed){
  Serial.print("Left Speed:");
  Serial.println(speed);
  digitalWrite(dirLA, LOW);   //Direction A = B
  digitalWrite(dirLB, HIGH);   //Direction A = B
  
  analogWrite(pwmL1, speed);   //Engage Motor1
  analogWrite(pwmL2, speed);   //Engage Motor2
}

void RightForward(int speed){
  Serial.print("Right Speed:");
  Serial.println(speed);
  digitalWrite(dirRA, LOW);   //Direction A = B
  digitalWrite(dirRB, HIGH);   //Direction A = B
  
  analogWrite(pwmR1, speed);   //Engage Motor1
  analogWrite(pwmR2, speed);   //Engage Motor2
}

void LeftBackwards(int speed){
  Serial.print("Left Speed:");
  Serial.println(speed);
  digitalWrite(dirLA, HIGH);   //Direction A = B
  digitalWrite(dirLB, LOW);   //Direction A = B
  
  analogWrite(pwmL1, speed);   //Engage Motor1
  analogWrite(pwmL2, speed);   //Engage Motor2
}

void RightBackwards(int speed){
  Serial.print("Right Speed:");
  Serial.println(speed);
  digitalWrite(dirRA, HIGH);   //Direction A = B
  digitalWrite(dirRB, LOW);   //Direction A = B
  
  analogWrite(pwmR1, speed);   //Engage Motor1
  analogWrite(pwmR2, speed);   //Engage Motor2
}

void BrakeLeft(){
  digitalWrite(dirLA, LOW);   //Direction A = B
  digitalWrite(dirLB, LOW);   //Direction A = B
  digitalWrite(pwmL1, HIGH);   //Engage the Brake
  digitalWrite(pwmL2, HIGH);   //Engage the Brake

}

void BrakeRight(){
  digitalWrite(dirRA, LOW);   //Direction A = B
  digitalWrite(dirRB, LOW);   //Direction A = B
  digitalWrite(pwmR1, HIGH);   //Engage the Brake
  digitalWrite(pwmR2, HIGH);   //Engage the Brake
}

void BrakeNow(){
  BrakeLeft();
  BrakeRight();
}

void Kick(int kickit){
  if(kickit){
    digitalWrite(kicker, HIGH);
  }else{
    digitalWrite(kicker, LOW);
  }
}

void resetData() {
  // Reset the values when there is no radio connection - Set initial default values
  packet.LButton = 1;
  packet.RButton = 1;
  packet.LStickXL = 0x00;
  packet.LStickYL = 0x00;
  packet.RStickXL = 0x00;
  packet.RStickYL = 0x00;
  packet.LTrimL = 0xFF;
  packet.RTrimL = 0xFF;
  packet.LStickXH = 0x01;
  packet.LStickYH = 0x01;
  packet.RStickXH = 0x01;
  packet.RStickYH = 0x01;
  packet.LTrimH = 0x01;
  packet.RTrimH = 0x01;
}

void resetCMD() {
  // Some of the comments are for when this was used to control a drone
  cmd.LButton=1;
  cmd.RButton=1;
  cmd.LStickY=MID;
  cmd.LStickX=MID;//YAW
  cmd.RStickX=MID;//ROLL
  cmd.RStickY=MID;//PITCH
  cmd.LTrim=0;//THROTTLE LARGE
  cmd.RTrim=0;//THROTTLE SMALL
  cmd.Switch1=1;
}