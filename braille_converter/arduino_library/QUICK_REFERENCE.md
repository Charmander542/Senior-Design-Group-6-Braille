# BrailleConverter Quick Reference

## Quick Start (30 seconds)

```cpp
#include <BrailleConverter.h>

BrailleConverter converter;

void setup() {
  Serial.begin(115200);
  
  // Convert and display a character
  BrailleChar bc = converter.convertChar('A');
  
  Serial.print("Dots: ");
  for (uint8_t i = 0; i < bc.dotCount; i++) {
    Serial.print(bc.dots[i]);
    if (i < bc.dotCount - 1) Serial.print(",");
  }
}

void loop() { }
```

## Common Tasks

### Convert a Single Character

```cpp
BrailleChar bc = converter.convertChar('A');
// bc.original = 'A'
// bc.dotPattern = 0x41
// bc.dotCount = 2
// bc.dots = {1, 7}
```

### Convert a String

```cpp
const char* text = "Hello";
uint16_t count = converter.convertText(text);

for (uint16_t i = 0; i < count; i++) {
  BrailleChar bc = converter.getCharAt(i);
  // Process each character
}
```

### Get Dot Pattern (Byte)

```cpp
uint8_t pattern = converter.getDotPattern('A');
// pattern = 0x41 (binary: 01000001)
// bit 0 = dot 1 (raised)
// bit 6 = dot 7 (raised)
```

### Get Dots Array

```cpp
uint8_t dots[8];
uint8_t count = converter.getDots('A', dots);
// count = 2
// dots = {1, 7, ...}
```

### Drive Physical Display

```cpp
const uint8_t PINS[8] = {2,3,4,5,6,7,8,9};

void displayChar(char c) {
  uint8_t pattern = converter.getDotPattern(c);
  
  for (uint8_t i = 0; i < 8; i++) {
    digitalWrite(PINS[i], (pattern & (1 << i)) ? HIGH : LOW);
  }
}
```

### Print Visual Pattern

```cpp
BrailleChar bc = converter.convertChar('A');
converter.printPattern(bc.dotPattern);

// Output:
//  *  o 
//  o  o 
//  o  o 
//  *  o 
```

## 8-Dot Layout Reference

```
1 • • 4
2 • • 5
3 • • 6
7 • • 8
```

## Common Characters

| Char | Dots      | Hex  | Binary   |
|------|-----------|------|----------|
| a    | 1         | 0x01 | 00000001 |
| A    | 1,7       | 0x41 | 01000001 |
| b    | 1,2       | 0x03 | 00000011 |
| B    | 1,2,7     | 0x43 | 01000011 |
| 0    | 2,4,5     | 0x1A | 00011010 |
| 1    | 1         | 0x01 | 00000001 |
| space| none      | 0x00 | 00000000 |
| ,    | 2         | 0x02 | 00000010 |
| .    | 2,5,6     | 0x32 | 00110010 |
| !    | 2,3,5     | 0x16 | 00010110 |
| ?    | 2,3,6     | 0x26 | 00100110 |

## Bit Pattern Reference

Each character is represented as an 8-bit value:

```
Bit 0 → Dot 1
Bit 1 → Dot 2
Bit 2 → Dot 3
Bit 3 → Dot 4
Bit 4 → Dot 5
Bit 5 → Dot 6
Bit 6 → Dot 7
Bit 7 → Dot 8
```

**Example**: Letter 'A'
- Dots: 1, 7
- Binary: `01000001`
- Hex: `0x41`
- Check if dot is raised: `pattern & (1 << (dot-1))`

## Pin Configuration Example

```cpp
// For 8-dot display
const uint8_t DOT_PINS[8] = {
  2,  // Dot 1
  3,  // Dot 2
  4,  // Dot 3
  5,  // Dot 4
  6,  // Dot 5
  7,  // Dot 6
  8,  // Dot 7
  9   // Dot 8
};
```

## Useful Patterns

### Check if Specific Dot is Raised

```cpp
uint8_t pattern = converter.getDotPattern('A');
bool dot1Raised = pattern & (1 << 0);  // Check dot 1
bool dot7Raised = pattern & (1 << 6);  // Check dot 7
```

