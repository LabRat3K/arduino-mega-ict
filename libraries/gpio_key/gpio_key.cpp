//
// Copyright (c) 2021, Andrew Williams (LabRat)
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice,
//    this list of conditions and the following disclaimer.
//
// 2. Redistributions in binary form must reproduce the above copyright notice,
//    this list of conditions and the following disclaimer in the documentation
//    and/or other materials provided with the distribution.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
// OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
// MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
// HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR
// TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
// EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// What does it do? Provides a simple 5 input (gpio) key pad interface
// that will inject events in a similar manner to the DFR_Key keypad. 
// 

#include "Arduino.h"
#include "gpio_key.h"

//
// Simple GPIO key/poll driver
//

// MAP GPIO pins to KEY function
static const int keyMapv[5]={DOWN_KEY,RIGHT_KEY,UP_KEY, SELECT_KEY, LEFT_KEY};

gpio_key::gpio_key(uint8_t down, uint8_t right, uint8_t up, uint8_t select, uint8_t left ){
   keyMap[0] =   down;
   keyMap[1] =   right;
   keyMap[2] =   up;
   keyMap[3] =   select;
   keyMap[4] =   left;
   // Default the GPIO for KEYS to INPUT
   for (int i=0;i<sizeof(keyMap);i++) {
      pinMode(keyMap[i],INPUT_PULLUP);
      lastKey[i] = digitalRead(keyMap[i]);
   }
}

// If no pins are assigned, use the following defaults
gpio_key::gpio_key(){
    const unsigned char keyMap[5] ={A4,A3,A2,A1,A0};
}

//
// Scan the key list (in order) and return the first one that has changed state.
// Does not allow for multiple key at once (much like the DFR_Key keypad).
//
int gpio_key::getKey() {
  static const int keyMapv[5] ={DOWN_KEY,RIGHT_KEY,UP_KEY, SELECT_KEY, LEFT_KEY};
  int i;

  for (i=0;i<5;i++) {
      int tempK =digitalRead(keyMap[i]);
      if (lastKey[i] != tempK) {
          lastKey[i]=tempK;
          if (tempK==0) {
             return(keyMapv[i]);
          }
      }
  }
  return NO_KEY;
}

