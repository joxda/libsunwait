/*******************************************************************************
  Copyright(c) 2021 Joachim Janz. All rights reserved.

  This program is free software; you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by the Free
  Software Foundation; either version 2 of the License, or (at your option)
  any later version.

  This program is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
  more details.

  You should have received a copy of the GNU Library General Public License
  along with this library; see the file COPYING.LIB.  If not, write to
  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
  Boston, MA 02110-1301, USA.

  The full GNU General Public License is included in this distribution in the
  file called LICENSE.

  This library version is adapted from the sunwait executable licencsed under
  the GPLv3 and written by Dan Risacher based on codes by Paul Schlyter and
  with contributions of others mentioned in the original code which can be
  found in https://github.com/risacher/sunwait

*******************************************************************************/

// sunwait.cpp : Defines the entry point for the console application.
//

/* copyright (c) 2000,2004 Daniel Risacher */
/* minor changes courtesy of Dr. David M. MacMillan */
/* major changes courtesy of Ian Craig (2012-13) */
/* Licensed under the Gnu General Public License */

// Who  When        Ver  What
// IFC  2013-07-13  0.4  Working version of "sunwait", ported to Windows.
// IFC  2014-12-01  0.5  The linux port was not working - the windows sleep is for seconds, the linux is miliseconds.
//                       I'm porting my home power control to Linux, so I've reworked the whole program to suit.
// IFC  2014-12-08  0.6  Add timezone handling for output - if required
// IFC  2015-04-29  0.7  Timezone and DST fixes - and for date line timings
// IFC  2015-05-27  0.8  Resolve 'dodgy day' and cleanup
// TLJ  2020-10-03  0.9  Fix build on osx
//

#include <iostream>
#include <cstring>
#include <math.h>

#include <thread>
#include <chrono>
// Windows
#if defined _WIN32 || defined _WIN64
#include <windows.h>
#endif

// Linux
#if defined __linux__ || defined __APPLE__
//#include <unistd.h>
#endif


#include "libsunwait.hpp"
#include "sun.hpp"
#include "sunarc.hpp"

using namespace std;

static const char* cTo    = " to ";
static const char* cComma = ", ";


inline long   myRound (const double d)
{
    return d > 0.0 ? (int) (d + 0.5) : (int) (d - 0.5) ;
}
inline long   myTrunc (const double d)
{
    return (d > 0) ? (int) floor(d) : (int) ceil(d) ;
}
inline double myAbs   (const double d)
{
    return (d > 0) ? d : -d ;
}

inline int hours   (const double d)
{
    return myTrunc (d);
}
inline int minutes (const double d)
{
    return myTrunc (fmod (myAbs (d) * 60,   60));
}

/*
** time_t converted to  struct tm. Using GMT (UTC) time.
*/
inline  void myUtcTime (const time_t *pTimet, struct tm *pTm)
{
    /* Windows code: Start */
#if defined _WIN32 || defined _WIN64
    errno_t err;
    err = _gmtime64_s (pTm, pTimet);
    if (err)
    {
        printf ("Error: Invalid Argument to _gmtime64_s ().\n");
        exit (EXIT_ERROR);
    }
#endif
    /* Windows code: End */

    /* Linux code: Start */
#if defined __linux__ || defined __APPLE__
    gmtime_r (pTimet, pTm);
#endif
    /* Linux code: End */
}

/*
** time_t converted to  struct tm. Using local time.
*/
inline void myLocalTime (const time_t *pTimet, struct tm *pTm)
{
    /* Windows code: Start */
#if defined _WIN32 || defined _WIN64
    errno_t err;
    err = _localtime64_s (pTm, pTimet);
    if (err)
    {
        printf ("Error: Invalid Argument to _gmtime64_s ().\n");
        exit (EXIT_ERROR);
    }
#endif
    /* Windows code: End */

    /* Linux code: Start */
#if defined __linux__ || defined __APPLE__
    localtime_r (pTimet, pTm);
#endif
    /* Linux code: End */
}


//
// Utility functions
//

// Reduce angle to within 0..359.999 degrees
inline double revolution (const double x)
{
    double remainder = fmod (x, (double) 360.0);
    return remainder < (double) 0.0 ? remainder + (double) 360.0 : remainder;
}



inline unsigned long daysSince2000 (const time_t *pTimet)
{
    struct tm tmpTm;

    myUtcTime (pTimet, &tmpTm);

    unsigned int yearsSince2000 = tmpTm.tm_year - 100; // Get year, but tm_year starts from 1900

    // Calucate number of leap days, but -
    //   yearsSince2000 - 1
    // Don't include this year as tm_yday includes this year's leap day in the next bit

    unsigned int leapDaysSince2000
        = (unsigned int) floor ((yearsSince2000 - 1) / 4) // Every evenly divisible 4 years is a leap-year
          - (unsigned int) floor ((yearsSince2000 - 1) / 100) // Except centuries
          + (unsigned int) floor ((yearsSince2000 - 1) / 400) // Unless evenlt divisible by 400
          + 1;                                             // 2000 itself was a leap year with the 400 rule (a fix for 0/400 == 0)

    return (yearsSince2000 * 365) + leapDaysSince2000 + tmpTm.tm_yday;
}


