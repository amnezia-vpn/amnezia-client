import com.android.build.api.dsl.SettingsExtension

pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
    }

    includeBuild("./gradle/plugins")
}

@Suppress("UnstableApiUsage")
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
    }
}

includeBuild("./gradle/plugins")

plugins {
    id("com.android.settings") version "8.5.2"
    id("settings-property-delegate")
}

rootProject.buildFileName = "build.gradle.kts"

include(":qt")
include(":utils")
include(":protocolApi")
include(":wireguard")
include(":awg")
include(":openvpn")
include(":xray")
include(":xray:libXray")

// get values from gradle or local properties
val androidBuildToolsVersion: String by gradleProperties
val androidCompileSdkVersion: String by gradleProperties
val androidNdkVersion: String by gradleProperties
val qtMinSdkVersion: String by gradleProperties

// set default values for all modules
configure<SettingsExtension> {
    buildToolsVersion = androidBuildToolsVersion
    compileSdk = androidCompileSdkVersion.split('-')[1].toInt()
    minSdk = qtMinSdkVersion.toInt()
    ndkVersion = androidNdkVersion
}
