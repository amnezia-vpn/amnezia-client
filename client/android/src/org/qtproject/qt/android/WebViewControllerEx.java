package org.qtproject.qt.android;

import android.view.ViewGroup;

public class WebViewControllerEx {

    public static ViewGroup.LayoutParams createQtLayoutParams(int width, int height, int x, int y) {
        return new QtLayout.LayoutParams(width, height, x, y);
    }
}