inline  bool myIsNumber (const char *arg)
{
    bool digitSet = false;
    for (int i = 0; ; i++)
    {
        switch (arg[i])
        {
            case  '0':
            case  '1':
            case  '2':
            case  '3':
            case  '4':
            case  '5':
            case  '6':
            case  '7':
            case  '8':
            case  '9':
                digitSet = true;
                break;
            case  '-':
            case  '+':
                if (digitSet) return false;
                break;
            case '\0':
                return digitSet;
                break; // Exit OK
            default:
                return false;           // Exit Error
        }
    }
    return false; // Shouldn't get here
}


/*
** A "struct tm" time can be different from UTC because of TimeZone
** or Daylight Savings.  This function gives the difference - unit: hours.
**
** Usage:
** Add the UTC bias to convert from local-time to UTC.
** ptrTm is set
*/
inline double getUtcBiasHours (const time_t *pTimet)
{
    struct tm utcTm;
    double utcBiasHours = 0.0;

    // Populate "struct tm" with UTC data for the given day
    myUtcTime (pTimet, &utcTm);

    /* Windows code: Start */
#if defined _WIN32 || defined _WIN64
    struct tm utcNoonTm, localNoonTm;

    // Keep to the same day given, but go for noon. Daylight savings changes usually happen in the early hours.
    // mktime() changes the values in "struct tm", so I need to use a private one anyway.
    utcTm.tm_hour = 12;
    utcTm.tm_min  = 0;
    utcTm.tm_sec  = 0;

    // Now convert this time to time_t (which is always, by definition, UTC),
    // so I can run both of the two functions I can use that differentiate between timezones, using the same UTC moment.
    time_t noonTimet = mktime (&utcTm); // Unfortunately this is noonTimet is local time. It's the best I can do.
    // If it was UTC, all locations on earth are within the same day at noon.
    // (Because UTC = GMT.  Noon GMT +/- 12hrs nestles upto, but not across, the dateline)
    // Local-time 'days' (away from GMT) will probably cross the date line.

    myLocalTime (&noonTimet, &localNoonTm);  // Generate 'struct tm' for local time
    myUtcTime   (&noonTimet, &utcNoonTm);    // Generate 'struct tm' for UTC

    // This is not nice, but Visual Studio does not support "strftime(%z)" (get UTC bias) like linux does.
    // I'll figure out the UTC bias by comparing readings of local time to UTC for the same moment.

    // Although localTm and utcTm may be different, some secret magic ensures that mktime() will bring
    // them back to the same time_t value (as it should: they identify the same moment, just in different timezones).
    // I'll just have to work out the utcBias using the differing 'struct tm' values. It isn't pretty.

    utcBiasHours = (localNoonTm.tm_hour - utcNoonTm.tm_hour)
                   + (localNoonTm.tm_min  - utcNoonTm.tm_min) / 60.0;

    // The day may be different between the two times, especially if the local timezone is near the dateline.
    // Rollover of tm_yday (from 365 to 0) is a further problem, but no bias is ever more than 24 hours - that wouldn't make sense.

    if (localNoonTm.tm_year >  utcNoonTm.tm_year) utcBiasHours +=
            24.0; // Local time is in a new year, utc isn't:     so local time is a day ahead
    else if (localNoonTm.tm_year <  utcNoonTm.tm_year) utcBiasHours -=
            24.0; // Local time is in old year, utc is new year: so local time is a day behind
    else utcBiasHours += (localNoonTm.tm_yday - utcNoonTm.tm_yday) *
                             24.0;   // Year has not changed, so we can use tm_yday normaly
#endif
    /* Windows code: End */

    /* Linux code: Start */
#if defined __linux__ || defined __APPLE__
    char buffer [80];

    mktime (&utcTm); // Let "mktime()" do it's magic

    strftime (buffer, 80, "%z", &utcTm);

    if (strlen (buffer) > 0 && myIsNumber (buffer))
    {
        signed long int tmpLong = atol (buffer);
        utcBiasHours = (int)(tmpLong / 100 + (tmpLong % 100) / 60.0);
    }
#endif
    /* Linux code: End */

    return utcBiasHours;
}

