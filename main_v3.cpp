#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUTTON_LEFT 27
#define BUTTON_RIGHT 14
#define BUTTON_BACK 12
#define BUTTON_ENTER 13
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;

enum state
{
  UNLOCKED,
  LOCKED,
  VIEW_LOG,
  CHANGE_PASSWORD
};
volatile state CurrentState = UNLOCKED;

//===================INTERRUPT BUTTON PIN==========================
void IRAM_ATTR Left()
{
  if (millis() - lastDebounceTime >= debounceDelay)
  {
    lastDebounceTime = millis();
    CurrentState = (state)(CurrentState - 1);
    if (CurrentState == -1)
    {
      CurrentState = CHANGE_PASSWORD;      
    }

    
    
  }
}
void IRAM_ATTR Right()
{
  if (millis() - lastDebounceTime >= debounceDelay)
  {
    lastDebounceTime = millis();
    CurrentState = (state)(CurrentState + 1);
    if (CurrentState == 4)
    {
      CurrentState = UNLOCKED;
    }

    
  }
}

//=================================================================
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
void setup()
{
  Serial.begin(9600);

  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    while (true)
      ;
  }
  
 
  // PINMODE
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_ENTER, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), Left, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), Right, FALLING);

  Serial.print("\n=================\nProject is Ready\n=================\n");
}
void loop()
{
  
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  oled.drawRect(5, 5, 118, 54, WHITE);
  if (CurrentState == 0)
  { 
    oled.clearDisplay();
    oled.drawRect(5, 5, 118, 54, WHITE);
    oled.setCursor(20, 20);
    oled.println("UNLOCKED");
  }
  else if (CurrentState == 1)
  {  
    oled.clearDisplay();
    oled.drawRect(5, 5, 118, 54, WHITE);
    oled.setCursor(20, 20);
    oled.println("LOCKED");
  }
  else if (CurrentState == 2)
  {
    oled.clearDisplay();
    oled.drawRect(5, 5, 118, 54, WHITE);
    oled.setCursor(20, 20);
    oled.println("CHANCE PASSWORD");
  }
  else if (CurrentState == 3)
  {
    oled.clearDisplay();
    oled.drawRect(5, 5, 118, 54, WHITE);
    oled.setCursor(20, 20);
    oled.println("LOG VIEW");
  }

  oled.display();
}