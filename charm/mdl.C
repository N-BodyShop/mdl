/*
 ** Charm++ version of MDL.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <math.h>
#include <limits.h>
#include <assert.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <values.h>
#include "mdl.h"
#include "mdlimpl.h"

#define MDL_NOCACHE			0
#define MDL_ROCACHE			1
#define MDL_COCACHE			2

#define MDL_DEFAULT_BYTES		80000
#define MDL_DEFAULT_SERVICES	50
#define MDL_DEFAULT_CACHEIDS	5

#define MDL_TRANS_SIZE		50000

void _srvNull(void *p1,void *vin,int nIn,void *vout,int *pnOut)
{
	return;
	}

double mdlCpuTimer(MDL mdl)
{
#ifndef _CRAYMPP
	struct rusage ru;

	getrusage(0,&ru);
	return((double)ru.ru_utime.tv_sec + 1e-6*(double)ru.ru_utime.tv_usec);
#else
	return( ((double) clock())/CLOCKS_PER_SEC);
#endif
	}

/* 
 * MDL debug and Timer functions 
 */
#define MDLPRINTF_STRING_MAXLEN 256
void mdlprintf( MDL mdl, const char *format, ... )
{
     static char ach[MDLPRINTF_STRING_MAXLEN];
     va_list args;

     if (mdl->bDiag) {	
         va_start( args, format);
         vsnprintf( ach, MDLPRINTF_STRING_MAXLEN, format, args);
         mdlDiag( mdl, ach);
         va_end( args);
         }
}

#ifdef MDLDEBUG
void mdldebug( MDL mdl, const char *format, ... )
{
     static char ach[MDLPRINTF_STRING_MAXLEN];
     va_list args;

     if (mdl->bDiag) {	
         va_start( args, format);
	 vsnprintf( ach, MDLPRINTF_STRING_MAXLEN, format, args);
	 mdlDiag( mdl, ach);
	 va_end( args);
         }
}
#endif

#ifdef MDLTIMER
void mdlZeroTimer(MDL mdl, mdlTimer *t)
{
  struct timezone tz;
  struct timeval tv;
  struct rusage ru;
  tz.tz_minuteswest = 0;
  tz.tz_dsttime = 0;
  gettimeofday(&tv,&tz);
  t->wallclock = tv.tv_sec + 1e-6*(double) tv.tv_usec;
  getrusage(0,&ru);
  t->cpu = (double)ru.ru_utime.tv_sec + 1e-6*(double)ru.ru_utime.tv_usec;
  t->system = (double)ru.ru_stime.tv_sec + 1e-6*(double)ru.ru_stime.tv_usec;
}

void mdlGetTimer(MDL mdl, mdlTimer *t0, mdlTimer *t)
{
  struct timezone tz;
  struct timeval tv;
  struct rusage ru;

  getrusage(0,&ru);
  t->cpu = (double)ru.ru_utime.tv_sec + 1e-6*(double)ru.ru_utime.tv_usec - t0->cpu;
  t->system = (double)ru.ru_stime.tv_sec + 1e-6*(double)ru.ru_stime.tv_usec - t0->system;
  tz.tz_minuteswest = 0;
  tz.tz_dsttime = 0;
  gettimeofday(&tv,&tz);
  t->wallclock = tv.tv_sec + 1e-6*(double) tv.tv_usec - t0->wallclock;
}

void mdlPrintTimer(MDL mdl,char *message, mdlTimer *t0) 
{
  mdlTimer lt;

  if (mdl->bDiag) {	
      mdlGetTimer(mdl,t0,&lt);
      mdlprintf(mdl,"%s %f %f %f\n",message,lt.wallclock,lt.cpu,lt.system);
      }
}
#endif

/*
 * Charm start-up strategy:
 * The mainchare Main() is called first.
 * It will then call the main() for PKDGRAV which will call 
 * mdlInitialize()
 * Slight change: a threaded routine has to start main to keep things
 * from blocking startMain() serves this purpose.
 * mdlInitialize will then call proxies on other processors which will
 * invoke fcnChild()
 * mdlInitialize will then return so that the main() in PKDGRAV will
 * continue.
 */


// Plan here: have a helper function do all the MDL structure setup
// For main thread return it via pmdl here
// For child threads pass it to fcnChild in the AMdl() constructor

