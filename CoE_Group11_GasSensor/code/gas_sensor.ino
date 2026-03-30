#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <Preferences.h>

const char* ssid     = "AAAAAAA"; // replace AAAAAAA with the actual ssid you want the system to connect to
const char* password = "XXXXXXX"; // replace XXXXXXX with the wifi password

#define BOTtoken  "XXXXXXXXXXXX" // replace with the actual bot token given by the BotFather
#define CHAT_ID   "-XXXXXXXX" // replace with the chat id of the telegram group

const String ADMIN_IDS[] = {
  "111111111",
  "222222222",
  "333333333" // change to the telegram IDs of the admin users
};
const int ADMIN_COUNT = 3;

const int gasAO    = 34;
const int buzzer   = 25;
const int redLed   = 26;
const int greenLed = 18;
const int lcd_sda  = 33;
const int lcd_scl  = 32;

#define BUZZ_DANGER_FREQ    4000
#define BUZZ_CAUTION_FREQ   2500
#define BUZZ_TEST_FREQ      4500

#define BUZZ_MIN_ON_MS      3000
#define TEST_BEEP_ON_MS      800
#define TEST_BEEP_OFF_MS     300
#define TEST_BEEP_COUNT        5
#define DANGER_BLINK_MS      250
#define CAUTION_BLINK_MS     800
#define CAUTION_BEEP_EVERY  3000
#define CAUTION_BEEP_LEN     300
#define REPEAT_DANGER_MS   20000
#define WIFI_CHECK_MS      20000
#define BOT_POLL_MS          800

#define PPM_SAFE_MAX     950
#define PPM_CAUTION_MAX 1150
#define READING_COUNT     10

bool systemActive  = true;
bool alarmMuted    = false;
bool wifiConnected = false;
int  gasState      = 0;
int  lastGasState  = -1;

unsigned long lastBlink       = 0;
unsigned long lastBotCheck    = 0;
unsigned long lastWiFiCheck   = 0;
unsigned long lastDangerMsg   = 0;
unsigned long lastCautionBeep = 0;
unsigned long buzzerOnSince   = 0;
bool          buzzerState     = false;

int readings[READING_COUNT];
int readIndex    = 0;
int lastPPM      = 0;
int history[10];
int historyIndex = 0;
int maxPPM       = 0;

WiFiClientSecure     client;
UniversalTelegramBot bot(BOTtoken, client);
LiquidCrystal_I2C    lcd(0x27, 16, 2);
Preferences          prefs;

void printLine(int row, String text) {
  lcd.setCursor(0, row);
  while (text.length() < 16) text += " ";
  lcd.print(text.substring(0, 16));
}

void sendMessage(String msg) {
  if (WiFi.status() == WL_CONNECTED) {
    bot.sendMessage(CHAT_ID, msg, "");
  }
}

bool isAdmin(String user_id) {
  for (int i = 0; i < ADMIN_COUNT; i++) {
    if (ADMIN_IDS[i] == user_id) return true;
  }
  return false;
}

bool isFromGroup(String chat_id) {
  return chat_id == CHAT_ID;
}

