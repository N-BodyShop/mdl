#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED
#include <stdio.h>
#include <mpi.h>


#define SRV_STOP		0

#define MDL_CACHE_SIZE		100000
#define MDL_CACHELINE_BITS	3
#define MDL_CACHELINE_ELTS	(1<<MDL_CACHELINE_BITS)
#define MDL_CACHE_MASK		(MDL_CACHELINE_ELTS-1)
#define MDL_INDEX_MASK		(~MDL_CACHE_MASK)

typedef struct cacheTag {
	int iKey;
	int id;
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
	int *pTrans;
	CTAG *pTag;
	char *pLine;
        long pDataMax;
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
	char *pszIn;
	char *pszOut;
	char *pszBuf;
	/*
	 ** Swapping buffer.
	 */
	char *pszTrans;
	/*
	 ** Caching stuff!
	 */
	unsigned long uRand;
	int iMaxDataSize;
	int nMaxCacheIds;
	CACHE *cache;
	} * MDL;


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
void *mdlSteal(MDL mdl,int cid,int iIndex,int id);
/*
 ** Cache statistics functions.
 */
double mdlNumAccess(MDL,int);
double mdlMissRatio(MDL,int);
double mdlCollRatio(MDL,int);
double mdlMinRatio(MDL,int);

#endif

