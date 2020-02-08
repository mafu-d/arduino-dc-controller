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
float speed = 0;
int power = 0;
float acceleration = 0.1;
float deceleration = 0.2;
int minPower = 10;
int maxPower = 255;
bool forwards = true;
bool changeForwards = true;
int lastHeartBeat = 0;
char received;
 
void setup() {
  // Initializing Serial
  Bluetooth.begin(9600);
  //inputString.reserve(4); 

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
}

void loop() {
  received = false;
  if (Bluetooth.available()) {
    received = Bluetooth.read();
  }

  if (received) {
    switch (received) {
      case 'X':
        // Emergency stop
        speed = 0;
        power = 0;
        break;
      case 'F':
        // Change direction to forwards
        changeForwards = true;
        break;
      case 'B':
        // Change direction to backwards
        changeForwards = false;
        break;
      case 'H':
        // Heartbeat
        lastHeartBeat = millis();
      default:
        // Handle power changes
        if ((int)received >= -10 && (int)received <= 10) {
          power = (int)received;
        }
    }

    // Change speed
    if (power > 0 && speed < maxPower) {
      speed += acceleration;
      if (speed < minPower) {
        speed = minPower;
      }
    }
    if (power < 0 && speed > 0) {
      speed -= deceleration;
      if (speed < minPower) {
        speed = 0;
      }
    }

    // Change direction when stopped
    if (speed === 0 && changeForwards !== forwards) {
      forwards = changeForwards;
    }

    // Cut out if no recent heartbeat
    if (millis() - lastHeartBeat > 10000) {
      speed = 0;
      power = 0;
    }

    // Set power
    digitalWrite(L298_IN1, forwards ? HIGH : LOW);
    digitalWrite(L298_IN2, forwards ? LOW : HIGH);
    analogWrite(L298_ENA, (int)speed);
  }
}
