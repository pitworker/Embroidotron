#include <AccelStepper.h>
#include <MultiStepper.h>

#define X_BOUND      5
#define X_STEP       2
#define X_DIR        5
#define X_SWITCH     9

#define Y_BOUND      5
#define Y_STEP       4
#define Y_DIR        7
#define Y_SWITCH    10

#define LED          6
#define BUTTON       3
#define BUTTON_VAL   1 //0 for pin 2, 1 for pin 3

#define MOTOR_ENABLE 8
#define M_INTERFACE  1

AccelStepper stepper1;
AccelStepper stepper2;
MultiStepper steppers;

bool stopReached(int pin) {
  return (digitalRead(pin) == 0);
}

void move(long a, long b) {
  stepper1.move(a);
  stepper2.move(b);
  while (stepper1.run() | stepper2.run()) {
    if (stopReached(X_SWITCH) || stopReached(Y_SWITCH)) {
      stepper1.stop();
      stepper2.stop();    
      return;
    }
  }
}

int homeX() {
  Serial.print("stepper1: ");
  Serial.println(stepper1.currentPosition());
  stepper1.moveTo(27000);

  Serial.print("stepper2: ");
  Serial.println(stepper2.currentPosition());
  stepper2.move(27000);
  
  while (stepper1.run() | stepper2.run()) {
    if (stopReached(X_SWITCH)) {     
      stepper1.stop();
      stepper2.stop();
      break;
    }
  }
  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);
  return 1;
}

int homeY() {
  Serial.print("stepper1: ");
  Serial.println(stepper1.currentPosition());
  stepper1.moveTo(-9000);

  Serial.print("stepper2: ");
  Serial.println(stepper2.currentPosition());
  stepper2.move(9000);
  
  while (stepper1.run() | stepper2.run()) {
    if (stopReached(Y_SWITCH)) {
      stepper1.stop();
      stepper2.stop();    
      break;
    }
  }
  stepper1.setCurrentPosition(0);
  stepper2.setCurrentPosition(0);
  return 1;
}

int calibrateHome() {
  stepper1.setMaxSpeed(12800);
  stepper2.setMaxSpeed(12800);
  stepper1.setAcceleration(800);
  stepper2.setAcceleration(800);
  
  homeX();
  homeY();

  return 1;
}

int center() {
  stepper2.moveTo(-5500);
  while (stepper2.run()) {
    /*if (stopReached(Y_SWITCH) || stopReached(X_SWITCH)) {
      stepper2.stop();    
      return -1;
    }*/
  }

  stepper1.move(-10500);
  stepper2.move(-10500);
  while (stepper1.run() | stepper2.run());
  
  return 1;
}

void setup() {
  Serial.begin(115200);

  // LED Interrupt
  pinMode(LED, OUTPUT);
  digitalWrite(LED, LOW);

  // Button Interrupt
  /*
  pinMode(BUTTON, INPUT);
  attachInterrupt(BUTTON_VAL, 
  */

  // Endstops
  pinMode(X_SWITCH, INPUT_PULLUP);
  pinMode(Y_SWITCH, INPUT_PULLUP);

  // Motors
  stepper1 = AccelStepper(M_INTERFACE, X_STEP, X_DIR);
  stepper1.setMaxSpeed(800);
  steppers.addStepper(stepper1);
  
  stepper2 = AccelStepper(M_INTERFACE, Y_STEP, Y_DIR);
  stepper2.setMaxSpeed(800);
  steppers.addStepper(stepper2);

  Serial.println("Motors initialized, calibrating home");

  int foundHome = calibrateHome();

  if (foundHome >= 0) {
    Serial.println("Home calibrated");
    int centered = center();
    if (centered >= 0) {  
      Serial.println("Centered");
      Serial.println("<Arduino is ready>");
    } else {
      Serial.println("ERROR! Unable to center");
    }
  } else {
    Serial.println("ERROR! Home not calibrated");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  if (digitalRead(Y_SWITCH) == 0) {
    Serial.println("Y ENDSTOP REACHED");
  }
  if (digitalRead(X_SWITCH) == 0) {
    Serial.println("X ENDSTOP REACHED");
  }
}
