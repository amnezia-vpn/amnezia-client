package org.amnezia.vpn

import android.os.Build
import android.os.Bundle
import androidx.biometric.BiometricManager
import androidx.biometric.BiometricManager.Authenticators.BIOMETRIC_STRONG
import androidx.biometric.BiometricManager.Authenticators.DEVICE_CREDENTIAL
import androidx.biometric.BiometricPrompt
import androidx.biometric.BiometricPrompt.AuthenticationResult
import androidx.core.content.ContextCompat
import androidx.fragment.app.FragmentActivity
import org.amnezia.vpn.qt.QtAndroidController
import org.amnezia.vpn.util.Log

private const val TAG = "AuthActivity"

private const val AUTHENTICATORS = BIOMETRIC_STRONG or DEVICE_CREDENTIAL

class AuthActivity : FragmentActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val biometricManager = BiometricManager.from(applicationContext)
        when (biometricManager.canAuthenticate(AUTHENTICATORS)) {
            BiometricManager.BIOMETRIC_SUCCESS -> {
                showBiometricPrompt(biometricManager)
                return
            }

            BiometricManager.BIOMETRIC_STATUS_UNKNOWN -> {
                Log.w(TAG, "Unknown biometric status")
                showBiometricPrompt(biometricManager)
                return
            }

            BiometricManager.BIOMETRIC_ERROR_UNSUPPORTED -> {
                Log.e(TAG, "The specified options are incompatible with the current Android " +
                    "version ${Build.VERSION.SDK_INT}")
            }

            BiometricManager.BIOMETRIC_ERROR_HW_UNAVAILABLE -> {
                Log.w(TAG, "The hardware is unavailable")
            }

            BiometricManager.BIOMETRIC_ERROR_NONE_ENROLLED -> {
                Log.w(TAG, "No biometric or device credential is enrolled")
            }

            BiometricManager.BIOMETRIC_ERROR_NO_HARDWARE -> {
                Log.w(TAG, "There is no suitable hardware")
            }

            BiometricManager.BIOMETRIC_ERROR_SECURITY_UPDATE_REQUIRED -> {
                Log.w(TAG, "A security vulnerability has been discovered with one or " +
                    "more hardware sensors")
            }
        }
        QtAndroidController.onAuthResult(true)
        finish()
    }

    private fun showBiometricPrompt(biometricManager: BiometricManager) {
        val executor = ContextCompat.getMainExecutor(applicationContext)
        val biometricPrompt = BiometricPrompt(this, executor,
            object : BiometricPrompt.AuthenticationCallback() {
                override fun onAuthenticationSucceeded(result: AuthenticationResult) {
                    super.onAuthenticationSucceeded(result)
                    Log.v(TAG, "Authentication succeeded")
                    QtAndroidController.onAuthResult(true)
                    finish()
                }

                override fun onAuthenticationFailed() {
                    super.onAuthenticationFailed()
                    Log.w(TAG, "Authentication failed")
                }

                override fun onAuthenticationError(errorCode: Int, errString: CharSequence) {
                    super.onAuthenticationError(errorCode, errString)
                    Log.e(TAG, "Authentication error $errorCode: $errString")
                    QtAndroidController.onAuthResult(false)
                    finish()
                }
            })



        val promptInfo = BiometricPrompt.PromptInfo.Builder()
            .setAllowedAuthenticators(AUTHENTICATORS)
            .setTitle("AmneziaVPN")
            .setSubtitle(biometricManager.getStrings(AUTHENTICATORS)?.promptMessage)
            .build()

        biometricPrompt.authenticate(promptInfo)
    }
}
