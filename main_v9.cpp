#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <WiFi.h>
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUTTON_LEFT 27
#define BUTTON_RIGHT 14
#define BUTTON_BACK 12
#define BUTTON_ENTER 13
volatile bool enter = false;
bool safeislock = true;
String correctPassword;
String lastUnlockSafe;

String fullString = "";

const char *ssid = "btm_F2.4G";     // *** ใส่ชื่อ Wi-Fi ของคุณ ***
const char *password = "136167118"; // *** ใส่รหัสผ่าน Wi-Fi ของคุณ ***
WiFiServer server(23);              // อ็อบเจกต์เซิร์ฟเวอร์ Port 23
WiFiClient client;                  // สร้าง อ็อบเจกต์ไคลเอนต์ Empty

//=======================DEBOUNCEBUTTON============================
volatile unsigned long lastDebounceTime = 0;
const unsigned long debounceDelay = 200;
volatile unsigned long lastDebounceEnter = 0;
const unsigned long debounceEnter = 500;
//=======================DELAYANIMATION============================
unsigned long lastDelayAnimation = 0;
const unsigned long DelayAnimation = 30;
Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Preferences record;
//=========================STATE MENU===============================
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
  if (enter == false)
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
}
void IRAM_ATTR Right()
{
  if (enter == false)
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
}
void IRAM_ATTR BACK()
{
  enter = false;
}
void IRAM_ATTR ENTER()
{
  enter = true;
}

