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
