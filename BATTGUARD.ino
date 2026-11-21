
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <math.h>

// =====================================================
// 1. OLED CONFIGURATION
// =====================================================

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(
  SCREEN_WIDTH,
  SCREEN_HEIGHT,
  &Wire,
  OLED_RESET
);

// =====================================================
// 2. PIN CONFIGURATION
// =====================================================

// Analog inputs
const int CELL1_PIN = 34;
const int CELL2_PIN = 35;
const int CELL3_PIN = 32;
const int CURRENT_PIN = 33;
const int NTC_PIN = 36;

// OLED: SDA = GPIO 21, SCL = GPIO 22

// LED outputs
const int STATUS_LED = 18;
const int FAULT_LED = 19;

const int BALANCE_LED1 = 23;
const int BALANCE_LED2 = 25;
const int BALANCE_LED3 = 26;

const int CONTACTOR_LED = 27;

// Pushbuttons
const int FAULT_BUTTON = 16;
const int RESET_BUTTON = 17;

// =====================================================
// 3. BATTERY CONFIGURATION
// =====================================================

const float CELL_MIN_VOLTAGE = 3.00;
const float CELL_MAX_VOLTAGE = 4.20;

const float MAX_CURRENT = 5.0;
const float MAX_TEMPERATURE = 60.0;

const float BALANCE_START_VOLTAGE = 3.80;
const float BALANCE_DIFFERENCE = 0.05;

const float BATTERY_CAPACITY_AH = 2.0;

float batterySOC = 80.0;

// NTC configuration
const float BETA = 3950.0;
const float NTC_R0 = 10000.0;
const float NTC_T0 = 298.15;

// ADC configuration
const float ADC_MAX = 4095.0;

// =====================================================
// 4. BATTERY VARIABLES
// =====================================================

float cell1Voltage = 0.0;
float cell2Voltage = 0.0;
float cell3Voltage = 0.0;

float packVoltage = 0.0;
float batteryCurrent = 0.0;
float batteryTemperature = 0.0;

float maximumCell = 0.0;
float minimumCell = 0.0;
float cellDifference = 0.0;

int cell1Raw = 0;
int cell2Raw = 0;
int cell3Raw = 0;
int ntcRaw = 0;

int balancingCell = 0;

// =====================================================
// 5. FAULT MANAGEMENT
// =====================================================

enum FaultCode {
  NO_FAULT,
  SENSOR_FAULT,
  CELL_OVERVOLTAGE,
  CELL_UNDERVOLTAGE,
  OVERCURRENT,
  OVERTEMPERATURE
};

FaultCode activeFault = NO_FAULT;
FaultCode latchedFault = NO_FAULT;

bool faultInjection = false;
bool resetRequested = false;

// =====================================================
// 6. BUTTON DEBOUNCING
// =====================================================

struct ButtonState {

  int pin;

  bool stableState;
  bool lastReading;

  unsigned long lastChange;
};

ButtonState faultButton = {
  FAULT_BUTTON, HIGH, HIGH, 0
};

ButtonState resetButton = {
  RESET_BUTTON, HIGH, HIGH, 0
};

bool buttonPressed(ButtonState &button) {

  bool reading = digitalRead(button.pin);

  if (reading != button.lastReading) {

    button.lastChange = millis();

    button.lastReading = reading;
  }

  if (millis() - button.lastChange >= 35) {

    if (reading != button.stableState) {

      button.stableState = reading;

      if (button.stableState == LOW) {
        return true;
      }
    }
  }

  return false;
}

// =====================================================
// 7. CELL VOLTAGE CONVERSION
// =====================================================

// Simulation mapping:
// ADC 0-4095 corresponds to 0-4.5V.

float readCellVoltage(int pin, int &rawValue) {

  rawValue = analogRead(pin);

  float voltage =
    (rawValue / ADC_MAX) * 4.5;

  return voltage;
}

// =====================================================
// 8. CURRENT CONVERSION
// =====================================================

// Simulation mapping:
// ADC 0-4095 corresponds to 0-10A.

float readCurrent() {

  int raw = analogRead(CURRENT_PIN);

  float current =
    (raw / ADC_MAX) * 10.0;

  return current;
}

// =====================================================
// 9. NTC TEMPERATURE CONVERSION
// =====================================================

// Assumed circuit:
// 3.3V -> fixed 10k resistor -> ADC -> NTC -> GND

