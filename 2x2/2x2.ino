#include <Arduino.h>
#include <WiFi.h>
#include <sntp.h>
#include <time.h>

#include <ESP32-HUB75-VirtualMatrixPanel_T.hpp>

/**
 * Dot matrix panel setup
 */

#define R1 25
#define G1 26
#define B1 27
#define R2 14
#define G2 12
#define B2 13

#define ADDR_A 23
#define ADDR_B 19
#define ADDR_C 5
#define ADDR_D 17
#define ADDR_E -1

#define CLK 16
#define LAT 4
#define OE 15

HUB75_I2S_CFG mxconfig(/* w= */ 64, /*h= */ 32, /* chain= */ 4,
                       {R1, G1, B1, R2, G2, B2, ADDR_A, ADDR_B, ADDR_C, ADDR_D,
                        ADDR_E, LAT, OE, CLK},
                       HUB75_I2S_CFG::SHIFTREG, HUB75_I2S_CFG::TYPE138,
                       /* doublebuffer= */ false,
                       /* i2sspeed= */ HUB75_I2S_CFG::HZ_8M,
                       /* latblank= */ 2, /* clockphase= */ false,
                       /* min_refresh_rate= */ 60,
                       /* color_depth_bits= */ 8);

MatrixPanel_I2S_DMA physicalDisplay(mxconfig);

VirtualMatrixPanel_T<CHAIN_TOP_RIGHT_DOWN> matrix(2, 2, 64, 32);

/**
 * WiFi setup
 */
#include "wifi_info.h"

/**
 * NTP server setup
 */
const char* ntpServer = "north-america.pool.ntp.org";
// Why do we not care about the timezone here?

/**
 * Timer setup
 */
hw_timer_t* timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
void ARDUINO_ISR_ATTR onTimer() { xSemaphoreGiveFromISR(timerSemaphore, NULL); }

const boolean test = false;
time_t time_xmas_2025;
boolean countdown_finished = false;

void setup() {
  Serial.begin(115200);

  // Setup dot matrix display panel
  physicalDisplay.begin();
  matrix.setDisplay(physicalDisplay);

  drawBackground();

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

  time_xmas_2025 = test ? time(nullptr) + 5 : 1766649600;

  time_t time_now = time(nullptr);
  long difference = (long)difftime(time_xmas_2025, time_now);
  if (difference < 0) {
    countdown_finished = true;
    drawChristmasText();
  } else {
    drawCountdownText();
  }
}
void loop() {
  // We only update once per second.
  if (xSemaphoreTake(timerSemaphore, 0) != pdTRUE) {
    return;
  }

  animateSnowFlakes();

  if (countdown_finished) {
    return;
  }

  time_t time_now = time(nullptr);
  long difference = (long)difftime(time_xmas_2025, time_now);

  if (difference < 0) {
    countdown_finished = true;
    // Redraw the background to clear the countdown text.
    drawBackground();
    drawChristmasText();
  } else {
    animateCountdown(difference);
  }
}

#include "../assets/snowflake.c"
#include "../assets/snowman.c"
#include "../assets/tree.c"

constexpr int snowflake_count = 5;
const int snowflake_params[snowflake_count][3] = {
    {1, 0, 36}, {42, 20, 64}, {78, 42, 64}, {98, 0, 14}, {120, 0, 37}};
int snowflake_y[snowflake_count];

void drawBackground() {
  matrix.fillScreen(0);
  matrix.drawRGBBitmap(0, 5, tree, 41, 59);
  matrix.drawRGBBitmap(78, 14, snowman, 50, 50);
}

void drawCountdownText() {
  matrix.setTextSize(1);

  matrix.setCursor(61, 2);
  matrix.setTextColor(matrix.color565(255, 255, 255), 0);
  matrix.print("DAYS");

  matrix.setCursor(52, 22);
  matrix.setTextColor(matrix.color565(0, 255, 0));
  matrix.print("'TIL");

  matrix.setCursor(52, 32);
  matrix.setTextColor(matrix.color565(255, 0, 0), 0);
  matrix.print("XMAS");

  matrix.setCursor(52, 42);
  matrix.setTextColor(matrix.color565(255, 255, 0), 0);
  matrix.print("2025");
}

void drawChristmasText() {
  matrix.setTextSize(1);
  matrix.setCursor(49, 2);
  matrix.setTextColor(matrix.color565(255, 0, 0));
  matrix.print("MERRY");

  matrix.setCursor(37, 12);
  matrix.setTextColor(matrix.color565(0, 255, 0));
  matrix.print("CHRISTMAS");

  matrix.setCursor(61, 22);
  matrix.setTextColor(matrix.color565(255, 255, 255));
  matrix.print("&");

  matrix.setTextSize(1);
  matrix.setCursor(49, 32);
  matrix.setTextColor(matrix.color565(255, 0, 0));
  matrix.print("HAPPY");

  matrix.setCursor(55, 42);
  matrix.setTextColor(matrix.color565(255, 255, 0));
  matrix.print("NEW");

  matrix.setCursor(52, 52);
  matrix.setTextColor(matrix.color565(0, 0, 255));
  matrix.print("YEAR");
}

void animateSnowFlakes() {
  static uint16_t white = matrix.color565(255, 255, 255);
  static uint16_t black = matrix.color565(0, 0, 0);

  for (int i = 0; i < snowflake_count; i++) {
    int max_y = snowflake_params[i][2];
    int h = min(max_y - snowflake_y[i], 7);
    if (snowflake_y[i] >= snowflake_params[i][1] && snowflake_y[i] < max_y) {
      matrix.drawBitmap(snowflake_params[i][0], snowflake_y[i], snowflake, 6, h,
                        black);
    }
    if (++snowflake_y[i] > max_y + 3) {
      snowflake_y[i] = snowflake_params[i][1];
    }
    h = min(max_y - snowflake_y[i], 7);
    if (snowflake_y[i] >= snowflake_params[i][1] && snowflake_y[i] < max_y) {
      matrix.drawBitmap(snowflake_params[i][0], snowflake_y[i], snowflake, 6, h,
                        white);
    }
  }
}

void animateCountdown(long difference) {
  // Sync with NTP once a day to calibrate our clock.
  const long NTP_SYNC_INTERVAL = 24 * 60 * 60 * 1000;  // 24 hours
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

  static int current_days = -1;

  int seconds = difference % 60;
  int minutes = (difference / 60) % 60;
  int hours = (difference / 3600) % 24;
  int days = difference / (3600 * 24);

  if (days != current_days) {
    current_days = days;
    matrix.setTextSize(1);
    matrix.setCursor(43, 2);
    matrix.setTextColor(matrix.color565(255, 255, 255), 0);
    matrix.printf("%02d", days);
  }

  static bool show_colon = true;
  show_colon = !show_colon;
  matrix.setTextSize(1);
  matrix.setTextColor(matrix.color565(255, 255, 255), 0);
  matrix.setCursor(40, 12);
  matrix.printf(show_colon ? "%02d:%02d:%02d" : "%02d %02d %02d", hours,
                minutes, seconds);
}
