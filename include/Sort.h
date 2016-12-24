/*
    This file is part of the BTAS client library.

    The BTAS client library is free software: you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public License as
    published by the Free Software Foundation, either version 3 of the License,
    or (at your option) any later version.

    BTAS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with BTAS.  If not, see <http://www.gnu.org/licenses/>.
*/
#pragma interface

/** Simple binary sort.  While STL probably has this by now, it didn't
 when I needed it.
 */
template <class T> void sort(T *a,int n) {
  for (int gap = n/2; gap > 0; gap /= 2) {
    for (int i = gap; i < n; ++i) {
      for (int j = i - gap; j >= 0; j -= gap) {
	if (a[j] <= a[j+gap]) break;
	T temp = a[j];
	a[j] = a[j+gap];
	a[j+gap] = temp;
      }
    }
  }
}