/*
** Debug: What's the time (include timezone)?
*/
inline  void myDebugTime (const char * pTitleChar, const time_t *pTimet)
{
    struct tm tmpLocalTm, tmpUtcTm;
    char   utcBuffer [80];
    char localBuffer [80];

    // Convert current time to struct tm for UTC and local timezone
    myLocalTime (pTimet, &tmpLocalTm);
    myUtcTime   (pTimet, &tmpUtcTm);

    strftime (  utcBuffer, 80, "%c %Z", &tmpUtcTm);
    printf ("Debug: %s   utcTm:  %s\n", pTitleChar, utcBuffer);
    strftime (localBuffer, 80, "%c %Z", &tmpLocalTm);
    printf ("Debug: %s localTm:  %s\n", pTitleChar, localBuffer);

    // Difference between UTC and local TZ
    strftime (  utcBuffer, 80, "%Z",   &tmpUtcTm);
    strftime (localBuffer, 80, "%Z", &tmpLocalTm);
    printf ("Debug: %s UTC bias (add to %s to get %s) hours: %f\n", pTitleChar,  utcBuffer, localBuffer,
            getUtcBiasHours (pTimet));
}



/*
** Return time_t of midnight UTC on the day given
** In effect, this function is going to shave upto 24 hours off a time
** returning 00:00 UTC on the day given.
*/
inline  time_t getMidnightUTC (const time_t *pTimet)
{
    struct tm tmpTm;

    // Convert target "struct tm" to time_t.  It'll be set to midnight local time, on the target day.
    myUtcTime (pTimet, &tmpTm);

    // Set to the start of the day
    tmpTm.tm_hour = 0;
    tmpTm.tm_min  = 0;
    tmpTm.tm_sec  = 0;

    // Fiddle with other "struct tm" fields to get things right
    tmpTm.tm_wday  = 0;   // mktime will ignore this when calculating time_t as it contradicts days and months
    tmpTm.tm_yday  = 0;   // mktime will ignore this when calculating time_t as it contradicts days and months
    tmpTm.tm_isdst = -1;  // -1 means: mktime() must work it out. 0=DST not in effect. 1=DST in effect. (Daylight Savings)

    // Shave off (add) UTC offset, so that time_t is converted from midnight local-time to midnight UTC on the target day
    tmpTm.tm_sec += myRound (getUtcBiasHours (pTimet) * 3600.0);

    // Let mktime() do it's magic
    return mktime (&tmpTm);
}


void SunWait::setCoordinates(double lat, double lon)
{
    latitude = fixLatitude(lat);
    longitude = fixLongitude(lon);
}



// Fix angle to 0-359.999
double  SunWait::fixLongitude (const double x)
{
    return revolution (x);
}

// Fix angle to 0-89.999 and -0.001 to -89.999
double  SunWait::fixLatitude (const double x)
{
    // Make angle 0 to 359.9999
    double y = revolution (x);

    if (y <= (double)  90.0) ;
    else if (y <= (double) 180.0) y = (double) 180.0 - y;
    else if (y <= (double) 270.0) y = (double) 180.0 - y;
    else if (y <= (double) 360.0) y = y - (double) 360.0;

    // Linux compile of sunwait doesn't like 90, Windows is OK.
    // Let's just wiggle things a little bit to make things OK.
    if (y == (double)  90.0) y = (double)  89.9999999;
    else if (y == (double) -90.0) y = (double) -89.9999999;

    return y;
}

void SunWait::print_a_time( const time_t *pMidnightTimet, const double  pEventHour)
{
    struct tm tmpTm;
    char tmpBuffer [80];

    // Convert current time to struct tm for UTC or local timezone
    if (utc)
    {
        myUtcTime   (pMidnightTimet, &tmpTm);
        tmpTm.tm_min += (int) (pEventHour * 60.0);
        /*time_t x = */mktime (&tmpTm);
        //myUtcTime   (&x, &tmpTm);
    }
    else
    {
        myLocalTime (pMidnightTimet, &tmpTm);
        tmpTm.tm_min += (int) (pEventHour * 60.0);
        mktime (&tmpTm);
    }

    strftime (tmpBuffer, 80, "%H:%M", &tmpTm);
    printf ("%s", tmpBuffer);
}

void SunWait::print_a_sun_time( const time_t *pMidnightTimet, const double  pEventHour, const double  pOffsetDiurnalArc)
{
    // A positive offset reduces the diurnal arc
    if (pOffsetDiurnalArc <=  0.0 || pOffsetDiurnalArc >= 24.0)
        printf ("--:--");
    else
        print_a_time (pMidnightTimet, pEventHour);
}

void SunWait::print_times( const time_t   pMidnightTimet, SunArc  result, const double   pOffset, const char   *pSeparator)
{
    double offsetDiurnalArc = result.diurnalArcWithOffset (pOffset);
    double riseHour         = result.getOffsetRiseHourUTC (pOffset);
    double setHour          = result.getOffsetSetHourUTC  (pOffset);

    print_a_sun_time (&pMidnightTimet, riseHour, offsetDiurnalArc);
    printf ("%s", pSeparator);
    print_a_sun_time (&pMidnightTimet, setHour, offsetDiurnalArc);

    if (offsetDiurnalArc >= 24.0) printf (" (Midnight sun)");
    else if (offsetDiurnalArc <=  0.0) printf (" (Polar night)");

    printf ("\n");
}

