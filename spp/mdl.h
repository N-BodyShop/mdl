#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED
#include <stdio.h>
#include <cps.h>

#define SRV_STOP		0

typedef struct mbxStruct {
    cps_mutex_t mux;
    int bReq;
    int bRpl;
    int bRel;
    int sid;
    int nBytes;
    char *pszIn;
    char *pszOut;
    } MBX;

typedef struct swxStruct {
    cps_mutex_t mux;
	int bSet;
    int nInBytes;
	int nOutBufBytes;
	char *pszBuf;
    } SWX;


typedef struct cacheTag {
	int id;
	int iLine;
	int nLock;
	int iLink;
	} CTAG;


typedef struct cacheSpace {
	int iType;
	char *pData;
	int iDataSize;
	int nData;
	int nLineElts;
	int iLineSize;
	int nLines;
	int nTrans;
	int iTransMask;
	int *pTrans;
	CTAG *pTag;
	char *pLine;
	int nMux;
	cps_mutex_t *pMux;
	void (*init)(void *);
	void (*combine)(void *,void *);
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
	ktid_t *pt;
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
	barrier_t bar;	/* only thread 0 initializes this barrier */
	unsigned long uRand;
	int nMaxCacheIds;
	int iMaxDataSize;
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








