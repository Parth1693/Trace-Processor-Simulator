#pragma implementation "cache.h"
/*--------------------------------------------------------------------------*\
 | dcache.cc
 | Timothy Heil
 | 1/23/96
 |
 | Provides D-Cache interface for simulator.
\*--------------------------------------------------------------------------*/

#include <stdlib.h>
#include <assert.h>

#include "ss.h"
#include "histogram.h"
#include "cache.h"
#include "dcache.h"

CacheClass::CacheClass(int sets, int assoc, int lineSize,
		       int hitLatency, int missLatency, 
		       int numMHSRs, int missSrvPorts,  int missSrvLat,
		       int histLen)
     : array(sets, assoc)  // Allocate cache array.
/*------------------------------------------------------------------------*\
 | Constructor.  Allocates data structures and initializes D-cache state.
 |  
 |  sets           The number of sets in the cache.
 |  assoc          The number of ways per set (associativity).
 |  hitLatency     The number of cycles required to process a hit
 |                  to the D-Cache.  Should be at least 1.
 |  missLatency    The number of cycles required to process a miss
 |                  to the D-Cache.  This is also used as the number
 |                  of cycles to flush a dirty line to memory.
 |  numMHSRs       The number of miss handleing status registers.
 |                  (Maximum number of outstanding cache misses. )
 |  missSrvPorts   The number of misses that can be serviced in a single
 |                  cycle (number of ports to the backing store.)
 |  missSrvLat     The number of cycles before a miss service port can be
 |                  reused (port pipeline latency).
\*------------------------------------------------------------------------*/
{
  int i;

  /* Set cache parameters. */
  this->lineSize = lineSize;
  lastCycle      = 0;
  hitLat         = hitLatency;
  missLat        = missLatency;
 
  /* Allocate MHSRs */
  mhsr = new MHSRClass[numMHSRs];
  assert(mhsr);
  maxOutMiss = numMHSRs;
  for (i=0;i<numMHSRs;i++) {
    mhsr[i].resolved = -1;
  }

  /* Allocate miss service ports. */
  missPortAvail = new CycleType[missSrvPorts];
  assert(missPortAvail);
  for (i=0;i<missSrvPorts;i++) {
    missPortAvail[i]=0;
  }

  missPorts = missSrvPorts;
  missSrv   = missSrvLat;

#ifdef STATS2
  cacheAccess[0] = 0;
  cacheAccess[1] = 0;
  cacheMisses[0] = 0;
  cacheMisses[1] = 0;

  // Allocate histogram.
  if (histLen > 0) {
    assert(accessLatency = new HistogramClass[2](histLen));
    
  }
  else {
    accessLatency = NULL;
  }
#endif

}

void CacheClass::flush()
{
    array.flush();
}

CacheClass::~CacheClass()
/*------------------------------------------------------------------------*\
 | Destructor.  Frees memory used by D-cache.
\*------------------------------------------------------------------------*/
{
  delete [] mhsr;
  delete [] missPortAvail;

#ifdef STATS2
  if (accessLatency) {
    delete [] accessLatency;
  }
#endif
}

CycleType CacheClass::Access(unsigned int Tid /* ER 11/16/02 */,
			     CycleType curCycle, AddressType addr, 
			     bool isStore, bool *isHit, 
			     bool probe, bool commit)
