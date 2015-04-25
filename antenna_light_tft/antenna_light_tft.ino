/* A simple antenna (vswr) analyser using ili9341 tft and arduino promini
*  Details and schematics: http://blog.riyas.org/2015/04/a-simple-standalone-antenna-analyzer-with-ili9341tft.html
*  Credits various antenna analyser projects
*  Credits to k6bez antenna analyzer
*  Credits to  IW2NDH (atom1945.host-ed.me) and zl2pd.com
*  An IR remote is used and can be modified to use a rotay encoder and a few push buttons
*  Use Freely for non commercial use, No warranties!
*/

#include <stdint.h>
#include <TFTv2.h>
#include <SPI.h>
#include <avr/eeprom.h>
#include <TimedAction.h>
#include <IRremote.h>
#include <AH_AD9850.h>
#define RECV_PIN 3
#define SWR_STEP 32

/* Structure containing received data */
decode_results results;
/* Used to store the last code received. Used when a repeat code is received */
unsigned long LastCode;
/* Create an instance of the IRrecv library */
IRrecv irrecv(RECV_PIN);
TimedAction timedAction = TimedAction(100,infrared);
TimedAction timedAction2 = TimedAction(100,routine);

AH_AD9850 AD9850(9, 8, 7, 2); //AH_AD9850(int CLK, int FQUP, int BitData, int RESET);
// Pins for Fwd/Rev detectors
const int REV_ANALOG_PIN = A1;
const int FWD_ANALOG_PIN = A0;
char sz[32];
float bandSwr[7];
float freqBand[7] =
{
    3650000.0, 7100000.0,10125000.0, 14175000.0,18118000.0, 21225000.0, 28850000.0
}
;
const byte lcdW = 320 ;
const byte lcdH = 240 ;
const byte fontWidth = 10 ;
const byte fontHeigth = 30 ;
const byte totSpan = 9;
/*
float spanValue[totSpan] =
{
    25000.0, 50000.0,100000.0, 250000.0,500000.0, 1000000.0, 2500000.0, 5000000.0,10000000.0
}
;
*/
float spanValue[totSpan] =
{
    25000.0, 50000.0,100000.0, 250000.0,500000.0, 1000000.0, 2500000.0, 10000000.0,30000000.0
}
;
byte SWR[SWR_STEP];
float minSwr;
float maxSwr;
uint32_t next_state_sweep = 0;
byte select_display = 0;
byte menu_redraw_required = 0;
byte doubleClicked = 0;
byte modSpan = 0;
byte posInput = 0;
byte sign = 0; // negative
float Swr ;
struct settings_t
{
    int spanIdx;
    float freqCenter;
}
settings;
//averaged analogue read slow but solid
double analog_read_value(int pin) {
double total = 0.0;
double reading;
int i;
  for (i=0; i< 80; i++) {
    reading = analogReadX(pin);
    total += reading * reading;
  }
  return total / 80.0;
}
double analogReadX(const int pin)
{
  if (pin==A0)
  return analogRead(pin);
  else
  return analogRead(pin); //For offset corrections 

}
void Docalcswr(){
    double FWD=0;
    double REV=0;
    double VSWR;
    // Read the forawrd and reverse voltages
    REV = analog_read_value(REV_ANALOG_PIN); 
    FWD = analog_read_value(FWD_ANALOG_PIN);
    if(REV >= FWD){
    // To avoid a divide by zero or negative VSWR then set to max 999
    VSWR = 999;
    } else {
    // Calculate VSWR
    VSWR = (FWD+REV)/(FWD-REV);
    // Rx=(25+REV)/(0.5*FWD-REV-0.5);
    Swr = abs(VSWR);  
}
}
void creaGrid()
{
    byte p0 = lcdH - 2;
    for (int i = 0 ;i < 9; i++)
    {
        Tft.drawVerticalLine((i*40),fontHeigth,164,RED); //240*320
    }
    for (int i = 1 ;i < 2; i++)
    {
        Tft.drawHorizontalLine(0,i*82+fontHeigth,319,RED); //240
    }
    Tft.drawCircle(160,164+fontHeigth,5,GREEN);
    if (settings.freqCenter >9999999)
    Tft.drawFloat(settings.freqCenter/1000000,130,214,2,YELLOW);
    else
    Tft.drawFloat(settings.freqCenter/1000000,130,214,2,YELLOW);
    float s = spanValue[settings.spanIdx]/8;
    for (int i = 0 ;i < 9; i++)
    {
        double freq = (settings.freqCenter + (i-4)*s)/1000000; 
        Tft.drawFloat(freq,(i*40),185,1,YELLOW);
    }    
    if (spanValue[settings.spanIdx] >9999999)
    Tft.drawFloat(spanValue[settings.spanIdx]/1000000,260,214,2,YELLOW);
    else
    Tft.drawFloat(spanValue[settings.spanIdx]/1000000,260,214,2,YELLOW);
    Tft.drawRectangle(0,fontHeigth,320,164, GREEN);
    Tft.drawString("SWR",0,0,2,CYAN);
    Tft.drawFloat(minSwr,40,0,2,GREEN);
    Tft.drawFloat(maxSwr,120, 0,2,RED);
    if (!modSpan)
    {
        //printReverse(0, p0 ,"freq");
        Tft.fillRectangle(40,210, 60,20, CYAN);
        Tft.drawString("freq",40, 210 ,2,BLUE);
        Tft.drawString("span",210, 210 ,2,BLUE);
    }
    else
    {
        Tft.fillRectangle(210,210, 50,20, CYAN);
        Tft.drawString("freq",40, 210 ,2,BLUE);
        Tft.drawString("span",210, 210 ,2,BLUE);
        //printReverse(70, p0 ,"span");
    }
}
void printSwr()
{
    for (int k = 0;k < SWR_STEP; k++)
    {
        // ucg.setColor(0,255,255);
        //delay(10);
        //ucg.drawLine(k,SWR[k],k+1,SWR[k+1]);
        int spacer=320/SWR_STEP;
        Tft.drawLine(k*spacer,SWR[k],(k+1)*spacer,SWR[k+1],WHITE);
        // Tft.drawCircle(k,Z[k], 1, U8G_DRAW_ALL) ;
        //Serial.print(k);
        //Serial.print(",");
        //Serial.println(SWR[k]);
    }
}
void showCursor(int x0, int y0)
{
    //u8g.drawBox(x0 + fontWidth*(7-posInput),y0,6,3);
    Tft.fillRectangle(x0,y0, 50,8, BRIGHT_RED);
}

