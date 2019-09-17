#include "ESP32_Servo.h"
#include <TM1637Display.h>
#include "WiFi.h"
#include <HTTPClient.h>

// wifi
const char* ssid     = "Spider";
const char* password = "12345679";

// display
#define CLK 21
#define DIO 22
uint8_t degree_sign = 0x63;
TM1637Display disp(CLK, DIO);

// servo
#define SERVO 19
Servo srv;

// led
#define LED_R 25
#define LED_G 26
#define LED_B 27

// pump
#define PUMP1 33
#define PUMP2 32
#define PUMP3 35
#define PUMP4 34
#define PUMP5 14

#define FULL_GLASS 60000

int last_temperature = 0;
int last_r = 255;
int last_g = 255;
int last_b = 255;

int positions[] = { 0, 45, 90, 135, 180 };
int doses[] = { 0, 0, 0, 0, 0 };
int servo_pos = 90;

void setup()
{
    Serial.begin(115200);
    // wifi
    WiFi.begin(ssid, password);
    
    // display
    disp.clear();
    disp.setBrightness(0x0f);
    uint8_t hi[] = { 0x40, 0x76, 0x06, 0x40 };
    disp.setSegments(hi);

    // servo
    pinMode(SERVO, OUTPUT);
    srv.attach(SERVO);
    setServo(90);

    // led
    ledcSetup(0, 5000, 8);
    ledcAttachPin(LED_R, 0);

    ledcSetup(1, 5000, 8);
    ledcAttachPin(LED_G, 1);
    
    ledcSetup(2, 5000, 8);
    ledcAttachPin(LED_B, 2);
    
    setRGB();

    // pump pins
    pinMode(PUMP1, OUTPUT);
    digitalWrite(PUMP1, LOW);
    
    pinMode(PUMP2, OUTPUT);
    digitalWrite(PUMP2, LOW);
    
    pinMode(PUMP3, OUTPUT);
    digitalWrite(PUMP3, LOW);
    
    pinMode(PUMP4, OUTPUT);
    digitalWrite(PUMP4, LOW);
    
    pinMode(PUMP5, OUTPUT);
    digitalWrite(PUMP5, LOW);

    // load
    delay(1000);
    uint8_t load[] = { 0x38, 0x3f, 0x77, 0x5e };
    disp.setSegments(load);
}

void loop()
{
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        // disp.clear();
        Serial.println("connecting...");
        uint8_t conn[] = { 0x58, 0x5c, 0x54, 0x54 };
        disp.setSegments(conn);
    }

    HTTPClient http;
 
    http.begin("http://cocktail.justanother.app/current");
    int httpCode = http.GET();
 
    if (httpCode == 200) { //Check for the returning code
        String input = http.getString();
        Serial.println("Got: " + input);
        // colors
        last_r = getValue(input, '|', 1).toInt();
        last_g = getValue(input, '|', 2).toInt();
        last_b = getValue(input, '|', 3).toInt();
        setRGB();
    
        // temperature
        last_temperature = getValue(input, '|', 0).toInt();
    
        // set value to display
        // disp.clear();
        uint8_t temperature[] = { 0x00, 0x00, 0x00, 0x00 };
        int first_digit = floor(last_temperature / 10.0);
        int second_digit = last_temperature - first_digit * 10;

        if (first_digit > 0) {
            temperature[1] = disp.encodeDigit(first_digit);
            temperature[2] = disp.encodeDigit(second_digit);
            temperature[3] = degree_sign;
        } else {
            temperature[1] = disp.encodeDigit(second_digit);
            temperature[2] = degree_sign;
        }
        disp.setSegments(temperature);
    
        // run
        int is_run = getValue(input, '|', 4).toInt();
        if(is_run > 0) {
            Serial.println("...Start sequence");
            calculateDoses();
    
            for(int k = 0; k < 5; k++) {
                int dose = doses[k];
                Serial.print("dose: "); Serial.print(k); Serial.print(", val: "); Serial.println(dose);
                if (dose > 0) {
                    delay(setServo(positions[k]));
                    changeLedTo(k);
                    turnOnPump(k);
                    delay(dose);
                    turnOffPumps();
                    delay(100);
                }
            }
            
            returnToZero();
            delay(1000);
        }

        http.end();
        delay(500);
    } else {
        // disp.clear();
        uint8_t err[] = { 0x7b, 0x50, 0x50, 0x0 };
        disp.setSegments(err);
        Serial.println("error");
        http.end();
        delay(3000);
    }
}

