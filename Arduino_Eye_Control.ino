#include <Wire.h>

// PCA9685 I2C adres (standaard 0x40)
#define PCA9685_ADDR 0x40

// PCA9685 registers
#define MODE1 0x00
#define MODE2 0x01
#define PRE_SCALE 0xFE
#define LED0_ON_L 0x06
#define LED0_ON_H 0x07
#define LED0_OFF_L 0x08
#define LED0_OFF_H 0x09

// Servo configuratie
struct ServoConfig {
  const char* name;
  int min_angle;
  int max_angle;
  int start_angle;
  int pca_pin;
};

// Alle servo's - SIMPEL array in plaats van std::map
ServoConfig servos[] = {
  {"LR", 40, 140, 90, 0},    // Links/Rechts - Pin 0
  {"UD", 40, 140, 90, 1},    // Omhoog/Omlaag - Pin 1
  {"TL", 90, 170, 130, 2},   // Ooglid Links Boven - Pin 2
  {"BL", 10, 90, 50, 3},     // Ooglid Links Onder - Pin 3
  {"TR", 10, 90, 50, 4},     // Ooglid Rechts Boven - Pin 4
  {"BR", 90, 160, 125, 5}    // Ooglid Rechts Onder - Pin 5
};

#define NUM_SERVOS 6

void setup() {
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== Arduino Mega Eye Controller ===");
  Serial.println("PCA9685 servo controller initialiseren...");
  
  Wire.begin();
  delay(100);
  
  initPCA9685();
  goToStartPositions();
  
  Serial.println("Klaar voor commando's!");
  Serial.println("Formaat: SERVO:HOEK (bijv: LR:90)");
}

void loop() {
  if (Serial.available()) {
    String command = Serial.readStringUntil('\n');
    command.trim();
    
    if (command.length() > 0) {
      parseCommand(command);
    }
  }
}

// PCA9685 initialiseren
void initPCA9685() {
  writePCA9685(MODE1, 0x01);
  delay(10);
  
  writePCA9685(MODE2, 0x04);
  
  // 50 Hz frequentie
  writePCA9685(PRE_SCALE, 121);
  delay(10);
  
  writePCA9685(MODE1, 0x01);
  
  Serial.println("PCA9685 geïnitialiseerd (50Hz)");
}

// Schrijf naar PCA9685 register
void writePCA9685(uint8_t reg, uint8_t data) {
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg);
  Wire.write(data);
  Wire.endTransmission();
}

// Stuur PWM naar een kanaal
void setPWM(uint8_t channel, uint16_t on, uint16_t off) {
  uint8_t reg_l = LED0_ON_L + 4 * channel;
  uint8_t reg_h = LED0_ON_H + 4 * channel;
  uint8_t reg_l_off = LED0_OFF_L + 4 * channel;
  uint8_t reg_h_off = LED0_OFF_H + 4 * channel;
  
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg_l);
  Wire.write(on & 0xFF);
  Wire.write((on >> 8) & 0x0F);
  Wire.write(off & 0xFF);
  Wire.write((off >> 8) & 0x0F);
  Wire.endTransmission();
}

// Vind servo in array
int findServo(String servoName) {
  for (int i = 0; i < NUM_SERVOS; i++) {
    if (servoName.equals(servos[i].name)) {
      return i;
    }
  }
  return -1;
}

// Parseer inkomende commando's "LR:90"
void parseCommand(String command) {
  int colonPos = command.indexOf(':');
  
  if (colonPos == -1) {
    Serial.println("FOUT: Gebruik formaat SERVO:HOEK (bijv: LR:90)");
    return;
  }
  
  String servoName = command.substring(0, colonPos);
  String angleStr = command.substring(colonPos + 1);
  
  servoName.trim();
  angleStr.trim();
  
  int servoIndex = findServo(servoName);
  
  if (servoIndex == -1) {
    Serial.print("FOUT: Servo '");
    Serial.print(servoName);
    Serial.println("' onbekend!");
    return;
  }
  
  int angle = angleStr.toInt();
  ServoConfig config = servos[servoIndex];
  
  if (angle < config.min_angle || angle > config.max_angle) {
    Serial.print("FOUT: Hoek ");
    Serial.print(angle);
    Serial.print(" buiten bereik [");
    Serial.print(config.min_angle);
    Serial.print("-");
    Serial.print(config.max_angle);
    Serial.println("]");
    return;
  }
  
  setServoAngle(servoIndex, angle);
  
  Serial.print("OK: ");
  Serial.print(servoName);
  Serial.print(" -> ");
  Serial.println(angle);
}

// Zet servo naar gegeven hoek
void setServoAngle(int servoIndex, int angle) {
  ServoConfig config = servos[servoIndex];
  int pulseLength = map(angle, 0, 180, 102, 512);
  setPWM(config.pca_pin, 0, pulseLength);
}

// Zet alle servo's naar startpositie
void goToStartPositions() {
  Serial.println("Servo's naar middenstand...");
  
  for (int i = 0; i < NUM_SERVOS; i++) {
    setServoAngle(i, servos[i].start_angle);
    delay(50);
  }
  
  Serial.println("Klaar!");
}
