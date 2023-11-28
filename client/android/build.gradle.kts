plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    id("property-delegate")
}

kotlin {
    jvmToolchain(17)
}

// get values from gradle or local properties
val qtTargetSdkVersion: String by gradleProperties
val qtTargetAbiList: String by gradleProperties

android {
    namespace = "org.amnezia.vpn"

    buildFeatures {
        viewBinding = true
    }

    androidResources {
        // don't compress Qt binary resources file
        noCompress += "rcc"
    }

    packaging {
        // compress .so binary libraries
        jniLibs.useLegacyPackaging = true
    }

    defaultConfig {
        applicationId = "org.amnezia.vpn"
        targetSdk = qtTargetSdkVersion.toInt()

        // keeps language resources for only the locales specified below
        resourceConfigurations += listOf("en", "ru", "b+zh+Hans")
    }

    sourceSets {
        getByName("main") {
            manifest.srcFile("AndroidManifest.xml")
            java.setSrcDirs(listOf("src"))
            res.setSrcDirs(listOf("res"))
            // androyddeployqt creates the folders below
            assets.setSrcDirs(listOf("assets"))
            jniLibs.setSrcDirs(listOf("libs"))
        }
    }

    signingConfigs {
        register("release") {
            storeFile = providers.environmentVariable("ANDROID_KEYSTORE_PATH").orNull?.let { file(it) }
            storePassword = providers.environmentVariable("ANDROID_KEYSTORE_KEY_PASS").orNull
            keyAlias = providers.environmentVariable("ANDROID_KEYSTORE_KEY_ALIAS").orNull
            keyPassword = providers.environmentVariable("ANDROID_KEYSTORE_KEY_PASS").orNull
        }
    }

    buildTypes {
        release {
            // exclude coroutine debug resource from release build
            packaging {
                resources.excludes += "DebugProbesKt.bin"
            }
            signingConfig = signingConfigs["release"]
        }
    }

    splits {
        abi {
            isEnable = true
            reset()
            include(*qtTargetAbiList.split(',').toTypedArray())
            isUniversalApk = false
        }
    }

    lint {
        disable += "InvalidFragmentVersionForActivityResult"
    }
}

dependencies {
    implementation(fileTree(mapOf("dir" to "libs", "include" to listOf("*.jar", "*.aar"))))
    implementation(project(":qt"))
    implementation(project(":utils"))
    implementation(project(":protocolApi"))
    implementation(project(":wireguard"))
    implementation(project(":awg"))
    implementation(project(":openvpn"))
    implementation(project(":cloak"))
    implementation(libs.androidx.core)
    implementation(libs.androidx.activity)
    implementation(libs.androidx.security.crypto)
    implementation(libs.kotlinx.coroutines)
    implementation(libs.bundles.androidx.camera)
    implementation(libs.google.mlkit)
}