// here is the helper functoin
void mdlSetup(MDL *pmdl, int bDiag, char *progname)
{
	MDL mdl;
	int i;
	char *p,ach[256],achDiag[256];
    
	*pmdl = NULL;
	mdl = (mdlContext *) malloc(sizeof(struct mdlContext));
	assert(mdl != NULL);
	/*
	 ** Set default "maximums" for structures. These are NOT hard
	 ** maximums, as the structures will be realloc'd when these
	 ** values are exceeded.
	 */
	mdl->nMaxServices = MDL_DEFAULT_SERVICES;
	mdl->nMaxSrvBytes = MDL_DEFAULT_BYTES;
	mdl->nMaxCacheIds = MDL_DEFAULT_CACHEIDS;
	/*
	 ** Now allocate the initial service slots.
	 */
	mdl->psrv = (SERVICE *)malloc(mdl->nMaxServices*sizeof(SERVICE));
	assert(mdl->psrv != NULL);
	/*
	 ** Initialize the new service slots.
	 */
	for (i=0;i<mdl->nMaxServices;++i) {
		mdl->psrv[i].p1 = NULL;
		mdl->psrv[i].nInBytes = 0;
		mdl->psrv[i].nOutBytes = 0;
		mdl->psrv[i].fcnService = NULL;
		}
	/*
	 ** Provide a 'null' service for sid = 0, so that stopping the 
	 ** service handler is well defined!
	 */
	mdl->psrv[0].p1 = NULL;
	mdl->psrv[0].nInBytes = 0;
	mdl->psrv[0].nOutBytes = 0;
	mdl->psrv[0].fcnService = _srvNull;
	/*
	 ** Allocate service buffers.
	 */
	mdl->pszOut = (char *)malloc(mdl->nMaxSrvBytes+sizeof(SRVHEAD));
	assert(mdl->pszOut != NULL);
	/*
	 ** Allocate initial cache spaces.
	 */
	mdl->cache = (CACHE *) malloc(mdl->nMaxCacheIds*sizeof(CACHE));
	assert(mdl->cache != NULL);
	/*
	 ** Initialize caching spaces.
	 */
	for (i=0;i<mdl->nMaxCacheIds;++i) {
		mdl->cache[i].iType = MDL_NOCACHE;
		}

	/*
	 ** Do some low level argument parsing for number of threads, and
	 ** diagnostic flag!
	 */
	if (bDiag) {
	    p = getenv("MDL_DIAGNOSTIC");
	    if (!p) p = getenv("HOME");
	    if (!p) sprintf(ach,"/tmp");
	    else sprintf(ach,"%s",p);
	    }

	mdl->nThreads = CkNumPes();
	mdl->idSelf = CkMyPe();
	/*
	 ** Allocate caching buffers, with initial data size of 0.
	 ** We need one reply buffer for each thread, to deadlock situations.
	 */
	mdl->iMaxDataSize = 0;
	mdl->iCaBufSize = sizeof(CAHEAD);
	mdl->bDiag = bDiag;
	*pmdl = mdl;
	if (mdl->bDiag) {
		sprintf(achDiag,"%s/%s.%d",ach,progname,mdl->idSelf);
		mdl->fpDiag = fopen(achDiag,"w");
		assert(mdl->fpDiag != NULL);
	    }
    }

Main::Main(CkArgMsg* m)
{
      char **argv = m->argv;
	int i,bDiag,bThreads;

	/*
	 ** Do some low level argument parsing for number of threads, and
	 ** diagnostic flag!
	 */
	bDiag = 0;
	bThreads = 0;
	i = 1;
	while (argv[i]) {
		if (!strcmp(argv[i],"-sz") && !bThreads) {
			++i;
			if (argv[i]) bThreads = 1;
			}
		if (!strcmp(argv[i],"-d") && !bDiag) {
			bDiag = 1;
			}
		++i;
		}
	if (bThreads) {
		fprintf(stderr,"Warning: -sz parameter ignored, using as many\n");
		fprintf(stderr,"         processors as specified in environment.\n");
		fflush(stderr);
		}

	char *tmp = strrchr(argv[0],'/');
	if (!tmp) tmp = argv[0];
	else ++tmp;

      nfinished = 0;
      MainId = thishandle;
      aId = CProxy_AMdl::ckNew(bDiag, tmp, CkNumPes());
	
      MainId.startMain(m);
    };

void
Main::startMain(CkArgMsg* m)
{
      // Repack command-line arguments
      int argc = m->argc;
      char **argv = m->argv;

      delete m;

      AMPI_Main(argc, argv);
};

void
Main::done(void)
  {
      nfinished++;
      
      if(nfinished >= CkNumPes())
	  CkExit();
  };

AMdl::AMdl(int bDiag, char *progname)
{
    int i;
    
	mdlSetup(&mdl, bDiag, progname);

	msgReply = (MdlMsg **) malloc(mdl->nThreads*sizeof(MdlMsg *));
	for(i = 0; i < mdl->nThreads; i++)
	    msgReply[i] = NULL;
	threadGetReply = 0;
	msgCache = NULL;
	threadCache = 0;
	threadBarrier = 0;
	nInBar = 0;
	swapData.id = -1;
    }

void
AMdl::AMdlInit(void *fcnPtr)
    {
	
	void (*fcnChild)(MDL) = (void (*)(MDL)) fcnPtr;
	
	
	if(thisIndex == 0) return;
	(*fcnChild)(mdl);
	mdlFinish(mdl);
    }

extern "C"
int mdlInitialize(MDL *pmdl,char **argv,void (*fcnChild)(MDL))
{

	void *fcnPtr = (void *)fcnChild;
	
	CProxy_AMdl amdlProxy(aId);

	for(int i = 0; i < CkNumPes(); i++) {
	    amdlProxy[i].AMdlInit(fcnPtr);
	    }
	
	*pmdl = amdlProxy[0].ckLocal()->mdl;
	
	return (*pmdl)->nThreads;
	
    }

// Creation routines for variable size messages.
void* MdlMsg::alloc(int mnum, size_t size, int *sizes, int priobits){
    int total_size = size + sizes[0] * sizeof(char);
    MdlMsg * mesg = (MdlMsg *)CkAllocMsg(mnum, total_size, priobits);
    mesg->pszBuf = (char *)((char*)mesg + sizeof(MdlMsg));
    return (void *)mesg;
}

void* MdlMsg::pack(MdlMsg *mesg){
    return (void *) mesg;
}

MdlMsg* MdlMsg::unpack(void *buf){
    MdlMsg *mesg = (MdlMsg*)buf;
    mesg->pszBuf = (char *)((char*)mesg + sizeof(MdlMsg));
    return mesg;
}

// Creation routines for swap messages.
void* MdlSwapMsg::alloc(int mnum, size_t size, int *sizes, int priobits){
    int total_size = size + sizes[0] * sizeof(char);
    MdlSwapMsg * mesg = (MdlSwapMsg *)CkAllocMsg(mnum, total_size, priobits);
    mesg->pszBuf = (char *)((char*)mesg + sizeof(MdlSwapMsg));
    return (void *)mesg;
}

void* MdlSwapMsg::pack(MdlSwapMsg *mesg){
    return (void *) mesg;
}

