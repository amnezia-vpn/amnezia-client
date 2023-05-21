/*
 * Copyright (C) 2015 Tobias Brunner
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

import java.util.Arrays;
import java.util.LinkedHashMap;
import java.util.Map.Entry;
import java.util.regex.Pattern;


/**
 * Simple generator for data/files that may be parsed by libstrongswan's
 * settings_t class.
 */
public class SettingsWriter
{
	/**
	 * Top-level section
	 */
	private final SettingsSection mTop = new SettingsSection();

	/**
	 * Set a string value
	 * @param key
	 * @param value
	 * @return the writer
	 */
	public SettingsWriter setValue(String key, String value)
	{
		Pattern pattern = Pattern.compile("[^#{}=\"\\n\\t ]+");
		if (key == null || !pattern.matcher(key).matches())
		{
			return this;
		}
		String[] keys = key.split("\\.");
		SettingsSection section = mTop;
		section = findOrCreateSection(Arrays.copyOfRange(keys, 0, keys.length-1));
		section.Settings.put(keys[keys.length-1], value);
		return this;
	}

	/**
	 * Set an integer value
	 * @param key
	 * @param value
	 * @return the writer
	 */
	public SettingsWriter setValue(String key, Integer value)
	{
		return setValue(key, value == null ? null : value.toString());
	}

	/**
	 * Set a boolean value
	 * @param key
	 * @param value
	 * @return the writer
	 */
	public SettingsWriter setValue(String key, Boolean value)
	{
		return setValue(key, value == null ? null : value ? "1" : "0");
	}

	/**
	 * Serializes the settings to a string in the format understood by
	 * libstrongswan's settings_t parser.
	 * @return serialized settings
	 */
	public String serialize()
	{
		StringBuilder builder = new StringBuilder();
		serializeSection(mTop, builder);
		return builder.toString();
	}

	/**
	 * Serialize the settings in a section and recursively serialize sub-sections
	 * @param section
	 * @param builder
	 */
	private void serializeSection(SettingsSection section, StringBuilder builder)
	{
		for (Entry<String, String> setting : section.Settings.entrySet())
		{
			builder.append(setting.getKey()).append('=');
			if (setting.getValue() != null)
			{
				builder.append("\"").append(escapeValue(setting.getValue())).append("\"");
			}
			builder.append('\n');
		}

		for (Entry<String, SettingsSection> subsection : section.Sections.entrySet())
		{
			builder.append(subsection.getKey()).append(" {\n");
			serializeSection(subsection.getValue(), builder);
			builder.append("}\n");
		}
	}

	/**
	 * Escape value so it may be wrapped in "
	 * @param value
	 * @return
	 */
	private String escapeValue(String value)
	{
		return value.replace("\\", "\\\\").replace("\"", "\\\"");
	}

	/**
	 * Find or create the nested sections with the given names
	 * @param sections list of section names
	 * @return final section
	 */
	private SettingsSection findOrCreateSection(String[] sections)
	{
		SettingsSection section = mTop;
		for (String name : sections)
		{
			SettingsSection subsection = section.Sections.get(name);
			if (subsection == null)
			{
				subsection = new SettingsSection();
				section.Sections.put(name, subsection);
			}
			section = subsection;
		}
		return section;
	}

	/**
	 * A section containing sub-sections and settings.
	 */
	private class SettingsSection
	{
		/**
		 * Assigned key/value pairs
		 */
		LinkedHashMap<String,String> Settings = new LinkedHashMap<String, String>();

		/**
		 * Assigned sub-sections
		 */
		LinkedHashMap<String,SettingsSection> Sections = new LinkedHashMap<String, SettingsWriter.SettingsSection>();
	}
}
