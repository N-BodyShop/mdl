#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED

#include <charm.h>

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>

#define SRV_STOP		0

#define AMPI

typedef struct cacheTag {
	int iKey;
	int nLock;
	int nLast;
	int iLink;
	} CTAG;

typedef struct cacheSpace {
	int iType;
	char *pData;
	int iDataSize;
	int nData;
	int iLineSize;
	int nLines;
	int nTrans;
	int iTransMask;
        int iKeyShift;
        int iInvKeyShift;
        int iIdMask;
	int *pTrans;
	CTAG *pTag;
	char *pLine;
	void (*init)(void *);
	void (*combine)(void *,void *);
	/*	
	 ** Statistics stuff.
	 */
	int nAccess;
	int nAccHigh;
	long nMiss;
	long nColl;
	long nMin;
	int nKeyMax;
	char *pbKey;
	} CACHE;

typedef struct serviceRec {
	int nInBytes;
	int nOutBytes;
	void *p1;
	void (*fcnService)(void *,void *,int,void *,int *);	
	} SERVICE;

typedef struct mdlContext {
	int nThreads;
	int idSelf;
	int bDiag;
	FILE *fpDiag;
	/*
	 ** Services stuff!
	 */
	int nMaxServices;
	int nMaxSrvBytes;
	SERVICE *psrv;
	char *pszOut;
	/*
	 ** Caching stuff!
	 */
	unsigned long uRand;
	int iMaxDataSize;
	int iCaBufSize;
	int nMaxCacheIds;
	CACHE *cache;
	} * MDL;


/*
 * MDL debug and Timer macros and prototypes 
 */
/* 
 * Compile time mdl debugging options
 *
 * mdl asserts: define MDLASSERT
 * Probably should always be on unless you want no mdlDiag output at all
 *
 * NB: defining NDEBUG turns off all asserts so MDLASSERT will not assert
 * however it will output using mdlDiag and the code continues.
 */
#define MDLASSERT
/* 
 * Debug functions active: define MDLDEBUG
 * Adds debugging mdldebug prints and mdldebugassert asserts
 */
#define MDLDEBUG
/* 
 * Timer functions active: define MDLTIMER
 * Makes mdl timer functions active
 */
#define MDLTIMER


#define printf CkPrintf

void mdlprintf( MDL mdl, const char *format, ... );

#ifdef MDLASSERT
#ifdef __ANSI_CPP__
#define mdlassert(mdl,expr) \
    { \
      if (!(expr)) { \
             CkPrintf("%s:%d Assertion `%s' failed.\n", __FILE__, __LINE__, # expr ); \
             assert( expr ); \
             } \
    }
#else
#define mdlassert(mdl,expr) \
    { \
      if (!(expr)) { \
             CkPrintf("%s:%d Assertion `%s' failed.\n", __FILE__, __LINE__, "expr" ); \
             assert( expr ); \
             } \
    }
#endif
#else
#define mdlassert(mdl,expr)  assert(expr)
#endif

#ifdef MDLDEBUG
#define mdldebugassert(mdl,expr)   mdlassert(mdl,expr)
void mdldebug( MDL mdl, const char *format, ... );
#else
#define mdldebug
#define mdldebugassert
#endif

typedef struct {
  double wallclock;
  double cpu;
  double system;
} mdlTimer;

#ifdef MDLTIMER
void mdlZeroTimer(MDL mdl,mdlTimer *);
void mdlGetTimer(MDL mdl,mdlTimer *,mdlTimer *);
void mdlPrintTimer(MDL mdl,char *message,mdlTimer *);
#else
#define mdlZeroTimer
#define mdlGetTimer
#define mdlPrintTimer
#endif

/*
 ** General Functions
 */
double mdlCpuTimer(MDL);
int mdlInitialize(MDL *,char **,void (*)(MDL));
void mdlFinish(MDL);
int mdlThreads(MDL);
int mdlSelf(MDL);
int mdlSwap(MDL,int,int,void *,int,int *,int *);
void mdlDiag(MDL,char *);
void mdlAddService(MDL,int,void *,void (*)(void *,void *,int,void *,int *),
				   int,int);
void mdlReqService(MDL,int,int,void *,int);
void mdlGetReply(MDL,int,void *,int *);
void mdlHandler(MDL);
/*
 ** Caching functions.
 */
void *mdlMalloc(MDL,int);
void mdlFree(MDL,void *);
void mdlROcache(MDL,int,void *,int,int);
void mdlCOcache(MDL,int,void *,int,int,
				void (*)(void *),void (*)(void *,void *));
void mdlFinishCache(MDL,int);
void mdlCacheCheck(MDL);
void *mdlAquire(MDL,int,int,int);
void mdlRelease(MDL,int,void *);
/*
 ** Cache statistics functions.
 */
double mdlNumAccess(MDL,int);
double mdlMissRatio(MDL,int);
double mdlCollRatio(MDL,int);
double mdlMinRatio(MDL,int);

#ifdef __cplusplus
}
#endif

#endif
