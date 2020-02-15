// (C) Matthew Dawkins 2020. All rights reserved.
// Some code adapted from arduinomodelrailway.com

#include <SoftwareSerial.h>

// SOFTWARE SERIAL
SoftwareSerial Bluetooth(12, 13); // RX, TX

// L298
#define L298_ENA 9
#define L298_IN1 6
#define L298_IN2 5

// VARIABLES //
float locoSpeed = 0.0;
int locoPower = 0;
float acceleration = 0.15;
float deceleration = 0.2;
unsigned int minPower = 20;
unsigned int maxPower = 255;
bool forwards = true;
bool changeForwards = true;
unsigned int lastHeartBeat = 0;
String received = "";
 
void setup() {
  // Initializing Serial
  Bluetooth.begin(9600);
  Serial.begin(4800);
  Serial.println("Starting");

  // Initializing Motor-Driver
  pinMode(L298_ENA, OUTPUT); 
  pinMode(L298_IN1, OUTPUT); 
  pinMode(L298_IN2, OUTPUT);

  // Set PWM frequency for D9 & D10
  // Timer 1 divisor to 256 for PWM frequency of 122.55 Hz
  TCCR1B = TCCR1B & B11111000 | B00000100; 

  // Set default direction to FORWARD
  digitalWrite(L298_IN1, HIGH);
  digitalWrite(L298_IN2, LOW);

  Serial.println("Ready");
}

// Process the Bluetooth buffer and get the full instruction
String getInstruction() {
  String instruction = "";
  bool complete = false;
  char buffer;
  int cycles = 0;
  while (!complete && cycles < 8) {
    delay(10); // Allow time for buffer to refill
    buffer = Bluetooth.read();
    if (buffer == '>') {
      Serial.print("Received: ");
      Serial.println(instruction);
      return instruction;
    }
    instruction += buffer;
    cycles++;
  }
}

// Update the loco state depending on the instruction sent
void handleInstruction(String instruction) {
  if (instruction == "X") {
    // Emergency stop
    Serial.println("Emergency stop");
    locoSpeed = 0;
    locoPower = 0;
    return;
  }
  if (instruction == "F") {
    // Change direction to forwards
    Serial.println("Forwards");
    changeForwards = true;
    return;
  }
  if (instruction == "B") {
    // Change direction to backwards
    Serial.println("Backwards");
    changeForwards = false;
    return;
  }
  if (received == "H") {
    // Heartbeat
    lastHeartBeat = millis() / 1000;
    Serial.print("Heartbeat ");
    Serial.println(lastHeartBeat);
    Serial.print("Speed: ");
    Serial.println(locoSpeed);
    Serial.print("Power: ");
    Serial.println(locoPower);
    return;
  }
  int in2 = instruction.toInt();
  if (in2 >= -10 && in2 <= 10) {
    // Handle desiredPower changes
    locoPower = in2;
    return;
  }
}

void loop() {
  // Check whether there is an incoming instruction
  received = "";
  if (Bluetooth.available() && Bluetooth.read() == '<') {
    received = getInstruction();
  }

  // If there is an instruction, do it
  if (received != "") {
    handleInstruction(received);
  }
  
  // Change speed of loco
  if (locoPower > 0 && locoSpeed < maxPower) {
    locoSpeed += acceleration * locoPower;
    if (locoSpeed < minPower) {
      locoSpeed = minPower;
    }
  }
  if (locoPower < 0 && locoSpeed > minPower) {
    locoSpeed += deceleration * locoPower;
    if (locoSpeed < minPower) {
      locoSpeed = 0;
    }
  }

  // Change direction when stopped
  if (locoSpeed == 0 && changeForwards != forwards) {
    forwards = changeForwards;
    Serial.println("Changed direction");
  }

  // Cut out if no recent heartbeat
  if ((millis() / 1000 - lastHeartBeat > 10) && (locoSpeed > 0)) {
    locoSpeed = 0;
    locoPower = 0;
    Serial.print("Auto cutout: ");
    Serial.println(lastHeartBeat);
  }

  // Set desiredPower
  if (forwards) {
    digitalWrite(L298_IN1, HIGH);
    digitalWrite(L298_IN2, LOW);
  analogWrite(L298_ENA, 255 - locoSpeed);
  } else {
    digitalWrite(L298_IN1, LOW);
    digitalWrite(L298_IN2, HIGH);
      analogWrite(L298_ENA, locoSpeed);

  }
//  digitalWrite(L298_IN1, forwards ? HIGH : LOW);
//  digitalWrite(L298_IN2, forwards ? LOW : HIGH);

  // Pause a bit
  delay(100);
}
