plugins {
    id(libs.plugins.android.library.get().pluginId)
    id(libs.plugins.kotlin.android.get().pluginId)
}

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "org.amnezia.vpn.billing"
}

dependencies {
    implementation(libs.androidx.core)
    implementation(libs.kotlinx.coroutines)
    implementation(libs.android.billing)
}
