#pragma implementation "Map.h"
#include <Map.h>
#pragma implementation "VHMap.h"
#include <VHMap.h>
#include <Map.cc>
#include <VHMap.cc>

class rcvr;
class Xrcvr;

static inline unsigned hash(const long &l) { return (unsigned)l; }

template class VHMap<long,rcvr *>;
template class Map<long,rcvr *>;
template class VHMap<long,Xrcvr *>;