void swrFrame(int pos, float swrVal){
        Tft.fillRectangle(50,pos,250*((min(swrVal,2.5)-1.0)/1.5),20,RED); 
        Tft.drawRectangle(50, pos, 250,20,GREEN);
        Tft.drawVerticalLine(150,pos,20,GREEN); 
        Tft.drawVerticalLine(250,pos,20,GREEN); 
}

void creaBand(){
        Tft.drawFloat(1.5,150,10,2,YELLOW); 
        Tft.drawFloat(2,250,10,2,YELLOW); 
	for (int i = 0 ;i < 7; i++){
		int pos = (30 * i) + 30;
                Tft.drawFloat(freqBand[i]/1000000,0,pos+5,2,YELLOW); 
		swrFrame(pos,bandSwr[i]);
	}
}
void draw(void)
{
    Tft.fillRectangle(0, 0, 320, 240, BLACK);
    switch (select_display)
    {
        case 1:  creaBand();//Tft.fillRectangle(0, 0, 320, 240, BLUE);
        break;
        case 2:  Tft.fillRectangle(0, 0, 320, 240, BLACK);
        break;
        default:
        {
            creaGrid();
            printSwr();
            if (doubleClicked == 1)
            {
                switch (modSpan)
                {
                    case 0 :
                    {
                        if (posInput <3) posInput = 3; showCursor(130,235);
                        break;
                    }
                    case 1 :
                    {
                        posInput = 6;
                        showCursor(260,235);
                        break;
                    }
                    default: break;
                }
            }
        }
    }
}
  

void setup(void)
{
    Tft.TFTinit(); // init TFT library
    Tft.fillRectangle(0, 0, 320, 240, BLUE);
    analogReference(DEFAULT);
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    sprintf(sz, "\nLB7UG Compiled %s %s", __TIME__, __DATE__);
    Serial.println(sz);  	
    eeprom_read_block((void*)&settings, (void*)0, sizeof(settings));
    if (settings.freqCenter <= 0)
    {
        settings.freqCenter = 7000000.0;
    }
    settings.spanIdx = 8;
    //settings.freqCenter = 7000000.0;
    AD9850.reset(); //reset module
    delay(200);
    AD9850.powerDown(); //set signal output to LOW
    AD9850.set_frequency(0,0,settings.freqCenter);
    //randomSeed(analogRead(0)); // for testing purpose
    irrecv.enableIRIn();
    LastCode = 0;
    pinMode(10, OUTPUT);
    digitalWrite(10, HIGH);

}
void calcParameters(int k, float stepSpan)
{
    float freq;
    float tempSwr;
    freq = settings.freqCenter + (k-SWR_STEP/2)* stepSpan;
    if (freq <= 0) freq = 1000000.0;
    AD9850.set_frequency(freq);
    delay(1);
    Docalcswr();
    Swr = max(1.00,Swr);
    minSwr = min(minSwr,Swr);
    maxSwr = max(maxSwr,Swr);
    tempSwr = min (3.0, Swr);
    SWR[k] = 30+(164 - round(82*(tempSwr - 1))); //MAX SWR = 3.0 164+fontHeigth
    //SWR[k] = random(10, 50);
}

