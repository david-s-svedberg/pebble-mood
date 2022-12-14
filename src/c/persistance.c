// #include "persistance.h"

// #include <pebble.h>
// #include <stdbool.h>
// #include <gcolor_definitions.h>

// const uint8_t FOUR_MINUTES_IN_SEC = 240;
// const uint8_t ONE_MINUTES_IN_SEC = 60;


// static MetricsGroup* m_metric_groups;
// static Metric* m_metrics;
// static Data m_data;
// static bool m_data_loaded = false;

// static Alarm create_alarm(uint8_t index)
// {
//     Alarm alarm;

//     alarm.index = index;
//     alarm.time.hour = 0;
//     alarm.time.minute = 0;
//     alarm.active = false;

//     return alarm;
// }

// static void seed_version_1_data(Data* data)
// {
//     data->snooze_alarm = create_alarm(SNOOZED_ALARM_ID);

//     data->summer_time_alarm = create_alarm(SUMMER_TIME_ALARM_ID);
//     data->summer_time_alarm.time.hour = 3;
//     data->summer_time_alarm.time.minute = 31;

//     data->background_color = GColorBlack;
//     data->foreground_color = GColorWhite;
//     data->alarm_timeout_sec = ONE_MINUTES_IN_SEC;
//     data->post_versioning_indicator = POST_VERSIONING_KEY;
//     for(uint8_t i = 0; i <= MAX_MEASUREMENT_POINT_INDEX, i++)
//     {
//         data->last_score[i] = 0;
//     }
// }

// static void seed_data()
// {
//     APP_LOG(APP_LOG_LEVEL_DEBUG, "Seeding data");
//     seed_version_1_data(&m_data);

//     m_data.data_version = CURRENT_DATA_VERSION;
//     persist_write_data(MAIN_DATA_KEY, &m_data, sizeof(Data));
// }

// static bool data_version_is_current(Data* data)
// {
//     bool is_current = true;
//     if(data->data_version < CURRENT_DATA_VERSION)
//     {
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "The data version:%d is lower than the current version:%d", data->data_version, CURRENT_DATA_VERSION);
//         is_current = false;
//     }
//     return is_current;
// }

// static void migrate_data(Data* data)
// {
//     if(data->data_version < 1)
//     {
//         APP_LOG(APP_LOG_LEVEL_DEBUG, "Migrating to data version 1");

//         seed_version_1_data(data);
//         data->data_version = 1;
//     }
// }

// static Data* get_data()
// {
//     if(!has_any_data())
//     {
//         seed_data();
//     }
//     if(!m_data_loaded)
//     {
//         persist_read_data(MAIN_DATA_KEY, &m_data, sizeof(Data));

//         m_strings = (String*)malloc(sizeof(String) * m_data.number_of_strings);
//         m_metric_groups = (MetricsGroup*)malloc(sizeof(MetricsGroup) * m_data.number_of_groups);
//         m_metrics = (Metric*)malloc(sizeof(Metric) * m_data.number_of_metrics);

//         m_data_loaded = true;
//         if(!data_version_is_current(&m_data))
//         {
//             migrate_data(&m_data);
//             save_data();
//         }
//     }
//     return &m_data;
// }

// Alarm* get_daily_alarm()
// {
//     return get_data()->daily_alarm;
// }

// static uint32_t minutes_until(uint8_t currentHour, uint8_t currentMinutes, uint8_t targetHour, uint8_t targetMinute)
// {
//     uint32_t ret = 0;

//     if(targetHour > currentHour)
//     {
//         ret += ((targetHour - currentHour)*MINUTES_PER_HOUR);
//         if(targetMinute >= currentMinutes)
//         {
//             ret += targetMinute - currentMinutes;
//         } else
//         {
//             ret -= currentMinutes - targetMinute;
//         }
//     } else if(targetHour == currentHour)
//     {
//         if(targetMinute >= currentMinutes)
//         {
//             ret += targetMinute - currentMinutes;
//         } else
//         {
//             ret = MINUTES_PER_DAY - (currentMinutes - targetMinute);
//         }
//     } else
//     {
//         ret = (HOURS_PER_DAY - currentHour + targetHour) * MINUTES_PER_HOUR;
//         if(targetMinute >= currentMinutes)
//         {
//             ret += targetMinute - currentMinutes;
//         } else
//         {
//             ret -= currentMinutes - targetMinute;
//         }
//     }

