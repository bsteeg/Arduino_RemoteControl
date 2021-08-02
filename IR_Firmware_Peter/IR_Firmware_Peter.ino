/*
 * IR_Firmware_Peter.cpp
 *
 * Setup: Will react to power up command beamer to roll down. It also includes all the other commands for 
 * controlling the projection screen including the timed stop after rolldown of the projection screen.
 *
 *  This file is part of Arduino-IRremote https://github.com/Arduino-IRremote/Arduino-IRremote.
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

#include <Arduino.h>

/*
 * Define macros for input and output pin etc.
 */
#include "PinDefinitionsAndMore.h"
#include <IRremote.h>

#define DELAY_AFTER_SEND 2000
#define DELAY_AFTER_LOOP 5000
#define DELAY_SCREEN_DOWN 16500
#define DELAY_QUICK_WAIT 500
#define DELAY_STARTUP_WAIT 2000
#define RECV_PIN 7

const int powerButton = 6; // power input gone detect.
const int groundpin = 10;  
uint8_t firsttime = 0;     // first time startup execution.

void setup() 
{
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(groundpin, OUTPUT);
  digitalWrite(groundpin, LOW);
  pinMode(powerButton, INPUT);

  #if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)  || defined(ARDUINO_attiny3217)
      delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
  #endif
      IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
      IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); // Start the receiver, enable feedback LED, take LED feedback pin from the internal boards definition
  #if !defined(SEND_PWM_BY_TIMER) && !defined(USE_NO_SEND_PWM) && !defined(ESP32) // for esp32 we use PWM generation by ledcWrite() for each pin
      IrSender.enableIROut(38);
      IrReceiver.enableIRIn();
  #endif
}

// Codes optoma projection screen IR
// Special Thanks to Alex/Killergeek
const uint8_t rawlen = 64;
unsigned int bufferUP[rawlen] = {1250,350,1300,350,1300,350,1300,400,400,1200,450,1200,450,1200,450,1200,450,1200,450,1200,450,1200,1250,400,450,1200,400,1250,400,1250,1250,400,1250,400,1250,400,400,1250,1250,350,1300,400,1250,350,450,1200,1250,400,1250,400,1250,400,450,1200,1250,400,1250,400,1250,400,1250,400,450};
unsigned int bufferSTOP[rawlen] = {1300,400,1250,400,1250,400,1250,400,400,1200,450,1200,450,1200,450,1200,450,1200,450,1200,1250,400,450,1200,450,1200,450,1200,450,1200,1250,400,1250,400,1250,400,400,1250,400,1250,1250,400,1250,350,450,1200,450,1200,1300,350,1300,350,450,1200,1250,400,1250,400,1250,400,450,1200,1250};
unsigned int bufferDOWN[rawlen] = {1250,400,1250,400,1250,400,1250,400,450,1200,450,1200,450,1200,450,1200,400,1250,1250,400,400,1250,400,1250,400,1250,400,1200,450,1200,1250,400,1250,400,1250,400,1250,400,450,1200,1250,400,1250,400,1250,400,450,1200,450,1200,450,1200,450,1200,1250,400,1250,400,1250,400,1250,400,1250};
unsigned int bufferUPSMALL[rawlen] = {1250,450,1200,450,1200,450,1200,450,400,1200,450,1200,450,1200,400,1250,1250,400,400,1250,400,1250,400,1250,400,1200,450,1200,450,1200,1250,450,1200,400,1250,450,400,1200,1250,400,450,1200,450,1200,450,1200,1250,400,450,1200,1250,400,450,1200,400,1250,400,1250,400,1250,1200,450,400};
unsigned int bufferDOWNSMALL[rawlen] = {1300,400,1250,400,1250,400,1200,450,400,1200,450,1200,450,1200,1250,450,400,1200,450,1200,450,1200,450,1200,400,1250,400,1250,400,1250,1250,400,1250,400,1250,400,1250,400,400,1200,450,1200,450,1200,450,1200,1250,450,1200,450,1200,450,400,1200,1250,400,450,1200,450,1200,1250,400,1250};

// Commands beamer remote (epson). This can be different for other epson remotes!
// TODO: use specific adress -> command for remote (NEC). This is only the command (other remotes can trigger the same action).
// Use: IrSender.sendNEC(0x5583, 0x90, 0); where 0x5583 = adress and 0x90 the command and 0 the number of repeats.
const String receiverRollUp = "5d";
const String receiverRollDown = "5f";
const String receiverRollUpStep = "5a";
const String receiverRollDownStep = "5c";
const String receiverShutdownBeamer = "90";
const String receiverStop = "5b";
const String receiverReset = "5e";

void(* resetFunc) (void) = 0;  // declare reset fuction at address 0. Will give it a warm restart.

void loop() 
{
    if(firsttime == 0){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(DELAY_STARTUP_WAIT);
      digitalWrite(LED_BUILTIN, LOW); 
    }

    if (IrReceiver.available()) 
    {
        if (IrReceiver.decode()) 
        {
          if(String(IrReceiver.decodedIRData.command, HEX) == receiverReset) 
          {
            resetFunc();
          }
          
          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollUp) 
          {
            IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollDown) 
          {
            IrSender.sendRaw(bufferDOWN, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollUpStep) 
          {
            IrSender.sendRaw(bufferUPSMALL, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollDownStep) 
          {
            IrSender.sendRaw(bufferDOWNSMALL, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverShutdownBeamer) 
          {
            IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverStop) 
          {
            IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
          }
        }
        
        IrReceiver.resume(); // resume receiver
    }
    
    // No Power (if using capacitor)
    if(digitalRead(powerButton)==LOW)
    {
      IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
      delay(DELAY_AFTER_LOOP);
    }

    // First time run
    // TODO: remove delay and use milli() for timing events.
    if(digitalRead(powerButton)==HIGH)
    {
      if(firsttime == 0)
      {
        IrSender.sendRaw(bufferDOWN, rawlen, 38 /* 38 Khz*/);
        delay(DELAY_SCREEN_DOWN);
        // wait 17 seconds and then send 3 times stop to make sure the stop commands work.
        IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
        delay(DELAY_QUICK_WAIT);
        IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
        delay(DELAY_QUICK_WAIT);
        IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
        delay(DELAY_QUICK_WAIT);
        firsttime = 1;
      }
    }
}
