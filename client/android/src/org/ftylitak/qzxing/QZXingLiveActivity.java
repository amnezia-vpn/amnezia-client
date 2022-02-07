package org.ftylitak.qzxing;

import android.Manifest;
import android.content.pm.PackageManager;
import org.qtproject.qt5.android.bindings.QtActivity;
import static org.ftylitak.qzxing.Utilities.REQUEST_CAMERA;

public class QZXingLiveActivity extends QtActivity {
    @Override
    public void onRequestPermissionsResult(int requestCode,
                                           String permissions[], int[] grantResults) {
        switch (requestCode) {
            case REQUEST_CAMERA: {
                // If request is cancelled, the result arrays are empty.
                if (grantResults.length > 0
                        && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                    NativeFunctions.onPermissionsGranted();
                } else {
                    NativeFunctions.onPermissionsDenied();
                }
                return;
            }
        }
    }
}
