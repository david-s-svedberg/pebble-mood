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

// Per-IconChoice bitmaps, cached lazily. Each choice has a black variant (for a
// light background) and a white variant (for a dark/highlighted background); a
// choice that only ships one color points both resource ids at the same asset.
static GBitmap *choice_black[IconChoice_COUNT];
static GBitmap *choice_white[IconChoice_COUNT];

static const uint32_t choice_black_res[IconChoice_COUNT] = {
    [IconChoice_CHECK]        = RESOURCE_ID_CHECK_ICON,
    [IconChoice_CROSS]        = RESOURCE_ID_CROSS_ICON,
    [IconChoice_UP]           = RESOURCE_ID_UP_ICON,
    [IconChoice_DOWN]         = RESOURCE_ID_DOWN_ICON,
    [IconChoice_MOOD]         = RESOURCE_ID_MOOD_48_ICON,
    [IconChoice_EXERCISE]     = RESOURCE_ID_EXERCISE_48_ICON,
    [IconChoice_PILL]         = RESOURCE_ID_PILL_ICON,
    [IconChoice_FACE_SAD]     = RESOURCE_ID_FACE_SAD_ICON,
    [IconChoice_FACE_NEUTRAL] = RESOURCE_ID_FACE_NEUTRAL_ICON,
    [IconChoice_FACE_HAPPY]   = RESOURCE_ID_FACE_HAPPY_ICON,
    [IconChoice_LEVEL_LOW]    = RESOURCE_ID_LEVEL_LOW_ICON,
    [IconChoice_LEVEL_MID]    = RESOURCE_ID_LEVEL_MID_ICON,
    [IconChoice_LEVEL_HIGH]   = RESOURCE_ID_LEVEL_HIGH_ICON,
    [IconChoice_SUN]          = RESOURCE_ID_MAIN_SUN_BLACK_ICON,
    [IconChoice_MOON]         = RESOURCE_ID_MAIN_MOON_BLACK_ICON,
    [IconChoice_DROPLET]      = RESOURCE_ID_MAIN_DROPLET_BLACK_ICON,
    [IconChoice_HEART]        = RESOURCE_ID_MAIN_HEART_BLACK_ICON,
    [IconChoice_BOLT]         = RESOURCE_ID_MAIN_BOLT_BLACK_ICON,
    [IconChoice_COFFEE]       = RESOURCE_ID_MAIN_COFFEE_BLACK_ICON,
    [IconChoice_GLASS]        = RESOURCE_ID_MAIN_GLASS_BLACK_ICON,
    [IconChoice_THERMO]       = RESOURCE_ID_MAIN_THERMO_BLACK_ICON,
    [IconChoice_PHONE]        = RESOURCE_ID_MAIN_PHONE_BLACK_ICON,
    [IconChoice_CLOUD]        = RESOURCE_ID_MAIN_CLOUD_BLACK_ICON,
    [IconChoice_DUMBBELL]     = RESOURCE_ID_MAIN_DUMBBELL_BLACK_ICON,
    [IconChoice_BUBBLE]       = RESOURCE_ID_MAIN_BUBBLE_BLACK_ICON,
    [IconChoice_CHECKBOX]     = RESOURCE_ID_MAIN_CHECKBOX_BLACK_ICON,
    [IconChoice_APPLE]        = RESOURCE_ID_MAIN_APPLE_BLACK_ICON,
    [IconChoice_TARGET]       = RESOURCE_ID_MAIN_TARGET_BLACK_ICON,
    [IconChoice_PULSE]        = RESOURCE_ID_MAIN_PULSE_BLACK_ICON,
};

