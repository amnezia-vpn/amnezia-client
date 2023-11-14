import com.android.build.api.dsl.SettingsExtension

pluginManagement {
    repositories {
        google()
        mavenCentral()
        gradlePluginPortal()
        // for jsocks todo: remove after finish refactoring
        maven("https://jitpack.io")
    }

    includeBuild("./gradle/plugins")
}

@Suppress("UnstableApiUsage")
dependencyResolutionManagement {
    repositoriesMode.set(RepositoriesMode.FAIL_ON_PROJECT_REPOS)
    repositories {
        google()
        mavenCentral()
        // for jsocks todo: remove after finish refactoring
        maven("https://jitpack.io")
    }
}

includeBuild("./gradle/plugins")

plugins {
    id("com.android.settings") version "8.1.3"
    id("settings-property-delegate")
}

rootProject.name = "AmneziaVPN"
rootProject.buildFileName = "build.gradle.kts"

include(":shadowsocks")

// get values from gradle or local properties
val androidBuildToolsVersion: String by gradleProperties
val androidCompileSdkVersion: String by gradleProperties
val androidNdkVersion: String by gradleProperties
val qtMinSdkVersion: String by gradleProperties

// set default values for all modules
configure<SettingsExtension> {
    buildToolsVersion = androidBuildToolsVersion
    compileSdk = androidCompileSdkVersion.substringAfter('-').toInt()
    minSdk = qtMinSdkVersion.toInt()
    ndkVersion = androidNdkVersion
}

// stop Gradle running by androiddeployqt
gradle.taskGraph.whenReady {
    if (providers.environmentVariable("ANDROIDDEPLOYQT_RUN").isPresent
        && !providers.systemProperty("explicitRun").isPresent) {
        allTasks.forEach { it.enabled = false }
    }
}
