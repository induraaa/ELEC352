
#ifndef VECTOR_SIGNAL_GENERATOR_H
#define VECTOR_SIGNAL_GENERATOR_H

class VectorSignalGenerator
{
public:
    static constexpr unsigned VECTOR_SIZE = 16U;

    void GenerateNext(float values[]);

private:
    unsigned frameNumber_ = 0U;
};

#endif
