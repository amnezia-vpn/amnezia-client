/*
 * Copyright (C) 2012-2017 Tobias Brunner
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

import java.net.InetAddress;
import java.net.UnknownHostException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.List;

import androidx.annotation.NonNull;

/**
 * Class that represents a range of IP addresses. This range could be a proper subnet, but that's
 * not necessarily the case (see {@code getPrefix} and {@code toSubnets}).
 */
public class IPRange implements Comparable<IPRange>
{
	private final byte[] mBitmask = { (byte)0x80, 0x40, 0x20, 0x10, 0x08, 0x04, 0x02, 0x01 };
	private byte[] mFrom;
	private byte[] mTo;
	private Integer mPrefix;

	/**
	 * Determine if the range is a proper subnet and, if so, what the network prefix is.
	 */
	private void determinePrefix()
	{
		boolean matching = true;

		mPrefix = mFrom.length * 8;
		for (int i = 0; i < mFrom.length; i++)
		{
			for (int bit = 0; bit < 8; bit++)
			{
				if (matching)
				{
					if ((mFrom[i] & mBitmask[bit]) != (mTo[i] & mBitmask[bit]))
					{
						mPrefix = (i * 8) + bit;
						matching = false;
					}
				}
				else
				{
					if ((mFrom[i] & mBitmask[bit]) != 0 || (mTo[i] & mBitmask[bit]) == 0)
					{
						mPrefix = null;
						return;
					}
				}
			}
		}
	}

	private IPRange(byte[] from, byte[] to)
	{
		mFrom = from;
		mTo = to;
		determinePrefix();
	}

	public IPRange(String from, String to) throws UnknownHostException
	{
		this(Utils.parseInetAddress(from), Utils.parseInetAddress(to));
	}

	public IPRange(InetAddress from, InetAddress to)
	{
		initializeFromRange(from, to);
	}

	private void initializeFromRange(InetAddress from, InetAddress to)
	{
		byte[] fa = from.getAddress(), ta = to.getAddress();
		if (fa.length != ta.length)
		{
			throw new IllegalArgumentException("Invalid range");
		}
		if (compareAddr(fa, ta) < 0)
		{
			mFrom = fa;
			mTo = ta;
		}
		else
		{
			mTo = fa;
			mFrom = ta;
		}
		determinePrefix();
	}

	public IPRange(String base, int prefix) throws UnknownHostException
	{
		this(Utils.parseInetAddress(base), prefix);
	}

	public IPRange(InetAddress base, int prefix)
	{
		this(base.getAddress(), prefix);
	}

	private IPRange(byte[] from, int prefix)
	{
		initializeFromCIDR(from, prefix);
	}

	private void initializeFromCIDR(byte[] from, int prefix)
	{
		if (from.length != 4 && from.length != 16)
		{
			throw new IllegalArgumentException("Invalid address");
		}
		if (prefix < 0 || prefix > from.length * 8)
		{
			throw new IllegalArgumentException("Invalid prefix");
		}
		byte[] to = from.clone();
		byte mask = (byte)(0xff << (8 - prefix % 8));
		int i = prefix / 8;

		if (i < from.length)
		{
			from[i] = (byte)(from[i] & mask);
			to[i] = (byte)(to[i] | ~mask);
			Arrays.fill(from, i+1, from.length, (byte)0);
			Arrays.fill(to, i+1, to.length, (byte)0xff);
		}
		mFrom = from;
		mTo = to;
		mPrefix = prefix;
	}

	public IPRange(String cidr) throws UnknownHostException
	{
		/* only verify the basic structure */
		if (!cidr.matches("(?i)^(([0-9.]+)|([0-9a-f:]+))(-(([0-9.]+)|([0-9a-f:]+))|(/\\d+))?$"))
		{
			throw new IllegalArgumentException("Invalid CIDR or range notation");
		}
		if (cidr.contains("-"))
		{
			String[] parts = cidr.split("-");
			InetAddress from = InetAddress.getByName(parts[0]);
			InetAddress to = InetAddress.getByName(parts[1]);
			initializeFromRange(from, to);
		}
		else
		{
			String[] parts = cidr.split("/");
			InetAddress addr = InetAddress.getByName(parts[0]);
			byte[] base = addr.getAddress();
			int prefix = base.length * 8;
			if (parts.length > 1)
			{
				prefix = Integer.parseInt(parts[1]);
			}
			initializeFromCIDR(base, prefix);
		}
	}

	/**
	 * Returns the first address of the range. The network ID in case this is a proper subnet.
	 */
	public InetAddress getFrom()
	{
		try
		{
			return InetAddress.getByAddress(mFrom);
		}
		catch (UnknownHostException ignored)
		{
			return null;
		}
	}

