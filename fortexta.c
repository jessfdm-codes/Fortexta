#include "lcd.h"
#include <string.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/io.h>


void addChar(const char a);
void updateScreen();
void openFortexta(char *editStr, uint16_t len);
void caretLeft();
void caretRight();
void writeChar(char a);
void init_fortexta();
void inputCursorRight();
void inputCursorLeft();
void inputCursorUp();
void inputCursorDown();

volatile int8_t delta;

char *text;
uint16_t caret;
uint16_t length;
uint8_t inputBarX, inputBarY;
char inputBar[3][26] = {
  {'a','b','c','d','e','f','g','h','i','j','k','l','m','n','o','p','q','r','s','t','u','v','w','x','y','z'},
  {'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O','P','Q','R','S','T','U','V','W','X','Y','Z'},
  {'0','1','2','3','4','5','6','7','8','9',' ','!',',','.','$','%','^','&','*','(',')','-','+','=','_','?'}
};

uint8_t fortexta_editing;

int main() {

  /* Initialize screen */
  init_lcd();

  char* str = "hello world how are you today";

  openFortexta(str, 29);

  clear_screen();

  display_color(WHITE_SMOKE, BLACK);
  display_string(str);

  return 0;
}

void openFortexta(char *editStr, uint16_t len){

    fortexta_editing = 1;

    init_fortexta();
    text = editStr;
    display_string(text);
    length = len;

    for(;fortexta_editing != 0;){
      updateScreen();
    }

    editStr = text;

}

void init_fortexta(){
  clear_screen();
  caret = 0;
  inputBarX = 0;
  inputBarY = 2;
  sei();

    /* 8MHz clock, no prescaling (DS, p. 48) */
    CLKPR = (1 << CLKPCE);
    CLKPR = 0;

    /* Configure I/O Ports */
    //Directional Buttons
    DDRC &= ~_BV(PE2) & ~_BV(PE3) & ~_BV(PE4) & ~_BV(PE5);
    PORTC |= _BV(PE2) | _BV(PE3) | _BV(PE4) | _BV(PE5);


    //Rotary encoder
    DDRE &= ~_BV(PE4) & ~_BV(PE5);  /* Rot. Encoder inputs */
    PORTE |= _BV(PE4) | _BV(PE5);   /* Rot. Encoder pull-ups */

    //Center Button
    DDRE &= ~_BV(PE7);
    PORTE |= _BV(PE7);


    /* Timer 0 for switch scan interrupt: */

    TCCR0A = _BV(WGM01);  /* CTC Mode, DS Table 14-7 */
    TCCR0B = _BV(CS01)
           | _BV(CS00);   /* Prescaler: F_CPU / 64, DS Table 14-8 */

    /* 1 ms for manual movement of rotary encoder: */
    /* 1 ms --> 1000 Hz, Formula for CTC mode from  DS 14.6.2  */
    OCR0A = (uint8_t)(F_CPU / (2 * 64.0 * 500) - 1);

    TIMSK0 |= _BV(OCIE0A);  /* Enable timer interrupt, DS 14.8.6  */

}

ISR( TIMER0_COMPA_vect ) {
     static int8_t last;
     int8_t new, diff;
     uint8_t wheel;

     /*
        Scan rotary encoder
        ===================
        This is adapted from Peter Dannegger's code available at:
        http://www.mikrocontroller.net/articles/Drehgeber
     */


     wheel = PINE;
     new = 0;
     if( wheel  & _BV(PE4) ) new = 3;
     if( wheel  & _BV(PE5) )
     new ^= 1;                  /* convert gray to binary */
     diff = last - new;         /* difference last - new  */
     if( diff & 1 ){            /* bit 0 = value (1) */
        if (diff < 0){
          caretLeft();
        } else if (diff > 0){
          caretRight();
        }
        last = new;            /* store new as next last  */
     }

     //LEFT BUTTON
     static uint8_t leftNeedsRelease = 0;

     if (!(PINC & _BV(PE5))){
      if (leftNeedsRelease == 0){
        inputCursorLeft();
        leftNeedsRelease = 1;
      }
    } else {
      leftNeedsRelease = 0;
    }

    //RIGHT BUTTON
    static uint8_t rightNeedsRelease = 0;

     if (!(PINC & _BV(PE3))){
      if (rightNeedsRelease == 0){
        inputCursorRight();
        rightNeedsRelease = 1;
      }
    } else {
      rightNeedsRelease = 0;
    }

    //UP BUTTON
    static uint8_t upNeedsRelease = 0;

    if (!(PINC & _BV(PE2))){
      if (upNeedsRelease == 0){
        inputCursorUp();
        upNeedsRelease = 1;
      }
    } else {
      upNeedsRelease = 0;
    }

    //DOWN BUTTON
    static uint8_t downNeedsRelease = 0;

    if (!(PINC & _BV(PE4))){
      if (downNeedsRelease == 0){
        inputCursorDown();
        downNeedsRelease = 1;
      }
    } else {
      downNeedsRelease = 0;
    }

    //Center button
    if (!((PINE) & _BV(PE7))){
      if (inputBarX == sizeof(inputBar[0])){ //If on special button
        //DoNothing for now
        if (inputBarY == 0){
          writeChar(*" ");
        } else if (inputBarY == 2) {
          fortexta_editing = 0;
        }
      } else {
        writeChar(inputBar[inputBarY][inputBarX]);
      }
    }

}

