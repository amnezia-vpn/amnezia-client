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
