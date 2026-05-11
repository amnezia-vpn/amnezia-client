plugins {
    id(libs.plugins.android.library.get().pluginId)
    id(libs.plugins.kotlin.android.get().pluginId)
}

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "org.amnezia.vpn.protocol.masterdnsvpn"
}

dependencies {
    compileOnly(project(":utils"))
    compileOnly(project(":protocolApi"))
    // tun2socks reused from libxray.aar — same binary, no second copy
    // bundled. The Kotlin Protocol class calls LibXray.startTun2Socks()
    // pointing at the SOCKS5 the native engine exposes.
    implementation(project(":xray:libXray"))
    implementation(libs.kotlinx.coroutines)
}
