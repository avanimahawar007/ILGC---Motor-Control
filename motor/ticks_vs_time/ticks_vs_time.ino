
#include <Arduino.h>
#include <mbed.h>
#include <math.h>

// ==========================================
// PIN DEFINITIONS
// ==========================================

#define MOTOR_PWM_PIN 1
#define MOTOR_DIR_PIN 0

#define ENC_PIN_A 3
#define ENC_PIN_B 2

// ==========================================
// EXPERIMENT SETTINGS
// ==========================================

#define MOTOR_SPEED 50.0f

#define SAMPLE_INTERVAL_US 100
#define BASELINE_US 100000
#define PULSE_DURATION_US 250000
#define POST_STOP_US 250000

#define MAX_SAMPLES 6100

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

mbed::PwmOut motor_pwm(
    digitalPinToPinName(MOTOR_PWM_PIN)
);

void motor_init() {

    pinMode(MOTOR_DIR_PIN, OUTPUT);
    digitalWrite(MOTOR_DIR_PIN, LOW);

    // 20 kHz PWM
    motor_pwm.period_us(50);

    // Motor initially OFF
    motor_pwm.write(0.0f);
}

// ==========================================
// MOTOR SPEED CONTROL
// ==========================================

void motor_set_speed(float percent) {

    percent = constrain(
        percent,
        -100.0f,
        100.0f
    );

    motor_pwm.write(0.0f);

    digitalWrite(
        MOTOR_DIR_PIN,
        percent < 0.0f ? HIGH : LOW
    );

    float duty = fabsf(percent) / 100.0f;

    motor_pwm.write(duty);
}

// ==========================================
// ENCODER INTERRUPT
// ==========================================

void encoder_isr() {

    uint8_t state =
        (digitalRead(ENC_PIN_A) << 1) |
        digitalRead(ENC_PIN_B);

    ticks += step_table[
        (prev_state << 2) | state
    ];

    prev_state = state;
}

// ==========================================
// ENCODER INITIALIZATION
// ==========================================

