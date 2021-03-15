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

//
// sunwait - sunwait.h
//
// 08-12-2014  IFC  0.6  Add timezone handling for output, if required
// 02-05-2015  IFC  0.7  Fix timezone and DST problems
// 06-05-2015  IFC  0.8  Fix polar calculations
//

//#include <time.h>

#include <vector>
#include <utility>

#ifndef LIBSUNWAIT_HPP
#define LIBSUNWAIT_HPP

#define EXIT_OK    0
#define EXIT_ERROR 1
#define EXIT_DAY   2
#define EXIT_NIGHT 3

#define DEFAULT_LATITUDE  65
#define DEFAULT_LONGITUDE 25.5 /* */

/* Sunrise/set is considered to occur when the Sun's upper limb (upper edge) is 50 arc minutes below the horizon */
/* (this accounts for the refraction of the Earth's atmosphere). */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below the horizon. */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees below the horizon. */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees below the horizon. */
#define TWILIGHT_ANGLE_DAYLIGHT     -50.0/60.0
#define TWILIGHT_ANGLE_CIVIL        -6.0
#define TWILIGHT_ANGLE_NAUTICAL     -12.0
#define TWILIGHT_ANGLE_ASTRONOMICAL -18.0

#define POLAR_DAY 0
#define POLAR_NIGHT 1

#define boolean bool
#define NOT_SET 9999999
#define NO_OFFSET 0.0


#define DAYS_TO_2000  365*30+7      // Number of days from 'C' time epoch (1/1/1970 to 1/1/2000) [including leap days]

struct SunArc;

class SunWait
{
    public:
        double        offsetHour = NO_OFFSET;          // Unit: Hours. Adjust sunrise or sunset by this amount, towards midday.
        double        twilightAngle =
            TWILIGHT_ANGLE_DAYLIGHT;       // Degrees. -ve = below horizon. Twilight angle requested by user.

        bool          utc;                 // Printed output is in GMT/UTC (on) or localtime (off)
        bool          debug;               // Is debug output required

        SunWait() = default;
        SunWait(double lat, double lon)
        {
            latitude = fixLatitude(lat);
            longitude = fixLongitude(lon);
        }; // CHECK?
        SunWait(const char *lat, const char *lon)
        {
            bool parse = isBearing(lat);
            parse = parse && isBearing(lon);
            if(!parse) printf ("Error: Couldnt parse the coordinates.");
        };
        SunWait(double lat, double lon, double angle) :  twilightAngle{angle}
        {
            if (twilightAngle <= -90 || twilightAngle >= 90)
            {
                printf ("Error: Twilight angle must be between -90 and +90 (-ve = below horizon), your setting: %f\n", twilightAngle);
                twilightAngle = TWILIGHT_ANGLE_DAYLIGHT;
            }
            latitude = fixLatitude(lat);
            longitude = fixLongitude(lon);
        };

        SunWait(const char *lat, const char *lon, double angle) :  twilightAngle{angle}
        {
            if (twilightAngle <= -90 || twilightAngle >= 90)
            {
                printf ("Error: Twilight angle must be between -90 and +90 (-ve = below horizon), your setting: %f\n", twilightAngle);
                twilightAngle = TWILIGHT_ANGLE_DAYLIGHT;
            }
            bool parse = isBearing(lat);
            parse = parse && isBearing(lon);
            if(!parse) printf ("Error: Couldnt parse the coordinates.");
        };

        void setCoordinates(double lon, double lat);
        bool setCoordinates(const char *lon, const char *lat);
        int poll (const time_t ttime = NOT_SET);
        int wait (bool reportSunrise = true, bool reportSunset = true, long *waitptr = nullptr);
        void generate_report (int yearInt = NOT_SET, int monInt = NOT_SET, int mdayInt = NOT_SET);
        void print_list (const int days, int yearInt = NOT_SET, int monInt = NOT_SET, int mdayInt = NOT_SET);
        std::pair<std::vector<time_t>, std::vector<time_t>> list (const int days, const int year, const int month, int day);

    private:

        double        latitude = DEFAULT_LATITUDE;            // Degrees N - Global position
        double        longitude = DEFAULT_LONGITUDE;           // Degrees E - Global position

        double fixLatitude(const double x);
        double fixLongitude(const double x);
        time_t targetTime(int yearInt = NOT_SET, int monInt = NOT_SET, int mdayInt = NOT_SET);

        void print_times( const time_t   pMidnightTimet, SunArc result, const double   pOffset, const char   *pSeparator);
        void print_a_sun_time( const time_t *pMidnightTimet, const double  pEventHour, const double  pOffsetDiurnalArc);
        void print_a_time(   const time_t *pMidnightTimet, const double  pEventHour);

        std::pair<time_t, time_t> get_times(   const time_t   pMidnightTimet, SunArc  result, const double   pOffset) ;
        bool isBearing (const char *pArg);
};

#endif
