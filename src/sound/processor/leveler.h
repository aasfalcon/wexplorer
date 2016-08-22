#ifndef SOUND_PROCESSOR_LEVELER_H
#define SOUND_PROCESSOR_LEVELER_H

#include "processor.h"

namespace Sound {

SOUND_PROPERTIES(Leveler, Processor,
    Balance_double,
    Level_double);

namespace Processor {
    template <typename T>
    class Leveler : public Processor<T> {
    public:
        Leveler();
        ~Leveler() override;

    protected:
        virtual void process() override;
    };

    SOUND_INSTANTIATION_DECLARE(Leveler);
}
}

#endif // SOUND_PROCESSOR_LEVELER_H