uint8_t update_graph_band(void) {
	if ( next_state_sweep < 7) {
		AD9850.set_frequency(freqBand[next_state_sweep]);   
		delay(10);
		Docalcswr();
		bandSwr[next_state_sweep] = Swr;
		next_state_sweep++;
		return 0;
	}else{
		next_state_sweep = 0;
		return 1;
	}
}

uint8_t dds(void) {
    Tft.fillRectangle(0, 0, 320, 240, BLACK);
    float steps = round(pow(10,posInput));
    Tft.drawFloat(posInput,130,214,2,RED);
    Tft.drawFloat(settings.freqCenter/1000,50,160,4,RED);
    AD9850.set_frequency(settings.freqCenter);
    delay(2);
    //printReverse(0, p0 ,"freq");
    Tft.fillRectangle(40,210, 60,20, CYAN);
    Tft.drawString("step",40, 210 ,2,BLUE);
    Tft.drawNumber(doubleClicked,5,210,2,GREEN);
    menu_redraw_required = 0;
    return 0;    
}


uint8_t update_graphics(void) {
 
	switch (select_display){
		case 1: return update_graph_band();
		break;
		case 2:	return dds();
		break;
		default: return update_graph();
	}
    // no update for the graphics required
	return 0;
}

uint8_t update_graph(void)
{
  Tft.fillRectangle(250, 0, 70, 20, BLUE);
  Tft.drawNumber(next_state_sweep,250,0,2,CYAN);
  
  if ( next_state_sweep < SWR_STEP)
    {
        if(next_state_sweep == 0)
        {
            minSwr = 10.00;
            maxSwr = 1.00;
        }
        float s = spanValue[settings.spanIdx]/SWR_STEP;
        calcParameters(next_state_sweep,s);
        next_state_sweep++;
        return 0;
    }
    else
    {
        next_state_sweep = 0;
        return 1;
    }
}
float updateFreq(double r, float v)
{
    float step = round(pow(10,posInput));
    if (r == 123 )
    {
        // do nothing
    }
    else if ((r == 16748655) && (v + spanValue[settings.spanIdx]/2 < 54000000.0)) // r depends on IR key so change accordingly
    {
        v = v + step;
    }
    else if ((r == 16754775) && (v >= step + 1000000.0))
    {
        v = v - step;
    }
    return v;
}
void updateSpan(double r)
{
    if (r == 123 )
    {
        // do nothing
    }
    else if (r == 16748655)
    {
        settings.spanIdx++;
    }
    else if (r == 16754775)
    {
        settings.spanIdx--;
        if (settings.spanIdx < 0) settings.spanIdx = totSpan -1;
    }
    settings.spanIdx = settings.spanIdx % totSpan;
}

void infrared()
{
    if (irrecv.decode(&results))
    {
        Serial.println(results.value);
        switch(results.value)
        {
            case 16728765:     // these strange nubers are the codes from ir remote, adjust to suit the keys
            singleclick();
            break;
            case 16730805:
            doubleclick();
            break;
            case 16732845:
            longclick();
            break;
            case 16738455:
            centerset();
            break;
        }
        if (doubleClicked)
        {
            if((select_display == 0) && modSpan)
            {
                updateSpan(results.value);
            }
            else
            settings.freqCenter = updateFreq(results.value, settings.freqCenter);
        }
        else if(select_display == 0) modSpan = !modSpan;
        menu_redraw_required = 1;
        irrecv.resume();
    }
}


void routine(void)
{

  if ( update_graphics() != 0 | menu_redraw_required != 0)
    {
        draw();
        menu_redraw_required=0;
        
    }
    menu_redraw_required = 0; // menu updated, reset redraw flag
  
}

void loop()
{
    timedAction.check(); //infrared
    timedAction2.check(); //routine
}

void singleclick()
{
    //change position in field
    //Serial.println("Single");
    if (doubleClicked)
    {
        posInput++;
        posInput = posInput % 7;
    }
    menu_redraw_required = 1;
}
void doubleclick()
{
    //change field
    //Serial.println("double");
    doubleClicked = !doubleClicked;
    if (!doubleClicked) eeprom_write_block((const void*)&settings, (void*)0, sizeof(settings));;
    menu_redraw_required = 1;
}
void longclick()
{
    //rotate displays
    //Serial.println("long");
    select_display = select_display++;
    select_display = select_display % 3;
    posInput = 0;
    doubleClicked = 0;
    eeprom_write_block((const void*)&settings, (void*)0, sizeof(settings));
    menu_redraw_required = 1;
}
void centerset()
{
    settings.freqCenter = 14000000.0;
    settings.spanIdx = 8; 
}
