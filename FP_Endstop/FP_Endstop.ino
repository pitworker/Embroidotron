#include <AccelStepper.h>
#include <MultiStepper.h>


/*     --PINS--     */
// MOTORS
#define X_BOUND      5
#define X_STEP       2
#define X_DIR        5
#define X_SWITCH     9

#define Y_BOUND      5
#define Y_STEP       4
#define Y_DIR        7
#define Y_SWITCH    10

#define MOTOR_ENABLE 8
#define M_INTERFACE  1

AccelStepper stepper1;
AccelStepper stepper2;
MultiStepper steppers;

// LED & BUTTON
#define LED          6
#define BUTTON       3
#define BUTTON_VAL   1 //0 for pin 2, 1 for pin 3


/*  --OTHER CONSTS--  */
#define NUM_CHARS     10

#define ARRAY_SIZE(array) ((sizeof(array))/(sizeof(array[0])))
#define BUFFER_LENGTH 10

#define X_LENGTH   27000 //ABSOLUTE
#define Y_LENGTH    9000 //ABSOLUTE
#define DIAG_CENTER 5500 //ABSOLUTE
#define X_CENTER   10500 //RELATIVE to DIAG_CENTER offset


/*  --VARS & RELATED--  */
char receivedCharsX[NUM_CHARS];
char receivedCharsY[NUM_CHARS];
bool readingX = true;
bool newData = false;
bool allPoints = false;
bool setPosition = true;

int bufferPoints[BUFFER_LENGTH];
int pointsA = 0;  // add
int pointsE = 0;  // extract
int x = 0;
int y = 0;
bool full = false;

volatile int needleFlag = 0;

long positions[2] = {0, 0};


/*  --HELPER FUNCTIONS--  */
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
  stepper1.moveTo(X_LENGTH);

  Serial.print("stepper2: ");
  Serial.println(stepper2.currentPosition());
  stepper2.move(X_LENGTH);
  
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
  stepper1.moveTo(-Y_LENGTH);

  Serial.print("stepper2: ");
  Serial.println(stepper2.currentPosition());
  stepper2.move(Y_LENGTH);
  
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
  stepper2.moveTo(-DIAG_CENTER);
  while (stepper2.run()) {
  // no endstop check needed because of dir & size of offset
  }

  stepper1.move(-X_CENTER);
  stepper2.move(-X_CENTER);
  while (stepper1.run() | stepper2.run()) {
  // no endstop check needed because of dir & size of offset
  }
  
  return 1;
}

void recvWithStartEndMarkers() {
  static bool recvInProgress = false;
  static byte ndx = 0;
  char startMarker = '<';
  char endMarker = '>';
  char rc;

  while (Serial.available() > 0 && newData == false) {
    rc = Serial.read();
    if (recvInProgress == true) {
      if (rc != endMarker && readingX) {   // read X
        receivedCharsX[ndx] = rc;
        ndx++;
        if (ndx >= NUM_CHARS) {
         ndx = NUM_CHARS - 1;
        }
      } else if (rc != endMarker) {        // read y
        receivedCharsY[ndx] = rc;
        ndx++;
        if (ndx >= NUM_CHARS) {
          ndx = NUM_CHARS - 1;
        }
      } else {
        receivedCharsY[ndx] = '\0';
        recvInProgress = false;
        ndx = 0;
        newData = true;
        readingX = true;
      }
      if (rc == ',') {
        receivedCharsX[ndx] = '\0';
        readingX = false;
        ndx = 0;
      }
    } else if (rc == startMarker) {
      recvInProgress = true;
    }
  }
  x = atoi(receivedCharsX);
  y = atoi(receivedCharsY);
}

