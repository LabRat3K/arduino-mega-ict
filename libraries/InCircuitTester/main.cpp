//
// Copyright (c) 2015, Paul R. Swan
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
#include "main.h"
#include "Arduino.h"
#include <MemoryFree.h>
#include <LiquidCrystal.h>
#include "keypad.h"
#include <CGameCallback.h>

//
// BUILD configuration options
//

//
// Uncomment to add KEYPAD_TEST to the config menu options
//
#define KEYPAD_TEST


//
// EYE_CANDY: Uncomment to add "eye candy animation" to the splash screen
//
#define EYE_CANDY

//
//  See "keypad.h" to select between DFRobot Keypad, or 'simple GPIO input'
//     See below for GPIO PIN definitions for inputs,
//     as well as for CONTRAST and RW  (could be connected direct to GND)
//

#ifdef USE_DFR_KEY
   //
   // Basic LCD diplay object (in this case, Sain 16 x 2).
   //
   const int  rs=8, en=9, d4=4, d5=5, d6=6, d7=7;

   // Function prototype for retrieving keypress token
   #define readKey() keypad.getKey()
#else
   //
   // Custom LCD configuration - update these pin definitions
   //
   const int rs = 12, en = 10, d4 = 9, d5 = 8, d6 = 7, d7 = 6;
   #define LCD_PIN_CONTRAST (5)
   #define LCD_PIN_RW       (11)

   // Function prototype for retrieving keypress token
   int readKey();
#endif
static LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

#ifdef USE_DFR_KEY
   //
   // Sain supplied keypad driver.
   //
   static DFR_Key keypad;
#else
   //
   // Simple GPIO key/poll driver
   //
   // List of 5 GPIO pins for data input
   unsigned char keyMap[5] ={A4,A3,A2,A1,A0};

   // previous key state (defaulted during setup)
   unsigned char lastKey[5];

   // MAP GPIO pins to KEY function
   int keyMapv[5]={DOWN_KEY,RIGHT_KEY,UP_KEY, SELECT_KEY, LEFT_KEY};
#endif

//
// The single on board LED is used to flash a heartbeat of sorts
// to be able to tell if the system in not running in the main loop.
//
static const UINT8 led = 13;

//
// This is the current selector.
//
static const SELECTOR *s_currentSelector;

//
// This is the game & configTable selector.
//
static SELECTOR *s_gameSelector;
static SELECTOR *s_configSelector;

//
// This is the current selection.
//
static int s_currentSelection;

//
// When true causes the soak test to run as soon as a game is selected.
//
bool s_runSoakTest;

//
// When set (none-zero) causes the select to repeat the selection callback
// for the set number of seconds.
//
int s_repeatSelectTimeInS;

//
// When true causes the repeat to ignore any reported error and continue the repeat
//
bool s_repeatIgnoreError;

//
// Function pre-declared for keypad_test - useful for first time setup/debugging of Arduino ICT tester
//
#ifdef KEYPAD_TEST
   PERROR keypad_test ( void *context, int  key);
#endif

//
// Forward Declarations for CONFIG handlers
//
PERROR config_SOAK( void *context, int  key);
PERROR config_REPEAT( void *context, int  key);
PERROR config_ERROR( void *context, int  key);
PERROR game_list( void *context, int key);

//
// The selector used for the general tester configuration options.
//
static const SELECTOR configSelector[] PROGMEM = {//0123456789abcde
                                                    {"\xA5Soak Test    ",  config_SOAK,   NULL, true},
                                                    {"\xA5Set Repeat   ",  config_REPEAT, NULL, true},
                                                    {"\xA5Set Error    ",  config_ERROR,  NULL, true},
#ifdef KEYPAD_TEST
                                                    {"\xA5Keypad Test  ",  keypad_test,   NULL, false},
#endif
                                                    {"\xA5Game Select  ",  game_list,     NULL, false},

                                                    { 0, 0 }
                                                   };