String getValue(String s, char separator, int index)
{
    int found = 0;
    int strIndex[] = { 0, -1 };
    int maxIndex = s.length() - 1;

    for (int i = 0; i <= maxIndex && found <= index; i++) {
        if (s.charAt(i) == separator || i == maxIndex) {
            found++;
            strIndex[0] = strIndex[1] + 1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found > index ? s.substring(strIndex[0], strIndex[1]) : "";
}

void setRGB() {
    setRGB(last_r, last_g, last_b);
}

void setRGB(int r, int g, int b) {
    Serial.print("Set rgb: ");
    Serial.print(r); Serial.print(", ");
    Serial.print(g); Serial.print(", ");
    Serial.println(b);
    
    ledcWrite(0, floor(r * 0.86));
    ledcWrite(1, floor(g * 0.5));
    ledcWrite(2, b);
}

void changeLedTo(int i) {
    switch (i) {
        case 0:
            // vodka
            setRGB(255, 255, 0);
            break;
        case 1:
            // water
            setRGB(255, 255, 255);
            break;
        case 2:
            // r
            setRGB(255, 0, 0);
            break;
        case 3:
            // g
            setRGB(0, 255, 0);
            break;
        case 4:
            // b
            setRGB(0, 0, 255);
            break;
    }
}

void calculateDoses() {
    float hsv[3];
    float rgb[3];
    rgb2hsv(last_r * 1.0 / 255.0, last_g * 1.0 / 255.0, last_b * 1.0 / 255.0, hsv);
    hsv2rgb(hsv[0], hsv[1], 1.0, rgb);

    float vodka = last_temperature * 1.0 / 40.0;
    if (vodka > 1.0) {
        vodka = 1.0;
    }

    float other = 1.0 - vodka;
    float bright = 1.0 - hsv[1];
    float water = other * bright;
    float color = 1.0 - vodka - water;
    float summ = rgb[0] + rgb[1] + rgb[2];
    if (summ == 0.0) {
        summ = 1.0;
    }
    float r = rgb[0] / summ * color;
    float g = rgb[1] / summ * color;
    float b = rgb[2] / summ * color;

    // set
    doses[0] = floor(FULL_GLASS * vodka);
    doses[1] = floor(FULL_GLASS * water);
    doses[2] = floor(FULL_GLASS * r);
    doses[3] = floor(FULL_GLASS * g);
    doses[4] = floor(FULL_GLASS * b);

    Serial.println("Doses: ");
    Serial.println(doses[0]);
    Serial.println(doses[1]);
    Serial.println(doses[2]);
    Serial.println(doses[3]);
    Serial.println(doses[4]);
}

void hsv2rgb(float h, float s, float b, float* rgb) {
  rgb[0] = b * mix(1.0, constrain(abs(fract(h + 1.0) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  rgb[1] = b * mix(1.0, constrain(abs(fract(h + 0.6666666) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
  rgb[2] = b * mix(1.0, constrain(abs(fract(h + 0.3333333) * 6.0 - 3.0) - 1.0, 0.0, 1.0), s);
}

void rgb2hsv(float r, float g, float b, float* hsv) {
    float s = step(b, g);
    float px = mix(b, g, s);
    float py = mix(g, b, s);
    float pz = mix(-1.0, 0.0, s);
    float pw = mix(0.6666666, -0.3333333, s);
    s = step(px, r);
    float qx = mix(px, r, s);
    float qz = mix(pw, pz, s);
    float qw = mix(r, px, s);
    float d = qx - min(qw, py);
    hsv[0] = abs(qz + (qw - py) / (6.0 * d + 1e-10));
    hsv[1] = d / (qx + 1e-10);
    hsv[2] = qx;
}

float fract(float x) { return x - int(x); }
float step(float e, float x) { return x < e ? 0.0 : 1.0; }
float mix(float a, float b, float t) { return a + (b - a) * t; }

void turnOffPumps() {
    Serial.println("off pumps");
    digitalWrite(PUMP1, LOW);
    digitalWrite(PUMP2, LOW);
    digitalWrite(PUMP3, LOW);
    digitalWrite(PUMP4, LOW);
    digitalWrite(PUMP5, LOW);
}

void turnOnPump(int i) {
    Serial.print("On pump "); Serial.println(i);
    switch (i) {
        case 0:
            digitalWrite(PUMP1, HIGH);
            break;
        case 1:
            digitalWrite(PUMP2, HIGH);
            break;
        case 2:
            digitalWrite(PUMP3, HIGH);
            break;
        case 3:
            digitalWrite(PUMP4, HIGH);
            break;
        case 4:
            digitalWrite(PUMP5, HIGH);
            break;
    }
}

void returnToZero() {
    Serial.println("...return to zero");
    turnOffPumps();
    setServo(positions[2]);

    // display show done
    disp.clear();
    uint8_t done[] = { 0x5e, 0x3f, 0x37, 0x7b };
    disp.setSegments(done);

    // blink twice
    for(int i = 0; i < 2; i++){
        setRGB(255, 255, 255); 
        delay(500);
        setRGB(0, 0, 0);
        delay(500);
    }
    setRGB();
}

int setServo(int p) {
    int diff = abs(p - servo_pos);
    servo_pos = p;
    srv.write(p);
    
    Serial.print("Set servo: ");
    Serial.print(p);
    Serial.print(", delay: ");
    Serial.println(diff * 10);
    
    return diff * 10;
}
