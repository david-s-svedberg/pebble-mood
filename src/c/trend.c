#include "trend.h"

#include "repositories/metrics_repository.h"

// Effective value range per metric type (mirrors data_export.c): BOOL 0..1,
// THREE_OPTION 0..2, INTERVAL uses the stored min/max. Used to normalize the
// sparkline height.
static void effective_range(Metrics* metric, uint8_t* min_v, uint8_t* max_v)
{
    switch(metric->type)
    {
        case MetricsType_BOOL:
            *min_v = 0; *max_v = 1; break;
        case MetricsType_THREE_OPTION:
            *min_v = 0; *max_v = 2; break;
        case MetricsType_INTERVAL:
        default:
            *min_v = metric->min_value; *max_v = metric->max_value; break;
    }
    if(*max_v <= *min_v) *max_v = *min_v + 1;   // guard against a zero span
}

static time_t start_of_day(time_t t)
{
    struct tm* lt = localtime(&t);
    lt->tm_hour = 0;
    lt->tm_min = 0;
    lt->tm_sec = 0;
    return mktime(lt);
}

bool trend_build(uint16_t metric_id, TrendSeries* out)
{
    Metrics* metric = metrics_get(metric_id);
    if(metric == NULL) return false;

    out->metric_id = metric_id;
    out->points = 0;
    effective_range(metric, &out->min_v, &out->max_v);

    uint32_t sums[TREND_DAYS] = {0};
    uint16_t counts[TREND_DAYS] = {0};

    time_t today_start = start_of_day(time(NULL));

    Registration* regs = registrations_get_all();
    uint32_t n = registrations_count();
    for(uint32_t i = 0; i < n; i++)
    {
        if(regs[i].metrics_id != metric_id) continue;
        time_t reg_day = start_of_day(regs[i].time_stamp);
        int32_t days_ago = (int32_t)((today_start - reg_day) / SECONDS_PER_DAY);
        if(days_ago < 0 || days_ago >= TREND_DAYS) continue;
        int idx = TREND_DAYS - 1 - days_ago;
        sums[idx] += regs[i].value;
        counts[idx]++;
    }

    for(int i = 0; i < TREND_DAYS; i++)
    {
        out->has[i] = counts[i] > 0;
        if(out->has[i])
        {
            // Rounded mean.
            out->value[i] = (uint8_t)((sums[i] + counts[i] / 2) / counts[i]);
            out->points++;
        } else
        {
            out->value[i] = 0;
        }
    }
    return true;
}
