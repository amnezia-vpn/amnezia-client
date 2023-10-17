/*
 * Copyright (C) 2014-2019 Tobias Brunner
 * HSR Hochschule fuer Technik Rapperswil
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

package com.wireguard.config;


import java.net.InetAddress;
import java.net.UnknownHostException;

public class Utils
{
	static final char[] HEXDIGITS = "0123456789abcdef".toCharArray();

	/**
	 * Converts the given byte array to a hexadecimal string encoding.
	 *
	 * @param bytes byte array to convert
	 * @return hex string
	 */
	public static String bytesToHex(byte[] bytes)
	{
		char[] hex = new char[bytes.length * 2];
		for (int i = 0; i < bytes.length; i++)
		{
			int value = bytes[i];
			hex[i*2]   = HEXDIGITS[(value & 0xf0) >> 4];
			hex[i*2+1] = HEXDIGITS[ value & 0x0f];
		}
		return new String(hex);
	}

	/**
	 * Validate the given proposal string
	 *
	 * @param ike true for IKE, false for ESP
	 * @param proposal proposal string
	 * @return true if valid
	 */
	public native static boolean isProposalValid(boolean ike, String proposal);

	/**
	 * Parse an IP address without doing a name lookup
	 *
	 * @param address IP address string
	 * @return address bytes if valid
	 */
	private native static byte[] parseInetAddressBytes(String address);

	/**
	 * Parse an IP address without doing a name lookup (as compared to InetAddress.fromName())
	 *
	 * @param address IP address string
	 * @return address if valid
	 * @throws UnknownHostException if address is invalid
	 */
	public static InetAddress parseInetAddress(String address) throws UnknownHostException
	{
		byte[] bytes = parseInetAddressBytes(address);
		if (bytes == null)
		{
			throw new UnknownHostException();
		}
		return InetAddress.getByAddress(bytes);
	}
}
