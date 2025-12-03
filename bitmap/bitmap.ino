#include <Adafruit_Protomatter.h>

#include "../assets/snowflake.c"
#include "../assets/snowman.c"
#include "../assets/tree.c"

/**
 * Dot matrix panel setup
 */
uint8_t rgbPins[] = {25, 26, 27, 14, 12, 13};
uint8_t addrPins[] = {23, 19, 5, 17};
uint8_t clockPin = 16;
uint8_t latchPin = 4;
uint8_t oePin = 15;

Adafruit_Protomatter matrix(128, 3, 1, rgbPins, 4, addrPins, clockPin, latchPin,
                            oePin, false, -2);

/**
 * Timer setup
 */
hw_timer_t *timer = NULL;
volatile SemaphoreHandle_t timerSemaphore;
void ARDUINO_ISR_ATTR onTimer() { xSemaphoreGiveFromISR(timerSemaphore, NULL); }

constexpr int snowflake_count = 5;
const int snowflake_params[snowflake_count][3] = {
    {1, 0, 32}, {42, 20, 64}, {78, 42, 64}, {98, 0, 8}, {120, 0, 31}};
int snowflake_y[snowflake_count];

void setup() {
  Serial.begin(115200);

  ProtomatterStatus status = matrix.begin();
  Serial.print("Initializing dot matrix panel. Status: ");
  Serial.println((int)status);
  if (status != PROTOMATTER_OK) {
    while (true)
      ;
  }
  matrix.setRotation(2);

  matrix.drawRGBBitmap(0, 5, tree, 41, 59);
  matrix.drawRGBBitmap(78, 14, snowman, 50, 50);

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

  matrix.show();

  // Set up timer to trigger an update every second.
  timerSemaphore = xSemaphoreCreateBinary();
  // Set timer resolution to 1Mhz; trigger the timer ervery 1M cycles (1s).
  timer = timerBegin(/* frequency = */ 1000000);
  timerAttachInterrupt(timer, &onTimer);
  timerAlarm(timer, /* alarm_value = */
             1000000, /* autoreload = */ true,
             /* reload_count = */ 0);

  // Initialize snowflake positions
  for (int i = 0; i < snowflake_count; i++) {
    snowflake_y[i] = snowflake_params[i][1];
  }
}

void loop() {
  if (xSemaphoreTake(timerSemaphore, 0) != pdTRUE) {
    return;
  }

  static uint16_t white = matrix.color565(255, 255, 255);
  static uint16_t black = matrix.color565(0, 0, 0);

  for (int i = 0; i < snowflake_count; i++) {
    if (snowflake_y[i] >= snowflake_params[i][1] &&
        snowflake_y[i] < snowflake_params[i][2]) {
      matrix.drawBitmap(snowflake_params[i][0], snowflake_y[i], snowflake, 6, 7,
                        black);
    }
    if (++snowflake_y[i] > snowflake_params[i][2] + 3) {
      snowflake_y[i] = snowflake_params[i][1];
    }
    if (snowflake_y[i] >= snowflake_params[i][1] &&
        snowflake_y[i] < snowflake_params[i][2]) {
      matrix.drawBitmap(snowflake_params[i][0], snowflake_y[i], snowflake, 6, 7,
                        white);
    }
  }

  matrix.show();
}