void replyToPython() {
  if (newData == true && !allPoints) {
    Serial.print("<*-*This just in from PC: __");
    Serial.print(receivedCharsX);
    if (receivedCharsX[0] != 'S') {
      bufferPoints[pointsA] = x;
      pointsA += 1;
      bufferPoints[pointsA] = y;
      pointsA += 1;
      pointsA = pointsA % BUFFER_LENGTH;
      if (pointsA == pointsE) {
        full = true;
      }
      Serial.print(receivedCharsY);
      Serial.print("__; (x,y):");
      Serial.print(x);
      Serial.print(",");
      Serial.print(y);
    } else {
      Serial.print("__;");          
    }
    Serial.print(", ");
    Serial.print("points_a = ");
    Serial.print(pointsA);
    Serial.print(", points_e = ");
    Serial.print(pointsE);
    if (full) {
      Serial.print(", FULL_BUFFER");
    } else {
      Serial.print(", NOT_FULL");
    }
    Serial.print(", len(buffer_points) = ");
    Serial.print(ARRAY_SIZE(bufferPoints));
    Serial.print("\n\t");
    for (int i = 0; i < ARRAY_SIZE(bufferPoints); i ++) {
      if(i == pointsA) {
        Serial.print("[");
        Serial.print(bufferPoints[i]);
        Serial.print("], ");
      } else if (i == pointsE) {
        Serial.print("(");
        Serial.print(bufferPoints[i]);
        Serial.print("), ");
      } else {
        Serial.print(bufferPoints[i]);
        Serial.print(", ");
      }
    }
    Serial.print("\t*-*>");
    newData = false;
  }
}


/*  --ARDUINO FUNCTIONS--  */
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
  stepper1.setMaxSpeed(12800);
  steppers.addStepper(stepper1);
  
  stepper2 = AccelStepper(M_INTERFACE, Y_STEP, Y_DIR);
  stepper2.setMaxSpeed(12800);
  steppers.addStepper(stepper2);

  Serial.println("Motors initialized, calibrating home");

  int foundHome = calibrateHome();

  if (foundHome < 0) {
    Serial.println("ERROR! Home not calibrated");
    Serial.println("<Arduino not ready>");
    return;
  }
  Serial.println("Home calibrated");
  
  int centered = center();
  
  if (centered < 0) {
    Serial.println("ERROR! Unable to center");
    Serial.println("<Arduino not ready>");
    return;
  }
  Serial.println("Centered");

  // RESET MAX SPEEDS AFTER CALIBRATION
  stepper1.setMaxSpeed(800);
  stepper2.setMaxSpeed(800);
  
  Serial.println("<Arduino is ready>");
}

void loop() {
  // endstops reached, adequate response not implemented
  if (stopReached(Y_SWITCH)) {
    Serial.println("Y ENDSTOP REACHED");
  }
  if (stopReached(X_SWITCH)) {
    Serial.println("X ENDSTOP REACHED");
  }

  // Original fabric positioner code with few alterations
  if (needleFlag && setPosition) {
    setPosition = false;
    positions[0] = long(bufferPoints[pointsE] * 1);
    pointsE += 1;
    positions[1] = long(bufferPoints[pointsE] * 1);
    pointsE += 1;
    digitalWrite(LED, HIGH);
    stepper1.setCurrentPosition(positions[0]);
    stepper2.setCurrentPosition(positions[1]);
    digitalWrite(LED, LOW);

    pointsE = pointsE % BUFFER_LENGTH;
    Serial.print("<Set Position to ");
    Serial.print(positions[0]);
    Serial.print(", ");
    Serial.print(positions[1]);
    Serial.print(">");
    full = false;
    needleFlag = 0;
  }
  else if (needleFlag && !allPoints) {
    positions[0] = long(bufferPoints[pointsE] * 1);
    pointsE += 1;
    positions[1] = long(bufferPoints[pointsE] * 1);
    pointsE += 1;
    digitalWrite(LED, HIGH);
    steppers.moveTo(positions);
    steppers.runSpeedToPosition(); 
    // Blocks until all are in position
    digitalWrite(LED, LOW);

    pointsE = pointsE % BUFFER_LENGTH;
    Serial.print("<MOVED to ");
    Serial.print(positions[0]);
    Serial.print(", ");
    Serial.print(positions[1]);
    Serial.print(">");
    full = false;
    needleFlag = 0;

    if (bufferPoints[pointsE] == -1000) {
      Serial.print("<DONE>");
      allPoints = true;
    }
  }
  recvWithStartEndMarkers();
  replyToPython();
}