void handleNewMessages(int numNewMessages) {
  for (int i = 0; i < numNewMessages; i++) {

    String from_id = bot.messages[i].from_id;
    String chat_id = bot.messages[i].chat_id;
    String text    = bot.messages[i].text;

    int atIndex = text.indexOf('@');
    if (atIndex != -1) text = text.substring(0, atIndex);

    if (!isFromGroup(chat_id)) {
      bot.sendMessage(chat_id, "This bot only accepts messages from the authorized group.", "");
      continue;
    }

    bool adminOnlyCommand = (
      text == "/alarm_off"   ||
      text == "/alarm_on"    ||
      text == "/reset_alarm" ||
      text == "/shutdown"    ||
      text == "/reset_system"
    );

    if (adminOnlyCommand && !isAdmin(from_id)) {
      bot.sendMessage(CHAT_ID, "⛔ Access denied. This command is for admins only.", "");
      continue;
    }

    if (text == "/status") {
      String stat = "📊 *SYSTEM STATUS*\n";
      stat += "PPM: " + String(lastPPM) + "\n";
      stat += "Max Recorded: " + String(maxPPM) + "\n";
      stat += "Alarm Muted: " + String(alarmMuted ? "Yes" : "No") + "\n";
      stat += "System Active: " + String(systemActive ? "Yes" : "No") + "\n";
      String lvl = (gasState == 0) ? "Safe" : (gasState == 1) ? "Caution" : "DANGER";
      stat += "Level: " + lvl;
      bot.sendMessage(CHAT_ID, stat, "Markdown");
    }

    else if (text == "/history") {
      String msg = "📈 *PPM HISTORY (last 10):*\n";
      for (int j = 0; j < 10; j++) {
        msg += "• " + String(history[j]) + " ppm\n";
      }
      bot.sendMessage(CHAT_ID, msg, "Markdown");
    }

    else if (text == "/test_alarm") {
      bot.sendMessage(CHAT_ID, "🔊 Testing Alarm Pulse...", "");
      for (int k = 0; k < TEST_BEEP_COUNT; k++) {
        tone(buzzer, BUZZ_TEST_FREQ);
        delay(TEST_BEEP_ON_MS);
        noTone(buzzer);
        delay(TEST_BEEP_OFF_MS);
      }
      bot.sendMessage(CHAT_ID, "🔊 Test complete.", "");
    }

    else if (text == "/alarm_off") {
      alarmMuted = true;
      noTone(buzzer);
      bot.sendMessage(CHAT_ID, "🔕 Buzzer Muted. Visuals remain.", "");
    }

    else if (text == "/alarm_on") {
      alarmMuted = false;
      bot.sendMessage(CHAT_ID, "🔔 Buzzer Enabled.", "");
    }

    else if (text == "/reset_alarm") {
      alarmMuted   = false;
      gasState     = 0;
      lastGasState = 0;
      noTone(buzzer);
      digitalWrite(redLed,   LOW);
      digitalWrite(greenLed, HIGH);
      printLine(1, "STATUS: SAFE");
      bot.sendMessage(CHAT_ID, "♻️ Alarm reset. System returned to safe state.", "");
    }

    else if (text == "/shutdown") {
      systemActive = false;
      noTone(buzzer);
      digitalWrite(redLed,   LOW);
      digitalWrite(greenLed, LOW);
      printLine(0, "SYSTEM SHUTDOWN");
      printLine(1, "By admin command");
      bot.sendMessage(CHAT_ID, "⛔ System shut down by admin. Send /restart to bring it back online.", "");
    }

    else if (text == "/reset_system") {
      bot.sendMessage(CHAT_ID, "♻️ Rebooting ESP32...", "");
      prefs.begin("tgbot", false);
      prefs.putInt("offset", bot.last_message_received + 1);
      prefs.end();
      delay(1000);
      ESP.restart();
    }

    else if (text == "/restart") {
      systemActive = true;
      gasState     = 0;
      lastGasState = -1;
      digitalWrite(greenLed, HIGH);
      digitalWrite(redLed,   LOW);
      printLine(0, "KNUST GAS SENSOR");
      printLine(1, "SYSTEM READY");
      bot.sendMessage(CHAT_ID, "🚀 System is back online.", "");
    }

    else if (text == "/help") {
      String msg = "📋 *Available Commands:*\n\n";
      msg += "*Everyone in the group:*\n";
      msg += "/status — Current PPM and system state\n";
      msg += "/history — Last 10 PPM readings\n";
      msg += "/test\\_alarm — Trigger a test beep\n";
      msg += "/restart — Bring system back online\n";
      msg += "/help — Show this list\n\n";
      msg += "*Admins only:*\n";
      msg += "/alarm\\_off — Mute the buzzer\n";
      msg += "/alarm\\_on — Re-enable the buzzer\n";
      msg += "/reset\\_alarm — Reset alarm to safe state\n";
      msg += "/shutdown — Shut down the monitoring system\n";
      msg += "/reset\\_system — Reboot the ESP32\n";
      bot.sendMessage(CHAT_ID, msg, "Markdown");
    }

    else {
      bot.sendMessage(CHAT_ID, "❓ Unknown command. Send /help to see available commands.", "");
    }
  }
}

void connectWiFi() {
  Serial.print("Connecting to: "); Serial.println(ssid);
  WiFi.begin(ssid, password);
  client.setInsecure();

  printLine(0, "WIFI CONNECTING");
  printLine(1, "Please wait...");

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 15) {
    delay(1000);
    Serial.print(".");
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    wifiConnected = true;
    Serial.println("\nConnected!");
    printLine(0, "SYSTEM ONLINE");
    sendMessage("🚀 *KNUST Lab Monitor Started*");
  } else {
    wifiConnected = false;
    printLine(0, "WIFI FAILED");
    printLine(1, "Offline mode...");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin(lcd_sda, lcd_scl);
  lcd.init();
  lcd.backlight();

  pinMode(buzzer,   OUTPUT);
  pinMode(redLed,   OUTPUT);
  pinMode(greenLed, OUTPUT);
  pinMode(gasAO,    INPUT);

  noTone(buzzer);
  digitalWrite(redLed,   LOW);
  digitalWrite(greenLed, HIGH);

  printLine(0, "KNUST GAS SENSOR");
  printLine(1, "INITIALIZING...");
  delay(2000);

  connectWiFi();

  prefs.begin("tgbot", true);
  int savedOffset = prefs.getInt("offset", 0);
  prefs.end();
  if (savedOffset > 0) {
    bot.last_message_received = savedOffset - 1;
    Serial.println("Restored bot offset: " + String(savedOffset));
  }

  printLine(0, "KNUST GAS SENSOR");
  printLine(1, "SYSTEM READY");
  delay(1000);
}

