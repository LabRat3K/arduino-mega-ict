#ifndef gpio_key_h
#define gpio_key_h

#include "Arduino.h"

#define SAMPLE_WAIT -1
#define NO_KEY 0
#define UP_KEY 3
#define DOWN_KEY 4
#define LEFT_KEY 2
#define RIGHT_KEY 5
#define SELECT_KEY 1

class gpio_key
{
  public:
    gpio_key(uint8_t down, uint8_t right, uint8_t up, uint8_t select, uint8_t left);
    gpio_key();
    int getKey();

    // This ia NOP for backwards compatability with DFR_Key class
    inline void setRate(int) { };
  private:
    unsigned char keyMap[5];   // Store the PIN map passed in
    unsigned char lastKey[5];  // Store state of the PINs to detect change
};

#endif
