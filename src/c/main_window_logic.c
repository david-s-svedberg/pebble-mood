#include "main_window_logic.h"

#include "repositories/metrics_repository.h"

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
