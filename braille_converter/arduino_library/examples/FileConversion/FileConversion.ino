/*
 * FileConversion.ino
 * 
 * Example for reading text from an SD card (or other storage)
 * and converting it to Braille
 * 
 * Note: Requires SD card module and SD library
 * Adjust the CS_PIN for your SD card module
 */

#include <BrailleConverter.h>
#include <SD.h>

BrailleConverter converter;

// SD card chip select pin (adjust for your hardware)
const int CS_PIN = 10;

// File to read
const char* INPUT_FILE = "input.txt";

void setup() {
  Serial.begin(115200);
  while (!Serial) {
    ; // Wait for serial port to connect
  }
  
  Serial.println("========================================");
  Serial.println("  BrailleConverter File Example");
  Serial.println("========================================");
  Serial.println();
  
  // Initialize SD card
  Serial.print("Initializing SD card... ");
  if (!SD.begin(CS_PIN)) {
    Serial.println("FAILED!");
    Serial.println("Please check:");
    Serial.println("  - SD card is inserted");
    Serial.println("  - CS_PIN is correct");
    Serial.println("  - Wiring is correct");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("OK!");
  Serial.println();
  
  // Read and convert file
  convertFile(INPUT_FILE);
}

void loop() {
  // Nothing to do in loop
  delay(1000);
}

void convertFile(const char* filename) {
  Serial.print("Opening file: ");
  Serial.println(filename);
  
  File file = SD.open(filename);
  if (!file) {
    Serial.println("ERROR: Could not open file!");
    Serial.println();
    Serial.println("Please ensure the file exists on the SD card.");
    return;
  }
  
  Serial.println("File opened successfully!");
  Serial.print("File size: ");
  Serial.print(file.size());
  Serial.println(" bytes");
  Serial.println();
  
  // Read file in chunks and convert
  Serial.println("========================================");
  Serial.println("Converting file contents:");
  Serial.println("========================================");
  Serial.println();
  
  uint32_t totalChars = 0;
  uint16_t lineNumber = 1;
  
  String line = "";
  
  while (file.available()) {
    char c = file.read();
    
    if (c == '\n') {
      // Process the line
      if (line.length() > 0) {
        Serial.print("Line ");
        Serial.print(lineNumber);
        Serial.print(": \"");
        Serial.print(line);
        Serial.println("\"");
        
        processLine(line);
        totalChars += line.length();
        lineNumber++;
        
        Serial.println();
      }
      line = "";
    } else if (c != '\r') {
      line += c;
    }
  }
  
  // Process last line if no trailing newline
  if (line.length() > 0) {
    Serial.print("Line ");
    Serial.print(lineNumber);
    Serial.print(": \"");
    Serial.print(line);
    Serial.println("\"");
    
    processLine(line);
    totalChars += line.length();
    
    Serial.println();
  }
  
  file.close();
  
  Serial.println("========================================");
  Serial.print("Conversion complete! Processed ");
  Serial.print(totalChars);
  Serial.println(" characters.");
  Serial.println("========================================");
}

void processLine(String line) {
  // Convert to char array
  char buffer[256];
  line.toCharArray(buffer, 256);
  
  uint16_t count = converter.convertText(buffer);
  
  // Display conversion summary
  Serial.print("  Converted ");
  Serial.print(count);
  Serial.print(" chars -> ");
  
  // Print dots for each character
  for (uint16_t i = 0; i < count; i++) {
    BrailleChar bc = converter.getCharAt(i);
    
    if (bc.original == ' ') {
      Serial.print("[_] ");
    } else {
      Serial.print(bc.original);
      Serial.print(":");
      if (bc.dotCount == 0) {
        Serial.print("[-] ");
      } else {
        Serial.print("[");
        for (uint8_t j = 0; j < bc.dotCount; j++) {
          Serial.print(bc.dots[j]);
          if (j < bc.dotCount - 1) Serial.print(",");
        }
        Serial.print("] ");
      }
    }
  }
  Serial.println();
}

