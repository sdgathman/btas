#pragma interface
/* binary sort */
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
