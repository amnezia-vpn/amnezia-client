plugins {
    id(libs.plugins.android.library.get().pluginId)
    id("property-delegate")
}

java {
    toolchain.languageVersion.set(JavaLanguageVersion.of(17))
}

val qtAndroidDir: String by gradleProperties

android {
    namespace = "org.qtproject.qt.android.binding"
    buildFeatures {
        // QtActivityBase relies on string resources from the Qt android binding
        // (e.g. fatal_error_msg). Keep android resources enabled explicitly even
        // if library defaults disable them globally.
        androidResources = true
    }

    sourceSets {
        getByName("main") {
            java.setSrcDirs(listOf("$qtAndroidDir/src"))
            res.setSrcDirs(listOf("$qtAndroidDir/res"))
        }
    }
}

dependencies {
    api(fileTree(mapOf("dir" to "../libs", "include" to listOf("*.jar"))))
}
