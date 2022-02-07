package org.ftylitak.qzxing;

import android.Manifest;
import android.app.Activity;
import android.content.pm.PackageManager;
import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import java.util.ArrayList;

public class Utilities {

    public static final int REQUEST_CAMERA = 0;

    public static final String[] requiredPermissionsModifyPhoneState = {
            Manifest.permission.CAMERA,
            Manifest.permission.READ_EXTERNAL_STORAGE,
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    public static void checkAndRequestPermissionList(Activity activity, String[] permissions) {
        ArrayList<String> permissionsToRequest = new ArrayList<>();
        for (int i = 0; i < permissions.length; i++) {
            if (ContextCompat.checkSelfPermission(activity, permissions[i])
                    != PackageManager.PERMISSION_GRANTED)
                permissionsToRequest.add(permissions[i]);
        }

        if (permissionsToRequest.size() != 0)
            ActivityCompat.requestPermissions(activity,
                    permissionsToRequest.toArray(new String[0]),
                    REQUEST_CAMERA);
        else
            NativeFunctions.onPermissionsGranted();
    }

    public static void requestQZXingPermissions(Activity activity) {
        checkAndRequestPermissionList(activity, requiredPermissionsModifyPhoneState);
    }


}