	/**
	 * Returns the last address of the range.
	 */
	public InetAddress getTo()
	{
		try
		{
			return InetAddress.getByAddress(mTo);
		}
		catch (UnknownHostException ignored)
		{
			return null;
		}
	}

	/**
	 * If this range is a proper subnet returns its prefix, otherwise returns null.
	 */
	public Integer getPrefix()
	{
		return mPrefix;
	}

	@Override
	public int compareTo(@NonNull IPRange other)
	{
		int cmp = compareAddr(mFrom, other.mFrom);
		if (cmp == 0)
		{	/* smaller ranges first */
			cmp = compareAddr(mTo, other.mTo);
		}
		return cmp;
	}

	@Override
	public boolean equals(Object o)
	{
		if (o == null || !(o instanceof IPRange))
		{
			return false;
		}
		return this == o || compareTo((IPRange)o) == 0;
	}

	@Override
	public String toString()
	{
		try
		{
			if (mPrefix != null)
			{
				return InetAddress.getByAddress(mFrom).getHostAddress() + "/" + mPrefix;
			}
			return InetAddress.getByAddress(mFrom).getHostAddress() + "-" +
				   InetAddress.getByAddress(mTo).getHostAddress();
		}
		catch (UnknownHostException ignored)
		{
			return super.toString();
		}
	}

	private int compareAddr(byte a[], byte b[])
	{
		if (a.length != b.length)
		{
			return (a.length < b.length) ? -1 : 1;
		}
		for (int i = 0; i < a.length; i++)
		{
			if (a[i] != b[i])
			{
				if (((int)a[i] & 0xff) < ((int)b[i] & 0xff))
				{
					return -1;
				}
				else
				{
					return 1;
				}
			}
		}
		return 0;
	}

	/**
	 * Check if this range fully contains the given range.
	 */
	public boolean contains(IPRange range)
	{
		return compareAddr(mFrom, range.mFrom) <= 0 && compareAddr(range.mTo, mTo) <= 0;
	}

	/**
	 * Check if this and the given range overlap.
	 */
	public boolean overlaps(IPRange range)
	{
		return !(compareAddr(mTo, range.mFrom) < 0 || compareAddr(range.mTo, mFrom) < 0);
	}

	private byte[] dec(byte[] addr)
	{
		for (int i = addr.length - 1; i >= 0; i--)
		{
			if (--addr[i] != (byte)0xff)
			{
				break;
			}
		}
		return addr;
	}

	private byte[] inc(byte[] addr)
	{
		for (int i = addr.length - 1; i >= 0; i--)
		{
			if (++addr[i] != 0)
			{
				break;
			}
		}
		return addr;
	}

	/**
	 * Remove the given range from the current range.  Returns a list of resulting ranges (these are
	 * not proper subnets). At most two ranges are returned, in case the given range is contained in
	 * this but does not equal it, which would result in an empty list (which is also the case if
	 * this range is fully contained in the given range).
	 */
	public List<IPRange> remove(IPRange range)
	{
		ArrayList<IPRange> list = new ArrayList<>();
		if (!overlaps(range))
		{	/*           | this | or | this |
		     * | range |                     | range | */
			list.add(this);
		}
		else if (!range.contains(this))
		{	/* we are not completely removed, so none of these cases applies:
			 * | this  | or  | this  |   or   | this  |
		     * | range |     | range   |    |   range | */
			if (compareAddr(mFrom, range.mFrom) < 0 && compareAddr(range.mTo, mTo) < 0)
			{	/* the removed range is completely within our boundaries:
				 * |    this    |
				 *   | range |   */
				list.add(new IPRange(mFrom, dec(range.mFrom.clone())));
				list.add(new IPRange(inc(range.mTo.clone()), mTo));
			}
			else
			{	/* one end is within our boundaries the other at or outside it:
			     * | this     | or    | this     | or | this    |  or  | this    |
			     * | range |       | range |            | range |           | range | */
				byte[] from = compareAddr(mFrom, range.mFrom) < 0 ? mFrom : inc(range.mTo.clone());
				byte[] to = compareAddr(mTo, range.mTo) > 0 ? mTo : dec(range.mFrom.clone());
				list.add(new IPRange(from, to));
			}
		}
		return list;
	}

	private boolean adjacent(IPRange range)
	{
		if (compareAddr(mTo, range.mFrom) < 0)
		{
			byte[] to = inc(mTo.clone());
			return compareAddr(to, range.mFrom) == 0;
		}
		byte[] from = dec(mFrom.clone());
		return compareAddr(from, range.mTo) == 0;
	}

	/**
	 * Merge two adjacent or overlapping ranges, returns null if it's not possible to merge them.
	 */
	public IPRange merge(IPRange range)
	{
		if (overlaps(range))
		{
			if (contains(range))
			{
				return this;
			}
			else if (range.contains(this))
			{
				return range;
			}
		}
		else if (!adjacent(range))
		{
			return null;
		}
		byte[] from = compareAddr(mFrom, range.mFrom) < 0 ? mFrom : range.mFrom;
		byte[] to = compareAddr(mTo, range.mTo) > 0 ? mTo : range.mTo;
		return new IPRange(from, to);
	}