/*------------------------------------------------------------------------*\
 | Access the data cache.  Determines how many cycles access will take.
 | 
 |  curCycle          The current simulation cycle.  This must always 
 |                     increase or be the same, from call to call.
 |  addr              The address of the word being accessed.
 |  isStore           Indicates whether the access is a store (true) or
 |                     load (false).
 |
 | Returns the cycle when the access will complete.  Returns -1 if the
 |  access can not be handled, due to limited miss handleing status 
 |  registers.
\*------------------------------------------------------------------------*/
{
  bool hit;
  AddressType lineAddr;
  AddressType oldAddr;
  CacheLineClass *line;
  CacheLineClass *newLine;
  int busyMHSR;
  int newMHSR;
  int newPort;
  CycleType portAvail;
  CycleType lineInArray;

  assert (curCycle >= lastCycle);
  lastCycle = curCycle;

  // ER 11/16/02
  //lineAddr = addr >> lineSize;
  assert((Tid < 4) && (lineSize >= 2));
  lineAddr = ((addr >> lineSize) | (Tid << 30));

  line = array.lookup(lineAddr, NULL, &hit, &oldAddr, false);

  if (probe) {
    (*isHit) = hit;
    return(curCycle);
  }

#ifdef STATS2
  cacheAccess[isStore ? 1 : 0]++;
#endif

  if (hit) {
    // Line has been allocated in cache.

    // Check if line is being dirtied.
    if ((isStore) && (!line->dirty)) {
      // Line must be changed to dirty.
      line->dirty = true;
    }

    // Check if line is currently being loaded (is busy).
    busyMHSR = line->mhsr;
    if (busyMHSR != -1) {

      // Line might be busy.
      // Check MHSR.
      if (mhsr[busyMHSR].resolved <= curCycle) {
	// Cache line was already loaded.
	lineInArray = curCycle;
	line->mhsr = -1;
	mhsr[busyMHSR].resolved = -1;
      }
      else {
	// Cache line is being loaded.
	lineInArray = mhsr[busyMHSR].resolved;
#ifdef STATS2
	cacheMisses[isStore ? 1 : 0]++;
#endif
	// Must wait at least one cache check time for busy lines.
	if ((lineInArray - curCycle) < hitLat) {
	  lineInArray = curCycle + hitLat;
	}
      }      
    }
    else {
      // Line is already available in cache.
      lineInArray = curCycle;
    }
  }
  else {
    // Line has not been allocated in cache.

#ifdef STATS2
    cacheMisses[isStore ? 1 : 0]++;
#endif

    // Allocate MHSR to handle cache miss.
    newMHSR = FindFreeMHSR(curCycle);
    //if (newMHSR == -1) return(-1);
    if (newMHSR == -1) assert(0);

    // Find the miss port to use for handling the miss.
    newPort = FindNextPort(curCycle, &portAvail);

    // Allocate a new cache line structure.
    if (commit) {
      // Allocate a new cache line structure.
      newLine = new CacheLineClass;
	assert(newLine);
      newLine -> mhsr = newMHSR;
      newLine -> dirty = isStore;

      // Replace the old line in the cache.
      line = array.lookup(lineAddr, newLine, &hit, &oldAddr, true);
    }

    // Compute the time to load the new line from the next memory level.
    lineInArray = portAvail;
   
    // Must wait one cache array access cycle, before beginning line load. 
    if ((lineInArray-curCycle) < hitLat) {
      lineInArray = curCycle + hitLat;
    }

    // See if line being replaced is itself still being loaded.
    if ((commit) && (line !=NULL)) {
      busyMHSR = line->mhsr;
      if (busyMHSR != -1) {
	// Line being replaced is being loaded.  Must wait until this
	//  line has been loaded to replace it.
	//  NOTE: There is a slight simulation approximation made here.
	//        The miss port is being tied up for the entire time,
	//        but will not actually be used until later.
	if (mhsr[busyMHSR].resolved > lineInArray) {
	  lineInArray = mhsr[busyMHSR].resolved;
	}
      }

      // See if line is dirty.  Line must be written back, if dirty.
      if (line->dirty) {
	lineInArray = lineInArray + missLat;
      }
    }

    // Allocate miss port.
    missPortAvail[newPort] = lineInArray + missSrv;

    // Add miss latency to access time.
    lineInArray = lineInArray + missLat;

    // Free old cache line, allocate miss port and MHSR.
    // NOTE: Slight simulation approximation error here.
    //       MHSR is being allocated this cycle, but in reality, can not
    //       be allocated until hitLat cycles later, when miss is
    //       known.
    if ((commit) && (line)) delete line;
    mhsr[newMHSR].resolved = lineInArray;
    mhsr[newMHSR].lineAddress = lineAddr;
  }

#ifdef STATS2
  accessLatency[isStore ? 1:0].Increment(lineInArray + hitLat - curCycle);
#endif

  if (isHit!=NULL) {
    (*isHit) = (lineInArray == curCycle);
  }

  return(lineInArray + hitLat);
}
 
int CacheClass::FindFreeMHSR(CycleType curCycle)
{
  int i;
  CacheLineClass *line;
  bool hit;
  unsigned int oldAddr;

  for (i=0;i<maxOutMiss;i++) {
    if (mhsr[i].resolved ==-1) return(i);
    if (mhsr[i].resolved < curCycle) {
      // MHSR is finished.  Free it.
      line = array.lookup(mhsr[i].lineAddress, NULL, &hit, &oldAddr, false);
      if (hit) {
	if (line->mhsr == i) {
	  line->mhsr = -1;
	}
      }
      mhsr[i].resolved = -1;
      return(i);
    }
  }

  // No MHSRs available.
  return(-1);
}

int CacheClass::FindNextPort(CycleType curCycle, CycleType *portAvail)
{
  int i;
  int soonestPort;
  CycleType soonestCycle;

  soonestPort = 0;
  soonestCycle = missPortAvail[0];

  for (i=1; i < missPorts; i++) {
    if (soonestCycle > missPortAvail[i]) {
      soonestCycle = missPortAvail[i];
      soonestPort = i;
    }
  }

  *portAvail = soonestCycle;
  return(soonestPort);
}


bool CacheClass::Probe(CycleType curCycle,
		       AddressType addr1,
		       unsigned int length) {
	AddressType addr2;
	AddressType lineAddr1, lineAddr2;
	bool hit1, hit2;

	assert(length > 0);
	addr2 = (addr1 + SS_INST_SIZE*(length - 1));

	lineAddr1 = addr1 >> lineSize;
	lineAddr2 = addr2 >> lineSize;

	Access(curCycle, addr1, false, &hit1);
	if (lineAddr2 != lineAddr1)
	   Access(curCycle, addr2, false, &hit2);
	else
	   hit2 = hit1;

	return(hit1 && hit2);
}


// ER 06/19/01
void
CacheClass::set_lat(unsigned int lat) {
	missLat = lat;
}
