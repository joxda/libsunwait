

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


typedef enum
{
    ONOFF_ON
    , ONOFF_OFF
} OnOff;





struct SunArc
{
    double diurnalArc;
    double southHourUTC;
    SunArc(double dA, double sH) : diurnalArc{dA}, southHourUTC{sH} {};
    double diurnalArcWithOffset (const double pOffset);
    double getOffsetRiseHourUTC (const double pOffsetHour);
    double getOffsetSetHourUTC (const double pOffsetHour);
};
