import android.Manifest.permission.INTERNET
import android.content.pm.ApplicationInfo
import android.content.pm.PackageInfo
import android.content.pm.PackageManager
import org.json.JSONArray
import org.json.JSONObject

object AppListProvider {
    fun getAppList(pm: PackageManager, selfPackageName: String): String {
        val jsonArray = JSONArray()
        pm.getPackagesHoldingPermissions(arrayOf(INTERNET), 0)
            .filter { it.packageName != selfPackageName }
            .map { App(it, pm) }
            .sortedWith(App::compareTo)
            .map(App::toJson)
            .forEach(jsonArray::put)
        return jsonArray.toString()
    }
}

private class App(pi: PackageInfo, pm: PackageManager, ai: ApplicationInfo = pi.applicationInfo) : Comparable<App> {
    val name: String?
    val packageName: String = pi.packageName
    val icon: Boolean = ai.icon != 0
    val isLaunchable: Boolean = pm.getLaunchIntentForPackage(packageName) != null

    init {
        val name = ai.loadLabel(pm).toString()
        this.name = if (name != packageName) name else null
        ai.flags and ApplicationInfo.FLAG_SYSTEM
    }

    override fun compareTo(other: App): Int {
        val r = other.isLaunchable.compareTo(isLaunchable)
        if (r != 0) return r
        if (name != other.name) {
            return when {
                name == null -> 1
                other.name == null -> -1
                else -> String.CASE_INSENSITIVE_ORDER.compare(name, other.name)
            }
        }
        return String.CASE_INSENSITIVE_ORDER.compare(packageName, other.packageName)
    }

    fun toJson(): JSONObject {
        val jsonObject = JSONObject()
        jsonObject.put("package", packageName)
        jsonObject.put("name", name)
        jsonObject.put("icon", icon)
        jsonObject.put("launchable", isLaunchable)
        return jsonObject
    }
}