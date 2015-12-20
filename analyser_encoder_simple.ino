#include <stdint.h>
#include <TFTv2.h> //https://github.com/riyas-org/ili9341
#include <SPI.h>
#include <avr/eeprom.h>
#include <AH_AD9850.h> //http://www.arduino-projekte.de/index.php?n=7
#include <OneButton.h> //https://github.com/riyas-org/OneButton
#include <Rotary.h> //https://github.com/riyas-org/Rotary
#define SWR_STEP 64 //set it high for more points in the plot

Rotary r = Rotary(3, 2); // Encoder connected to interrupt pins 2 and 3 on arduino promini (atmega328)
OneButton button(A0,true);// Click button on the encoder the other end is connected to ground
AH_AD9850 AD9850(9, 8, 7, 10); //AH_AD9850(int CLK, int FQUP, int BitData, int RESET);

// Pins for Fwd/Rev detectors
const int REV_ANALOG_PIN = A2;
const int FWD_ANALOG_PIN = A1;
//const int EXTRA_ANALOG_PIN = A3; // an extra detector for rf power 

const byte lcdW = 320 ;
const byte lcdH = 240 ;
const byte fontWidth = 10 ;
const byte fontHeigth = 30 ;
const byte totSpan = 9;
float spanValue[totSpan] =
{
    25000.0, 50000.0,100000.0, 250000.0,500000.0, 1000000.0, 2500000.0, 5000000.0,10000000.0
}
;
byte SWR[SWR_STEP];
//byte PWR[SWR_STEP];
float minSwr;
float maxSwr;
uint32_t next_state_sweep = 0;
byte menu_redraw_required = 0;
byte doubleClicked = 0;
byte modSpan = 0;
byte posInput = 0;
byte sign = 0; // negative
float Swr ;
float Pwr ;
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
    if (pin==FWD_ANALOG_PIN) //in my case forward reading has an offset
    return analogRead(pin);
    else
    return analogRead(pin)-17;
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
       // Pwr = sqrt(analog_read_value(EXTRA_ANALOG_PIN));
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
        Tft.fillRectangle(40,210, 60,20, CYAN);
        Tft.drawString("freq",40, 210 ,2,BLUE);
        Tft.drawString("span",210, 210 ,2,BLUE);
    }
    else
    {
        Tft.fillRectangle(210,210, 50,20, CYAN);
        Tft.drawString("freq",40, 210 ,2,BLUE);
        Tft.drawString("span",210, 210 ,2,BLUE);
    }
}
void printSwr()
{
    for (int k = 0;k < SWR_STEP; k++)
    {
        int spacer=320/SWR_STEP;
        Tft.drawLine(k*spacer,SWR[k],(k+1)*spacer,SWR[k+1],WHITE); //plot vswr
        //Tft.drawLine(k*spacer,PWR[k],(k+1)*spacer,PWR[k+1],GREEN); //plot power
    }
}
void showCursor(int x0, int y0)
{
    Tft.fillRectangle(x0,y0, 50,8, BRIGHT_RED);
}
void draw(void)
{
    Tft.fillRectangle(0, 0, 320, 240, BLACK);
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
void setup(void)
{
    Tft.TFTinit(); // init TFT library
    Tft.fillRectangle(0, 0, 320, 240, BLACK);
    analogReference(DEFAULT);
    // initialize serial communication at 9600 bits per second:
    Serial.begin(9600);
    //rotory interrupt
    PCICR |= (1 << PCIE2);
    PCMSK2 |= (1 << PCINT18) | (1 << PCINT19);
    sei();
    //set up the click on the encoder
    button.attachDoubleClick(doubleclick);
    button.attachClick(singleclick);
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
}
void calcParameters(int k, float stepSpan)
{
    float freq;
    float tempSwr;
    //float tempPwr;
    freq = settings.freqCenter + (k-SWR_STEP/2)* stepSpan;
    if (freq <= 0) freq = 1000000.0;
    AD9850.set_frequency(freq);
    delay(1);
    Docalcswr();
    Swr = max(1.00,Swr);
    //Pwr= max(1.00,Pwr);
    //tempPwr= min (600, Pwr);
    //Serial.print("temppwr:");
    //Serial.println(tempPwr);
    minSwr = min(minSwr,Swr);
    maxSwr = max(maxSwr,Swr);
    tempSwr = min (3.0, Swr);
    SWR[k] = 30+(164 - round(82*(tempSwr - 1))); //MAX SWR = 3.0 164+fontHeigth
    //PWR[k] = 30+(164 - round(164*((tempPwr*tempPwr)/360000))); //Pwr;
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
    if (r == DIR_NONE )
    {
        // do nothing
    }
    else if ((r == DIR_CW) && (v + spanValue[settings.spanIdx]/2 < 54000000.0))
    {
        v = v + step;
    }
    else if ((r == DIR_CCW) && (v >= step + 1000000.0))
    {
        v = v - step;
    }
    return v;
}
void updateSpan(double r)
{
    if (r == DIR_NONE )
    {
        // do nothing
    }
    else if (r == DIR_CW)
    {
        settings.spanIdx++;
    }
    else if (r == DIR_CCW)
    {
        settings.spanIdx--;
        if (settings.spanIdx < 0) settings.spanIdx = totSpan -1;
    }
    settings.spanIdx = settings.spanIdx % totSpan;
}
void routine(void)
{
    if ( update_graph() != 0 | menu_redraw_required != 0)
    {
        draw();
    }
    menu_redraw_required = 0; // menu updated, reset redraw flag
}
void loop()
{
    // keep watching the push button:
    button.tick();
    routine();
}
ISR(PCINT2_vect) {
    //Serial.println("interrupt");
    unsigned char result = r.process();
    if (doubleClicked)
    {
        if(modSpan){
            updateSpan(result);
        }
        else
        {
            settings.freqCenter = updateFreq(result, settings.freqCenter);
        }
    }
    else modSpan = !modSpan;
    menu_redraw_required = 1;
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