MdlSwapMsg* MdlSwapMsg::unpack(void *buf){
    MdlSwapMsg *mesg = (MdlSwapMsg*)buf;
    mesg->pszBuf = (char *)((char*)mesg + sizeof(MdlSwapMsg));
    return mesg;
}

// Creation routines for cache messages.
void* MdlCacheMsg::alloc(int mnum, size_t size, int *sizes, int priobits){
    int total_size = size + sizes[0] * sizeof(char);
    MdlCacheMsg * mesg = (MdlCacheMsg *)CkAllocMsg(mnum, total_size, priobits);
    mesg->pszBuf = (char *)((char*)mesg + sizeof(MdlCacheMsg));
    return (void *)mesg;
}

void* MdlCacheMsg::pack(MdlCacheMsg *mesg){
    return (void *) mesg;
}

MdlCacheMsg* MdlCacheMsg::unpack(void *buf){
    MdlCacheMsg *mesg = (MdlCacheMsg*)buf;
    mesg->pszBuf = (char *)((char*)mesg + sizeof(MdlCacheMsg));
    return mesg;
}


extern "C"
void mdlFinish(MDL mdl)
{
	/*
	 ** Close Diagnostic file.
	 */
	if (mdl->bDiag) {
		fclose(mdl->fpDiag);
		}
	/*
	 ** Deallocate storage.
	 */
	free(mdl->psrv);
	free(mdl->pszOut);
	free(mdl->cache);
	free(mdl);
	
	CProxy_Main proxyMain(MainId);
	proxyMain.done();
	CthSuspend(); 
	}


/*
 ** This function returns the number of threads in the set of 
 ** threads.
 */
extern "C"
int mdlThreads(MDL mdl)
{
	return(mdl->nThreads);
	}


/*
 ** This function returns this threads 'id' number within the specified
 ** MDL Context. Parent thread always has 'id' of 0, where as children
 ** have 'id's ranging from 1..(nThreads - 1).
 */
extern "C"
int mdlSelf(MDL mdl)
{
	return(mdl->idSelf);
	}


/*
 ** This is a tricky function. It initiates a bilateral transfer between
 ** two threads. Both threads MUST be expecting this transfer. The transfer
 ** occurs between idSelf <---> 'id' or 'id' <---> idSelf as seen from the
 ** opposing thread. It is designed as a high performance non-local memory
 ** swapping primitive and implementation will vary in non-trivial ways 
 ** between differing architectures and parallel paradigms (eg. message
 ** passing and shared address space). A buffer is specified by 'pszBuf'
 ** which is 'nBufBytes' in size. Of this buffer the LAST 'nOutBytes' are
 ** transfered to the opponent, in turn, the opponent thread transfers his
 ** nBufBytes to this thread's buffer starting at 'pszBuf'.
 ** If the transfer completes with no problems the function returns 1.
 ** If the function returns 0 then one of the players has not received all
 ** of the others memory, however he will have successfully transfered all
 ** of his memory.
 */
// Plan here:
// invoke method to give "OutBytes" and "BufBytes"
// That method invokes sendMore()
// Which invokes GetMore()
// repeat until done

extern "C"
int mdlSwap(MDL mdl,int id,
	    int nBufBytes,	// available free space on my processor
	    void *vBuf,		// beginning of buffer space
	    int nOutBytes,	// Bytes I want to transfer
	    int *pnSndBytes,int *pnRcvBytes
	    )
{
	AMdl *mmdl;
	char *pszBuf = (char *) vBuf;
	
	CProxy_AMdl proxyAmdl(aId);
	proxyAmdl[mdl->idSelf].ckLocal()->threadSwap = CthSelf();

	mmdl = proxyAmdl[mdl->idSelf].ckLocal();
	mmdl->swapData.nOutBytes = nOutBytes;
	mmdl->swapData.nBufBytes = nBufBytes;
	mmdl->swapData.nRcvBytes = 0;
	mmdl->swapData.nSndBytes = 0;
	mmdl->swapData.id = id;
	mmdl->swapData.pszIn = pszBuf;
	mmdl->swapData.pszOut = &pszBuf[nBufBytes - nOutBytes];
	mmdl->swapData.done = 0;
	
	assert(nBufBytes >= nOutBytes);
	proxyAmdl[id].swapInit(nOutBytes, nBufBytes);
	proxyAmdl[mdl->idSelf].ckLocal()->waitSwapDone();

	*pnRcvBytes = mmdl->swapData.nRcvBytes;
	*pnSndBytes = mmdl->swapData.nSndBytes;

	if (mmdl->swapData.nOutBytes) return(0);
	else if (mmdl->swapData.nInBytes) return(0);
	else return(1);
    }

void
AMdl::swapInit(int nInBytes, int nBufBytes)
{
    swapData.nInBytes = nInBytes;
    swapData.nOutBufBytes = nBufBytes;
    
    while(swapData.id == -1)
	CthYield();
    
    swapSendMore();
    }

void
AMdl::swapSendMore() 
{
    int i;
    
    CProxy_AMdl proxyAmdl(aId);

    if(swapData.nOutBytes && swapData.nOutBufBytes) {
	int nOutMax = (swapData.nOutBytes < MDL_TRANS_SIZE)
	    ? swapData.nOutBytes : MDL_TRANS_SIZE;
	nOutMax = (nOutMax < swapData.nOutBufBytes)
	    ? nOutMax : swapData.nOutBufBytes;

	assert(nOutMax > 0);
	
	MdlSwapMsg *mesg = new(&nOutMax, 0) MdlSwapMsg;
	mesg->nBytes = nOutMax;
	for (i=0;i<nOutMax;++i) mesg->pszBuf[i] = swapData.pszOut[i];
	
	// Adjust counts for next itteration
	swapData.pszOut += nOutMax;
	swapData.nOutBytes -= nOutMax;
	swapData.nOutBufBytes -= nOutMax;
	swapData.nSndBytes += nOutMax;
	proxyAmdl[swapData.id].swapGetMore(mesg);
	}
    else {
	proxyAmdl[thisIndex].swapDone();
	proxyAmdl[swapData.id].swapDone();
	}
    }

