import java.io.File
import java.io.FileInputStream
import java.io.InputStreamReader
import java.util.Properties
import kotlin.properties.ReadOnlyProperty
import kotlin.reflect.KProperty
import org.gradle.api.Plugin
import org.gradle.api.Project
import org.gradle.api.initialization.Settings
import org.gradle.api.provider.ProviderFactory

private fun localProperties(rootDir: File) = Properties().apply {
    val localProperties = File(rootDir, "local.properties")
    if (localProperties.isFile) {
        InputStreamReader(FileInputStream(localProperties), Charsets.UTF_8).use {
            load(it)
        }
    }
}

private class PropertyDelegate(
    rootDir: File,
    private val providers: ProviderFactory,
    private val localProperties: Properties = localProperties(rootDir)
) : ReadOnlyProperty<Any?, String> {
    override fun getValue(thisRef: Any?, property: KProperty<*>): String =
        providers.gradleProperty(property.name).orNull ?: localProperties.getProperty(property.name).orEmpty()
}

private lateinit var settingsPropertyDelegate: ReadOnlyProperty<Any?, String>
private lateinit var projectPropertyDelegate: ReadOnlyProperty<Any?, String>

class SettingsPropertyDelegate : Plugin<Settings> {
    override fun apply(settings: Settings) {
        settingsPropertyDelegate = PropertyDelegate(settings.rootDir, settings.providers)
    }
}

class ProjectPropertyDelegate : Plugin<Project> {
    override fun apply(project: Project) {
        projectPropertyDelegate = PropertyDelegate(project.rootDir, project.providers)
    }
}

val Settings.gradleProperties: ReadOnlyProperty<Any?, String>
    get() = settingsPropertyDelegate

val Project.gradleProperties: ReadOnlyProperty<Any?, String>
    get() = projectPropertyDelegate
