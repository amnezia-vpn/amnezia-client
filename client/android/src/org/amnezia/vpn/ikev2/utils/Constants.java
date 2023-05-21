/*
 * Copyright (C) 2016-2020 Tobias Brunner
 *
 * Copyright (C) secunet Security Networks AG
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.  See <http://www.fsf.org/copyleft/gpl.txt>.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 */

package org.amnezia.vpn.ikev2.utils;

public final class Constants
{
	/**
	 * Intent action used to notify about changes to the VPN profiles
	 */
	public static final String VPN_PROFILES_CHANGED = "org.strongswan.android.VPN_PROFILES_CHANGED";

	/**
	 * Used in the intent above to notify about edits or inserts of a VPN profile (long)
	 */
	public static final String VPN_PROFILES_SINGLE = "org.strongswan.android.VPN_PROFILES_SINGLE";

	/**
	 * Used in the intent above to notify about the deletion of multiple VPN profiles (array of longs)
	 */
	public static final String VPN_PROFILES_MULTIPLE = "org.strongswan.android.VPN_PROFILES_MULTIPLE";

	/**
	 * Limits for MTU
	 */
	public static final int MTU_MAX = 1500;
	public static final int MTU_MIN = 1280;

	/**
	 * Limits for NAT-T keepalive
	 */
	public static final int NAT_KEEPALIVE_MAX = 120;
	public static final int NAT_KEEPALIVE_MIN = 10;

	/**
	 * Preference key for default VPN profile
	 */
	public static final String PREF_DEFAULT_VPN_PROFILE = "pref_default_vpn_profile";

	/**
	 * Value used to signify that the most recently used profile should be used as default
	 */
	public static final String PREF_DEFAULT_VPN_PROFILE_MRU = "pref_default_vpn_profile_mru";

	/**
	 * Preference key to store the most recently used VPN profile
	 */
	public static final String PREF_MRU_VPN_PROFILE = "pref_mru_vpn_profile";

	/**
	 * Preference key to store whether the user permanently dismissed our warning to add the app to the power whitelist
	 */
	public static final String PREF_IGNORE_POWER_WHITELIST = "pref_ignore_power_whitelist";
}
