#include "mbed.h"
#include <chrono>
#include <cstdio>

#include "Adafruit_SSD1306.h"

I2C i2c1(D0, D1);
Adafruit_SSD1306_I2c oled(i2c1, D2);

BufferedSerial ser(USBTX, USBRX, 115200);
FileHandle *mbed::mbed_override_console(int fd) { return &ser; }

#define NSAMPLES 2048
#define VMAX 3.3f
#define R1 10000 // 10k

uint16_t samplebuf[NSAMPLES];
int sample_i;

float averageVoltage() {
    // Calculate average ADC reading over the last 64 samples
    uint32_t average = 0;
    for (int i = 0; i < NSAMPLES; ++i) average += samplebuf[i];
    average /= NSAMPLES;

    return (float(average) / float(2 << 15)) * VMAX;
}

void sampleVoltage(AnalogIn input) {
    sample_i = (sample_i + 1) % NSAMPLES;
    samplebuf[sample_i] = input.read_u16();
}

float voltageToWeight(float voltage) {
    float R2 = (voltage * float(R1)) / (VMAX - voltage); // Resistance of FSR
    float G2 = 1000.f / R2; // Conductance of FSR in millisiemens (x1000)

    #define x G2
    float w = -34.2f + 2008.f*G2 - 2715.f*G2*G2 + 7159.f*G2*G2*G2; // Weight calculated by regression of weight over conductance on google sheets
    return roundf(w * 10.f) / 10.f; // Round weight to nearest tenth
}

int main() {
    AnalogIn vsense(A0);
    vsense.set_reference_voltage(VMAX);

    i2c1.frequency(1000000);
    oled.setTextWrap(false);

    // TODO: Running average
    while (true) {
        sampleVoltage(vsense);

        if (sample_i == 0) {
            float vin = averageVoltage();
            float weight = voltageToWeight(vin);

            oled.clearDisplay();
            printf("%f\n", vin);
            oled.setTextCursor(0, 0);
            oled.printf("\nRaw: %.4fv", vin);
            oled.printf("\nWeight (g): %.4f", weight);
            oled.display();
        }
    }
}