float readTemperature() {

  ntcRaw = analogRead(NTC_PIN);

  if (ntcRaw <= 5 || ntcRaw >= 4090) {
    return -999.0;
  }

  // Calculate thermistor resistance

  float resistance =
    NTC_R0 *
    (ntcRaw / (ADC_MAX - ntcRaw));

  // Beta equation

  float temperatureKelvin =
    1.0 /
    (
      (log(resistance / NTC_R0) / BETA)
      + (1.0 / NTC_T0)
    );

  float temperatureCelsius =
    temperatureKelvin - 273.15;

  return temperatureCelsius;
}

// =====================================================
// 10. READ ALL BATTERY PARAMETERS
// =====================================================

void readBatteryData() {

  cell1Voltage =
    readCellVoltage(CELL1_PIN, cell1Raw);

  cell2Voltage =
    readCellVoltage(CELL2_PIN, cell2Raw);

  cell3Voltage =
    readCellVoltage(CELL3_PIN, cell3Raw);

  batteryCurrent = readCurrent();

  batteryTemperature = readTemperature();

  // Fault injection simulates an 8A overload

  if (faultInjection) {
    batteryCurrent = 8.0;
  }

  // Calculate total pack voltage

  packVoltage =
    cell1Voltage +
    cell2Voltage +
    cell3Voltage;

  // Find highest cell voltage

  maximumCell = max(
    cell1Voltage,
    max(cell2Voltage, cell3Voltage)
  );

  // Find lowest cell voltage

  minimumCell = min(
    cell1Voltage,
    min(cell2Voltage, cell3Voltage)
  );

  // Calculate cell imbalance

  cellDifference =
    maximumCell - minimumCell;
}

// =====================================================
// 11. FAULT DETECTION
// =====================================================

FaultCode detectFault() {

  // Check temperature sensor validity

  if (batteryTemperature == -999.0) {
    return SENSOR_FAULT;
  }

  // Cell overvoltage

  if (
    cell1Voltage > CELL_MAX_VOLTAGE ||
    cell2Voltage > CELL_MAX_VOLTAGE ||
    cell3Voltage > CELL_MAX_VOLTAGE
  ) {
    return CELL_OVERVOLTAGE;
  }

  // Cell undervoltage

  if (
    cell1Voltage < CELL_MIN_VOLTAGE ||
    cell2Voltage < CELL_MIN_VOLTAGE ||
    cell3Voltage < CELL_MIN_VOLTAGE
  ) {
    return CELL_UNDERVOLTAGE;
  }

  // Overcurrent

  if (batteryCurrent > MAX_CURRENT) {
    return OVERCURRENT;
  }

  // Overtemperature

  if (batteryTemperature > MAX_TEMPERATURE) {
    return OVERTEMPERATURE;
  }

  return NO_FAULT;
}

// =====================================================
// 12. FAULT NAME
// =====================================================

const char* getFaultName(FaultCode fault) {

  switch (fault) {

    case NO_FAULT:
      return "NORMAL";

    case SENSOR_FAULT:
      return "SENSOR ERROR";

    case CELL_OVERVOLTAGE:
      return "OVER VOLTAGE";

    case CELL_UNDERVOLTAGE:
      return "UNDER VOLTAGE";

    case OVERCURRENT:
      return "OVER CURRENT";

    case OVERTEMPERATURE:
      return "OVER TEMP";

    default:
      return "UNKNOWN";
  }
}

// =====================================================
// 13. STATE OF CHARGE ESTIMATION
// =====================================================

unsigned long previousSOCUpdate = 0;

void updateSOC() {

  unsigned long currentTime = millis();

  float elapsedSeconds =
    (currentTime - previousSOCUpdate) / 1000.0;

  previousSOCUpdate = currentTime;

  // Update SOC only when protection is inactive

  if (latchedFault == NO_FAULT) {

    // Coulomb counting

    float consumedPercentage =
      (batteryCurrent * elapsedSeconds)
      / (BATTERY_CAPACITY_AH * 3600.0)
      * 100.0;

    batterySOC -= consumedPercentage;
  }

  // Restrict SOC between 0 and 100

  batterySOC = constrain(
    batterySOC,
    0.0,
    100.0
  );
}

// =====================================================
// 14. CELL BALANCING LOGIC
// =====================================================