//=======================DISPLAY STATE=============================
void unlock()
{
  oled.clearDisplay();
  oled.drawRect(2, 2, 124, 60, WHITE);
  oled.setCursor(40, 20);
  oled.println("UNLOCKED");

  oled.setCursor(32, 35);
  oled.println("Press Enter");
}
void locked()
{
  oled.clearDisplay();
  oled.drawRect(2, 2, 124, 60, WHITE);
  oled.setCursor(45, 20);
  oled.println("LOCKED");
  oled.setCursor(32, 35);
  oled.println("Press Enter");
}
void change_password()
{
  oled.clearDisplay();
  oled.drawRect(2, 2, 124, 60, WHITE);
  oled.setCursor(20, 20);
  oled.println("CHANCE PASSWORD");
  oled.setCursor(32, 35);
  oled.println("Press Enter");
}
void log_view()
{
  oled.clearDisplay();
  oled.drawRect(2, 2, 124, 60, WHITE);
  oled.setCursor(40, 20);
  oled.println("LOG VIEW");
  oled.setCursor(32, 35);
  oled.println("Press Enter");
}
//======================DISPLAY ANIMATION==========================
void UnlockAnimation()
{
  if (safeislock == true)
  {

    if (millis() - lastDebounceEnter > debounceEnter)
    {
      lastDebounceEnter = millis();
      for (int i = 0; i <= 10; i++)
      {
        oled.clearDisplay(); // ล้างจอ
        // static
        oled.fillRect(50, 30, 30, 20, WHITE); // body
        // animation
        oled.fillRect(52, 15, 4, 15, WHITE);     // left
        oled.fillRect(74, 15, 4, 15 - i, WHITE); // right
        oled.fillRect(52, 11, 26, 4, WHITE);     // above
        oled.display();
      }
    }
    safeislock = false;
  }
  else
  {
    oled.clearDisplay(); // ล้างจอ
    // static
    oled.fillRect(50, 30, 30, 20, WHITE);
    // animation
    oled.fillRect(52, 15, 4, 15, WHITE);
    oled.fillRect(74, 15, 4, 5, WHITE);
    oled.fillRect(52, 11, 26, 4, WHITE);
  }
}
void LockAnimation()
{
  if (safeislock == false)
  {

    if (millis() - lastDebounceEnter > debounceEnter)
    {
      lastDebounceEnter = millis();
      for (int i = 0; i <= 10; i++)
      {

        oled.clearDisplay(); // ล้างจอ
        // static
        oled.fillRect(50, 30, 30, 20, WHITE);
        // animation
        oled.fillRect(52, 15, 4, 15, WHITE);
        oled.fillRect(74, 15, 4, 5 + i, WHITE);
        oled.fillRect(52, 11, 26, 4, WHITE);
        oled.display();
      }
    }
    safeislock = true;
  }
  else
  {
    oled.clearDisplay(); // ล้างจอ
    // static
    oled.fillRect(50, 30, 30, 20, WHITE);
    // animation
    oled.fillRect(52, 15, 4, 15, WHITE);
    oled.fillRect(74, 15, 4, 15, WHITE);
    oled.fillRect(52, 11, 26, 4, WHITE);
  }
}
void ChangePassword()
{
  while (enter == true)
  {
    oled.clearDisplay();
    oled.drawRect(2, 2, 124, 60, WHITE);
    oled.setCursor(17, 12);
    oled.println("Change Your Password");
    oled.setCursor(20, 27);
    oled.printf("-> %s", lastUnlockSafe);
    oled.setCursor(36, 42);
    oled.println("Press Back");
    oled.display();
  }
}
void ShowLastUnlock()
{
  oled.clearDisplay();
  oled.drawRect(2, 2, 124, 60, WHITE);
  oled.setCursor(17, 12);
  oled.println("Last Unlock Time");
  oled.setCursor(20, 27);
  oled.printf("-> %s", lastUnlockSafe);
  oled.setCursor(36, 42);
  oled.println("Press Back");
}
//=======================PASSWORD STATE=============================
void input_password()
{
  char c;
  if (!client.connected())
    {
      client = server.available();
      if (client)
      {
        Serial.println("New client connected !");
        // client.println("Welcome !Please enter the password ");
      }
    }
    else if (fullString.length() == 39)
    {
      oled.clearDisplay();
      oled.drawRect(2, 2, 124, 60, WHITE);
      oled.setCursor(7, 12);
      oled.println("Input Your Password");

      fullString = "";
      client.println("Welcome !Please enter the password ");
      Serial.println("\nWelcome !Please enter the password ");
    }
    else if (client.available())
    {
      c = client.read();
      client.write(c);
      fullString += c;

      oled.clearDisplay();
      oled.drawRect(2, 2, 124, 60, WHITE);
      oled.setCursor(12, 12);
      oled.println("Input Your Password");
      oled.setCursor(20, 27);
      oled.print(fullString);
    }
    if (c == '\n')
    {
      fullString.trim();
      Serial.println("Password Correct");
      if (fullString == "1234")
      {
        CurrentState = LOCKED;
        UnlockAnimation();
      }
      else//Password worng
      {
        oled.clearDisplay(); // ล้างจอ
        // static
        oled.fillRect(50, 30, 30, 20, WHITE);
        // animation
        oled.fillRect(52, 15, 4, 15, WHITE);
        oled.fillRect(74, 15, 4, 15, WHITE);
        oled.fillRect(52, 11, 26, 4, WHITE);
        fullString = "";
        //input_password();
      }
      fullString = "";
    }
}
void setup()
{
  Serial.begin(9600);
  //===========================SETUP WIFI==============================
  WiFi.begin(ssid, password);
  Serial.print("Connecting to WiFi...");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  // เริ่มต้น Server
  server.begin(); // Prepair connect
  Serial.println("Telnet server started on port 23");

  //===========================SETUP OLED==============================
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    while (true)
      ;
  }
  //======================SETUP PREFERENCES==========================
  record.begin("mySafe", false);                                 // namespace "mySafe"
  correctPassword = record.getString("password", "1234");        // default "1234"
  lastUnlockSafe = record.getString("lastUnlock", "No Log Yet"); // default "No Log Yet"
  record.end();
  Serial.println("Safe ready. Password and Last unlock loaded.");
  //============================PIN MODE=============================
  pinMode(BUTTON_LEFT, INPUT_PULLUP);
  pinMode(BUTTON_RIGHT, INPUT_PULLUP);
  pinMode(BUTTON_BACK, INPUT_PULLUP);
  pinMode(BUTTON_ENTER, INPUT_PULLUP);
  //============================SETUP INTERRRUPT=============================
  attachInterrupt(digitalPinToInterrupt(BUTTON_LEFT), Left, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_RIGHT), Right, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_BACK), BACK, FALLING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_ENTER), ENTER, FALLING);

  Serial.print("\n=================\nProject is Ready\n=================\n");
}
void loop()
{
  
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  // input_password();
  if (safeislock == true)
  {
    input_password();
  }
  else //safeislock = false ปลอด lock
  {
    switch (CurrentState)
    {
    case 0:
      if (enter == true)
      {
        UnlockAnimation();
      }
      else
      {
        unlock(); // display
      }
      break; // จบ case 0

    case 1:
      if (enter == true)
      {
        LockAnimation(); // display
      }
      else
      {
        locked(); // display
      }
      break; // จบ case 1

    case 2:
      if (enter == true)
      {
        ChangePassword();
      }
      else
      {
        change_password(); // display
      }
      break; // จบ case 2

    case 3:
      if (enter == true)
      {
        ShowLastUnlock(); // display
      }
      else
      {
        log_view(); // display
      }

      break; // จบ case 3

    default:
      oled.println("ERROR");
      break;
    }
  }

  oled.display();
}