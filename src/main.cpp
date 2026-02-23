#include <Arduino.h>

// put function declarations here:
int myFunction(int, int);

void setup() {
  // put your setup code here, to run once:
  Serial.begin(19200);
  int result = myFunction(2, 3);
  Serial.print(result);
}

void loop() {
  Serial.println("Essai avec un s");
  // put your main code here, to run repeatedly:
  delay(1000);
}

// put function definitions here:
int myFunction(int x, int y) {
  return x + y;
}