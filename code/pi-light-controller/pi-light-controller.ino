    #include <LiquidCrystal.h>

    LiquidCrystal lcd(12, 11, 5, 4, 3, 2);

    const uint8_t POT_PIN = A0;
    const uint8_t SENS_PIN = A1;
    const uint8_t LED_PIN = 9;

    const float KP = 1.0;
    const float KI = 5.0;
    const unsigned long DT_US = 5000;
    const float DT = DT_US / 1e6;

    struct Btn { uint8_t pin; bool prev; unsigned long t; };
    Btn viewBtn = {7, HIGH, 0};
    Btn modeBtn = {8, HIGH, 0};

    bool showPot = true;
    bool closedLoop = true;
    float integ = 0;
    float r = 0, y = 0, u = 0;

    float readVolts(uint8_t pin) {
        long s = 0;
        for (uint8_t i = 0; i < 4; i++) s += analogRead(pin);
        return s * 5.0 / (4 * 1023.0);
    }

    bool pressed(Btn &b) {
        bool now = digitalRead(b.pin);
        if (now != b.prev && millis() - b.t > 30) {
            b.t = millis();
            b.prev = now;
            return now == LOW;
        }
        return false;
    }

    void setup() {
        pinMode(LED_PIN, OUTPUT);
        pinMode(viewBtn.pin, INPUT_PULLUP);
        pinMode(modeBtn.pin, INPUT_PULLUP);
        lcd.begin(16, 2);
        Serial.begin(115200);
    }

    void loop() {
        static unsigned long lastCtl = 0, lastLcd = 0;

        if (pressed(viewBtn)) showPot = !showPot;
        if (pressed(modeBtn)) closedLoop = !closedLoop;

        if (micros() - lastCtl >= DT_US) {
            lastCtl += DT_US;
            r = readVolts(POT_PIN);
            y = readVolts(SENS_PIN);

            if (closedLoop) {
                float e = r - y;
                integ += KI * e * DT;
                integ = constrain(integ, 0.0, 1.0);   // anti-windup
                u = KP * e + integ;
            } else {
                u = r / 5.0;      // pot drives the LED directly
                integ = u;        // bumpless switch back to PI
            }
            u = constrain(u, 0.0, 1.0);
            analogWrite(LED_PIN, (int)(u * 255.0 + 0.5));
        }

        if (millis() - lastLcd >= 200) {
            lastLcd = millis();
            lcd.setCursor(0, 0);
            lcd.print(showPot ? "Pot input       " : "Light sensor    ");
            lcd.setCursor(0, 1);
            lcd.print(showPot ? r : y, 2);
            lcd.print(" V     ");
            lcd.setCursor(12, 1);
            lcd.print(closedLoop ? "PI  " : "OPEN");
            Serial.print(r); Serial.print(",");
            Serial.print(y); Serial.print(",");
            Serial.println(u * 5.0);
        }
    }