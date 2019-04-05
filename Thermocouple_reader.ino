#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <OneWire.h>

Adafruit_SSD1306 display = Adafruit_SSD1306();
OneWire maxim1(19); // 5th pin from the top left down
byte addr[]={0,0,0,0,0,0,0,0}; // the address of the 1wire device
byte present = 0; // whether a device exists ( is address worth checking)
volatile uint8_t i2c_regs[] =
{
  0xfa,  // 00- arrays are 0 indexed
  0x21,  // 01- bit 5 indicates a device is present, bit one is the address CRC check
  0x02,  // 02- address family code (LSb)
  0x01,  // 03- address 48-bit serial number
  0x01,  // 04- address 48-bit serial number
  0x01,  // 05- address 48-bit serial number
  0x01,  // 06- address 48-bit serial number
  0x01,  // 07- address 48-bit serial number
  0x01,  // 08- address 48-bit serial number
  0x01,  // 09- address 8-bit CRC (MSb)
  0x01,  // 10- local comp of address CRC
  0x01,  // 11- scratchpad cold-junct-comp thermo T lsb  byte0
  0x01,  // 12- scratchpad
  0x01,  // 13- scratchpad
  0x01,  // 14- scratchpad
  0x01,  // 15- scratchpad config register
  0x01,  // 16- scratchpad ffh
  0x01,  // 17- scratchpad ffh
  0x01,  // 18- scratchpad ffh
  0x01,  // 19- scratchpad CRC
  0x01,  // 20- local comp of scratchpad CRC
  0x01,  // 21- CJC THERMO T byte1, divisor 16
  0x01,  // 22- CJC THERMO T byte2
  0x01,  // 23- CJ Temp byte1  divisor 256
  0x01,  // 24- CJ Temp byte2
  0x01,  // 25- corrected Temp byte 1, divisor 16
  0x01,  // 26- corrected temp byte 2
  0x01,  // 27- setpoint byte 1, divisor 16
  0x90,   // 28- setpoint byte 2
  0x01
}; // not sure if those hex numbers mean anything
const byte reg_size = sizeof(i2c_regs);
const float coeffmVperC[] = {0E0, 0.387481063640E-1, 0.332922278800E-4, 0.206182434040E-6 , -0.21882256846E-8, 0.109968809280E-10, -0.30815758720E-13, 0.454791352900E-16, -0.27512901673E-19};  
const float coeffCpermV[] = {0E0, 2.592800E1, -7.602961E-1, 4.637791E-2, -2.165394E-3, 6.048144E-5, -7.293422E-7, 0E0};
uint16_t cjTemp, hjTemp;
float cjTemp_fl, hjTemp_fl,totTemp_fl,cjmV, hjmV,totmV;

#if defined(ESP8266)
  #define BUTTON_A 0
  #define BUTTON_B 16
  #define BUTTON_C 2
  #define LED      0
#elif defined(ESP32)
  #define BUTTON_A 15
  #define BUTTON_B 32
  #define BUTTON_C 14
  #define LED      13
#elif defined(ARDUINO_STM32F2_FEATHER)
  #define BUTTON_A PA15
  #define BUTTON_B PC7
  #define BUTTON_C PC5
  #define LED PB5
#elif defined(TEENSYDUINO)
  #define BUTTON_A 4
  #define BUTTON_B 3
  #define BUTTON_C 8
  #define LED 13
#elif defined(ARDUINO_FEATHER52)
  #define BUTTON_A 31
  #define BUTTON_B 30
  #define BUTTON_C 27
  #define LED 17
#else // 32u4, M0, and 328p
  #define BUTTON_A 9
  #define BUTTON_B 6
  #define BUTTON_C 5
  #define LED      13
#endif

#if (SSD1306_LCDHEIGHT != 32)
 #error("Height incorrect, please fix Adafruit_SSD1306.h!");
#endif

