<div>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"

// -------- CONFIG ----------
#define DHTPIN        2
//#define DHTTYPE      DHT11  // uncomment if using DHT11
#define DHTTYPE       DHT22  // uncomment if using DHT22

#define TRIG_PIN      3
#define ECHO_PIN      4
#define IR_PIN        5
#define LDR_PIN       6
#define LED_PIN       7
#define RELAY1_PIN    8   // fan
#define RELAY2_PIN    9   // spare

#define TEMP_THRESHOLD_C  30.0   // Temperature threshold to switch fan ON (°C)
#define PRESENCE_DISTANCE_CM 120 // distance threshold (cm) for presence detection
#define LCD_ADDR 0x27            // try 0x27 or 0x3F depending on your I2C backpack

// Smoothing/intervals
const unsigned long DHT_INTERVAL_MS = 2000;      // DHT read interval (ms)
const unsigned long ULTRASONIC_INTERVAL_MS = 300; // Ultrasonic interval
const unsigned long LCD_UPDATE_MS = 500;        // LCD refresh rate
const unsigned int ULTRASONIC_TIMEOUT_US = 30000; // pulseIn timeout (us) ~ 10m => 30000 = 5m

// -------- GLOBALS ----------
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);
DHT dht(DHTPIN, DHTTYPE);

unsigned long lastDhtMillis = 0;
unsigned long lastUltrasonicMillis = 0;
unsigned long lastLcdMillis = 0;

float tempC = NAN;
float hum = NAN;
int distanceCM = 999;

bool relay1State = false; // fan OFF
bool relay2State = false; // spare OFF

// For simple median smoothing of distance (3 samples)
int distSamples[3] = {999, 999, 999};
int sampleIndex = 0;

void setup() {
  Serial.begin(115200);
  // Pins
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(IR_PIN, INPUT);
  pinMode(LDR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);

  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);

  // Many relay boards are active LOW; we'll use helper functions to abstract.
  relayWrite(RELAY1_PIN, false);
  relayWrite(RELAY2_PIN, false);

  // Init libs
  dht.begin();
  Wire.begin(); // I2C
  lcd.init();
  lcd.backlight();

  // initial lcd message
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Automated Room");
  lcd.setCursor(0,1);
  lcd.print("Control System");
  delay(800);
  lcd.clear();
}

void loop() {
  unsigned long now = millis();

  // DHT readings every DHT_INTERVAL_MS
  if (now - lastDhtMillis >= DHT_INTERVAL_MS) {
    lastDhtMillis = now;
    readDHT();
  }

  // Ultrasonic reading more frequently
  if (now - lastUltrasonicMillis >= ULTRASONIC_INTERVAL_MS) {
    lastUltrasonicMillis = now;
    int d = readUltrasonic();
    // update median circular buffer
    distSamples[sampleIndex] = d;
    sampleIndex = (sampleIndex + 1) % 3;
    distanceCM = median3(distSamples[0], distSamples[1], distSamples[2]);
  }

  // Sensors digital states
  bool irDetected = (digitalRead(IR_PIN) == LOW) ? true : false;
  // Many IR obstacle modules pull LOW on detection. Adjust logic if yours is opposite
  bool isDark = (digitalRead(LDR_PIN) == LOW) ? true : false;
  // Many LDR modules pull LOW when dark; adjust if yours is opposite

  // Logic: Fan ON if temp above threshold OR presence + hot
  bool shouldFanOn = false;
  if (!isnan(tempC)) {
    shouldFanOn = (tempC >= TEMP_THRESHOLD_C);
  }
  // Optionally: if presence detected (ultrasonic or IR) & temp moderately high, turn on fan
  if (!shouldFanOn && (distanceCM < PRESENCE_DISTANCE_CM || irDetected) && !isnan(tempC) && tempC >= (TEMP_THRESHOLD_C - 2)) {
    shouldFanOn = true;
  }
  setRelay(RELAY1_PIN, shouldFanOn);

  // Example usage of RELAY2: turn on when presence detected (close distance)
  bool shouldRelay2On = (distanceCM < 50) || irDetected; // e.g., presence -> light on
  setRelay(RELAY2_PIN, shouldRelay2On);

  // LED alert: on when dark and presence detected
  digitalWrite(LED_PIN, (isDark && (distanceCM < 120 || irDetected)) ? HIGH : LOW);

  // Update LCD at slower rate
  if (now - lastLcdMillis >= LCD_UPDATE_MS) {
    lastLcdMillis = now;
    updateLCD(tempC, hum, distanceCM, irDetected, isDark);
  }

  // small yield to avoid WDT issues
  delay(10);
}

