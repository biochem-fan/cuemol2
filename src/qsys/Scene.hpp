// -*-Mode: C++;-*-
//
// Scene: root object of the document
//

#ifndef QSYS_SCENE_HPP_INCLUDED
#define QSYS_SCENE_HPP_INCLUDED

#include "qsys.hpp"

#include <qlib/LScrObjects.hpp>
#include <qlib/LScrSmartPtr.hpp>
#include <qlib/ObjectManager.hpp>
#include <qlib/LScrCallBack.hpp>
#include <qlib/LPropEvent.hpp>

#include <gfx/AbstractColor.hpp>

#include "Object.hpp"
#include "ObjectEvent.hpp"
#include "View.hpp"
#include "Camera.hpp"
#include "UndoManager.hpp"

class Scene_wrap;

namespace qlib {
  class InStream;
  class OutStream;
  class LByteArray;
}

namespace gfx {
  class DisplayContext;
}

// interpreter bound to this scene
namespace jsbr {
  class Interp;
}

using qlib::LString;

namespace qsys {

  class SceneEventCaster;
  class SceneEventListener;
  class SceneEvent;
  class ObjLoadEditInfo;
  class StyleMgr;
  class AnimMgr;

  class QSYS_API Scene :
    public qlib::LNoCopyScrObject,
    public qlib::LUIDObject,
    public qlib::LPropEventListener,
    public ObjectEventListener,
    public RendererEventListener
  {
    MC_SCRIPTABLE;

    friend class ObjLoadEditInfo;
    friend class ::Scene_wrap;
    
    ////////////////////////////////////////////////////////////
    // Typedefs

  private:
    typedef std::map<qlib::uid_t, ObjectPtr> data_t;

    typedef std::map<qlib::uid_t, RendererPtr> rendtab_t;

    typedef std::map<qlib::uid_t, ViewPtr> viewtab_t;

    typedef std::map<LString, CameraPtr> camtab_t;

    typedef qlib::LNoCopyScrObject super_t;
    // typedef qlib::LSimpleCopyScrObject super_t;

  public:
    typedef data_t::const_iterator ObjIter;
    typedef viewtab_t::const_iterator ViewIter;
    typedef camtab_t::const_iterator CameraIter;
    
    ////////////////////////////////////////////////////////////
    // Implementation: Data

  private:

    /// Name of this scene
    LString m_name;

    /// Source of this scene
    ///  (currently only supports local path name)
    LString m_source;
    LString m_sourceType;

    /// Background color
    gfx::ColorPtr m_pBgColor;

    /// UID of this scene
    qlib::uid_t m_nUID;

    /// script event handler: onload
    LString m_scrOnLoadEvent;

    ////////////////////////////////////////////////////////////

    /// Objects in this scene
    data_t m_data;

    /// Renderers in this scene
    rendtab_t m_rendtab;

    /// Views for this scene
    viewtab_t m_viewtab;

    /// Cameras in this scene
    camtab_t m_camtab;

    /// Active view's ID
    qlib::uid_t m_nActiveViewID;
    
    SceneEventCaster *m_pEvtCaster;

    /// Undo/Redo manager for this scene
    UndoManager m_undomgr;

    /// Cached style mgr ptr
    StyleMgr *m_pStyleMgr;

    /// Interpreter context for this scene
    jsbr::Interp *m_pInterp;

    /// Scene loading flag : true when scene is loading from storage
    ///  (used to avoid event generation on the loading phase)
    bool m_bLoading;

  public:

    ////////////////////////////////////////////////////////////

    /// constructor
    Scene();

    /// destructor
    virtual ~Scene();

    /// initialization
    void init();

    ////////////////////////////////////////////////////////////
    // Properties

    /// Name of the scene (read only from UI)
    const LString &getName() const { return m_name; }
    void setName(const LString &name);
  
    /// Get modified flag
    bool isModified() const;

    /// check if the scene is just created and empty
    bool isJustCreated() const;

    /// Get UID of this scene (readonly)
    qlib::uid_t getUID() const { return m_nUID; }

    const gfx::ColorPtr &getBgColor() const { return m_pBgColor; }
    void setBgColor(const gfx::ColorPtr &r) {
      setUpdateFlag();      
      m_pBgColor = r;
    }

    /// get source path of this scene
    const LString &getSource() const {
      return m_source;
    }
    /// set source path of this scene
    void setSource(const LString &name);

    /// get source path of this scene
    const LString &getSourceType() const {
      return m_sourceType;
    }
    /// set source path of this scene
    void setSourceType(const LString &name) {
      m_sourceType = name;
    }

    /// Return the base path for this scene (directory containing source file)
    LString getBasePath() const;
    /// Resolve relative path name using base_path of this scene
    LString resolveBasePath(const LString &aLocalFile) const;

    /// Convert rel and abs of src and alt_src paths
    std::pair<LString, LString> convSrcPaths(const LString &aSrc,
                                             const LString &aAltSrc) const;
    
    std::pair<LString, LString> setPathsToNode(const LString &aSrc,
                                               const LString &aAltSrc,
                                               qlib::LDom2Node *pNode) const;

    ////////////////////////////////////////////////////////////
    // Script interpreter operations

    jsbr::Interp *getInterp() const { return m_pInterp; }

    bool execJSFile(const LString &scr);

    ////////////////////////////////////////////////////////////
    // Object manager

    /// Add a new object to this scene
    ///  (object should not belong to any scene.)
    bool addObject(ObjectPtr pobj);

    /// Destroy object by UID
    bool destroyObject(qlib::uid_t uid);
    /// Destroy all objects
    void destroyAllObjects();

    /// Get object by UID
    ObjectPtr getObject(qlib::uid_t uid) const;
    /// Get (first) object by name
    ObjectPtr getObjectByName(const LString &name) const;

    int getAllObjectUIDs(qlib::UIDList &uids) const;

    ObjIter beginObj() const { return m_data.begin(); }
    ObjIter endObj() const { return m_data.end(); }

    /// get the number of object in the scene
    int getObjectCount() const { return m_data.size(); }

    // Scripting interface wrapper
    LString getObjUIDList() const;

    /// Get current data structure in JSON string representation.
    // Without renderer group / used from UI&javascript
    LString getObjectTreeJSON() const;

    /// Get scene information in JSON format (w/ rend groups)
    LString getSceneDataJSON(bool bGroup = true) const;

    ////////////////////////////////////////////////////////////
    // View manager

    /// Create new view
    ViewPtr createView();

    /// Get view object by UID
    ViewPtr getView(qlib::uid_t uid) const;

    /// Get (first) view object by name
    ViewPtr getViewByName(const LString &name) const;

    /// Destroy view by UID
    bool destroyView(qlib::uid_t uid);

    ViewIter beginView() const { return m_viewtab.begin(); }
    ViewIter endView() const { return m_viewtab.end(); }

    /// Get current view count (size)
    int getViewCount() const { return m_viewtab.size(); }

    void setActiveViewID(qlib::uid_t uid);

    ViewPtr getActiveView() const;

    //////////
    // Scripting interface wrapper
    LString getViewUIDList() const;
    // qlib::LVarArray getViewArray() const;

    ////////////////////////////////////////////////////////////
    // Camera manager

  private:
    // /// Load camera setting from file (impl/throw excp)
    // CameraPtr loadCameraImpl(const LString &aLocalFile) const;

    /// Register new camera setting (impl/without event firing)
    /// This method overwrite if the camera with the same name exists.
    void setCameraImpl(const LString &name, CameraPtr r);

  public:
    /// Register new camera setting (with event firing)
    /// This method overwrite if the camera with the same name exists.
    void setCamera(const LString &name, CameraPtr r);

    /// Get the copy of the camera by name
    CameraPtr getCamera(const LString &nm) const;

    /// Get the reference of the camera by name
    CameraPtr getCameraRef(const LString &nm) const;

    /// check whether the camera exists or not
    bool hasCamera(const LString &nm) const;

    /// Destroy camera by name
    bool destroyCamera(const LString &nm);

    CameraIter beginCamera() const { return m_camtab.begin(); }
    CameraIter endCamera() const { return m_camtab.end(); }

    int getCameraCount() const { return m_camtab.size(); }

    /// Save view setting to the camera (view --> camera)
    bool saveViewToCam(qlib::uid_t viewid, const LString &nm);

    /// Apply camera setting to the view (camera --> view)
    void loadViewFromCam(qlib::uid_t viewid, const LString &nm) {
      setCamToViewAnim(viewid, nm, false);
    }

    /// Apply camera setting to the view with anim (camera --> view)
    void setCamToViewAnim(qlib::uid_t viewid, const LString &camname, bool bAnim);

    /// retrieve camera information by JSON (for UI)
    LString getCameraInfoJSON() const;

    /// Load camera from the local file
    CameraPtr loadCamera(const LString &filename) const;

    /// Save camera to the local file
    bool saveCameraTo(const LString &name, const LString &filename) const;

    ////////////////////////////////////////////////////////////
    // Renderer manager (cache for display)

    //RendererPtr createRenderer(const LString &tpnm);
    //RendererPtr getRenderer(qlib::uid_t uid) const;
    //bool destroyRenderer(qlib::uid_t uid);

    bool addRendCache(RendererPtr rrend);
    bool removeRendCache(RendererPtr rrend);

    //RendIter beginRend() const { return m_rendtab.begin(); }
    //RendIter endRend() const { return m_rendtab.end(); }

    void display(DisplayContext *);
    void processHit(DisplayContext *);

    RendererPtr getRendByName(const LString &nm) const;

  private:
    void displayRendImpl(DisplayContext *pdc, ObjectPtr pObj, RendererPtr pRend);
    
  public:
    ////////////////////////////////////////////////////////////
    // Undo/Redo

    UndoManager *getUndoMgr() {
      return &m_undomgr;
    }

    bool undo(int nstep);

    bool redo(int nstep);

    void startUndoTxn(const LString &descr) {
      m_undomgr.startTxn(descr);
    }
    void rollbackUndoTxn() {
      m_undomgr.rollbackTxn();
    }
    void commitUndoTxn();

    bool isUndoable() const {
      return m_undomgr.isUndoable();
    }
    bool isRedoable() const {
      return m_undomgr.isRedoable();
    }

    LString getUndoDesc(int n) const {
      LString desc;
      m_undomgr.getUndoDesc(n, desc);
      return desc;
    }
    LString getRedoDesc(int n) const {
      LString desc;
      m_undomgr.getRedoDesc(n, desc);
      return desc;
    }

    int getUndoSize() const {
      return m_undomgr.getUndoSize();
    }
    int getRedoSize() const {
      return m_undomgr.getRedoSize();
    }

    // clear undo/redo data for scripting interface
    // this method fires event (sceneundoinfo)
    void clearUndoDataScr();

    ////////////////////////////////////////////////
    // Animation management
  private:
    qlib::LScrSp<AnimMgr> m_pAnimMgr;
  public:
    qlib::LScrSp<AnimMgr> getAnimMgr() const { return m_pAnimMgr; }

    ////////////////////////////////////////////////
    // Event related operations
  private:
    bool m_bUpdateRequired;

  public:
    void setUpdateFlag() { m_bUpdateRequired = true; }
    void clearUpdateFlag() { m_bUpdateRequired = false; }
    void checkAndUpdate();

    int addListener(SceneEventListener *pL);
    bool removeListener(SceneEventListener *pL);

    int addListener(qlib::LSCBPtr scb);
    bool removeListener(int nid);

    void fireSceneEvent(SceneEvent &ev);

    /// Unloading event handler
    //  (Called by framework before the unloading this scene)
    virtual void unloading();

    //////////
    // for property event propagation
    virtual qlib::uid_t getRootUID() const;
    /// Property changed event handler for the scene properties
    virtual void propChanged(qlib::LPropEvent &ev);

    /// object changed event
    virtual void objectChanged(ObjectEvent &ev);

    /// renderer changed event
    virtual void rendererChanged(RendererEvent &ev);

    ////////////////////////////////////////////////////////////
    // Serialization/Deserialization

    ///
    /// Serialize this scene to the stream
    ///
    virtual void writeTo2(qlib::LDom2Node *pNode) const;

    ///
    /// Serialize this scene to the localfile
    ///
    virtual void readFrom2(qlib::LDom2Node *pNode);

  private:
    // writeTo2/readFrom2 impl for specific settings

    /// WriteTo2() impl for style settings
    void stylesWriteTo(qlib::LDom2Node *pNode) const;
    /// WriteTo2() impl for camera settings
    void camerasWriteTo(qlib::LDom2Node *pNode) const;

    void objectReadFrom(qlib::LDom2Node *pNode);
    void cameraReadFrom(qlib::LDom2Node *pNode);
    void stylesReadFrom(qlib::LDom2Node *pNode);

  private:
    qlib::LDom2Node *m_pQscOpts;

  public:
    void setQscOpts(qlib::LDom2Node *ptree);
    qlib::LDom2Node *getQscOpts() const { return m_pQscOpts; }

    ////////////////////////////////////////////////////////////
    // Misc operations

    /// Cleanup all contents of this scene (including views)
    void clearAll();

    /// Cleanup all contents of this scene (other than views)
    void clearAllData();

    /// Scriptable version, cleanup all contents of this scene
    ///  (the same as clearAllData() except firing event)
    void clearAllDataScr();

    /// Show debug dump to the debug stream
    void dump() const;

  private:

    ////////////////////////////////////////////////////////////
    // Implementation

    bool registerObjectImpl(ObjectPtr robj);

  };

}

#endif // QSYS_SCENE_HPP_INCLUDE_
