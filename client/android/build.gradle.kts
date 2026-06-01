import com.android.build.gradle.internal.api.BaseVariantOutputImpl

plugins {
    alias(libs.plugins.android.application)
    alias(libs.plugins.kotlin.android)
    alias(libs.plugins.kotlin.serialization)
    id("property-delegate")
}

kotlin {
    jvmToolchain(17)
}

// get values from gradle or local properties
val qtTargetSdkVersion: String by gradleProperties
val qtTargetAbiList: String by gradleProperties
val outputBaseName: String by gradleProperties

android {
    namespace = "org.amnezia.vpn"

    buildFeatures {
        buildConfig = true
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

    val abiList = qtTargetAbiList.split(",")

    defaultConfig {
        applicationId = "org.amnezia.vpn"
        targetSdk = qtTargetSdkVersion.toInt()

        // keeps language resources for only the locales specified below
        resourceConfigurations += listOf("en", "ru", "b+zh+Hans")
        // ndk.abiFilters is only used for single-ABI builds; multi-ABI uses splits below
        if (abiList.size == 1) {
            ndk.abiFilters += abiList
        }
    }

    signingConfigs {
        register("release") {
            storeFile = providers.environmentVariable("QT_ANDROID_KEYSTORE_PATH").orNull?.let { file(it) }
            storePassword = providers.environmentVariable("QT_ANDROID_KEYSTORE_STORE_PASS").orNull
            keyAlias = providers.environmentVariable("QT_ANDROID_KEYSTORE_ALIAS").orNull
            keyPassword = providers.environmentVariable("QT_ANDROID_KEYSTORE_STORE_PASS").orNull
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

        create("fdroid") {
            initWith(getByName("release"))
            signingConfig = null
            matchingFallbacks += "release"
        }
    }

    flavorDimensions += "billing"

    productFlavors {
        create("oss") {
            dimension = "billing"
        }
        create("play") {
            dimension = "billing"
        }
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

        getByName("oss") {
            java.setSrcDirs(listOf("oss"))
        }

        getByName("play") {
            java.setSrcDirs(listOf("play"))
        }
    }

    splits {
        abi {
            // splits only make sense for multi-ABI builds; single-ABI uses ndk.abiFilters
            isEnable = abiList.size > 1
            reset()
            include(*abiList.toTypedArray())
            isUniversalApk = false
        }
    }

    // fix for Qt Creator to allow deploying the application to a device
    // to enable this fix, add the line outputBaseName=android-build to local.properties
    if (outputBaseName.isNotEmpty()) {
        applicationVariants.all {
            outputs.map { it as BaseVariantOutputImpl }
                .forEach { output ->
                    if (output.outputFileName.endsWith(".apk")) {
                        output.outputFileName = "$outputBaseName-${buildType.name}.apk"
                    }
                }
        }
    }

    lint {
        disable += "InvalidFragmentVersionForActivityResult"
    }
}

dependencies {
    implementation(project(":qt"))
    implementation(project(":utils"))
    implementation(project(":protocolApi"))
    implementation(project(":wireguard"))
    implementation(project(":awg"))
    implementation(project(":openvpn"))
    implementation(project(":xray"))
    implementation(libs.androidx.core)
    implementation(libs.androidx.activity)
    implementation(libs.androidx.fragment)
    implementation(libs.kotlinx.coroutines)
    implementation(libs.kotlinx.serialization.protobuf)
    implementation(libs.bundles.androidx.camera)
    implementation(libs.google.mlkit)
    implementation(libs.androidx.datastore)
    implementation(libs.androidx.biometric)

    playImplementation(project(":billing"))
}

fun DependencyHandler.playImplementation(dependency: Any): Dependency? =
    add("playImplementation", dependency)
