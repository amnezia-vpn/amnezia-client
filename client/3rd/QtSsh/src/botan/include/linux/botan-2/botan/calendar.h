/*
* Calendar Functions
* (C) 1999-2009,2015 Jack Lloyd
* (C) 2015 Simon Warta (Kullo GmbH)
*
* Botan is released under the Simplified BSD License (see license.txt)
*/

#ifndef BOTAN_CALENDAR_H_
#define BOTAN_CALENDAR_H_

#include <botan/types.h>
#include <chrono>
#include <string>

namespace Botan {

/**
* Struct representing a particular date and time
*/
class BOTAN_PUBLIC_API(2,0) calendar_point
   {
   public:

      /** The year */
      uint32_t get_year() const { return year; }

      /** The month, 1 through 12 for Jan to Dec */
      uint32_t get_month() const { return month; }

      /** The day of the month, 1 through 31 (or 28 or 30 based on month */
      uint32_t get_day() const { return day; }

      /** Hour in 24-hour form, 0 to 23 */
      uint32_t get_hour() const { return hour; }

      /** Minutes in the hour, 0 to 60 */
      uint32_t get_minutes() const { return minutes; }

      /** Seconds in the minute, 0 to 60, but might be slightly
      larger to deal with leap seconds on some systems
      */
      uint32_t get_seconds() const { return seconds; }

      /**
      * Initialize a calendar_point
      * @param y the year
      * @param mon the month
      * @param d the day
      * @param h the hour
      * @param min the minute
      * @param sec the second
      */
      calendar_point(uint32_t y, uint32_t mon, uint32_t d, uint32_t h, uint32_t min, uint32_t sec) :
         year(y), month(mon), day(d), hour(h), minutes(min), seconds(sec) {}

      /**
      * Returns an STL timepoint object
      */
      std::chrono::system_clock::time_point to_std_timepoint() const;

      /**
      * Returns a human readable string of the struct's components.
      * Formatting might change over time. Currently it is RFC339 'iso-date-time'.
      */
      std::string to_string() const;

   BOTAN_DEPRECATED_PUBLIC_MEMBER_VARIABLES:
      /*
      The member variables are public for historical reasons. Use the get_xxx() functions
      defined above. These members will be made private in a future major release.
      */
      uint32_t year;
      uint32_t month;
      uint32_t day;
      uint32_t hour;
      uint32_t minutes;
      uint32_t seconds;
   };

/**
* Convert a time_point to a calendar_point
* @param time_point a time point from the system clock
* @return calendar_point object representing this time point
*/
BOTAN_PUBLIC_API(2,0) calendar_point calendar_value(
   const std::chrono::system_clock::time_point& time_point);

}

#endif
