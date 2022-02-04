#include <string.h>
#include <string>
#include <utility>
#include "protocol/edge/Protocol.h"

struct EnergyMeterMeasure
{
    float current_consumption;
    uint64_t pulse_count;
};

namespace parser
{
    using namespace protocol::edge;

    DataMessage *toDataMessage(const EnergyMeterMeasure &energyMeterMeasure)
    {
        auto *message = new DataMessage();
        printf("Consumption: %f\n", energyMeterMeasure.current_consumption);
        sprintf(message->data, "%f;%lld", energyMeterMeasure.current_consumption, energyMeterMeasure.pulse_count);
        return message;
    }

    EnergyMeterMeasure *fromDataMessage(const DataMessage &dataMessage)
    {
        EnergyMeterMeasure *energyMeterMeasure = new EnergyMeterMeasure();
        sscanf(dataMessage.data, "%f;%lld", &energyMeterMeasure->current_consumption, &energyMeterMeasure->pulse_count);
        return energyMeterMeasure;
    }
}

namespace EnergyMeterMeasureFactory {
    
    EnergyMeterMeasure createBy(PulseCounterData* data, int pulse_rate_kwh) {
        auto pulse_width_ms = (data->last_pulse_width_us / 1000.0f);
        auto power_consumption = data->last_pulse_width_us > 0 ? (60.0f * 1000.0f) / (pulse_width_ms) : 0.0f;
        return {
            .current_consumption = power_consumption * ((60 / pulse_rate_kwh) / 1000.0f),
            .pulse_count = data->pulse_count
        };
    }
}