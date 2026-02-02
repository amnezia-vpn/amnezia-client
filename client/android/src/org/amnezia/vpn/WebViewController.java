package org.amnezia.vpn;

import android.annotation.SuppressLint;
import android.app.Activity;
import android.graphics.Bitmap;
import android.graphics.Canvas;
import android.os.Handler;
import android.os.Looper;
import android.os.SystemClock;
import android.util.Log;
import android.util.DisplayMetrics;
import android.view.ViewGroup;
import android.view.MotionEvent;
import android.webkit.*;
import android.net.http.SslError;
import android.os.Message;
import org.qtproject.qt.android.WebViewControllerEx;

import androidx.annotation.NonNull;
import androidx.annotation.Nullable;

import java.net.URLDecoder;
import java.io.ByteArrayInputStream;
import java.io.UnsupportedEncodingException;
import java.util.concurrent.Semaphore;
import android.app.Activity;
import android.view.View;
import android.widget.FrameLayout;

public class WebViewController
{
    private interface RequestFinished {
        void onRequestCompleted();
    }

    private String baseUrl = "";
    private static final String INTERNAL_BASE_URL = "file:///";
    private static final long GEOMETRY_STABLE_INTERVAL = 150; //ms wait geometry settle

    private final Activity m_activity;
    private final long m_id;
    private WebView m_webView = null;
    private ViewGroup m_layout = null;
    private boolean m_loading = true;
    private long mLastGeometryChange = 0L;

    private float m_displayDensity = (float) 1.0;

    public native void urlChanged(long viewId, String url);
    public native byte[] dataForUrl(long viewId, String url, StringBuilder mimeType, StringBuilder encoding);
    public native boolean canHandleUrl(long viewId, String url);
    private final Handler m_handler;

    private native void pageFinished(long id, String url);
    private native void pageStarted(long id, String url);
    private static final String TAG = WebViewController.class.getSimpleName();
    
    private class AndroidWebChromeClient extends WebChromeClient {
        @Override
        public boolean onCreateWindow(WebView view, boolean dialog, boolean userGesture, Message resultMsg)
        {
            // Prevent opening new windows/tabs - load URLs in the same WebView instead
            // This handles links with target="_blank" or window.open()
            // Return false to prevent creating new windows - URLs will be handled by shouldOverrideUrlLoading
            return false;
        }
    }

