/*
 * IR_Firmware_Barth.cpp
 *
 * Setup: No capacitors installed. Has always power. Placed where the receiver is visible for the apple remote and IR led can send commands to the projectionscreen and beamer.
 * Apple remote button configured: mute (double tap). Buttons configurable are the volume up, down and mute button. Double tap will not mess with the amplifier if configured because you will unmute it.
 * Double Tap mute button action: rolldown projection screen and start beamer or rollup projection screen and shutdown beamer according to the stored state. 
 * State in the beginning will be on poweroff.
 *
 *
 ************************************************************************************
 * MIT License
 *
 * Copyright (c) 2020-2021 Armin Joachimsmeyer
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is furnished
 * to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
 * PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE
 * OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 ************************************************************************************
 */

// Lib
#include <Arduino.h>
#include "PinDefinitionsAndMore.h"
#include <IRremote.h>

#define DELAY_SCREEN_DOWN 17000           // Setting Delay screen
#define DELAY_QUICK_WAIT 500
#define RECV_PIN 7                        // IR receiver

// Two stored states.
#define STATE_ROLLEDUP_BEAMER_OFF 1
#define STATE_ROLLEDDOWN_BEAMER_ON 2

// Time Limit double click button. Will reset.
#define MAX_TIME_LIMIT_BETWEEN_CLICKS 2000 // 2000ms
#define MIN_TIME_LIMIT_BETWEEN_CLICKS 200 // 500ms

// Commands projection screen
const uint8_t rawlen = 64;
unsigned int bufferUP[rawlen] = {1250,350,1300,350,1300,350,1300,400,400,1200,450,1200,450,1200,450,1200,450,1200,450,1200,450,1200,1250,400,450,1200,400,1250,400,1250,1250,400,1250,400,1250,400,400,1250,1250,350,1300,400,1250,350,450,1200,1250,400,1250,400,1250,400,450,1200,1250,400,1250,400,1250,400,1250,400,450};
unsigned int bufferSTOP[rawlen] = {1300,400,1250,400,1250,400,1250,400,400,1200,450,1200,450,1200,450,1200,450,1200,450,1200,1250,400,450,1200,450,1200,450,1200,450,1200,1250,400,1250,400,1250,400,400,1250,400,1250,1250,400,1250,350,450,1200,450,1200,1300,350,1300,350,450,1200,1250,400,1250,400,1250,400,450,1200,1250};
unsigned int bufferDOWN[rawlen] = {1250,400,1250,400,1250,400,1250,400,450,1200,450,1200,450,1200,450,1200,400,1250,1250,400,400,1250,400,1250,400,1250,400,1200,450,1200,1250,400,1250,400,1250,400,1250,400,450,1200,1250,400,1250,400,1250,400,450,1200,450,1200,450,1200,450,1200,1250,400,1250,400,1250,400,1250,400,1250};
unsigned int bufferUPSMALL[rawlen] = {1250,450,1200,450,1200,450,1200,450,400,1200,450,1200,450,1200,400,1250,1250,400,400,1250,400,1250,400,1250,400,1200,450,1200,450,1200,1250,450,1200,400,1250,450,400,1200,1250,400,450,1200,450,1200,450,1200,1250,400,450,1200,1250,400,450,1200,400,1250,400,1250,400,1250,1200,450,400};
unsigned int bufferDOWNSMALL[rawlen] = {1300,400,1250,400,1250,400,1200,450,400,1200,450,1200,450,1200,1250,450,400,1200,450,1200,450,1200,450,1200,400,1250,400,1250,400,1250,1250,400,1250,400,1250,400,1250,400,400,1200,450,1200,450,1200,450,1200,1250,450,1200,450,1200,450,400,1200,1250,400,450,1200,450,1200,1250,400,1250};

// Command mute Apple remote
const String receiverMuteButtonApple = "d";

// State (initial)
int state = STATE_ROLLEDUP_BEAMER_OFF;
int counterPressMuteButton = 0;
unsigned long timePressedFirstTime = 0; // this is used to determine the first time press. when exceed time limit reset counter. Makes sure that when using mute for amp it will not mess with the beamer/screen.

