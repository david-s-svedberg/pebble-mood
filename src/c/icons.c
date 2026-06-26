#include "icons.h"

static GBitmap *check_icon;
static GBitmap *check_icon_black;
static GBitmap *check_icon_white;
static GBitmap *config_icon;
static GBitmap *alarm_icon;
static GBitmap *no_alarm_icon;
static GBitmap *alarm_icon_trans;
static GBitmap *no_alarm_icon_trans;
static GBitmap *edit_icon;
static GBitmap *up_icon;
static GBitmap *down_icon;
static GBitmap *play_icon;
static GBitmap *snooze_icon;
static GBitmap *silence_icon;
static GBitmap *pill_icon;
static GBitmap *mood_icon;
static GBitmap *exercise_icon;
static GBitmap *cross_icon;
static GBitmap *face_sad_icon;
static GBitmap *face_neutral_icon;
static GBitmap *face_happy_icon;
static GBitmap *level_low_icon;
static GBitmap *level_mid_icon;
static GBitmap *level_high_icon;
static GBitmap *main_sun_icon;
static GBitmap *main_moon_icon;
static GBitmap *main_droplet_icon;
static GBitmap *main_heart_icon;
static GBitmap *main_bolt_icon;
static GBitmap *main_coffee_icon;
static GBitmap *main_glass_icon;
static GBitmap *main_thermo_icon;
static GBitmap *main_phone_icon;
static GBitmap *main_cloud_icon;
static GBitmap *main_dumbbell_icon;
static GBitmap *main_bubble_icon;
static GBitmap *main_checkbox_icon;
static GBitmap *main_apple_icon;
static GBitmap *main_target_icon;
static GBitmap *main_pulse_icon;

static GBitmap* get_icon(uint32_t id, GBitmap** icon)
{
    if(*icon == NULL)
    {
        *icon = gbitmap_create_with_resource(id);
    }

    return *icon;
}

static void destroy_icon(GBitmap** icon)
{
    if(*icon != NULL)
    {
        gbitmap_destroy(*icon);
        *icon = NULL;
    }
}

GBitmap* get_check_icon()
{
    return get_icon(RESOURCE_ID_CHECK_ICON, &check_icon);
}

GBitmap* get_check_icon_black()
{
    return get_icon(RESOURCE_ID_CHECK_ICON_BLACK, &check_icon_black);
}

GBitmap* get_check_icon_white()
{
    return get_icon(RESOURCE_ID_CHECK_ICON_WHITE, &check_icon_white);
}

GBitmap* get_config_icon()
{
    return get_icon(RESOURCE_ID_CONFIG_ICON, &config_icon);
}

GBitmap* get_alarm_icon()
{
    return get_icon(RESOURCE_ID_ALARM_ICON, &alarm_icon);
}

GBitmap* get_no_alarm_icon()
{
    return get_icon(RESOURCE_ID_NO_ALARM_ICON, &no_alarm_icon);
}

GBitmap* get_alarm_icon_trans()
{
    return get_icon(RESOURCE_ID_ALARM_ICON_TRANS, &alarm_icon_trans);
}

GBitmap* get_no_alarm_icon_trans()
{
    return get_icon(RESOURCE_ID_NO_ALARM_ICON_TRANS, &no_alarm_icon_trans);
}

GBitmap* get_edit_icon()
{
    return get_icon(RESOURCE_ID_EDIT_ICON, &edit_icon);
}

GBitmap* get_up_icon()
{
    return get_icon(RESOURCE_ID_UP_ICON, &up_icon);
}

GBitmap* get_down_icon()
{
    return get_icon(RESOURCE_ID_DOWN_ICON, &down_icon);
}

GBitmap* get_play_icon()
{
    return get_icon(RESOURCE_ID_PLAY_ICON, &play_icon);
}

GBitmap* get_snooze_icon()
{
    return get_icon(RESOURCE_ID_SNOOZE_ICON, &snooze_icon);
}

GBitmap* get_silence_icon()
{
    return get_icon(RESOURCE_ID_SILENCE_ICON, &silence_icon);
}

GBitmap* get_pill_icon()
{
    return get_icon(RESOURCE_ID_PILL_ICON, &pill_icon);
}

GBitmap* get_mood_icon()
{
    return get_icon(RESOURCE_ID_MOOD_50_ICON, &mood_icon);
}

GBitmap* get_exercise_icon()
{
    return get_icon(RESOURCE_ID_EXERCISE_50_ICON, &exercise_icon);
}

GBitmap* get_cross_icon()
{
    return get_icon(RESOURCE_ID_CROSS_ICON, &cross_icon);
}