static const uint32_t choice_white_res[IconChoice_COUNT] = {
    [IconChoice_CHECK]        = RESOURCE_ID_CHECK_W20_ICON,
    [IconChoice_CROSS]        = RESOURCE_ID_CROSS_W20_ICON,
    [IconChoice_UP]           = RESOURCE_ID_UP_WHITE_ICON,
    [IconChoice_DOWN]         = RESOURCE_ID_DOWN_WHITE_ICON,
    [IconChoice_MOOD]         = RESOURCE_ID_MOOD_48_WHITE_ICON,
    [IconChoice_EXERCISE]     = RESOURCE_ID_EXERCISE_48_WHITE_ICON,
    [IconChoice_PILL]         = RESOURCE_ID_PILL_WHITE_ICON,
    [IconChoice_FACE_SAD]     = RESOURCE_ID_FACE_SAD_WHITE_ICON,
    [IconChoice_FACE_NEUTRAL] = RESOURCE_ID_FACE_NEUTRAL_WHITE_ICON,
    [IconChoice_FACE_HAPPY]   = RESOURCE_ID_FACE_HAPPY_WHITE_ICON,
    [IconChoice_LEVEL_LOW]    = RESOURCE_ID_LEVEL_LOW_WHITE_ICON,
    [IconChoice_LEVEL_MID]    = RESOURCE_ID_LEVEL_MID_WHITE_ICON,
    [IconChoice_LEVEL_HIGH]   = RESOURCE_ID_LEVEL_HIGH_WHITE_ICON,
    [IconChoice_SUN]          = RESOURCE_ID_MAIN_SUN_ICON,
    [IconChoice_MOON]         = RESOURCE_ID_MAIN_MOON_ICON,
    [IconChoice_DROPLET]      = RESOURCE_ID_MAIN_DROPLET_ICON,
    [IconChoice_HEART]        = RESOURCE_ID_MAIN_HEART_ICON,
    [IconChoice_BOLT]         = RESOURCE_ID_MAIN_BOLT_ICON,
    [IconChoice_COFFEE]       = RESOURCE_ID_MAIN_COFFEE_ICON,
    [IconChoice_GLASS]        = RESOURCE_ID_MAIN_GLASS_ICON,
    [IconChoice_THERMO]       = RESOURCE_ID_MAIN_THERMO_ICON,
    [IconChoice_PHONE]        = RESOURCE_ID_MAIN_PHONE_ICON,
    [IconChoice_CLOUD]        = RESOURCE_ID_MAIN_CLOUD_ICON,
    [IconChoice_DUMBBELL]     = RESOURCE_ID_MAIN_DUMBBELL_ICON,
    [IconChoice_BUBBLE]       = RESOURCE_ID_MAIN_BUBBLE_ICON,
    [IconChoice_CHECKBOX]     = RESOURCE_ID_MAIN_CHECKBOX_ICON,
    [IconChoice_APPLE]        = RESOURCE_ID_MAIN_APPLE_ICON,
    [IconChoice_TARGET]       = RESOURCE_ID_MAIN_TARGET_ICON,
    [IconChoice_PULSE]        = RESOURCE_ID_MAIN_PULSE_ICON,
};

// 20px row-sized variants of the (otherwise 48px) main icons, so they can show a
// preview in a menu row too. Only the main-icon range is populated; smaller
// choices already fit a row and fall back to their normal bitmap.
static GBitmap *choice_row_black[IconChoice_COUNT];
static GBitmap *choice_row_white[IconChoice_COUNT];

static const uint32_t choice_row_black_res[IconChoice_COUNT] = {
    [IconChoice_MOOD]     = RESOURCE_ID_MOOD_SM_ICON,
    [IconChoice_EXERCISE] = RESOURCE_ID_EXERCISE_SM_ICON,
    [IconChoice_SUN]      = RESOURCE_ID_MAIN_SUN_SM_ICON,
    [IconChoice_MOON]     = RESOURCE_ID_MAIN_MOON_SM_ICON,
    [IconChoice_DROPLET]  = RESOURCE_ID_MAIN_DROPLET_SM_ICON,
    [IconChoice_HEART]    = RESOURCE_ID_MAIN_HEART_SM_ICON,
    [IconChoice_BOLT]     = RESOURCE_ID_MAIN_BOLT_SM_ICON,
    [IconChoice_COFFEE]   = RESOURCE_ID_MAIN_COFFEE_SM_ICON,
    [IconChoice_GLASS]    = RESOURCE_ID_MAIN_GLASS_SM_ICON,
    [IconChoice_THERMO]   = RESOURCE_ID_MAIN_THERMO_SM_ICON,
    [IconChoice_PHONE]    = RESOURCE_ID_MAIN_PHONE_SM_ICON,
    [IconChoice_CLOUD]    = RESOURCE_ID_MAIN_CLOUD_SM_ICON,
    [IconChoice_DUMBBELL] = RESOURCE_ID_MAIN_DUMBBELL_SM_ICON,
    [IconChoice_BUBBLE]   = RESOURCE_ID_MAIN_BUBBLE_SM_ICON,
    [IconChoice_CHECKBOX] = RESOURCE_ID_MAIN_CHECKBOX_SM_ICON,
    [IconChoice_APPLE]    = RESOURCE_ID_MAIN_APPLE_SM_ICON,
    [IconChoice_TARGET]   = RESOURCE_ID_MAIN_TARGET_SM_ICON,
    [IconChoice_PULSE]    = RESOURCE_ID_MAIN_PULSE_SM_ICON,
};

