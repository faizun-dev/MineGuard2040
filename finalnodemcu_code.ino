#include <LiquidCrystal_I2C.h>
#include <DHT.h>
#include <ESP8266WiFi.h>
 
//thingspeak connection
String apiKey = "api_key";     //  Enter your Write API key from ThingSpeak
 
const char *ssid =  "ssid";     // replace with your wifi ssid and wpa2 key
const char *pass =  "wifi_password";
const char* server = "api.thingspeak.com";

// Define pins
#define DHTPIN D3         // DHT11 data pin connected to NodeMCU D3
#define MQ7PIN A0         // MQ2 analog output connected to NodeMCU A0
#define TEMP_BUZZ D4      // LED for temperature warning
#define GAS_BUZZ D5       // LED for gas warning

// Define thresholds
#define TEMP_THRESHOLD 40.0   // Temperature threshold in Celsius
#define GAS_THRESHOLD 700   // Gas concentration threshold (adjust based on calibration)

// Initialize DHT sensor
#define DHTTYPE DHT11
DHT dht(DHTPIN, DHTTYPE);

// Set the LCD address to 0x27 for a 16 chars and 2 line display
LiquidCrystal_I2C lcd(0x27, 16, 2);

WiFiClient client;

void setup() {
  // Initialize serial communication
  Serial.begin(115200);
    // Turn off BUZZER INITIALLY initially
  
  
  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");
  
  // Initialize DHT sensor
  dht.begin();
  
  // Set pin modes
  pinMode(TEMP_BUZZ, OUTPUT);
  pinMode(GAS_BUZZ, OUTPUT);
  digitalWrite(TEMP_BUZZ, LOW);
  digitalWrite(GAS_BUZZ, LOW);
  

 
  //connecting to wifi
  Serial.println("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  while (WiFi.status() != WL_CONNECTED) 
    {
      delay(500);
      Serial.print(".");
    }
  Serial.println("");
  Serial.println("WiFi connected");
  
  delay(2000); // Give sensors time to initialize
  lcd.clear();
}

void loop() {
   float heartRate = 0.0;
  float weight = 0.0;
  float dust = 0.0;

  if (Serial.available()) {
    String incomingData = Serial.readStringUntil('\n');
    incomingData.trim(); // Remove \r or whitespace

    Serial.println("From Arduino: " + incomingData);

    int firstComma = incomingData.indexOf(',');
    int secondComma = incomingData.indexOf(',', firstComma + 1);

    if (firstComma > 0 && secondComma > firstComma) {
      String hrStr = incomingData.substring(0, firstComma);
      String wtStr = incomingData.substring(firstComma + 1, secondComma);
      String dustStr = incomingData.substring(secondComma + 1);

      heartRate = hrStr.toFloat();
      weight = wtStr.toFloat();
      dust = dustStr.toFloat();
    }
  }


  // Read sensors
  float temperature = dht.readTemperature();
  int gasValue = analogRead(MQ7PIN);
  int gasPPM = map(gasValue, 0, 1023, 0, 1000);

  // Sensor error check
  if (isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("DHT Error");
    delay(2000);
    return;
  }

  // LCD Display
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.print(temperature, 1);
  lcd.print(" C");
  
  lcd.setCursor(0, 1);
  lcd.print("Gas: ");
  lcd.print(gasPPM);
  lcd.print(" ppm");

  

  // ThingSpeak Upload
  if (client.connect(server, 80)) {
    String postStr = apiKey;
    postStr += "&field1=" + String(temperature);
    postStr += "&field2=" + String(weight);
    postStr += "&field3=" + String(gasPPM);
    postStr += "&field4=" + String(heartRate);
    postStr += "&field5=" + String(dust);
    postStr += "\r\n\r\n";

    client.println("POST /update HTTP/1.1");
    client.println("Host: api.thingspeak.com");
    client.println("Connection: close");
    client.println("X-THINGSPEAKAPIKEY: " + apiKey);
    client.println("Content-Type: application/x-www-form-urlencoded");
    client.println("Content-Length: " + String(postStr.length()));
    client.println();
    client.print(postStr);

    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print("°C, Gas: ");
    Serial.print(gasValue);
    Serial.println(". Sent to ThingSpeak.");
  }
  client.stop();

 // Control LED and buzzer based on thresholds
  if (temperature > TEMP_THRESHOLD) {
    digitalWrite(TEMP_BUZZ, HIGH);
  } else {
    digitalWrite(TEMP_BUZZ, LOW);
  }
  
  if (gasPPM > GAS_THRESHOLD) {
    digitalWrite(GAS_BUZZ, HIGH);
  } else {
    digitalWrite(GAS_BUZZ, LOW);
  }
  

  Serial.print("Heart Rate: ");
  Serial.print(heartRate);
  Serial.print(" bpm | Weight: ");
  Serial.print(weight);
  Serial.print(" kg | Dust: ");
  Serial.print(dust);
  Serial.println(" mg/m3");
  
  delay(2000); // ThingSpeak requires 15s between updates (minimum)
}