void updateBalancing() {

  balancingCell = 0;

  // Do not balance during a protection event

  if (latchedFault != NO_FAULT) {
    return;
  }

  // Check balancing conditions

  if (
    maximumCell < BALANCE_START_VOLTAGE ||
    cellDifference < BALANCE_DIFFERENCE
  ) {
    return;
  }

  // Identify the highest-voltage cell

  if (
    cell1Voltage >= cell2Voltage &&
    cell1Voltage >= cell3Voltage
  ) {

    balancingCell = 1;
  }

  else if (
    cell2Voltage >= cell1Voltage &&
    cell2Voltage >= cell3Voltage
  ) {

    balancingCell = 2;
  }

  else {

    balancingCell = 3;
  }
}

// =====================================================
// 15. OUTPUT CONTROL
// =====================================================

void updateOutputs() {

  bool protectionActive =
    (latchedFault != NO_FAULT);

  // Status LED

  digitalWrite(
    STATUS_LED,
    protectionActive ? LOW : HIGH
  );

  // Fault LED

  digitalWrite(
    FAULT_LED,
    protectionActive ? HIGH : LOW
  );

  // Contactor indicator

  digitalWrite(
    CONTACTOR_LED,
    protectionActive ? LOW : HIGH
  );

  // Switch OFF all balancing LEDs first

  digitalWrite(BALANCE_LED1, LOW);
  digitalWrite(BALANCE_LED2, LOW);
  digitalWrite(BALANCE_LED3, LOW);

  // Activate the selected balancing LED

  if (!protectionActive) {

    if (balancingCell == 1) {

      digitalWrite(BALANCE_LED1, HIGH);
    }

    else if (balancingCell == 2) {

      digitalWrite(BALANCE_LED2, HIGH);
    }

    else if (balancingCell == 3) {

      digitalWrite(BALANCE_LED3, HIGH);
    }
  }
}

// =====================================================
// 16. BUTTON AND FAULT CONTROL
// =====================================================

void handleButtons() {

  // Toggle fault injection

  if (buttonPressed(faultButton)) {

    faultInjection = !faultInjection;

    Serial.print("Fault injection: ");

    Serial.println(
      faultInjection ? "ON" : "OFF"
    );
  }

  // Request a protection reset

  if (buttonPressed(resetButton)) {

    resetRequested = true;
  }
}

// =====================================================
// 17. PROCESS FAULT RESET
// =====================================================

void processFaultReset() {

  if (!resetRequested) {
    return;
  }

  // Reset only when all faults have disappeared

  if (
    activeFault == NO_FAULT &&
    !faultInjection
  ) {

    latchedFault = NO_FAULT;

    Serial.println(
      "Protection reset successfully."
    );
  }

  else {

    Serial.println(
      "RESET BLOCKED: Fault still active."
    );
  }

  resetRequested = false;
}

// =====================================================
// 18. OLED DASHBOARD
// =====================================================

void drawDashboard() {

  display.clearDisplay();

  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);

  // Automatically switch pages every 3 seconds

  int page = (millis() / 3000) % 2;

  if (page == 0) {

    // PAGE 1: BATTERY OVERVIEW

    display.setCursor(0, 0);

    display.print("BATTGUARD | 3S BMS");

    display.drawLine(
      0, 10, 127, 10, SSD1306_WHITE
    );

    display.setCursor(0, 14);

    display.print("C1:");
    display.print(cell1Voltage, 2);

    display.print(" C2:");
    display.print(cell2Voltage, 2);

    display.setCursor(0, 25);

    display.print("C3:");
    display.print(cell3Voltage, 2);

    display.print(" P:");
    display.print(packVoltage, 2);

    display.setCursor(0, 37);

    display.print("SOC:");
    display.print(batterySOC, 1);
    display.print("%");

    display.print(" I:");
    display.print(batteryCurrent, 1);
    display.print("A");

    display.setCursor(0, 48);

    display.print("TEMP:");
    display.print(batteryTemperature, 1);
    display.print(" C");

    display.setCursor(0, 56);

    display.print(
      latchedFault == NO_FAULT
      ? "SYSTEM: RUN"
      : "SYSTEM: TRIP"
    );
  }

  else {

    // PAGE 2: DIAGNOSTICS

    display.setCursor(0, 0);

    display.print("BATTGUARD DIAGNOSTICS");

    display.drawLine(
      0, 10, 127, 10, SSD1306_WHITE
    );

    display.setCursor(0, 15);

    display.print("IMBALANCE:");

    display.print(
      cellDifference * 1000.0,
      0
    );

    display.print("mV");

    display.setCursor(0, 27);

    display.print("BALANCING CELL:");

    if (balancingCell == 0) {

      display.print("NONE");
    }

    else {

      display.print(balancingCell);
    }

    display.setCursor(0, 39);

    display.print("FAULT:");

    display.print(
      getFaultName(latchedFault)
    );

    display.setCursor(0, 52);

    display.print("INJECTION:");

    display.print(
      faultInjection ? "ON" : "OFF"
    );
  }

  // Send buffer to OLED

  display.display();
}