void caretRight(){
  if (caret < length - 1){
    caret++;
  }
}

void caretLeft(){
  if (caret > 0){
    caret--;
  }
}

void inputCursorRight(){

  //Moving to side "Backspace"
  if ((inputBarX == sizeof(inputBar[0]) - 1) && (inputBarY == 0 || inputBarY == 1)){
    inputBarX++;
    inputBarY = 0;
  }

  //Moving to "Save"
  if ((inputBarX == sizeof(inputBar[0]) - 1) && inputBarY == 2){
    inputBarX++;
  }

  //General Case
  if ((inputBarX < sizeof(inputBar[0]) - 1) /*&& (inputBar[inputBarY][inputBarX + 1]) != '\0'*/){
    inputBarX++;
  }
}

void inputCursorLeft(){
  if (inputBarX > 0){
    inputBarX--;
  }
}

void inputCursorUp(){
  //if "save" is selected
  if (inputBarX == sizeof(inputBar[0]) && inputBarY == 2){
    inputBarY = 0; //Move to "backspace"
  }

  //General case
  if (inputBarY > 0 && inputBar[inputBarY - 1][inputBarX] != '\0'){
    inputBarY--;
  }
}

void inputCursorDown(){
  //if "backspace" selected
  if (inputBarX == sizeof(inputBar[0]) && inputBarY == 0){
    inputBarY = 2; //Move to "save"
  }

  //General Case
  if (inputBarY < 2 && inputBar[inputBarY + 1][inputBarX] != '\0'){
    inputBarY++;
  }
}


void writeChar(char a){
  text[caret] = a;
}

void updateScreen(){
  display.x = 0;
  display.y = 0;
  uint16_t i = 0;
  for (i = 0; i < length; i++){
    if (i == caret){
      display_color(BLACK, WHITE_SMOKE);
      display_char(text[i]);
      display_color(WHITE_SMOKE, BLACK);
    } else {
      display_color(WHITE_SMOKE, BLACK);
      display_char(text[i]);
      display_color(BLACK, WHITE_SMOKE);
    }
  }



  display.x = 0;
  display.y = display.height-24;
  //Draw lower bar

  uint8_t x = 0;
  uint8_t y = 0;
  uint8_t maxX = 0;

  for (y = 0; y < 3; y++){
    for (x = 0; x < 26; x++){
      if (x == inputBarX && y == inputBarY){
        display_color(WHITE_SMOKE, BLACK);
        display_char(inputBar[y][x]);
        display_color(BLACK, WHITE_SMOKE);
      } else {
        display_color(BLACK, WHITE_SMOKE);
        display_char(inputBar[y][x]);
        display_color(WHITE_SMOKE, BLACK);
      }
    }
    maxX = display.x;
    display.x = 0;
    display.y += 8;
  }


  display.x = maxX + 8;
  display.y = display.height - 24;

  //If "backspace" is selected
  if (inputBarX == sizeof(inputBar[0]) && inputBarY == 0){
    display_color(WHITE_SMOKE, BLACK);
    display_string("BACKSPACE");
    display_color(BLACK, WHITE_SMOKE);
  }
  else {
    display_color(BLACK, WHITE_SMOKE);
    display_string("BACKSPACE");
    display_color(WHITE_SMOKE, BLACK);
  }

  display.x = maxX + 8;
  display.y += 16;

  //if "save" is selected
  if (inputBarX == sizeof(inputBar[0]) && inputBarY == 2){
    display_color(WHITE_SMOKE, BLACK);
    display_string("SAVE");
    display_color(BLACK, WHITE_SMOKE);
  } else {
    display_color(BLACK, WHITE_SMOKE);
    display_string("SAVE");
    display_color(WHITE_SMOKE, BLACK);
  }


}
