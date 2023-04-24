/*
 * Copyright (C) 2013 Tobias Brunner
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

import java.nio.ByteBuffer;

/**
 * Very similar to ByteBuffer (although with a stripped interface) but it
 * automatically resizes the underlying buffer.
 */
public class BufferedByteWriter
{
	/**
	 * The underlying byte buffer
	 */
	private byte[] mBuffer;

	/**
	 * ByteBuffer used as wrapper around the buffer to easily convert values
	 */
	private ByteBuffer mWriter;

	/**
	 * Create a writer with a default initial capacity
	 */
	public BufferedByteWriter()
	{
		this(0);
	}

	/**
	 * Create a writer with the given initial capacity (helps avoid expensive
	 * resizing if known).
	 * @param capacity initial capacity
	 */
	public BufferedByteWriter(int capacity)
	{
		capacity = capacity > 4 ? capacity : 32;
		mBuffer = new byte[capacity];
		mWriter = ByteBuffer.wrap(mBuffer);
	}

	/**
	 * Ensure that there is enough space available to write the requested
	 * number of bytes. If necessary the internal buffer is resized.
	 * @param required required number of bytes
	 */
	private void ensureCapacity(int required)
	{
		if (mWriter.remaining() >= required)
		{
			return;
		}
		byte[] buffer = new byte[(mBuffer.length + required) * 2];
		System.arraycopy(mBuffer, 0, buffer, 0, mWriter.position());
		mBuffer = buffer;
		ByteBuffer writer = ByteBuffer.wrap(buffer);
		writer.position(mWriter.position());
		mWriter = writer;
	}

	/**
	 * Write the given byte array to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put(byte[] value)
	{
		ensureCapacity(value.length);
		mWriter.put(value);
		return this;
	}

	/**
	 * Write the given byte to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put(byte value)
	{
		ensureCapacity(1);
		mWriter.put(value);
		return this;
	}

	/**
	 * Write the 8-bit length of the given data followed by the data itself
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter putLen8(byte[] value)
	{
		ensureCapacity(1 + value.length);
		mWriter.put((byte)value.length);
		mWriter.put(value);
		return this;
	}

	/**
	 * Write the 16-bit length of the given data followed by the data itself
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter putLen16(byte[] value)
	{
		ensureCapacity(2 + value.length);
		mWriter.putShort((short)value.length);
		mWriter.put(value);
		return this;
	}

	/**
	 * Write the given short value (16-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put16(byte value)
	{
		return this.put16((short)(value & 0xFF));
	}

	/**
	 * Write the given short value (16-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put16(short value)
	{
		ensureCapacity(2);
		mWriter.putShort(value);
		return this;
	}

	/**
	 * Write 24-bit of the given value in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put24(byte value)
	{
		ensureCapacity(3);
		mWriter.putShort((short)0);
		mWriter.put(value);
		return this;
	}

	/**
	 * Write 24-bit of the given value in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put24(short value)
	{
		ensureCapacity(3);
		mWriter.put((byte)0);
		mWriter.putShort(value);
		return this;
	}

	/**
	 * Write 24-bit of the given value in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put24(int value)
	{
		ensureCapacity(3);
		mWriter.put((byte)(value >> 16));
		mWriter.putShort((short)value);
		return this;
	}

	/**
	 * Write the given int value (32-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put32(byte value)
	{
		return put32(value & 0xFF);
	}

	/**
	 * Write the given int value (32-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put32(short value)
	{
		return put32(value & 0xFFFF);
	}

	/**
	 * Write the given int value (32-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put32(int value)
	{
		ensureCapacity(4);
		mWriter.putInt(value);
		return this;
	}

	/**
	 * Write the given long value (64-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put64(byte value)
	{
		return put64(value & 0xFFL);
	}

	/**
	 * Write the given long value (64-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put64(short value)
	{
		return put64(value & 0xFFFFL);
	}

	/**
	 * Write the given long value (64-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put64(int value)
	{
		return put64(value & 0xFFFFFFFFL);
	}

	/**
	 * Write the given long value (64-bit) in big-endian order to the buffer
	 * @param value
	 * @return the writer
	 */
	public BufferedByteWriter put64(long value)
	{
		ensureCapacity(8);
		mWriter.putLong(value);
		return this;
	}

	/**
	 * Convert the internal buffer to a new byte array.
	 * @return byte array
	 */
	public byte[] toByteArray()
	{
		int length = mWriter.position();
		byte[] bytes = new byte[length];
		System.arraycopy(mBuffer, 0, bytes, 0, length);
		return bytes;
	}
}
