#include <Wire.h>
#include <MAX30105.h>
#include <heartRate.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_ADXL345_U.h>
#include <SoftwareSerial.h>

// ---------- GSM ----------
SoftwareSerial gsm(8, 9); // RX, TX
const char phoneNumber[] = "+919XXXXXXXXX";

// ---------- Sensors ----------
MAX30105 particleSensor;
Adafruit_ADXL345_Unified accel = Adafruit_ADXL345_Unified(123);

// ---------- Thresholds ----------
#define MIN_BPM 50
#define MAX_BPM 120
#define VIB_THRESHOLD 1.0

// ---------- Calibration Offsets ----------
float offsetX = 0, offsetY = 0, offsetZ = 0;
bool alertSent = false;

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);
  gsm.begin(9600);
  Wire.begin();

  // MAX30102 Init
  if (!particleSensor.begin(Wire, I2C_SPEED_STANDARD)) {
    Serial.println("MAX30102 not found");
    while (1);
  }
  particleSensor.setup();

  // ADXL345 Init
  if (!accel.begin()) {
    Serial.println("ADXL345 not found");
    while (1);
  }

  Serial.println("Keep ADXL345 still... Calibrating");
  delay(5000);
  calibrateADXL();

  Serial.println("System Ready");
}

// ---------- LOOP ----------
void loop() {
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue)) {
    static unsigned long lastBeat = 0;
    unsigned long now = millis();
    int bpm = 60 / ((now - lastBeat) / 1000.0);
    lastBeat = now;

    Serial.print("BPM: ");
    Serial.println(bpm);

    // Read calibrated vibration
    float vibration = readVibration();

    Serial.print("Vibration: ");
    Serial.println(vibration);

    // Alert Condition
    if ((bpm < MIN_BPM || bpm > MAX_BPM || vibration > VIB_THRESHOLD) && !alertSent) {
      makeCall();
      alertSent = true;
    }

    // Reset alert
    if (bpm >= MIN_BPM && bpm <= MAX_BPM && vibration <= VIB_THRESHOLD) {
      alertSent = false;
    }
  }

  delay(200);
}

// ---------- ADXL345 CALIBRATION ----------
void calibrateADXL() {
  const int samples = 200;

  for (int i = 0; i < samples; i++) {
    sensors_event_t event;
    accel.getEvent(&event);

    offsetX += event.acceleration.x;
    offsetY += event.acceleration.y;
    offsetZ += (event.acceleration.z - 9.81); // remove gravity

    delay(10);
  }

  offsetX /= samples;
  offsetY /= samples;
  offsetZ /= samples;

  Serial.println("Calibration Done");
  Serial.print("Offset X: "); Serial.println(offsetX);
  Serial.print("Offset Y: "); Serial.println(offsetY);
  Serial.print("Offset Z: "); Serial.println(offsetZ);
}

// ---------- READ CALIBRATED VIBRATION ----------
float readVibration() {
  sensors_event_t event;
  accel.getEvent(&event);

  float ax = event.acceleration.x - offsetX;
  float ay = event.acceleration.y - offsetY;
  float az = event.acceleration.z - offsetZ;

  return sqrt(ax * ax + ay * ay + az * az);
}

// ---------- GSM CALL ----------
void makeCall() {
  Serial.println("Calling Emergency Number...");
  gsm.println("ATD" + String(phoneNumber) + ";");
  delay(15000);
  gsm.println("ATH");
}