//
// Handler for the Keypad Self-test
//
#ifdef KEYPAD_TEST
PERROR keypad_test ( void *context, int  key) {
    PERROR error=errorCustom;

    int inch = NO_KEY;
    lcd.setCursor(0,1);
    lcd.print("PRESSED KEY: \x5B \x5D");
    while (inch != SELECT_KEY) {
       inch = readKey();
       lcd.setCursor(14,1);
       switch (inch) {
          case NO_KEY:     lcd.write(" ");   break;
          case LEFT_KEY:   lcd.write("\x7F");break;
          case RIGHT_KEY:  lcd.write("\x7E");break;
          case UP_KEY:     lcd.write("^");   break;
          case DOWN_KEY:   lcd.write("v");   break;
          case SELECT_KEY: lcd.write("$");   break;
       }

       // Pause if there is something to see
       if (inch != NO_KEY) {
         delay(500);
       }
    }
    errorCustom->code = ERROR_SUCCESS;
    return error;
}
#endif

//
// Handler for ERROR configuration setting.
//
PERROR config_ERROR( void *context, int  key)
{
    PERROR error = errorCustom;

    switch(key) {
       case NO_KEY: break;
       case RIGHT_KEY:
       case LEFT_KEY:
       case SELECT_KEY:
       case SAMPLE_WAIT:
            break;

       case UP_KEY:
       case DOWN_KEY:
            s_repeatIgnoreError = !s_repeatIgnoreError;
            break;
    }

    lcd.setCursor(2,1);
    if (s_repeatIgnoreError) {
       lcd.print("On Err: STOP  ");
    } else {
       lcd.print("On Err: IGNORE");
    }

    errorCustom->code = ERROR_SUCCESS;

    return error;
}

//
// Handler for SOAK configuration setting.
//
PERROR config_SOAK( void *context, int  key)
{
    PERROR error = errorCustom;

    switch(key) {
       case NO_KEY: break;
       case RIGHT_KEY:
       case LEFT_KEY:
       case SELECT_KEY:
       case SAMPLE_WAIT:
            break;

       case UP_KEY:
       case DOWN_KEY:
            s_runSoakTest = !s_runSoakTest;
            break;
    }

    lcd.setCursor(2,1);
    if (s_runSoakTest) {
       lcd.print("Soak: AUTO  ");
    } else {
       lcd.print("Soak: MANUAL");
    }

    errorCustom->code = ERROR_SUCCESS;

    return error;
}


//
// Handler for the REPEAT configuration setting.
//
PERROR config_REPEAT( void *context, int  key)
{
    PERROR error = errorCustom;
    char tempBuf[16];

    switch(key) {
       case NO_KEY: break;
       case RIGHT_KEY:
       case LEFT_KEY:
       case SELECT_KEY:
       case SAMPLE_WAIT:
            break;

       case UP_KEY:
            if (s_repeatSelectTimeInS<20)
               s_repeatSelectTimeInS = s_repeatSelectTimeInS+5;
            break;

       case DOWN_KEY:
            if (s_repeatSelectTimeInS>=5)
               s_repeatSelectTimeInS = s_repeatSelectTimeInS-5;
            break;
    }

    lcd.setCursor(2,1);
    sprintf(tempBuf,"Repeat %2.1d s",s_repeatSelectTimeInS);
    lcd.print(tempBuf);

    errorCustom->code = ERROR_SUCCESS;

    return error;
}

PERROR game_list( void *context, int  key)
{
    PERROR error = errorCustom;
    boolean done = false;
    unsigned short tempVal = 0;
    int inch;

        while(done != true) {
	   lcd.setCursor(0,1);
           lcd.print(s_gameSelector[tempVal].description);
           inch = readKey();
           switch(inch) {
              case NO_KEY: break;
              case RIGHT_KEY:
              case LEFT_KEY:
                  done = true;
                  errorCustom->code = ERROR_SUCCESS;
                  break;

              case UP_KEY:
                   if (s_gameSelector[tempVal+1].function != NULL)
                     tempVal++;
                  break;

              case DOWN_KEY:
                  if (tempVal>0)
                     tempVal--;
                  break;

              case SELECT_KEY:
		  lcd.setCursor(0,0);
                  lcd.print("=== LAUNCH ? ===");
                  inch = NO_KEY;
                  done= true;
                  while (inch<=NO_KEY) inch = readKey();
                  if (inch == SELECT_KEY) {
                     s_currentSelection = tempVal; // Is this necessary?

                      // * Need to invoke the "select game" callback from here...
                       error = s_gameSelector[tempVal].function(
                                  s_gameSelector[tempVal].context,
                                  inch );
                  } else {
                     errorCustom->code = ERROR_SUCCESS;
                  }
                  break;
           }
        }


    return error;
}


