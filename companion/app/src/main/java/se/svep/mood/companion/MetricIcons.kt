package se.svep.mood.companion

import androidx.annotation.DrawableRes

/**
 * Maps a metric's mainIcon (the watch IconChoice ordinal, exported as-is) to the
 * ported watch icon drawable. The PNGs in res/drawable-nodpi/ic_metric_N.png are
 * the watch's black glyphs — tint them to the theme colour where they're drawn.
 * 0 (IconChoice_NONE) and the companion-only health metrics have no icon.
 */
object MetricIcons {

    @DrawableRes
    fun drawableFor(mainIcon: Int): Int? = when (mainIcon) {
        1 -> R.drawable.ic_metric_1
        2 -> R.drawable.ic_metric_2
        3 -> R.drawable.ic_metric_3
        4 -> R.drawable.ic_metric_4
        5 -> R.drawable.ic_metric_5
        6 -> R.drawable.ic_metric_6
        7 -> R.drawable.ic_metric_7
        8 -> R.drawable.ic_metric_8
        9 -> R.drawable.ic_metric_9
        10 -> R.drawable.ic_metric_10
        11 -> R.drawable.ic_metric_11
        12 -> R.drawable.ic_metric_12
        13 -> R.drawable.ic_metric_13
        14 -> R.drawable.ic_metric_14
        15 -> R.drawable.ic_metric_15
        16 -> R.drawable.ic_metric_16
        17 -> R.drawable.ic_metric_17
        18 -> R.drawable.ic_metric_18
        19 -> R.drawable.ic_metric_19
        20 -> R.drawable.ic_metric_20
        21 -> R.drawable.ic_metric_21
        22 -> R.drawable.ic_metric_22
        23 -> R.drawable.ic_metric_23
        24 -> R.drawable.ic_metric_24
        25 -> R.drawable.ic_metric_25
        26 -> R.drawable.ic_metric_26
        27 -> R.drawable.ic_metric_27
        28 -> R.drawable.ic_metric_28
        29 -> R.drawable.ic_metric_29
        else -> null
    }
}
