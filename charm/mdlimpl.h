#ifndef MDLIMPL_HINCLUDED
#define MDLIMPL_HINCLUDED

#include "pup_stl.h"

#include "mdl.decl.h"

CProxy_AMdl aId;
CProxy_Main MainId;
CProxy_grpCache CacheId;

#define MDL_CACHE_SIZE		8000000
#define MDL_CACHELINE_BITS	4
#define MDL_CACHELINE_ELTS	(1<<MDL_CACHELINE_BITS)
#define MDL_CACHE_MASK		(MDL_CACHELINE_ELTS-1)
#define MDL_INDEX_MASK		(~MDL_CACHE_MASK)

/*
 ** This structure should be "maximally" aligned, with 4 ints it
 ** should align up to at least QUAD word, which should be enough.
 */
typedef struct srvHeader {
	int idFrom;
	int sid;
	int nInBytes;
	int nOutBytes;
	} SRVHEAD;

/*
 ** This structure should be "maximally" aligned, with 4 ints it
 ** should align up to at least QUAD word, which should be enough.
 */
typedef struct cacheHeader {
	int cid;
	int mid;
	int id;
	int iLine;
	} CAHEAD;

class MdlMsg : public CMessage_MdlMsg 
{
 public:
    SRVHEAD ph;
    char *pszBuf;

    static void *alloc(int mnum, size_t size, int *sizes, int priobits);
    static void *pack(MdlMsg *msg);
    static MdlMsg *unpack(void *buf);  
    };

class MdlSwapMsg : public CMessage_MdlSwapMsg 
{
 public:
    int nBytes;
    char *pszBuf;

    static void *alloc(int mnum, size_t size, int *sizes, int priobits);
    static void *pack(MdlSwapMsg *msg);
    static MdlSwapMsg *unpack(void *buf);  
    };

class MdlCacheMsg : public CMessage_MdlCacheMsg 
{
 public:
    CAHEAD ch;
    char *pszBuf;

    static void *alloc(int mnum, size_t size, int *sizes, int priobits);
    static void *pack(MdlCacheMsg *msg);
    static MdlCacheMsg *unpack(void *buf);  
    };

class MdlCacheFlshMsg : public CMessage_MdlCacheFlshMsg 
{
 public:
    CAHEAD ch;
    int nLines;
    int *pLine;
    char *pszBuf;

    static void *alloc(int mnum, size_t size, int *sizes, int priobits);
    static void *pack(MdlCacheFlshMsg *msg);
    static MdlCacheFlshMsg *unpack(void *buf);  
    };

extern "C"
void AMPI_Main(int argc, char **);

class Main : public CBase_Main
{
    int nfinished;
    
public:
    Main(CkArgMsg* m);
    void startMain(CkArgMsg* m);
    void done(void);
};

typedef struct cacheTag {
	int iKey;
	int nLock;
	int nLast;
	int iLink;
	int iIdLock;
	int bFetching;
	} CTAG;

typedef	struct procDATA {
	    int iProc;
	    char *pData;
	    int nData;
	    } PDATA;

typedef struct cacheSpace {
	int iType;
        PDATA *procData;
	int iDataSize;
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
	int nOut;
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

class grpCache : public NodeGroup 
{
 public:
    int nMaxCacheIds;
    int iMaxDataSize;
    int iCaBufSize;
    CACHE *cache;
    CmiNodeLock lock;
    CthThreadStruct * threadBarrier;
    int nFlush;
    
    grpCache();
    void CacheInitialize(int cid,void *pData,int iDataSize,int nData,
		    void (*init)(void *),void (*combine)(void *,void *));
    void AdjustDataSize();
    void flushreply();
    void waitflush();
    void FinishCache(int cid);
};

void mdlSetup(MDL *pmdl, int bDiag, const char *);

PUPbytes(void *);

// class AMdl : public ArrayElement1D
class AMdl : public CBase_AMdl
{
public:
    struct {			/* state data for mdlSwap() */
	int nInBytes;
	int nOutBytes;
	int nBufBytes;
	int nOutBufBytes;
	int nRcvBytes;
	int nSndBytes;
	int id;
	char *pszOut;
	char *pszIn;
	int done;
	} swapData;
    CthThreadStruct * threadSwap;
    CthThreadStruct * threadGetReply;
    CthThreadStruct * threadSrvWait;
    CthThreadStruct * threadBarrier;
    CthThreadStruct * threadCache;
    CACHE *cache;		/* pointer to nodegroup cache */
    CmiNodeLock *lock;		/* pointer to nodegroup lock */
    MdlMsg ** msgReply;
    MdlCacheMsg *msgCache;
    int idReplyWait;
    int nInBar;
    int nFlush;
    
    MDL mdl;
    AMdl(int bDiag, const std::string& progname);

    void AMdlInit(void *fcnPtr);
    AMdl(CkMigrateMessage*) {}
    void swapInit(int, int);
    void swapSendMore();
    void swapGetMore(MdlSwapMsg *);
    void swapDone();
    void waitSwapDone();
    MdlMsg *waitReply(int id);
    void waitSrvStop();
    void stopSrv();
    void reqReply(MdlMsg * mesg);
    void reqHandle(MdlMsg * mesg);
    void CacheRequest(MdlCacheMsg *mesg);
    void CacheReply(MdlCacheMsg *mesg);
    void CacheFlush(MdlCacheMsg *mesg);
    void CacheFlushAll(MdlCacheFlshMsg *mesg);
    MdlCacheMsg *waitCache() ;
    void barrier();
    void barrierEnter();
    void barrierRel();
    void waitflush();
};

#endif
