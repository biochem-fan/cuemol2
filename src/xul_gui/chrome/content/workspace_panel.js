// -*-Mode: C++;-*-
//
// workspace-panel.js: workspace sidepanel implementation
//
// $Id: workspace_panel.js,v 1.39 2011/04/24 15:05:49 rishitani Exp $
//

if (!("workspace" in cuemolui.panels)) {

( function () {

var ws = cuemolui.panels.workspace = new Object();

// panel's ID
ws.id = "workspace-panel";
  
ws.collapsed = false;
ws.command_id = "menu-workspace-panel-toggle";
  
ws._callbackID = null;
ws.mTgtSceneID = -1;

// create TreeView object
ws.mViewObj = new cuemolui.TreeView(window, "objectTree");

// Setup tree-view's event handlers
ws.mViewObj.clickHandler = function (ev, row, col) {
  try { ws.onTreeItemClick(ev, row, col); } catch (e) { debug.exception(e); }
};
ws.mViewObj.twistyClickHandler = function (row, node) {
  try { ws.onTwistyClick(row, node); } catch (e) { debug.exception(e); }
};
ws.mViewObj.canDropHandler = function (tind, ori, dt)
{
  return ws.canDrop(tind, ori, dt);
};
ws.mViewObj.dropHandler = function (elem, ori, dt)
{
  ws.drop(elem, ori, dt);
};

const ITEM_DROP_TYPE = "application/x-cuemol-workspace-item";

// Setup DOM event handlers
window.addEventListener("load", function(){ws.onLoad();}, false);
window.addEventListener("unload", function() {ws.onUnLoad();}, false);

ws.mSceneCtxtMenuID = "wspcPanelSceneCtxtMenu";
ws.mObjCtxtMenuID = "wspcPanelObjCtxtMenu";
ws.mRendCtxtMenuID = "wspcPanelRendCtxtMenu";

ws._nodes = null;

//////////////////////////
// methods

ws.createRendNodeData = function (aRend, aParent)
{
  let rnode = new Object();
  rnode.orig_name = aRend.name;
  rnode.type_name = aRend.type;
  // display name
  rnode.name = aRend.name + " ("+aRend.type+")";
  //rnode.values = { object_vis: aRend.visible };
  
  let rvis = "invisible";
  if (aRend.visible) {
    if (aParent.visible)
      rvis = "visible";
    else
      rvis = "disabled";
  }
  rnode.props = {
  object_vis: rvis,
  object_lck: (aRend.locked?"locked":"unlocked")
  };
  
  rnode.obj_id = aRend.ID;
  rnode.parent_id = aParent.ID;

  return rnode;
};

// Camera
ws.createCameraNodeData = function (aScene)
{
  var data, node;
  let json_str = aScene.getCameraInfoJSON();

  dd("CameraInfo json: "+json_str);
  data = JSON.parse(json_str);
  
  node = new Object();
  node.name = "Camera";
  node.collapsed = true;
  node.obj_id = "camera_topnode";
  node.menu_id = "wspcPanelCameraCtxtMenu";
  node.type = "cameraRoot";
  let nlen = data.length;
  if (nlen>0) {
    node.childNodes = new Array();
    for (i=0; i<nlen; ++i) {
      let rnode = new Object();
      rnode.name = data[i].name;
      rnode.obj_id = data[i].name;
      rnode.menu_id = "wspcPanelCameraCtxtMenu";
      rnode.type = "camera";
      rnode.props = {
      object_vis: (data[i].src==""?"":"linked")
      };
      node.childNodes.push(rnode);
    }
  }

  return node;
};

// Styles
ws.createStyleNodeData = function (aScene)
{
  var i;
  let stylem = cuemol.getService("StyleManager");
  
  let json_str = stylem.getStyleSetsJSON(0);
  let styles = JSON.parse(json_str);
  
  json_str = stylem.getStyleSetsJSON(aScene.uid);
  styles = styles.concat( JSON.parse(json_str) );
  
  // dd("Workspace Panel: style="+debug.dumpObjectTree(styles, 1));
  // dd("Workspace Panel: style(sce)="+debug.dumpObjectTree(scene_styles, 1));
  
  node = new Object();
  node.name = "Styles";
  node.collapsed = true;
  node.obj_id = "styles_topnode";
  node.menu_id = "wspcStyleCtxtMenu";
  node.type = "styleRoot";
  nlen = styles.length;
  if (nlen>0) {
    node.childNodes = new Array();
    for (i=0; i<nlen; ++i) {
      let rnode = new Object();
      rnode.name = styles[i].name;
      if (rnode.name=="")
        rnode.name = "(anonymous)";
      //rnode.obj_id = styles[i].name;
      //rnode.uid = styles[i].uid;
      rnode.obj_id = styles[i].uid;
      rnode.scene_id = styles[i].scene_id;
      rnode.menu_id = "wspcStyleCtxtMenu";
      rnode.type = "style";
      //rnode.src = styles[i].src;
      
      let readonly = styles[i].readonly;
      let lck=false;
      if (styles[i].readonly)
        lck = true;
      if (rnode.scene_id==0)
        lck = true;
      
      rnode.props = {
      object_lck: (lck?"locked":"unlocked"),
      object_vis: (styles[i].src==""?"":"linked")
      };
      
      node.childNodes.push(rnode);
    }
  }

  return node;
}

ws.syncContents = function (scid)
{
  let scene = cuemol.getScene(scid);
  if (!scene) {
    dd("Workspace panel: syncContents failed, invalid scene: "+scid);
    return;
  }

  //let json_str = scene.getObjectTreeJSON();
  let json_str = scene.getSceneDataJSON();
  // dd("WS> ObjTree json: "+json_str);
  let data;
  try {
    data = JSON.parse(json_str);
  }
  catch (e) {
    dd("error : "+json_str);
    require("debug_util").exception(e);
    return;
  }

  let nodes = new Array();
  let i, nlen = data.length;

  // scene
  let sc = data[0];
  let node = new Object();
  node.name = "Scene: "+sc.name;
  node.menu_id = this.mSceneCtxtMenuID;
  node.obj_id = sc.ID;
  node.type = "scene";
  node.props = { object_name: "noindent" };
  nodes.push(node);

  // objects and renderers
  for (i=1; i<nlen; ++i) {
    let obj = data[i];
    node = new Object();
    node.name = obj.name + " ("+obj.type+")";
    //node.values = { object_vis: obj.visible };
    node.props = {
      object_vis: (obj.visible?"visible":"invisible"),
      object_lck: (obj.locked?"locked":"unlocked")
    };
    node.collapsed = obj.ui_collapsed; //false;
    node.menu_id = this.mObjCtxtMenuID;
    node.obj_id = obj.ID;
    node.type = "object";
    // dd("WS> build node="+debug.dumpObjectTree(node,2));
    if (obj.rends && obj.rends.length>0) {
      node.childNodes = new Array();
      var j, njlen = obj.rends.length;
      for (j=0; j<njlen; ++j) {
        let rend = obj.rends[j];
        let rnode = this.createRendNodeData(rend, obj);

        if (rend.type=="*group") {
	  dd("*** group rnode = "+debug.dumpObjectTree(rend));
          rnode.menu_id = "wspcPanelRendGrpCtxtMenu";
          rnode.type = "rendGroup";
          // rnode.collapsed = false;
	  rnode.collapsed = rend.ui_collapsed;
          rnode.props["object_name"] = "group";
          let grpsz = rend.childNodes ? rend.childNodes.length : 0;
	  rnode.childNodes = new Array();
	  for (let k=0; k<grpsz; ++k) {
	    let cn = this.createRendNodeData(rend.childNodes[k], rend);
	    cn.menu_id = this.mRendCtxtMenuID;
	    cn.type = "renderer";
	    rnode.childNodes.push(cn);
	  }
        }
        else {
          rnode.menu_id = this.mRendCtxtMenuID;
          rnode.type = "renderer";
        }

        node.childNodes.push(rnode);
      }
    }
    nodes.push(node);
  }

  //dd("Workspace Panel: syncContents data="+debug.dumpObjectTree(data, 1));
  //dd("Workspace Panel: syncContents nodes="+debug.dumpObjectTree(nodes, 1));

  // Cameras
  nodes.push( this.createCameraNodeData(scene) );

  // Styles
  nodes.push( this.createStyleNodeData(scene) );

  // dd("Workspace Panel: cameraInfo="+debug.dumpObjectTree(data, 1));
  //dd("Workspace Panel: syncContents nodes="+debug.dumpObjectTree(nodes, 1));

  // Setup nodes for the tree view
  this._nodes = nodes;
  this.mViewObj.setData(this._nodes);
  this.mViewObj.restoreOpenState(scid);
  this.mViewObj.buildView();
}

ws.syncContentsPropChg = function (srcUID, propname)
{
  var node = this.findNodeByObjId(srcUID);
  
  var src;
  var type2;
  if (node.type=="object") {
    src = cuemol.getObject(srcUID);
    type2 = src._wrapped.getClassName();
  }
  else if (node.type=="renderer"||node.type=="rendGroup") {
    src = cuemol.getRenderer(srcUID);
    type2 = src.type_name;
    //dd(debug.dumpObjectTree(src));
    //type2 = src._wrapped.getProp("type_name");
  }
  else if (node.type=="scene") {
    src = cuemol.getScene(srcUID);
    type2 = "";
  }
  else
    return;

  if (propname=="visible") {
    var newval = src.visible;
    if (node.type=="renderer") {
      node.props.object_vis = newval?"visible":"invisible";
    }
    else {
      node.props.object_vis = newval?"visible":"invisible";
      const nch = (node.childNodes)? node.childNodes.length : 0;
      for (var i=0; i<nch; ++i) {
        var rnode = node.childNodes[i];
        if (newval) {
          if (rnode.props.object_vis=="disabled")
            rnode.props.object_vis = "visible";
        }
        else {
          if (rnode.props.object_vis=="visible")
            rnode.props.object_vis = "disabled";
        }
      }
      this.mViewObj.invalidate();
      return;
    }
  }
  else if (propname=="locked") {
    var newval = src.locked;
    node.props.object_lck = newval?"locked":"unlocked";
    dd("syncContPropChg> locked, newval="+newval+", props="+node.props.object_lck);
  }
  else if (propname=="name") {
    if (node.type=="scene") {
      node.name = "Scene: "+ src.name;
    }
    else {
      let newval = src.name + " ("+type2+")";
      dd("SyncContPropChg name UID: "+srcUID+", name="+newval);
      node.orig_name = src.name;
      node.name = newval;
    }
  }
  else
    return;

  //this.mViewObj._tvi.invalidate();
  this.mViewObj.updateNode( function(elem) {
    return (elem.obj_id==srcUID)?true:false;
  } );
}

ws.removeObject = function (aId)
{
  dd("WS.removeObject ID="+aId);
  let irow = this.mViewObj.getSelectedRow();

  this.mViewObj.removeNode( function(elem) {
    return (elem.obj_id==aId)?true:false;
  } );

  this.mViewObj.setSelectedRow(irow-1);
}

ws.findNodeByObjId = function (aId)
{
  if (!this._nodes)
    return null;

  var elem, jelem, kelem;
  var i, j, k, imax, jmax, kmax;

  imax = this._nodes.length;
  for (i=0; i<imax; ++i) {
    elem = this._nodes[i];
    if (elem.obj_id==aId)
      return elem;

    if (!("childNodes" in elem))
      continue;

    jmax = elem.childNodes.length;
    //dd("WS.findNode> "+elem.childNodes);

    for (j=0; j<jmax; ++j) {
      jelem = elem.childNodes[j];
      if (jelem.obj_id==aId)
        return jelem;

      if (!("childNodes" in jelem))
        continue;

      kmax = jelem.childNodes.length;
      for (k=0; k<kmax; ++k) {
        kelem = jelem.childNodes[k];
        if (kelem.obj_id==aId)
          return kelem;
      }
    }
  }

  return null;
}

ws.selectByUID = function (uid)
{
  return this.mViewObj.selectNodeByFunc( function (aNode) {
    return (aNode.obj_id == uid);
  });
}

//////////////////////////
// event handlers

ws.onLoad = function ()
{
  var that = this;
  var mainWnd = this._mainWnd = document.getElementById("main_view");

  //
  // for toolbar buttons
  //
  this.mBtnNew = document.getElementById("wspcPanelAddBtn");
  this.mBtnDel = document.getElementById("wspcPanelDeleteBtn");
  this.mBtnProp = document.getElementById("wspcPanelPropBtn");
  this.mBtnZoom = document.getElementById("wspcPanelZoomBtn");
  //this.mBtnUp = document.getElementById("wspcPanelUpBtn");
  //this.mBtnDown = document.getElementById("wspcPanelDownBtn");

  var objtree = document.getElementById("objectTree");
  objtree.addEventListener("select", function(e) { that.onTreeSelChanged(); }, false);

  //
  // setup the target scene
  //
  var scid = mainWnd.getCurrentSceneID();
  if (scid && scid>0)
    this.targetSceneChanged(scid);

  dd("workspace panel onLoad: MainView="+this._mainWnd+", target scene="+this.mTgtSceneID);

  //
  // setup tab-event handler for the MainTabView
  //
  mainWnd.mPanelContainer.addEventListener("select", function(aEvent) {
    var scid = mainWnd.getCurrentSceneID();
    //dd("Workspace panel: onSelect called: "+scid+", cur="+that.mTgtSceneID);
    if (scid != that.mTgtSceneID)
      that.targetSceneChanged(scid);
  }, false);

  this.mCamCtxtDisableTgt = document.getElementsByClassName("wspcCamCtxt-disable");

  this.mStyCtxtDisableTgt = document.getElementsByClassName("wspcStyCtxt-disable");

  //this.onTreeSelChanged();
}

ws.onUnLoad = function ()
{
  dd("Workspace Panel Unloading: scene ID="+this.mTgtSceneID+", callbackID = "+this._callbackID);
  // dd("Workspace Panel Unloading: scenemgr ="+this._scMgr._wrapped);

  // this._mainWnd.removeEventListener("select", this._mainViewEventHandler, false);
  var scene = cuemol.getScene(this.mTgtSceneID);

  dd("Workspace Panel Unloading: scene ="+scene);

  if (this._callbackID!=null && scene)
    scene.removeListener(this._callbackID);

  delete this._node;
  delete this.mViewObj;

  // dd(require("traceback").format());
  // window.alert('ws.fini');
}

ws._attachScene = function (scid)
{
  var scene = cuemol.getScene(scid);
  if (!scene)
    return;

  this.mTgtSceneID = scid;
  this.syncContents(scid);
  dd("WorkspacePanel: change the tgt scene to "+this.mTgtSceneID);

  var that = this;
  var handler = function (args) {
    switch (args.evtType) {
    case cuemol.evtMgr.SEM_ADDED:
      // window.alert("Scene event: SEM_ADDED "+debug.dumpObjectTree(args));
      that.mViewObj.saveOpenState(args.srcUID);
      that.syncContents(args.srcUID);
      that.selectByUID(args.obj.target_uid);
      break;

    case cuemol.evtMgr.SEM_REMOVING:
      if (args.method=="cameraRemoving")
        that.removeObject(args.obj.name);
      else {
        that.removeObject(args.obj.target_uid);
        if (args.method=="styleRemoving") {
          let stylem = cuemol.getService("StyleManager");
          stylem.firePendingEvents();
        }
      }
      break;

    case cuemol.evtMgr.SEM_CHANGED:
      //window.alert("Scene event: SEM_CHANGED "+debug.dumpObjectTree(args));
      if (args.method=="sceneAllCleared" ||
          args.method=="sceneLoaded")
        that.syncContents(args.srcUID);
      break;

    case cuemol.evtMgr.SEM_PROPCHG:
      //dd(debug.dumpObjectTree(args.obj));
      //dd("%%% WORKSPACE evtMgr.SEM_PROPCHG propname = "+args.obj.propname);
      if ("propname" in args.obj) {
        let pnm = args.obj.propname;
        if (pnm=="name" || pnm=="visible" || pnm=="locked") {
          // that.mViewObj.saveOpenState(args.srcUID);
          that.syncContentsPropChg(args.obj.target_uid, pnm);
        }
        else if (pnm=="group") {
          // Group changed
          //  --> tree structure can be changed, so we update all contents.
          that.syncContents(args.srcUID);
        }
      }
      break;
    }
  };
  
  this._callbackID = cuemol.evtMgr.addListener("",
                                               cuemol.evtMgr.SEM_SCENE|
                                               cuemol.evtMgr.SEM_OBJECT|
                                               cuemol.evtMgr.SEM_RENDERER|
                                               cuemol.evtMgr.SEM_CAMERA|
                                               cuemol.evtMgr.SEM_STYLE, // source type
                                               cuemol.evtMgr.SEM_ANY, // event type
                                               scene.uid, // source UID
                                               handler);

  /*
  // window.alert("XXX getScnene" + scid);
  if (scid>0) {
    var scene = cuemol.getScene(scid);
    this._callbackID = scene.addListener(new SceneEventListener(this));
  }
   */
}

// detach from the previous active scene
ws._detachScene = function (oldid)
{
  if (oldid<0) return;

  var oldscene = cuemol.getScene(oldid);
  if (oldscene && this._callbackID)
    cuemol.evtMgr.removeListener(this._callbackID);
  this._callbackID = null;

  // dd("===================");
  this.mViewObj.saveOpenState(oldid);
}

ws.targetSceneChanged = function (scid)
{
  try {
    if (scid==this.mTgtSceneID)
      return;
    //var oldid = this.mTgtSceneID;

    this._detachScene(this.mTgtSceneID);

    // attach to the new active scene
    this._attachScene(scid);

    this.onTreeSelChanged();
  }
  catch (e) {
    dd("Error in WS.targetSceneChanged !!");
    debug.exception(e);
  }
}

ws.onPanelShown = function ()
{
  this.mViewObj.ressignTreeView();
}
ws.onPanelMoved = function ()
{
  this.mViewObj.ressignTreeView();
}

ws.onNewCmd = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var id = elem.obj_id;

  dd("onNewCmd> elem.type="+elem.type);

  switch (elem.type) {
  case "object":
    gQm2Main.setupRendByObjID(id);
    break;

  case "renderer": {
    let rend = cuemol.getRenderer(id);
    let obj = rend.getClientObj();
    let grpnm = rend.group;
    if (grpnm)
      gQm2Main.setupRendByObjID(obj.uid, grpnm);
    else
      gQm2Main.setupRendByObjID(obj.uid);
    break;
  }
  case "rendGroup": {
    let rendgrp = cuemol.getRenderer(id);
    let obj = rendgrp.getClientObj();
    gQm2Main.setupRendByObjID(obj.uid, rendgrp.name);
    dd("Rend created in group="+rendgrp.group);
    break;
  }
    
  case "camera":
  case "cameraRoot":
    this.createCamera();
    break;
    
  case "style":
  case "styleRoot":
    this.createStyle();
    break;
  }
}

ws.onDeleteCmd = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var id = elem.obj_id;

  if (elem.type=="object") {
    gQm2Main.deleteObject(id);
  }
  else if (elem.type=="renderer") {
    this.mViewObj.saveSelection();
    gQm2Main.deleteRendByID(id);
    this.mViewObj.restoreSelection();
  }
  else if (elem.type=="rendGroup") {
    if (elem.childNodes.length>0)
      util.alert(window, "Group is not empty");
    else {
      this.mViewObj.saveSelection();
      gQm2Main.deleteRendByID(id);
      this.mViewObj.restoreSelection();
    }
  }
  else if (elem.type=="camera") {
    this.destroyCamera(id);
  }
  else if (elem.type=="style") {
    this.destroyStyle(elem);
  }
};

ws.onPropCmd = function ()
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  //dd("onNewCmd elem="+require("debug_util").dumpObjectTree(elem, 1));
  let id = elem.obj_id;

  let target, scene;
  switch (elem.type) {
  case "scene":
    scene = target = cuemol.getScene(id);
    break;

  case "object":
    target = cuemol.getObject(id);
    scene = target.getScene();
    break;
    
  case "renderer":
    target = cuemol.getRenderer(id);
    scene = target.getScene();
    break;
    
  case "rendGroup":
    // TO DO: impl (change group name)
    return this.onRenameRendGrp();
    
  case "camera":
    scene = this._mainWnd.currentSceneW;
    target = scene.getCameraRef(id);
    break;
    
  case "style": {
    this.showStyleEditor(elem);
    return;
  }

  default:
    return;
  }
  
  gQm2Main.showPropDlg(target, scene, window, elem.type);
};