void SunWait::generate_report (const int year, const int month, const int day)
{
    time_t targetTimet = targetTime(year, month, day);
    unsigned long t2000 = daysSince2000(&targetTimet);
    if (debug) myDebugTime ("Target:", &targetTimet);

    Sun sun(longitude, latitude, TWILIGHT_ANGLE_DAYLIGHT);
    SunArc tmpTarget = sun.riset(t2000);


    /*
    ** Now generate the report
    */
    time_t nowTimet;
    time(&nowTimet);
    struct tm nowTm;
    struct tm targetTm;
    char buffer [80];

    if (utc)
    {
        myUtcTime (&nowTimet,    &nowTm);
        myUtcTime (&targetTimet, &targetTm);
    }
    else
    {
        myLocalTime (&nowTimet,    &nowTm);
        myLocalTime (&targetTimet, &targetTm);
    }

    printf ("\n");

    strftime (buffer, 80, "%d-%b-%Y %H:%M %Z", &nowTm);
    printf
    ("      Current Date and Time: %s\n", buffer);

    printf ("\n\nTarget Information ...\n\n");

    printf
    ("                   Location: %10.6fN, %10.6fE\n"
     , latitude
     , longitude
    );

    strftime (buffer, 80, "%d-%b-%Y", &nowTm);
    printf
    ("                       Date: %s\n", buffer);

    strftime (buffer, 80, "%Z", &nowTm);
    printf
    ("                   Timezone: %s\n", buffer);

    printf
    ("   Sun directly north/south: ");
    print_a_time (&targetTimet, tmpTarget.southHourUTC);
    printf ("\n");

    if (offsetHour != NO_OFFSET)
    {
        printf
        ( "                     Offset: %2.2d:%2.2d hours\n"
          , hours   (offsetHour)
          , minutes (offsetHour)
        );
    }

    if (twilightAngle == TWILIGHT_ANGLE_DAYLIGHT)     printf("             Twilight angle: %5.2f degrees (daylight)\n",
                twilightAngle);
    else if (twilightAngle == TWILIGHT_ANGLE_CIVIL)        printf("             Twilight angle: %5.2f degrees (civil)\n",
                twilightAngle);
    else if (twilightAngle == TWILIGHT_ANGLE_NAUTICAL)     printf("             Twilight angle: %5.2f degrees (nautical)\n",
                twilightAngle);
    else if (twilightAngle == TWILIGHT_ANGLE_ASTRONOMICAL) printf("             Twilight angle: %5.2f degrees (astronomical)\n",
                twilightAngle);
    else
        printf("             Twilight angle: %5.2f degrees (custom angle)\n", twilightAngle);

    printf   ("          Day with twilight: ");
    print_times (targetTimet, tmpTarget, NO_OFFSET, cTo);

    if (offsetHour != NO_OFFSET)
    {
        printf (" Day with twilight & offset: ");
        print_times (targetTimet, tmpTarget, offsetHour, cTo);
    }

    printf   ("                      It is: %s\n", poll () == EXIT_DAY ? "Day (or twilight)" : "Night");

    /*
    ** Generate times for different types of twilight
    */

    SunArc  daylightTarget = sun.riset(t2000);

    sun.twilightAngle = TWILIGHT_ANGLE_CIVIL;
    SunArc civilTarget = sun.riset(t2000);

    sun.twilightAngle = TWILIGHT_ANGLE_NAUTICAL;
    SunArc  nauticalTarget = sun.riset(t2000);

    sun.twilightAngle = TWILIGHT_ANGLE_ASTRONOMICAL;
    SunArc astronomicalTarget = sun.riset(t2000);

    printf ("\nGeneral Information (no offset) ...\n\n");

    printf (" Times ...         Daylight: ");
    print_times (targetTimet, daylightTarget,  NO_OFFSET, cTo);
    printf ("        with Civil twilight: ");
    print_times (targetTimet, civilTarget,        NO_OFFSET, cTo);
    printf ("     with Nautical twilight: ");
    print_times (targetTimet, nauticalTarget,     NO_OFFSET, cTo);
    printf (" with Astronomical twilight: ");
    print_times (targetTimet, astronomicalTarget, NO_OFFSET, cTo);
    printf ("\n");
    printf (" Duration ...    Day length: %2.2d:%2.2d hours\n", hours (    daylightTarget.diurnalArc),
            minutes (    daylightTarget.diurnalArc));
    printf ("        with civil twilight: %2.2d:%2.2d hours\n", hours (       civilTarget.diurnalArc),
            minutes (       civilTarget.diurnalArc));
    printf ("     with nautical twilight: %2.2d:%2.2d hours\n", hours (    nauticalTarget.diurnalArc),
            minutes (    nauticalTarget.diurnalArc));
    printf (" with astronomical twilight: %2.2d:%2.2d hours\n", hours (astronomicalTarget.diurnalArc),
            minutes (astronomicalTarget.diurnalArc));
    printf ("\n");
}


