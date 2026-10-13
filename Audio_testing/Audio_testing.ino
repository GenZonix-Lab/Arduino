#include "BluetoothA2DPSource.h"
#include <math.h>

BluetoothA2DPSource a2dp_source;

#define SAMPLE_RATE 44100
#define FREQUENCY   440.0
#define AMPLITUDE   12000

volatile uint32_t callbackCount = 0;

int32_t get_audio_data(Frame *frames, int32_t frame_count)
{
    static float phase = 0.0;

    const float twoPi = 2.0 * PI;
    const float phaseIncrement =
        twoPi * FREQUENCY / SAMPLE_RATE;

    for (int i = 0; i < frame_count; i++)
    {
        int16_t sample =
            (int16_t)(AMPLITUDE * sin(phase));

        frames[i].channel1 = sample;
        frames[i].channel2 = sample;

        phase += phaseIncrement;

        if (phase >= twoPi)
        {
            phase -= twoPi;
        }
    }

    callbackCount++;

    delay(1);

    return frame_count;
}


void setup()
{
    Serial.begin(115200);
    delay(1000);

    Serial.println();
    Serial.println("====================================");
    Serial.println(" ESP32 A2DP AUDIO DIAGNOSTIC TEST");
    Serial.println("====================================");

    // Register callback FIRST
    a2dp_source.set_data_callback_in_frames(
        get_audio_data
    );

    a2dp_source.set_volume(100);

    Serial.println("Starting A2DP...");
    Serial.println("Target: trüke Buds Lite");

    a2dp_source.start("trüke Buds Lite");

    Serial.println("A2DP started.");
}


void loop()
{
    static unsigned long lastTime = 0;

    if (millis() - lastTime >= 1000)
    {
        lastTime = millis();

        Serial.print("State: ");

        Serial.print(
            a2dp_source.to_str(
                a2dp_source.get_connection_state()
            )
        );

        Serial.print(" | Audio callbacks: ");
        Serial.println(callbackCount);
    }

    delay(10);
}