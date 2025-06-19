#include <HX711.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <PulseSensorPlayground.h>
#include <GP2YDustSensor.h>

// ========== Load Cell & Pulse Sensor Setup (Original Code) ==========
// HX711 Pins
#define LOADCELL_DOUT_PIN  4
#define LOADCELL_SCK_PIN   5

// LCD Configuration (I2C Address: 0x27, 16x2)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// LED Pin (for load cell alert)
#define LED_PIN  2

// Calibration Factor (Adjust based on known weight)
float calibration_factor = 976.0;

HX711 scale;

// PulseSensor Setup
const int PulseWire = A1;       // PulseSensor PURPLE wire on Analog Pin 0
const int PulseLED = LED_BUILTIN; // Built-in LED (usually pin 13)
const int BPM_LED = 12;       // External LED on Pin 12 (for BPM > 140)
int Threshold = 519;          // Adjust if needed (default: 520)

PulseSensorPlayground pulseSensor;  // PulseSensor object

// ========== Dust Sensor Setup (Original Code) ==========
const uint8_t SHARP_LED_PIN = 3;   // LED control pin
const uint8_t SHARP_VO_PIN = A0;   // Analog pin for sensor output
const uint8_t RELAY_PIN = 7;       // Relay control pin

GP2YDustSensor dustSensor(GP2YDustSensorType::GP2Y1014AU0F, SHARP_LED_PIN, SHARP_VO_PIN, 10);

// ========== Setup Function ==========
void setup() {
  Serial.begin(115200);
  
  // Initialize LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Welcome to");
  lcd.setCursor(0, 1);
  lcd.print("MineGuard 2040");
  delay(2000); // Wait 2 seconds before showing "Remove Weight..."
  lcd.clear();
  
  // Initialize LED (for load cell)
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);
  
  // Initialize Load Cell
  scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
  scale.set_scale(calibration_factor);
  
  // Tare the scale (remove any initial offset)
  lcd.setCursor(0, 1);
  lcd.print("Remove Weight...");
  delay(2000); // Wait 2 sec to ensure no weight is placed
  scale.tare(); // Reset to zero
  lcd.clear();
  lcd.setCursor(0, 1);
  lcd.print("Ready to Measure");
  delay(1000);
  lcd.clear();

  // Initialize PulseSensor
  pulseSensor.analogInput(PulseWire);   
  pulseSensor.blinkOnPulse(PulseLED);  // Blink built-in LED with heartbeat
  pulseSensor.setThreshold(Threshold);   
  pinMode(BPM_LED, OUTPUT);           // LED for high BPM
  digitalWrite(BPM_LED, LOW);         // Start with LED off

  if (pulseSensor.begin()) {
      
  }

  // Initialize Dust Sensor (Original Code)
  dustSensor.setBaseline(0.95); // Adjust based on your "no dust" readings
  dustSensor.begin();

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, HIGH); // Make sure relay is OFF initially
}

// ========== Main Loop ==========
void loop() {
  // --- Load Cell Section (UNCHANGED) ---
  float weight = scale.get_units(3); // Read weight (average of 3 readings)

  lcd.setCursor(0, 0);
  lcd.print("W:");
  lcd.print(weight, 1);
  lcd.print("g  ");

  if (weight > 500.0) {
    digitalWrite(LED_PIN, HIGH); // Turn on LED
    lcd.setCursor(0, 1);
    lcd.print("ALERT: >500g!");
  } else {
    digitalWrite(LED_PIN, LOW); // Turn off LED
    // Don't clear this line completely as we might display other info
  }

  static int myBPM = 0;  // Declare outside the if block and retain last value
  // --- Pulse Sensor Section (UNCHANGED) ---
  myBPM = pulseSensor.getBeatsPerMinute(); 

  // Control BPM LED (Pin 12)
  if (myBPM > 140) {
    digitalWrite(BPM_LED, HIGH);
    lcd.setCursor(8, 0);
    lcd.print("REST     ");
  } else {
    digitalWrite(BPM_LED, LOW);
    lcd.setCursor(8, 0);
    lcd.print("B:");
    lcd.print(myBPM);
    lcd.print("   "); // Clear remaining chars
  }

  // --- Dust Sensor Section (UNCHANGED) ---
  float dust_ugm3 = dustSensor.getDustDensity();
  float average_ugm3 = dustSensor.getRunningAverage();

  float dust_mgm3 = dust_ugm3 / 1000.0;           // Convert to mg/m³
  float average_mgm3 = average_ugm3 / 1000.0;     // Convert to mg/m³

  // Check and control the relay
  if (dust_mgm3 > 0.35) {
    digitalWrite(RELAY_PIN, LOW);  // Turn ON the relay (pump ON)
  } else {
    digitalWrite(RELAY_PIN, HIGH);   // Turn OFF the relay (pump OFF)
  }

  // Display dust info on LCD second line when not showing weight alert
  if (weight <= 500.0) {
    lcd.setCursor(0, 1);
    if (dust_mgm3 > 0.35) {
      lcd.print("High dust:");
       lcd.print(average_mgm3, 2); // Pad with spaces to clear old text
    } else {
      lcd.print("Dust:");
      lcd.print(average_mgm3, 2);
      lcd.print("mg/m3   "); // Clear remaining characters
    }
  }

  // Serial data
  Serial.print(myBPM);
  Serial.print(",");
  Serial.print(weight, 1);
  Serial.print(",");
  Serial.print(average_mgm3, 2);
  Serial.println();

  delay(2000); // Maintain original timing from pulse/load cell code
}





