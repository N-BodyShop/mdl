#ifndef MDL_HINCLUDED
#define MDL_HINCLUDED
#include <stdio.h>
#include <pthread.h>

#define MDL_MAX_SERVICES		500
#define MDL_MAX_SERVICE_BYTES	4096
#define MDL_MAX_CACHE_SPACES	10
#define MDL_TRANS_SIZE			8096

#define SRV_STOP		0
#define DATA(D,S)	((struct S *)(D))
#define SIZE(S)		(sizeof(struct S))


typedef struct mbxStruct {
    pthread_mutex_t mux;
    int bReq;
    int bRpl;
    int bRel;
    int sid;
    int nBytes;
    __align128 char msgBuf[MDL_MAX_SERVICE_BYTES];
    } MBX;

typedef struct swxStruct {
    pthread_mutex_t mux;
	int bSet;
    int nInBytes;
	int nOutBufBytes;
	__align128 char achBuf[MDL_TRANS_SIZE];
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
	pthread_mutex_t *pMux;
	void (*init)(void *);
	void (*combine)(void *,void *);
	} CACHE;


typedef struct mdlContext {
	int nThreads;
	int idSelf;
	int bDiag;
	FILE *fpDiag;
	pthread_t *pt;
	struct mdlContext **pmdl;
    MBX mbxOwn;
	SWX swxOwn;
	/*
	 ** Services stuff!
	 */
	void *pp1[MDL_MAX_SERVICES];
	void (*pfcnService[MDL_MAX_SERVICES])(void *,char *,int,char *,int *);
	__align128 char msgBuf[MDL_MAX_SERVICE_BYTES];
	/*
	 ** Caching stuff!
	 */
	pthread_barrier_t bar;	/* only thread 0 initializes this barrier */
	unsigned long uRand;
	CACHE cache[MDL_MAX_CACHE_SPACES];
	} * MDL;


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