void SunWait::print_list (const int days, const int year, const int month, const int day)
{
    time_t targetTimet = targetTime(year, month, day);
    if (debug) myDebugTime ("Target:", &targetTimet);

    unsigned long t2000 = daysSince2000(&targetTimet);

    Sun sun(longitude, latitude, twilightAngle);

    for (int dday = 0; dday < days; dday++)
    {
        SunArc  tmpTarget = sun.riset(t2000);
        print_times
        ( targetTimet
          , tmpTarget
          , offsetHour
          , cComma
        );
        t2000++;
    }

}


std::pair<time_t, time_t> SunWait::get_times
(
    const time_t   pMidnightTimet
    , SunArc  result
    , const double   pOffset
)
{
    double offsetDiurnalArc = result.diurnalArcWithOffset (pOffset);
    double riseHour         = result.getOffsetRiseHourUTC (pOffset);
    double setHour          = result.getOffsetSetHourUTC  (pOffset);

    std::pair<time_t, time_t> res;
    res.first = pMidnightTimet + riseHour * 3600;
    res.second = pMidnightTimet + setHour * 3600;

    if (offsetDiurnalArc >= 24.0)
    {
        res.first = POLAR_DAY;
        res.second = POLAR_DAY;
    }
    else if (offsetDiurnalArc <=  0.0)
    {
        res.first = POLAR_NIGHT;
        res.second = POLAR_NIGHT;
    }

    return res;
}



std::pair<std::vector<time_t>, std::vector<time_t>> SunWait::list (const int days, const int year, const int month, int day)
{
    std::vector<time_t> rises;
    std::vector<time_t> sets;
    Sun sun(longitude, latitude, twilightAngle);

    for (int d = 0; d < days; d++)
    {
        time_t targetTimet = targetTime(year, month, day);
        if (debug) myDebugTime ("Target:", &targetTimet);

        unsigned long t2000 = daysSince2000(&targetTimet);

        SunArc  tmpTarget = sun.riset(t2000);

        std::pair<time_t, time_t> sunriset = get_times
                                             ( targetTimet
                                               , tmpTarget
                                               , offsetHour
                                             );
        rises.push_back(sunriset.first);
        sets.push_back(sunriset.second);
        day++;

    }
    std::pair<std::vector<time_t>, std::vector<time_t>> result;
    result.first = rises;
    result.second = sets;
    return result;
}

int SunWait::poll (const time_t ttime)
{
    // Get current time (hours from UTC midnight of current day).
    // Difftime() returns "seconds", we want "hours".
    time_t nowTimet;
    if( ttime != NOT_SET )
    {
        nowTimet = ttime;
        if (debug) myDebugTime ("Target:", &nowTimet);
    }
    else
    {
        time(&nowTimet);
        if (debug) myDebugTime ("Now:", &nowTimet);
    }
    time_t midnightUTC = getMidnightUTC (&nowTimet);
    double nowHourUTC = difftime (nowTimet, midnightUTC) / (3600.0);

    // If the time is before sunrise or after sunset, I need to know that
    // we're not in the daylight of either the neighbouring days.
    Sun sun(longitude, latitude, twilightAngle);
    unsigned long now2000 = daysSince2000(&nowTimet);
    SunArc yesterday = sun.riset(now2000 - 1);
    SunArc today = sun.riset(now2000);
    SunArc tomorrow = sun.riset(now2000 + 1);

    yesterday.southHourUTC -= 24.0;
    tomorrow.southHourUTC += 24.0;

    // Get the time (0-24) of sunrise and set, of the three days
    double riseHourUTCYesterday = yesterday.getOffsetRiseHourUTC (offsetHour);
    double  setHourUTCYesterday =  yesterday.getOffsetSetHourUTC (offsetHour);
    double riseHourUTCToday     = today.getOffsetRiseHourUTC (offsetHour);
    double  setHourUTCToday     =  today.getOffsetSetHourUTC (offsetHour);
    double riseHourUTCTomorrow  = tomorrow.getOffsetRiseHourUTC (offsetHour);
    double  setHourUTCTomorrow  =  tomorrow.getOffsetSetHourUTC (offsetHour);

    // Figure out if we're between sunrise and sunset (with offset) of any of the three days
    return
        (  (nowHourUTC >= riseHourUTCYesterday && nowHourUTC <= setHourUTCYesterday)
           || (nowHourUTC >= riseHourUTCToday     && nowHourUTC <= setHourUTCToday    )
           || (nowHourUTC >= riseHourUTCTomorrow  && nowHourUTC <= setHourUTCTomorrow )
        ) ? EXIT_DAY : EXIT_NIGHT;
}




