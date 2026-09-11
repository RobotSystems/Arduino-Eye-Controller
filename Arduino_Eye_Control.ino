#include <Wire.h>

// PCA9685 I2C adres (standaard 0x40)
#define PCA9685_ADDR 0x40

// PCA9685 registers
#define MODE1 0x00
#define MODE2 0x01
#define SUBREG1 0x02
#define SUBREG2 0x03
#define SUBREG3 0x04
#define LED0_ON_L 0x06
#define LED0_ON_H 0x07
#define LED0_OFF_L 0x08
#define LED0_OFF_H 0x09
#define ALL_LED_ON_L 0xFA
#define ALL_LED_ON_H 0xFB
#define ALL_LED_OFF_L 0xFC
#define ALL_LED_OFF_H 0xFD
#define PRE_SCALE 0xFE

// Servo configuratie: {min_angle, max_angle, start_angle, PCA9685_pin}
struct ServoConfig {
  int min_angle;
  int max_angle;
  int start_angle;
  int pca_pin;
};

// Servo's opslaan
std::map<String, ServoConfig> servos;

void setup() {
  // Seriele communicatie starten (115200 baud)
  Serial.begin(115200);
  delay(2000);
  
  Serial.println("\n=== Arduino Mega Eye Controller ===");
  Serial.println("PCA9685 servo controller initialiseren...");
  
  // I2C starten
  Wire.begin();
  delay(100);
  
  // PCA9685 initialiseren
  initPCA9685();
  
  // Servo configuratie definiëren
  // LET OP: Pas de PCA9685 pin nummers aan naar waar jouw servos zitten!
  servos["LR"]  = {40, 140, 90, 0};   // Links/Rechts - Pin 0
  servos["UD"]  = {40, 140, 90, 1};   // Omhoog/Omlaag - Pin 1
  servos["TL"]  = {90, 170, 130, 2};  // Ooglid Links Boven - Pin 2
  servos["BL"]  = {10, 90, 50, 3};    // Ooglid Links Onder - Pin 3
  servos["TR"]  = {10, 90, 50, 4};    // Ooglid Rechts Boven - Pin 4
  servos["BR"]  = {90, 160, 125, 5};  // Ooglid Rechts Onder - Pin 5
  
  // Alle servo's naar startpositie
  goToStartPositions();
  
  Serial.println("Klaar voor commando's!");
  Serial.println("Formaat: SERVO:HOEK (bijv: LR:90)");
}

void loop() {
  // Seriële commando's verwerken
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
  // MODE1: Sleep uitschakelen, auto-increment inschakelen
  writePCA9685(MODE1, 0x01);
  delay(10);
  
  // MODE2: invert uit, totem-pole
  writePCA9685(MODE2, 0x04);
  
  // Frequentie instellen op 50 Hz (voor servo's)
  // Formule: prescale = (osc_freq / (freq * 4096)) - 1
  // Met osc_freq = 25 MHz: prescale = (25000000 / (50 * 4096)) - 1 = 121
  writePCA9685(PRE_SCALE, 121);
  
  delay(10);
  
  // MODE1: Sleep uitzetten
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

// Lees van PCA9685 register
uint8_t readPCA9685(uint8_t reg) {
  Wire.beginTransmission(PCA9685_ADDR);
  Wire.write(reg);
  Wire.endTransmission();
  Wire.requestFrom(PCA9685_ADDR, 1);
  return Wire.read();
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

// Parseer inkomende commando's in formaat: "LR:90"
void parseCommand(String command) {
  int colonPos = command.indexOf(':');
  
  if (colonPos == -1) {
    Serial.println("FOUT: Ongeldig formaat. Gebruik: SERVO:HOEK");
    return;
  }
  
  String servoName = command.substring(0, colonPos);
  String angleStr = command.substring(colonPos + 1);
  
  servoName.trim();
  angleStr.trim();
  
  // Controleer of servo bestaat
  if (servos.find(servoName) == servos.end()) {
    Serial.print("FOUT: Servo '");
    Serial.print(servoName);
    Serial.println("' onbekend!");
    return;
  }
  
  int angle = angleStr.toInt();
  ServoConfig config = servos[servoName];
  
  // Controleer bereik
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
  
  // Stuur servo aan
  setServoAngle(servoName, angle);
  
  Serial.print("OK: ");
  Serial.print(servoName);
  Serial.print(" -> ");
  Serial.println(angle);
}

// Zet servo naar gegeven hoek (0-180 graden)
void setServoAngle(String servoName, int angle) {
  ServoConfig config = servos[servoName];
  
  // Omzetten van hoek (0-180) naar PWM pulsbreedte voor servo
  // Standaard servo: 1ms (0°) tot 2ms (180°) bij 50Hz
  // Bij 50Hz = 20ms per periode
  // 1ms = 102 / 4096 * 20ms
  // 2ms = 512 / 4096 * 20ms
  int pulseLength = map(angle, 0, 180, 102, 512);
  
  setPWM(config.pca_pin, 0, pulseLength);
}

// Zet alle servo's naar startpositie
void goToStartPositions() {
  Serial.println("Servo's naar middenstand...");
  
  for (auto& servo : servos) {
    String name = servo.first;
    ServoConfig config = servo.second;
    setServoAngle(name, config.start_angle);
    delay(50);
  }
  
  Serial.println("Klaar!");
}