void loop() {

  if (millis() - lastWiFiCheck > WIFI_CHECK_MS) {
    if (WiFi.status() != WL_CONNECTED) connectWiFi();
    lastWiFiCheck = millis();
  }

  if (WiFi.status() == WL_CONNECTED && millis() - lastBotCheck > BOT_POLL_MS) {
    int num = bot.getUpdates(bot.last_message_received + 1);
    while (num) {
      handleNewMessages(num);
      num = bot.getUpdates(bot.last_message_received + 1);
    }
    lastBotCheck = millis();
  }

  if (systemActive) {
    readings[readIndex++] = analogRead(gasAO);

    if (readIndex >= READING_COUNT) {
      readIndex = 0;

      long sum = 0;
      for (int i = 0; i < READING_COUNT; i++) sum += readings[i];
      int avgRaw = sum / READING_COUNT;

      int ppm = map(avgRaw, 0, 4095, 0, 5000);

      Serial.println("Current PPM: " + String(ppm));
      printLine(0, "PPM: " + String(ppm));

      history[historyIndex++] = ppm;
      if (historyIndex >= 10) historyIndex = 0;
      if (ppm > maxPPM) maxPPM = ppm;

      if (lastPPM != 0 && (ppm - lastPPM) > 300) {
        sendMessage("⚠️ *Rapid Rise Detected!*\nPPM: " + String(ppm));
      }
      lastPPM = ppm;

      int newState;
      if      (ppm > PPM_CAUTION_MAX) newState = 3;
      else if (ppm > PPM_SAFE_MAX)    newState = 1;
      else                            newState = 0;

      if (newState != lastGasState) {
        if (newState == 0) {
          sendMessage("✅ Air is clear.");
        } else if (newState == 1) {
          sendMessage("ℹ️ Smoke/Gas detected (Low)\nPPM: " + String(ppm));
        } else if (newState == 3) {
          sendMessage("🚨 *CRITICAL LEAK DETECTED!*\nPPM: " + String(ppm));
          lastDangerMsg = millis();
        }
        gasState     = newState;
        lastGasState = newState;
      }

      if (gasState == 3 && millis() - lastDangerMsg > REPEAT_DANGER_MS) {
        sendMessage("🚨 *STILL DANGEROUS!*\nPPM: " + String(ppm));
        lastDangerMsg = millis();
      }
    }

    if (gasState == 3) {
      if (millis() - lastBlink > DANGER_BLINK_MS) {
        buzzerState = !buzzerState;
        digitalWrite(redLed, buzzerState);

        if (!alarmMuted) {
          if (buzzerState) {
            tone(buzzer, BUZZ_DANGER_FREQ);
            buzzerOnSince = millis();
          } else {
            if (millis() - buzzerOnSince >= BUZZ_MIN_ON_MS) {
              noTone(buzzer);
            }
          }
        } else {
          noTone(buzzer);
        }

        lastBlink = millis();
      }
      digitalWrite(greenLed, LOW);
      printLine(1, "!! DANGER !!");
    }

    else if (gasState == 1) {
      if (millis() - lastBlink > CAUTION_BLINK_MS) {
        digitalWrite(redLed, !digitalRead(redLed));
        lastBlink = millis();
      }

      if (!alarmMuted && millis() - lastCautionBeep > CAUTION_BEEP_EVERY) {
        tone(buzzer, BUZZ_CAUTION_FREQ, CAUTION_BEEP_LEN);
        lastCautionBeep = millis();
      }

      digitalWrite(greenLed, LOW);
      printLine(1, "!! CAUTION !!");
    }

    else {
      digitalWrite(greenLed, HIGH);
      digitalWrite(redLed,   LOW);
      noTone(buzzer);
      printLine(1, "STATUS: SAFE");
    }
  }

  delay(50);
}