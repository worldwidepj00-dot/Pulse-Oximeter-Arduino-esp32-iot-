#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "MAX30100_PulseOximeter.h"

#define SAMPLE_INTERVAL 800   // slightly faster sampling

PulseOximeter pox;
LiquidCrystal_I2C lcd(0x27, 16, 2);

uint32_t lastSampleTime = 0;

// Buffers (20 samples)
float hrBuffer[20];
float spo2Buffer[20];
int sampleCount = 0;

bool measurementDone = false;

// Icons
byte heart[8] = {
  B00000,B01010,B11111,B11111,B11111,B01110,B00100,B00000
};

byte oxygen[8] = {
  B00100,B01010,B01010,B10001,B10001,B01010,B00100,B00000
};

byte okIcon[8] = {
  B00001,B00010,B10100,B01000,B00000,B00000,B00000,B00000
};

byte warnIcon[8] = {
  B00100,B00100,B00100,B00100,B00000,B00100,B00000,B00000
};

// 🔧 Sorting function
void sortArray(float arr[], int n)
{
    for (int i = 0; i < n - 1; i++)
    {
        for (int j = i + 1; j < n; j++)
        {
            if (arr[j] < arr[i])
            {
                float temp = arr[i];
                arr[i] = arr[j];
                arr[j] = temp;
            }
        }
    }
}

void setup()
{
    Serial.begin(9600);

    lcd.init();
    lcd.backlight();

    lcd.createChar(0, heart);
    lcd.createChar(1, oxygen);
    lcd.createChar(2, okIcon);
    lcd.createChar(3, warnIcon);

    lcd.print("Initializing...");
    delay(2000);

    if (!pox.begin())
    {
        lcd.clear();
        lcd.print("Sensor FAIL");
        while (1);
    }

    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);

    lcd.clear();
    lcd.print("Place Finger");
}

void loop()
{
    pox.update();

    float hr = pox.getHeartRate();
    float spo2 = pox.getSpO2();

    if (measurementDone) return;

    // Wait for valid signal
    if (hr < 50 || spo2 < 85)
    {
        lcd.setCursor(0, 1);
        lcd.print("Adjust Finger ");
        return;
    }

    // Sampling
    if (millis() - lastSampleTime > SAMPLE_INTERVAL)
    {
        lastSampleTime = millis();

        if (sampleCount < 20)
        {
            hrBuffer[sampleCount] = hr;
            spo2Buffer[sampleCount] = spo2;
            sampleCount++;

            lcd.setCursor(0, 0);
            lcd.print("Measuring...  ");

            lcd.setCursor(0, 1);
            lcd.print("Sample ");
            lcd.print(sampleCount);
            lcd.print("/20   ");
        }
    }

    // When 20 samples collected
    if (sampleCount == 20)
    {
        // Sort arrays
        sortArray(hrBuffer, 20);
        sortArray(spo2Buffer, 20);

        float sumHR = 0, sumSpO2 = 0;

        // Trim first 2 & last 2 values
        for (int i = 2; i < 18; i++)
        {
            sumHR += hrBuffer[i];
            sumSpO2 += spo2Buffer[i];
        }

        float avgHR = sumHR / 16;
        float avgSpO2 = sumSpO2 / 16;

        lcd.clear();

        // ❤️ HR
        lcd.setCursor(0, 0);
        lcd.write(byte(0));
        lcd.print((int)avgHR);
        lcd.print(" BPM");

        if (avgHR < 60)
            lcd.setCursor(13, 0), lcd.print("LOW");
        else if (avgHR > 100)
            lcd.setCursor(13, 0), lcd.print("HI ");
        else
            lcd.setCursor(13, 0), lcd.write(byte(2));

        // 🫁 SpO2
        lcd.setCursor(0, 1);
        lcd.write(byte(1));
        lcd.print((int)avgSpO2);
        lcd.print("%");

        if (avgSpO2 >= 95)
            lcd.setCursor(13, 1), lcd.print("OK ");
        else
            lcd.setCursor(13, 1), lcd.write(byte(3));

        measurementDone = true;

        Serial.println("===== FINAL (TRIMMED AVG) =====");
        Serial.print("HR: "); Serial.println(avgHR);
        Serial.print("SpO2: "); Serial.println(avgSpO2);
    }
}