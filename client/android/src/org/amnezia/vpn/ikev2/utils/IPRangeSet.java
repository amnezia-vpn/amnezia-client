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

import java.util.ArrayList;
import java.util.Collection;
import java.util.Iterator;
import java.util.List;
import java.util.TreeSet;

/**
 * Class that represents a set of IP address ranges (not necessarily proper subnets) and allows
 * modifying the set and enumerating the resulting subnets.
 */
public class IPRangeSet implements Iterable<IPRange>
{
	private TreeSet<IPRange> mRanges = new TreeSet<>();

	/**
	 * Parse the given string (space separated ranges in CIDR or range notation) and return the
	 * resulting set or {@code null} if the string was invalid. An empty set is returned if the given string
	 * is {@code null}.
	 */
	public static IPRangeSet fromString(String ranges)
	{
		IPRangeSet set = new IPRangeSet();
		if (ranges != null)
		{
			for (String range : ranges.split("\\s+"))
			{
				try
				{
					set.add(new IPRange(range));
				}
				catch (Exception unused)
				{	/* besides due to invalid strings exceptions might get thrown if the string
					 * contains a hostname (NetworkOnMainThreadException) */
					return null;
				}
			}
		}
		return set;
	}

	/**
	 * Add a range to this set. Automatically gets merged with existing ranges.
	 */
	public void add(IPRange range)
	{
		if (mRanges.contains(range))
		{
			return;
		}
		reinsert:
		while (true)
		{
			Iterator<IPRange> iterator = mRanges.iterator();
			while (iterator.hasNext())
			{
				IPRange existing = iterator.next();
				IPRange replacement = existing.merge(range);
				if (replacement != null)
				{
					iterator.remove();
					range = replacement;
					continue reinsert;
				}
			}
			mRanges.add(range);
			break;
		}
	}

	/**
	 * Add all ranges from the given set.
	 */
	public void add(IPRangeSet ranges)
	{
		if (ranges == this)
		{
			return;
		}
		for (IPRange range : ranges.mRanges)
		{
			add(range);
		}
	}

	/**
	 * Add all ranges from the given collection to this set.
	 */
	public void addAll(Collection<? extends IPRange> coll)
	{
		for (IPRange range : coll)
		{
			add(range);
		}
	}

	/**
	 * Remove the given range from this set. Existing ranges are automatically adjusted.
	 */
	public void remove(IPRange range)
	{
		ArrayList <IPRange> additions = new ArrayList<>();
		Iterator<IPRange> iterator = mRanges.iterator();
		while (iterator.hasNext())
		{
			IPRange existing = iterator.next();
			List<IPRange> result = existing.remove(range);
			if (result.size() == 0)
			{
				iterator.remove();
			}
			else if (!result.get(0).equals(existing))
			{
				iterator.remove();
				additions.addAll(result);
			}
		}
		mRanges.addAll(additions);
	}

	/**
	 * Remove the given ranges from ranges in this set.
	 */
	public void remove(IPRangeSet ranges)
	{
		if (ranges == this)
		{
			mRanges.clear();
			return;
		}
		for (IPRange range : ranges.mRanges)
		{
			remove(range);
		}
	}

	/**
	 * Get all the subnets derived from all the ranges in this set.
	 */
	public Iterable<IPRange> subnets()
	{
		return new Iterable<IPRange>()
		{
			@Override
			public Iterator<IPRange> iterator()
			{
				return new Iterator<IPRange>()
				{
					private Iterator<IPRange> mIterator = mRanges.iterator();
					private List<IPRange> mSubnets;

					@Override
					public boolean hasNext()
					{
						return (mSubnets != null && mSubnets.size() > 0) || mIterator.hasNext();
					}

					@Override
					public IPRange next()
					{
						if (mSubnets == null || mSubnets.size() == 0)
						{
							IPRange range = mIterator.next();
							mSubnets = range.toSubnets();
						}
						return mSubnets.remove(0);
					}

					@Override
					public void remove()
					{
						throw new UnsupportedOperationException();
					}
				};
			}
		};
	}

	@Override
	public Iterator<IPRange> iterator()
	{
		return mRanges.iterator();
	}

	/**
	 * Returns the number of ranges, not subnets.
	 */
	public int size()
	{
		return mRanges.size();
	}

	@Override
	public String toString()
	{	/* we could use TextUtils, but that causes the unit tests to fail */
		StringBuilder sb = new StringBuilder();
		for (IPRange range : mRanges)
		{
			if (sb.length() > 0)
			{
				sb.append(" ");
			}
			sb.append(range.toString());
		}
		return sb.toString();
	}
}