void setup() {  
  Serial.begin(9600);
  //while(!Serial);

  Serial.println("OLED FeatherWing test");
  // by default, we'll generate the high voltage from the 3.3v line internally! (neat!)
  display.begin(SSD1306_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3C (for the 128x32)
  // init done
  Serial.println("OLED begun");

  // initialize the one wire interface on pin 18 (A0)
  if ( !maxim1.search(addr)) {
    maxim1.reset_search();
    delay(250);
    return;
  }
  present = maxim1.reset();
  i2c_regs[10] = OneWire::crc8( addr, 7);

  
  // Show image buffer on the display hardware.
  // Since the buffer is intialized with an Adafruit splashscreen
  // internally, this will display the splashscreen.
  display.display();
  delay(1000);
  display.setRotation(2);
  // Clear the buffer.
  display.clearDisplay();
  display.display();
  display.invertDisplay(0);
  Serial.println("IO test");

  pinMode(BUTTON_A, INPUT_PULLUP);
  pinMode(BUTTON_B, INPUT_PULLUP);
  pinMode(BUTTON_C, INPUT_PULLUP);
  // text display tests
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.print("IO test");
  display.display(); // actually display all of the above
  delay(1000);
  display.clearDisplay();
  display.setCursor(0,0);
  display.print(addr[0],HEX);
  display.print(addr[1],HEX);
  display.print(addr[2],HEX);
  display.print(addr[3],HEX);
  display.print(addr[4],HEX);
  display.print(addr[5],HEX);
  display.print(addr[6],HEX);
  display.print(addr[7],HEX);
  display.setCursor(0,0);
  display.display();
}


void loop() {
  //Serial.println("Enter Loop");
  if (! digitalRead(BUTTON_A)) display.print("A");
  if (! digitalRead(BUTTON_B)) display.print("B");
  if (! digitalRead(BUTTON_C)) display.print("C");
  delay(100);
  //yield();
  //display.display();
  //display.clearDisplay();

  //Serial.println("1");
  convertT();
  //Serial.println("2");
  delay(1000);
  //Serial.println("3");
  readScratchpad();
  //Serial.println("4");
  delay(1000);
  //Serial.println("5");
  datatoTemps();
  //Serial.println("6");
  

  display.clearDisplay();
  display.print("CJ Temp: ");
  display.print(cjTemp_fl);
  display.print("\nHJ Temp: ");
  display.print(hjTemp_fl);
  display.print("\nTotal Temp: ");
  display.print(totTemp_fl);
  display.print("\nTotal Temp F: ");
  display.print(totTemp_fl*9.0/5.0+32.0);  
  display.setCursor(0,0);
  display.display(); 

  
}



// FUNCTIONS FOR ONE WIRE INTERACTION
void convertT()
// this function tells a maxim 31850 chip at address (addr) to take a T measurement
{
  //Serial.println("convertT() enter");
  maxim1.reset();
  maxim1.select(addr);
  maxim1.write(0x44, 1); // tells chip to start conversion
  delay(100);
  //Serial.println("convertT() exit");
}

void readScratchpad()
//
{
  //Serial.println("readScratchpad() enter");
  present = maxim1.reset();
  maxim1.select(addr);
  maxim1.write(0xBE);         // Read Scratchpad

  //display.clearDisplay();
  
  int i;
  //noInterrupts();
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    i2c_regs[i+11] = maxim1.read();
    Serial.println(i2c_regs[i+11],HEX);
    //display.print("\nTotal Temp: ");
  }
  //display.setCursor(0,0);
  //display.display();
  //interrupts();
  Serial.println("readScratchpad() exit");
}

void datatoTemps()
{
  Serial.println("datatoTemps() enter");
  //int i;
  //for ( i = 11; i < 20; i++) {
  //  //Serial.print(addr[i], HEX);
  //  //Serial.print(" ");
  //  i2c_regs[i] = data[i - 11];
  //}
  uint16_t reg1,reg2;  
  reg1 = i2c_regs[14];
  reg2 = i2c_regs[13];
  cjTemp = (((reg1<<8)&0xFF00)|((reg2)&0x00F0));
  
  reg1 = i2c_regs[12];
  reg2 = i2c_regs[11];
  hjTemp = (((reg1<<8) & 0xFF00) | ((reg2) & 0x00FC));
  
  cjTemp_fl = ((float)cjTemp)/256.0;
  hjTemp_fl = ((float)hjTemp)/16.0;
  
  hjmV = (hjTemp_fl-cjTemp_fl)*.05218;
  cjmV = 0.0;
  cjmV += coeffmVperC[1]*cjTemp_fl;
  cjmV += coeffmVperC[2]*cjTemp_fl*cjTemp_fl;
  cjmV += coeffmVperC[3]*cjTemp_fl*cjTemp_fl*cjTemp_fl;
  cjmV += coeffmVperC[4]*cjTemp_fl*cjTemp_fl*cjTemp_fl*cjTemp_fl;
  cjmV += coeffmVperC[5]*cjTemp_fl*cjTemp_fl*cjTemp_fl*cjTemp_fl*cjTemp_fl;
  totmV = cjmV + hjmV;
  totTemp_fl = 0.0;
  totTemp_fl += coeffCpermV[1]*totmV;
  totTemp_fl += coeffCpermV[2]*totmV*totmV;
  totTemp_fl += coeffCpermV[3]*totmV*totmV*totmV;
  totTemp_fl += coeffCpermV[4]*totmV*totmV*totmV*totmV;
  totTemp_fl += coeffCpermV[5]*totmV*totmV*totmV*totmV*totmV;
  //totTemp = (uint16_t)(totTemp_fl*16.0);
  //inputT = totTemp_fl;
  
  Serial.println("datatoTemps() exit");
}