ws.onBtnZoomCmd = function ()
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  //dd("onNewCmd elem="+require("debug_util").dumpObjectTree(elem, 1));
  var id = elem.obj_id;

  var target;
  var view = this._mainWnd.currentViewW;
  if (elem.type=="object") {
    target = cuemol.getObject(id);
    if (!('fitView' in target))
      return;
    target.fitView(false, view);
  }
  else if (elem.type=="renderer") {
    var rend = cuemol.getRenderer(id);
    target = rend.getClientObj();
    if (!('sel' in rend) || !('fitView' in target))
      return;
    target.fitView2(rend.sel, view);
  }
  else {
    return;
  }

  sel = null;
  target = null;
}

/*
ws.onMoveUpCmd = function (aEvent)
{
  let elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  if (elem.type=="object")
    this.onMoveUpObj(elem);
  else if (elem.type=="renderer")
    this.onMoveUpDownRend(elem, -1);
};

ws.onMoveDownCmd = function (aEvent)
{
  let elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  if (elem.type=="object")
    this.onMoveDownObj(elem);
  else if (elem.type=="renderer")
    this.onMoveUpDownRend(elem, +1);
}

ws.onMoveUpObj = function (elem)
{
  let id = elem.obj_id;

  let prev_id = -1;
  let i, imax = this._nodes.length;

  for (i=0; i<imax; ++i) {
    let nd = this._nodes[i];
    if (nd.type!="object") continue;
    if (nd.obj_id==id)
      break;
    prev_id = nd.obj_id;
  }
  
  dd("prev_id = "+prev_id);
  dd("id = "+id);
  if (prev_id<0) {
    dd("cannot move up");
    return;
  }

  this.swapNodes(prev_id, id, true);
  this.selectByUID(id);
}

ws.onMoveDownObj = function (elem)
{
  let id = elem.obj_id;

  let prev_id = -1;
  let i, imax = this._nodes.length;
  for (i=imax-1; i>=0; --i) {
    elem = this._nodes[i];
    if (elem.type!="object") continue;
    if (elem.obj_id==id)
      break;
    prev_id = elem.obj_id;
  }

  dd("prev_id = "+prev_id);
  dd("id = "+id);
  if (prev_id<0) {
    dd("cannot move down");
    return;
  }

  this.swapNodes(prev_id, id, true);
  this.selectByUID(id);
}

ws.onMoveUpDownRend = function (elem, idelta)
{
  let id = elem.obj_id;

  let irow = this.mViewObj.getSelectedRow();
  let irow_prev = irow + idelta;

  let elem_prev = this.mViewObj.getNodeByRow(irow_prev);
  if (!elem_prev) return;
  let id_prev = elem_prev.obj_id;

  dd("prev_id = "+id_prev);
  dd("id = "+id);

  if ((elem.type=="renderer" && elem_prev.type=="renderer")) {
    this.swapNodes(id_prev, id, false);
    this.selectByUID(id);
  }
  else {
    dd("cannot move up renderer");
  }
  
  return;
}

ws.swapNodes = function (prev_id, id, bObj)
{
  var obj1, obj2;
  if (bObj) {
    obj1 = cuemol.getObject(prev_id);
    obj2 = cuemol.getObject(id);
  }
  else {
    obj1 = cuemol.getRenderer(prev_id);
    obj2 = cuemol.getRenderer(id);
  }
  if (obj1==null || obj2==null) {
    dd("cannot swap/err1");
    return;
  }

  let uio = obj1.ui_order;
  obj1.ui_order = obj2.ui_order;
  obj2.ui_order = uio;

  this.mViewObj.saveOpenState(this.mTgtSceneID);
  this.syncContents(this.mTgtSceneID);
}
*/

