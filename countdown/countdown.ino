#include <Adafruit_Protomatter.h>
#include <WiFi.h>
#include <sntp.h>
#include <time.h>

/**
 * Dot matrix panel setup
 */
uint8_t rgbPins[] = {25, 26, 27, 14, 12, 13};
uint8_t addrPins[] = {23, 19, 5, 17};
uint8_t clockPin = 16;
uint8_t latchPin = 4;
uint8_t oePin = 15;

Adafruit_Protomatter matrix(/* width= */ 128,
                            /* depth= */ 4,
                            /* matrix_chains= */ 1, rgbPins,
                            /* addr_count= */ 4, addrPins, clockPin, latchPin,
                            oePin,
                            /* double_buffer= */ false, /* tile= */ -2);

/**
 * WiFi setup
 */
#include "wifi_info.h"

/**
 * NTP server setup
 */
const char *ntpServer = "north-america.pool.ntp.org";
// Why do we not care about the timezone here?

/**
 * Timer setup
 */
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
void ARDUINO_ISR_ATTR onTimer() { xSemaphoreGiveFromISR(timerSemaphore, NULL); }

const time_t time_xmas_2025 = 1766649600;

void setup() {
  Serial.begin(115200);

  // Setup dot matrix display panel
  ProtomatterStatus status = matrix.begin();
  Serial.print("Initializing dot matrix panel. Status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    while (true)
      ;
  }
  matrix.setRotation(2);
  matrix.setTextSize(2);

  matrix.setCursor(10, 0);
  matrix.setTextColor(color444(15, 0, 0), 0); // Red on black
  matrix.print("Christmas");

  matrix.setCursor(10, 16);
  matrix.setTextColor(color444(0, 15, 0), 0); // Green on black
  matrix.print("Countdown");

  matrix.setCursor(64, 32);
  matrix.setTextColor(color444(15, 15, 15), 0); // White on black
  matrix.print("days");

  matrix.show();

  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi...");

  // Wait for the connection to be established.
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected: ");
  Serial.println(WiFi.localIP());

  // Sync time with NTP server
  configTime(0, 0, ntpServer);

  // Wait for the sync to be completed.
  Serial.println("Waiting for NTP time synchronization...");
  while (sntp_get_sync_status() != SNTP_SYNC_STATUS_COMPLETED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.print("\nTime synchronized: ");
  Serial.println(time(nullptr));

  // Set up timer to trigger an update every second.
  timerSemaphore = xSemaphoreCreateBinary();
  // Set timer resolution to 1Mhz; trigger the timer ervery 1M cycles (1s).
  timer = timerBegin(/* frequency = */ 1000000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, /* alarm_value = */
             1000000, /* autoreload = */ true,
             /* reload_count = */ 0);
}

void loop() {
  // We only update once per second.
  if (xSemaphoreTake(timerSemaphore, 0) != pdTRUE) {
    return;
  }

  // Sync with NTP once a day to calibrate our clock.
  const long NTP_SYNC_INTERVAL = 24 * 60 * 60 * 1000; // 24 hours
  static long lastSync = 0;
  long millisNow = millis();
  if (millisNow - lastSync > NTP_SYNC_INTERVAL) {
    lastSync = millisNow;
    configTime(0, 0, ntpServer);
  }

  sntp_sync_status_t syncStatus = sntp_get_sync_status();
  if (syncStatus == SNTP_SYNC_STATUS_COMPLETED) {
    // TODO: display a notification on the panel
    Serial.println("Time synchronized.");
  }

  time_t time_now = time(nullptr);
  long difference = (long)difftime(time_xmas_2025, time_now);

  if (difference < 0) {
    Serial.println("Merry Christmas! 🎄");
    return;
  }

  static int current_days = -1;

  int seconds = difference % 60;
  int minutes = (difference / 60) % 60;
  int hours = (difference / 3600) % 24;
  int days = difference / (3600 * 24);

  char buffer[100];
  snprintf(buffer, sizeof(buffer),
           "Christmas\nCountdown\n%03d days\n%02d:%02d:%02d", days, hours,
           minutes, seconds);
  Serial.println(buffer);

  if (days != current_days) {
    current_days = days;
    matrix.setCursor(16, 32);
    matrix.setTextColor(color444(15, 15, 15), 0);
    matrix.printf("%03d days", days);
  }

  static bool show_colon = true;
  show_colon = !show_colon;
  matrix.setCursor(16, 48);
  matrix.setTextColor(color444(15, 15, 15), 0);
  matrix.printf(show_colon ? "%02d:%02d:%02d" : "%02d %02d %02d", hours,
                minutes, seconds);
  matrix.show();
}

inline uint16_t color444(uint8_t r, uint8_t g, uint8_t b) {
  return ((r & 0x0F) << 12) | ((g & 0x0F) << 7) | ((b & 0x0F) << 1);
}