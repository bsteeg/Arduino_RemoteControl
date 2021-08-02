/*
 * SendDemo.cpp
 *
 * Demonstrates sending IR codes in standard format with address and command
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

//#define EXCLUDE_EXOTIC_PROTOCOLS // saves around 240 bytes program space if IrSender.write is used
//#define SEND_PWM_BY_TIMER
//#define USE_NO_SEND_PWM

#include <IRremote.h>

#define DELAY_AFTER_SEND 2000
#define DELAY_AFTER_LOOP 5000
#define DELAY_SCREEN_DOWN 16500
#define DELAY_QUICK_WAIT 500
#define DELAY_STARTUP_WAIT 2000
#define RECV_PIN 7

const int powerButton = 6; // power input gone detect.
const int groundpin = 10;

decode_results receiverIRresults; //Infra red result

uint8_t firsttime = 0;

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(groundpin, OUTPUT);
    digitalWrite(groundpin, LOW);
    pinMode(powerButton, INPUT);
    //Serial.begin(115200);
    //Serial.println(F("Starting!"));
    //Serial.print(IR_RECEIVE_PIN);
#if defined(__AVR_ATmega32U4__) || defined(SERIAL_USB) || defined(SERIAL_PORT_USBVIRTUAL)  || defined(ARDUINO_attiny3217)
    delay(4000); // To be able to connect Serial monitor after reset or power up and before first print out. Do not wait for an attached Serial Monitor!
#endif
    // Just to know which program is running on my Arduino
    //Serial.println(F("START " __FILE__ " from " __DATE__ "\r\nUsing library version " VERSION_IRREMOTE));
    
    IrSender.begin(IR_SEND_PIN, ENABLE_LED_FEEDBACK); // Specify send pin and enable feedback LED at default feedback LED pin
    
    IrReceiver.begin(RECV_PIN, ENABLE_LED_FEEDBACK); // Start the receiver, enable feedback LED, take LED feedback pin from the internal boards definition
    
    //Serial.print(F("Ready to send IR signals at pin "));
#if defined(ARDUINO_ARCH_STM32) || defined(ESP8266)
    //Serial.println(IR_SEND_PIN_STRING);
#else
    //Serial.println(IR_SEND_PIN);
#endif

#if !defined(SEND_PWM_BY_TIMER) && !defined(USE_NO_SEND_PWM) && !defined(ESP32) // for esp32 we use PWM generation by ledcWrite() for each pin
    /*
     * Print internal signal generation info
     */
    IrSender.enableIROut(38);
    IrReceiver.enableIRIn();
    
    //Serial.print(F("Send signal mark duration is "));
    //Serial.print(IrSender.periodOnTimeMicros);
    //Serial.print(F(" us, pulse correction is "));
    //Serial.print(IrSender.getPulseCorrectionNanos());
    //Serial.print(F(" ns, total period is "));
    //Serial.print(IrSender.periodTimeMicros);
    //Serial.println(F(" us"));
#endif
}

const uint8_t rawlen = 64;
unsigned int bufferUP[rawlen] = {1250,350,1300,350,1300,350,1300,400,400,1200,450,1200,450,1200,450,1200,450,1200,450,1200,450,1200,1250,400,450,1200,400,1250,400,1250,1250,400,1250,400,1250,400,400,1250,1250,350,1300,400,1250,350,450,1200,1250,400,1250,400,1250,400,450,1200,1250,400,1250,400,1250,400,1250,400,450};
unsigned int bufferSTOP[rawlen] = {1300,400,1250,400,1250,400,1250,400,400,1200,450,1200,450,1200,450,1200,450,1200,450,1200,1250,400,450,1200,450,1200,450,1200,450,1200,1250,400,1250,400,1250,400,400,1250,400,1250,1250,400,1250,350,450,1200,450,1200,1300,350,1300,350,450,1200,1250,400,1250,400,1250,400,450,1200,1250};
unsigned int bufferDOWN[rawlen] = {1250,400,1250,400,1250,400,1250,400,450,1200,450,1200,450,1200,450,1200,400,1250,1250,400,400,1250,400,1250,400,1250,400,1200,450,1200,1250,400,1250,400,1250,400,1250,400,450,1200,1250,400,1250,400,1250,400,450,1200,450,1200,450,1200,450,1200,1250,400,1250,400,1250,400,1250,400,1250};
unsigned int bufferUPSMALL[rawlen] = {1250,450,1200,450,1200,450,1200,450,400,1200,450,1200,450,1200,400,1250,1250,400,400,1250,400,1250,400,1250,400,1200,450,1200,450,1200,1250,450,1200,400,1250,450,400,1200,1250,400,450,1200,450,1200,450,1200,1250,400,450,1200,1250,400,450,1200,400,1250,400,1250,400,1250,1200,450,400};
unsigned int bufferDOWNSMALL[rawlen] = {1300,400,1250,400,1250,400,1200,450,400,1200,450,1200,450,1200,1250,450,400,1200,450,1200,450,1200,450,1200,400,1250,400,1250,400,1250,1250,400,1250,400,1250,400,1250,400,400,1200,450,1200,450,1200,450,1200,1250,450,1200,450,1200,450,400,1200,1250,400,450,1200,450,1200,1250,400,1250};

