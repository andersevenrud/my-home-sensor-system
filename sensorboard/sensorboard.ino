/*
 * my-home-sensor-system
 * @author Anders Evenrud <andersevenrud@gmail.com>
 * @license CC BY
 */
#include <AirQuality.h>
#include <DHT.h>
#include <DpsClass.h>
#include <Dps310.h>
#include <rgb_lcd.h>

///////////////////////////////////////////////////////////////////////
// DEFINITIONS
///////////////////////////////////////////////////////////////////////

#define SOUND_SENSOR_PIN A3
#define LIGHT_SENSOR_PIN A2
#define AIR_QUALITY_SENSOR_PIN A1
#define AIR_QUALITY_SAMPLES 122
#define TEMP_HUMID_SENSOR_PIN A0
#define TEMP_HUMID_SENSOR_TYPE DHT22
#define BUTTON_PIN 2
#define LED_PIN 4
#define CYCLE_TIME 1000
//#define ENABLE_LCD 1

byte heartCharacter[8] = { 0b00000, 0b01010, 0b11111, 0b11111, 0b11111, 0b01110, 0b00100, 0b00000 };
byte bucketEmpty[8] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b10001, 0b11111 };
byte bucketHalfEmpty[8] = { 0b10001, 0b10001, 0b10001, 0b10001, 0b11111, 0b11111, 0b11111, 0b11111 };
byte bucketFull[8] = { 0b10001, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };
char airQualities[5][3] = { "--", "??", "LO", "MD", "HI" };

///////////////////////////////////////////////////////////////////////
// STATE
///////////////////////////////////////////////////////////////////////

String message1 = "Initializing...";
String message2 = "(andersevenrud)";
long soundCurrentValue = -1;
int lightCurrentValue = -1;
char* airQualityCurrentValue = airQualities[0];
int airQualityCurrent = -1;
float tempAndHumidityCurrentValue[2] = {0, 0};
float pressureCurrentValue = 0;
int buttonCurrentValue = LOW;
int viewCount = 4;
int viewCurrent = 0;
DHT dht(TEMP_HUMID_SENSOR_PIN, TEMP_HUMID_SENSOR_TYPE);
AirQuality airQualitySensor;
Dps310 Dps310PressureSensor = Dps310();
rgb_lcd lcd;

///////////////////////////////////////////////////////////////////////
// LCD
///////////////////////////////////////////////////////////////////////

void setupLCD() {
#ifdef ENABLE_LCD
  lcd.begin(16, 2);
  lcd.setRGB(10, 10, 10);
  lcd.createChar(1, heartCharacter);
  lcd.createChar(2, bucketEmpty);
  lcd.createChar(3, bucketHalfEmpty);
  lcd.createChar(4, bucketFull);
#endif
}

void printLCD() {
#ifdef ENABLE_LCD
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(message1.substring(0, 16));
  lcd.setCursor(0, 1);
  lcd.print(message2.substring(0, 16));
#endif
}

///////////////////////////////////////////////////////////////////////
// SENSORS
///////////////////////////////////////////////////////////////////////

void setupSensors() {
  dht.begin();
  airQualitySensor.init(AIR_QUALITY_SENSOR_PIN);
  Dps310PressureSensor.begin(Wire);
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

void readPressureSensor() {
  float pressure;
  uint8_t oversampling = 7;

  Dps310PressureSensor.measurePressureOnce(pressure, oversampling);
  pressureCurrentValue = pressure / 100;
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
      String lightPercentage = String(map(lightCurrentValue, 0, 800, 0, 100));
      String soundPercentage = String(map(soundCurrentValue, 0, 1023, 0, 100));
      String tempString = String(tempAndHumidityCurrentValue[1], 1);
      String humidityString = String(tempAndHumidityCurrentValue[0], 1);
      String quality = String(airQualityCurrentValue);
      String qualityFirst = String(airQualitySensor.first_vol);

      message1 = tempString + "C" + " " + humidityString + "%" + "   " + quality;
      message2 = lightPercentage + "L" + " " + soundPercentage + "S" + " " + qualityFirst + "Q";
    }
    break;

    case 1: {
      int lightIntensity = map(lightCurrentValue, 0, 800, 0, 2);
      String light = String(lightCurrentValue);
      String lightIntensityString = String(lightIntensity);
      String lightIntensityIcon = String(char(lightIntensity + 2));

      int soundIntensity = map(soundCurrentValue, 0, 1023, 0, 2);
      String sound = String(soundCurrentValue);
      String soundIntensityString = String(soundIntensity);
      String soundIntensityIcon = String(char(soundIntensity + 2));

      message1 = "Light " + light + " " + lightIntensityIcon + " " + lightIntensityString;
      message2 = "Sound " + sound + " " + soundIntensityIcon + " " + soundIntensityString;
    }
    break;

    case 2: {
      int qualityIntensity = map(airQualityCurrent, -1, 3, 0, 2);
      String first = String(airQualitySensor.first_vol);
      String last = String(airQualitySensor.last_vol);
      String icon = String(char(qualityIntensity + 2));

      message1 = "Quality " + icon + " " + airQualityCurrent + "/3";
      message2 = "Quality " + first + "^" + last;
    }
    break;

    case 3:
      message1 = "Update time (ms)";
      message2 = diffTime;
    break;

    default:
      message1 = "Invalid view";
      message2 = viewCurrent;
    break;
  }
}

void dump() {
  String tempString = String(tempAndHumidityCurrentValue[1], 1);
  String metricTemp = "grove_sensor_temperature ";
  metricTemp += tempString;

  String humidityString = String(tempAndHumidityCurrentValue[0], 1);
  String metricHumidity = "grove_sensor_humidity ";
  metricHumidity += humidityString;

  String metricLight = "grove_sensor_light ";
  metricLight += lightCurrentValue;

  String metricSound = "grove_sensor_sound ";
  metricSound += soundCurrentValue;

  String metricQuality = "grove_sensor_quality ";
  metricQuality += airQualityCurrent;

  String metricQualityRaw = "grove_sensor_quality_raw ";
  metricQualityRaw += airQualitySensor.first_vol;

  String pressureString = String(pressureCurrentValue, 2);
  String metricPressureRaw = "grove_sensor_pressure ";
  metricPressureRaw += pressureString;

  Serial.println(metricTemp);
  Serial.println(metricHumidity);
  Serial.println(metricLight);
  Serial.println(metricSound);
  Serial.println(metricQuality);
  Serial.println(metricQualityRaw);
  Serial.println(metricPressureRaw);
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
  readPressureSensor();

  int diffTime = millis() - startTime;
  render(diffTime);
  printLCD();
  dump();
  digitalWrite(LED_PIN, HIGH);

  int sleepTime = max(0, CYCLE_TIME - diffTime);
  delay(sleepTime);
}

ISR(TIMER2_OVF_vect) {
  if(airQualitySensor.counter == AIR_QUALITY_SAMPLES) {
    airQualitySensor.last_vol = airQualitySensor.first_vol;
    airQualitySensor.first_vol = analogRead(AIR_QUALITY_SENSOR_PIN);
    airQualitySensor.counter = 0;
    airQualitySensor.timer_index = 1;
    PORTB = PORTB ^ 0x20;
  } else {
    airQualitySensor.counter++;
  }
}
