plugins {
    id(libs.plugins.android.library.get().pluginId)
    id(libs.plugins.kotlin.android.get().pluginId)
}

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "com.fblink.vpn.util"

    buildFeatures {
        // add BuildConfig class
        buildConfig = true
    }
}

dependencies {
    implementation(libs.androidx.core)
    implementation(libs.kotlinx.coroutines)
    implementation(libs.androidx.security.crypto)
}
