plugins {
    `kotlin-dsl`
}

repositories {
    gradlePluginPortal()
}

kotlin {
    jvmToolchain(17)
}

gradlePlugin {
    plugins {
        register("settingsGradlePropertyDelegate") {
            id = "settings-property-delegate"
            implementationClass = "SettingsPropertyDelegate"
        }

        register("projectGradlePropertyDelegate") {
            id = "property-delegate"
            implementationClass = "ProjectPropertyDelegate"
        }
    }
}

// stop Gradle running by androiddeployqt
gradle.taskGraph.whenReady {
    if (providers.environmentVariable("ANDROIDDEPLOYQT_RUN").isPresent
        && !providers.systemProperty("explicitRun").isPresent) {
        tasks.forEach { it.enabled = false }
    }
}