void
AMdl::swapGetMore(MdlSwapMsg *mesg) 
{
    CProxy_AMdl proxyAmdl(aId);
    int i;
    int nBytes = mesg->nBytes;	// temporary for bytes transferred
    
    while(swapData.pszIn + nBytes > swapData.pszOut)
	CthYield();		// pause while buffer is transferred out

    for(i = 0; i < nBytes; i++)
	swapData.pszIn[i] = mesg->pszBuf[i];

    swapData.pszIn += nBytes;
    swapData.nInBytes -= nBytes;
    swapData.nBufBytes -= nBytes;
    swapData.nRcvBytes += nBytes;

    delete mesg;
    proxyAmdl[swapData.id].swapSendMore();
    }

void
AMdl::swapDone()
{
    swapData.done++;
    if(swapData.done == 2) {
	CthAwaken(threadSwap);
	}
    
    }
	
void
AMdl::waitSwapDone()
{
	CthSuspend();
	swapData.id = -1;
    }

extern "C"
void mdlDiag(MDL mdl,char *psz)
{
	if (mdl->bDiag) {	
		fputs(psz,mdl->fpDiag);
		fflush(mdl->fpDiag);
		}
	}

extern "C"
void mdlAddService(MDL mdl,int sid,void *p1,
				   void (*fcnService)(void *,void *,int,void *,int *),
				   int nInBytes,int nOutBytes)
{
	int i,nMaxServices,nMaxBytes;

	assert(sid > 0);
	if (sid >= mdl->nMaxServices) {
		/*
		 ** reallocate service buffer, adding space for 8 new services
		 ** including the one just defined.
		 */
		nMaxServices = sid + 9;
		mdl->psrv = (SERVICE *) realloc(mdl->psrv,
						nMaxServices*sizeof(SERVICE));
		assert(mdl->psrv != NULL);
		/*
		 ** Initialize the new service slots.
		 */
		for (i=mdl->nMaxServices;i<nMaxServices;++i) {
			mdl->psrv[i].p1 = NULL;
			mdl->psrv[i].nInBytes = 0;
			mdl->psrv[i].nOutBytes = 0;
			mdl->psrv[i].fcnService = NULL;
			}
		mdl->nMaxServices = nMaxServices;
		}
	/*
	 ** Make sure the service buffers are big enough!
	 */
	nMaxBytes = (nInBytes > nOutBytes)?nInBytes:nOutBytes;
	if (nMaxBytes > mdl->nMaxSrvBytes) {
		mdl->pszOut = (char *) realloc(mdl->pszOut,nMaxBytes+sizeof(SRVHEAD));
		assert(mdl->pszOut != NULL);
		mdl->nMaxSrvBytes = nMaxBytes;
		}
	mdl->psrv[sid].p1 = p1;
	mdl->psrv[sid].nInBytes = nInBytes;
	mdl->psrv[sid].nOutBytes = nOutBytes;
	mdl->psrv[sid].fcnService = fcnService;
	}


extern "C"
void mdlReqService(MDL mdl,int id,int sid,void *vin,int nInBytes)
{
	char *pszIn = (char *) vin;
	MdlMsg *mesg;
	int i;
	
	mesg = new(&nInBytes, sizeof(int)) MdlMsg;
	// *((int *)CkPriorityPtr(mesg)) = MAXINT;
	// CkSetQueueing(mesg, CK_QUEUEING_IFIFO);
	
	mesg->ph.idFrom = mdl->idSelf;
	mesg->ph.sid = sid;
	if (!pszIn) mesg->ph.nInBytes = 0;
	else mesg->ph.nInBytes = nInBytes;
	if (nInBytes > 0 && pszIn != NULL) {
		for (i=0;i<nInBytes;++i) mesg->pszBuf[i] = pszIn[i];
		}
	CProxy_AMdl aProxy(aId);
	
	assert(aProxy[mdl->idSelf].ckLocal()->msgReply[mdl->idSelf] == NULL);
	
	aProxy[id].reqHandle(mesg);
	}

extern "C"
void mdlGetReply(MDL mdl,int id,void *vout,int *pnOutBytes)
{
	char *pszOut = (char *) vout;
	int i, nOutBytes;
	MdlMsg *mesg;

	CProxy_AMdl proxyAMdl(aId);
	mesg = proxyAMdl[mdl->idSelf].ckLocal()->waitReply(id);

	nOutBytes = mesg->ph.nOutBytes;
	
	if (nOutBytes > 0 && pszOut != NULL) {
		for (i=0; i< nOutBytes; ++i) pszOut[i] = mesg->pszBuf[i];
		}
	if (pnOutBytes) *pnOutBytes = nOutBytes;
	delete mesg;
	}

MdlMsg *
AMdl::waitReply(int id) 
{
    MdlMsg * msgTmp;
    
    if(msgReply[id]) {
	msgTmp = msgReply[id];
	msgReply[id] = NULL;
	return msgTmp;
	}
    else {
	threadGetReply = CthSelf();
	idReplyWait = id;
	CthSuspend();
	}
    assert(msgReply[id]);
    threadGetReply = 0;
    msgTmp = msgReply[id];
    msgReply[id] = NULL;
    return msgTmp;
    }

void
AMdl::reqReply(MdlMsg *mesg) 
{
    assert(msgReply[mesg->ph.idFrom] == NULL);
    
    msgReply[mesg->ph.idFrom] = mesg;
    
    if(threadGetReply && mesg->ph.idFrom == idReplyWait)
	CthAwaken(threadGetReply);
}


// In the CHARM case, this doesn't do much:
// It needs to just wait for the SRV_STOP function to happen, then return.