time_t SunWait::targetTime(int year, int mon, int mday)
{
    /*
    ** Get: Target Date
    */

    struct tm targetTm;

    // Populate targetTm :-
    // Get the target day (as "struct tm") for "now" - the default
    // I'll get the local-time day, as it'll make sense with the user, unless UTC was asked for
    //
    time_t nowTimet;
    time(&nowTimet);

    if (utc)
        myUtcTime   (&nowTimet, &targetTm); // User wants UTC
    else
        myLocalTime (&nowTimet, &targetTm); // User gets local timezone

    //
    // Parse "target" year, month and day [adjust target]
    //

    if (year != NOT_SET)
    {
        if (year < 0 || year > 99)
        {
            printf ("Error: \"Year\" must be between 0 and 99: %d\n", year);
            exit (EXIT_ERROR);
        }
        targetTm.tm_year = year + 100;
    }
    if (debug) printf ("Debug: Target  year set to: %d\n", targetTm.tm_year);

    if (mon != NOT_SET)
    {
        if (mon < 1 || mon > 12)
        {
            printf ("Error: \"Month\" must be between 1 and 12: %d\n", mon);
            exit (EXIT_ERROR);
        }
        targetTm.tm_mon = mon - 1; // We need month 0 to 11, not 1 to 12
    }
    if (debug) printf ("Debug: Target   mon set to: %d\n", targetTm.tm_mon);

    if (mday != NOT_SET)
    {
        if (mday < 1 || mday > 31)
        {
            printf ("Error: \"Day of month\" must be between 1 and 31: %d\n", mday);
            exit (EXIT_ERROR);
        }
        targetTm.tm_mday = mday;
    }
    if (debug) printf ("Debug: Target  mday set to: %d\n", targetTm.tm_mday);

    // Set target time to the start of the UTC day
    targetTm.tm_hour = 0;
    targetTm.tm_min  = 0;
    targetTm.tm_sec  = 0;

    // Fiddle with other "struct tm" fields to get things right
    targetTm.tm_wday  = 0;   // mktime will ignore this when calculating time_t as it contradicts days and months
    targetTm.tm_yday  = 0;   // mktime will ignore this when calculating time_t as it contradicts days and months
    targetTm.tm_isdst = -1;  // -1 means: mktime() must work it out. 0=DST not in effect. 1=DST in effect. (Daylight Savings)

    // Convert target "struct tm" to time_t.  It'll be set to midnight local time, on the target day.
    time_t targetTimet = mktime (&targetTm);

    // Shave off (add) UTC offset, so that time_t is converted from midnight local-time to midnight UTC on the target day
    targetTm.tm_sec += myRound (getUtcBiasHours(&targetTimet) * 60.0 * 60.0);

    // Adjustment below suggested by Phos Quartz on sourceforge.net to
    // address the case when invocation time and target time are not
    // in the same timezone (i.e. standard time vs daylight time)
    // see https://sourceforge.net/p/sunwait4windows/discussion/general/thread/98fc17dd5f/
    struct tm localTm;
    myLocalTime (&nowTimet, &localTm);
    int isdst = localTm.tm_isdst;
    targetTm.tm_isdst = isdst;

    // All done
    targetTimet = mktime (&targetTm);  // <<<<<< The important bit done <<< targetTimet is set to midnight UTC

    if (debug) myDebugTime ("Target", &targetTimet);
    return targetTimet;
}


/*
** Wait until sunrise or sunset occurs on the target day.
** That sounds simple, until you start to consider longitudes near the dateline.
** Midnight UTC can lands later and later within a local day for large longitudes.
** Latitudes heading towards the poles can either shrink the day length to nothing or the whole day, depending on the season.
** A user-specified offset messes around with daylength too.
** Exit immediately if its a polar day or midnight sun (including offset).
*/