    public WebViewController(final Activity activity, final long id) {
        m_activity = activity;
        m_id = id;

        ViewGroup root = (ViewGroup)(((ViewGroup)(m_activity.findViewById(android.R.id.content))).getChildAt(0));
        if (root != null) {
            m_layout = root;
        }

        m_displayDensity = m_activity.getResources().getDisplayMetrics().density;

        m_handler = new Handler(Looper.getMainLooper());
        m_handler.post(new Runnable() {
            @SuppressLint("SetJavaScriptEnabled")
            @Override
            public void run() {
                m_webView = new WebView(m_activity);
                m_webView.setFocusable(true);

                m_webView.setFocusableInTouchMode(true);
                m_webView.getSettings().setJavaScriptEnabled(true);
                m_webView.getSettings().setAllowFileAccess(true);
                m_webView.getSettings().setAllowFileAccessFromFileURLs(true);
                m_webView.getSettings().setAllowUniversalAccessFromFileURLs(true);
                m_webView.getSettings().setAllowContentAccess(true);
                m_webView.getSettings().setBuiltInZoomControls(true);
                m_webView.getSettings().setDisplayZoomControls(false);
                m_webView.getSettings().setLoadWithOverviewMode(true);

                m_webView.getSettings().setSupportMultipleWindows(false); // Prevent opening new windows
                m_webView.getSettings().setMixedContentMode(WebSettings.MIXED_CONTENT_ALWAYS_ALLOW);

                m_webView.getSettings().setUseWideViewPort(true);
                m_webView.getSettings().setLoadWithOverviewMode(true);
                m_webView.getSettings().setLayoutAlgorithm(WebSettings.LayoutAlgorithm.NORMAL);

                m_webView.getSettings().setSupportZoom(true);
                m_webView.getSettings().setBuiltInZoomControls(true);
                m_webView.getSettings().setDisplayZoomControls(false);

                m_webView.setInitialScale(0);
                m_webView.setVisibility(android.view.View.INVISIBLE);
                
                // Ensure WebView can receive and handle touch events for link clicks
                m_webView.setClickable(true);
                m_webView.setLongClickable(true);
                m_webView.setHapticFeedbackEnabled(false);
                
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                    m_webView.setElevation(0f);
                }

                m_webView.setWebViewClient(buildWebViewClient());
                m_webView.setWebChromeClient(buildWebChromeClient());

                // Ensure IME appears on tap when focusing editable content
                m_webView.setOnTouchListener(new android.view.View.OnTouchListener() {
                    @Override
                    public boolean onTouch(android.view.View v, MotionEvent event) {
                        // Let WebView handle touch events normally for link clicks
                        // Only request focus on ACTION_UP to allow IME to appear for input fields
                        if (event.getAction() == MotionEvent.ACTION_UP) {
                            v.requestFocus();
                            // Do not show IME if app temporarily suppresses it
                            try {
                                android.view.inputmethod.InputMethodManager imm = (android.view.inputmethod.InputMethodManager) v.getContext().getSystemService(android.content.Context.INPUT_METHOD_SERVICE);
                                if (imm != null) {
                                    imm.showSoftInput(v, 0);
                                }
                            } catch (Throwable ignore) {}
                        }
                        // Return false to let WebView handle the touch event (for link clicks, etc.)
                        return false;
                    }
                });
            }
        });
    }

    private WebViewClient buildWebViewClient() {
        return new WebViewClient() {
            @Override
            public void onReceivedSslError (WebView view, SslErrorHandler handler, SslError error) {
                handler.proceed();
                Log.e(TAG, "SSL certificate error");
            }

            @Override
            public void onPageStarted (WebView view, String url, Bitmap favicon) {
                m_loading = true;

                String dataUrl = updateUrl(url);
                urlChanged(m_id, dataUrl);
                pageStarted(m_id, dataUrl);
            }

            @Override
            public void onPageFinished(WebView view, String url) {
                m_loading = false;

                String dataUrl = updateUrl(url);
                pageFinished(m_id, dataUrl);
                urlChanged(m_id, dataUrl);
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, String url) {
                Log.d(TAG, "shouldOverrideUrlLoading (deprecated): " + url);
                if (url == null || url.isEmpty()) {
                    return false;
                }
                
                urlChanged(m_id, url);

                // Always load URLs within WebView, don't open in external browser
                // Explicitly load the URL in the WebView and return true to indicate we handled it
                view.loadUrl(url);
                return true;
            }

            @Override
            public boolean shouldOverrideUrlLoading(WebView view, android.webkit.WebResourceRequest request) {
                String url = request.getUrl().toString();
                Log.d(TAG, "shouldOverrideUrlLoading (new): " + url + ", isMainFrame: " + request.isForMainFrame() + ", method: " + request.getMethod());
                
                if (url == null || url.isEmpty()) {
                    return false;
                }
                
                urlChanged(m_id, url);

                // Always load URLs within WebView, don't open in external browser
                // Handle main frame navigation (link clicks, form submissions, etc.)
                // For sub-resources (images, CSS, JS), let WebView handle normally by returning false
                if (request.isForMainFrame()) {
                    view.loadUrl(url);
                    return true;
                }
                // For sub-resources, let WebView handle normally
                return false;
            }

            @Override
            public WebResourceResponse shouldInterceptRequest(WebView view, String url) {

                if (url.startsWith("data:") || !canHandleUrl(m_id, url)) {
                    return super.shouldInterceptRequest(view, url);
                }

                StringBuilder mimeType = new StringBuilder();
                StringBuilder encoding = new StringBuilder();
                byte[] data = dataForUrl(m_id, url, mimeType, encoding);

                boolean isDataInvalid = (data == null) || (data.length == 0);
                if (isDataInvalid) {
                    Log.w(TAG, String.format("Invalid data received for url: %s", url));
                    return null;
                }
                if ((mimeType.length() == 0) || mimeType.toString().isEmpty()) {
                    Log.w(TAG, String.format("Invalid mimeType received for url: %s", url));
                }
                if ((encoding.length() == 0) || encoding.toString().isEmpty()) {
                    Log.w(TAG, String.format("Invalid encoding received for url: %s", url));
                }

                ByteArrayInputStream dataStream = new ByteArrayInputStream(data);
                return new WebResourceResponse(mimeType.toString(), encoding.toString(), dataStream);
            }
        };
    }

    private WebChromeClient buildWebChromeClient() {
        return new AndroidWebChromeClient() {
            @Override
            public void onProgressChanged(WebView view, int newProgress) {
                super.onProgressChanged(view, newProgress);
                if (newProgress == 100) {
                    m_loading = false;
                }
            }

            @Override
            public void onGeolocationPermissionsShowPrompt(String origin, GeolocationPermissions.Callback callback) {
                callback.invoke(origin, true, false);
            }
        };
    }

    private String updateUrl(String url) {
        if (!url.startsWith(INTERNAL_BASE_URL)) {
            return url;
        }

        String dataUrl = url;

        try {
            dataUrl = URLDecoder.decode(url.substring(INTERNAL_BASE_URL.length()), "UTF-8");
            if ((dataUrl.length() == 1) && dataUrl.endsWith("#")) {
                dataUrl = baseUrl;
            } else {
                dataUrl = baseUrl + dataUrl;
            }
        } catch (UnsupportedEncodingException e) {
            Log.e(TAG, e.toString());
        }

        return dataUrl;
    }

    public void release() {
        if (m_handler == null) {
            return;
        }

        m_handler.post(() -> {
            if (m_webView == null) {
                return;
            }
            m_webView.setVisibility(android.view.View.INVISIBLE);
            m_layout.removeView(m_webView);
            m_webView.stopLoading();
            m_webView.setWebViewClient(new WebViewClient());
            m_webView.setWebChromeClient(null);
            m_webView = null;
        });
    }

  public void setGeometry(final int x, final int y, final int width, final int height) {

  
    Log.d(TAG, String.format(
            "setGeometry called: x=%d, y=%d, width=%d, height=%d",
            x, y, width, height));


    if (m_handler == null)
        return;

    m_handler.post(() -> {
        if (m_webView == null || m_layout == null)
            return;

        if (m_layout.indexOfChild(m_webView) < 0) {
            m_layout.addView(m_webView);
        }

        float scale = m_activity.getResources().getDisplayMetrics().density;

        int pxX = Math.round(x * scale);
        int pxY = Math.round(y * scale);
        int pxW = Math.round(width * scale);
        int pxH = Math.round(height * scale);
        
        Log.d(TAG, String.format(
                "density=%.2f qml: x=%d y=%d w=%d h=%d -> px: x=%d y=%d w=%d h=%d",
                scale, x, y, width, height, pxX, pxY, pxW, pxH));

        ViewGroup.LayoutParams params =
                WebViewControllerEx.createQtLayoutParams(
                        pxW,
                        pxH,
                        pxX,
                        pxY
                );

        m_webView.setLayoutParams(params);
        m_webView.setInitialScale(0);

        Log.d(TAG, String.format(
                "WebView positioned (QtLayout) at px: x=%d, y=%d, w=%d, h=%d",
                pxX, pxY, pxW, pxH));

        m_layout.requestLayout();
        m_webView.requestLayout();

        mLastGeometryChange = SystemClock.uptimeMillis();
    });
}


    public void show() {
        if (m_handler == null) {
            return;
        }

        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                if (m_webView.getVisibility() != android.view.View.VISIBLE) {
                    m_webView.setVisibility(android.view.View.VISIBLE);
                }
                // Don't bring WebView to front - let QML elements render on top
                // Set low elevation so QML elements can appear above WebView
                if (android.os.Build.VERSION.SDK_INT >= android.os.Build.VERSION_CODES.LOLLIPOP) {
                    m_webView.setElevation(0f);
                }
                m_webView.requestLayout();
                m_layout.requestLayout();
                m_layout.postInvalidate();
            }
        });
    }

    public void hide() {
        hideWebView();
        long now = SystemClock.uptimeMillis();
    }

    private void hideWebView() {
        if (m_handler == null) {
            return;
        }
        m_handler.post(() -> {
            if (m_webView == null) {
                return;
            }
            if (m_webView.getVisibility() == android.view.View.VISIBLE) {
                m_webView.setVisibility(android.view.View.INVISIBLE);
            }
        });
    }

    public void loadUrl(String url) {
        final String newUrl;
        if ((baseUrl.length() > 0) && url.startsWith(baseUrl)) {
            newUrl = INTERNAL_BASE_URL + url.substring(baseUrl.length());
        } else {
            newUrl = url;
        }

        if (m_handler == null) {
            return;
        }

        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                m_webView.loadUrl(newUrl);
            }
        });
    }

    public void loadDataWithBaseURL(final String url, final String html, final String mime, final String encoding) {
        baseUrl = url.trim();
        if (m_handler == null) {
            return;
        }

        m_handler.postDelayed(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }

                m_webView.loadUrl("about:blank");
                m_webView.loadDataWithBaseURL(INTERNAL_BASE_URL, html, mime, encoding, null);
            }
        }, 100);
    }

    public void evaluateJavaScript(final String script) {
        if (m_handler == null || script == null) {
            return;
        }

        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                m_webView.evaluateJavascript(script, null);
            }
        });
    }

    public boolean canGoBack() {
        final WebView view = m_webView;
        if (view == null) {
            return false;
        }

        final boolean[] ret = new boolean[1];
        ret[0] = false;
        boolean can = false;

        final Semaphore semaphore = new Semaphore(0);
        if (m_activity != null) {
            m_activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    ret[0] = view.canGoBack();
                    semaphore.release();
                }
            });
        }

        try {
            semaphore.acquire(1);
            can = ret[0];
        } catch (InterruptedException e) {
            Log.e(TAG, e.toString());
            Thread.currentThread().interrupt();
        }
        return can;
    }

    public void goBack() {
        if (m_handler == null) {
            return;
        }

        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                m_webView.goBack();
            }
        });
    }

    public boolean canGoForward() {
        final WebView view = m_webView;
        if (view == null) {
            return false;
        }

        final boolean[] ret = new boolean[1];
        ret[0] = false;
        boolean can = false;

        final Semaphore semaphore = new Semaphore(0);
        if (m_activity != null) {
            m_activity.runOnUiThread(new Runnable() {
                @Override
                public void run() {
                    ret[0] = view.canGoForward();
                    semaphore.release();
                }
            });
        }

        try {
            semaphore.acquire(1);
            can = ret[0];
        } catch (InterruptedException e) {
            Log.e(TAG, e.toString());
            Thread.currentThread().interrupt();
        }
        return can;
    }

    public void goForward() {
        if (m_handler == null) {
            return;
        }

        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                m_webView.goForward();
            }
        });
    }

    public void setBackgroundColor(final int color) {
        if (m_handler == null) {
            return;
        }

        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                m_webView.setBackgroundColor(color);
            }
        });
    }

    public void setTextZoom(final int percent) {
        if (m_handler == null) {
            return;
        }
        final int clamped = Math.max(25, Math.min(500, percent));
        m_handler.post(new Runnable() {
            @Override
            public void run() {
                if (m_webView == null) {
                    return;
                }
                m_webView.getSettings().setTextZoom(clamped);
            }
        });
    }

    private int convertToDp(int input) {
        return (int)(input / m_displayDensity + 0.5f);
    }

    public void setDefaultFontSize(int size) {
        if (m_handler == null) {
            return;
        }
        final int fontSize = size;
        m_handler.post(new Runnable() {
            @Override
            public void run() {
                m_webView.getSettings().setDefaultFontSize(convertToDp(fontSize));
            }
        });
    }

    void setStandardFontFamily(String family) {
        if (m_handler == null) {
            return;
        }

        final String fontFamily = family;
        m_handler.post(new Runnable() {
            @Override
            public void run() {
                m_webView.getSettings().setStandardFontFamily(fontFamily);
            }
        });
    }

}
