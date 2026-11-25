
#include <Arduino.h>
#include <mbed.h>
// ==========================================
// PIN DEFINITIONS
// ==========================================

#define MOTOR_PWM_PIN 1
#define MOTOR_DIR_PIN 0

//ITS SWAPPED FOR A REASON DON'T CHANGE IT
#define ENC_PIN_A 3
#define ENC_PIN_B 2

// ==========================================
// MOTOR SETTINGS
// ==========================================

#define PWM_FREQ_HZ 20000
#define PWM_RANGE 6249

#define TARGET_TICKS 2707.0
// 97 is a random number right now
// It is rounded down. Ticks for 1 rev is . 2707

// ==========================================
// ENCODER VARIABLES
// ==========================================

volatile int32_t ticks = 0;
volatile uint8_t prev_state = 0;

const int8_t step_table[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
};

// ==========================================
// MOTOR INITIALIZATION
// ==========================================

// Create PWM output on GP1
mbed::PwmOut motor_pwm(digitalPinToPinName(MOTOR_PWM_PIN));

void motor_init() {
    pinMode(MOTOR_DIR_PIN, OUTPUT);
    digitalWrite(MOTOR_DIR_PIN, LOW);
    // Set PWM period to 50 microseconds = 20 kHz
    motor_pwm.period_us(50);
    // Initially stop motor
    motor_pwm.write(0.0f);
}

// input to speed is a percentage
void motor_set_speed(int percent) {
    percent = constrain(percent, -100, 100);
    digitalWrite(MOTOR_DIR_PIN, percent < 0);
    int magnitude = abs(percent);
    // Convert percentage to duty cycle (0.0 to 1.0)
    float duty = magnitude / 100.0f;
    motor_pwm.write(duty);
}
// ==========================================
// ENCODER INTERRUPT SERVICE ROUTINE
// ==========================================

void encoder_isr() {

    uint8_t state =
        (digitalRead(ENC_PIN_A) << 1) |
        digitalRead(ENC_PIN_B);

    ticks += step_table[(prev_state << 2) | state];

    prev_state = state;
}

// ==========================================
// ENCODER INITIALIZATION
// ==========================================

void encoder_init() {

    pinMode(ENC_PIN_A, INPUT_PULLUP);
    pinMode(ENC_PIN_B, INPUT_PULLUP);

    // Read initial encoder state
    prev_state =
        (digitalRead(ENC_PIN_A) << 1) |
        digitalRead(ENC_PIN_B);

    // Interrupts on both encoder channels
    attachInterrupt(
        digitalPinToInterrupt(ENC_PIN_A),
        encoder_isr,
        CHANGE
    );

    attachInterrupt(
        digitalPinToInterrupt(ENC_PIN_B),
        encoder_isr,
        CHANGE
    );
}

// ==========================================
// GET ENCODER COUNT
// ==========================================

int32_t encoder_get_count() {

    noInterrupts();
    int32_t current_ticks = ticks;
    interrupts();

    return current_ticks;
}

// ==========================================
// RESET ENCODER
// ==========================================

void encoder_reset() {

    noInterrupts();
    ticks = 0;
    interrupts();
}

// ==========================================
// MAIN SETUP
// ==========================================

void setup() {
    Serial.begin(115200);
    // Allow time for Serial connection
    delay(5000);

    motor_init();

    encoder_init();

    encoder_reset();


    // ======================================
    // MAIN LOOP (move till number of ticks achieved)
    // ======================================

    while (encoder_get_count() != TARGET_TICKS) {
        int32_t current_ticks = encoder_get_count();
        int32_t difference = TARGET_TICKS - current_ticks;
        int32_t speed;
        if (difference>0) {
            speed = (difference/(float)(TARGET_TICKS)*95.0f + 5.0f);
        }
        else{
            speed = (difference/(float)(TARGET_TICKS)*95.0f - 5.0f);
        }
        //Serial.print("speed: ");
        //Serial.println(TARGET_TICKS);
        motor_set_speed(speed); 
        //Serial.print(" L:");
        //Serial.print(0);

        //Serial.print(" U:");
        //Serial.print(200);

        Serial.print("target:");
        Serial.print(TARGET_TICKS);

        Serial.print(" ticks:");
        Serial.println(current_ticks);

        delay(10);
    }

    // ======================================
    // STOP MOTOR
    // ======================================

    motor_set_speed(0);

    // ======================================
    // RECORD 800 SAMPLES AFTER STOPPING
    // ======================================

    for (int i = 0; i < 800; i++) {

        int32_t current_ticks = encoder_get_count();

        Serial.print(" L:");
        Serial.print(0);

        Serial.print(" U:");
        Serial.print(200);

        Serial.print(" target:");
        Serial.print(TARGET_TICKS);

        Serial.print(" ticks:");
        Serial.println(current_ticks);

        delay(10);
    }
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

    // Nothing else to do.
    // The experiment runs once inside setup().
}
