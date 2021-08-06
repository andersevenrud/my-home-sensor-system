/*
 * my-home-sensor-system
 * @author Anders Evenrud <andersevenrud@gmail.com>
 * @license CC BY
 */
#include <AirQuality.h>
#include <DHT.h>
#include <rgb_lcd.h>

///////////////////////////////////////////////////////////////////////
// DEFINITIONS
///////////////////////////////////////////////////////////////////////

#define SOUND_SENSOR_PIN A3
#define LIGHT_SENSOR_PIN A2
#define AIR_QUALITY_SENSOR_PIN A1
#define TEMP_HUMID_SENSOR_PIN A0
#define TEMP_HUMID_SENSOR_TYPE DHT22
#define BUTTON_PIN 2
#define LED_PIN 4

byte heartCharacter[8] = { 0b00000, 0b01010, 0b11111, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000 };
byte bucketEmpty[8] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111 };
byte bucketHalfEmpty[8] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b11111, 0b11111, 0b11111, 0b11111 };
byte bucketFull[8] = { 0b10001, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };
char airQualities[5][3] = { "--", "??", "LO", "MD", "HI" };

///////////////////////////////////////////////////////////////////////
// STATE
///////////////////////////////////////////////////////////////////////

char message1[16] = "Initializing...";
char message2[16] = "(andersevenrud)";
long soundCurrentValue = -1;
int lightCurrentValue = -1;
char* airQualityCurrentValue = airQualities[0];
int airQualityCurrent = -1;
float tempAndHumidityCurrentValue[2] = {0, 0};
int buttonCurrentValue = LOW;
int viewCount = 4;
int viewCurrent = 0;
DHT dht(TEMP_HUMID_SENSOR_PIN, TEMP_HUMID_SENSOR_TYPE);
AirQuality airQualitySensor;
rgb_lcd lcd;

///////////////////////////////////////////////////////////////////////
// LCD
///////////////////////////////////////////////////////////////////////

void setupLCD() {
  lcd.begin(16, 2);
  lcd.setRGB(10, 10, 10);
  lcd.createChar(1, heartCharacter);
  lcd.createChar(2, bucketEmpty);
  lcd.createChar(3, bucketHalfEmpty);
  lcd.createChar(4, bucketFull);
}

void printLCD() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message1);
  lcd.setCursor(0, 1);
  lcd.print(message2);
}

///////////////////////////////////////////////////////////////////////
// SENSORS
///////////////////////////////////////////////////////////////////////

void setupSensors() {
  dht.begin();
  airQualitySensor.init(AIR_QUALITY_SENSOR_PIN);
}

void readLightSensor() {
  lightCurrentValue = analogRead(LIGHT_SENSOR_PIN);
}

void readSoundSensor() {
  long sound = 0;
  for(int i = 0; i < 32; i++) {
      sound += analogRead(SOUND_SENSOR_PIN);
  }

  soundCurrentValue = sound >> 5;
}

void readAirQualitySensor() {
  int quality = airQualitySensor.slope();
  if (quality >= 0) {
    airQualityCurrent = quality;
  }
  airQualityCurrentValue = airQualities[airQualityCurrent + 1];
}

void readTempAndHumiditySensor() {
  float temp_hum_val[2] = {0};
  if (!dht.readTempAndHumidity(temp_hum_val)) {
    tempAndHumidityCurrentValue[0] = temp_hum_val[0];
    tempAndHumidityCurrentValue[1] = temp_hum_val[1];
  }
}

void readButton() {
  int buttonState = digitalRead(BUTTON_PIN);
  if (buttonState != buttonCurrentValue) {
    if (buttonState) {
      viewCurrent = (viewCurrent + 1) % viewCount;
    }

    buttonCurrentValue = buttonState;
  }
}

///////////////////////////////////////////////////////////////////////
// INTEGRATIONS
///////////////////////////////////////////////////////////////////////