extern "C"
void mdlHandler(MDL mdl)
{
    CProxy_AMdl proxyAMdl(aId);
    
    proxyAMdl[mdl->idSelf].ckLocal()->waitSrvStop();
    }

void
AMdl::waitSrvStop()
{
    threadSrvWait = CthSelf();
    CthSuspend();
}

void
AMdl::stopSrv()
{
    assert(threadSrvWait);
    CthAwaken(threadSrvWait);
    }

void
AMdl::reqHandle(MdlMsg * mesg) 
{
	char *pszIn = mesg->pszBuf;
	int sid,id,nInBytes, nOutBytes;
	char *pszOut = &mdl->pszOut[sizeof(SRVHEAD)];
	int i;

	id = mesg->ph.idFrom;
	sid = mesg->ph.sid;
	nInBytes = mesg->ph.nInBytes;
	mdlassert(mdl, sid < mdl->nMaxServices);
	while(mdl->psrv[sid].fcnService == NULL)
	    CthYield();		// Wait for service to be registered
	
	mdlassert(mdl, nInBytes <= mdl->psrv[sid].nInBytes);
	nOutBytes = 0;
	assert(mdl->psrv[sid].fcnService != NULL);
	
	(*mdl->psrv[sid].fcnService)(mdl->psrv[sid].p1, pszIn,
				     nInBytes, pszOut, &nOutBytes);
	delete mesg;
	assert(nOutBytes <= mdl->psrv[sid].nOutBytes);

	mesg = new(&nOutBytes, 0) MdlMsg;
	mesg->ph.idFrom = mdl->idSelf;
	mesg->ph.sid = sid;
	mesg->ph.nInBytes = nInBytes;
	mesg->ph.nOutBytes = nOutBytes;
	for(i = 0; i < nOutBytes; i++)
	    mesg->pszBuf[i] = pszOut[i];

	CProxy_AMdl proxyAMdl(aId);
	proxyAMdl[id].reqReply(mesg);

	if(sid == SRV_STOP) {
	    // Stop handler
	    CProxy_AMdl proxyAMdl(aId);
	    proxyAMdl[mdl->idSelf].stopSrv();
	    }
	}

#define MDL_MID_CACHEREQ	2
#define MDL_MID_CACHERPL	3
#define MDL_MID_CACHEFLSH	5

#define MDL_CHECK_MASK  	0x7f
#define BILLION				1000000000

void
AMdl::CacheReceive(MdlCacheMsg *mesg)
{
	CACHE *c;
	char *pszRcv = mesg->pszBuf;
	char *pszRpl;
	char *t;
	int n,i;
	int iLineSize;
	int iDataSize;

	c = &mdl->cache[mesg->ch.cid];
	assert(c->iType != MDL_NOCACHE);
	CProxy_AMdl proxyAMdl(aId);
	MdlCacheMsg *mesgRpl;
	
	switch (mesg->ch.mid) {
	case MDL_MID_CACHEREQ:
		/*
		 ** This is the tricky part! Here is where the real deadlock
		 ** difficulties surface. Making sure to have one buffer per
		 ** thread solves those problems here.
		 */
		t = &c->pData[mesg->ch.iLine*c->iLineSize];
		if(t+c->iLineSize > c->pData + c->nData*c->iDataSize)
			iLineSize = c->pData + c->nData*c->iDataSize - t;
		else
			iLineSize = c->iLineSize;
		mesgRpl = new(&c->iLineSize, 0) MdlCacheMsg;
		
		pszRpl = mesgRpl->pszBuf;
		mesgRpl->ch.cid = mesg->ch.cid;
		mesgRpl->ch.mid = MDL_MID_CACHERPL;
		mesgRpl->ch.id = mdl->idSelf;
		for (i=0;i<iLineSize;++i) pszRpl[i] = t[i];
		proxyAMdl[mesg->ch.id].CacheReceive(mesgRpl);
		delete mesg;
		break;
	case MDL_MID_CACHEFLSH:
		assert(c->iType == MDL_COCACHE);
		i = mesg->ch.iLine*MDL_CACHELINE_ELTS;
		t = &c->pData[i*c->iDataSize];
		/*
		 ** Make sure we don't combine beyond the number of data elements!
		 */
		n = i + MDL_CACHELINE_ELTS;
		if (n > c->nData) n = c->nData;
		n -= i;
		n *= c->iDataSize;
		iDataSize = c->iDataSize;
		for (i=0;i<n;i+=iDataSize) {
			(*c->combine)(&t[i],&pszRcv[i]);
			}
		delete mesg;
		break;
	case MDL_MID_CACHERPL:
		/*
		 ** For now assume no prefetching!
		 ** This means that this WILL be the reply to this Aquire
		 ** request.
		 */
		msgCache = mesg;
		if(threadCache)
		    CthAwaken(threadCache);
		break;
	default:
		assert(0);
		}
	}


void AdjustDataSize(MDL mdl)
{
	int i,iMaxDataSize;

	/*
	 ** Change buffer size?
	 */
	iMaxDataSize = 0;
	for (i=0;i<mdl->nMaxCacheIds;++i) {
		if (mdl->cache[i].iType == MDL_NOCACHE) continue;
		if (mdl->cache[i].iDataSize > iMaxDataSize) {
			iMaxDataSize = mdl->cache[i].iDataSize;
			}
		}
	if (iMaxDataSize != mdl->iMaxDataSize) {
		mdl->iMaxDataSize = iMaxDataSize;
		mdl->iCaBufSize = sizeof(CAHEAD) + 
			iMaxDataSize*(1 << MDL_CACHELINE_BITS);
		
		}
	}