// commands beamer remote (epson). This can be different for other epson remotes!
const String receiverRollUp = "5d";
const String receiverRollDown = "5f";
const String receiverRollUpStep = "5a";
const String receiverRollDownStep = "5c";
const String receiverShutdownBeamer = "90";
const String receiverStop = "5b";
const String receiverReset = "5e";

void(* resetFunc) (void) = 0;  // declare reset fuction at address 0

void loop() {
    /*
     * Print values 
     */
    //Serial.println();
    //Serial.print("testing IR");
    //Serial.flush();
    
    if(firsttime == 0){
      digitalWrite(LED_BUILTIN, HIGH);
      delay(DELAY_STARTUP_WAIT);
      digitalWrite(LED_BUILTIN, LOW); 
    }

    if (IrReceiver.available()) 
    {
        if (IrReceiver.decode()) 
        {
          //Serial.print(F("Decoded protocol: "));
          //Serial.print(getProtocolString(IrReceiver.decodedIRData.protocol));
          //Serial.print(F("Decoded raw data: "));
          //Serial.print(IrReceiver.decodedIRData.decodedRawData, HEX);
          //Serial.print(F(", decoded address: "));
          //Serial.print(IrReceiver.decodedIRData.address, HEX);
          //Serial.print(F(", decoded command: "));
          //Serial.println(IrReceiver.decodedIRData.command, HEX);
          //Serial.println("String:" + String(IrReceiver.decodedIRData.command, HEX));

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverReset) 
          {
            resetFunc();
          }
          
          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollUp) 
          {
            //Serial.println("RollUp");
            IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollDown) 
          {
            //Serial.println("RollDown");
            IrSender.sendRaw(bufferDOWN, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollUpStep) 
          {
            //Serial.println("RollUpStep");
            IrSender.sendRaw(bufferUPSMALL, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverRollDownStep) 
          {
            //Serial.println("RollDownStep");
            IrSender.sendRaw(bufferDOWNSMALL, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverShutdownBeamer) 
          {
            //Serial.println("Shutdown");
            IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
          }

          if(String(IrReceiver.decodedIRData.command, HEX) == receiverStop) 
          {
            //Serial.println("Stop");
            IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
          }

          IrReceiver.resume();
        }
        
        IrReceiver.resume(); // resume receiver
    }
    
    //Serial.println(F("Send custom NEC 16 bit adres, 16 bit data"));
    //Serial.flush();
    //digitalWrite(LED_BUILTIN, LOW); 
    
    //digitalWrite(LED_BUILTIN, HIGH);

    //IrSender.sendRaw(bufferDOWN, rawlen, 38 /* 38 Khz*/);
    //delay(DELAY_SCREEN_DOWN);
    //IrSender.sendRaw(bufferSTOP, rawlen, 38 /* 38 Khz*/);
    //delay(DELAY_SCREEN_DOWN);
    //IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
    //delay(DELAY_AFTER_LOOP);

    //digitalWrite(LED_BUILTIN, LOW); 

    if(digitalRead(powerButton)==LOW){
      Serial.println("we have no power");
      IrSender.sendRaw(bufferUP, rawlen, 38 /* 38 Khz*/);
      delay(DELAY_AFTER_LOOP);
    }
    if(digitalRead(powerButton)==HIGH){
      Serial.println("FirstTime loop");
      Serial.print("first time bit: ");
      Serial.println(firsttime);
      if(firsttime == 0){
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
    
    //delay(DELAY_AFTER_LOOP); // additional delay at the end of each loop
}
