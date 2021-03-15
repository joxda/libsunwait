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

#include <time.h>
#include <vector>
#include <utility>
#include <cstdio>

#ifndef LIBSUNWAIT_HPP
#define LIBSUNWAIT_HPP

/**
 * @defgroup ReturnCodes Definitions for return codes
 * 
 * @brief Defines for return codes
 * 
 * long description
 */
/**
 * @addtogroup ReturnCodes
 * @{
 */
/// Normal exit status
#define EXIT_OK    0
/// Error
#define EXIT_ERROR 1
/// Poll resulted in "DAY"
#define EXIT_DAY   2
/// Poll resulted in "NIGHT"
#define EXIT_NIGHT 3
/**@}*/

#define DEFAULT_LATITUDE  65
#define DEFAULT_LONGITUDE 25.5 /* */

/**
 * @defgroup TwilightAngles Definitions for pre-set angles
 * 
 * @brief Defines for twilight angles
 * 
 * long description
 */
/* Sunrise/set is considered to occur when the Sun's upper limb (upper edge) is 50 arc minutes below the horizon */
/* (this accounts for the refraction of the Earth's atmosphere). */
/* Civil twilight starts/ends when the Sun's center is 6 degrees below the horizon. */
/* Nautical twilight starts/ends when the Sun's center is 12 degrees below the horizon. */
/* Astronomical twilight starts/ends when the Sun's center is 18 degrees below the horizon. */


/**
 * @addtogroup TwilightAngles
 * @{
 */
/// Sun rise and set
#define TWILIGHT_ANGLE_DAYLIGHT     -50.0/60.0 
/// Civil twilight
#define TWILIGHT_ANGLE_CIVIL        -6.0       
/// Nautical twilight
#define TWILIGHT_ANGLE_NAUTICAL     -12.0      
/// Astronomical twilight
#define TWILIGHT_ANGLE_ASTRONOMICAL -18.0      
/**@}*/

/**
 * @defgroup PolarDayNight Definitions for polar day and night
 * 
 * @brief Defines for polar day and night in the list function
 * 
 * long description
 */
/**
 * @addtogroup PolarDayNight
 * @{
 */
/// Polar day
#define POLAR_DAY 0
/// Polar night
#define POLAR_NIGHT 1
/**@}*/

#define NOT_SET 9999999
#define NO_OFFSET 0.0


#define DAYS_TO_2000  365*30+7                                   // Number of days from 'C' time epoch (1/1/1970 to 1/1/2000) [including leap days]

struct SunArc;

/**
 * @brief Main class
 * 
 * Calculate sunrise and sunset times for the current or targetted day.
 * The times can be adjusted either for twilight or fixed durations.
 * 
 * The geographical coordinates should be set (e.g. via the constructor)
 * and optionally the twilight angle and fixed offset can be specified.
 * 
 * There are functions for waiting for sunrise or sunset (wait)
 * listing such event times (list and print_list)
 * reporting the day length and twilight times (report)
 * and reporting whether it is day or night (poll)
 * 
 */
class SunWait
{
    public:
    /// Adjust sunrise or sunset by this amount, in hours, towards midday. This allows to report time / wait until / etc that correspond to a given time before/after any event (such as 15 before sun rise).
        double        offsetHour = NO_OFFSET;                   
    
    /// Twilight angle requested by user in degrees. Negative values are below the horizon. The default value is used for calculating sun rise and set taking into account standard refraction and the upper limb instead of the center of the sun.
        double        twilightAngle = TWILIGHT_ANGLE_DAYLIGHT;  

    /// Printed output is in GMT/UTC (true) or localtime (false).
        bool          utc;                                      
    
    /// When true, debug information is printed to the standard output.
        bool          debug;                                    

    /**
     * @brief Construct a new SunWait object with default geographical coordinates and twilight angle
     * 
     */
        SunWait() = default;
    
    /**
     * @brief Construct a new SunWait object with the requested geographical coordinates and the default twilight angle
     * 
     * @param lat Geographical latitude in decimal degrees (N positive, S negative)
     * @param lon Geographical longitude in decimal degrees (E positive, W negative)
     */
        SunWait(double lat, double lon)
        {
            latitude = fixLatitude(lat);
            longitude = fixLongitude(lon);
        }; 
    