void render(int diffTime) {
  switch (viewCurrent) {
    case 0: {
      int lightPercentage = map(lightCurrentValue, 0, 800, 0, 100);
      int soundPercentage = map(soundCurrentValue, 0, 1023, 0, 100);
      char tempString[5];
      char humidityString[5];

      dtostrf(tempAndHumidityCurrentValue[0], 3, 1, humidityString);
      dtostrf(tempAndHumidityCurrentValue[1], 3, 1, tempString);

      sprintf(message1, "%sC %s%%   %s", tempString, humidityString, airQualityCurrentValue);
      sprintf(message2, "%dL %dS %dQ", lightPercentage, soundPercentage, airQualitySensor.first_vol);
    }
    break;

    case 1: {
      int lightIntensity = map(lightCurrentValue, 0, 800, 0, 2);
      int soundIntensity = map(soundCurrentValue, 0, 1023, 0, 2);

      sprintf(message1, "Light %4d %c %d", lightCurrentValue, char(lightIntensity + 2), lightIntensity);
      sprintf(message2, "Sound %4lu %c %d", soundCurrentValue, char(soundIntensity + 2), soundIntensity);
    }
    break;

    case 2: {
      int qualityIntensity = map(airQualityCurrent, -1, 3, 0, 2);

      sprintf(message1, "Quality %c %d/3", char(qualityIntensity + 2), airQualityCurrent);
      sprintf(message2, "Quality %d^%d", airQualitySensor.first_vol, airQualitySensor.last_vol);
    }
    break;

    case 3:
      sprintf(message1, "Update time (ms)");
      sprintf(message2, "%d", diffTime);
    break;

    default:
      sprintf(message1, "Invalid view");
      sprintf(message2, "%d", viewCurrent);
    break;
  }
}

void dump() {
  char metricTemp[64];
  char metricHumidity[64];
  char metricLight[64];
  char metricSound[64];
  char metricQuality[64];
  char metricQualityRaw[64];

  char tempString[5];
  char humidityString[5];
  dtostrf(tempAndHumidityCurrentValue[0], 3, 1, humidityString);
  dtostrf(tempAndHumidityCurrentValue[1], 3, 1, tempString);

  sprintf(metricTemp, "grove_sensor_temperature %s", tempString);
  sprintf(metricHumidity, "grove_sensor_humidity %s", humidityString);
  sprintf(metricLight, "grove_sensor_light %d", lightCurrentValue);
  sprintf(metricSound, "grove_sensor_sound %lu", soundCurrentValue);
  sprintf(metricQuality, "grove_sensor_quality %d", airQualityCurrent);
  sprintf(metricQualityRaw, "grove_sensor_quality_raw %d", airQualitySensor.first_vol);

  Serial.println(metricTemp);
  Serial.println(metricHumidity);
  Serial.println(metricLight);
  Serial.println(metricSound);
  Serial.println(metricQuality);
  Serial.println(metricQualityRaw);
}

///////////////////////////////////////////////////////////////////////
// MAIN
///////////////////////////////////////////////////////////////////////

void setup() {
  Serial.begin(9600);
  while (!Serial);

  pinMode(LED_PIN, OUTPUT);

  setupLCD();
  printLCD();
  setupSensors();
}

void loop() {
  int startTime = millis();
  digitalWrite(LED_PIN, LOW);

  readButton();
  readLightSensor();
  readSoundSensor();
  readAirQualitySensor();
  readTempAndHumiditySensor();

  int diffTime = millis() - startTime;
  render(diffTime);
  printLCD();
  dump();
  digitalWrite(LED_PIN, HIGH);

  int sleepTime = max(0, 1000 - diffTime);
  delay(sleepTime);
}

ISR(TIMER2_OVF_vect) {
  if(airQualitySensor.counter == 122) {
    airQualitySensor.last_vol = airQualitySensor.first_vol;
    airQualitySensor.first_vol = analogRead(AIR_QUALITY_SENSOR_PIN);
    airQualitySensor.counter = 0;
    airQualitySensor.timer_index = 1;
    PORTB = PORTB ^ 0x20;
  } else {
    airQualitySensor.counter++;
  }
}