	/**
	 * Split the given range into a sorted list of proper subnets.
	 */
	public List<IPRange> toSubnets()
	{
		ArrayList<IPRange> list = new ArrayList<>();
		if (mPrefix != null)
		{
			list.add(this);
		}
		else
		{
			int i = 0, bit = 0, prefix, netmask, common_byte, common_bit;
			int from_cur, from_prev = 0, to_cur, to_prev = 1;
			boolean from_full = true, to_full = true;

			byte[] from = mFrom.clone();
			byte[] to = mTo.clone();

			/* find a common prefix */
			while (i < from.length && (from[i] & mBitmask[bit]) == (to[i] & mBitmask[bit]))
			{
				if (++bit == 8)
				{
					bit = 0;
					i++;
				}
			}
			prefix = i * 8 + bit;

			/* at this point we know that the addresses are either equal, or that the
			 * current bits in the 'from' and 'to' addresses are 0 and 1, respectively.
			 * we now look at the rest of the bits as two binary trees (0=left, 1=right)
			 * where 'from' and 'to' are both leaf nodes.  all leaf nodes between these
			 * nodes are addresses contained in the range.  to collect them as subnets
			 * we follow the trees from both leaf nodes to their root node and record
			 * all complete subtrees (right for from, left for to) we come across as
			 * subnets.  in that process host bits are zeroed out.  if both addresses
			 * are equal we won't enter the loop below.
			 *      0_____|_____1       for the 'from' address we assume we start on a
			 *   0__|__ 1    0__|__1    left subtree (0) and follow the left edges until
			 *  _|_   _|_   _|_   _|_   we reach the root of this subtree, which is
			 * |   | |   | |   | |   |  either the root of this whole 'from'-subtree
			 * 0   1 0   1 0   1 0   1  (causing us to leave the loop) or the root node
			 * of the right subtree (1) of another node (which actually could be the
			 * leaf node we start from).  that whole subtree gets recorded as subnet.
			 * next we follow the right edges to the root of that subtree which again is
			 * either the 'from'-root or the root node in the left subtree (0) of
			 * another node.  the complete right subtree of that node is the next subnet
			 * we record.  from there we assume that we are in that right subtree and
			 * recursively follow right edges to its root.  for the 'to' address the
			 * procedure is exactly the same but with left and right reversed.
			 */
			if (++bit == 8)
			{
				bit = 0;
				i++;
			}
			common_byte = i;
			common_bit = bit;
			netmask = from.length * 8;
			for (i = from.length - 1; i >= common_byte; i--)
			{
				int bit_min = (i == common_byte) ? common_bit : 0;
				for (bit = 7; bit >= bit_min; bit--)
				{
					byte mask = mBitmask[bit];

					from_cur = from[i] & mask;
					if (from_prev == 0 && from_cur != 0)
					{	/* 0 -> 1: subnet is the whole current (right) subtree */
						list.add(new IPRange(from.clone(), netmask));
						from_full = false;
					}
					else if (from_prev != 0 && from_cur == 0)
					{	/* 1 -> 0: invert bit to switch to right subtree and add it */
						from[i] ^= mask;
						list.add(new IPRange(from.clone(), netmask));
						from_cur = 1;
					}
					/* clear the current bit */
					from[i] &= ~mask;
					from_prev = from_cur;

					to_cur = to[i] & mask;
					if (to_prev != 0 && to_cur == 0)
					{	/* 1 -> 0: subnet is the whole current (left) subtree */
						list.add(new IPRange(to.clone(), netmask));
						to_full = false;
					}
					else if (to_prev == 0 && to_cur != 0)
					{	/* 0 -> 1: invert bit to switch to left subtree and add it */
						to[i] ^= mask;
						list.add(new IPRange(to.clone(), netmask));
						to_cur = 0;
					}
					/* clear the current bit */
					to[i] &= ~mask;
					to_prev = to_cur;
					netmask--;
				}
			}

			if (from_full && to_full)
			{	/* full subnet (from=to or from=0.. and to=1.. after common prefix) - not reachable
				 * due to the shortcut at the top */
				list.add(new IPRange(from.clone(), prefix));
			}
			else if (from_full)
			{	/* full from subnet (from=0.. after prefix) */
				list.add(new IPRange(from.clone(), prefix + 1));
			}
			else if (to_full)
			{	/* full to subnet (to=1.. after prefix) */
				list.add(new IPRange(to.clone(), prefix + 1));
			}
		}
		Collections.sort(list);
		return list;
	}
}