//
// Handler for the game select callback that will switch the current
// game to the one supplied.
//
PERROR
onSelectGameCallback(
    void *context,
    int  key,
    const SELECTOR *selector
)
{
    PERROR error = errorSuccess;
    GameConstructor gameConstructor = (GameConstructor) context;

    // Assign the new selector for the game
    s_currentSelector  = selector;
    s_currentSelection = 0;

    // Free the game selector memory before we construct the game.
    if (s_gameSelector != NULL)
    {
        free(s_gameSelector);
        s_gameSelector = NULL;
    }
    if (s_configSelector != NULL)
    {
        free(s_configSelector);
        s_configSelector = NULL;
    }

    if (CGameCallback::game != NULL)
    {
        delete CGameCallback::game;
        CGameCallback::game = (IGame *) NULL;
    }

    // Construct the game object
    CGameCallback::game = (IGame *) gameConstructor();

    // After game construction check the free memory
    {
        String description = " ";

        description += freeMemory();
        description += "b free";

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(description);

        delay(1000);
    }

    return error;
}

//
// Handler for the game select callback that will switch the current
// selector to the one supplied.
//
PERROR
onSelectGame(
    void *context,
    int  key
)
{
    PERROR error = errorNotImplemented;

    error = onSelectGameCallback(context,
                                 key,
                                 CGameCallback::selectorGame);

    if (SUCCESS(error) && s_runSoakTest)
    {
        error = onSelectSoakTest(context,
                                 key);
    }

    return error;
}

//
// Handler for the generic select callback that will switch the current
// selector to the one supplied.
//
PERROR
onSelectGeneric(
    void *context,
    int  key
)
{
    return onSelectGameCallback(context,
                                key,
                                CGameCallback::selectorGeneric);
}

//
// Handler for the soak test select callback that will run the soak test
// for the current game forever (if no error occurs).
//
PERROR
onSelectSoakTest(
    void *context,
    int  key
)
{
    PERROR error = errorNotImplemented;
    const SELECTOR *selector = CGameCallback::selectorSoakTest;
    int numSelections = 0;
    int selection = 0;
    int loop = 1;

    //
    // Count up how many selections were provided.
    //
    while(selector[numSelections].function != NULL)
    {
        numSelections++;
    }

    //
    // Loop to execute selections in random order forever.
    //
    do
    {
        String status = "* ";

        //
        // The random function seems to be averse to selecting 0
        // so this adjusts the range to skew a bit more towards 0
        //
        selection = random(numSelections + 1);
        if (selection != 0)
        {
            selection--;
        }

        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print(selector[selection].description);

        lcd.setCursor(0, 1);
        status += loop;
        lcd.print(status);

        error = selector[selection].function(
                   selector[selection].context,
                   SELECT_KEY );

        //
        // Some games may not implement all the selections so account for
        // not implemented errors as benign.
        //
        if (error == errorNotImplemented)
        {
            error = errorSuccess;
        }

        //
        // The reset of the seed based on the loop count is done because the
        // tests reset the random seed so on exit the result of "random" could
        // select the same test again and cause the soak test to get stuck
        // running one selection.
        //
        randomSeed(loop++);
    }
    while (SUCCESS(error));

    //
    // If we get an error, leave the selector set and parked at the failing
    // test.
    //

    s_currentSelector  = selector;
    s_currentSelection = selection;

    return error;
}

