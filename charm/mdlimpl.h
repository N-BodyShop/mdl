#ifndef MDLIMPL_HINCLUDED
#define MDLIMPL_HINCLUDED

#include "mdl.decl.h"

CProxy_AMdl aId;
CProxy_Main MainId;

#define MDL_CACHE_SIZE		4000000
#define MDL_CACHELINE_BITS	3
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

void mdlSetup(MDL *pmdl, int bDiag, char *);

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
    CthThreadStruct * threadCache;
    CthThreadStruct * threadBarrier;
    MdlMsg ** msgReply;
    int idReplyWait;
    MdlCacheMsg *msgCache;
    int nInBar;
    
    MDL mdl;
    AMdl(int bDiag, char *progname);

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
    void CacheReceive(MdlCacheMsg *mesg);
    MdlCacheMsg *waitCache() ;
    void barrier();
    void barrierEnter();
    void barrierRel();
};

#endif