    /**
     * @brief Construct a new SunWait object with the requested geographical coordinates and the default twilight angle
     * 
     * With thise versions a char* is used and the library attempts to parse the information.
     * 
     * @param lat Geographical latitude 
     * @param lon Geographical longitude 
     */
        SunWait(const char *lat, const char *lon)
        {
            bool parse = isBearing(lat);
            parse = parse && isBearing(lon);
            if(!parse) printf ("Error: Couldnt parse the coordinates.");
        };
    
    /**
     * @brief Construct a new SunWait object with the requested geographical coordinates and twilight angle
     * 
     * @param lat Geographical latitude in decimal degrees (N positive, S negative)
     * @param lon Geographical longitude in decimal degrees (E positive, W negative)
     * @param angle Twilight angle (in decimal degrees, negative for the Sun below horizon)
     */
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

    /**
     * @brief Construct a new SunWait object with the requested geographical coordinates and twilight angle
     * 
     * With thise versions a char* is used and the library attempts to parse the information.
     * 
     * @param lat Geographical latitude 
     * @param lon Geographical longitude
     * @param angle Twilight angle (in decimal degrees, negative for the Sun below horizon)
     */
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

    /**
     * @brief Update the geographical coordinates
     * 
     * @param lat Geographical latitude in decimal degrees (N positive, S negative)
     * @param lon Geographical longitude in decimal degrees (E positive, S negative)
     */
        void setCoordinates(double lat, double lon);
    

    /**
     * @brief Update the geographical coordinates
     * 
     * With thise versions a char* is used and the library attempts to parse the information.
     * 
     * @param lat Geographical latitude 
     * @param lon Geographical longitude 
     * @return Return true when successful.
     */
        bool setCoordinates(const char *lat, const char *lon);
    
    /**
     * @brief Poll whether it is day or night for a given time.
     * 
     * @param ttime Time for the request (optional). By default the current time is used.
     * @return Returns one if the return codes EXIT_DAY or EXIT_NIGHT
     */
        int poll (const time_t ttime = NOT_SET);
    
    /**
     * @brief Sleep until specified event occurs (sun rise or sun set or either)
     * 
     * With a twilight angle and offset set, the corresponding event(s) will be queried.
     * 
     * @param reportSunrise When true sun rises are considered
     * @param reportSunset  When true sun sets are considered
     * @param waitptr When provided the wait time in seconds is written to the variable which is pointed to instead of waiting
     * @return Returns EXIT_OK or EXIT_ERROR
     */
        int wait (bool reportSunrise = true, bool reportSunset = true, unsigned long *waitptr = nullptr);
    /**
     * @brief This replicates the generate report of the original sunwait command line executable
     * 
     * A report contatining the day length and twilight timings for a given date (today by default) are printed to the standard output.
     * Optionally the starting date can be given (year, month, day).
     * 
     * @param year Specify the year
     * @param mon Specify the month
     * @param mday Specify the day
     */
        void generate_report (int year = NOT_SET, int mon = NOT_SET, int mday = NOT_SET);
    
    /**
     * @brief This replicates the list command of the original sunwait executable
     * 
     * Prints the time (GMT or local) the event occurs for a given date (and a number of following days) to the standard output.
     * Optionally the starting date can be given (year, month, day).
     * 
     * @param days Number of days to report
     * @param year Specify the year
     * @param mon Specify the month
     * @param mday Specify the day
     */
        void print_list (const int days, int year = NOT_SET, int mon = NOT_SET, int mday = NOT_SET);
    
    /**
     * @brief This will return a pair of vectors with the times (time_t) of requested events.
     * 
     * This is similar to the print_list function, but the returned times can easily be processed by the main program.
     * If for any given day polar day or night apply, both event times are set to POLAR_DAY or POLAR_NIGHT, respectively.
     * Optionally the starting date can be given (year, month, day).
     * 
     * @param days  Number of days to report
    * @param year Specify the year
     * @param mon Specify the month
     * @param mday Specify the day
     * @return std::pair<std::vector<time_t>, std::vector<time_t>> 
     */
        std::pair<std::vector<time_t>, std::vector<time_t>> list (const int days, const int year, const int month, int day);

    private:

        double        latitude = DEFAULT_LATITUDE;              // Degrees N - Global position
        double        longitude = DEFAULT_LONGITUDE;            // Degrees E - Global position

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
