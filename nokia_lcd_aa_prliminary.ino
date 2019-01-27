#include < avr / eeprom.h > 
#include "U8glib.h"
#include < AH_AD9850.h > 
#include < Rotary.h > 
#include < OneButton.h >

U8GLIB_PCD8544 u8g(13, 11, 6, 10, 5); //SPI Com: SCK = 13, MOSI = 11, CS = 10, A0 = 9
AH_AD9850 AD9850(A3, A5, 4, A4); //AH_AD9850(int CLK, int FQUP, int BitData, int RESET);
Rotary r = Rotary(2, 3);
OneButton button(A1, true);

float bandSwr[7];
float freqBand[7] = {
   3650000.0,
   7100000.0,
   10125000.0,
   14175000.0,
   18118000.0,
   21225000.0,
   28850000.0
};

const byte lcdW = 84;
const byte lcdH = 48;
const byte fontWidth = 5;
const byte fontHeigth = 8;
const byte totSpan = 9;

byte SWR[lcdW];
float minSwr;
float maxSwr;

uint32_t next_state_sweep = 0;
byte select_display = 0;
byte menu_redraw_required = 0;
byte doubleClicked = 0;
byte modSpan = 0;
byte posInput = 0;
byte sign = 0; // negative

float spanValue[totSpan] = {
   25000.0,
   50000.0,
   100000.0,
   250000.0,
   500000.0,
   1000000.0,
   2500000.0,
   5000000.0,
   10000000.0
};

struct settings_t {
   int spanIdx;
   float freqCenter;
}
settings;

//from radians to degrees
const float R2d = 57.296;
//Standard characteristic impedance
const float Z0 = 50.0;
float Angle;
float Mag;

float Ximp;
float Rimp;
float Zimpl;

float Swr;

void Docalcswr() {
   Swr = analogRead(A0);
   Serial.println((1023 - Swr) / 10.0);
}

void calcParameters(int k, float stepSpan) {

   float freq;
   float tempSwr;

   freq = settings.freqCenter + (k - lcdW / 2) * stepSpan;
   if (freq <= 0) freq = 1000000.0;
   AD9850.set_frequency(freq);
   Serial.print(freq / 1000000.0);
   Serial.print(" ");
   delay(10);
   Docalcswr();
   Swr = 48 * Swr / 1023.0;
   SWR[k] = Swr; // 8+(46 - round(23*(tempSwr - 1)));  //MAX SWR = 3.0
}
void printReverse(u8g_uint_t x, u8g_uint_t y,
   const char * s) {

   u8g_uint_t w = u8g.getStrWidth(s);
   u8g.drawBox(x, y - 7, w, 9); // draw cursor bar
   u8g.setDefaultBackgroundColor();
   u8g.drawStr(x, y, s);
   u8g.setDefaultForegroundColor();
}

void creaGrid() {

   u8g_uint_t p0 = lcdH - 2;

   for (int i = 1; i < 2; i++) {
      u8g.drawHLine(0, (i * 24) + fontHeigth, lcdW - 1);
   }

   for (int i = 0; i < 9; i++) {
      u8g.drawVLine(42 + (i - 4) * 10.5, fontHeigth, 46);
   }
   u8g.drawCircle(42, 49 + fontHeigth, 2);

   if (settings.freqCenter > 9999999) u8g.setPrintPos(30, p0);
   else u8g.setPrintPos(35, p0);
   u8g.print(settings.freqCenter / 1000000, 3);

   if (spanValue[settings.spanIdx] > 9999999) u8g.setPrintPos(60, 30);
   else u8g.setPrintPos(60, 30);
   u8g.print(spanValue[settings.spanIdx] / 1000000, 3);

   u8g.drawRFrame(0, fontHeigth, lcdW, 46, 1);

   u8g.drawStr(0, fontHeigth - 1, "SWR");

   u8g.setPrintPos(40, fontHeigth - 1);
   u8g.print(minSwr, 2);

   u8g.setPrintPos(80, fontHeigth - 1);
   u8g.print(maxSwr, 2);

   if (!modSpan) {
      printReverse(0, p0, "freq");
      u8g.drawStr(60, p0, "span");
   } else {
      u8g.drawStr(0, p0, "freq");
      printReverse(70, p0, "span");
   }

}

void printSwr() {
   for (int k = 0; k < lcdW - 1; k++) {
      u8g.drawLine(k, SWR[k], k + 1, SWR[k + 1]);
      //  u8g.drawCircle(k,Z[k], 1, U8G_DRAW_ALL) ;
   }
}
void swrFrame(u8g_uint_t pos, float swrVal) {
   u8g.drawBox(50, pos, 77 * ((min(swrVal, 2.5) - 1.0) / 1.5), 6);
   u8g.drawFrame(50, pos, 77, 6);
   u8g.drawVLine(76, pos, 5);
   u8g.drawVLine(102, pos, 5);
}

