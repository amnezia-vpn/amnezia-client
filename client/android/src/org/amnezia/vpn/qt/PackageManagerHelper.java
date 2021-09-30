/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

package org.amnezia.vpn.qt;

import android.Manifest;
import android.content.Context;
import android.content.Intent;
import android.content.pm.ApplicationInfo;
import android.content.pm.PackageInfo;
import android.content.pm.PackageManager;
import android.content.pm.ResolveInfo;
import android.graphics.Color;
import android.graphics.drawable.ColorDrawable;
import android.graphics.drawable.Drawable;
import android.Manifest.permission;
import android.net.Uri;
import android.os.Build;
import android.util.Log;
import android.webkit.WebView;

import org.json.JSONException;
import org.json.JSONObject;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;
import java.util.regex.Pattern;

// Gets used by /platforms/android/androidAppListProvider.cpp
public class PackageManagerHelper {
  final static String TAG = "PackageManagerHelper";
  final static int MIN_CHROME_VERSION = 65;

  final static List<String> CHROME_BROWSERS = Arrays.asList(
      new String[] {"com.google.android.webview", "com.android.webview", "com.google.chrome"});

  private static String getAllAppNames(Context ctx) {
    JSONObject output = new JSONObject();
    PackageManager pm = ctx.getPackageManager();
    List<String> browsers = getBrowserIDs(pm);
    List<PackageInfo> packs = pm.getInstalledPackages(PackageManager.GET_PERMISSIONS);
    for (int i = 0; i < packs.size(); i++) {
      PackageInfo p = packs.get(i);
      // Do not add ourselves and System Apps to the list, unless it might be a browser
      if ((!isSystemPackage(p,pm) || browsers.contains(p.packageName))
          && !isSelf(p)) {
        String appid = p.packageName;
        String appName = p.applicationInfo.loadLabel(pm).toString();
        try {
          output.put(appid, appName);
        } catch (JSONException e) {
          e.printStackTrace();
        }
      }
    }
    return output.toString();
  }

  private static Drawable getAppIcon(Context ctx, String id) {
    try {
      return ctx.getPackageManager().getApplicationIcon(id);
    } catch (PackageManager.NameNotFoundException e) {
      e.printStackTrace();
    }
    return new ColorDrawable(Color.TRANSPARENT);
  }

  private static boolean isSystemPackage(PackageInfo pkgInfo, PackageManager pm) {
    if( (pkgInfo.applicationInfo.flags & ApplicationInfo.FLAG_SYSTEM) == 0){
      // no system app
      return false;
    }
    // For Systems Packages there are Cases where we want to add it anyway:
    // Has the use Internet permission (otherwise makes no sense)
    // Had at least 1 update (this means it's probably on any AppStore)
    // Has a a launch activity (has a ui and is not just a system service)

    if(!usesInternet(pkgInfo)){
      return true;
    }
    if(!hadUpdate(pkgInfo)){
      return true;
    }
    if(pm.getLaunchIntentForPackage(pkgInfo.packageName) == null){
      // If there is no way to launch this from a homescreen, def a sys package
      return true;
    }
    return false;
  }
  private static boolean isSelf(PackageInfo pkgInfo) {
    return pkgInfo.packageName.equals("org.amnezia.vpn")
        || pkgInfo.packageName.equals("org.amnezia.vpn.debug");
  }
  private static boolean usesInternet(PackageInfo pkgInfo){
    if(pkgInfo.requestedPermissions == null){
      return false;
    }
    for(int i=0; i < pkgInfo.requestedPermissions.length; i++) {
      String permission = pkgInfo.requestedPermissions[i];
      if(Manifest.permission.INTERNET.equals(permission)){
        return true;
      }
    }
    return false;
  }
  private static boolean hadUpdate(PackageInfo pkgInfo){
    return pkgInfo.lastUpdateTime > pkgInfo.firstInstallTime;
  }

  // Returns List of all Packages that can classify themselves as browsers
  private static List<String> getBrowserIDs(PackageManager pm) {
    Intent intent = new Intent(Intent.ACTION_VIEW, Uri.parse("https://www.mozilla.org/"));
    intent.addCategory(Intent.CATEGORY_BROWSABLE);
    // We've tried using PackageManager.MATCH_DEFAULT_ONLY flag and found that browsers that
    // are not set as the default browser won't be matched even if they had CATEGORY_DEFAULT set
    // in the intent filter

    List<ResolveInfo> resolveInfos = pm.queryIntentActivities(intent, PackageManager.MATCH_ALL);
    List<String> browsers = new ArrayList<String>();
    for (int i = 0; i < resolveInfos.size(); i++) {
      ResolveInfo info = resolveInfos.get(i);
      String browserID = info.activityInfo.packageName;
      browsers.add(browserID);
    }
    return browsers;
  }

  // Gets called in AndroidAuthenticationListener;
  public static boolean isWebViewSupported(Context ctx) {
    Log.v(TAG, "Checking if installed Webview is compatible with FxA");
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
      // The default Webview is able do to FXA
      return true;
    }
    if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
      PackageInfo pi = WebView.getCurrentWebViewPackage();
      if (CHROME_BROWSERS.contains(pi.packageName)) {
        return isSupportedChromeBrowser(pi);
      }
      return isNotAncientBrowser(pi);
    }

    // Before O the webview is hardcoded, but we dont know which package it is.
    // Check if com.google.android.webview is installed
    PackageManager pm = ctx.getPackageManager();
    try {
      PackageInfo pi = pm.getPackageInfo("com.google.android.webview", 0);
      return isSupportedChromeBrowser(pi);
    } catch (PackageManager.NameNotFoundException e) {
    }
    // Otherwise check com.android.webview
    try {
      PackageInfo pi = pm.getPackageInfo("com.android.webview", 0);
      return isSupportedChromeBrowser(pi);
    } catch (PackageManager.NameNotFoundException e) {
    }
    Log.e(TAG, "Android System WebView is not found");
    // Giving up :(
    return false;
  }

  private static boolean isSupportedChromeBrowser(PackageInfo pi) {
    Log.d(TAG, "Checking Chrome Based Browser: " + pi.packageName);
    Log.d(TAG, "version name: " + pi.versionName);
    Log.d(TAG, "version code: " + pi.versionCode);
    try {
      String versionCode = pi.versionName.split(Pattern.quote(" "))[0];
      String majorVersion = versionCode.split(Pattern.quote("."))[0];
      int version = Integer.parseInt(majorVersion);
      return version >= MIN_CHROME_VERSION;
    } catch (Exception e) {
      Log.e(TAG, "Failed to check Chrome Version Code " + pi.versionName);
      return false;
    }
  }

  private static boolean isNotAncientBrowser(PackageInfo pi) {
    // Not a google chrome - So the version name is worthless
    // Lets just make sure the WebView
    // used is not ancient ==> Was updated in at least the last 365 days
    Log.d(TAG, "Checking Chrome Based Browser: " + pi.packageName);
    Log.d(TAG, "version name: " + pi.versionName);
    Log.d(TAG, "version code: " + pi.versionCode);
    double oneYearInMillis = 31536000000L;
    return pi.lastUpdateTime > (System.currentTimeMillis() - oneYearInMillis);
  }
}