ws.onTreeSelChanged = function ()
{
  //dd("******** ontreeselchanged called");

  //this.mBtnUp.disabled = true;
  //this.mBtnDown.disabled = true;

  var elem = this.mViewObj.getSelectedNode();
  if (elem) {
    if (elem.type=="scene") {
      this.mBtnNew.disabled = true;
      this.mBtnDel.disabled = true;
      this.mBtnProp.disabled = false;
      this.mBtnZoom.disabled = true;

      return;
    }
    else if (elem.type=="object") {
      this.mBtnNew.disabled = false;
      this.mBtnDel.disabled = false;
      this.mBtnProp.disabled = false;
      this.mBtnZoom.disabled = false;

      //this.mBtnUp.disabled = false;
      //this.mBtnDown.disabled = false;
      return;
    }
    else if (elem.type=="renderer") {
      this.mBtnNew.disabled = false;
      this.mBtnDel.disabled = false;
      this.mBtnProp.disabled = false;
      this.mBtnZoom.disabled = false;

      //this.mBtnUp.disabled = false;
      //this.mBtnDown.disabled = false;
      return;
    }
    else if (elem.type=="rendGroup") {
      this.mBtnNew.disabled = false;
      this.mBtnDel.disabled = false;
      this.mBtnProp.disabled = false;
      this.mBtnZoom.disabled = true;
      return;
    }
    else if (elem.type=="camera") {
      this.mBtnNew.disabled = false;
      this.mBtnDel.disabled = false;
      this.mBtnProp.disabled = false;
      this.mBtnZoom.disabled = true;
      return;
    }
    else if (elem.type=="cameraRoot") {
      this.mBtnNew.disabled = false;
      this.mBtnDel.disabled = true;
      this.mBtnProp.disabled = true;
      this.mBtnZoom.disabled = true;
      return;
    }
    else if (elem.type=="styleRoot") {
      this.mBtnNew.disabled = false;
      this.mBtnDel.disabled = true;
      this.mBtnProp.disabled = true;
      this.mBtnZoom.disabled = true;
      return;
    }
    else if (elem.type=="style") {
      this.mBtnNew.disabled = false;
      if (elem.scene_id==0) {
        // global (locked) style def
        this.mBtnDel.disabled = true;
        this.mBtnProp.disabled = true;
        this.mBtnZoom.disabled = true;
      }
      else {
        // local style def
        this.mBtnDel.disabled = false;
        this.mBtnProp.disabled = false;
        this.mBtnZoom.disabled = true;
      }
      return;
    }
  }
  this.mBtnNew.disabled = true;
  this.mBtnDel.disabled = true;
  this.mBtnProp.disabled = true;
  this.mBtnZoom.disabled = true;
}

ws.onTreeItemClick = function (aEvent, elem, col)
{
  //dd("WS onClick: row="+row+", col="+col);
  // dd("WS onClick: detail="+aEvent.detail);

  if (col=="object_vis") {
    this.toggleVisible(elem);
  }
  else if (col=="object_lck") {
    // dd("WS> Toggle LCK, obj_id="+elem.obj_id);
    // dd("    Toggle LCK, object_lck="+elem.props.object_lck);
    this.toggleLocked(elem);
  }
  else if (aEvent.detail==2) {
    if (elem.type=="camera") {
      var scene = this._mainWnd.currentSceneW;
      var view = this._mainWnd.currentViewW;
      if (!scene || !view) return;
      scene.loadViewFromCam(view.uid, elem.obj_id);
      aEvent.preventDefault();
      aEvent.stopPropagation();
      return;
    }
    else {
      this.onPropCmd();
      return;
    }
  }
  
  aEvent.preventDefault();
  aEvent.stopPropagation();
}

ws.toggleVisible = function (aElem)
{
  var obj = null;
  if (aElem.type=="object")
    obj = cuemol.getObject(aElem.obj_id);
  else if (aElem.type=="renderer")
    obj = cuemol.getRenderer(aElem.obj_id);
  else if (aElem.type=="rendGroup")
    return this.toggleVisibleRendGrp(aElem);

  if (!obj) return;

  var scene = cuemol.getScene(this.mTgtSceneID);
  if (!scene) return;

  // EDIT TXN START //
  scene.startUndoTxn("Change visibility");
  try {
    obj.visible = !obj.visible;
  }
  catch (e) {
    dd("***** ERROR: Change visibility "+e);
    debug.exception(e);
  }
  scene.commitUndoTxn();
  // EDIT TXN END //
}

