#include "main_window_logic.h"

#include "repositories/metrics_repository.h"

void main_window_format_average(Metrics* metric, char* buffer, size_t size)
{
    time_t now = time(NULL);
    time_t cutoff = now - (7 * SECONDS_PER_DAY);

    uint32_t count = 0;
    uint32_t sum = 0;

    Registration* registrations = registrations_get_all();
    uint32_t total = registrations_count();
    for(uint32_t i = 0; i < total; i++)
    {
        Registration* reg = &registrations[i];
        if(reg->metrics_id == metric->id && reg->time_stamp >= cutoff)
        {
            sum += reg->value;
            count++;
        }
    }

    if(count == 0)
    {
        snprintf(buffer, size, "No data");
    } else if(metric->type == MetricsType_BOOL)
    {
        uint32_t percent = (sum * 100) / count;
        snprintf(buffer, size, "%d%% yes (%d)", (int)percent, (int)count);
    } else
    {
        uint32_t tenths = (sum * 10) / count;
        snprintf(buffer, size, "%d.%d avg (%d)", (int)(tenths / 10), (int)(tenths % 10), (int)count);
    }
}

void main_window_format_value(Metrics* metric, uint8_t value, char* buffer, size_t size)
{
    if(metric->type == MetricsType_BOOL)
    {
        snprintf(buffer, size, "%s", value ? "Yes" : "No");
    } else if(metric->type == MetricsType_THREE_OPTION)
    {
        const char* text = metrics_get_option_text(metric, value);
        if(text[0] != '\0')
        {
            snprintf(buffer, size, "%s", text);
        } else
        {
            snprintf(buffer, size, "%d", value + 1);
        }
    } else
    {
        snprintf(buffer, size, "%d", value);
    }
}