void encoder_init() {

    pinMode(ENC_PIN_A, INPUT_PULLUP);
    pinMode(ENC_PIN_B, INPUT_PULLUP);

    prev_state =
        (digitalRead(ENC_PIN_A) << 1) |
        digitalRead(ENC_PIN_B);

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

    int32_t count = ticks;

    interrupts();

    return count;
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
// DATA STORAGE
// ==========================================

struct Sample {
    uint32_t time_us;
    int32_t encoder_ticks;
};

Sample data[MAX_SAMPLES];

uint32_t sample_count = 0;
uint32_t missed_samples = 0;

// PWM event timestamps
uint32_t pwm_on_time = 0;
uint32_t pwm_off_time = 0;

// Encoder ticks at PWM events
int32_t pwm_on_ticks = 0;
int32_t pwm_off_ticks = 0;

// ==========================================
// MAIN SETUP
// ==========================================

void setup() {

    Serial.begin(115200);

    delay(5000);

    motor_init();
    encoder_init();
    encoder_reset();

    // ======================================
    // START EXPERIMENT
    // ======================================

    uint32_t start_time = micros();

    uint32_t next_sample = start_time;

    // Motor states:
    // 0 = baseline
    // 1 = motor ON
    // 2 = motor OFF
    int motor_state = 0;

    while (true) {

        uint32_t now = micros();
        uint32_t elapsed = now - start_time;

        // ==================================
        // MOTOR ON AFTER 100 ms
        // ==================================

        if (motor_state == 0) {

            if (elapsed >= BASELINE_US) {

                motor_set_speed(MOTOR_SPEED);

                pwm_on_time =
                    micros() - start_time;

                pwm_on_ticks =
                    encoder_get_count();

                motor_state = 1;
            }
        }

        // ==================================
        // MOTOR OFF AFTER 250 ms
        // ==================================

        else if (motor_state == 1) {

            if (
                elapsed - pwm_on_time
                >= PULSE_DURATION_US
            ) {

                motor_set_speed(0);

                pwm_off_time =
                    micros() - start_time;

                pwm_off_ticks =
                    encoder_get_count();

                motor_state = 2;
            }
        }

        // ==================================
        // FINISH 250 ms AFTER MOTOR OFF
        // ==================================

        else if (motor_state == 2) {

            if (
                elapsed - pwm_off_time
                >= POST_STOP_US
            ) {
                break;
            }
        }

        // ==================================
        // SAMPLE ENCODER AT 10 kHz
        // ==================================

        if (
            (int32_t)(now - next_sample)
            >= 0
        ) {

            if (sample_count >= MAX_SAMPLES) {
                break;
            }

            int32_t current_ticks =
                encoder_get_count();

            uint32_t sample_time =
                micros() - start_time;

            data[sample_count].time_us =
                sample_time;

            data[sample_count].encoder_ticks =
                current_ticks;

            sample_count++;

            // Next sample after 100 us
            next_sample += SAMPLE_INTERVAL_US;

            // Check for missed samples
            uint32_t after_sample = micros();

            if (
                (int32_t)(
                    after_sample - next_sample
                ) >= 0
            ) {

                uint32_t skipped =
                    (after_sample - next_sample)
                    / SAMPLE_INTERVAL_US + 1;

                missed_samples += skipped;

                next_sample +=
                    skipped * SAMPLE_INTERVAL_US;
            }
        }
    }

    // ======================================
    // ENSURE MOTOR IS OFF
    // ======================================

    if (motor_state != 2) {

        motor_set_speed(0);

        pwm_off_time =
            micros() - start_time;

        pwm_off_ticks =
            encoder_get_count();
    }

    // ======================================
    // PRINT CSV AFTER EXPERIMENT
    // ======================================

    Serial.println(
        "Time_us,Encoder_Ticks,PWM_percent,Event"
    );

    bool on_printed = false;
    bool off_printed = false;

    for (
        uint32_t i = 0;
        i < sample_count;
        i++
    ) {

        uint32_t t = data[i].time_us;

        // ==================================
        // PWM ON EVENT
        // ==================================

        if (
            !on_printed &&
            t >= pwm_on_time
        ) {

            Serial.print(pwm_on_time);
            Serial.print(",");

            Serial.print(pwm_on_ticks);
            Serial.println(",50,PWM_ON");

            on_printed = true;
        }

        // ==================================
        // PWM OFF EVENT
        // ==================================

        if (
            !off_printed &&
            t >= pwm_off_time
        ) {

            Serial.print(pwm_off_time);
            Serial.print(",");

            Serial.print(pwm_off_ticks);
            Serial.println(",0,PWM_OFF");

            off_printed = true;
        }

        // ==================================
        // PWM STATE DURING SAMPLE
        // ==================================

        int pwm = 0;

        if (
            t >= pwm_on_time &&
            t < pwm_off_time
        ) {
            pwm = 50;
        }

        // ==================================
        // PRINT ENCODER SAMPLE
        // ==================================

        Serial.print(t);
        Serial.print(",");

        Serial.print(data[i].encoder_ticks);
        Serial.print(",");

        Serial.print(pwm);
        Serial.println(",SAMPLE");
    }

    // ======================================
    // PRINT ANY REMAINING EVENTS
    // ======================================

    if (!on_printed && motor_state != 0) {

        Serial.print(pwm_on_time);
        Serial.print(",");

        Serial.print(pwm_on_ticks);
        Serial.println(",50,PWM_ON");
    }

    if (!off_printed) {

        Serial.print(pwm_off_time);
        Serial.print(",");

        Serial.print(pwm_off_ticks);
        Serial.println(",0,PWM_OFF");
    }

    // ======================================
    // EXPERIMENT SUMMARY
    // ======================================

    Serial.println("END_OF_DATA");

    Serial.print("PWM ON (us): ");
    Serial.println(pwm_on_time);

    Serial.print("PWM OFF (us): ");
    Serial.println(pwm_off_time);

    Serial.print("Actual pulse duration (us): ");
    Serial.println(
        pwm_off_time - pwm_on_time
    );

    Serial.print("Samples recorded: ");
    Serial.println(sample_count);

    Serial.print("Missed sampling slots: ");
    Serial.println(missed_samples);

    Serial.print("Final encoder ticks: ");
    Serial.println(encoder_get_count());
}

// ==========================================
// MAIN LOOP
// ==========================================

void loop() {

    // Experiment runs once inside setup()
}