ws.toggleLocked = function (aElem)
{
  var obj = null;
  if (aElem.type=="object")
    obj = cuemol.getObject(aElem.obj_id);
  else if (aElem.type=="renderer")
    obj = cuemol.getRenderer(aElem.obj_id);
  else if (aElem.type=="rendGroup")
    ; // TO DO: impl

  if (!obj)
    return; // toggleLocked is not supported for this row...

  var scene = cuemol.getScene(this.mTgtSceneID);
  if (!scene) return;

  var msg;
  if (obj.locked)
    msg="Unlock "+aElem.type;
  else
    msg="Lock "+aElem.type;

  // EDIT TXN START //
  scene.startUndoTxn(msg);
  try {
    obj.locked = !obj.locked;
  }
  catch (e) {
    dd("***** ERROR: Change locked "+e);
    debug.exception(e);
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

  dd(">WS tglLck result, locked="+obj.locked);
};

ws.onTwistyClick = function (row, elem)
{
  var target;

  if (elem.type=="object")
    target = cuemol.getObject(elem.obj_id);
  else if (elem.type=="rendGroup")
    target = cuemol.getRenderer(elem.obj_id);
  else
    return;

  // save collapsed state to the scene
  // for persistance of open/collapsed state
  target.ui_collapsed = elem.collapsed;
};

ws.selectMol = function (aSelStr)
{
  var elem = this.mViewObj.getSelectedNode();
  if (elem.type!="object") return;

  var target = cuemol.getObject(elem.obj_id);
  if (!('sel' in target))
    return;

  var scene = target.getScene();
  var sel;

  // EDIT TXN START //
  if (aSelStr) {
    sel = cuemol.makeSel(aSelStr);
    scene.startUndoTxn("Select molecule");
  }
  else {
    sel = cuemol.createObj("SelCommand");
    scene.startUndoTxn("Unselect molecule");
  }

  try {
    target.sel = sel;
  }
  catch(e) {
    dd("SetProp error");
    debug.exception(e);
    scene.rollbackUndoTxn();
    return;
  }

  scene.commitUndoTxn();
  // EDIT TXN END //

  // Save to history
  util.selHistory.append(aSelStr);
}

/// Invert selection
ws.invertMolSel = function ()
{
  var elem = this.mViewObj.getSelectedNode();
  if (elem.type!="object") return;

  var target = cuemol.getObject(elem.obj_id);
  if (!('sel' in target))
    return;

  cuemolui.molSelInvert(target);
};

/// Toggle bysidech modifier
ws.toggleSideCh = function ()
{
  var elem = this.mViewObj.getSelectedNode();
  if (elem.type!="object") return;

  var target = cuemol.getObject(elem.obj_id);
  if (!('sel' in target))
    return;

  cuemolui.molSelToggleSideCh(target);
};

ws.aroundMolSel = function (aDist, aByres)
{
  var elem = this.mViewObj.getSelectedNode();
  if (elem.type!="object") return;

  var target = cuemol.getObject(elem.obj_id);
  if (!('sel' in target))
    return;

  cuemolui.molSelAround(target, aDist, aByres);
}

ws.getSelectedRend = function ()
{
  var elem = this.mViewObj.getSelectedNode();
  if (elem.type!="renderer")
    return null;
  return cuemol.getRenderer(elem.obj_id);
};

ws.checkColoring = function ()
{
  var target = this.getSelectedRend();

  if (target==null||
      target.type_name=="*selection" ||
      target.type_name=="*namelabel" ||
      target.type_name=="atomintr")
    return null;

  if (!('coloring' in target)) {
    dd("WS.coloringMol> Error, coloring not supported in rend, "+elem.obj_id);
    return null;
  }

  return target;
};

ws.onColoringMol = function (aEvent)
{
  var target = this.checkColoring();
  if (target) {
    //alert("event value="+aEvent.target.value);
    gQm2Main.setRendColoring(aEvent.target.value, target);
  }
}

ws.checkPaintColoring = function ()
{
  var target = this.checkColoring();
  if (!target)
    return null;
  var coloring = target.coloring;

  var clsname = coloring._wrapped.getClassName();
  if (clsname!="PaintColoring") {
    dd("WS.coloringMol> Error, not paint coloring");
    return null;
  }

  return [coloring, target];
}

ws.onPaintMol = function (aEvent)
{
  dd("WS.paintMol: "+aEvent.target.localName);
  let value = aEvent.target.value;
  dd("WS.paintMol: "+value);

  let coloring = null;
  let elem = this.mViewObj.getSelectedNode();
  let uobj = cuemol.getUIDObj(elem.obj_id);
  if (!('coloring' in uobj))
    return;
  let coloring = uobj.coloring;

  let sel = null;
  if ('getClientObj' in uobj) {
    // renderer
    var mol = uobj.getClientObj();
    if ('sel' in mol)
      sel = mol.sel;
  }    
  else if ('sel' in uobj) {
    // object
    sel = uobj.sel;
  }

  if (sel==null || sel.isEmpty()) {
    dd("WS.coloringMol> Error, cur sel is empty");
    util.alert(window, "Selection is empty");
    return;
  }

  let scene = uobj.getScene();

  // EDIT TXN START //
  scene.startUndoTxn("Insert paint entry");

  try {
    if (uobj._wrapped.isPropDefault("coloring"))
      uobj.coloring = coloring;
    coloring.insertBefore(0, sel, cuemol.makeColor(value));
  }
  catch (e) {
    dd("***** ERROR: insewrtBefore "+e);
    debug.exception(e);
    scene.rollbackUndoTxn();
    return;
  }

  scene.commitUndoTxn();
  // EDIT TXN END //
};

ws.onStyleShowing = function (aEvent)
{
  try {

    var elem = this.mViewObj.getSelectedNode();
    //dd("elem.type_name="+elem.type_name);

    if (elem.type!="renderer") return;

    var menu = aEvent.currentTarget.menupopup;

    var regex = null;
    if (elem.type_name == "ribbon") {
      regex = /Ribbon$/;
    }
    else if (elem.type_name == "cartoon") {
      regex = /Ribbon$/;
    }
    else if (elem.type_name == "ballstick") {
      regex = /BallStick$/;
    }
    else if (elem.type_name == "atomintr") {
      regex = /AtomIntr$/;
    }
    else if (elem.type_name == "simple") {
      regex = /Simple$/;
    }
    else if (elem.type_name == "trace") {
      regex = /Trace$/;
    }

    cuemolui.populateStyleMenus(this.mTgtSceneID, menu, regex, true);
    
    // add edge styles
    if (elem.type_name != "simple" &&
        elem.type_name != "trace" &&
        elem.type_name != "spline" &&
        elem.type_name != "*namelabel" &&
        elem.type_name != "*selection" &&
        elem.type_name != "coutour") {
      regex = /^EgLine/;
      util.appendMenuSep(document, menu);
      cuemolui.populateStyleMenus(this.mTgtSceneID, menu, regex, false);
    }

  } catch (e) { debug.exception(e); }
};

ws.styleMol = function (aEvent)
{
  try {
    var rend = this.getSelectedRend();
    if (rend==null)
      return;

    var value = aEvent.target.value;
    var remove_re = aEvent.target.getAttribute("remove_re");
    remove_re = remove_re.substr(1, remove_re.length-2);
    remove_re = RegExp(remove_re);

    var style = value.substr("style-".length);
    dd("style: "+style);
    dd("remove_re: "+remove_re);

    var curstyle = rend.style;
    curstyle = styleutil.remove(curstyle, remove_re);
    style =  styleutil.push(curstyle, style);
    dd("styleMol> new style: "+style);

    var scene = rend.getScene();

    // EDIT TXN START //
    scene.startUndoTxn("Change style");

    try {
      rend.applyStyles(style);
    }
    catch (e) {
      dd("***** ERROR: pushStyle "+e);
      debug.exception(e);
      scene.rollbackUndoTxn();
      return;
    }
    scene.commitUndoTxn();
    // EDIT TXN END //

  } catch (e) { debug.exception(e); }
};

ws.setRendSel = function (aSelStr)
{
  var rend = this.getSelectedRend();
  if (rend==null)
    return;
  
  var mol = rend.getClientObj();
  if (!('sel' in rend) || !('sel' in mol))
    return;

  var sel;

  if (aSelStr=='current') {
    sel = mol.sel;
  }
  else {
    sel = cuemol.makeSel(aSelStr);
  }

  var scene = rend.getScene();

  // EDIT TXN START //
  scene.startUndoTxn("Set renderer sel");

  try {
    rend.sel = sel;
  }
  catch(e) {
    dd("SetProp error");
    debug.exception(e);
    scene.rollbackUndoTxn();
    return;
  }

  scene.commitUndoTxn();
  // EDIT TXN END //
};

ws.onSaveAsObj = function(event)
{
  try {
    let elem = this.mViewObj.getSelectedNode();
    let objid = elem.obj_id;
    gQm2Main.onSaveAsObj(objid);
  }
  catch (e) {
    debug.exception(e);
  }
};

ws.onRendCtxtMenuShowing = function (aEvent)
{
  // setup rend context menu items

  var paintitem = document.getElementById("wspcPanelPaintMenu");
  var clrngitem = document.getElementById("wspcPanelRendColMenu");
  var selitem = document.getElementById("wspcPanelRendSelMenu");
  var editiitem = document.getElementById("wspcPanelEditIntrMenu");

  editiitem.hidden = true;
  if (this.checkIntrRend()!=null)
    editiitem.hidden = false;

  var rend = this.getSelectedRend();
  if (rend==null ||
      rend.type_name=="*selection") {
    selitem.hidden = true;
    paintitem.disabled = true;
    clrngitem.disabled = true;
    document.getElementById("wspcPanelStyleMenu").disabled = true;
    document.getElementById("wspcPanelCopyMenu").disabled = true;
    return;
  }

  selitem.hidden = false;
  document.getElementById("wspcPanelStyleMenu").disabled = false;
  document.getElementById("wspcPanelCopyMenu").disabled = false;
  
  if (this.checkColoring()==null) {
    paintitem.disabled = true;
    clrngitem.disabled = true;
  }
  else if (this.checkPaintColoring()==null) {
    paintitem.disabled = true;
    clrngitem.disabled = false;
  }
  else {
    paintitem.disabled = false;
    clrngitem.disabled = false;
  }

}

ws.checkIntrRend = function ()
{
  var target = this.getSelectedRend();
  if (target==null||
      target.type_name !== "atomintr")
    return null;
  return target;
}

ws.onEditIntr = function ()
{
  var rend = this.checkIntrRend();
  
  var args = Cu.getWeakReference({target: rend});
  window.openDialog("chrome://cuemol2/content/tools/aintr-edit-dlg.xul",
                    null,
                    "chrome,modal,resizable=no,dependent,centerscreen",
                    args);
}

//////////////////////////////
// Copy & Paste (renderer)

/// Context menu setup for Scene, Object, and RendGrp items
ws.onCtxtMenuShowing = function (aEvent)
{
  var clipboard = require("qsc-copipe");
  var item, xmlstr;
  try {
    if (aEvent.target.id=="wspcPanelObjCtxtMenu") {
      // Update the renderer-paste menu
      item = document.getElementById("wspcPanelObjCtxtMenu-Paste");
      xmlstr = clipboard.get("qscrend");
      if (xmlstr)
        item.disabled = false;
      else
        item.disabled = true;
    }
    else if (aEvent.target.id=="wspcPanelRendGrpCtxtMenu") {
      // Update the renderer-paste in rendgrp menu 
      item = document.getElementById("wspcPanelRendGrpCtxtMenu-Paste");
      xmlstr = clipboard.get("qscrend");
      dd("RendGrp CtxtMenu clipboard="+xmlstr);
      if (xmlstr)
        item.disabled = false;
      else
        item.disabled = true;
    }
    else if (aEvent.target.id=="wspcPanelSceneCtxtMenu") {
      // Update the object-paste menu
      item = document.getElementById("wspcPanelSceneCtxtMenu-Paste");
      xmlstr = clipboard.get("qscobj");
      if (xmlstr)
        item.disabled = false;
      else
        item.disabled = true;
    }
  } catch (e) { debug.exception(e); }
}

ws.onCopyCmd = function (aEvent)
{
  try {
    var elem = this.mViewObj.getSelectedNode();
    if (!elem) return;
    var id = elem.obj_id;

    let clipboard = require("qsc-copipe");

    if (elem.type=="renderer") {
      let rend = cuemol.getRenderer(id);
      let xmldat = gQm2Main.mStrMgr.toXML(rend);
      clipboard.set(xmldat, "qscrend");
    }
    else if (elem.type=="rendGroup") {
      // TO DO: impl
    }
    else if (elem.type=="object") {
      let obj = cuemol.getObject(id);
      let xmldat = gQm2Main.mStrMgr.toXML(obj);
      clipboard.set(xmldat, "qscobj");
    }
    
  } catch (e) { debug.exception(e); }
};

ws.onPasteRend = function (aEvent)
{
  try {
    let elem = this.mViewObj.getSelectedNode();
    if (!elem) return;
    let id = elem.obj_id;

    let clipboard = require("qsc-copipe");

    let destgrp = "";
    var obj;
    if (elem.type=="rendGroup") {
      let rendgrp = cuemol.getRenderer(id);
      destgrp = rendgrp.name;
      obj = rendgrp.getClientObj();
    }
    else if (elem.type=="object") {
      obj = cuemol.getObject(id);
    }
    else
      return;

    let xmldat = clipboard.get("qscrend");
    if (!xmldat) {
      dd("PasteRend, ERROR: "+xmldat);
      return;
    }

    //dd("XML: "+xmlstr);
      
    let scene = obj.getScene();
    let rend = gQm2Main.mStrMgr.fromXML(xmldat, scene.uid);
    if (!rend.isCompatibleObj(obj)) {
      util.alert(window, "Cannot paste renderer to incompatible object.");
      return;
    }

    // check the uniqueness of renderer's name
    let name = rend.name;
    if (scene.getRendByName(name)) {
      // duplicated --> set differernt name
      name = util.makeUniqName2(
	function (a) {return "copy"+a+"_"+name; },
	function (a) {return scene.getRendByName(a);} );
    }
    rend.name = name;
    
    // check & change the group
    rend.group = destgrp;
    // alert("rend.group="+rend.group);

    // EDIT TXN START //
    scene.startUndoTxn("Paste renderer");
    try {
      obj.attachRenderer(rend);
    }
    catch (e) {
      dd("***** ERROR: Paste renderer "+e);
      debug.exception(e);
      scene.rollBackUndoTxn();
      rend = null;
      return;
    }
    scene.commitUndoTxn();
    // EDIT TXN END //
    
    rend = null;
  }
  catch (e) {
    debug.exception(e);
  }
};

ws.onPasteObj = function (aEvent)
{
  var elem, id, name;
  var scene, obj, xmldat;

  try {
    elem = this.mViewObj.getSelectedNode();
    if (!elem) return;
    id = elem.obj_id;
    scene = cuemol.getScene(id);

    let clipboard = require("qsc-copipe");

    if (elem.type!="scene")
      return;

    xmldat = clipboard.get("qscobj");
    if (!xmldat) {
      dd("PasteObj, ERROR: "+xmldat);
      return;
    }
  } catch (e) { debug.exception(e); }

  try {
    // dd("XML: length="+xmldat.length);
    obj = gQm2Main.mStrMgr.fromXML(xmldat, scene.uid);
  }
  catch (e) {
    dd("ERROR XML ="+xmldat);
    debug.exception(e);
  }

  try {
    name = obj.name;
    if (scene.getObjectByName(name)) {
      name = util.makeUniqName2(
	function (a) {return "copy"+a+"_"+name; },
	function (a) {return scene.getObjectByName(a);} );
    }

    obj.name = name;

    // EDIT TXN START //
    scene.startUndoTxn("Paste object");
    try {
      scene.addObject(obj);
    }
    catch (e) {
      dd("***** ERROR: Paste object "+e);
      debug.exception(e);
      scene.rollBackUndoTxn();
      return;
    }
    scene.commitUndoTxn();
    // EDIT TXN END //


  } catch (e) { debug.exception(e); }
};

//////////////////////////
// Camera manipulations

ws.createCamera = function ()
{
  var scene = this._mainWnd.currentSceneW;
  var view = this._mainWnd.currentViewW;
  var i, name, res;
  for (i=0; ; ++i) {
    name = "camera_"+i;
    if (!scene.getCamera(name)) {
      res = util.prompt(window, "Name for new camera: ", name);
      if (res===null) return;
      break;
    }
  }

  // EDIT TXN START //
  scene.startUndoTxn("Create camera: "+res);
  try {
    scene.saveViewToCam(view.uid, res);
  }
  catch (e) {
    dd("***** ERROR: Create camera "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

ws.destroyCamera = function (name)
{
  var scene = this._mainWnd.currentSceneW;
  var res = util.confirm(window, "Delete camera: "+name);
  if (!res)
    return; // canceled

  // EDIT TXN START //
  scene.startUndoTxn("Destroy camera: "+name);
  try {
    scene.destroyCamera(name);
  }
  catch (e) {
    dd("***** ERROR: Destroy camera "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

///////////////////////
// Camera menu

ws.onRenameCamera = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var name = elem.obj_id;

  var scene = this._mainWnd.currentSceneW;
  var cam = scene.getCamera(name);
  if (cam==null) {
    dd("WS Error: camera "+name);
    return;
  }

  var res = util.prompt(window, "Rename camera \""+name+"\": ", name);
  if (res===null) return; // canceled
  
  if (name===res) return; // not changed
  
  if (scene.hasCamera(res)) {
    util.alert(window, "Cannot rename camera: \""+res+"\" already exists.");
    return;
  }

  // EDIT TXN START //
  scene.startUndoTxn("Rename camera "+name);
  try {
    scene.destroyCamera(name);
    scene.setCamera(res, cam);
  }
  catch (e) {
    dd("***** ERROR: Rename camera "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

ws.onCamSaveFileAs = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var name = elem.obj_id;

  if (!gQm2Main.onSaveCamera(name)) {
    // failed/canceled
    return;
  }

  if (name=="__current")
    return;

  // saved camera becomes the file-linked item
  let node = this.findNodeByObjId(name);
  if (node==null)
    return;
  node.props.object_vis = "linked";

  this.mViewObj.updateNode( function(elem) {
    return (elem.obj_id==name)?true:false;
  } );
};

ws.onCamSaveFile = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var name = elem.obj_id;

  var scene = this._mainWnd.currentSceneW;
  var cam = scene.getCamera(name);

  if (cam.src.length==0) {
    // embeded camera (no src prop) --> perform save-as
    this.onCamSaveFileAs(aEvent);
    return;
  }

  // save to the same file as the src property
  if (!scene.saveCameraTo(name, cam.src)) {
    util.alert(window, "Save camera failed!");
    return;
  }
};

/// Load camera from the file
ws.onCamLoadFile = function (aEvent)
{
  var cam = gQm2Main.onLoadCamera();
  if (cam==null)
    return;

  var name = cam.name;
  var scene = this._mainWnd.currentSceneW;

  if (scene.hasCamera(name)) {
    // make a unique name, if the same name exists
    name = util.makeUniqName2(
      function (a) {return "copy"+a+"_"+name; },
      function (a) {return (scene.hasCamera(a)?1:null);} );
  }

  // EDIT TXN START //
  scene.startUndoTxn("Load camera file "+name);
  try {
    scene.setCamera(name, cam);
  }
  catch (e) {
    dd("***** ERROR: Change camera link "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

  // apply the loaded camera to the view
  var view = this._mainWnd.currentViewW;
  if (!view) return;
  scene.loadViewFromCam(view.uid, name);
};

/// Reload file-linked camera
ws.onCamReloadFile = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var name = elem.obj_id;

  var scene = this._mainWnd.currentSceneW;
  var cam = scene.getCamera(name);
  if (cam==null) {
    util.alert(window, "Camera not found");
    return;
  }

  var srcpath = cam.src;
  if (srcpath.length==0) {
    util.alert(window, "This camera is not linked to a file");
    return;
  }

  var newcam;
  try {
    newcam = scene.loadCamera(srcpath);
  }
  catch (e) {
    util.alert(window, "Cannot load camera from file: "+srcpath);
    return;
  }

  // EDIT TXN START //
  scene.startUndoTxn("Reoad camera file "+name);
  try {
  scene.setCamera(name, newcam);
  }
  catch (e) {
    dd("***** ERROR: Change camera link "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

ws.onLoadSaveCam = function (aEvent, aLoad)
{
  var elem = this.mViewObj.getSelectedNode();
  if (elem.type!="camera") return;

  var scene = this._mainWnd.currentSceneW;
  var view = this._mainWnd.currentViewW;
  if (!scene || !view) return;

  if (aLoad) {
    scene.loadViewFromCam(view.uid, elem.obj_id);
    return;
  }

  // EDIT TXN START //
  scene.startUndoTxn("Change camera "+elem.obj_id);
  try {
    scene.saveViewToCam(view.uid, elem.obj_id);
  }
  catch (e) {
    dd("***** ERROR: Chg camera "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

/// initialize camera context menu
ws.onCamCtxtShowing = function (aEvent)
{
  try {
    //
    // Update the camera-paste menu
    //
    var item = document.getElementById("wspcCamCtxt-Paste");

    let clipboard = require("qsc-copipe");
    let xmlstr = clipboard.get("qsccam");
    if (xmlstr)
      item.disabled = false;
    else
      item.disabled = true;

    //
    // Update other menus
    //
    var elem = this.mViewObj.getSelectedNode();
    var tgt = Array.prototype.slice.call(this.mCamCtxtDisableTgt, 0);
    
    if (elem.type=="camera") {
      tgt.forEach( function (elem, ind, ary) {
	  elem.setAttribute("disabled", false);
	});
    }
    else {
      tgt.forEach( function (elem, ind, ary) {
	  elem.setAttribute("disabled", true);
	});
    }

    // update reload menu
    dd("elem type="+elem.type);
    if (elem.type=="camera") {
      let widget = document.getElementById("wspcCamCtxtReload");
      let name = elem.obj_id;
      let scene = this._mainWnd.currentSceneW;
      let cam = scene.getCamera(name);
      let srcpath = cam.src;
      if (srcpath.length==0)
        widget.disabled = true;
      else
        widget.disabled = false;
    }
    
  } catch (e) { debug.exception(e); }
};

ws.onCameraCopy = function (aEvent)
{
  try {
    var elem = this.mViewObj.getSelectedNode();
    if (!elem || elem.type!="camera")
      return;

    var scene = cuemol.getScene(this.mTgtSceneID);
    var target = scene.getCamera(elem.obj_id);
    if (target==null)
      return;

    let clipboard = require("qsc-copipe");

    // alert("copy camera: "+target.name);

    let xmldat = gQm2Main.mStrMgr.toXML(target);
    clipboard.set(xmldat, "qsccam");

  } catch (e) { debug.exception(e); }
};

ws.onCameraPaste = function (aEvent)
{
  try {
    let elem = this.mViewObj.getSelectedNode();
    if (!elem) return;
    if (elem.type!="cameraRoot" && elem.type!="camera") return;
    
    let clipboard = require("qsc-copipe");

    let xmldat = clipboard.get("qsccam");
    if (!xmldat) {
      dd("PasteCamera, ERROR: "+xmldat);
      return;
    }

    dd("XML: "+xmldat);
    let scene = cuemol.getScene(this.mTgtSceneID);
    let cam = gQm2Main.mStrMgr.fromXML(xmldat, scene.uid);

    let name = cam.name;
    if (scene.hasCamera(name)) {
      // make a unique name, if the same name exists
      name = util.makeUniqName2(
	function (a) {return "copy"+a+"_"+name; },
        function (a) {return (scene.hasCamera(a)?1:null);} );
    }
    
    // cam.name = name;
    // alert("paste camera: "+name);
    
    // EDIT TXN START //
    scene.startUndoTxn("Paste camera");
    try {
      scene.setCamera(name, cam);
    }
    catch (e) {
      dd("***** ERROR: Paste camera "+e);
      debug.exception(e);
      scene.rollBackUndoTxn();
      cam = null;
      return;
    }
    scene.commitUndoTxn();
    // EDIT TXN END //
    
    cam = null;
  }
  catch (e) {
    debug.exception(e);
  }
};

////////////////////////////////////////////////////
// Style operation

ws.createStyle = function ()
{
  let sceneid = this.mTgtSceneID;
  let scene = cuemol.getScene(sceneid);
  let stylem = cuemol.getService("StyleManager");

  let i, name, set_id = -1;
  for (i=0; ; ++i) {
    name = "style_"+i;
    if (!stylem.hasStyleSet(name, sceneid)) {
      var res = util.prompt(window, "Name for new style: ", name);
      if (res===null)
        return;
      break;
    }
  }

  // EDIT TXN START //
  scene.startUndoTxn("Create style");
  try {
    set_id = stylem.createStyleSet(res, sceneid);
  }
  catch (e) {
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

/*
  // update scene's tree UI (createStyleSet doesn't generate ADD event)
  this.mViewObj.saveOpenState(sceneid);
  this.mViewObj.saveSelection();
  this.syncContents(sceneid);
  if (!this.selectByUID(set_id))
    this.mViewObj.restoreSelection();
*/
  
}

ws.destroyStyle = function (elem)
{
  let irow = this.mViewObj.getSelectedRow();

  let name = elem.name;
  let sceneid = this.mTgtSceneID;
  let scene = cuemol.getScene(sceneid);
  let res = util.confirm(window, "Delete style: "+name);
  if (!res)
    return; // canceled

  let stylem = cuemol.getService("StyleManager");

  // EDIT TXN START //
  scene.startUndoTxn("Destroy style");
  try {
    stylem.destroyStyleSet(sceneid, elem.obj_id);
  }
  catch (e) {
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

/*
  // update scene's tree UI
  this.mViewObj.saveOpenState(sceneid);
  this.syncContents(sceneid);
  this.mViewObj.setSelectedRow(irow-1);
*/
  // update all
  // stylem.firePendingEvents();
};

ws.showStyleEditor = function (elem)
{
  var argobj = {
  //target: elem.obj_id,
  //target_id: elem.uid,
  target: elem.name,
  target_id: elem.obj_id,
  scene_id: elem.scene_id
  };
  var args = Cu.getWeakReference(argobj);

  var stylestr = "chrome,resizable=yes,dependent,centerscreen";

  var win = gQm2Main.mWinMed.getMostRecentWindow("CueMol2:StyleEditorDlg");
  if (win)
    win.focus();
  else
    window.openDialog("chrome://cuemol2/content/style/style_editor.xul",
                      "", stylestr, args);
};

/// initialize style context menu
ws.onStyCtxtShowing = function (aEvent)
{
  try {
    //
    // Update other menus
    //
    var elem = this.mViewObj.getSelectedNode();
    var tgt = Array.prototype.slice.call(this.mStyCtxtDisableTgt, 0);
    
    if (elem.type=="style") {
      tgt.forEach( function (elem, ind, ary) {
	  elem.setAttribute("disabled", false);
	});
    }
    else {
      tgt.forEach( function (elem, ind, ary) {
	  elem.setAttribute("disabled", true);
	});
    }

    dd("elem type="+elem.type);
    if (elem.type=="style") {
      // get target style set
      let stylem = cuemol.getService("StyleManager");
      let styleset = stylem.getStyleSet(elem.obj_id);

      // update reload menu
      let widget = document.getElementById("wspcStyCtxtReload");
      if (styleset.src.length>0)
        // external style set --> reloadable
        widget.setAttribute("disabled", false);
      else
        widget.setAttribute("disabled", true);

      // update toggle read-only menu
      widget = document.getElementById("wspcStyCtxtReadOnly");
      if (styleset.readonly)
        widget.setAttribute("checked", true);
      else
        widget.removeAttribute("checked");
      
      if (elem.scene_id==0)
        widget.setAttribute("disabled", true);
      else
        widget.setAttribute("disabled", false);

      // modified style cannot be changed to read-only mode!!
      if (!styleset.readonly && styleset.modified)
        widget.setAttribute("disabled", true);
    }
    
  } catch (e) { debug.exception(e); }
};

ws.onStyToggleRo = function (event)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem)
    return;

  // system styles is not editable
  if (elem.scene_id==0)
    return;

  let stylem = cuemol.getService("StyleManager");
  let styleset = stylem.getStyleSet(elem.obj_id);

  if (styleset.readonly)
    styleset.readonly = false;
  else {
    // read/write --> read-only
    // modified style cannot be changed to read-only mode!!
    if (styleset.modified)
      alert("Modified styles cannot be changed to read-only mode.");
    else
      styleset.readonly = true;
  }
};

/// Save style to new file (save-as)
ws.onStySaveFileAs = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var uid = elem.obj_id;

  const nsIFilePicker = Ci.nsIFilePicker;
  let fp = Cc["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  fp.appendFilter("Style file (*.xml)", "*.xml");

  if (elem.name!="(anonymous)")
    fp.defaultString = elem.name;
  fp.defaultExtension = "*.xml";
  
  fp.init(window, "Save style to file", nsIFilePicker.modeSave);

  let res=fp.show();
  if (res==nsIFilePicker.returnCancel)
    return;

  let path = fp.file.path;
  let res = util.splitFileName2(path, "*.xml");
  if (res)
    path = res.path;

  ////////////////////////
  dd("save style to file: "+path);

  let scene = cuemol.getScene(this.mTgtSceneID);
  let stylem = cuemol.getService("StyleManager");
  let result = false;

  // EDIT TXN START //
  scene.startUndoTxn("Change style's source");
  try {
    result = stylem.saveStyleSetToFile(this.mTgtSceneID, uid, path);
  }
  catch (e) {
    dd("***** ERROR: Change style's source "+e);
    debug.exception(e);
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

  if (!result) {
    util.alert(window, "Save style file failed!");
    return;
  }

  ////////////////////////
  // saved style becomes the file-linked item

  let node = this.findNodeByObjId(uid);
  if (node==null)
    return;
  node.props.object_vis = "linked";

  this.mViewObj.updateNode( function(elem) {
    return (elem.obj_id==name)?true:false;
  } );
};

/// Save style to file (ovw)
ws.onStySaveFile = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;
  var id = elem.obj_id;

  let stylem = cuemol.getService("StyleManager");
  let srcpath = stylem.getStyleSetSource(id);
  if (srcpath.length==0) {
    // embeded style (no src prop) --> perform save-as
    this.onStySaveFileAs(aEvent);
    return;
  }

  if (!stylem.saveStyleSetToFile(this.mTgtSceneID, id, srcpath)) {
    util.alert(window, "Save style file failed!");
    return;
  }
  
};

/// Load style from the file
ws.onStyLoadFile = function (aEvent)
{
  const nsIFilePicker = Ci.nsIFilePicker;
  let fp = Cc["@mozilla.org/filepicker;1"].createInstance(nsIFilePicker);
  fp.appendFilter("Style file (*.xml)", "*.xml");

  fp.init(window, "Open style file", nsIFilePicker.modeOpen);

  let res=fp.show();
  if (res==nsIFilePicker.returnCancel)
    return;

  let path = fp.file.path;
  let res = util.splitFileName2(path, "*.xml");
  if (res)
    path = res.path;

  //////////////////////
  dd("load style from file: "+path);

  let scene_id = this.mTgtSceneID;
  let stylem = cuemol.getService("StyleManager");

  // Load style file (In default, the external styles should be loaded as read-only.)
  if (stylem.loadStyleSetFromFile(scene_id, path, true)<0) {
    util.alert(window, "Open style file failed!");
    return;
  }

  // update scene's tree UI
  this.mViewObj.saveOpenState(scene_id);
  this.syncContents(scene_id);

  // update all
  stylem.firePendingEvents();
};

/// Reload style (link to the file)
ws.onStyReloadFile = function (aEvent)
{
  // TO DO: impl ??
  alert("Not implemented!!");
};

//
// Change renderer type impl
//

ws.onChgRendTypeShowing = function (aEvent)
{
  try {
    // clear the popup menu
    let menu = aEvent.currentTarget.menupopup;
    util.clearMenu(menu);

    let elem = this.mViewObj.getSelectedNode();
    //dd("elem.type_name="+elem.type_name);

    if (elem.type!="renderer") return;

    let rend = this.getSelectedRend();
    if (rend==null) return;

    let obj = rend.getClientObj();
    if (obj==null) return;

    // skip special renderers
    let rendtype = rend.type_name;
    if (rendtype.startsWith("*") && rendtype!="*selection")
      return;
    if (rendtype=="atomintr"||rendtype=="disorder")
      return;

    // enumerate the compatible renderer names
    let rend_types = obj.searchCompatibleRendererNames().split(",");
    dd("supported rend types="+rend_types);
    //cuemolui.populateStyleMenus(this.mTgtSceneID, menu, regex, true);

    // setup the popup menu
    let nitems = rend_types.length;
    let name, value, label, res;
    for (let i=0; i<nitems; ++i) {
      name = rend_types[i];
      if (name==rendtype || name.startsWith("*") || name=="atomintr" || name=="disorder")
        continue;
      let item = util.appendMenu(document, menu, name, name);
      dd("ChgRendTypeAddMenu = "+name+", desc="+label);
    }
    
  } catch (e) { debug.exception(e); }
};

ws.chgRendType = function (aEvent)
{
  let rend = this.getSelectedRend();
  if (rend==null)
    return;
  let obj = rend.getClientObj();
  if (obj==null)
    return;
  
  let newtype = aEvent.target.value;
  let scene = rend.getScene();
  
  if (rend.type_name=="*selection") {
    // conv sel to others
    let result = new Object();
    result.obj_id = obj.uid;
    result.rendtype = newtype;
    result.sel = rend.sel;
    result.ok = true;
    result.center = false;
    
    let sgnm = util.makeUniqName2(
      function (a) {return newtype+a; },
      function (a) {return scene.getRendByName(a);} );

    result.rendname = sgnm;

    // EDIT TXN START //
    scene.startUndoTxn("Change rend type");
    try {
      gQm2Main.doSetupRend(scene, result);
    }
    catch (e) {
      dd("***** ERROR: pushStyle "+e);
      debug.exception(e);
      scene.rollbackUndoTxn();
      return;
    }
    scene.commitUndoTxn();
    // EDIT TXN END //
    return;
  }
    
  let xmldat = gQm2Main.mStrMgr.toXML2(rend, newtype);

  let newrend = gQm2Main.mStrMgr.fromXML(xmldat, scene.uid);
  gQm2Main.setDefaultStyles(obj, newrend);

  // EDIT TXN START //
  scene.startUndoTxn("Change rend type");
  
  try {
    gQm2Main.deleteRendByID(rend.uid);
    obj.attachRenderer(newrend);
  }
  catch (e) {
    dd("***** ERROR: pushStyle "+e);
    debug.exception(e);
    scene.rollbackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

//////////////////////////////////////////////////
// Drag&drop impl for obj&rend

ws.onDragStart = function (event)
{
  dd("WSItem dragStart: "+event.target.localName);

  if (event.target.localName != "treechildren")
    return;

  var elem = this.mViewObj.getSelectedNode();
  var irow = this.mViewObj.getSelectedRow();
  dd("elem: "+elem.type);

  if (elem.type!="object" &&
      elem.type!="renderer" &&
      elem.type!="rendGroup")
    return;

  let dt = event.dataTransfer;
  dt.mozSetDataAt(ITEM_DROP_TYPE, elem, 0);

  event.stopPropagation();
};

ws.canDrop = function (elem, ori, dt)
{
  dd("ws candrop called");

  if (elem.type!="object" &&
      elem.type!="renderer" &&
      elem.type!="rendGroup")
    return false;

  var types = dt.mozTypesAt(0);
  if (types[0] != ITEM_DROP_TYPE)
    return false;

  var sourceElem = dt.mozGetDataAt(ITEM_DROP_TYPE, 0);
  dd("source elem type = "+sourceElem.type);
  dd("target elem type = "+elem.type);
  
  if (sourceElem.type=="renderer") {
    // Drop source=renderer

    // target should be rend or rendgrp
    if (elem.type!="renderer" && elem.type!="rendGroup")
      return false;

    if (elem.parent_id!=sourceElem.parent_id) {
      let destpar = this.findNodeByObjId(elem.parent_id);
      let srcpar = this.findNodeByObjId(sourceElem.parent_id);
      if (srcpar.type=="object") {
	if (destpar.type=="object") {
	  // cannot drop across different objects
	  return false;
	}
	else if (destpar.type=="rendGroup" &&
		 destpar.parent_id==sourceElem.parent_id) {
	  // Drop from obj to rendGrp
	  return true;
	}
      }
      else if (srcpar.type=="rendGroup") {
	if (destpar.type=="object" &&
	    srcpar.parent_id==elem.parent_id) {
	  // Drop from rendGrp to obj (in the same obj)
	  return true;
	}
	else if (destpar.type=="rendGroup" &&
		 destpar.parent_id==srcpar.parent_id) {
	  // Drop across different group in the same obj
	  return true;
	}
      }

      return false;
    }
    
    // Drop in object
    return true;
  }
  else if (sourceElem.type=="rendGroup") {
    // Drop source=renderer group

    // Target should be rend or rendgrp
    if (elem.type!="renderer" && elem.type!="rendGroup")
      return false;

    // Cannot drop rendgrp into other rendgrp
    if (elem.type=="rendGroup" && ori==0)
      return false;

    // Cannot drop rendgrp into other rendgrp
    if (elem.parent_id!=sourceElem.parent_id)
      return false;
  }
  else {
    // object
    if (elem.type!="object") return false;
    // if (ori==0) return false;
  }

  return true;
};

ws.drop = function (elem, ori, dt)
{
  dd("ws dropped");

  if (elem.type!="object" &&
      elem.type!="renderer" &&
      elem.type!="rendGroup")
    return;

  var types = dt.mozTypesAt(0);
  if (types[0] != ITEM_DROP_TYPE)
    return;

  var sourceElem = dt.mozGetDataAt(ITEM_DROP_TYPE, 0);

  let dst_type = elem.type;
  let dst_parent_id = elem.parent_id;
  let dst_id = elem.obj_id;
  if (elem.type=="rendGroup"&&ori==0) {
    // Drop into the renderer group
    dd("Drop into the renderer group, ori==0");
    dst_type="renderer";
    dst_parent_id = elem.obj_id;
    if (elem.childNodes.length>0)
      dst_id = elem.childNodes[0].obj_id;
  }

  dd("source elem type = "+sourceElem.type);
  dd("target elem type = "+dst_type);
  
  if (sourceElem.type=="renderer") {
    // Drop source=renderer

    // target should be rend or rendgrp
    if (dst_type!="renderer" && dst_type!="rendGroup")
      return;

    let id1 = sourceElem.obj_id;
    let id2 = dst_id;

    let srcpar = this.findNodeByObjId(sourceElem.parent_id);

    if (dst_parent_id!=sourceElem.parent_id) {
      let destpar = this.findNodeByObjId(dst_parent_id);
      // dd("destpar="+debug.dumpObjectTree(destpar));

      if (srcpar.type=="object") {
	if (destpar.type=="object") {
	  // cannot drop across different objects
	  dd("cannot drop across different objects");
	}
	else if (destpar.type=="rendGroup" &&
		 destpar.parent_id==sourceElem.parent_id) {
	  // Drop from obj to rendGrp
	  let grpname = destpar.orig_name;
	  dd("*** Drop from obj to rendGrp: "+grpname);
	  this.moveRendTo(sourceElem.parent_id, id1, id2, ori, grpname);
	}
      }
      else if (srcpar.type=="rendGroup") {
	if (destpar.type=="object" &&
	    srcpar.parent_id==dst_parent_id) {
	  // Drop from rendGrp to obj (in the same obj)
	  dd("Drop from rendGrp to obj (in the same obj)");
	  this.moveRendTo(dst_parent_id, id1, id2, ori, "");
	}
	else if (destpar.type=="rendGroup" &&
		 destpar.parent_id==srcpar.parent_id) {
	  // Drop across different group in the same obj
	  let grpname = destpar.orig_name;
	  dd("Drop across different group ("+grpname+") in the same obj");
	  this.moveRendTo(sourceElem.parent_id, id1, id2, ori, grpname);
	}
      }

      return;
    }

    if (srcpar.type=="rendGroup") {
      // Movement in the same group
      this.moveRendTo(srcpar.parent_id, id1, id2, ori);
      return;
    }

    this.moveRendTo(dst_parent_id, id1, id2, ori);
  }
  else if (sourceElem.type=="rendGroup") {
    // Drop source=renderer group
    // Target should be rend or rendgrp
    if (dst_type!="renderer" && dst_type!="rendGroup")
      return;
    // Cannot drop rendgrp into other rendgrp
    if (dst_type=="rendGroup" && ori==0)
      return;
    // Cannot drop rendgrp into other rendgrp
    if (dst_parent_id!=sourceElem.parent_id)
      return;

    let id1 = sourceElem.obj_id;
    let id2 = elem.obj_id;
    this.moveRendTo(dst_parent_id, id1, id2, ori);
  }
  else {
    // object
    if (dst_type!="object")
      return;

    // if (ori==0) return;

    let id1 = sourceElem.obj_id;
    let id2 = elem.obj_id;
    if (id1==id2) return;

    this.moveObjTo(id1, id2, ori);
  }

};

ws.moveRendTo = function (objid, id1, id2, ori, destgrp)
{
  var i;
  let rend1 = cuemol.getRenderer(id1);
  let rend2 = cuemol.getRenderer(id2);
  let rends=new Array();
  let imax = this._nodes.length;

  // set group name
  if (destgrp) {
    if (rend1.group!=destgrp) {
      dd("!!! Set group "+destgrp);
      rend1.group = destgrp;
    }
  }
  else {
    if (rend1.group!="") {
      dd("!!! Clear group");
      rend1.group = "";
    }
  }

  for (i=0; i<imax; ++i) {
    let nd = this._nodes[i];
    if (nd.type!="object") continue;
    if (nd.obj_id==objid) {
      let obj = cuemol.getObject(objid);
      dd("Rend list for obj:"+obj.name);
      let renduids = obj.rend_uids.split(",");
      for (i=0; i<renduids.length; ++i) {
	let uid = parseInt(renduids[i]);
	dd("Rend UID="+uid);
        rends.push( cuemol.getRenderer(uid) );
      }
      /*let rendnodes = nd.childNodes;
	for (i=0; i<rendnodes.length; ++i) {
        rends.push( cuemol.getRenderer(rendnodes[i].obj_id) );
	}*/
      break;
    }
  }
  
  if (rends.length==0)
    return;
  
  dd("Rend list OK");

  this._moveToImpl(rends, rend1, rend2, ori);
};

ws.moveObjTo = function (id1, id2, ori)
{
  var i;
  let obj1 = cuemol.getObject(id1);
  let obj2 = cuemol.getObject(id2);
  let objs=new Array();
  let imax = this._nodes.length;
  for (i=0; i<imax; ++i) {
    let nd = this._nodes[i];
    if (nd.type=="object") {
      objs.push( cuemol.getObject(nd.obj_id) );
    }
  }
  
  if (objs.length==0)
    return;
  
  this._moveToImpl(objs, obj1, obj2, ori);
};

ws._moveToImpl = function (rends, rend1, rend2, ori)
{
  let ord_1 = rend1.ui_order;
  let ord_2 = rend2.ui_order;

  dd("Move from "+ord_1+" to "+ord_2);

  // check orientation
  if (ori!=0) {
    let irow2 = -1;
    for (i=0; i<rends.length; ++i) {
      if (rends[i].uid==rend2.uid) {
        irow2 = i;
        break;
      }
    }
    if (irow2==-1) {
      dd("ERROR: irow for rend2 not found");
      return;
    }
  
    if (ord_1<ord_2) {
      if (ori==-1) {
        // shift-up the target location (rend2)
        if (irow2-1<0) {
          dd("ERROR -1: row before rend2 not found: "+irow2);
          return;
        }
        rend2 = rends[irow2-1];
        ord_2 = rend2.ui_order;
      }
    }
    else {
      if (ori==1) {
        // shift-down the target location (rend2)
        if (irow2+1<0) {
          dd("ERROR +1: row after rend2 not found: "+irow2);
          return;
        }
        rend2 = rends[irow2+1];
        ord_2 = rend2.ui_order;
      }
    }
  }

  dd("Rearranged Move from "+ord_1+" to "+ord_2);

  if (ord_1==ord_2) {
    // no movement
    dd("no movement");
    // Update of tree is required here,
    // because the group of rend can be changed...
    // return;
  }
  else if (ord_1<ord_2) {
    // move down
    dd("Move down");
    for (i=rends.length-1; i>=0; --i) {
      if (rends[i].ui_order==ord_2)
        break;
    }
    for (; i>=1; --i) {
      rend1 = rends[i-1];
      rend2 = rends[i];
      // swap UI order
      dd("Xchg: "+(i-1)+", "+i);
      let o = rend1.ui_order;
      rend1.ui_order = rend2.ui_order;
      rend2.ui_order = o;
      if (o==ord_1)
        break;
    }
  }
  else {
    // move up
    dd("Move up");
    for (i=0; i<rends.length-1; ++i) {
      if (rends[i].ui_order==ord_2)
        break;
    }
    for (; i<rends.length-1; ++i) {
      rend2 = rends[i];
      rend1 = rends[i+1];
      // swap UI order
      dd("Xchg: "+i+", "+(i+1));
      let o = rend1.ui_order;
      rend1.ui_order = rend2.ui_order;
      rend2.ui_order = o;
      if (o==ord_1)
        break;
    }
  }

  this.mViewObj.saveOpenState(this.mTgtSceneID);
  this.syncContents(this.mTgtSceneID);
};

//////////////////////////////////
// renderer group

ws.onNewRendGrp = function (aEvent)
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;

  dd("onNewRendGrp> elem.type="+elem.type);
  if (elem.type!="object")
    return;

  var id = elem.obj_id;

  var scene = this._mainWnd.currentSceneW;
  var sgnm = util.makeUniqName2(
    function (a) {return "group"+a; },
    function (a) {return scene.getRendByName(a);} );

  dd("suggested name="+sgnm);

  var grpname = util.prompt(window, "Name for new group: ", sgnm);
  if (grpname===null) return;

  var obj = scene.getObject(id);

  // EDIT TXN START //
  scene.startUndoTxn("Create renderer group: "+grpname);
  try {
    var rend = obj.createRenderer("*group");
    rend.name = grpname;
  }
  catch (e) {
    dd("***** ERROR: Create rend grp "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //

};

ws.onRenameRendGrp = function ()
{
  var elem = this.mViewObj.getSelectedNode();
  if (!elem) return;

  dd("onRenameRendGrp> elem.type="+elem.type);
  if (elem.type!="rendGroup")
    return;

  var grpname = util.prompt(window, "Change rend group name: ", elem.orig_name);
  if (grpname===null) return;
  if (grpname==elem.orig_name) return;

  var scene = this._mainWnd.currentSceneW;
  var rendgrp = cuemol.getRenderer(elem.obj_id);
  var rends = new Array();
  for (var i=0; i<elem.childNodes.length; ++i) {
    rends.push( cuemol.getRenderer(elem.childNodes[i].obj_id) );
  }

  // EDIT TXN START //
  scene.startUndoTxn("Change rend group name: "+grpname);
  try {
    rendgrp.name = grpname;
    for (var i=0; i<rends.length; ++i)
      rends[i].group = grpname;
  }
  catch (e) {
    dd("***** ERROR: Create rend grp "+e);
    debug.exception(e);
    scene.rollBackUndoTxn();
    return;
  }
  scene.commitUndoTxn();
  // EDIT TXN END //
};

ws.toggleVisibleRendGrp = function (aElem)
{
  if (aElem.type!="rendGroup")
    return;

  var scene = cuemol.getScene(this.mTgtSceneID);
  var rendgrp = cuemol.getRenderer(aElem.obj_id);
  var rends = new Array();
  for (var i=0; i<aElem.childNodes.length; ++i) {
    rends.push( cuemol.getRenderer(aElem.childNodes[i].obj_id) );
  }

  // EDIT TXN START //
  scene.startUndoTxn("Change group visibility");
  try {
    rendgrp.visible = !rendgrp.visible;
    for (var i=0; i<rends.length; ++i)
      rends[i].visible = rendgrp.visible;
  }
  catch (e) {
    dd("***** ERROR: Change group visibility "+e);
    debug.exception(e);
  }
  scene.commitUndoTxn();
  // EDIT TXN END //
};

} )();

}

