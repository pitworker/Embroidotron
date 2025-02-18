#include <Encoder.h>

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder myEnc(3, 2);
//   avoid using pins with LEDs attached

void setup() {
  Serial.begin(115200);
  Serial.println("Basic Encoder Test:");
  pinMode(8, OUTPUT);
  pinMode(13, OUTPUT);
}

long oldPosition  = -999;

void loop() {
  long newPosition = myEnc.read();
  Serial.println(newPosition);
  if (newPosition > 0) {
    digitalWrite(8, HIGH);
    digitalWrite(13, HIGH);
  } else {
    digitalWrite(8, LOW);
    digitalWrite(13, LOW);
  }
}