// ----------------- helper functions -------------------
void readDHT() {
  // DHT read with retries
  float h = NAN, t = NAN;
  const int MAX_TRIES = 3;
  for (int i=0;i<MAX_TRIES;i++){
    h = dht.readHumidity();
    t = dht.readTemperature(); // Celsius
    if (!isnan(h) && !isnan(t)) break;
    delay(200); // small wait before retry
  }
  if (!isnan(h) && !isnan(t)) {
    hum = h;
    tempC = t;
    Serial.print("DHT -> T: "); Serial.print(tempC); Serial.print(" °C  H: "); Serial.print(hum); Serial.println(" %");
  } else {
    Serial.println("DHT read failed");
    // keep previous valid values (do not overwrite with NAN) so system stays stable
  }
}

int readUltrasonic() {
  // trigger pulse
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(4);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  // read echo (microseconds)
  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ULTRASONIC_TIMEOUT_US);
  if (duration == 0) {
    // timeout -> no reading
    Serial.println("Ultrasonic: timeout");
    return 999; // indicate no object (large value)
  }
  // distance in cm = duration / 58.0 (approx)
  int dist = (int)(duration / 58.0 + 0.5);
  Serial.print("Ultrasonic -> "); Serial.print(dist); Serial.println(" cm");
  return dist;
}

int median3(int a, int b, int c) {
  // simple median of 3
  if ((a <= b && b <= c) || (c <= b && b <= a)) return b;
  if ((b <= a && a <= c) || (c <= a && a <= b)) return a;
  return c;
}

void setRelay(int pin, bool on) {
  if (on != (relayState(pin))) {
    relayWrite(pin, on);
    Serial.print("Relay pin "); Serial.print(pin);
    Serial.print(on ? " -> ON" : " -> OFF");
    Serial.println();
  }
}

// Abstract relay write for active LOW modules: write LOW to turn ON
void relayWrite(int pin, bool on) {
  // If your module is active HIGH, flip logic: digitalWrite(pin, on ? HIGH : LOW);
  digitalWrite(pin, on ? LOW : HIGH);
}

// Get current relay state from output pin (inverted if active LOW)
bool relayState(int pin) {
  int val = digitalRead(pin);
  // If active LOW, LOW means ON
  return (val == LOW);
}

void updateLCD(float tC, float h, int dist, bool ir, bool dark) {
  lcd.clear();
  // Line 1: Temp Hum
  lcd.setCursor(0,0);
  if (!isnan(tC)) {
    lcd.print("T:");
    lcd.print(tC,1);
    lcd.print((char)223); // degree symbol
    lcd.print("C ");
  } else {
    lcd.print("T: -- ");
  }
  if (!isnan(h)) {
    lcd.print("H:");
    lcd.print(h,0);
    lcd.print("%");
  } else {
    lcd.print("H: --");
  }

  // Line 2: Distance / flags
  lcd.setCursor(0,1);
  lcd.print("D:");
  if (dist < 999) {
    lcd.print(dist);
    lcd.print("cm ");
  } else {
    lcd.print("--  ");
  }
  // show IR and dark status compactly
  lcd.print(ir ? "IR" : "--");
  lcd.print(" ");
  lcd.print(dark ? "D" : "--");
}
</div>
