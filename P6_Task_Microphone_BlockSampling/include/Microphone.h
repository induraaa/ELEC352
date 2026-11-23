
#ifndef MICROPHONE_H
#define MICROPHONE_H

#include "stm32f4xx_hal.h"
#include <cstdint>
#include <cstddef>

class Microphone
{
public:
    Microphone();
    void Initialise();
    bool IsReady() const;
    bool ReadRaw(uint16_t& value);
    bool StartSampling(uint32_t sampleRateHz);
    bool Update(float samples[], std::size_t sampleCount);
    float GetSampleRateHz() const;

private:
    ADC_HandleTypeDef adc_;
    bool ready_;
    bool samplingStarted_;
    float* samples_;
    std::size_t sampleCount_;
    std::size_t index_;
    uint32_t periodCycles_;
    uint32_t nextSampleCycle_;
    float sampleRateHz_;
};

#endif
