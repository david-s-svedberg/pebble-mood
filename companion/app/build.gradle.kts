plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
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
}

dependencies {
    // Spike keeps dependencies at zero beyond the platform: the import listener
    // is a hand-rolled ServerSocket HTTP server and the UI a plain TextView.
    // Room/Compose/Vico arrive with the real ingest + UI steps (see
    // ../design/companion_app_plan.md).
    implementation("androidx.core:core-ktx:1.13.1")
    implementation("androidx.appcompat:appcompat:1.7.0")
}
