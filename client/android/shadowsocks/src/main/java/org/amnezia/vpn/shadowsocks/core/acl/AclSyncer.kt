package org.amnezia.vpn.shadowsocks.core.acl

import android.content.Context
import androidx.work.*
import kotlinx.coroutines.Dispatchers
import java.io.IOException
import java.net.URL
import java.util.concurrent.TimeUnit

class AclSyncer(context: Context, workerParams: WorkerParameters) : CoroutineWorker(context, workerParams) {
    companion object {
        private const val KEY_ROUTE = "route"

        fun schedule(route: String) = WorkManager.getInstance().enqueueUniqueWork(route, ExistingWorkPolicy.REPLACE,
                OneTimeWorkRequestBuilder<AclSyncer>().run {
                    setInputData(Data.Builder().putString(KEY_ROUTE, route).build())
                    setConstraints(Constraints.Builder()
                            .setRequiredNetworkType(NetworkType.UNMETERED)
                            .setRequiresCharging(true)
                            .build())
                    setInitialDelay(10, TimeUnit.SECONDS)
                    build()
                })
    }

    override val coroutineContext get() = Dispatchers.IO

    override suspend fun doWork(): Result = try {
        val route = inputData.getString(KEY_ROUTE)!!
        val acl = URL("https://shadowsocks.org/acl/android/v1/$route.acl").openStream().bufferedReader()
                .use { it.readText() }
        Acl.getFile(route).printWriter().use { it.write(acl) }
        Result.success()
    } catch (e: IOException) {
        e.printStackTrace()
        Result.retry()
    }
}