static const uint32_t choice_row_white_res[IconChoice_COUNT] = {
    [IconChoice_MOOD]     = RESOURCE_ID_MOOD_SM_WHITE_ICON,
    [IconChoice_EXERCISE] = RESOURCE_ID_EXERCISE_SM_WHITE_ICON,
    [IconChoice_SUN]      = RESOURCE_ID_MAIN_SUN_SM_WHITE_ICON,
    [IconChoice_MOON]     = RESOURCE_ID_MAIN_MOON_SM_WHITE_ICON,
    [IconChoice_DROPLET]  = RESOURCE_ID_MAIN_DROPLET_SM_WHITE_ICON,
    [IconChoice_HEART]    = RESOURCE_ID_MAIN_HEART_SM_WHITE_ICON,
    [IconChoice_BOLT]     = RESOURCE_ID_MAIN_BOLT_SM_WHITE_ICON,
    [IconChoice_COFFEE]   = RESOURCE_ID_MAIN_COFFEE_SM_WHITE_ICON,
    [IconChoice_GLASS]    = RESOURCE_ID_MAIN_GLASS_SM_WHITE_ICON,
    [IconChoice_THERMO]   = RESOURCE_ID_MAIN_THERMO_SM_WHITE_ICON,
    [IconChoice_PHONE]    = RESOURCE_ID_MAIN_PHONE_SM_WHITE_ICON,
    [IconChoice_CLOUD]    = RESOURCE_ID_MAIN_CLOUD_SM_WHITE_ICON,
    [IconChoice_DUMBBELL] = RESOURCE_ID_MAIN_DUMBBELL_SM_WHITE_ICON,
    [IconChoice_BUBBLE]   = RESOURCE_ID_MAIN_BUBBLE_SM_WHITE_ICON,
    [IconChoice_CHECKBOX] = RESOURCE_ID_MAIN_CHECKBOX_SM_WHITE_ICON,
    [IconChoice_APPLE]    = RESOURCE_ID_MAIN_APPLE_SM_WHITE_ICON,
    [IconChoice_TARGET]   = RESOURCE_ID_MAIN_TARGET_SM_WHITE_ICON,
    [IconChoice_PULSE]    = RESOURCE_ID_MAIN_PULSE_SM_WHITE_ICON,
};

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

GBitmap* get_icon_by_choice_ex(uint8_t choice, bool light)
{
    if(choice == IconChoice_NONE || choice >= IconChoice_COUNT)
    {
        return NULL;
    }

    uint32_t res = light ? choice_white_res[choice] : choice_black_res[choice];
    GBitmap** cache = light ? &choice_white[choice] : &choice_black[choice];
    return get_icon(res, cache);
}

GBitmap* get_icon_by_choice(uint8_t choice)
{
    return get_icon_by_choice_ex(choice, false);
}

GBitmap* get_icon_row_by_choice(uint8_t choice, bool light)
{
    if(choice == IconChoice_NONE || choice >= IconChoice_COUNT)
    {
        return NULL;
    }

    // Main icons have dedicated 20px row variants; everything else is already
    // small enough to draw in a row.
    uint32_t res = light ? choice_row_white_res[choice] : choice_row_black_res[choice];
    if(res == 0)
    {
        return get_icon_by_choice_ex(choice, light);
    }

    GBitmap** cache = light ? &choice_row_white[choice] : &choice_row_black[choice];
    return get_icon(res, cache);
}

const char* icon_choice_name(uint8_t choice)
{
    switch(choice)
    {
        case IconChoice_CHECK:    return "Check";
        case IconChoice_CROSS:    return "Cross";
        case IconChoice_UP:       return "Up";
        case IconChoice_DOWN:     return "Down";
        case IconChoice_MOOD:     return "Mood";
        case IconChoice_EXERCISE: return "Exercise";
        case IconChoice_PILL:     return "Pill";
        case IconChoice_FACE_SAD:     return "Sad face";
        case IconChoice_FACE_NEUTRAL: return "Neutral face";
        case IconChoice_FACE_HAPPY:   return "Happy face";
        case IconChoice_LEVEL_LOW:    return "Level low";
        case IconChoice_LEVEL_MID:    return "Level mid";
        case IconChoice_LEVEL_HIGH:   return "Level high";
        case IconChoice_SUN:      return "Sun";
        case IconChoice_MOON:     return "Moon";
        case IconChoice_DROPLET:  return "Droplet";
        case IconChoice_HEART:    return "Heart";
        case IconChoice_BOLT:     return "Bolt";
        case IconChoice_COFFEE:   return "Coffee";
        case IconChoice_GLASS:    return "Glass";
        case IconChoice_THERMO:   return "Thermometer";
        case IconChoice_PHONE:    return "Phone";
        case IconChoice_CLOUD:    return "Cloud";
        case IconChoice_DUMBBELL: return "Dumbbell";
        case IconChoice_BUBBLE:   return "Speech";
        case IconChoice_CHECKBOX: return "Checkbox";
        case IconChoice_APPLE:    return "Apple";
        case IconChoice_TARGET:   return "Target";
        case IconChoice_PULSE:    return "Pulse";
        default:                  return "None";
    }
}

bool icon_choice_is_small(uint8_t choice)
{
    if(choice == IconChoice_MOOD || choice == IconChoice_EXERCISE)
    {
        return false;
    }
    return choice < IconChoice_SUN;
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
    for(int i = 0; i < IconChoice_COUNT; i++)
    {
        destroy_icon(&choice_black[i]);
        destroy_icon(&choice_white[i]);
        destroy_icon(&choice_row_black[i]);
        destroy_icon(&choice_row_white[i]);
    }
}
