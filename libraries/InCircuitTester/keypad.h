#ifndef keypad_h
#define keypad_h

//
// Comment out to use GPIOs in place of DFRobot Keypad
//
#define USE_DFR_KEY

#ifdef USE_DFR_KEY
   #include <DFR_Key.h>
#else
   #define SAMPLE_WAIT -1
   #define NO_KEY 0
   #define UP_KEY 1
   #define DOWN_KEY 2
   #define LEFT_KEY 3
   #define RIGHT_KEY 4
   #define SELECT_KEY 5
#endif

#endif
