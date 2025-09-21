# 2568-CPE-213-R3-Electronic-Lock-Safe-You-and-Only-You
# 🔐 Electronic Safe Lock (ตู้เซฟดิจิทัล)

โปรเจกต์นี้เป็น ตู้นิรภัยดิจิทัล ที่พัฒนาด้วย ESP32 โดยสามารถล็อก/ปลดล็อกด้วยรหัสผ่านผ่าน Wi-Fi (Telnet) และแสดงผลบน OLED Display พร้อมทั้งบันทึกเวลาเปิดล่าสุด โดยมีระบบควบคุม Servo Motor และตรวจจับวัตถุด้วย Ultrasonic Sensor

---

## 📦 อุปกรณ์ที่ใช้
-ESP32 DevKit V1
-OLED Display (128x64, I2C)
-Servo Motor (SG90 หรือเทียบเท่า)
-Ultrasonic Sensor (HC-SR04)
-Push Buttons (4 ตัว: LEFT, RIGHT, BACK, ENTER)
-Breadboard & Jumper wires

---

## ⚡ การทำงานของระบบ
 -ระบบล็อก/ปลดล็อกตู้นิรภัยด้วยรหัสผ่าน
 -การเชื่อมต่อ Wi-Fi และ Telnet เพื่อป้อนรหัสผ่านจากคอมพิวเตอร์
 -การบันทึกและอ่านรหัสผ่านจาก EEPROM (Preferences)
 -การแสดงผลสถานะและเมนูต่าง ๆ บน OLED Display (128x64, I2C)
 -ระบบ Servo Motor ควบคุมการหมุนกุญแจ (0° - 180°)
 -ตรวจจับการเข้าใกล้ด้วย Ultrasonic Sensor
 -การบันทึกเวลา Unlock ล่าสุดโดยใช้ NTP Server (RTC ผ่าน Wi-Fi)
 -ระบบเปลี่ยนรหัสผ่านผ่านเมนู Change Password

---

## 🖥️ การต่อวงจร

OLED → SDA = GPIO21, SCL = GPIO22
Servo → PWM = GPIO23
Ultrasonic → TRIG = GPIO33, ECHO = GPIO25
Buttons → GPIO27, GPIO14, GPIO12, GPIO13 (พร้อม Pull-up)

---

## ▶️ วิธีใช้งาน
1. ต่อวงจรตามที่ระบุ
2. อัปโหลดโค้ดไปยัง ESP32 ด้วย Arduino IDE
3. เปิดไฟเลี้ยง → OLED จะขึ้นข้อความ "Locked"
4. กดรหัสถูกต้อง → Servo จะหมุนและขึ้นข้อความ "Unlocked"
5. หากกดผิดจะยังคงล็อก

---

## 📸 ตัวอย่างการประกอบ
![safe lock demo](./images/safe_lock_demo.png)

---

## 👨‍💻 ผู้พัฒนา
- ชื่อ: [Mikito Enomoto]
- ชื่อ: [Peranut Karunrattanakul]
- วิชา: [CPE-213_Electronic Safe Lock]

