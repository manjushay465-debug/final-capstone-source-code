#include <Wire.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Pins
#define OLED_CS     5
#define OLED_DC    16
#define OLED_RESET 17

Adafruit_SSD1306 display(128, 64, &SPI, OLED_DC, OLED_RESET, OLED_CS);

// GSR
#define GSR_PIN 34

uint32_t tsLastReport = 0;

// -------- Stress Window --------
#define WINDOW_SIZE 5   // Reduced for fast calibration
float gsrWindow[WINDOW_SIZE];
int windowIndex = 0;
bool windowFilled = false;

// ===============================
// Stress Logic
// ===============================
String getStressLevel(int gsrValue) {

  gsrWindow[windowIndex] = gsrValue;
  windowIndex++;

  if (windowIndex >= WINDOW_SIZE) {
    windowIndex = 0;
    windowFilled = true;
  }

  // Start showing stress after 3 readings
  if (!windowFilled && windowIndex < 3)
    return "Calibrating";

  float sum = 0;
  int count = windowFilled ? WINDOW_SIZE : windowIndex;

  for (int i = 0; i < count; i++)
    sum += gsrWindow[i];

  float mean = sum / count;

  float variance = 0;
  for (int i = 0; i < count; i++)
    variance += pow(gsrWindow[i] - mean, 2);

  variance /= count;

  float stdDev = sqrt(variance);
  if (stdDev == 0) stdDev = 1;

  float zScore = (gsrValue - mean) / stdDev;

  if (zScore < 0.5)
    return "Normal";
  else if (zScore < 1.5)
    return "Moderate";
  else
    return "High";
}

// ===============================
void setup() {

  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC)) {
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println("System Ready");
  display.display();
  delay(1000);
}

// ===============================
void loop() {

  if (millis() - tsLastReport > 1000) {

    int rawGSR = analogRead(GSR_PIN);

    // Smooth GSR
    static float smoothGSR = rawGSR;
    smoothGSR = (smoothGSR * 0.7) + (rawGSR * 0.3);
    int gsrValue = (int)smoothGSR;

    // Wear sensor logic
    if (gsrValue > 2000) {

      display.clearDisplay();
      display.setCursor(20, 30);
      display.println("Wear Sensor");
      display.display();

      Serial.println("Wear Sensor - GSR High");

      tsLastReport = millis();
      return;
    }

    String stress = getStressLevel(gsrValue);

    display.clearDisplay();

    display.setCursor(0, 10);
    display.print("GSR: ");
    display.print(gsrValue);

    display.setCursor(0, 30);
    display.print("Stress: ");
    display.print(stress);

    display.display();

    Serial.print("GSR: ");
    Serial.print(gsrValue);
    Serial.print(" | Stress: ");
    Serial.println(stress);

    tsLastReport = millis();
  }
}