//     return ret;
// }

// static Alarm* get_next_schedueled(Alarm** alarms, size_t number_of_alarms)
// {
//     time_t now = time(NULL);
//     Alarm* next = NULL;
//     uint32_t currentlyClosest = 0;
//     time_t wakup_time;
//     for(size_t i = 0; i < number_of_alarms; i++)
//     {
//         Alarm* current = *(alarms + i);
//         if(current != NULL)
//         {
//             if(current->active)
//             {
//                 bool scheduled = wakeup_query(current->wakeup_id, &wakup_time);
//                 if(scheduled)
//                 {
//                     uint32_t minutesUntilAlarm = (wakup_time - now)/SECONDS_PER_MINUTE;
//                     if(next == NULL || minutesUntilAlarm < currentlyClosest)
//                     {
//                         APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm:%d is currently closest with %d minutes until alarm", current->index, (int)minutesUntilAlarm);
//                         next = current;
//                         currentlyClosest = minutesUntilAlarm;
//                     } else {
//                         APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm:%d is not closer with %d minutes until alarm", current->index, (int)minutesUntilAlarm);
//                     }
//                 } else {
//                     APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm:%d not scheduled", current->index);
//                 }
//             } else {
//                 APP_LOG(APP_LOG_LEVEL_DEBUG, "Alarm:%d not active", current->index);
//             }

//         }
//     }
//     return next;
// }

// Alarm* get_next_alarm()
// {
//     Alarm* all_alarms[MAX_ALARMS + 1];

//     for(int i = 0; i < MAX_ALARMS ; i++)
//     {
//         all_alarms[i] = get_alarm(i);
//     }
//     all_alarms[MAX_ALARMS] = config_get_snooze_alarm();

//     return get_next_schedueled(all_alarms, sizeof(all_alarms)/sizeof(all_alarms[0]));
// }

// bool has_any_data()
// {
//     return persist_exists(DATA_KEY);
// }

// void save_data()
// {
//     persist_write_data(DATA_KEY, &m_data, sizeof(Data));
// }

// GColor8 get_background_color()
// {
//     return get_data()->background_color;
// }

// GColor8 get_foreground_color()
// {
//     return get_data()->foreground_color;
// }

// uint8_t config_get_alarm_timeout()
// {
//     return get_data()->alarm_timeout_sec;
// }

// bool is_dark_theme()
// {
//     uint8_t background = get_background_color().argb;
//     uint8_t black = GColorBlack.argb;
//     return (background == black);
// }

// void config_toggle_theme()
// {
//     Data* data = get_data();
//     GColor8 previous_background_color = data->background_color;
//     GColor8 previous_foreground_color = data->foreground_color;

//     data->background_color = previous_foreground_color;
//     data->foreground_color = previous_background_color;
//     save_data();
// }

// Alarm* config_get_snooze_alarm()
// {
//     return &get_data()->snooze_alarm;
// }

// void config_set_alarm_timeout(uint8_t sec)
// {
//     Data* data = get_data();
//     data->alarm_timeout_sec = sec;
//     save_data();
// }

// uint8_t get_last_score(uint8_t measurement_point_index)
// {
//     Data* data = get_data();
//     return data->last_score[measurement_point_index];
// }

// void set_last_score(uint8_t measurement_point_index, uint8_t score)
// {
//     Data* data = get_data();
//     data->last_score[measurement_point_index] = score;
//     save_data();
// }

// Metric* get_metric(uint8_t index)
// {
//     return m_metrics[index * sizeof(Metric)];
// }

// void tear_down_data()
// {
//     free(m_metric_groups);
//     free(m_strings);
//     free(m_metrics);
// }