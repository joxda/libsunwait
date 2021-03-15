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

#pragma once

#include "sunarc.hpp"

/* Some conversion factors between radians and degrees */
#define RADIAN_TO_DEGREE   ( 180.0 / PI )
#define DEGREE_TO_RADIAN   ( PI / 180.0 )

/* The trigonometric functions in degrees */
#define sind(x)     (sin((x)*DEGREE_TO_RADIAN))
#define cosd(x)     (cos((x)*DEGREE_TO_RADIAN))
#define tand(x)     (tan((x)*DEGREE_TO_RADIAN))
#define atand(x)    (RADIAN_TO_DEGREE*atan(x))
#define asind(x)    (RADIAN_TO_DEGREE*asin(x))
#define acosd(x)    (RADIAN_TO_DEGREE*acos(x))
#define atan2d(y,x) (RADIAN_TO_DEGREE*atan2(y,x))

#define PI 3.1415926535897932384

// TBD : SUN POSITION?

class Sun
{
    public:
        Sun(double lon, double lat, double angle) : longitude{lon}, latitude{lat}, twilightAngle{angle} {};
        SunArc riset (unsigned long daysSince2000);
        double longitude;
        double latitude;
        bool debug = false;
        double twilightAngle;

    private:
        void sunpos (const double d, double *lon, double *r);

        double rev180 (const double x);
        double fix24 (const double x);
        double GMST0 (const double d);
        void sun_RA_dec (double d, double *RA, double *dec, double *r);
};