// =====================================================
// 19. SERIAL DIAGNOSTICS
// =====================================================

unsigned long previousSerialUpdate = 0;

void printSerialData() {

  if (
    millis() - previousSerialUpdate < 1000
  ) {
    return;
  }

  previousSerialUpdate = millis();

  Serial.println("--------------------------");

  Serial.print("Cell 1: ");
  Serial.println(cell1Voltage, 2);

  Serial.print("Cell 2: ");
  Serial.println(cell2Voltage, 2);

  Serial.print("Cell 3: ");
  Serial.println(cell3Voltage, 2);

  Serial.print("Pack: ");
  Serial.println(packVoltage, 2);

  Serial.print("Current: ");
  Serial.println(batteryCurrent, 2);

  Serial.print("Temperature: ");
  Serial.println(batteryTemperature, 2);

  Serial.print("SOC: ");
  Serial.println(batterySOC, 1);

  Serial.print("Cell imbalance: ");
  Serial.println(cellDifference * 1000.0, 0);

  Serial.print("Balancing cell: ");
  Serial.println(balancingCell);

  Serial.print("Active fault: ");
  Serial.println(getFaultName(activeFault));

  Serial.print("Latched fault: ");
  Serial.println(getFaultName(latchedFault));

  Serial.println("--------------------------");
}

// =====================================================
// 20. SETUP
// =====================================================

void setup() {

  Serial.begin(115200);

  // Configure ADC resolution

  analogReadResolution(12);

  // Configure LED outputs

  pinMode(STATUS_LED, OUTPUT);
  pinMode(FAULT_LED, OUTPUT);

  pinMode(BALANCE_LED1, OUTPUT);
  pinMode(BALANCE_LED2, OUTPUT);
  pinMode(BALANCE_LED3, OUTPUT);

  pinMode(CONTACTOR_LED, OUTPUT);

  // Start with all LEDs OFF

  digitalWrite(STATUS_LED, LOW);
  digitalWrite(FAULT_LED, LOW);

  digitalWrite(BALANCE_LED1, LOW);
  digitalWrite(BALANCE_LED2, LOW);
  digitalWrite(BALANCE_LED3, LOW);

  digitalWrite(CONTACTOR_LED, LOW);

  // Configure buttons

  pinMode(FAULT_BUTTON, INPUT_PULLUP);
  pinMode(RESET_BUTTON, INPUT_PULLUP);

  // Initialize I2C

  Wire.begin(21, 22);

  // Initialize OLED

  if (
    !display.begin(
      SSD1306_SWITCHCAPVCC,
      OLED_ADDRESS
    )
  ) {

    Serial.println(
      "OLED initialization failed!"
    );

    while (true) {
      delay(100);
    }
  }

  // Display startup screen

  display.clearDisplay();

  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(10, 20);

  display.println("BATTGUARD");

  display.display();

  delay(1500);

  // Initialize SOC timing

  previousSOCUpdate = millis();

  Serial.println(
    "BATTGUARD SYSTEM STARTED"
  );
}

// =====================================================
// 21. MAIN LOOP
// =====================================================

void loop() {

  // Step 1: Handle button inputs

  handleButtons();

  // Step 2: Read battery measurements

  readBatteryData();

  // Step 3: Detect current fault

  activeFault = detectFault();

  // Step 4: Latch the first detected fault

  if (
    activeFault != NO_FAULT &&
    latchedFault == NO_FAULT
  ) {

    latchedFault = activeFault;

    Serial.print("FAULT DETECTED: ");

    Serial.println(
      getFaultName(latchedFault)
    );
  }

  // Step 5: Process reset request

  processFaultReset();

  // Step 6: Update battery SOC

  updateSOC();

  // Step 7: Calculate balancing decision

  updateBalancing();

  // Step 8: Update output indicators

  updateOutputs();

  // Step 9: Update OLED

  drawDashboard();

  // Step 10: Print diagnostics

  printSerialData();

  delay(100);
}