#ifndef USE_DFR_KEY
// A very simple poll for GPIO's looking for which one is depressed.
// NOTE: Does NOT support multiple keys at one time (similar to original keypad)
// Works GREAT with a joystick.
int readKey() {
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
#endif

#ifdef EYE_CANDY
// Define custom characters for the LCD
void splash_page(void) {
   #define BANNER_LEFT_CHAR byte(0)
   #define BANNER_RIGHT_CHAR byte(1)
   //Eye Candy Fun - add animated Invaders to Splash Screen
   {
     byte Inv1L[8] = {
        0b00001,
        0b00011,
        0b00111,
        0b01101,
        0b01111,
        0b00010,
        0b00101,
        0b01010,
     };
     byte Inv1R[8] = {
        0b10000,
        0b11000,
        0b11100,
        0b10110,
        0b11110,
        0b01000,
        0b10100,
        0b01010,
     };
     byte Inv2L[8] = {
        0b00001,
        0b00011,
        0b00111,
        0b01101,
        0b01111,
        0b00101,
        0b01000,
        0b00100,
     };
     byte Inv2R[8] = {
        0b10000,
        0b11000,
        0b11100,
        0b10110,
        0b11110,
        0b10100,
        0b00010,
        0b00100,
     };

     lcd.createChar(0,Inv1L);
     lcd.createChar(1,Inv1R);
     lcd.createChar(2,Inv2L);
     lcd.createChar(3,Inv2R);
   }

   lcd.setCursor(0, 0);
   lcd.write(BANNER_LEFT_CHAR);
   lcd.write(BANNER_RIGHT_CHAR);
   lcd.print("Arduino  ICT");
   lcd.write(BANNER_LEFT_CHAR);
   lcd.write(BANNER_RIGHT_CHAR);
}

void splash_wait(void) {
   int i =0;
      // EYE CANDY OPENING
      lcd.setCursor(0,1);
      lcd.print(" <PRESS SELECT>");
      i=0;
      while (readKey() != SELECT_KEY) {
         i++;
         lcd.setCursor(0,0);
         lcd.write(byte((i%2)*2));
         lcd.write(byte(((i%2)*2)+1) );
         lcd.setCursor(14,0);
         lcd.write(byte((i%2)*2));
         lcd.write(byte(((i%2)*2)+1));
         delay(500);
      }
}
#else
   //
   //  No flashy animation (save code space)
   //
   #define BANNER_LEFT_CHAR byte(0xDF)
   #define BANNER_RIGHT_CHAR byte(0xDF)

   void splash_page(void) {
      lcd.setCursor(0,0);
      lcd.print("In Circuit Test");
   }

   void splash_wait(void) {
      delay(2000);
   }
#endif

void mainSetup(
    const SELECTOR *gameSelector
)
{
#ifdef LCD_PIN_RW
    // Setup the RW pins (must occur before lcd.begin
    pinMode(LCD_PIN_RW,OUTPUT);
    digitalWrite(LCD_PIN_RW,0);
#endif
    lcd.begin(16, 2);
    lcd.clear();
#ifdef LCD_PIN_CONTRAST
    // Setup the CONTRAST pins - after the CLEAR
    pinMode(LCD_PIN_CONTRAST,OUTPUT);
    analogWrite(LCD_PIN_CONTRAST,40);
#endif

    splash_page();

    pinMode(led, OUTPUT);
    digitalWrite(led, LOW);

#ifdef USE_DFR_KEY
    keypad.setRate(10);
#else
    // Default the GPIO for KEYS to INPUT
    int i = 0;
    for (i=0;i<sizeof(keyMap);i++) {
       pinMode(keyMap[i],INPUT_PULLUP);
       lastKey[i] = digitalRead(keyMap[i]);
    }
#endif

    // Copy the PROGMEM based selectors into a single local SRAM selector
    {
        UINT16 uConfigRegionSize = 0;
        UINT16 uConfigIndexCount = 0;

        UINT16 uGameRegionSize   = 0;
        UINT16 uGameIndexCount   = 0;

        for ( ; pgm_read_word_near(&configSelector[uConfigIndexCount].function) != 0 ; uConfigIndexCount++) {}

        for ( ; pgm_read_word_near(&gameSelector[uGameIndexCount].function) != 0 ; uGameIndexCount++) {}

        uConfigRegionSize += sizeof(configSelector[0]) * (uConfigIndexCount+1); // +1 to include null terminator
        uGameRegionSize   += sizeof(gameSelector[0])   * (uGameIndexCount+1);

        s_configSelector = (PSELECTOR) malloc( uConfigRegionSize );
        s_gameSelector = (PSELECTOR) malloc( uGameRegionSize );

        memcpy_P( &s_configSelector[0], configSelector, uConfigRegionSize );
        memcpy_P( &s_gameSelector[0], gameSelector, uGameRegionSize );
    }

    s_currentSelector = s_configSelector;

    splash_wait();
}


void mainLoop()
{
    int previousKey = SAMPLE_WAIT;

    do {

        int currentKey = readKey();

        //
        // Special case of the first pass through to park at the first selector.
        //
        if (previousKey == SAMPLE_WAIT)
        {
            currentKey = LEFT_KEY;
        }

        if ( (currentKey == SAMPLE_WAIT) ||
             (currentKey == previousKey) )
        {
            digitalWrite(led, LOW);
            delay(100);
            digitalWrite(led, HIGH);

            continue;
        }

        switch (currentKey)
        {
            case NO_KEY : { break; }

            case LEFT_KEY :
            {
                if (s_currentSelection > 0)
                {
                    s_currentSelection--;
                }

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(s_currentSelector[s_currentSelection].description);

                if (s_currentSelector[s_currentSelection].subMenu)
                {
                    PERROR error = s_currentSelector[s_currentSelection].function(
                        s_currentSelector[s_currentSelection].context,
                        NO_KEY );

                    lcd.setCursor(0, 1);
                    lcd.print(error->description);
                }

                break;
            }

            case RIGHT_KEY :
            {
                if (s_currentSelector[s_currentSelection+1].function != NULL)
                {
                    s_currentSelection++;
                }

                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(s_currentSelector[s_currentSelection].description);

                if (s_currentSelector[s_currentSelection].subMenu)
                {
                    PERROR error = s_currentSelector[s_currentSelection].function(
                        s_currentSelector[s_currentSelection].context,
                        NO_KEY );

                    lcd.setCursor(0, 1);
                    lcd.print(error->description);
                }

                break;
            }

            case UP_KEY     :
            case DOWN_KEY   :
            {
                if (s_currentSelector[s_currentSelection].subMenu)
                {
                    lcd.setCursor(0, 1);
                    lcd.print(BLANK_LINE_16);

                    PERROR error = s_currentSelector[s_currentSelection].function(
                                    s_currentSelector[s_currentSelection].context,
                                    currentKey );

                    lcd.setCursor(0, 1);
                    lcd.print(error->description);
                }
                break;
            }

            case SELECT_KEY :
            {
                unsigned long startTime = millis();
                unsigned long endTime = startTime + ((unsigned long) s_repeatSelectTimeInS * 1000);
                const SELECTOR *inSelector = s_currentSelector;
                PERROR error = errorSuccess;

                lcd.setCursor(0, 1);
                lcd.print(BLANK_LINE_16);

                do {

                    error = s_currentSelector[s_currentSelection].function(
                               s_currentSelector[s_currentSelection].context,
                               currentKey );
                }
                while ( (s_repeatIgnoreError || SUCCESS(error)) &&  // Ignoring or no failures
                        (millis() < endTime)                    &&  // Times not up.
                        (inSelector != s_gameSelector)          &&  // The input selector wasn't the game selector.
                        (inSelector != s_configSelector) );         // OR the config selector
                //
                // The selection may have changed so update the whole display.
                //
                lcd.clear();
                lcd.setCursor(0, 0);
                lcd.print(s_currentSelector[s_currentSelection].description);

                lcd.setCursor(0, 1);
                lcd.print(error->description);
            }

            default : { break; };
        }

        previousKey = currentKey;

    } while (1);
}