void creaBand() {
   u8g.setPrintPos(76, 7);
   u8g.print(1.5, 1);

   u8g.setPrintPos(102, 7);
   u8g.print(2.0, 1);

   for (int i = 0; i < 7; i++) {
      u8g_uint_t pos = (8 * i) + 10;
      u8g.setPrintPos(0, pos + 5);
      u8g.print(freqBand[i] / 1000000, 3);
      swrFrame(pos, bandSwr[i]);
   }
}

uint8_t update_graph_band(void) {
   if (next_state_sweep < 7) {
      AD9850.set_frequency(freqBand[next_state_sweep]);
      delay(10);
      Docalcswr();
      bandSwr[next_state_sweep] = Swr;
      next_state_sweep++;
      return 0;
   } else {
      next_state_sweep = 0;
      return 1;
   }
}

uint8_t update_graph(void) {
   if (next_state_sweep < lcdW) {
      if (next_state_sweep == 0) {
         minSwr = 10.0;
         maxSwr = 1.0;
      }
      float s = spanValue[settings.spanIdx] / lcdW;
      calcParameters(next_state_sweep, s);
      next_state_sweep++;
      return 0;
   } else {
      next_state_sweep = 0;
      return 1;
   }
}

// update graphics, will return none-zero if an update is required
uint8_t update_graphics(void) {

   switch (select_display) {
   case 1:
      return update_graph_band();
      break;
   default:
      return update_graph();
   }
   // no update for the graphics required
   return 0;
}

void showCursor(u8g_uint_t x0, u8g_uint_t y0) {
   u8g.drawBox(x0 + fontWidth * (7 - posInput), y0, 6, 3);
}

float updateFreq(unsigned char r, float v) {
   float step = round(pow(10, posInput));
   if (r == DIR_NONE) {
      // do nothing
   } else if (r == DIR_CW) {
      v = v + step;
   } else if ((r == DIR_CCW) && (v >= step + 1000000.0)) {
      v = v - step;
   }
   return v;
}
void updateSpan(unsigned char r) {
   if (r == DIR_NONE) {
      // do nothing
   } else if (r == DIR_CW) {
      settings.spanIdx++;
   } else if (r == DIR_CCW) {
      settings.spanIdx--;
      if (settings.spanIdx < 0) settings.spanIdx = totSpan - 1;
   }
   settings.spanIdx = settings.spanIdx % totSpan;
}
//================================================================
// overall draw procedure for u8glib

void draw(void) {
   u8g.setDefaultForegroundColor();
   switch (select_display) {
   case 1:
      creaBand();
      break;
   default:
      {
         creaGrid();
         printSwr();
         if (doubleClicked == 1) {
            switch (modSpan) {
            case 0:
               {
                  if (posInput < 3) posInput = 3;showCursor(35, lcdH - 1);
                  break;
               }
            case 1:
               {
                  posInput = 6;
                  showCursor(91, lcdH - 1);
                  break;
               }
            default:
               break;
            }

         }
      }
   }

}

//================================================================
// Arduino setup and loop

void setup(void) {
   analogReference(EXTERNAL);
   // initialize serial communication at 9600 bits per second:
   Serial.begin(9600);
   //analogReference(EXTERNAL);
   PCICR |= (1 << PCIE2);
   PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);

   sei();

   // link the doubleclick function to be called on a doubleclick event.   
   button.attachDoubleClick(doubleclick);
   button.attachClick(singleclick);
   button.attachPress(longclick);

   u8g.setFont(u8g_font_5x7);

   eeprom_read_block((void * ) & settings, (void * ) 0, sizeof(settings));
   settings.spanIdx = settings.spanIdx % totSpan;
   if (settings.freqCenter <= 0) {
      settings.freqCenter = 7000000.0;
   }
   settings.spanIdx = 0;
   settings.freqCenter = 7000000.0;
   AD9850.reset(); //reset module
   delay(1000);
   AD9850.powerDown(); //set signal output to LOW

   AD9850.set_frequency(0, 0, settings.freqCenter);
}

void loop() {
   button.tick();
   if (update_graphics() != 0 | menu_redraw_required != 0) {
      u8g.firstPage();
      do {
         draw();
      } while (u8g.nextPage());
      menu_redraw_required = 0; // menu updated, reset redraw flag
   }
}

ISR(PCINT2_vect) {
   unsigned char result = r.process();
   if (doubleClicked) {
      if ((select_display == 0) && modSpan) {
         updateSpan(result);
      } else
         settings.freqCenter = updateFreq(result, settings.freqCenter);
   } else if (select_display == 0) modSpan = !modSpan;

   menu_redraw_required = 1;
}

void singleclick() { //change position in field
   if (doubleClicked) {
      posInput++;
      posInput = posInput % 7;
   }
   menu_redraw_required = 1;
}

void doubleclick() { //change field
   doubleClicked = !doubleClicked;
   if (!doubleClicked) eeprom_write_block((const void * ) & settings, (void * ) 0, sizeof(settings));;
   menu_redraw_required = 1;
}
void longclick() { //rotate displays
   select_display = select_display++;
   select_display = select_display % 3;
   posInput = 0;
   doubleClicked = 0;
   eeprom_write_block((const void * ) & settings, (void * ) 0, sizeof(settings));
   menu_redraw_required = 1;
}
