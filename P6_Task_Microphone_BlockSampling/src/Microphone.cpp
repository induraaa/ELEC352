
#include "Microphone.h"

Microphone::Microphone()
    : adc_{},
      ready_(false),
      samplingStarted_(false),
      samples_(nullptr),
      sampleCount_(0U),
      index_(0U),
      periodCycles_(0U),
      nextSampleCycle_(0U),
      sampleRateHz_(0.0f)
{
}

void Microphone::Initialise()
{
    ready_ = false;
    samplingStarted_ = false;
    samples_ = nullptr;
    sampleCount_ = 0U;
    index_ = 0U;

    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_ADC1_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {};
    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;

    HAL_GPIO_Init(GPIOA, &gpio);

    adc_ = {};
    adc_.Instance = ADC1;
    adc_.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
    adc_.Init.Resolution = ADC_RESOLUTION_12B;
    adc_.Init.ScanConvMode = DISABLE;
    adc_.Init.ContinuousConvMode = DISABLE;
    adc_.Init.DiscontinuousConvMode = DISABLE;
    adc_.Init.NbrOfConversion = 1;
    adc_.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
    adc_.Init.ExternalTrigConv = ADC_SOFTWARE_START;
    adc_.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    adc_.Init.DMAContinuousRequests = DISABLE;
    adc_.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

    if (HAL_ADC_Init(&adc_) != HAL_OK)
        return;

    ADC_ChannelConfTypeDef channel = {};
    channel.Channel = ADC_CHANNEL_3;
    channel.Rank = 1;
    channel.SamplingTime = ADC_SAMPLETIME_84CYCLES;

    if (HAL_ADC_ConfigChannel(&adc_, &channel) != HAL_OK)
        return;

    ready_ = true;
}

bool Microphone::IsReady() const
{
    return ready_;
}

bool Microphone::ReadRaw(uint16_t& value)
{
    if (!ready_)
        return false;

    if (HAL_ADC_Start(&adc_) != HAL_OK)
        return false;

    if (HAL_ADC_PollForConversion(&adc_, 10U) != HAL_OK)
    {
        HAL_ADC_Stop(&adc_);
        return false;
    }

    const uint32_t raw = HAL_ADC_GetValue(&adc_);

    if (HAL_ADC_Stop(&adc_) != HAL_OK)
        return false;

    value = static_cast<uint16_t>(raw);
    return true;
}

bool Microphone::StartSampling(uint32_t sampleRateHz)
{
    samplingStarted_ = false;
    samples_ = nullptr;
    sampleCount_ = 0U;
    index_ = 0U;
    periodCycles_ = 0U;
    nextSampleCycle_ = 0U;
    sampleRateHz_ = 0.0f;

    if (!ready_ || sampleRateHz == 0U || SystemCoreClock == 0U)
        return false;

    const uint64_t cycles =
        (static_cast<uint64_t>(SystemCoreClock)
         + sampleRateHz / 2U) / sampleRateHz;

    if (cycles == 0U || cycles >= (1ULL << 31))
        return false;

    periodCycles_ = static_cast<uint32_t>(cycles);

    sampleRateHz_ =
        static_cast<float>(SystemCoreClock) /
        static_cast<float>(periodCycles_);

    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    samplingStarted_ = true;
    return true;
}

bool Microphone::Update(float samples[], std::size_t sampleCount)
{
    if (!samplingStarted_ || samples == nullptr || sampleCount == 0U)
        return false;

    if (samples_ != samples || sampleCount_ != sampleCount)
    {
        samples_ = samples;
        sampleCount_ = sampleCount;
        index_ = 0U;
        nextSampleCycle_ = DWT->CYCCNT;
    }

    const uint32_t now = DWT->CYCCNT;

    if (static_cast<int32_t>(now - nextSampleCycle_) < 0)
        return false;

    uint16_t value = 0U;

    if (!ReadRaw(value))
    {
        samples_ = nullptr;
        sampleCount_ = 0U;
        index_ = 0U;
        return false;
    }

    samples_[index_] = static_cast<float>(value);
    ++index_;

    nextSampleCycle_ += periodCycles_;

    const uint32_t after = DWT->CYCCNT;

    if (static_cast<int32_t>(after - nextSampleCycle_) >= 0)
        nextSampleCycle_ = after + periodCycles_;

    if (index_ == sampleCount_)
    {
        samples_ = nullptr;
        sampleCount_ = 0U;
        index_ = 0U;
        return true;
    }

    return false;
}

float Microphone::GetSampleRateHz() const
{
    return sampleRateHz_;
}