### Count Raised Dots

```cpp
uint8_t pattern = converter.getDotPattern('A');
uint8_t count = 0;
for (uint8_t i = 0; i < 8; i++) {
  if (pattern & (1 << i)) count++;
}
```

### Convert Pattern to Array

```cpp
uint8_t pattern = 0x41;  // Letter 'A'
uint8_t dots[8];
uint8_t count = converter.patternToDots(pattern, dots);
// count = 2, dots = {1, 7}
```

## Memory Usage

```cpp
#define MAX_BRAILLE_DOTS 8       // 8 dots per cell
#define MAX_INPUT_LENGTH 256     // Max 256 chars per conversion
```

- **Per Character**: ~12 bytes
- **Buffer**: ~3KB (256 chars)
- **Code**: ~4KB Flash
- **Total RAM**: ~4KB

### Reduce Memory Usage

Edit `BrailleConverter.h`:

```cpp
#define MAX_INPUT_LENGTH 64  // Reduce buffer to 64 chars
```

## Error Handling

```cpp
// Unknown character returns 0xFF (all dots)
uint8_t pattern = converter.getDotPattern('€');
if (pattern == 0xFF) {
  Serial.println("Unknown character");
}

// Check bounds
uint16_t count = converter.getCharCount();
if (index < count) {
  BrailleChar bc = converter.getCharAt(index);
}
```

## Common Pitfalls

### ❌ Don't do this:
```cpp
char* text = "Hello";  // Don't use char*
text[0] = 'h';         // Can cause crashes
```

### ✅ Do this instead:
```cpp
const char* text = "Hello";  // Use const char*
// OR
char text[] = "Hello";       // Mutable array
```

### ❌ Don't do this:
```cpp
// Dots are 1-indexed, not 0-indexed!
digitalWrite(PINS[bc.dots[0]], HIGH);  // Wrong if dots[0] = 1
```

### ✅ Do this instead:
```cpp
digitalWrite(PINS[bc.dots[0] - 1], HIGH);  // Correct
```

## Performance Tips

1. **Reuse Objects**: Create one `BrailleConverter` instance
2. **Use getDotPattern()**: Faster than full conversion for simple cases
3. **Batch Operations**: Convert entire strings at once
4. **Avoid Serial.print() in loops**: It's slow

## Debugging Tips

```cpp
// Print character details
converter.printChar(bc);

// Print visual pattern
converter.printPattern(pattern);

// Print all converted text
converter.printConvertedText();

// Check pattern value
Serial.print("Pattern: 0x");
Serial.println(pattern, HEX);
```

## Integration Examples

### With SD Card

```cpp
#include <SD.h>
File file = SD.open("input.txt");
while (file.available()) {
  char c = file.read();
  BrailleChar bc = converter.convertChar(c);
  displayChar(bc);
}
file.close();
```

### With WiFi (ESP32)

```cpp
#include <WiFi.h>
WiFiServer server(80);

while (true) {
  WiFiClient client = server.available();
  if (client) {
    String line = client.readStringUntil('\n');
    converter.convertText(line.c_str());
    // Display converted text
  }
}
```

### With Bluetooth Serial

```cpp
#include <BluetoothSerial.h>
BluetoothSerial BT;

void loop() {
  if (BT.available()) {
    String text = BT.readString();
    converter.convertText(text.c_str());
    // Display converted text
  }
}
```

## API Quick Reference

| Function | Purpose | Returns |
|----------|---------|---------|
| `convertChar(c)` | Convert single char | `BrailleChar` |
| `convertText(text)` | Convert string | `uint16_t` count |
| `getCharAt(i)` | Get converted char | `BrailleChar` |
| `getCharCount()` | Get count | `uint16_t` |
| `getDotPattern(c)` | Get pattern byte | `uint8_t` |
| `getDots(c, arr)` | Get dots array | `uint8_t` count |
| `patternToDots(p, arr)` | Convert to array | `uint8_t` count |
| `printChar(bc)` | Print to Serial | `void` |
| `printPattern(p)` | Print visual | `void` |

---

**Keep this reference handy while coding! 📌**

