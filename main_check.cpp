#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Preferences.h>
#include <WiFi.h>
#include "time.h" // ไลบรารีสำหรับจัดการเวลา
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define BUTTON_LEFT 27
#define BUTTON_RIGHT 14
#define BUTTON_BACK 12
#define BUTTON_ENTER 13
#define TRIG_PIN 33
#define ECHO_PIN 25

volatile bool enter = false;
bool safeislock = true;
String correctPassword;      // import key in preferences
String lastUnlockSafe;       // import key in preferences
String fullString = "";      // ใช้ตอนเข้า/ตอนเปลื่ยน
String confirmPassword = ""; // ตอนเปลื่ยน
String currentTime = "";
bool confirm = false; // type string 1 เสร็จ
//=======================FUNCTION ULTRASONIC============================
float measureOnceUS(uint32_t timeout_us = 30000)
{
  // trigger pulse 10 μs to TRIG pin
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10); // >= 10 μs
  digitalWrite(TRIG_PIN, LOW);
  // measre HIGH (pulse width) at ECHO pin (μs)
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, timeout_us);
  return (float)duration;
}
float usToCm(float us)
{
  // formula: cm ≈ (us x 10^-6 / 10^-2) x (340 / 2)
  // original formula : distance(m) = high time (s) × 340 m/s / 2)
  return us <= 0 ? NAN : (us * 34 / 2000.0f);
}
float median5cm()
{
  float vals[5];
  for (int i = 0; i < 5; i++)
  {
    vals[i] = usToCm(measureOnceUS());
    delay(60); // >60 ms
  }
  // median for 5 values
  for (int i = 0; i < 5; i++)
    for (int j = i + 1; j < 5; j++)
      if (vals[j] < vals[i])
      {
        float t = vals[i];
        vals[i] = vals[j];
        vals[j] = t;
      }
  return vals[2];
}
//=======================VARIABLE SERVO============================
const int SERVO_PIN = 23;  // Servo pin
const int SERVO_CH = 0;    // PWM channel
const int SERVO_FREQ = 50; // 50 Hz frequency for servo
const int SERVO_RES = 12;  // 12-bit -> 0..4095
const int MIN_US = 500;    // 500 μs ~ -90° --> 0° (may need adjust)
const int MAX_US = 2500;   // 2500 μs ~ +90° --> 180°
//=======================VARIABLE TIME============================
const char *ntpServer = "pool.ntp.org"; // web import time
const long gmtOffset_sec = 7 * 3600;    // Timezone ของประเทศไทย (GMT+7)
const int daylightOffset_sec = 0;       // ประเทศไทยไม่มี Daylight Saving
//=======================VARIABLE SERVER============================
const char *ssid = "btm_F2.4G";     // *** ใส่ชื่อ Wi-Fi ของคุณ ***
const char *password = "136167118"; // *** ใส่รหัสผ่าน Wi-Fi ของคุณ ***
WiFiServer server(23);              // อ็อบเจกต์เซิร์ฟเวอร์ Port 23
WiFiClient client;                  // สร้าง อ็อบเจกต์ไคลเอนต์ Empty
//=======================FUNCTION SERVO============================
int angleToMicros(int angle)
{ // convert angle to pulse width
  angle = constrain(angle, 0, 180);
  return MIN_US + ((long)(MAX_US - MIN_US) * angle) / 180;
}
int microsToDuty(int us)
{ // convert pulse width to duty
  us = constrain(us, MIN_US, MAX_US);
  const int maxCount = (1 << SERVO_RES) - 1;  // 4095 (12-bit)
  long duty = ((long)us * maxCount) / 20000L; // duty = (pulse / period) * maxCount
  return (int)constrain(duty, 0, maxCount);
}
void writeAngle(int angleDeg)
{
  int us = angleToMicros(angleDeg);
  int duty = microsToDuty(us);
  ledcWrite(SERVO_CH, duty);
}
//=======================FUNCTION GETTIME============================
String getCurrentTime()
{
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo))
  {
    Serial.println("Failed to obtain time");
    return "TIME_ERROR";
  }
  char timeString[20];
  // รูปแบบการแสดงผล: "วัน/เดือน/ปี(2ตัวท้าย) ชั่วโมง:นาที:วินาที"
  // เช่น "13/09/25 21:04:40"
  strftime(timeString, sizeof(timeString), "%d/%m/%y %H:%M:%S", &timeinfo);

  return String(timeString);
}

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
  fullString = "";
  confirmPassword = "";
  confirm = false;
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
      writeAngle(90);
      safeislock = false;
    }
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
      record.begin("mySafe", false);
      correctPassword = record.getString("password", "1234");        // default "1234"
      lastUnlockSafe = record.getString("lastUnlock", "No Log Yet"); // record time
      record.end();
      safeislock = true;
      writeAngle(180);
    }
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
  char c;
  if (confirm == false)
  {
    oled.clearDisplay();
    oled.drawRect(2, 2, 124, 60, WHITE);
    oled.setCursor(20, 12);
    oled.println("Change Password");
    oled.setCursor(20, 27);
    oled.printf("-> %s", fullString.c_str());
    if (client.available())
    {
      c = client.read();
      client.write(c);
      fullString += c;
    }
    if (c == '\n')
    {
      confirm = true;
      fullString.trim();
    }
  }
  else
  {
    oled.clearDisplay();
    oled.drawRect(2, 2, 124, 60, WHITE);
    oled.setCursor(20, 12);
    oled.println("Change Password");
    oled.setCursor(20, 27);
    oled.printf("-> %s", fullString.c_str());
    oled.setCursor(20, 42);
    oled.printf("-> %s", confirmPassword.c_str());
    if (client.available())
    {
      c = client.read();
      client.write(c);
      confirmPassword += c;
    }
    if (c == '\n')
    {
      confirmPassword.trim();
      if (fullString == confirmPassword && fullString != "" && confirmPassword != "")
      {
        Serial.println("Password is same");

        record.begin("mySafe", false);
        record.putString("password", confirmPassword);
        record.end();
        enter = false;
        confirm = false;
        fullString = "";
        confirmPassword = "";
      }
      else
      {
        Serial.println("Password is not same");
        confirm = false;
        fullString = "";
        confirmPassword = "";
      }
    }
  }
}
void ShowLastUnlock()
{
  oled.clearDisplay();
  oled.drawRect(2, 2, 124, 60, WHITE);
  oled.setCursor(17, 12);
  oled.println("Last Unlock Time");
  oled.setCursor(17, 27);
  oled.printf("%s", lastUnlockSafe.c_str());
  oled.setCursor(17, 42);
  oled.printf("%s", currentTime.c_str());
}
//=======================PASSWORD STATE=============================
void client_connected()
{
  if (!client.connected())
  {
    client = server.available();
    if (client)
    {
      Serial.println("New client connected !");
      // client.println("Welcome !Please enter the password ");
    }
  }
}
void wifi_bruh()
{
  if (fullString.length() == 39)
  {
    oled.clearDisplay();
    oled.drawRect(2, 2, 124, 60, WHITE);
    oled.setCursor(7, 12);
    oled.println("Input Your Password");

    fullString = "";
    client.println("Welcome !Please enter the password ");
    Serial.println("\nWelcome !Please enter the password ");
  }
}
void input_password()
{
  char c;
  if (client.available())
  {
    c = client.read();
    client.write(c);
    fullString += c;

    oled.clearDisplay();
    oled.drawRect(2, 2, 124, 60, WHITE);
    oled.setCursor(8, 12);
    oled.println("Input Your Password");
    oled.setCursor(20, 27);
    oled.print(fullString);

    if (c == '\n')
    {
      fullString.trim();

      if (fullString == correctPassword) // default "1234"
      {
        Serial.println("Password Correct");
        currentTime = getCurrentTime(); // ฟังก์ชันอ่านเวลาจาก RTC/NTP
        record.begin("mySafe", false);
        record.putString("lastUnlock", currentTime); // record time
        record.end();
        Serial.printf("%s\n", currentTime.c_str());
        enter = false; // anti hallucination
        CurrentState = LOCKED;
        UnlockAnimation(); // safeislock = false <- in function
      }
      else // Password worng
      {
        LockAnimation();
      }
      fullString = "";
    }
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
  server.begin(); // Prepair connect
  Serial.println("Telnet server started on port 23");
  //===========================SETUP TIME==============================
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  Serial.println("Time configured via NTP.");
  //===========================SETUP OLED==============================
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  {
    Serial.println("SSD1306 allocation failed");
    while (true)
      ;
  }
  //===========================SETUP ULTRASONIC==============================
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  //===========================SETUP SERVO==============================
  ledcSetup(SERVO_CH, SERVO_FREQ, SERVO_RES);
  ledcAttachPin(SERVO_PIN, SERVO_CH);
  writeAngle(180);
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
  oled.clearDisplay(); // ล้างจอ
  oled.fillRect(50, 30, 30, 20, WHITE);
  oled.fillRect(52, 15, 4, 15, WHITE);
  oled.fillRect(74, 15, 4, 15, WHITE);
  oled.fillRect(52, 11, 26, 4, WHITE);
}
void loop()
{
  oled.setTextSize(1);
  oled.setTextColor(WHITE);
  float d_cm = median5cm();

  // input_password();
  if (safeislock == true)
  {
    client_connected(); // if not connect tera -> conect tera
    wifi_bruh();
    input_password();
  }
  else if (safeislock == false)
  // safeislock = false ปลอด lock
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
  // Serial.printf("Distance: %.1f cm\n", d_cm);
  
  // if(d_cm >= 10)
  // {
  //   oled.clearDisplay();
  // }
  // else if (d_cm <= 40)
  // {
  //   oled.fillRect(50, 30, 30, 20, WHITE);
  //   oled.fillRect(52, 15, 4, 15, WHITE);
  //   oled.fillRect(74, 15, 4, 15, WHITE);
  //   oled.fillRect(52, 11, 26, 4, WHITE);
    
  // }
  oled.display();
}