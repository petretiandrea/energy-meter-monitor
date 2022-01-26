
struct EnergyMeterMeasure
{
    float current_consumption;
    uint64_t pulse_count;
};

namespace EnergyMeterMeasureFactory {
    EnergyMeterMeasure createBy(PulseCounterData* data) {
        auto pulse_width_ms = (data->last_pulse_width_us / 1000.0f);
        auto power_consumption = data->last_pulse_width_us > 0 ? (60.0f * 1000.0f) / (pulse_width_ms) : 0.0f;
        return {
            .current_consumption = power_consumption,
            .pulse_count = data->pulse_count
        };
    }
}