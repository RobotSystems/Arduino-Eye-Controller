#include <Wire.h>
#include <Adafruit_PCA9685.h>

// PCA9685 PWM servo controller
Adafruit_PCA9685 pwm = Adafruit_PCA9685();

// Servo configuratie: {min_angle, max_angle, start_angle, PCA9685_pin}
struct ServoConfig {
  int min_angle;
  int max_angle;
  int start_angle;
  int pca_pin;
};

// Definieer servos met hun pins op PCA9685
// Zet deze pins aan waar jouw servos fysiek zijn aangesloten!
std::map<String, ServoConfig> servos;

void setup() {
  // Seriele communicatie starten (115200 baud, zoals in Python-script)
  Serial.begin(115200);
  delay(2000);  // Wachten op initialisatie
  
  Serial.println("Arduino Mega Eye Controller gestart!");
  Serial.println("PCA9685 servo controller initialiseren...");
  
  // PCA9685 initialiseren (standaard I2C adres: 0x40)
  if (!pwm.begin()) {
    Serial.println("FOUT: PCA9685 niet gevonden! Controleer I2C verbinding.");
    while (1);
  }
  
  // PCA9685 frequentie instellen (50 Hz voor servo's)
  pwm.setOscillatorFrequency(27000000);
  pwm.setPWMFreq(50);
  
  delay(100);
  
  // Servo configuratie definiëren (naam, min, max, start, PCA9685_pin)
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
  int pulseLength = map(angle, 0, 180, 102, 512);  // 102-512 = ~1ms-2ms
  
  pwm.setPWM(config.pca_pin, 0, pulseLength);
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