/*
 ** Special MDL memory allocation functions for allocating memory 
 ** which must be visible to other processors thru the MDL cache 
 ** functions.
 ** mdlMalloc() is defined to return a pointer to AT LEAST iSize bytes 
 ** of memory. This pointer will be passed to either mdlROcache or 
 ** mdlCOcache as the pData parameter.
 ** For PVM and most machines these functions are trivial, but on the 
 ** T3D and perhaps some future machines these functions are required.
 */
extern "C"
void *mdlMalloc(MDL mdl,int iSize)
{	
	return(malloc(iSize));
	}

extern "C"
void mdlFree(MDL mdl,void *p)
{
	free(p);
	}

/*
 ** Common initialization for all types of caches.
 */
CACHE *CacheInitialize(MDL mdl,int cid,void *pData,int iDataSize,int nData)
{
	CACHE *c;
	int i,nMaxCacheIds;
	int first;

	/*
	 ** Allocate more cache spaces if required!
	 */
	assert(cid >= 0);
	/*
	 * first cache?
	 */
	first = 1;
	for(i = 0; i < mdl->nMaxCacheIds; ++i) {
	    if(mdl->cache[i].iType != MDL_NOCACHE) {
		first = 0;
		break;
		}
	    }
	
	if (cid >= mdl->nMaxCacheIds) {
		/*
		 ** reallocate cache spaces, adding space for 2 new cache spaces
		 ** including the one just defined.
		 */
		nMaxCacheIds = cid + 3;
		mdl->cache = (CACHE *) realloc(mdl->cache,nMaxCacheIds*sizeof(CACHE));
		assert(mdl->cache != NULL);
		/*
		 ** Initialize the new cache slots.
		 */
		for (i=mdl->nMaxCacheIds;i<nMaxCacheIds;++i) {
			mdl->cache[i].iType = MDL_NOCACHE;
			}
		mdl->nMaxCacheIds = nMaxCacheIds;
		}
	c = &mdl->cache[cid];
	assert(c->iType == MDL_NOCACHE);
	c->pData = (char *) pData;
	c->iDataSize = iDataSize;
	c->nData = nData;
	c->iLineSize = MDL_CACHELINE_ELTS*c->iDataSize;
	c->iKeyShift = 0;
	while((1 << c->iKeyShift) < mdl->nThreads) ++c->iKeyShift;
	c->iIdMask = (1 << c->iKeyShift) - 1;
	
	if(c->iKeyShift < MDL_CACHELINE_BITS) {
	  /*
	   * Key will be (index & MDL_INDEX_MASK) | id.
	   */
	    c->iInvKeyShift = MDL_CACHELINE_BITS;
	    c->iKeyShift = 0;
	    }
	else {
	  /*
	   * Key will be (index & MDL_INDEX_MASK) << KeyShift | id.
	   */
	    c->iInvKeyShift = c->iKeyShift;
	    c->iKeyShift -= MDL_CACHELINE_BITS;
	    }
	
	/*
	 ** Determine the number of cache lines to be allocated.
	 */
	c->nLines = (MDL_CACHE_SIZE/c->iDataSize) >> MDL_CACHELINE_BITS;
	c->nTrans = 1;
	while(c->nTrans < c->nLines) c->nTrans *= 2;
	c->nTrans *= 2;
	c->iTransMask = c->nTrans-1;
	/*
	 **	Set up the translation table.
	 */
	c->pTrans = (int *) malloc(c->nTrans*sizeof(int));	
	assert(c->pTrans != NULL);
	for (i=0;i<c->nTrans;++i) c->pTrans[i] = 0;
	/*
	 ** Set up the tags. Note pTag[0] is a Sentinel!
	 */
	c->pTag = (CTAG *) malloc(c->nLines*sizeof(CTAG));
	assert(c->pTag != NULL);
	for (i=0;i<c->nLines;++i) {
		c->pTag[i].iKey = -1;	/* invalid */	
		c->pTag[i].nLock = 0;
		c->pTag[i].nLast = 0;	/* !!! */
		c->pTag[i].iLink = 0;
		}
	c->pTag[0].nLock = 1;		/* always locked */
	c->pTag[0].nLast = INT_MAX;  	/* always Most Recently Used */
	c->nAccess = 0;
	c->nAccHigh = 0;
	c->nMiss = 0;				/* !!!, not NB */
	c->nColl = 0;				/* !!!, not NB */
	c->nMin = 0;				/* !!!, not NB */	
	c->nKeyMax = 500;				/* !!!, not NB */
	c->pbKey = (char *) malloc(c->nKeyMax);			/* !!!, not NB */
	assert(c->pbKey != NULL);			/* !!!, not NB */
	for (i=0;i<c->nKeyMax;++i) c->pbKey[i] = 0;	/* !!!, not NB */
	/*
	 ** Allocate cache data lines.
	 */
	c->pLine = (char *) malloc(c->nLines*c->iLineSize);
	assert(c->pLine != NULL);
	return(c);
	}

void
AMdl::barrierRel()
{
    CthAwaken(threadBarrier);
    threadBarrier = 0;
    }
     
void
AMdl::barrierEnter()
{
    CProxy_AMdl proxyAMdl(aId);
    int i;

	nInBar++;
	if(nInBar == mdl->nThreads) {
	    nInBar = 0;
	    for(i = 0; i < mdl->nThreads; i++)
		proxyAMdl[i].barrierRel();
	    }
    }

void
AMdl::barrier()
{
    CProxy_AMdl proxyAMdl(aId);
    
    proxyAMdl[0].barrierEnter();
    threadBarrier = CthSelf();
    CthSuspend();
}

/*
 ** Initialize a Read-Only caching space.
 */
extern "C"
void mdlROcache(MDL mdl,int cid,void *pData,int iDataSize,int nData)
{
	CACHE *c;
	CProxy_AMdl proxyAMdl(aId);

	c = CacheInitialize(mdl,cid,pData,iDataSize,nData);
	c->iType = MDL_ROCACHE;
	/*
	 ** For an ROcache these two functions are not needed.
	 */
	c->init = NULL;
	c->combine = NULL;
	AdjustDataSize(mdl);
	proxyAMdl[mdl->idSelf].ckLocal()->barrier();
	}

