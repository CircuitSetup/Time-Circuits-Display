#include "tc_keypad.h"
#include "tc_time.h"
#include "tc_audio.h"

const char keys[4][3] = {
    {'1', '2', '3'},
    {'4', '5', '6'},
    {'7', '8', '9'},
    {'*', '0', '#'}};

byte rowPins[4] = {1, 6, 5, 3};  //connect to the row pinouts of the keypad
byte colPins[3] = {2, 0, 4};     //connect to the column pinouts of the keypad
Keypad_I2C keypad(makeKeymap(keys), rowPins, colPins, 4, 3, KEYPAD_ADDR, PCF8574);

int enterState = 0;
static unsigned int dateIndex = 0;
const int maxDateLength = 12; //month, day, year, hour, min
const int minDateLength = 8; //month, day, year
char dateBuffer[maxDateLength + 1];
boolean dateComplete = false;

unsigned long keyPrevMillis = 0;
const unsigned long keySampleIntervalMs = 25;
byte longKeyPressCountMax = 80;    // 80 * 25 = 2000 ms
byte longKeyPressCount = 0;

byte prevKeyState = HIGH;         // button is active low

boolean keyPressed = false;
boolean menuFlag = false;

void keypad_setup() {
    keypad.begin(makeKeymap(keys));
    keypad.addEventListener(keypadEvent);  //add an event listener for this keypad
    keypad.setHoldTime(BUTTON_HOLD_TIME);  // 3 sec hold time
    byte hold = keypad.getState();
    char key = keypad.getKey();

    //setup pins for white LED and Enter key
    pinMode(WHITE_LED, OUTPUT);
    digitalWrite(WHITE_LED, LOW); //turn on white LEDs
    pinMode(ENTER_BUTTON, INPUT_PULLDOWN);
    digitalWrite(ENTER_BUTTON, LOW);

    Serial.println("Keypad Setup");
}

char get_key() {
    char getKey = keypad.getKey();
    //Serial.print("Key: ");
    //Serial.println(getKey);
    return getKey;
}

boolean isKeyHeld(char key) {
    if (key == keypad.getKey() && ('HOLD' == keypad.getState())) {
        return true;
    } else {
        return false;
    }
}

//handle different cases for keypresses
void keypadEvent(KeypadEvent key) {
    switch (keypad.getState()) {
        case PRESSED:
            if (key != '#') {
                play_keypad_sound(key);
                keyPressed = true;

                recordKey(key);
                break;
            }
            if (menuFlag) {
                switch (key) {
                    case '2':
                        Serial.println("ButtonUp");
                        //digitalWrite(buttonUp, LOW);
                        break;
                    case '8':
                        Serial.println("ButtonDown");
                        //digitalWrite(buttonDown, LOW);
                        break;
                }
            }
            break;
        case RELEASED:
            if (menuFlag) {
                switch (key) {
                    case '2':
                        //Serial.println("ButtonUp Release");
                        //digitalWrite(buttonUp, HIGH);
                        break;
                    case '8':
                        //Serial.println("ButtonDown Release");
                        //digitalWrite(buttonDown, HIGH);
                        break;
                }
            }
            break;
        case HOLD:
            switch (key) {
                case '0':
                    menuFlag = true;
                    enter_menu();
                    break;
            }
            break;
        case IDLE:
            break;
    }
}

boolean isEnterKeyPressed() {
    //Serial.print(digitalRead(ENTER_BUTTON));
    enterState = digitalRead(ENTER_BUTTON);
    if (enterState == HIGH) {
        Serial.println("Enter Key Pressed");
        return true;
    } else {
        return false;
    }
}

boolean isEnterKeyHeld() {
    if (millis() - keyPrevMillis >= keySampleIntervalMs) {
        keyPrevMillis = millis();
       
        enterState = digitalRead(ENTER_BUTTON);
       
        if ((prevKeyState == HIGH) && (enterState == LOW)) {
            longKeyPressCount = 0;
        }
        else if ((prevKeyState == LOW) && (enterState == HIGH)) {
            if (longKeyPressCount >= longKeyPressCountMax) {
                //enter menu
                return true;
                Serial.println("Enter Menu");
            }
            else {
                return false;
            }
        }
        else if (enterState == LOW) {
            longKeyPressCount++;
        }
       
        prevKeyState = enterState;
    }
}

void recordKey(char key) {
    dateBuffer[dateIndex++] = key;
    dateBuffer[dateIndex] = '\0';                                   // to maintain a c-string style
    if (dateIndex >= maxDateLength) dateIndex = maxDateLength - 1;  // don't overflow, will overwrite end of date next time
}

void keypadLoop() {
    if (!isEnterKeyHeld()) {
        Serial.println(F("Enter Key Held"));
        //enter_menu();
    }
    if (isEnterKeyPressed()) {
        play_file("/enter.mp3");
        digitalWrite(WHITE_LED, HIGH); //turn on white LEDs
        allOff();
        delay(1200);

        if (strlen(dateBuffer) > maxDateLength) {
            Serial.println(F("Date is too long, try again"));
            dateIndex = 0; //reset
        }
        else if (strlen(dateBuffer) < minDateLength) {
            Serial.println(F("Date is too short, try again"));
            dateIndex = 0; //reset
        }
        else {
            dateComplete = true;
            Serial.print(F("date entered: ["));
            Serial.print(dateBuffer);
            Serial.println(F("]"));

            String dateBufferString(dateBuffer); //convert char to String so substring can be used

            //copy dates in dateBuffer and make ints
            int _setMonth = dateBufferString.substring(0, 2).toInt();
            int _setDay = dateBufferString.substring(2, 4).toInt();
            int _setYear = dateBufferString.substring(4, 8).toInt();
            int _setHour = dateBufferString.substring(8, 10).toInt();
            int _setMin = dateBufferString.substring(10, 12).toInt();

            //check if day makes sense for the month entered. Also checks if it is a leap year.
            if (_setDay > daysInMonth(_setMonth, _setYear)) {
                _setDay = daysInMonth(_setMonth, _setYear);
            }
            //set date to destination time
            destinationTime.setMonth(_setMonth);
            destinationTime.setDay(_setDay);
            destinationTime.setYear(_setYear);
            destinationTime.setHour(_setHour);
            destinationTime.setMinute(_setMin);
            destinationTime.save();

            dateIndex = 0;  // prepare for next time
        }
        animate(); //turn everything back on
        digitalWrite(WHITE_LED, LOW); //turn off white LEDs
    }

}