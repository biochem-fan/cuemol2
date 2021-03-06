//
// CueMol main service for Mozilla/XPCOM interface
//
// $Id: XPCCueMol.cpp,v 1.38 2011/03/10 13:11:55 rishitani Exp $
//
#define __STDC_LIMIT_MACROS
#define __STDC_CONSTANT_MACROS

#include <common.h>

#include "xpcom.hpp"
#include <nsIObserverService.h>

#include "XPCCueMol.hpp"
#include "XPCObjWrapper.hpp"
#include "XPCTimerImpl.hpp"

#ifdef XP_WIN
#  undef NEW_H
#  define NEW_H "new.h"
#  include "XPCNativeWidgetWin.hpp"
#endif
#if defined(XP_MACOSX)
#  include "XPCNativeWidgetCocoa.hpp"
#elif defined(XP_UNIX)
#  include "XPCNativeWidgetGDK.hpp"
#endif

//

#include <qlib/ClassRegistry.hpp>
#include <qlib/EventManager.hpp>
#include <qlib/LByteArray.hpp>
#include <gfx/TextRenderManager.hpp>
#include <qsys/qsys.hpp>
#include <sysdep/sysdep.hpp>

#ifdef HAVE_JAVASCRIPT
#include <jsbr/jsbr.hpp>
#endif

#ifdef HAVE_PYTHON
#include <pybr/pybr.hpp>
#endif


gfx::TextRenderImpl *createTextRender();
void destroyTextRender(void *pTR);

//#define _num_to_str(num) #num
//#define num_to_str(num) _num_to_str(num)
//#pragma message ("new = " num_to_str(new))

#if defined(XP_WIN)
void registerFileType()
{
}
#elif defined(XP_MACOSX)

#include <Carbon/Carbon.h>
#include <ApplicationServices/ApplicationServices.h>

void registerFileType()
{
  CFBundleRef myAppsBundle = CFBundleGetMainBundle();
  if (myAppsBundle==NULL) {
    MB_DPRINTLN("Cannot get main bundle ref");
    return;
  }
  
  //CFURLRef myBundleURL = CFBundleCopyExecutableURL(myAppsBundle);
  CFURLRef myBundleURL = CFBundleCopyBundleURL(myAppsBundle);
  if (myBundleURL==NULL) {
    MB_DPRINTLN("Cannot get main bundle url");
    return;
  }

  FSRef myBundleRef;
  Boolean ok = CFURLGetFSRef(myBundleURL, &myBundleRef);
  CFRelease(myBundleURL);
  if (!ok) {
    MB_DPRINTLN("Cannot get main bundle FSRef");
  }

  OSStatus res = LSRegisterURL(myBundleURL, true);
  MB_DPRINTLN(">>>>> LSRegisterURL %d <<<<<", res);

  unsigned char sbuf[256];
  FSRefMakePath(&myBundleRef, sbuf, sizeof sbuf-1);
  MB_DPRINTLN(">>>>> registerFileType OK (%s)!! <<<<<", sbuf);
}
#else
void registerFileType()
{
}
#endif

namespace render {
  extern bool init();
  extern void fini();
}

namespace molstr {
  extern bool init();
  extern void fini();
}

namespace molvis {
  extern bool init();
  extern void fini();
}

namespace xtal {
  extern bool init();
  extern void fini();
}

namespace molanl {
  extern bool init();
  extern void fini();
}

namespace surface {
  extern bool init();
  extern void fini();
}

namespace symm {
  extern bool init();
  extern void fini();
}

namespace lwview {
  extern bool init();
  extern void fini();
}

namespace anim {
  extern bool init();
  extern void fini();
}

#ifdef HAVE_MDTOOLS_MODULE
namespace mdtools {
  extern bool init();
  extern void fini();
}
#endif

#ifdef HAVE_PSEREAD_MODULE
namespace pseread {
  extern bool init();
  extern void fini();
}
#endif

using namespace xpcom;

NS_IMPL_ISUPPORTS2(XPCCueMol, qICueMol, nsIObserver)

// singleton instance of XPCCueMol
XPCCueMol *gpXPCCueMol;

XPCCueMol::XPCCueMol()
  : m_bInit(false)
{
  gpXPCCueMol = this;
}

XPCCueMol::~XPCCueMol()
{
  gpXPCCueMol = NULL;
  if (m_bInit)
    Fini();
}

//static
XPCCueMol *XPCCueMol::getInstance()
{
  return gpXPCCueMol;
}

//////////

// Quit-app observer

NS_IMETHODIMP
XPCCueMol::Observe(nsISupports* aSubject, const char* aTopic,
                   const PRUnichar* aData)
{
  // dumpWrappers();
  return NS_OK;
}

