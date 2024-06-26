// Include the Necessary Libraries
#include <SoftwareSerial.h>
#include <DHT.h>
#include <MQ2.h>
#include <LiquidCrystal.h>

// DHT11 Sensor
#define dht_type DHT11
#define dht_pin 6
float humidity;
float temp_C, temp_F;
float heat_index_C, heat_index_F;

// MQ2 Gas Sensor
#define mq2_pin A0
float level_lpg, level_co, level_smoke;
const float max_lpg   = 200.00;  // ppm
const float max_co    = 10.00;   // ppm
const float max_smoke = 300;     // ppm

// Buzzer
#define buzzer_pin 8

// Bluetooth Module
#define RX 0
#define TX 1
int input_value;

// 4-Channel Relay
int switch_map[] = {5, 4, 3, 2};
#define Fan switch_map[0]
#define Room_Light switch_map[1]
#define Kitchen_Light switch_map[2]
#define Night_Bulb switch_map[3]

// 16x2 LCD
#define RS 13
#define E  12
#define D4 16
#define D5 17
#define D6 18
#define D7 19

// Delay-Times
#define delay_time              10
#define mq2_warm_up_delay_time  20000
#define buzzer_delay_time       250

// Voice Automation
String input_voice;

// Initialization of the Sensors and Modules
SoftwareSerial Bluetooth(RX, TX);         // Initialize the SoftwareSerial Object: Bluetooth Class
DHT dht(dht_pin, dht_type);               // Initialize the DHT Object: dht Class
MQ2 mq2(mq2_pin);                         // Initialize the MQ2 Object: mq2 Class
LiquidCrystal lcd(RS, E, D4, D5, D6, D7); // Initialize the LiquidCrystal Object: lcd Class

// Setup
void setup() {
  Serial.begin(9600);     // Begin Serial Communication with the Arduino
  Bluetooth.begin(9600);  // Begin Serial Communication with the Bluetooth Module
  dht.begin(9600);        // Begin Serial Communication with the DHT11 Module
  mq2.begin();            // Begin Serial Communication with the MQ2 Module: Calibrate the Device
  lcd.begin(16, 2);       // Begin Serial Communication with the LCD Module: Use a 16x2 Matrix
  lcd.clear();

  // Set the Built-In LED as OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);
  
  //Set the DHT11 Temperature and Humidity Sensor as INPUT
  pinMode(dht_pin, INPUT);

  // Set the MQ2 Gas Sensor Pin as INPUT
  pinMode(mq2_pin, INPUT);

  //Set the Buzzer Pin as OUTPUT
  pinMode(buzzer_pin, OUTPUT);

  // Set the Relay Pins as Output
  for (int pin_number : switch_map) {
    pinMode(pin_number, OUTPUT);
    pin_number == Fan ? analogWrite(pin_number, 255) : digitalWrite(pin_number, HIGH);
  }

  // Wait for the MQ2 Sensor to Warm Up
  delay(mq2_warm_up_delay_time);
  digitalWrite(LED_BUILTIN, HIGH);
}

// Loop
void loop() {

  // Read the Temperature and Humidity values
  humidity = dht.readHumidity();
  temp_C = dht.readTemperature();
  temp_F = dht.readTemperature(true);
  if (isnan(humidity) || isnan(temp_C) || isnan(temp_F)) {
    return;  // Return if reading fails
  }
  heat_index_C = dht.computeHeatIndex(temp_C, humidity, false);
  heat_index_F = dht.computeHeatIndex(temp_F, humidity, true);
  // Print the Temperature and Humidity Values on the LCD Screen
  // Celsius
  lcd.setCursor(0, 0);
  lcd.print(temp_C);
  lcd.setCursor(5, 0);
  lcd.print("\xB0");
  lcd.setCursor(6, 0);
  lcd.print("C|");
  // Fahrenheit
  lcd.setCursor(9, 0);
  lcd.print(temp_F);
  lcd.setCursor(14, 0);
  lcd.print("\xB0");
  lcd.print("F");
  // Humidity
  lcd.setCursor(0, 1);
  lcd.print(humidity);
  if (humidity == 100) {
    lcd.setCursor(0, 4);
    lcd.print("%           ");
  }
  else {
    lcd.setCursor(0, 3);
    lcd.print("%            ");
  }

  // Read the LPG, CO, and Smoke Levels
  // Get the array containing the Readings as { LPG, CO, Smoke }
  float *ptr_values = mq2.read(false);  // Set to false, because we do not want to print the values to the Serial Monitor
  level_lpg   = mq2.readLPG();
  level_co    = mq2.readCO();
  level_smoke = mq2.readSmoke();
  // Turn on the Buzzer if measured Air-Quality is hazardous
  while (level_lpg > max_lpg || level_co > max_co || level_smoke > max_smoke) {
    digitalWrite(buzzer_pin, HIGH);
    delay(buzzer_delay_time);
    digitalWrite(buzzer_pin, LOW);
    delay(buzzer_delay_time);
  }

  // Get Voice-Input Bluetooth Module
  while (Serial.available() > 0) {
    input_value = Serial.read();
    if (input_value == '#') break;
    input_voice += input_value;
    delay(delay_time);
  }

  // Control the Appliances based on Voice-Input
  control_appliances(input_voice);

  // Print the values in the app through the Bluetooth Module
  bluetooth_print();
}

void control_appliances(String input_voice) {
  if (input_voice.length() > 0) {
    if (input_voice == "fan on") {
      analogWrite(Fan, 0);
    }
    else if (input_voice == "fan off") {
      analogWrite(Fan, 255);
    }
    else if (input_voice == "room light on") {
      digitalWrite(Room_Light, LOW);
    }
    else if (input_voice == "room light off") {
      digitalWrite(Room_Light, HIGH);
    }
    else if (input_voice == "kitchen light on") {
      digitalWrite(Kitchen_Light, LOW);
    }
    else if (input_voice == "kitchen light off") {
      digitalWrite(Kitchen_Light, HIGH);
    }
    else if (input_voice == "night bulb on") {
      digitalWrite(Night_Bulb, LOW);
    }
    else if (input_voice == "night bulb off") {
      digitalWrite(Night_Bulb, HIGH);
    }
  }
}
void bluetooth_print() {
  for (int pin_number : switch_map) {
    if (pin_number == Fan) {
      if (analogRead(pin_number) == 0) {
        Bluetooth.print("Appliance ");
        Bluetooth.print(pin_number);
        Bluetooth.print("ON\n");
      }
      else if (analogRead(pin_number) == 255) {
        Bluetooth.print("Appliance ");
        Bluetooth.print(pin_number);
        Bluetooth.print("OFF\n");
      }
    }
    else {
      if (digitalRead(pin_number) == 0) {
        Bluetooth.print("Appliance ");
        Bluetooth.print(pin_number);
        Bluetooth.print("ON\n");
      }
      else {
        Bluetooth.print("Appliance ");
        Bluetooth.print(pin_number);
        Bluetooth.print("OFF\n");
      }
    }
  }
}