/*
 ** Initialize a Combiner caching space.
 */
extern "C"
void mdlCOcache(MDL mdl,int cid,void *pData,int iDataSize,int nData,
				void (*init)(void *),void (*combine)(void *,void *))
{
	CACHE *c;
	CProxy_AMdl proxyAMdl(aId);

	c = CacheInitialize(mdl,cid,pData,iDataSize,nData);
	c->iType = MDL_COCACHE;
	assert(init);
	c->init = init;
	assert(combine);
	c->combine = combine;
	AdjustDataSize(mdl);
	proxyAMdl[mdl->idSelf].ckLocal()->barrier();
	}

extern "C"
void mdlFinishCache(MDL mdl,int cid)
{
	CACHE *c = &mdl->cache[cid];
	char *pszFlsh;
	int i,id;
	char *t;
	int j, iKey;
	CProxy_AMdl proxyAMdl(aId);

	if (c->iType == MDL_COCACHE) {
		/*
		 ** Must flush all valid data elements.
		 */
		for (i=1;i<c->nLines;++i) {
			iKey = c->pTag[i].iKey;
			if (iKey >= 0) {
				/*
				 ** Flush element since it is valid!
				 */
			    MdlCacheMsg *mesgFlsh = new(&c->iLineSize,0)
				MdlCacheMsg;
			    pszFlsh = mesgFlsh->pszBuf;
			    mesgFlsh->ch.cid = cid;
			    mesgFlsh->ch.mid = MDL_MID_CACHEFLSH;
			    mesgFlsh->ch.id = mdl->idSelf;
			    mesgFlsh->ch.iLine = iKey >> c->iInvKeyShift;
			    t = &c->pLine[i*c->iLineSize];
			    for(j = 0; j < c->iLineSize; ++j)
				pszFlsh[j] = t[j];
			    id = iKey & c->iIdMask;
			    proxyAMdl[id].CacheReceive(mesgFlsh);
			    }
			/*
			 * Check for any incoming cache requests.
			*/
			mdlCacheCheck(mdl);
			}
		}
	proxyAMdl[mdl->idSelf].ckLocal()->barrier();
	/*
	 ** Free up storage and finish.
	 */
	free(c->pTrans);
	free(c->pTag);
	free(c->pbKey);
	free(c->pLine);
	c->iType = MDL_NOCACHE;
	  
	AdjustDataSize(mdl);
	}


extern "C"
void mdlCacheCheck(MDL mdl)
{
    CthYield();
    }

MdlCacheMsg *
AMdl::waitCache() 
{
    MdlCacheMsg * msgTmp;
    
    if(msgCache) {
	msgTmp = msgCache;
	msgCache = NULL;
	return msgTmp;
	}
    else {
	threadCache = CthSelf();
	CthSuspend();
	}
    assert(msgCache);
    threadCache = 0;
    msgTmp = msgCache;
    msgCache = NULL;
    return msgTmp;
    }

