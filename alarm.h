/* $Log$
 * Revision 1.3  2013/03/18 20:09:43  stuart
 * Add GPL License
 *
    Alarm class monitors time to auto-flush buffers and signals a shutdown
    for fatal signals.
    Copyright (C) 1985-2013 Business Management Systems, Inc

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#pragma interface

class Alarm {
  int limit;
  static int ticks, fatal, interval;
  static void handler(int);
  static void handleFatal(int);
public:
  Alarm();
  bool check(class btserve *);		// should we shutdown? (msgrcv err)
  void enable(int t);			// make sure alarm is ticking
  ~Alarm();
};
