#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED
#include <stdio.h>


#define SRV_STOP		0

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
	int req[4];
	int rpl[4];
	int chko[4];
	int chki[4];
	int flsh[4];
	int nAccess;
	int nCheckIn;
	int nCheckOut;
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
	int *atid;
	FILE *fpDiag;
	/*
	 ** Services stuff!
	 */
	int nMaxServices;
	int nMaxInBytes;
	int nMaxOutBytes;
	SERVICE *psrv;
	char *pszIn;
	char *pszOut;
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
void mdlROcache(MDL,int,void *,int,int);
void mdlCOcache(MDL,int,void *,int,int,
				void (*)(void *),void (*)(void *,void *));
void mdlFinishCache(MDL,int);
void *mdlAquire(MDL,int,int,int);
void mdlRelease(MDL,int,void *);

#endif