extern "C"
void *mdlAquire(MDL mdl,int cid,int iIndex,int id)
{
	CACHE *c = &mdl->cache[cid];
	char *pLine;
	int iElt,iLine,i,iKey,iKeyVic,nKeyNew;
	int idVic;
	int iVictim,*pi;
	char ach[80];
	char *pszFlsh;

	++c->nAccess;
	if (!(c->nAccess & MDL_CHECK_MASK))
	        mdlCacheCheck(mdl);
	/*
	 ** Is it a local request?
	 */
	if (id == mdl->idSelf) {
		return(&c->pData[iIndex*c->iDataSize]);
		}
	/*
	 ** Determine memory block key value and cache line.
	iLine = iIndex >> MDL_CACHELINE_BITS;
	iKey = iLine*mdl->nThreads + id;
	 */
	iKey = ((iIndex&MDL_INDEX_MASK) << c->iKeyShift)| id;

	i = c->pTrans[iKey & c->iTransMask];
	/*
	 ** Check for a match!
	 */
	if (c->pTag[i].iKey == iKey) {
		++c->pTag[i].nLock;
		c->pTag[i].nLast = c->nAccess;
		pLine = &c->pLine[i*c->iLineSize];
		iElt = iIndex & MDL_CACHE_MASK;
		return(&pLine[iElt*c->iDataSize]);
		}
	i = c->pTag[i].iLink;
	/*
	 ** Collision chain search.
	 */
	while (i) {
		++c->nColl;
		if (c->pTag[i].iKey == iKey) {
			++c->pTag[i].nLock;
			c->pTag[i].nLast = c->nAccess;
			pLine = &c->pLine[i*c->iLineSize];
			iElt = iIndex & MDL_CACHE_MASK;
			return(&pLine[iElt*c->iDataSize]);
			}
		i = c->pTag[i].iLink;
		}
	/*
	 ** Cache Miss.
	 */
	iLine = iIndex >> MDL_CACHELINE_BITS;
	int nRequestBytes = 0;	// Just a place holder

	MdlCacheMsg *mesg = new(&nRequestBytes, 0) MdlCacheMsg;
	mesg->ch.cid = cid;
	mesg->ch.mid = MDL_MID_CACHEREQ;
	mesg->ch.id = mdl->idSelf;
	mesg->ch.iLine = iLine;

	CProxy_AMdl proxyAMdl(aId);
	proxyAMdl[id].CacheReceive(mesg);
	
	++c->nMiss;
	/*
	 **	LRU Victim Search!
	 ** If nAccess > BILLION then we reset all LRU counters.
	 ** This *should* be sufficient to prevent overflow of the 
	 ** Access counter, but it *is* cutting corners a bit. 
	 */
	iElt = iIndex & MDL_CACHE_MASK;
	if (c->nAccess > BILLION) {
		for (i=1;i<c->nLines;++i) c->pTag[i].nLast = 0;
		c->nAccess -= BILLION;
		c->nAccHigh += 1;
		}
	iVictim = 0;
	for (i=1;i<c->nLines;++i) {
		if (c->pTag[i].nLast < c->pTag[iVictim].nLast) {
			if (!c->pTag[i].nLock) iVictim = i;
			}
		}
	if (!iVictim) {
		/*
		 ** Cache Failure!
		 */
		sprintf(ach,"MDL CACHE FAILURE: cid == %d, no unlocked lines!\n",cid);
		mdlDiag(mdl,ach);
		exit(1);
		}
	iKeyVic = c->pTag[iVictim].iKey;
	/*
	 ** 'pLine' will point to the actual data line in the cache.
	 */
	pLine = &c->pLine[iVictim*c->iLineSize];

	if (iKeyVic >= 0) {
		if (c->iType == MDL_COCACHE) {
			/*
			 ** Flush element since it is valid!
			 */
		        idVic = iKeyVic&c->iIdMask;
			MdlCacheMsg *mesgFlsh = new(&c->iLineSize,0) MdlCacheMsg;
			
			pszFlsh = mesgFlsh->pszBuf;
		        mesgFlsh->ch.cid = cid;
			mesgFlsh->ch.mid = MDL_MID_CACHEFLSH;
			mesgFlsh->ch.id = mdl->idSelf;
			mesgFlsh->ch.iLine = iKeyVic >> c->iInvKeyShift;
			for(i = 0; i < c->iLineSize; ++i)
			    pszFlsh[i] = pLine[i];
			proxyAMdl[idVic].CacheReceive(mesgFlsh);
			}
		/*
		 ** If valid iLine then "unlink" it from the cache.
		 */
		pi = &c->pTrans[iKeyVic & c->iTransMask];
		while (*pi != iVictim) pi = &c->pTag[*pi].iLink;
		*pi = c->pTag[iVictim].iLink;
		}
	c->pTag[iVictim].iKey = iKey;
	c->pTag[iVictim].nLock = 1;
	c->pTag[iVictim].nLast = c->nAccess;
	/*
	 **	Add the modified victim tag back into the cache.
	 ** Note: the new element is placed at the head of the chain.
	 */
	pi = &c->pTrans[iKey & c->iTransMask];
	c->pTag[iVictim].iLink = *pi;
	*pi = iVictim;
	/*
	 ** Figure out whether this is a "new" miss.
	 ** This is for statistics only!
	 */
	if (iKey >= c->nKeyMax) {			/* !!! */
		nKeyNew = iKey+500;
		c->pbKey = (char *) realloc(c->pbKey,nKeyNew);
		assert(c->pbKey != NULL);
		for (i=c->nKeyMax;i<nKeyNew;++i) c->pbKey[i] = 0;
		c->nKeyMax = nKeyNew;
		}
	if (!c->pbKey[iKey]) {
		c->pbKey[iKey] = 1;
		++c->nMin;
		}					/* !!! */
	/*
	 ** At this point 'pLine' is the recipient cache line for the 
	 ** data requested from processor 'id'.
	 */
	
	mesg = proxyAMdl[mdl->idSelf].ckLocal()->waitCache();
	assert(mesg->ch.id == id);
	assert(mesg->ch.cid == cid);
	assert(mesg->ch.mid == MDL_MID_CACHERPL);
	
	char *pszLine = mesg->pszBuf;
	for(i = 0; i < c->iLineSize; i++)
	    pLine[i] = pszLine[i];
	delete mesg;
	
	if (c->iType == MDL_COCACHE && c->init) {
	    /*
	    ** Call the initializer function for all elements in 
	    ** the cache line.
	    */
	    for (i=0;i<c->iLineSize;i+=c->iDataSize) {
		(*c->init)(&pLine[i]);
		}
	    }
	
	return(&pLine[iElt*c->iDataSize]);
	}

extern "C"
void mdlRelease(MDL mdl,int cid,void *p)
{
	CACHE *c = &mdl->cache[cid];
	int iLine,iData;
	
	iLine = ((char *)p - c->pLine) / c->iLineSize;
	/*
	 ** Check if the pointer fell in a cache line, otherwise it
	 ** must have been a local pointer.
	 */
	if (iLine > 0 && iLine < c->nLines) {
		--c->pTag[iLine].nLock;
		assert(c->pTag[iLine].nLock >= 0);
		}
	else {
		iData = ((char *)p - c->pData) / c->iDataSize;
		assert(iData >= 0 && iData < c->nData);
		}
	}

extern "C"
double mdlNumAccess(MDL mdl,int cid)
{
	CACHE *c = &mdl->cache[cid];

	return(c->nAccHigh*1e9 + c->nAccess);
	}


extern "C"
double mdlMissRatio(MDL mdl,int cid)
{
	CACHE *c = &mdl->cache[cid];
	double dAccess = c->nAccHigh*1e9 + c->nAccess;
	
	if (dAccess > 0.0) return(c->nMiss/dAccess);
	else return(0.0);
	}


extern "C"
double mdlCollRatio(MDL mdl,int cid)
{
	CACHE *c = &mdl->cache[cid];
	double dAccess = c->nAccHigh*1e9 + c->nAccess;

	if (dAccess > 0.0) return(c->nColl/dAccess);
	else return(0.0);
	}


extern "C"
double mdlMinRatio(MDL mdl,int cid)
{
	CACHE *c = &mdl->cache[cid];
	double dAccess = c->nAccHigh*1e9 + c->nAccess;

	if (dAccess > 0.0) return(c->nMin/dAccess);
	else return(0.0);
	}

#include "mdl.def.h"
