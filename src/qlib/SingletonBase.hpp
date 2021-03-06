// -*-Mode: C++;-*-
//
// Helper class for Singleton implementation
//
// $Id: SingletonBase.hpp,v 1.2 2008/12/25 14:20:37 rishitani Exp $

#ifndef SINGLETON_BASE_HPP_INCLUDE_
#define SINGLETON_BASE_HPP_INCLUDE_

namespace qlib {

  class LClass;

  template <class _Type>
  class SingletonBase
  {
  public:
    static inline _Type *getInstance() {
      return s_pInst;
    }

    static inline bool init()
    {
      s_pInst = MB_NEW _Type;
      if (!s_pInst) return false;
      return true;
    }

    static inline void fini()
    {
      if (s_pInst!=NULL) {
        delete s_pInst;
        s_pInst = NULL;
      }
      else {
        MB_DPRINTLN("SingletonBase> Singleton %s already finalized or not initialized.",
                    typeid(_Type).name());
      }
    }
    
  private:
    static _Type *s_pInst;
  };

}

// for static object implementation
#define SINGLETON_BASE_IMPL(classname) \
template <> MB_PUBLIC classname *qlib::SingletonBase<classname>::s_pInst = 0;

#endif

