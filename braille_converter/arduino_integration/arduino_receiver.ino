/*
 * Arduino Braille Receiver
 * 
 * Receives braille dot patterns from Python script over serial
 * and controls braille cell pins/motors.
 * 
 * Protocol: "DOTS:1,2,3\n" or "DOTS:NONE\n"
 * Baud Rate: 115200
 */

// Pin configuration for 6-dot braille cell
// Adjust these pin numbers based on your hardware setup
const int DOT_PINS[6] = {2, 3, 4, 5, 6, 7};  // Pins for dots 1-6

// Or if using shift register or other method:
// const int LATCH_PIN = 8;
// const int CLOCK_PIN = 12;
// const int DATA_PIN = 11;

const int BAUD_RATE = 115200;
String inputBuffer = "";

void setup() {
  // Initialize serial communication
  Serial.begin(BAUD_RATE);
  
  // Initialize dot pins as outputs
  for (int i = 0; i < 6; i++) {
    pinMode(DOT_PINS[i], OUTPUT);
    digitalWrite(DOT_PINS[i], LOW);
  }
  
  // Send ready message
  Serial.println("READY:Arduino Braille Receiver");
  Serial.flush();
  
  // Optional: Flash all dots to indicate ready
  flashAllDots();
}

void loop() {
  // Read incoming serial data
  while (Serial.available() > 0) {
    char c = Serial.read();
    
    if (c == '\n') {
      // Process complete message
      processMessage(inputBuffer);
      inputBuffer = "";
    } else {
      inputBuffer += c;
    }
  }
}

void processMessage(String message) {
  message.trim();
  
  // Check if message starts with "DOTS:"
  if (message.startsWith("DOTS:")) {
    String dotsStr = message.substring(5);  // Remove "DOTS:" prefix
    
    if (dotsStr == "NONE" || dotsStr == "") {
      // Clear all dots (space character)
      clearAllDots();
      Serial.println("ACK:CLEARED");
    } else {
      // Parse dot numbers and activate them
      activateDots(dotsStr);
      Serial.println("ACK:DOTS:" + dotsStr);
    }
  } else {
    Serial.println("ERROR:Unknown command");
  }
}

void activateDots(String dotsStr) {
  // First, clear all dots
  clearAllDots();
  
  // Parse comma-separated dot numbers
  int startIdx = 0;
  int commaIdx = dotsStr.indexOf(',');
  
  while (true) {
    String dotStr;
    
    if (commaIdx == -1) {
      // Last dot
      dotStr = dotsStr.substring(startIdx);
      if (dotStr.length() == 0) break;
    } else {
      dotStr = dotsStr.substring(startIdx, commaIdx);
    }
    
    dotStr.trim();
    int dotNum = dotStr.toInt();
    
    // Activate the dot (1-6)
    if (dotNum >= 1 && dotNum <= 6) {
      activateDot(dotNum);
    }
    
    if (commaIdx == -1) break;
    
    startIdx = commaIdx + 1;
    commaIdx = dotsStr.indexOf(',', startIdx);
  }
}

void activateDot(int dotNum) {
  // Activate single dot (1-6)
  // dotNum is 1-indexed, array is 0-indexed
  
  if (dotNum < 1 || dotNum > 6) return;
  
  int pin = DOT_PINS[dotNum - 1];
  digitalWrite(pin, HIGH);
  
  // Optional: Debug output
  // Serial.print("Activated dot ");
  // Serial.println(dotNum);
}

void clearAllDots() {
  // Turn off all dots
  for (int i = 0; i < 6; i++) {
    digitalWrite(DOT_PINS[i], LOW);
  }
}

void flashAllDots() {
  // Flash all dots as startup indicator
  for (int repeat = 0; repeat < 3; repeat++) {
    for (int i = 0; i < 6; i++) {
      digitalWrite(DOT_PINS[i], HIGH);
    }
    delay(200);
    
    for (int i = 0; i < 6; i++) {
      digitalWrite(DOT_PINS[i], LOW);
    }
    delay(200);
  }
}

// Alternative: If using shift register for controlling multiple cells
/*
void activateDotShiftRegister(int dotNum) {
  // Example for 74HC595 shift register
  byte pattern = 0;
  
  for (int i = 0; i < 6; i++) {
    if (digitalRead(DOT_PINS[i]) == HIGH) {
      pattern |= (1 << i);
    }
  }
  
  if (dotNum >= 1 && dotNum <= 6) {
    pattern |= (1 << (dotNum - 1));
  }
  
  digitalWrite(LATCH_PIN, LOW);
  shiftOut(DATA_PIN, CLOCK_PIN, MSBFIRST, pattern);
  digitalWrite(LATCH_PIN, HIGH);
}
*/

// Alternative: If using PWM for motors/solenoids
/*
void activateDotPWM(int dotNum, int intensity = 255) {
  if (dotNum < 1 || dotNum > 6) return;
  
  int pin = DOT_PINS[dotNum - 1];
  analogWrite(pin, intensity);  // 0-255
}

void raiseDotSmoothly(int dotNum, int duration = 200) {
  // Gradually raise a dot
  for (int i = 0; i <= 255; i += 5) {
    activateDotPWM(dotNum, i);
    delay(duration / 51);
  }
}
*/

