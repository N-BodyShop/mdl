#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED
#include <stdio.h>
#include <pthread.h>

#define MDL_CACHE_SIZE		1000000
#define MDL_CACHELINE_BITS	3
#define MDL_CACHELINE_ELTS	(1<<MDL_CACHELINE_BITS)
#define MDL_CACHE_MASK		(MDL_CACHELINE_ELTS-1)
#define MDL_INDEX_MASK		(~MDL_CACHE_MASK)

#define SRV_STOP		0

typedef struct cacheTag {
	int iKey;
	int nLock;
	int nLast;
	int iLink;
	} CTAG;

typedef struct mbxStruct {
    pthread_mutex_t mux;
	pthread_cond_t sigReq;
	pthread_cond_t sigRpl;
	pthread_cond_t sigRel;
	int bReq;
    int bRpl;
    int bRel;
    int sid;
    int nBytes;
    char *pszIn;
    char *pszOut;
    } MBX;

typedef struct swxStruct {
    pthread_mutex_t mux;
	pthread_cond_t sigRel;
	pthread_cond_t sigRec;
	pthread_cond_t sigSnd;
	int bRel;
	int bRec;
    int nInBytes;
	int nOutBufBytes;
	char *pszBuf;
    } SWX;

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
	/*
	 ** Combiner cache Mutexes, one per data line.
	 */
	int nMux;
	pthread_mutex_t *pMux;
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
	pthread_t *pt;
	struct mdlContext **pmdl;
	/*
	 ** Services stuff!
	 */
	int nMaxServices;
	int nMaxInBytes;
	int nMaxOutBytes;
	SERVICE *psrv;
	MBX mbxOwn;
	/*
	 ** Swapping Box.
	 */
	SWX swxOwn;
	/*
	 ** Caching stuff!
	 */
	unsigned long uRand;
	int nMaxCacheIds;
	int iMaxDataSize;
	CACHE *cache;
	} * MDL;


double mdlCpuTimer(MDL);
int mdlInitialize(MDL *,char **,void *(*)(void *));
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
void *mdlAquire(MDL,int,int,int);
void mdlRelease(MDL,int,void *);
void mdlCacheCheck(MDL);
/*
 ** Cache statistics functions.
 */
double mdlNumAccess(MDL,int);
double mdlMissRatio(MDL,int);
double mdlCollRatio(MDL,int);
double mdlMinRatio(MDL,int);

#endif