int SunWait::wait (bool reportSunrise, bool reportSunset, unsigned long *waitptr)
{
    //
    // Calculate start/end of twilight for given twilight type/angle.
    // For latitudes near poles, the sun might not pass through specified twilight angle that day.
    // For big longitudes, it's quite likely the sun is up at midnight UTC: this means we have to calculate successive days.
    //
    // Get current time (hours from UTC midnight of current day).
    // Difftime() returns "seconds", we want "hours".
    int year = NOT_SET;
    int month = NOT_SET;
    int day = NOT_SET;
    // TBD -> if ttime given -> tm to get year month day?
    time_t targetTimet = targetTime(year, month, day);
    if (debug) myDebugTime ("Target:", &targetTimet);

    unsigned long t2000 = daysSince2000(&targetTimet);

    time_t nowTimet;
    time(&nowTimet);

    // If the time is before sunrise or after sunset, I need to know that
    // we're not in the daylight of either the neighbouring days.
    Sun sun(longitude, latitude, twilightAngle);

    SunArc yesterday = sun.riset(t2000 - 1);
    SunArc today = sun.riset(t2000);
    SunArc tomorrow = sun.riset(t2000 + 1);

    yesterday.southHourUTC -= 24;
    tomorrow.southHourUTC += 24;

    // Calculate duration (seconds) from "now" to "midnight UTC on the target day". [difftime (end, beginning)]
    long waitMidnightUTC = static_cast <long> (difftime (targetTimet, nowTimet));

    // Calculate duration to wait for each day's rise and set (seconds)
    // (targetTimet is set to midnight on the target day)
    long waitRiseYesterday = waitMidnightUTC + static_cast <long> ( 3600.0 * yesterday.getOffsetRiseHourUTC (offsetHour) );
    long waitSetYesterday  = waitMidnightUTC + static_cast <long> ( 3600.0 * yesterday.getOffsetSetHourUTC  (offsetHour) );
    long waitRiseToday     = waitMidnightUTC + static_cast <long> ( 3600.0 * today.getOffsetRiseHourUTC (offsetHour)     );
    long waitSetToday      = waitMidnightUTC + static_cast <long> ( 3600.0 * today.getOffsetSetHourUTC  (offsetHour)     );
    long waitRiseTomorrow  = waitMidnightUTC + static_cast <long> ( 3600.0 * tomorrow.getOffsetRiseHourUTC (offsetHour)  );
    long waitSetTomorrow   = waitMidnightUTC + static_cast <long> ( 3600.0 * tomorrow.getOffsetSetHourUTC  (offsetHour)  );

    // Determine next sunrise and sunset
    // (we may be in DAY, so the next event is sunset - followed by sunrise)

    long waitRiseSeconds = 0;
    long waitSetSeconds = 0;

    if      (waitRiseYesterday > 0)
    {
        waitRiseSeconds = waitRiseYesterday;
        waitSetSeconds = waitSetYesterday;
    }
    else if (waitSetYesterday  > 0)
    {
        waitRiseSeconds = waitRiseToday;
        waitSetSeconds = waitSetYesterday;
    }
    else if (waitRiseToday     > 0)
    {
        waitRiseSeconds = waitRiseToday;
        waitSetSeconds = waitSetToday;
    }
    else if (waitSetToday      > 0)
    {
        waitRiseSeconds = waitRiseTomorrow;
        waitSetSeconds = waitSetToday;
    }
    else if (waitRiseTomorrow  > 0)
    {
        waitRiseSeconds = waitRiseTomorrow;
        waitSetSeconds = waitSetTomorrow;
    }
    else if (waitSetTomorrow   > 0)
    {
        waitRiseSeconds = 0;
        waitSetSeconds = waitSetTomorrow;
    }

    // Is it currently DAY or NIGHT?

    OnOff isDay = ONOFF_OFF;

    if      (waitRiseYesterday > 0)
    {
        isDay = ONOFF_OFF;
    }
    else if (waitSetYesterday  > 0)
    {
        isDay = ONOFF_ON;
    }
    else if (waitRiseToday     > 0)
    {
        isDay = ONOFF_OFF;
    }
    else if (waitSetToday      > 0)
    {
        isDay = ONOFF_ON;
    }
    else if (waitRiseTomorrow  > 0)
    {
        isDay = ONOFF_OFF;
    }
    else if (waitSetTomorrow   > 0)
    {
        isDay = ONOFF_ON;
    }

    // Determine if the day is "normal" (where the rises and sets) or "polar" ("midnight sun" or "polar night")

    bool exitPolar = false;

    if      (waitSetYesterday > 0)
    {
        double diurnalArc = yesterday.diurnalArcWithOffset (offsetHour);
        exitPolar = diurnalArc <= 0.0 || diurnalArc >= 24.0;
    }
    else if (waitSetToday     > 0)
    {
        double diurnalArc = today.diurnalArcWithOffset (offsetHour);
        exitPolar = diurnalArc <= 0.0 || diurnalArc >= 24.0;
    }
    else
    {
        double diurnalArc = tomorrow.diurnalArcWithOffset (offsetHour);
        exitPolar = diurnalArc <= 0.0 || diurnalArc >= 24.0;
    }

    if (exitPolar)
    {
        if (debug) printf ("Debug: Polar region or large offset: No sunrise today, there's nothing to wait for!\n");
        return EXIT_ERROR;
    }

    // Get next rise or set time UNLESS the opposite event happens first (unless less than 6 hours to required event)
    // IF both rise and set requested THEN wait for whichever is next

    long waitSeconds = 0;

    if (reportSunrise  && !reportSunset )
    {
        if (isDay == ONOFF_OFF || waitRiseSeconds < 6 * 60 * 60) waitSeconds = waitRiseSeconds;
    }
    else if ( !reportSunrise && reportSunset )
    {
        if (isDay == ONOFF_ON || waitSetSeconds < 6 * 60 * 60)  waitSeconds = waitSetSeconds;
    }
    else
    {
        waitSeconds = waitRiseSeconds < waitSetSeconds ? waitRiseSeconds : waitSetSeconds;
    }

    // Don't wait if event has passed (or next going to occur soon [6hrs])
    if (waitSeconds <= 0)
    {
        if (debug ) printf ("Debug: Event already passed today, can't wait for that!\n");
        return EXIT_ERROR;
    }

    //
    // In debug mode, we don't want to wait for sunrise or sunset. Wait a minute instead.
    //

    if (debug)
    {
        printf("Debug: Wait reduced from %li to 10 seconds.\n", waitSeconds);
        waitSeconds = 10;
    }
    // else if (pRun->functionPoll == ONOFF_ON) waitSeconds += 60; // Make more sure that a subsequent POLL works properly (wink ;-)

    //
    // Sleep (wait) until the event is expected
    //
    if(waitptr == nullptr)
    {
        std::this_thread::sleep_for(std::chrono::seconds{waitSeconds});
        /*
          // Windows code: Start
          #if defined _WIN32 || defined _WIN64
            waitSeconds *= 1000; // Convert hours to milliseconds for Windows
            Sleep ((DWORD) waitSeconds); // Windows-only . waitSec is tested positive or zero
          #endif
          // Windows code: End

          // Linux code: Start
          #if defined __linux__ || defined __APPLE__
            sleep (waitSeconds);    // Linux-only (seconds OK)
          #endif
          // Linux code: End */
    }
    else
    {
        *waitptr = waitSeconds;
    }
    return EXIT_OK;
}