//////////

NS_IMETHODIMP XPCCueMol::Init(const char *confpath, bool *_retval)
{
  // XXX
  //AddRef();
  
  nsresult rv = NS_OK;
  
  if (m_bInit) {
    LOG_DPRINTLN("XPCCueMol> ERROR: CueMol already initialized.");
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  registerFileType();

  // CueMol2 Application initialization
  qsys::init(confpath);
  sysdep::init();
  //MB_DPRINTLN("---------- qsys::init(confpath) OK");

  // load other modules
  render::init();
  molstr::init();
  molvis::init();
  xtal::init();
  symm::init();
  surface::init();
  molanl::init();
  lwview::init();
  anim::init();

#ifdef HAVE_MDTOOLS_MODULE
  mdtools::init();
#endif

#ifdef HAVE_PSEREAD_MODULE
  pseread::init();
#endif

  initTextRender();
  MB_DPRINTLN("---------- initTextRender() OK");

  // setup timer
  qlib::EventManager::getInstance()->initTimer(new XPCTimerImpl);

  // setup quit-app observer
  nsCOMPtr<nsIObserverService> obs = do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = obs->AddObserver(this, "xpcom-shutdown", PR_FALSE);
  // rv = obs->AddObserver(this, "quit-application", PR_FALSE);
  NS_ENSURE_SUCCESS(rv, rv);

  MB_DPRINTLN("---------- setup observers OK");

#ifdef HAVE_JAVASCRIPT
  // load internal JS module
  jsbr::init();
  //MB_DPRINTLN("---------- jsbr::init() OK");
#endif

#ifdef HAVE_PYTHON
  // load python module
  pybr::init();
  MB_DPRINTLN("---------- setup PYBR OK");
#endif

  MB_DPRINTLN("XPCCueMol> CueMol initialized.");
  m_bInit = true;
  *_retval = PR_TRUE;

  return NS_OK;
}

NS_IMETHODIMP XPCCueMol::Fini()
{
  int i;

#ifdef HAVE_PYTHON
  // unload python module
  pybr::fini();
  MB_DPRINTLN("=== pybr::fini() OK ===");
#endif

#ifdef HAVE_JAVASCRIPT
  jsbr::fini();
  MB_DPRINTLN("=== jsbr::fini() OK ===");
#endif

  //finiTextRender();
  destroyTextRender(m_pTR);

  MB_DPRINTLN("=== Cleaning up the unreleased wrappers... ===");
  for (i=0; i<m_pool.size(); ++i) {
    if (m_pool[i].ptr) {
      XPCObjWrapper *pwr = m_pool[i].ptr;
      MB_DPRINTLN("Unreleased wrapper: %d %p", i, pwr);
      pwr->detach();
    }
  }
  MB_DPRINTLN("=== Done ===");

  // cleanup timer
  qlib::EventManager::getInstance()->finiTimer();

  if (!m_bInit) {
    LOG_DPRINTLN("XPCCueMol> ERROR: CueMol not initialized.");
    return NS_ERROR_NOT_INITIALIZED;
  }

#ifdef HAVE_PSEREAD_MODULE
  pseread::fini();
#endif

#ifdef HAVE_MDTOOLS_MODULE
  mdtools::fini();
#endif

  anim::fini();
  lwview::fini();
  molanl::fini();
  surface::fini();
  symm::fini();
  xtal::fini();
  molvis::fini();
  molstr::fini();
  render::fini();

  // CueMol-App finalization
  sysdep::fini();
  qsys::fini();

  MB_DPRINTLN("XPCCueMol> CueMol finalized.");
  m_bInit = false;
  return NS_OK;
}

bool XPCCueMol::initTextRender()
{
  //ThebesTextRender *pTTR = new ThebesTextRender;
  //pTTR->setupFont(12.0, "sans-serif", "normal", "normal");
  //pTTR->setupFont(20.0, "Times New Roman", FONT_STYLE_NORMAL, FONT_WEIGHT_NORMAL);

  gfx::TextRenderImpl *pTR = createTextRender();
  gfx::TextRenderManager *pTRM = gfx::TextRenderManager::getInstance();
  pTRM->setImpl(pTR);

  m_pTR = pTR;
  return true;
}

/* boolean isInitialized (); */
NS_IMETHODIMP XPCCueMol::IsInitialized(bool *_retval)
{
  *_retval = m_bInit;
  return NS_OK;
  //return NS_ERROR_NOT_IMPLEMENTED;
}


using qlib::ClassRegistry;

NS_IMETHODIMP XPCCueMol::GetService(const char *svcname,
                                    qIObjWrapper **_retval)
{
  ClassRegistry *pMgr = ClassRegistry::getInstance();
  if (pMgr==NULL) {
    LOG_DPRINTLN("XPCCueMol> ERROR: CueMol not initialized.");
    return NS_ERROR_NOT_INITIALIZED;
  }

  qlib::LDynamic *pobj;
  try {
    pobj = pMgr->getSingletonObj(svcname);
  }
  catch (const qlib::LException &e) {
    LOG_DPRINTLN("GetService> Caught exception <%s>", typeid(e).name());
    LOG_DPRINTLN("GetService> Reason: %s", e.getMsg().c_str());
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  catch (...) {
    LOG_DPRINTLN("Caught unknown exception");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  qlib::LScriptable *pscr = dynamic_cast<qlib::LScriptable *>(pobj);
  if (pscr==NULL) {
    LOG_DPRINTLN("GetService> Fatal error dyncast to scriptable failed!!");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  XPCObjWrapper *pWrap = createWrapper();
  pWrap->setWrappedObj(pscr);

  *_retval = pWrap;
  NS_ADDREF((*_retval));
  MB_DPRINTLN("getService(%s) OK: %p", svcname, pscr);
  return NS_OK;
}

NS_IMETHODIMP XPCCueMol::CreateObj(const char *clsname,
                                   qIObjWrapper **_retval)
{
  ClassRegistry *pMgr = ClassRegistry::getInstance();
  if (pMgr==NULL) {
    LOG_DPRINTLN("XPCCueMol> ERROR: CueMol not initialized.");
    return NS_ERROR_NOT_INITIALIZED;
  }

  qlib::LDynamic *pobj;
  try {
    qlib::LClass *pcls = pMgr->getClassObj(clsname);
    if (pcls==NULL)
      MB_THROW(qlib::NullPointerException, "null");
    pobj = pcls->createScrObj();
  }
  catch (const qlib::LException &e) {
    LOG_DPRINTLN("CreateObj> Caught exception <%s>", typeid(e).name());
    LOG_DPRINTLN("CreateObj> Reason: %s", e.getMsg().c_str());
    return NS_ERROR_NOT_IMPLEMENTED;
  }
  catch (...) {
    LOG_DPRINTLN("CreateObj> Caught unknown exception");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  qlib::LScriptable *pscr = dynamic_cast<qlib::LScriptable *>(pobj);
  if (pscr==NULL) {
    LOG_DPRINTLN("CreateObj> Fatal error dyncast to scriptable failed!!");
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  XPCObjWrapper *pWrap = createWrapper();
  pWrap->setWrappedObj(pscr);

  *_retval = pWrap;
  NS_ADDREF((*_retval));
  MB_DPRINTLN("XPCCueMol> createObj(%s) OK: %p", clsname, pscr);
  return NS_OK;
}

XPCObjWrapper *XPCCueMol::createWrapper()
{
  int nind;
  if (m_freeind.size()>0) {
    // reuse allocated entry
    nind = m_freeind.back();
    m_freeind.pop_back();
  }
  else {
    // append to the last entry
    nind = m_pool.size();
    m_pool.push_back(Cell());
  }

  MB_ASSERT(m_pool[nind].ptr==NULL);

  XPCObjWrapper *pWr = new XPCObjWrapper(this, nind);
  m_pool[nind].ptr = pWr;

#ifdef MB_DEBUG
  m_pool[nind].dbgmsg = LString();
#endif

  return pWr;
}

void XPCCueMol::notifyDestr(int nind)
{
  m_pool[nind].ptr = NULL;

#ifdef MB_DEBUG
  m_pool[nind].dbgmsg = LString();
#endif

  m_freeind.push_back(nind);
}

void XPCCueMol::setWrapperDbgMsg(int nind, const char *dbgmsg)
{
#ifdef MB_DEBUG
  if (m_pool[nind].ptr==NULL)
    return; // unused wrapper
  m_pool[nind].dbgmsg = dbgmsg;
#endif
}

void XPCCueMol::dumpWrappers() const
{
  MB_DPRINTLN("=== Unreleased wrappers... ===");
  for (int i=0; i<m_pool.size(); ++i) {
    if (m_pool[i].ptr) {
      XPCObjWrapper *pwr = m_pool[i].ptr;
      LScriptable *pscr = pwr->getWrappedObj();
      //MB_DPRINTLN("Wrapper: %d %p (%s)", i, pwr, typeid(*pscr).name());
      MB_DPRINTLN("Wrapper: %d %p", i, pwr);
#ifdef MB_DEBUG
      MB_DPRINT("   MSG: ", m_pool[i].dbgmsg.c_str() );
      MB_DPRINTLN( m_pool[i].dbgmsg.c_str() );
      MB_DPRINTLN("----------");
#endif
    }
  }
  MB_DPRINTLN("=== Done ===");
}

/* void getErrMsg (out string confpath); */
NS_IMETHODIMP XPCCueMol::GetErrMsg(char **_retval )
{
  nsAutoCString nsstr(m_errMsg.c_str());
  *_retval = ToNewCString(nsstr);

  return NS_OK;
}

///////////////////////

/* qINativeWidget createNativeWidget (); */
NS_IMETHODIMP XPCCueMol::CreateNativeWidget(qINativeWidget **_retval )
{
  XPCNativeWidget *pWgt = NULL;

#ifdef XP_WIN
  pWgt = new XPCNativeWidgetWin();
#endif

#if defined(XP_MACOSX)
  pWgt = new XPCNativeWidgetCocoa();
#elif defined(XP_UNIX)
  pWgt = new XPCNativeWidgetGDK();
#endif

  if (pWgt==NULL) {
    LOG_DPRINTLN("XPCCueMol> FATAL ERROR: cannot create native widget");
    return NS_ERROR_FAILURE;
  }

  *_retval = pWgt;
  NS_ADDREF((*_retval));
  MB_DPRINTLN("XPCCueMol> createNativeWidget OK: %p", pWgt);
  return NS_OK;
}

/////////////////////////////////////////////////////////////////////
// ByteArray operation

/* ACString convBAryToStr (in qIObjWrapper aObj); */
NS_IMETHODIMP XPCCueMol::ConvBAryToStr(qIObjWrapper *aObj, nsACString & _retval )
{
  XPCObjWrapper *pp = dynamic_cast<XPCObjWrapper *>(aObj);
  if (pp==NULL) {
    LOG_DPRINTLN("ConvBAryToStr> FATAL ERROR: unknown wrapper type (unsupported)");
    return NS_ERROR_FAILURE;
  }
  
  qlib::LByteArrayPtr *pByteAry = dynamic_cast<qlib::LByteArrayPtr *>( pp->getWrappedObj() );
  if (pByteAry==NULL) {
    LOG_DPRINTLN("ConvBAryToStr> FATAL ERROR: arg1 is not ByteArray");
    return NS_ERROR_FAILURE;
  }
  
  int nlen = pByteAry->get()->getSize();
  _retval.SetLength(nlen);

  if (_retval.Length() != nlen)
    return NS_ERROR_OUT_OF_MEMORY;

  char *ptr = _retval.BeginWriting();
  const char *pBuf = (const char *) pByteAry->get()->data();

  for (int i=0; i<nlen; ++i)
    ptr[i] = pBuf[i];

  return NS_OK;
}

/* qIObjWrapper createBAryFromStr (in ACString aString, in PRUint32 aCount); */
NS_IMETHODIMP XPCCueMol::CreateBAryFromStr(const nsACString & aString, qIObjWrapper **_retval )
{
  nsresult rv = NS_OK;

  PRUint32 nlen = aString.Length();

  qlib::LByteArray *pNewObj = new qlib::LByteArray(nlen);
  if (nlen>0) {
    const char *ptr = aString.BeginReading();
    char *pBuf = (char *)(pNewObj->data());
    for (int i=0; i<nlen; ++i)
      pBuf[i] = ptr[i];
  }
  
  XPCObjWrapper *pWrap = createWrapper();
  pWrap->setWrappedObj(pNewObj);
  *_retval = pWrap;
  NS_ADDREF((*_retval));

  return NS_OK;
}


/* qIObjWrapper createBAryFromIStream (in nsIInputStream aInputStream); */
NS_IMETHODIMP XPCCueMol::CreateBAryFromIStream(nsIInputStream *aInputStream, qIObjWrapper **_retval )
{
  nsresult rv = NS_OK;

  uint64_t nlen;
  rv = aInputStream->Available(&nlen);
  NS_ENSURE_SUCCESS(rv, rv);

  qlib::LByteArray *pNewObj = new qlib::LByteArray(nlen);
  if (nlen>0) {
    char *pBuf = (char *)(pNewObj->data());
    PRUint32 nres;
    rv = aInputStream->Read(pBuf, nlen, &nres);
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  XPCObjWrapper *pWrap = createWrapper();
  pWrap->setWrappedObj(pNewObj);
  *_retval = pWrap;
  NS_ADDREF((*_retval));

  return NS_OK;
}

///////////////////////

NS_IMETHODIMP XPCCueMol::Test(nsISupports *arg)
{
  //*((int *) 0) = 100;
  dumpWrappers();
  return NS_OK;
}

