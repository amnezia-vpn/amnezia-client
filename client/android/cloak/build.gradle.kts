plugins {
    id(libs.plugins.android.library.get().pluginId)
    id(libs.plugins.kotlin.android.get().pluginId)
}

kotlin {
    jvmToolchain(17)
}

android {
    namespace = "org.amnezia.vpn.protocol.cloak"
}

dependencies {
    compileOnly(project(":utils"))
    compileOnly(project(":protocolApi"))
    implementation(project(":openvpn"))
}