// Time (one call to millis() in loop)
unsigned long timeNow = 0;

void setup() 
{
      //pinMode(LED_BUILTIN, OUTPUT);
      //Serial.begin(115200);
      //Serial.println(F("Starting!"));  
      IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
      IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); // Start the receiver, enable feedback LED, take LED feedback pin from the internal boards definition
      IrSender.enableIROut(38);
      IrReceiver.enableIRIn();
}

void debugOutputReceiver() {
  Serial.print(F("Decoded protocol: "));
  Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
  Serial.print(F("Decoded raw data: "));
  Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
  Serial.print(F(", decoded address: "));
  Serial.print(IrReceiver.decodedIRData.address, HEX);
  Serial.print(F(", decoded command: "));
  Serial.println(IrReceiver.decodedIRData.command, HEX);
  Serial.println("String:" + String(IrReceiver.decodedIRData.command, HEX));
}

// Turn on or off Epson Beamer
void SendBeamerPowerCommand() 
{
    IrSender.sendNEC(0x5583, 0x90, 0);
    delay(1000);
    IrSender.sendNEC(0x5583, 0x90, 0);
    delay(1000);
    IrSender.sendNEC(0x5583, 0x90, 0);
    delay(1000);
    IrSender.sendNEC(0x5583, 0x90, 0);
    delay(1000);
}

// Commit the state change that has been made.
void commitStateIR() 
{
  if(state == STATE_ROLLEDUP_BEAMER_OFF) 
  {
    //do power off (rollup and shutdown)    
    IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);     
    delay(DELAY_QUICK_WAIT);
    IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_QUICK_WAIT);
    IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_QUICK_WAIT);
    IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);     
    delay(DELAY_QUICK_WAIT);
    
  } 
  else 
  {
    // power on roll down  
    IrSender.sendRaw(bufferDOWN, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_QUICK_WAIT);
    IrSender.sendRaw(bufferDOWN, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_SCREEN_DOWN);    
    IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_QUICK_WAIT);
    IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_QUICK_WAIT);
    IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
    delay(DELAY_QUICK_WAIT);      
  }

  // On / Off beamer
  SendBeamerPowerCommand();
}

void ResetCounterAndTimeFirstPress() 
{
  counterPressMuteButton = 0; //reset counter
  timePressedFirstTime = 0; //reset first time pressed
}

void loop() 
{
    timeNow = millis();
    
    if (IrReceiver.available()) 
    {
        if (IrReceiver.decode()) 
        {
          if(String(IrReceiver.decodedIRData.command, HEX) == receiverMuteButtonApple) 
          {
            counterPressMuteButton++;

            // Filter clicks (can be sometimes more than one). MIN TIME BETWEEN CLICKS!!
            if(timeNow < (timePressedFirstTime + MIN_TIME_LIMIT_BETWEEN_CLICKS))
            {
              counterPressMuteButton--;
              timePressedFirstTime = timeNow; //reset time to now. (ignore long pressed button)
            }
            
            // Set start time pressed if not set when the button is first time pressed (mute button).
            if(counterPressMuteButton == 1 && timePressedFirstTime == 0) 
            {
              timePressedFirstTime = timeNow;
            }
            
            // Bigger then 1 execute action according to the current state.
            if(counterPressMuteButton > 1) 
            { 
              if(state == STATE_ROLLEDUP_BEAMER_OFF) 
              {
                state = STATE_ROLLEDDOWN_BEAMER_ON;
              } 
              else 
              {
                state = STATE_ROLLEDUP_BEAMER_OFF;
              }

              // method that commits the changed state to the screen and beamer.
              commitStateIR();

              // reset
              ResetCounterAndTimeFirstPress();
            }
          }
        }   
        
        IrReceiver.resume(); // resume receiver
    }

    // Exceed time limit between button presses. reset.
    if(counterPressMuteButton > 0 && timeNow > (unsigned long)(timePressedFirstTime + MAX_TIME_LIMIT_BETWEEN_CLICKS)) 
    {
      ResetCounterAndTimeFirstPress();
    }
}
