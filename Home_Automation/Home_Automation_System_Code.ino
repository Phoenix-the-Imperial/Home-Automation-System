//---------------------------------------------------------------------------
// Written by Rishav Chakraborty, Roll No. 21AG30030, Section 6-A, IIT KGP.
//---------------------------------------------------------------------------

//-------- Include the Necessary Libraries -------
#include <DHT.h>
#include <MQ2.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal.h>

//-------- DHT11 ---------------------------------
#define dht_type DHT11
#define dht_pin 6
float humidity;
float temp_C, temp_F;
float heat_index_C, heat_index_F;
float level_lpg, level_co, level_smoke;
const float max_lpg   = 200;  // ppm
const float max_co    = 10 ;  // ppm
const float max_smoke = 300;  // ppm

//-------- MQ2 -----------------------------------
#define mq2_pin A0
const bool print_values_to_serial = false;
const unsigned int mq2_warm_up_delay_time = 20000;
const unsigned int sensor_measurement_delay_time = 3000;

//-------- Buzzer --------------------------------
#define buzzer_pin 8

//-------- HC-05 ---------------------------------
#define RX 0
#define TX 1
String input_voice;
unsigned short int index;
const unsigned int bluetooth_delay_time = 10;

//-------- Liquid-Crystal Display ----------------
#define RS 13
#define E  12
#define D4 16
#define D5 17
#define D6 18
#define D7 19

//-------- Relay and Appliances ------------------
const unsigned short int num_channels = 4;
const unsigned short int switch_map[] = {5, 4, 3, 2};
#define fan             switch_map[0]
#define room_light      switch_map[1]
#define kitchen_light   switch_map[2]
#define night_bulb      switch_map[3]

//-------- Voice Commands ------------------------
const String voice_cmds[] = {
  "turn on fan",  "turn on room light",  "turn on kitchen light",  "turn on night bulb",
  "turn off fan", "turn off room light", "turn off kitchen light", "turn off night bulb",
  "turn on power", "turn off power"
};

//-------- Record time-elapsed for Multi-tasking -
unsigned int current_time = millis();
unsigned int previous_measurement_time = current_time;
unsigned int previous_bluetooth_time = current_time;

//-------- Define the Objects Classes ------------
DHT dht(dht_pin, dht_type);
MQ2 mq2(mq2_pin);
SoftwareSerial Bluetooth(RX, TX);
LiquidCrystal lcd(RS, E, D4, D5, D6, D7);

//-------- setup() function ----------------------
void setup() {
  Serial.begin(9600);

  // Set the built-in LED as OUTPUT
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // Set the DHT11, MQ2 Pins as INPUT and the Buzzer Pin as OUTPUT
  pinMode(dht_pin, INPUT);
  pinMode(mq2_pin, INPUT);
  pinMode(buzzer_pin, OUTPUT);

  // Set the Appliances as OUTPUT and turn them OFF
  pinMode(fan, OUTPUT);
  analogWrite(fan, 0);

  // Initialize the dht, mq2, Bluetooth and lcd instances of Object Classes
  dht.begin(9600);
  mq2.begin();
  Bluetooth.begin(9600);
  lcd.begin(16, 2);

  // Wait for the MQ2 Gas Sensor to warm-up
  delay(mq2_warm_up_delay_time);

  // Turn on the built-in LED to indicate usability
  digitalWrite(LED_BUILTIN, HIGH);
}

//-------- loop() function -----------------------
void loop() {
  current_time = millis();
  if (current_time - previous_measurement_time > sensor_measurement_delay_time) {
    previous_measurement_time = current_time;
    if (make_measurement_dht11() == -1) return;
    make_measurement_mq2();
  }
  if (current_time - previous_bluetooth_time > bluetooth_delay_time) {
    previous_bluetooth_time = current_time;
    control_appliances();
  }
}

//-------- Make Measurements with the DHT11 Sensor ----------
int make_measurement_dht11()  {

  // Read the Humidity and Temperature Values
  humidity = dht.readHumidity();
  temp_C = dht.readTemperature();
  temp_F = dht.readTemperature(true);

  // Return if any of the readings fail
  if (isnan(humidity) || isnan(temp_C) || isnan(temp_F)) {
    return -1;
  }

  // Compute the Heat Indices for Celsius and Fahrenheit Units
  heat_index_C = dht.computeHeatIndex(temp_C, humidity, false);
  heat_index_F = dht.computeHeatIndex(temp_F, humidity, true);

  // Print the Results on the Serial Monitor
  //  Serial.print("Temperature: ");
  lcd.setCursor(0, 0);
  lcd.print("Temp: ");
  lcd.setCursor(6, 0);
  lcd.print(int(temp_C));
  lcd.setCursor(8, 0);
  lcd.print((char)223);
  lcd.setCursor(9, 0);
  lcd.print("C|");
  lcd.setCursor(11, 0);
  lcd.print(int(temp_F));
  lcd.setCursor(13, 0);
  lcd.print((char)223);
  lcd.setCursor(14, 0);
  lcd.print("F");
  lcd.setCursor(0, 1);
  lcd.print("Humidity: ");
  lcd.setCursor(10, 1);
  lcd.print(humidity);
  if (humidity == 100) {
    lcd.setCursor(13, 1);
    lcd.print("%  ");
  }
  else {
    lcd.setCursor(12, 1);
    lcd.print("%   ");
  }
  return 0;
}

//-------- Make Measurements with the MQ2 Gas Sensor --------
void make_measurement_mq2() {
jump_label:
  if (print_values_to_serial) {
    float *ptr_values = mq2.read(true);
  }
  if (mq2.readLPG() > max_lpg || mq2.readCO() > max_co || mq2.readSmoke() > max_smoke) {
    for (int i = 0; i < 150; i++) {
      digitalWrite(buzzer_pin, HIGH);
      delay(10);
      digitalWrite(buzzer_pin, LOW);
      delay(10);
    }
    goto jump_label;
  }
  return;
}

//-------- Control the Appliances using Voice Command recieved from the HC-05 Bluetooth Module --------
void control_appliances() {
  while (Bluetooth.available() > 0) {
    input_voice = Bluetooth.readString();
  }
  if (input_voice != "") {
    Serial.print("Recieved Voice Command: ");
    Serial.println(input_voice);
  }
  if (input_voice == "turn fan on") {
    if (analogRead(fan) == 0) {
      analogWrite(fan, 255);
    }
  }
  else if (input_voice == "turn fan off") {
    if (analogRead(fan) > 0) {
      analogWrite(fan, 0);
    }
  }
  for (index = 0; index <= 2 * num_channels + 1; index++) {
    if (index == 2 * num_channels + 1) {
      index = -1;
      break;
    }
    if (input_voice == switch_map[index]) {
      break;
    }
  }
  if (index >= 0 && index <= num_channels - 1) {
    if (digitalRead(switch_map[index]) == LOW) digitalWrite(switch_map[index], HIGH);
  }
  else if (index >= num_channels && index <= (2 * num_channels - 2)) {
    if (digitalRead(switch_map[index - num_channels]) == HIGH) digitalWrite(switch_map[index - num_channels], LOW);
  }
  else if (index == (2 * num_channels - 1)) {
    for (int i : switch_map) {
      if (digitalRead(switch_map[i]) == LOW) digitalWrite(switch_map[i], HIGH);
    }
  }
  else if (index == (2 * num_channels)) {
    for (int i : switch_map) {
      if (digitalRead(switch_map[i]) == HIGH) digitalWrite(switch_map[i], LOW);
    }
  }
  input_voice = "";
}