GBitmap* get_icon_by_choice(uint8_t choice)
{
    switch(choice)
    {
        case IconChoice_CHECK:    return get_check_icon();
        case IconChoice_CROSS:    return get_cross_icon();
        case IconChoice_UP:       return get_up_icon();
        case IconChoice_DOWN:     return get_down_icon();
        case IconChoice_MOOD:     return get_mood_icon();
        case IconChoice_EXERCISE: return get_exercise_icon();
        case IconChoice_PILL:     return get_pill_icon();
        case IconChoice_FACE_SAD:     return get_icon(RESOURCE_ID_FACE_SAD_ICON, &face_sad_icon);
        case IconChoice_FACE_NEUTRAL: return get_icon(RESOURCE_ID_FACE_NEUTRAL_ICON, &face_neutral_icon);
        case IconChoice_FACE_HAPPY:   return get_icon(RESOURCE_ID_FACE_HAPPY_ICON, &face_happy_icon);
        case IconChoice_LEVEL_LOW:    return get_icon(RESOURCE_ID_LEVEL_LOW_ICON, &level_low_icon);
        case IconChoice_LEVEL_MID:    return get_icon(RESOURCE_ID_LEVEL_MID_ICON, &level_mid_icon);
        case IconChoice_LEVEL_HIGH:   return get_icon(RESOURCE_ID_LEVEL_HIGH_ICON, &level_high_icon);
        case IconChoice_SUN:      return get_icon(RESOURCE_ID_MAIN_SUN_ICON, &main_sun_icon);
        case IconChoice_MOON:     return get_icon(RESOURCE_ID_MAIN_MOON_ICON, &main_moon_icon);
        case IconChoice_DROPLET:  return get_icon(RESOURCE_ID_MAIN_DROPLET_ICON, &main_droplet_icon);
        case IconChoice_HEART:    return get_icon(RESOURCE_ID_MAIN_HEART_ICON, &main_heart_icon);
        case IconChoice_BOLT:     return get_icon(RESOURCE_ID_MAIN_BOLT_ICON, &main_bolt_icon);
        case IconChoice_COFFEE:   return get_icon(RESOURCE_ID_MAIN_COFFEE_ICON, &main_coffee_icon);
        case IconChoice_GLASS:    return get_icon(RESOURCE_ID_MAIN_GLASS_ICON, &main_glass_icon);
        case IconChoice_THERMO:   return get_icon(RESOURCE_ID_MAIN_THERMO_ICON, &main_thermo_icon);
        case IconChoice_PHONE:    return get_icon(RESOURCE_ID_MAIN_PHONE_ICON, &main_phone_icon);
        case IconChoice_CLOUD:    return get_icon(RESOURCE_ID_MAIN_CLOUD_ICON, &main_cloud_icon);
        case IconChoice_DUMBBELL: return get_icon(RESOURCE_ID_MAIN_DUMBBELL_ICON, &main_dumbbell_icon);
        case IconChoice_BUBBLE:   return get_icon(RESOURCE_ID_MAIN_BUBBLE_ICON, &main_bubble_icon);
        case IconChoice_CHECKBOX: return get_icon(RESOURCE_ID_MAIN_CHECKBOX_ICON, &main_checkbox_icon);
        case IconChoice_APPLE:    return get_icon(RESOURCE_ID_MAIN_APPLE_ICON, &main_apple_icon);
        case IconChoice_TARGET:   return get_icon(RESOURCE_ID_MAIN_TARGET_ICON, &main_target_icon);
        case IconChoice_PULSE:    return get_icon(RESOURCE_ID_MAIN_PULSE_ICON, &main_pulse_icon);
        default:                  return NULL;
    }
}

void destroy_all_icons()
{
    destroy_icon(&check_icon);
    destroy_icon(&check_icon_black);
    destroy_icon(&check_icon_white);
    destroy_icon(&config_icon);
    destroy_icon(&alarm_icon);
    destroy_icon(&no_alarm_icon);
    destroy_icon(&alarm_icon_trans);
    destroy_icon(&no_alarm_icon_trans);
    destroy_icon(&edit_icon);
    destroy_icon(&up_icon);
    destroy_icon(&down_icon);
    destroy_icon(&play_icon);
    destroy_icon(&snooze_icon);
    destroy_icon(&silence_icon);
    destroy_icon(&pill_icon);
    destroy_icon(&mood_icon);
    destroy_icon(&exercise_icon);
    destroy_icon(&cross_icon);
    destroy_icon(&face_sad_icon);
    destroy_icon(&face_neutral_icon);
    destroy_icon(&face_happy_icon);
    destroy_icon(&level_low_icon);
    destroy_icon(&level_mid_icon);
    destroy_icon(&level_high_icon);
    destroy_icon(&main_sun_icon);
    destroy_icon(&main_moon_icon);
    destroy_icon(&main_droplet_icon);
    destroy_icon(&main_heart_icon);
    destroy_icon(&main_bolt_icon);
    destroy_icon(&main_coffee_icon);
    destroy_icon(&main_glass_icon);
    destroy_icon(&main_thermo_icon);
    destroy_icon(&main_phone_icon);
    destroy_icon(&main_cloud_icon);
    destroy_icon(&main_dumbbell_icon);
    destroy_icon(&main_bubble_icon);
    destroy_icon(&main_checkbox_icon);
    destroy_icon(&main_apple_icon);
    destroy_icon(&main_target_icon);
    destroy_icon(&main_pulse_icon);
}
