/*--------------------------------------------------------------------------*\
 | dcache.h
 | Timothy Heil
 | 1/23/96
 |
 | Provides D-Cache interface for simulator.
 | Currently, D-Cache keeps only time-stamps and does not actually cache
 |  the data.  
 |
 | Adjustable Cache parameters:
 |  Cache Line Size
 |  Cache Associativity
 |  Cache Size
 |  Cache Access Latency
 |  Cache Miss Latency
 |  Number of outstanding misses
 |  Number of ports to backing store
 |  Backing store port reuse latency
 |
 | Fixed cache parameters:
 |  Replacement policy (LRU)
 |  Write policy (Write Back)
 |  Number of cache ports (unlimited)
\*--------------------------------------------------------------------------*/

typedef unsigned int AddressType;
typedef SS_TIME_TYPE CycleType;

/*--------------------------------------------------------------------------*\
 | Miss Handleing Status Register provides multiple outstanding reads and
 |  writes.  Collapses requests for the same line.
\*--------------------------------------------------------------------------*/
class MHSRClass {
 public:
  AddressType lineAddress;    /* Line being loaded by MHSR.        */
  CycleType   resolved;       /* When miss will be completed.      */
};

/*--------------------------------------------------------------------------*\
 | State maintained by a D-Cache line.
\*--------------------------------------------------------------------------*/
class CacheLineClass {
 public:
  int mhsr;   /* Index of MHSR that is loading this line.        */
              /* -1 indicates that the line is not being loaded. */
  bool dirty; /* Indicates the line is dirty.                    */
};

typedef cache<CacheLineClass> CacheArray;

class CacheClass {
 public:
  CacheClass(int sets, int assoc, int lineSize,
	     int hitLatency, int missLatency, 
	     int numMHSRs, int missSrvPorts, int missSrvLat,
	     int histLen = 50);
  /*------------------------------------------------------------------------*\
   | Constructor.  Allocates data structures and initializes D-cache state.
   |  
   |  sets           The number of sets in the cache.
   |  assoc          The number of ways per set (associativity).
   |  lineSize       The log2 of the size of a line in bytes.
   |                  (A line is  (1<<lineSize) bytes long. 
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

  ~CacheClass();
  /*------------------------------------------------------------------------*\
   | Destructor.  Frees memory used by D-cache.
  \*------------------------------------------------------------------------*/

  void flush();
  void set_lat(unsigned int lat);	// ER 06/19/01

  CycleType Access(unsigned int Tid /* ER 11/16/02 */,
		   CycleType curCycle, AddressType addr, bool isStore, 
		   bool *hit=NULL, bool probe=false, bool commit=true);
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

  bool Probe(CycleType curCycle, AddressType addr1, unsigned int length);

  inline int Accesses(bool isStore) {return(cacheAccess[isStore ? 1 : 0]);};
  inline int Misses(bool isStore) {return(cacheMisses[isStore ? 1 : 0]);};
  inline int Hits(bool isStore)  
    {
      return(cacheAccess[isStore ? 1 : 0] - cacheMisses[isStore ? 1 : 0]);
    }

  HistogramClass *accessLatency;
 private:
  
  int FindFreeMHSR(CycleType curCycle);
  int FindNextPort(CycleType curCycle, CycleType *portAvail);

  CacheArray array;          /* The D-Cache array.                           */
  MHSRClass *mhsr;           /* The miss handling status registers.          */
  int       maxOutMiss;      /* The number of MHSRs.                         */
  int       lineSize;        /* D-Cache line size.  Must be a power of 2.    */
  CycleType lastCycle;       /* curCycle of last access.                     */
  int hitLat;                /* Cycles of delay for cache hits.              */
  int missLat;               /* Cycles of delay for cache misses, and write 
                              *  backs.                                      */
  CycleType *missPortAvail;  /* Cycle each miss port will be available.      
                              *  Currently these ports are used for write-
                              *  backs as well.                              */
  int missPorts;             /* Number of miss ports available.              */
  int missSrv;               /* Pipeline reuse latency for miss ports.       */
  int cacheAccess[2];        /* Number of cache accesses. 0=loads, 1=stores. */
  int cacheMisses[2];        /* Number of cache misses.   0=loads, 1=stores. */
};
