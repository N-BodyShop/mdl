#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED
#include <stdio.h>


#define MDL_MAX_SERVICES		500
#define MDL_MAX_SERVICE_BYTES	4096
#define MDL_MAX_CACHE_SPACES	10

#define SRV_STOP		0
#define DATA(D,S)	((struct S *)(D))
#define SIZE(S)		(sizeof(struct S))


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


typedef struct mdlContext {
	int nThreads;
	int idSelf;
	int bDiag;
	int *atid;
	FILE *fpDiag;
	/*
	 ** Services stuff!
	 */
	void *pp1[MDL_MAX_SERVICES];
	void (*pfcnService[MDL_MAX_SERVICES])(void *,char *,int,char *,int *);
	char pszIn[MDL_MAX_SERVICE_BYTES];
	char pszOut[MDL_MAX_SERVICE_BYTES];
	/*
	 ** Caching stuff!
	 */
	unsigned long uRand;
	int iMaxDataSize;
	CACHE cache[MDL_MAX_CACHE_SPACES];
	} * MDL;

double mdlCpuTimer(MDL);
int mdlInitialize(MDL *,char **,void (*)(MDL));
void mdlFinish(MDL);
int mdlThreads(MDL);
int mdlSelf(MDL);
int mdlSwap(MDL,int,int,char *,int,int *,int *);
void mdlDiag(MDL,char *);
void mdlAddService(MDL,int,void *,void (*)());
void mdlReqService(MDL,int,int,char *,int);
void mdlGetReply(MDL,int,char *,int *);
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