bool SunWait::setCoordinates(const char *lat, const char *lon)
{
    bool parse = isBearing(lat);
    parse = parse && isBearing(lon);
    if(!parse) printf ("Error: Couldn't parse the coordinates.");
    return parse;
}

bool SunWait::isBearing (const char *pArg)
{
    double bearing = 0;
    int    exponent = 0;
    bool   negativeBearing = false;
    bool   exponentSet = false;
    char   compass = 'X';

    for (int i = 0; ; i++)
    {
        switch (pArg[i])
        {
            case '0':
                bearing = (bearing * 10) + 0;
                exponentSet ? exponent++ : true;
                break;
            case '1':
                bearing = (bearing * 10) + 1;
                exponentSet ? exponent++ : true;
                break;
            case '2':
                bearing = (bearing * 10) + 2;
                exponentSet ? exponent++ : true;
                break;
            case '3':
                bearing = (bearing * 10) + 3;
                exponentSet ? exponent++ : true;
                break;
            case '4':
                bearing = (bearing * 10) + 4;
                exponentSet ? exponent++ : true;
                break;
            case '5':
                bearing = (bearing * 10) + 5;
                exponentSet ? exponent++ : true;
                break;
            case '6':
                bearing = (bearing * 10) + 6;
                exponentSet ? exponent++ : true;
                break;
            case '7':
                bearing = (bearing * 10) + 7;
                exponentSet ? exponent++ : true;
                break;
            case '8':
                bearing = (bearing * 10) + 8;
                exponentSet ? exponent++ : true;
                break;
            case '9':
                bearing = (bearing * 10) + 9;
                exponentSet ? exponent++ : true;
                break;
            case '.':
            case ',':
                exponentSet = true;
                exponent = 0; // May be: N36.513679 (not right, but it'll do)
                break;
            case '+':
                if (i > 0) return false; // Sign only at start
                negativeBearing = false;
                break;
            case '-':
                if (i > 0) return false; // Sign only at start
                negativeBearing = true;
                break;
            case 'n':
            case 'N':
                compass = 'N';
                exponentSet = true;
                break; // Can support 36N513679 (not right, but it'll do)
            case 'e':
            case 'E':
                compass = 'E';
                exponentSet = true;
                break;
            case 's':
            case 'S':
                compass = 'S';
                exponentSet = true;
                break;
            case 'w':
            case 'W':
                compass = 'W';
                exponentSet = true;
                break;
            case ' ':  /* Exit */
            case '\0': /* Exit */
                /* Fail, if the compass has not been set */
                if (compass == 'X') return false;

                /* Place decimal point in bearing */
                if (exponentSet && exponent > 0) bearing = bearing / pow (10, (double) exponent);

                /* Fix-up bearing so that it is in range zero to just under 360 */
                bearing = revolution (bearing);
                bearing = negativeBearing ? 360 - bearing : bearing;

                /* Fix-up bearing to Northings or Eastings only */
                if (compass == 'S')
                {
                    bearing = 360 - bearing;
                    compass = 'N';
                }
                else if (compass == 'W')
                {
                    bearing = 360 - bearing;
                    compass = 'E';
                }

                /* It's almost done, assign bearing to appropriate global */
                if (compass == 'N') latitude  = fixLatitude  (bearing);
                else if (compass == 'E') longitude = fixLongitude (bearing);
                else return false;
                return true;  /* All done */
                break;
            default:
                return false;
        }
    }
    return false; /* Shouldn't get to here */
}
