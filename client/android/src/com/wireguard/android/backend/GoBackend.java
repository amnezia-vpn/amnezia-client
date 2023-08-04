package com.wireguard.android.backend;


public final class GoBackend {
    private static final String TAG = "WireGuard/GoBackend";

    public static native String wgGetConfig(int handle);

    public static native int wgGetSocketV4(int handle);

    public static native int wgGetSocketV6(int handle);

    public static native void wgTurnOff(int handle);

    public static native int wgTurnOn(String ifName, int tunFd, String settings);

    public static native String wgVersion();

}
