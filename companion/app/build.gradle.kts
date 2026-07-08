plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
    id("org.jetbrains.kotlin.plugin.compose")
    id("com.google.devtools.ksp")
}

android {
    namespace = "se.svep.mood.companion"
    compileSdk = 35

    defaultConfig {
        applicationId = "se.svep.mood.companion"
        minSdk = 26
        targetSdk = 35
        versionCode = 1
        versionName = "0.1-spike"
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }
    kotlinOptions {
        jvmTarget = "17"
    }
    buildFeatures {
        compose = true
    }
}

dependencies {
    // Phase 2 (ingest): Room for storage, coroutines/lifecycle for the async
    // plumbing. The import listener stays a hand-rolled ServerSocket server.
    // Compose/Vico arrive with the graph phase (see ../design/companion_app_plan.md).
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
    implementation("androidx.lifecycle:lifecycle-runtime-ktx:2.8.4")
    implementation("org.jetbrains.kotlinx:kotlinx-coroutines-android:1.8.1")

    implementation("androidx.room:room-runtime:2.6.1")
    implementation("androidx.room:room-ktx:2.6.1")
    ksp("androidx.room:room-compiler:2.6.1")

    // Phase 6: read Pebble health data (steps/sleep/heart rate) that the Core
    // Devices app writes into Health Connect, aggregate it per day, and surface
    // it as auto-metrics in the graph + correlation engine.
    // Pinned to alpha10: the rc/stable line requires AGP 8.9 + compileSdk 36,
    // which would drag the whole toolchain forward. alpha10 has the same read/
    // aggregate API we use and builds fine against compileSdk 35.
    implementation("androidx.health.connect:connect-client:1.1.0-alpha10")

    // Graph phase: Compose UI. The chart itself is hand-drawn on Canvas — full
    // control over the bespoke marks (bool-as-event tracks, normalized lines)
    // without a chart-library dependency.
    val composeBom = platform("androidx.compose:compose-bom:2024.09.00")
    implementation(composeBom)
    implementation("androidx.activity:activity-compose:1.9.2")
    implementation("androidx.compose.material3:material3")
    implementation("androidx.compose.ui:ui")
    implementation("androidx.compose.ui:ui-graphics")
    implementation("androidx.compose.ui:ui-tooling-preview")
}
