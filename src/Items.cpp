/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

//Note: adding Input fails and need to add indexNo for new dwg Items added
#include "Items.h"
#include <SafeString.h>
#include "ItemsIOsupport.h"
#include "FileSupport.h"
#include "CodeSupport.h"
#include "CodeSupport.h"
#include "DwgPanel.h"
#include "DwgsList.h"

const char Items::versionNo[10] = "1.0.47";

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;
// for later use
void Items::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

pfodLinkedPointerList<ValuesClass> Items::listDwgValues;
pfodLinkedPointerList<DwgName> Items::listOfDwgs; // list of dwg names

struct pfodDwgVALUES *Items::nullValuesPtr = NULL;
ValuesClass *Items::nullValuesClassPtr = NULL;
size_t Items::controlItemIdx = 0;  // when editing actions the controlItemIdx holds the idx of the parent touchZone
size_t Items::dwgItemIdx = 0;
bool Items::dwgItemIdxUpdated = false;
bool Items::controlItemIdxUpdated = false;

char Items::currentDwgName[DwgName::MAX_DWG_NAME_LEN + 1]; // the name of dwg currently being edited allow for adding _999 + '\0'
char Items::currentDwgNameBASE[DwgName::MAX_DWG_NAME_LEN + 1]; // the name of dwg currently being edited WITHOUT THE  (999)
const char Items::pushZeroFileName[15] = "item_pushZero";

pfodDwgs *Items::dwgsPtr = NULL;
pfodParser *Items::parserPtr = NULL;


ValuesClass *Items::dwgSelectedItemPtr; // never null
ValuesClass *Items::controlSelectedItemPtr; // never null

bool Items::seenUpdateIdx = false;
//size_t Items::writeIdxNo = 0;
const char Items::idxPrefix[] = "_idx_";
const char Items::updateIdxPrefix[] = "_update_idx_";
const char Items::touchZoneCmdPrefix[] = "_touchZone_cmd_";


// return idx of this item or getListDwgValuesSize() if not found, should not happen
size_t Items::getIndexOfDwgItem(ValuesClass *dwgItem) {
  return listDwgValues.getIndex(dwgItem);
}

// return dwg item referenced by indexNo OR NULL if not found or indexNo is 0;
//   Note: current iterator is saved and restored
ValuesClass *Items::getReferencedPtr(size_t indexNo) {
  //  if (debugPtr) {
  //    debugPtr->print("::getReferencedPtr("); debugPtr->print(indexNo); debugPtr->println(")");
  //  }
  ValuesClass *currentPtr = listDwgValues.getCurrentIterator(); // save current iterator
  //  if (debugPtr) {
  //    debugPtr->print("currentPtr "); debugPtr->println(currentPtr == NULL ? " NULL " : " not null");
  //  }
  size_t listIdx;
  size_t foundIdx = findReferredIdx(indexNo, listIdx); // returns idx() of dwgItem of dwg item found if match else 0,
  if (foundIdx != listDwgValues.size()) {
    ValuesClass * rtn = listDwgValues.get(foundIdx);
    //    if (debugPtr) {
    //      debugPtr->print("calling "); debugPtr->println("setCurrentIterator");
    //    }
    listDwgValues.setCurrentIterator(currentPtr); // reset iterator
    return rtn;
  } // ELSE
  return NULL;
}

const char* Items::getCurrentDwgName() {
  return currentDwgName;
}

void Items::trimDedupText(SafeString &sfName) {
  cSF(sfNameBase, sfName.length()); //DwgName::MAX_DWG_NAME_LEN);
  sfNameBase = sfName;
  sfNameBase.trim();
  bool foundDedupText = true;
  // look for (xxx) and remove
  if (sfNameBase[sfNameBase.length() - 1] == ')') {
    sfNameBase.removeLast(1);
    if (isdigit(sfNameBase[sfNameBase.length() - 1])) {
      sfNameBase.removeLast(1);
      if (isdigit(sfNameBase[sfNameBase.length() - 1])) {
        sfNameBase.removeLast(1);
        if (isdigit(sfNameBase[sfNameBase.length() - 1])) {
          sfNameBase.removeLast(1);
          if (sfNameBase[sfNameBase.length() - 1] == '(') {
            sfNameBase.removeLast(1);
          } else {
            foundDedupText = false;
          }
        } else {
          foundDedupText = false;
        }
      } else {
        foundDedupText = false;
      }
    } else {
      foundDedupText = false;
    }
  } else {
    foundDedupText = false;
  }
  if (!foundDedupText) {
    sfNameBase = sfName;
  }
  sfNameBase.trim();
  sfName = sfNameBase; // return result
}

const char* Items::getCurrentDwgNameBase() {
  cSFA(sfCurrentDwgNameBASE, currentDwgNameBASE);
  sfCurrentDwgNameBASE = currentDwgName;
  trimDedupText(sfCurrentDwgNameBASE);
  return currentDwgNameBASE;
}

/**
   findUniqueName
   updates argument to unique name by adding (001) or (002) etc if necessary
   params
   sfNewName - SafeString of name to check and make unique

*/
void Items::findUniqueName(SafeString &sfNewName) {
  trimDedupText(sfNewName); // trim of any (xxx) e.g. (001) etc
  cSF(sfPath, DwgName::MAX_DWG_NAME_LEN + 1); // for /
  sfPath = DIR_SEPARATOR;
  sfPath += sfNewName;

  // check for unique name if not add <space>(001) etc and check again until not found
  int i = 0; // pad with leading 0's
  File f = exists(sfPath.c_str());
  while (f && checkForZeroFile(getPath(f)) ) { // have dir with pushZero file
    f.close();
    i++;
    sfPath = DIR_SEPARATOR;
    sfPath += sfNewName;
    //sfPath += DWG_NAME_SEPERATOR;
    sfPath += "(";
    if (i < 10) {
      sfPath += "00";
    } else if (i < 100) {
      sfPath += "0";
    } // just add
    sfPath += i;
    sfPath += ")";
    f = exists(sfPath.c_str());
  }
  // here have available path
  if (debugPtr) {
    debugPtr->print("found newName : '"); debugPtr->print(sfPath); debugPtr->println("'");
  }
  // remove leading'/
  sfPath.remove(0, 1);
  sfNewName = sfPath; // update for return
}

/**
   newDwg
   creates new empty dwg with just the default pushZero

   params
   newName - SafeString with the new name, this will be modified if the name already exists
   sfErrMsg - updated with any error msgs

   Checks for existing dwg with newName and if one found add (001) or (002) etc to the newName to make it unique.
   The Dwg files are stored in directory that has the dwg's name.
*/
bool Items::newDwg(SafeString &sfNewName, SafeString &sfErrMsg) {
  sfErrMsg.clear();
  if (listOfDwgs.size() >= DwgsList::MAX_DWG_NAMES) {
    sfErrMsg = DwgsList::MAX_DWG_NAMES;
    sfErrMsg += " dwgs already";
    return false;
  }
  // should have been check before this call
  if (!cleanDwgName(sfNewName, sfErrMsg)) {
    return false;
  }

  findUniqueName(sfNewName);

  createNewDwg(sfNewName);
  //  listDir(DIR_SEPARATOR, debugPtr);
  // rebuild dwgs list
  initListOfDwgs();
  return true;
}


/**
   saveDwgAs
   saves current dwg to new name
   returns true on success, else false

   params
   newName - SafeString with the new name, this will be modified if the name already exists
   sfErrMsg - updated with any error msgs

   Checks for existing dwg with newName and if one found add (001) or (002) etc to the newName to make it unique.
   The Dwg files are stored in directory that has the dwg's name.

*/
bool Items::saveDwgAs(SafeString &sfNewName, SafeString &sfErrMsg) {
  sfErrMsg.clear();
  if (listOfDwgs.size() >= DwgsList::MAX_DWG_NAMES) {
    sfErrMsg = DwgsList::MAX_DWG_NAMES;
    sfErrMsg += " already saved";
    return false;
  }
  // should have been check before this call
  if (!cleanDwgName(sfNewName, sfErrMsg)) {
    return false;
  }

  findUniqueName(sfNewName);

  // copy current to new path
  // just change current name and do save
  // close the current dir
  //  String currentDir = DIR_SEPARATOR;
  //  currentDir += currentDwgName;
  //  File file = openDir(currentDir.c_str());
  //  file.close();

  cSFA(sfCurrentDwgName, currentDwgName);
  cSF(sfPath, DwgName::MAX_DWG_NAME_LEN + 1); // for /
  sfPath = DIR_SEPARATOR;
  sfPath += sfNewName;
  if (createDir(sfPath.c_str())) {
    // remove leading path separator
    sfPath.remove(0, 1);
    sfCurrentDwgName = sfPath;
    if (debugPtr) {
      debugPtr->print("created dir : '"); debugPtr->print(sfPath); debugPtr->println("'");
    }
    if (writeDwgName(sfCurrentDwgName)) {
      if (debugPtr) {
        debugPtr->print("wrote current name : '"); debugPtr->print(sfCurrentDwgName); debugPtr->println("'");
      }
      saveItems();
    }
  }
  //  listDir(DIR_SEPARATOR, debugPtr);
  // rebuild dwgs list
  initListOfDwgs();
  return true;
}

/**
   renameDwg
   renames the current dwg to new name
   returns true on success, else false

   params
   newName - String with the new name, this will be modified if the name already exists

   Checks for existing dwg with newName and if one found add _1 or _2 etc to the newName to make it unique.
   The Dwg files are stored in directory that has the dwg's name.

*/
bool Items::renameDwg(String &newName) {
  // search root dir for dirs/files with newName
  // if found search for first available newName_1, newName_2 etc and use that name
  // rename all the existing files to the new directory name
  return true;
}


ValuesClass *Items::getActionActionPtr(ValuesClass *valuesClassPtr) {
  if ((!valuesClassPtr) || (valuesClassPtr->type != pfodTOUCH_ACTION)) {
    return nullValuesClassPtr;
  }
  // else
  ValuesClass *rtn = valuesClassPtr->getActionActionPtr();
  if (!rtn) {
    return nullValuesClassPtr;
  }
  // else
  return rtn;
}

void Items::setSeenUpdateIdx() {
  seenUpdateIdx = true;
}

bool Items::haveSeenUpdateIdx() {
  return seenUpdateIdx;
}

/**
    setDwgItemIdx
    params
    idx - the listDwgValues idx to set the dwgSelectedItemPtr to
    doActionUpdates - update Action indexNo references if true (defaults true), else skip if false.

    returns true if saved items, else false.

    Sets dwgItemIdx and dwgSelectedItemPtr, optionally update Action reference to newly selected dwg item<br>
    If current Contol Panel item not an Action, then update the controlItemIdx and controlSelectedItemPtr as well<br>
    If current Control Panel item an Action then do not change it as user is editing that action<p>
    updateTouchActionInputIndexNo() and updateTouchActionIndexNo() will save if updated.<br>
    If either one saves, return true to allow caller to suppress extra save.

*/
bool Items::setDwgItemIdx(size_t idx, bool doActionUpdates) {
  bool rtn = false;
  //    if (debugPtr) {
  //      debugPtr->print("Set idx:"); debugPtr->println(idx);
  //    }
  if (idx > (listDwgValues.size() - 1)) {
    idx = (listDwgValues.size() - 1);
  }
  if (debugPtr) {
    debugPtr->print("::setDwgItemIdx idx:"); debugPtr->println(idx);
  }
  dwgItemIdx = idx;
  dwgSelectedItemPtr = getDwgValuesClassPtr(idx);
  if (doActionUpdates) { // defaults true
    //  setControlSelectedItemFromDwg(); // default setting
    bool rtn1 = updateTouchActionInputIndexNo(); // does nothing if current control NOT a touchActionInput else updates and saves
    bool rtn2 = updateTouchActionIndexNo(); // does nothing if current control NOT a touchActionInput else updates and saves
    if (debugPtr) {
      debugPtr->print("      updateTouchActionInputIndexNo() saved:"); debugPtr->print(rtn1 ? "Y" : "N");
      debugPtr->print("  updateTouchActionIndexNo() saved:"); debugPtr->print(rtn2 ? "Y" : "N");
      debugPtr->println();
    }
    rtn = rtn1 || rtn2;
  }
  if ((getControlSelectedItemType() == pfodTOUCH_ACTION) || (getControlSelectedItemType() == pfodTOUCH_ACTION_INPUT)) {
    return rtn; // do not update to control panel item
  } // else
  controlItemIdx = dwgItemIdx;
  controlSelectedItemPtr = dwgSelectedItemPtr;
  setControlItemIdxUpdated();
  return rtn;
}


void Items::setDwgItemIdxUpdated() {
  dwgItemIdxUpdated = true;
}
void Items::setControlItemIdxUpdated() {
  controlItemIdxUpdated = true;
}

// use this to close actions panels
void Items::setControlSelectedItemFromItemIdx() {
  if (debugPtr) {
    debugPtr->println("::setControlSelectedItemFromItemIdx() -- Close ");
    debugListDwgValues();
  }
  controlSelectedItemPtr = getDwgValuesClassPtr(controlItemIdx);
  dwgItemIdx = controlItemIdx;
  dwgSelectedItemPtr = getDwgValuesClassPtr(controlItemIdx);
  setControlItemIdxUpdated();
  setDwgItemIdxUpdated();
  if (debugPtr) {
    debugPtr->println("  at end of : setControlSelectedItemFromItemIdx()");
    debugListDwgValues();
    debugPtr->println("  =====================");
  }
}


Items::Items(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr) {
  parserPtr = _parserPtr;
  dwgsPtr = _dwgsPtr;
  nullValuesClassPtr = createDwgItemValues(pfodNONE);
  nullValuesPtr = nullValuesClassPtr->valuesPtr;
  dwgSelectedItemPtr = nullValuesClassPtr;
  controlSelectedItemPtr = nullValuesClassPtr;
}

bool Items::checkForZeroFile(const char* dirPath) {
  String path; // = DIR_SEPARATOR
  path += dirPath;
  path += DIR_SEPARATOR;
  path += pushZeroFileName;
  return exists(path.c_str());
}

void Items::removeEmptyDirs() {
  File rootDir = openDir(DIR_SEPARATOR);
  if (rootDir) {
    if (debugPtr) {
      debugPtr->print(" opened ");  debugPtr->println(getPath(rootDir));
    }
  } else {
    return;
  }
  File dir = getNextDir(rootDir);
  while (dir) {
    if (!checkForZeroFile(getPath(dir))) {
      deleteDir(getPath(dir));
      if (debugPtr) {
        debugPtr->print(" --> removed empty dir ");  debugPtr->println(getPath(dir));
      }
    } else {
      if (debugPtr) {
        debugPtr->print(" ++> kept Dwg dir ");  debugPtr->println(getPath(dir));
      }
    }
    dir.close();
    dir = getNextDir(rootDir);
  }
  rootDir.close();
}

bool Items::init() {
  // open files load existing items
  currentDwgName[0] = '\0';

  if (!initializeFileSystem()) { // lists dir
    return false;
  }

  //  if (debugPtr) {
  //    debugPtr->println("calling listDir");
  //    listDir(DIR_SEPARATOR, debugPtr);
  //    debugPtr->println("  =========================");
  //  }

  // remove empty dirs
  removeEmptyDirs();

  cSFA(sfCurrentDwgName, currentDwgName);
  readDwgName(sfCurrentDwgName);
  if (debugPtr) {
    debugPtr->print(" openDwg:"); debugPtr->println(sfCurrentDwgName);
  }
  if (sfCurrentDwgName.isEmpty()) {
    createDefaultDwg();
  }
  initListOfDwgs();

  openDwg();
  return true;
}

/**
   showDwgDelete
   returns true if ChoicePanel should show Delete button, else false

   Delete not shown if there are dwg items for this dwg
*/
bool Items::showDwgDelete() {
  return !haveItems();
}

/**
   createDefaultDwg
   returns true if new dwg "Dwg_1" created and loaded, else false, e.g. dwg alreay exists<br>

   Creates new empty dwg with name Dwg_1
*/
bool Items::createDefaultDwg() {
  cSF(sfNewName, DwgName::MAX_DWG_NAME_LEN);
  sfNewName = "Dwg_1";
  return createNewDwg(sfNewName);
}

/**
   createNewDwg
   returns true if new dwg created and loaded, else false, e.g. dwg alreay exists<br>

   params
   sfNewName - SafeString ref to new dwg name

   Creates new empty dwg with newName
*/
bool Items::createNewDwg(SafeString &sfNewName) {
  if (debugPtr) {
    debugPtr->print("::createNewDwg : "); debugPtr->println(sfNewName);
  }
  cSF(sfPath, DwgName::MAX_DWG_NAME_LEN + 1); // for /
  sfPath = DIR_SEPARATOR;
  sfPath += sfNewName;
  if (exists(sfPath.c_str()) && checkForZeroFile(sfPath.c_str())) {
    return true; // already have this name
  }
  // else
  listDwgValues.clear();
  DwgPanel::setZero(initDwgSize / 2, initDwgSize / 2, 1);
  if (createDir(sfPath.c_str())) {
    if (debugPtr) {
      debugPtr->print("created dir : '"); debugPtr->print(sfPath); debugPtr->println("'");
    }
    sfPath.remove(0, 1);
    cSFA(sfCurrentDwgName, currentDwgName);
    sfCurrentDwgName = sfPath;
    // remove leading path separator
    if (writeDwgName(sfCurrentDwgName)) {
      if (debugPtr) {
        debugPtr->print("wrote current name : '"); debugPtr->print(sfCurrentDwgName); debugPtr->println("'");
      }
      return saveItems();
    }
  }
  return false;
}

/**
   deleteDwg
   return true if current dwg deleted, else false

   If current dwg deleted, set current dwg to first one in list<br>
   OR if list empty, create "Dwg_1"

*/
bool Items::deleteDwg() {
  if (debugPtr) {
    debugPtr->println("::deleteDwg");
  }

  if (haveItems()) {
    if (debugPtr) {
      debugPtr->println(" haveItems return false");
    }
    return false; // not empty should not happen
  }
  if (!deleteFile(createPushZeroPath().c_str())) {
    //    if (debugPtr) {
    //      debugPtr->print(" delete file: ");  debugPtr->print(createPushZeroPath());  debugPtr->println(" failed ");
    //    }
    // ignore errors
  }
  String path = DIR_SEPARATOR;
  path = getCurrentDwgName();
  if (!deleteDir(path.c_str())) {  // <<< this seems to fail
    //    if (debugPtr) {
    //      debugPtr->print(" delete dir:  ");  debugPtr->print(path);  debugPtr->println(" failed ");
    //    }
    // ignore errors
  }
  path = getCurrentDwgName();
  if (!deleteDir(path.c_str())) {  // <<< this seems to fail also
    //    if (debugPtr) {
    //      debugPtr->print(" delete dir:  ");  debugPtr->print(path);  debugPtr->println(" failed ");
    //    }
    // ignore errors
  }

  // rebuild list
  initListOfDwgs();
  DwgName *firstNamePtr = listOfDwgs.getFirst();
  if (!firstNamePtr) {
    createDefaultDwg();
    initListOfDwgs();
    firstNamePtr = listOfDwgs.getFirst();
  }
  cSF(sfCurrentDwgName, DwgName::MAX_DWG_NAME_LEN);
  if (firstNamePtr != NULL) {
    sfCurrentDwgName = firstNamePtr->name();
    if (writeDwgName(sfCurrentDwgName)) {
      return openDwg(); // clear current items
    }
  } //else {
  if (debugPtr) {
    debugPtr->println(" Error: createDefaultDwg failed");
  }
  return false;
}

// for debugging only
void Items::closeAction(ValuesClass *touchActionPtr) {
  if (!debugPtr) {
    return;
  }
  printDwgValues("::closeAction >>>>>>> Close edit of ", touchActionPtr);
}

void Items::cleanUpAllActionReferenceNos() {
  for (size_t i = 0; i < listDwgValues.size(); i++) {
    ValuesClass *valuesClassPtr = listDwgValues.get(i);
    if (valuesClassPtr->type == pfodTOUCH_ZONE) {
      ValuesClass *actionPtr = valuesClassPtr->getActionsPtr();
      while (actionPtr) {
        cleanUpActionReferenceNos(actionPtr); // uses iterator so use for loop here
        actionPtr = actionPtr->getActionsPtr();
      }
    }
  }
}

bool Items::openDwg() {
  // load current dwg name
  cSFA(sfCurrentDwgName, currentDwgName);
  readDwgName(sfCurrentDwgName);
  if (debugPtr) {
    debugPtr->print("::openDwg:"); debugPtr->println(sfCurrentDwgName);
  }
  // clear current items
  listDwgValues.clear(); // remove and delete items

  String path = createPushZeroPath();
  size_t size = fileSize(path.c_str());
  if (size) {
    if (debugPtr) {
      char buffer[size + 1]; // +1 for '\0'
      readCharFile(path.c_str(), buffer, size + 1);
      debugPtr->print("Processing file: "); debugPtr->println(path);
      debugPtr->println(buffer);
    }
    File file = openFileForRead(path.c_str());
    if (!readPushZeroFromStream(&file)) { // updated DwgPanel if valid
      // error reading this file data
      file.close();
      deleteFile(path.c_str()); // remove invalid file
      // use defaults
      DwgPanel::setZero(initDwgSize / 2, initDwgSize / 2, 1);
    }
    // ignore this error
  }

  // open any existing items here
  // print current items
  size_t idx = 0;
  path = createItemFilePath(idx);
  size = fileSize(path.c_str());
  while (size) {
    if (debugPtr) {
      char buffer[size + 1]; // +1 for '\0'
      readCharFile(path.c_str(), buffer, size + 1);
      debugPtr->println();
      debugPtr->print("Processing file: "); debugPtr->println(path);
      debugPtr->println(buffer);
    }
    File file = openFileForRead(path.c_str());
    if (!appendDwgItemFromStream(&file)) {
      // error reading this file data
      file.close();
      deleteItemFile(idx); // deletes this file and move other ones down
    } else {
      idx++; // move on to next one
    }
    path = createItemFilePath(idx);
    size = fileSize(path.c_str());
  }
  if (debugPtr) {
    debugPtr->println();
    debugPtr->println("Loaded from files");
    debugListDwgValues();
    debugPtr->print("=============="); debugPtr->println();
  }
  // now set max used idx and set unique idx for other items
  cleanupIndices();
  cleanUpAllActionReferenceNos();
  cleanUpReferences();
  processTouchZonesForCovered();
  setDisplayIdx();
  return true;
}

/**
   initListOfDwgs
   initialized dwgsList<DwgName>

   Searches for dirs and checks for createPushZeroPath() exits
*/
bool Items::initListOfDwgs() {
  if (debugPtr) {
    debugPtr->println("::initListOfDwgs");
  }

  listOfDwgs.clear();

  File rootDir = openDir(DIR_SEPARATOR);
  if (rootDir) {
    //    if (debugPtr) {
    //      debugPtr->print(" opened ");  debugPtr->println(getPath(rootDir));
    //    }
  } else {
    return false;
  }
  File dir = getNextDir(rootDir);
  while (dir) {
    //    if (debugPtr) {
    //      debugPtr->print(" found dir ");  debugPtr->println(getPath(dir));
    //    }
    if (checkForZeroFile(getPath(dir))) {
      cSF(sfDwgName, strlen(getPath(dir)));
      sfDwgName = getPath(dir);
      sfDwgName.remove(0, 1); // remove the /
      DwgName *dwgNamePtr = new DwgName(sfDwgName.c_str());
      listOfDwgs.append(dwgNamePtr);
    }
    dir.close();
    dir = getNextDir(rootDir);
  }
  rootDir.close();
  if (debugPtr) {
    debugPtr->println("Dwgs found:");
    DwgName *dwgNamePtr = listOfDwgs.getFirst();
    if (!dwgNamePtr) {
      debugPtr->println("  NONE found");
    } else {
      while (dwgNamePtr) {
        debugPtr->println(dwgNamePtr->name());
        dwgNamePtr = listOfDwgs.getNext();
      }
    }
    debugPtr->println("  -------------");
  }
  if (listOfDwgs.size() == 0) {
    createDefaultDwg();
  }
  //  listDir(DIR_SEPARATOR, debugPtr); // iterate through dir but do not print
  return true;
}

// add idx to all non touchZones to force layer display
void Items::setDisplayIdx() {
  size_t idx = 1;
  ValuesClass *valuesClassPtr = listDwgValues.getFirst();
  while (valuesClassPtr) {
    valuesClassPtr->valuesPtr->idx = idx++;
    if (valuesClassPtr->type == pfodTOUCH_ZONE) {
      idx += 2; //  allow for 3 outline rectangles for touchZone display idx, idx+1, idx+2
    }
    valuesClassPtr = listDwgValues.getNext();
  }
  debugListDwgValues();
}

void Items::findMaxIndices(size_t &maxIdx, size_t &maxCmdNo) {
  maxIdx = 0;
  maxCmdNo = 0;
  ValuesClass *valuesClassPtr = listDwgValues.getFirst();
  // find max currently index
  while (valuesClassPtr) {
    if (valuesClassPtr->getIndexNo() > maxIdx) {
      maxIdx = valuesClassPtr->getIndexNo();
    }
    if (valuesClassPtr->getCmdNo() > maxCmdNo) {
      maxCmdNo = valuesClassPtr->getCmdNo();
    }
    valuesClassPtr = listDwgValues.getNext();
  }
  if (maxIdx > 0) {
    if (debugPtr) {
      debugPtr->print("set nextIndexNo to max used Idx:"); debugPtr->println(maxIdx);
    }
    ValuesClass::nextIndexNo = maxIdx; // setNextIndexNo increment first
  } else {
    if (debugPtr) {
      debugPtr->println("No indexNo used leave nextIndexNo as 0");
    }
  }
  if (maxCmdNo > 0) {
    if (debugPtr) {
      debugPtr->print("set nextCmdNo to max used cmdNo:"); debugPtr->println(maxCmdNo);
    }
    ValuesClass::nextCmdNo = maxCmdNo; // setNextCmdNo increment first
  } else {
    if (debugPtr) {
      debugPtr->println("No cmdNo used leave nextCmdNo as 0");
    }
  }
}

void Items::cleanupIndices() {
  if (debugPtr) {
    debugPtr->println("::cleanupIndices");
  }
  size_t maxIdx = 0;
  size_t maxCmdNo = 0;
  findMaxIndices(maxIdx, maxCmdNo);
  // now set indexNo for all that don't have them
  ValuesClass *valuesClassPtr = listDwgValues.getFirst();
  while (valuesClassPtr) {
    if (valuesClassPtr->getIndexNo() == 0) {
      valuesClassPtr->setNextIndexNo();
      if (debugPtr) {
        debugPtr->print("set indexNo :"); debugPtr->print(valuesClassPtr->getIndexNo()); debugPtr->print(" on "); debugPtr->println(valuesClassPtr->getTypeAsStr());
      }
    }
    if (valuesClassPtr->type == pfodTOUCH_ZONE) {
      if (valuesClassPtr->getCmdNo() == 0) {
        valuesClassPtr->setNextCmdNo();
        if (debugPtr) {
          debugPtr->println("found touchZone without a cmdNo add:"); debugPtr->println(valuesClassPtr->getCmdNo());
        }
      }
    }
    valuesClassPtr = listDwgValues.getNext();
  }
}

// clean up double referenced dwgItems
// if the current action has ref != 0 and another action also has the same refNo
// the zero the other actions referNo
// called from ControlPanel on close, uses
// getControlDisplayedItem() to pickup action and
// getControlSelectedItem to pickup touchZone
void Items::cleanUpActionReferenceNos(ValuesClass *touchActionPtr) {
  if (debugPtr) {
    debugPtr->println("::cleanUpActionReferenceNos");
  }
  bool needToSave = false;
  //  ValuesClass *touchZonePtr = getDwgValuesClassPtr(controlItemIdx);
  //printDwgValues("touchZonePtr", touchZonePtr);
  printDwgValues("touchActionPtr", touchActionPtr);
  //  if (touchZonePtr->type != pfodTOUCH_ZONE) {
  //    return; // an input nothing to do, actually an error
  //  }
  if (touchActionPtr->type != pfodTOUCH_ACTION) {
    return; // a touchActionInput so nothing to do
  }
  ValuesClass *actionActionPtr = touchActionPtr->getActionActionPtr();
  if (!actionActionPtr) {
    return; // actually an error
  }
  debugListDwgValues(" start of cleanUpActionReferenceNos");
  size_t tIdxNo = touchActionPtr->getIndexNo();
  size_t idxNo = actionActionPtr->getIndexNo();
  if (idxNo != tIdxNo) {
    if (debugPtr) {
      debugPtr->print("touchAction indexNo:"); debugPtr->print(tIdxNo); debugPtr->print(" does not equal action indexNo:"); debugPtr->println(idxNo);
    }
  }
  if (idxNo != 0) {
    // set referenced on this index no clear others with this index no
    // find the first touchZone
    ValuesClass *valuesClassPtr = listDwgValues.getFirst();
    while (valuesClassPtr) {
      printDwgValues(" checking for touchZone", valuesClassPtr);
      if (valuesClassPtr->type == pfodTOUCH_ZONE) {
        ValuesClass *actionPtr = valuesClassPtr->getActionsPtr();
        while (actionPtr) {
          printDwgValues("     found touchZone, checking touchAction", actionPtr);
          if ((actionPtr != touchActionPtr) && (actionPtr->type != pfodTOUCH_ACTION_INPUT)) {
            // check indexNo
            if (actionPtr->getIndexNo() == idxNo) {
              needToSave = true;
              actionPtr->setIndexNo(0); actionPtr->getActionActionPtr()->setIndexNo(0);
            }
          }
          actionPtr = actionPtr->getActionsPtr();
        }
      }
      valuesClassPtr = listDwgValues.getNext();
    }
  }
  debugListDwgValues(" end of cleanUpActionReferenceNos");
  if (needToSave) {
    saveItems();
  } else {
    if (debugPtr) {
      debugPtr->println("     no changes after cleanUpActionReferenceNos, do not save");
    }
  }
}

void Items::cleanUpReferences() {
  if (debugPtr) {
    debugPtr->println("::cleanUpReferences ");
  }
  ValuesClass *valuesClassPtr = listDwgValues.getFirst();
  while ((valuesClassPtr) && (valuesClassPtr->type != pfodTOUCH_ZONE)) {
    valuesClassPtr->isReferenced = false;
    valuesClassPtr = listDwgValues.getNext();
  }
  while (valuesClassPtr) { // touchZones
    //valuesClassPtr->isReferenced = false; //clear referenced in touchZones
    if (debugPtr) {
      debugPtr->print("checking "); debugPtr->print(valuesClassPtr->getTypeAsStr()); debugPtr->print(" :"); debugPtr->println(valuesClassPtr->getIndexNo());
    }
    ValuesClass *actionsPtr = valuesClassPtr->getActionsPtr();
    while (actionsPtr) {
      actionsPtr->isReferenced = false; //clear referenced in touchAction
      if (actionsPtr->getActionActionPtr()) {
        actionsPtr->getActionActionPtr()->isReferenced = false;
      }
      if (debugPtr) {
        debugPtr->print("  checking "); debugPtr->print(actionsPtr->getTypeAsStr()); debugPtr->print(" :"); debugPtr->println(actionsPtr->getIndexNo());
      }
      bool foundRef = false;
      ValuesClass *dwgValuesClassPtr = listDwgValues.getFirst();
      // loop until get to touchZones again
      while ((dwgValuesClassPtr) && (dwgValuesClassPtr->type != pfodTOUCH_ZONE)) {
        if (debugPtr) {
          debugPtr->print("   checking "); debugPtr->print(dwgValuesClassPtr->getTypeAsStr()); debugPtr->print(" :"); debugPtr->println(dwgValuesClassPtr->getIndexNo());
        }
        if (dwgValuesClassPtr->getIndexNo() == actionsPtr->getIndexNo()) {
          dwgValuesClassPtr->isReferenced = true; // force update idx for this item
          if (debugPtr) {
            debugPtr->print("   found ref "); debugPtr->print(dwgValuesClassPtr->getTypeAsStr()); debugPtr->print(" :"); debugPtr->println(dwgValuesClassPtr->getIndexNo());
          }
          foundRef = true;
        }
        dwgValuesClassPtr = listDwgValues.getNext();
      }
      if (!foundRef) { // clear idx for this action as dwg item not there
        actionsPtr->setIndexNo(0);
      }
      actionsPtr = actionsPtr->getActionsPtr();
    }
    listDwgValues.setCurrentIterator(valuesClassPtr); // force iterator to reset to here so get next works
    valuesClassPtr = listDwgValues.getNext();
  }
  debugListDwgValues(" after clean up referneces ");
}

void Items::processTouchZonesForCovered() {
  //  if (debugPtr) {
  //    debugPtr->println("::processTouchZonesForCovered processing touchZone overlays  -- ");
  //  }

  // check for touchZone overlays
  // Note touchZones processed for covered in the order they were first seen by pfodApp
  ValuesClass *touchZonePtr = listDwgValues.getFirst();
  // first clear all isCovered flags
  while (touchZonePtr) {
    if (touchZonePtr->type == pfodTOUCH_ZONE) {
      touchZonePtr->setCovered(false);  // clear all
    }
    touchZonePtr = listDwgValues.getNext();
  }

  touchZonePtr = listDwgValues.getFirst();
  // now do them all, this is an N-factorial process
  while (touchZonePtr) {
    if (touchZonePtr->type == pfodTOUCH_ZONE) {
      break; // found first lowest touchZone
    }
    touchZonePtr = listDwgValues.getNext();
  }
  if (!touchZonePtr) { // no touchZones
    return;
  }
  // else if (touchZonePtr) {
  // check for coverage
  size_t idx = listDwgValues.getIndex(touchZonePtr); // does NOT set iterator
  // iterator still set from above to touchZonePtr
  // save idx for reset of iterator below
  while (touchZonePtr) {
    size_t higherIdx = idx; // for debug output
    //    if (debugPtr) {
    //      debugPtr->print("Checking idx:"); debugPtr->println(idx);
    //    }
    // compare this to all the following
    ValuesClass * nextTouchZonePtr = listDwgValues.getNext();
    higherIdx++;
    while (nextTouchZonePtr) {
      checkCovered(nextTouchZonePtr, touchZonePtr); // compare higher to lower
      //      if (debugPtr) {
      //        debugPtr->print("idx:"); debugPtr->print(idx);  debugPtr->print(" is "); debugPtr->print(touchZonePtr->isCovered() ? "covered" : "not covered");
      //        debugPtr->print("  higherIdx:"); debugPtr->print(higherIdx);  debugPtr->print(" is "); debugPtr->println(nextTouchZonePtr->isCovered() ? "covered" : "not covered");
      //      }
      nextTouchZonePtr = listDwgValues.getNext();
      higherIdx++;
    }
    // start from next idx
    idx++;
    touchZonePtr = listDwgValues.get(idx);
    listDwgValues.setCurrentIterator(touchZonePtr);  // resets iterator
  }

  if (debugPtr) {
    // count covered
    ValuesClass *touchZonePtr = listDwgValues.getFirst();
    size_t countOfCovered = 0;
    while (touchZonePtr) {
      if (touchZonePtr->type == pfodTOUCH_ZONE) {
        if (touchZonePtr->isCovered()) {
          countOfCovered++;
        }
      }
      touchZonePtr = listDwgValues.getNext();
    }
    debugPtr->print("Number of touchZones covered, not active:"); debugPtr->println(countOfCovered);
  }
}

void Items::getBounds(struct pfodDwgVALUES * valuesPtr, float & leftX, float & topY, float & rightX, float & bottomY) {
  if (!valuesPtr) {
    return;
  }
  //  if (debugPtr) {
  //    debugPtr->print("Pos ");  debugPtr->print(" X:"); debugPtr->print(valuesPtr->colOffset); debugPtr->print(" Y:"); debugPtr->print(valuesPtr->rowOffset);
  //    debugPtr->print(" w:"); debugPtr->print(valuesPtr->width); debugPtr->print(" h:"); debugPtr->print(valuesPtr->height);
  //    if (valuesPtr->centered) {
  //      debugPtr->print(" Centered");
  //    }
  //    debugPtr->println();
  //  }

  leftX = valuesPtr->colOffset;
  if (valuesPtr->centered) {
    leftX -= valuesPtr->width / 2.0;
  }
  topY = valuesPtr->rowOffset;
  if (valuesPtr->centered) {
    topY -= valuesPtr->height / 2.0;
  }
  rightX = leftX + valuesPtr->width;
  bottomY = topY + valuesPtr->height;
  // sort so that left < right and top < bottom
  if (leftX > rightX) {
    float temp = rightX; rightX = leftX, leftX = temp;
  }
  if (topY > bottomY) {
    float temp = bottomY; bottomY = topY, topY = temp;
  }
  //  if (debugPtr) {
  //    debugPtr->print("Bounds:");  debugPtr->print(" "); debugPtr->print(leftX); debugPtr->print(" "); debugPtr->print(rightX);
  //    debugPtr->print(" "); debugPtr->print(topY); debugPtr->print(" "); debugPtr->print(bottomY);
  //    debugPtr->println();
  //  }
}

// check if covered and return true if isCovered() flag changes state
void Items::checkCovered(ValuesClass * tzHigher, ValuesClass * tzLower) {
  if ((!tzHigher) || (!tzLower)) {
    return;
  }

  struct pfodDwgVALUES *higherValuesPtr = tzHigher->valuesPtr;
  struct pfodDwgVALUES *lowerValuesPtr = tzLower->valuesPtr;
  if ((higherValuesPtr->colOffset == lowerValuesPtr->colOffset) && (higherValuesPtr->rowOffset == lowerValuesPtr->rowOffset) &&
      (higherValuesPtr->width == lowerValuesPtr->width) && (higherValuesPtr->height == lowerValuesPtr->height) &&
      ( (higherValuesPtr->centered && lowerValuesPtr->centered) ||  ((!higherValuesPtr->centered) && (!lowerValuesPtr->centered)) )) {
    // exactly the same place lower one is covered
    //    if (debugPtr) {
    //      debugPtr->println(" Lower covered by higher");
    //    }
    tzLower->setCovered(true);
    return;
  }
  // else need to offset by centered and check all complete coverage
  float H_leftX, H_topY, H_rightX, H_bottomY;
  float L_leftX, L_topY, L_rightX, L_bottomY;
  H_leftX = H_topY = H_rightX = H_bottomY = L_leftX = L_topY = L_rightX = L_bottomY = 0;
  getBounds(higherValuesPtr, H_leftX, H_topY, H_rightX, H_bottomY);
  getBounds(lowerValuesPtr, L_leftX, L_topY, L_rightX, L_bottomY);
  // now check if higher conatins lower, check this first as higher has priority if exactly covers
  if (H_leftX <= L_leftX && H_rightX >= L_rightX && H_topY <= L_topY && H_bottomY >= L_bottomY) {
    // H contains L
    tzLower->setCovered(true);
    return;
  }
  // else see if lower covers higher
  if (L_leftX <= H_leftX && L_rightX >= H_rightX && L_topY <= H_topY && L_bottomY >= H_bottomY) {
    // L contains H
    tzHigher->setCovered(true);
    return;
  }
  return ;
}

// set both xy but only process touchzones once
// calling method handles save
bool Items::setDwgSelectedOffset(float fx, float fy) {
  if (!haveItems()) {
    return false;
  }
  ValuesClass *valuesClassPtr = getDwgSelectedItem(); // never NULL
  //  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr; // never null
  // check for action
  ValuesClass *controlSelectedPtr = getControlSelectedItem();
  if (controlSelectedPtr->type == pfodTOUCH_ACTION) {
    valuesClassPtr = getControlDisplayedItem();
    //   valuesPtr = valuesClassPtr->valuesPtr;
  } else {
    // do normal stuff with dwg selected item
  }

  valuesClassPtr->setColOffset(fx);
  valuesClassPtr->setRowOffset(fy);
  if (getDwgSelectedItem()->type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
    return true;
  } // return false
  return false;
}

bool Items::setControlSelectedColOffset(float f) {
  if (!haveItems()) {
    return false;
  }
  getControlDisplayedItem()->setColOffset(f);
  if (getControlDisplayedItem()->type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
    return true;
  } // return false
  return false;
}

bool Items::setControlSelectedRowOffset(float f) {
  if (!haveItems()) {
    return false;
  }
  getControlDisplayedItem()->setRowOffset(f);
  if (getControlDisplayedItem()->type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
    return true;
  }
  return false;
}
bool Items::setControlSelectedWidth(float f) {
  if (!haveItems()) {
    return false;
  }
  getControlDisplayedItem()->setWidth(f);
  if (getControlDisplayedItem()->type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
    return true;
  }
  return false;
}
bool Items::setControlSelectedHeight(float f) {
  if (!haveItems()) {
    return false;
  }
  getControlDisplayedItem()->setHeight(f);
  if (getControlDisplayedItem()->type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
    return true;
  }
  return false;
}

bool Items::setControlSelectedCentered(bool _centered) {
  if (!haveItems()) {
    return false;
  }
  getControlDisplayedItem()->valuesPtr->centered = _centered;
  if (getControlDisplayedItem()->type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
    return true;
  }
  return false;
}

bool Items::readDwgName(SafeString & dwgName) {
  dwgName.clear();
  String path = createDwgNamePath();
  File file = openFileForRead(path.c_str());
  if (!file) {
    // no name
    return true;
  }
  // read in upto MAX_DWG_NAME_LEN
  dwgName.read(file);
  dwgName.trim();
  if (debugPtr) {
    debugPtr->print("::readDwgName '"); debugPtr->print(dwgName); debugPtr->println("'");
  }
  return true;
}

/**
    cleanDwgName
    returns false on error with sfErrMsg, quitely removes non-alph number non_ chars, replaces space with _
    params
    dwgName - the name to clean
    sfErrMsg - the returned errorMsg, empty if no errors.
*/
bool Items::cleanDwgName(SafeString & dwgName, SafeString & sfErrMsg) {
  sfErrMsg.clear();
  dwgName.trim();
  if (dwgName.isEmpty()) {
    sfErrMsg += "is blank";
    return false;
  }
  if (dwgName.length() > DwgName::MAX_DWG_BASE_NAME_LEN) {
    sfErrMsg += "length exceeds ";
    sfErrMsg += DwgName::MAX_DWG_BASE_NAME_LEN;
    return false;
  }
  // check for first char alpha and rest alphnumeric or _
  if (!isalpha(dwgName[0])) {
    sfErrMsg += "first char must be a..Z";
    return false;
  }
  // remove non-alpanumeric or _ or space
  dwgName.replace(' ', '_'); // replace space with _
  size_t i = 0;
  while (i < dwgName.length()) {
    if ( (isalnum(dwgName[i])) || (dwgName[i] == '_') ) {
      i++;
    } else {
      // remove
      dwgName.remove(i, 1);
    }
  }
  dwgName.trim();
  return true;
}

/**
   writeDwgName
   returns true if success, else false
   params
   dwgName - the name to save

   Checks for empty dwgName, but does not other checks
*/
bool Items::writeDwgName(SafeString & dwgName) {
  // remove special chars like non-ascii and \ and / and
  // trim to MAX_DWG_NAME_LEN
  dwgName.trim();
  if (dwgName.isEmpty()) {
    return false;
  }
  String path = createDwgNamePath();
  if (debugPtr) {
    debugPtr->print("::writeDwgName to "); debugPtr->println(path);
  }
  File file = openFileForRewrite(path.c_str());
  if (!file) {
    if (debugPtr) {
      debugPtr->println("::writeDwgName failed ");
    }
    // no name
    return false;
  }
  file.print(dwgName);
  return true;
}

bool Items::readPushZeroFromStream(Stream * inPtr) {
  if (!inPtr) {
    return false;
  }
  float xCenter = 0; float yCenter = 0; float scale = 1.0;
  if (ItemsIOsupport::readPushZero(*inPtr, xCenter, yCenter, scale)) {
    DwgPanel::setZero(xCenter, yCenter, scale);
    return true;
  }
  return false;
}

// call this after finding .send( and need to skip  );
bool Items::skipSend(Stream & in) {
  return ItemsIOsupport::skipRoundBracketSemiColon(in);
}

// returns new item from this Stream * and appends to listDwgValues and updates dwgItemIdx
// returns false on error
// uses defaults for any value missing
bool Items::appendDwgItemFromStream(Stream * inPtr) {
  //    if (debugPtr) {
  //      debugPtr->println("::appendDwgItemFromStream");
  //    }
  if (!inPtr) {
    return false;
  }

  ValuesClass *valuesClassPtr = ItemsIOsupport::readItem(*inPtr, ControlPanel::getDwgSize());
  if (valuesClassPtr && (valuesClassPtr->type == pfodTOUCH_ZONE)) {
    if (debugPtr) {
      debugPtr->println("Looking for following actions");
    }
    // look for need to loop her
    ValuesClass *actionPtr = ItemsIOsupport::readItem(*inPtr, ControlPanel::getDwgSize());
    while (actionPtr) {
      // check cmd is same
      if (actionPtr->type == pfodTOUCH_ZONE) {
        cSFP(sfActionTouchCmd, actionPtr->cmdStr);
        cSFP(sfTouchZoneCmd, valuesClassPtr->cmdStr);
        if (sfActionTouchCmd != sfTouchZoneCmd) {
          if (debugPtr) {
            debugPtr->print("Cmd for Action/touchZone '"); debugPtr->print(sfActionTouchCmd);
            debugPtr->print("' does not match cmd for touchZone '");  debugPtr->print(sfTouchZoneCmd);  debugPtr->println("'");
          }
          return false; // missmatched cmds
        }
        if (valuesClassPtr->getActionsPtr()) {
          // have action so cannot replace this
          if (debugPtr) {
            printDwgValues(" found following touchZone ", actionPtr);
            printDwgValues(" but existing touchZone has actions ", valuesClassPtr);
          }
          return false;
        } else {
          // replace it
          delete(valuesClassPtr);
          valuesClassPtr = actionPtr;
          valuesClassPtr->isIndexed = true;
          actionPtr = ItemsIOsupport::readItem(*inPtr, ControlPanel::getDwgSize());
          continue;
        }

      } else { // action
        cSFP(sfActionTouchCmd, actionPtr->touchCmdStr);
        cSFP(sfTouchZoneCmd, valuesClassPtr->cmdStr);
        if (sfActionTouchCmd != sfTouchZoneCmd) {
          if (debugPtr) {
            debugPtr->print("Cmd for Action/touchZone '"); debugPtr->print(sfActionTouchCmd);
            debugPtr->print("' does not match cmd for touchZone '");  debugPtr->print(sfTouchZoneCmd);  debugPtr->println("'");
          }
          return false; // missmatched cmds
        }
        size_t idx = 0;
        if (actionPtr->type == pfodTOUCH_ACTION_INPUT) {
          idx = 0;
        } else if (actionPtr->type == pfodTOUCH_ACTION) {
          if (!valuesClassPtr->hasAction1()) {
            idx = 1;
          } else if (!valuesClassPtr->hasAction2()) {
            idx = 2;
          } else if (!valuesClassPtr->hasAction3()) {
            idx = 3;
          } else if (!valuesClassPtr->hasAction4()) {
            idx = 4;
          } else { // if (!valuesClassPtr->hasAction5()) {
            idx = 5; //may fail
          }
        } else {
          printDwgValues("Item read not  touchAction or touchActionInput'", actionPtr);
          return false; // missmatched cmds
        }
        valuesClassPtr->appendAction(actionPtr, idx); // could fail
        actionPtr = ItemsIOsupport::readItem(*inPtr, ControlPanel::getDwgSize());
      }
    } // while (actionPtr)
  }
  if (valuesClassPtr == NULL) {
    if (debugPtr) {
      debugPtr->println("Read of Item failed, Out of Memory?");
    }
    return false;
  }
  listDwgValues.append(valuesClassPtr);
  setDwgItemIdxUpdated();
  setDwgItemIdx(listDwgValues.size() - 1);  // updates touchActionInput if necessary and saves, returns true if saves
  //    if (debugPtr) {
  //      debugPtr->print("appendDwgItemFromStream:"); debugPtr->print(valuesClassPtr->getTypeAsStr()); debugPtr->println();
  //      debugListDwgValues();
  //      debugPtr->print("=============="); debugPtr->println();
  //    }
  return true;
}

bool Items::writeCode() {
  parserPtr->print("{=<y><b>Generated Code</b>\n<-2><i><w>Use Back button to return}");
  return CodeSupport::writeCode(&listDwgValues, DwgPanel::getXCenter(), DwgPanel::getYCenter(), DwgPanel::getScale());
}

pfodDwgTypeEnum Items::getType(size_t idx) {
  return getDwgValuesClassPtr(idx)->type;
}

pfodDwgTypeEnum Items::getControlSelectedItemType() {
  if (!haveItems()) {
    return pfodNONE;
  } // else
  return controlSelectedItemPtr->type;
}

pfodDwgTypeEnum Items::getDwgSelectedItemType() {
  if (!haveItems()) {
    return pfodNONE;
  } // else
  return dwgSelectedItemPtr->type;
}

const char* Items::getItemTypeAsString(ValuesClass * valuesClassPtr) {
  return valuesClassPtr->getTypeAsStr();
}

String Items::createDwgNamePath() {
  String path(DIR_SEPARATOR);
  path += "pfodCurrentDwgName";
  return path;
}

String Items::createPushZeroPath() {
  String path(DIR_SEPARATOR);
  path += currentDwgName;
  if (strlen(currentDwgName)) {
    path += DIR_SEPARATOR;
  }
  path += pushZeroFileName;
  return path;
}

String Items::createItemFilePath(size_t idx) {
  String path(DIR_SEPARATOR);
  path += currentDwgName;
  if (strlen(currentDwgName)) {
    path += DIR_SEPARATOR;
  }
  path += "item_";
  path += idx; // note item_0 is the 'first' item file name
  return path;
}

String Items::getTempPathName() {
  String path(DIR_SEPARATOR);
  path += currentDwgName;
  if (strlen(currentDwgName)) {
    path += DIR_SEPARATOR;
  }
  path += "_tempItem";
  return path;
}

// resets flag on each call
bool Items::isControlSelectedItemIdxUpdated() {
  bool rtn = controlItemIdxUpdated;
  controlItemIdxUpdated = false;
  return rtn;
}

bool Items::isDwgSelectedItemIdxUpdated() {
  bool rtn = dwgItemIdxUpdated;
  dwgItemIdxUpdated = false;
  return rtn;
}

// returns true if updated and saved
/**
   updateTouchActionIndexNo
   returns true if update saved

   Called from setDwgItemIdx( ), which is called fromMbr>
   readItem(), decreaseDwgSelectedItemIdx(), increaseDwgSelectedItemIdx(), forwardDwgSelectedItemIdx(), backwardDwgSelectedItemIdx()<br>
   addTouchAction_1or2() -- problem here??<br>
   addTouchActionInput(), appendControlItem() and deleteControlItem()

*/
bool Items::updateTouchActionIndexNo() {
  if (controlSelectedItemPtr->type == pfodTOUCH_ACTION) {
    //   ValuesClass *currentCtrlItem = getDwgValuesClassPtr(controlItemIdx);
    ValuesClass *actionActionPtr = controlSelectedItemPtr->getActionActionPtr();
    if (debugPtr) {
      debugPtr->println("::updateTouchActionIndexNo ");
    }
    printDwgValues("    controlSelectedItemPtr", controlSelectedItemPtr);
    //   printDwgValues("    currentCtrlItem",currentCtrlItem);
    printDwgValues("    actionActionPtr", actionActionPtr);
    if (dwgSelectedItemPtr->type != pfodTOUCH_ZONE) {
      printDwgValues(" updateTouchActionIndexNo from ", dwgSelectedItemPtr);
      if (debugPtr) {
        debugPtr->print("set pfodTOUCH_ACTION indexNo to :"); debugPtr->println(dwgSelectedItemPtr->getIndexNo());
      }
      // currentCtrlItem->setIndexNo(dwgSelectedItemPtr->getIndexNo());
      controlSelectedItemPtr->setIndexNo(dwgSelectedItemPtr->getIndexNo());
      actionActionPtr->setIndexNo(dwgSelectedItemPtr->getIndexNo());
    } else { // moved out if normal items zero reference
      printDwgValues(" updateTouchActionIndexNo", dwgSelectedItemPtr);
      if (debugPtr) {
        debugPtr->print("is touchZone so set indexNo to 0 instead of "); debugPtr->println(dwgSelectedItemPtr->getIndexNo());
      }
      //  currentCtrlItem->setIndexNo(0);//dwgSelectedItemPtr->getIndexNo());
      controlSelectedItemPtr->setIndexNo(0);
      actionActionPtr->setIndexNo(0);
    }
    if (debugPtr) {
      debugPtr->println("   on return ");
    }
    printDwgValues("    controlSelectedItemPtr", controlSelectedItemPtr);
    //   printDwgValues("    currentCtrlItem",currentCtrlItem);
    printDwgValues("    actionActionPtr", actionActionPtr);

    saveSelectedItem();
    return true;
  } // else
  return false;
}

// returns true if updated and saved
bool Items::updateTouchActionInputIndexNo() {
  if (debugPtr) {
    debugPtr->println("::updateTouchActionInputIndexNo");
  }
  if (controlSelectedItemPtr->type == pfodTOUCH_ACTION_INPUT) {
    int idxNo = controlSelectedItemPtr->getIndexNo();
    if (dwgSelectedItemPtr->type == pfodLABEL) {
      if (debugPtr) {
        debugPtr->print("set pfodTOUCH_ACTION_INPUT indexNo to :"); debugPtr->println(dwgSelectedItemPtr->getIndexNo());
      }
      controlSelectedItemPtr->setIndexNo(dwgSelectedItemPtr->getIndexNo());
    } else {
      controlSelectedItemPtr->setIndexNo(0);
    }
    if (idxNo != controlSelectedItemPtr->getIndexNo()) {
      saveSelectedItem(); // save changes
      return true;
    }
    return false;
  } // else
  return false;
}

void Items::decreaseDwgSelectedItemIdx() {
  if (dwgItemIdx > 0) {
    dwgItemIdx--;
    setDwgItemIdxUpdated();
    if (!setDwgItemIdx(dwgItemIdx)) { // updates touchActionInput if necessary and saves returns true if saved
      //  saveSelectedItem(); // no need to save here save here
    }
  }
  if (debugPtr) {
    debugPtr->print("decreaseDwgSelectedItemIdx  dwgSelectedItemPtr type:"); debugPtr->println(dwgSelectedItemPtr->getTypeAsStr());
    debugPtr->print("decreaseDwgSelectedItemIdx  controlSelectedItemPtrtype:"); debugPtr->println(controlSelectedItemPtr->getTypeAsStr());
  }
}

void Items::increaseDwgSelectedItemIdx() {
  if (dwgItemIdx < listDwgValues.size() - 1) {
    dwgItemIdx++;
    setDwgItemIdxUpdated();
    if (!setDwgItemIdx(dwgItemIdx)) { // updates touchActionInput/touchAction if necessary and saves returns true if saved
      // saveSelectedItem(); // no need to save here
    }
  }
  if (debugPtr) {
    debugPtr->print("increaseDwgSelectedItemIdx  dwgSelectedItemPtr type:"); debugPtr->println(dwgSelectedItemPtr->getTypeAsStr());
    debugPtr->print("increaseDwgSelectedItemIdx  controlSelectedItemPtr type:"); debugPtr->println(controlSelectedItemPtr->getTypeAsStr());
  }
}

void Items::forwardDwgSelectedItemIdx() {
  // remove current item and add back in lower down i.e. higher layer
  // increase dwgItemIdx
  if (listDwgValues.size() <= 1) {
    return; //nothing to do
  }
  // at least 2 items
  if (dwgItemIdx >= (listDwgValues.size() - 1)) {
    return; // already at the top
  }
  if (getType(dwgItemIdx + 1) == pfodTOUCH_ZONE) {
    // do not move
    return;
  }
  if (debugPtr) {
    debugPtr->print("forwardDwgSelectedItemIdx before remove"); debugPtr->println();
    debugListDwgValues();
  }

  // dwgItemIdx < last item
  ValuesClass *itemPtr = listDwgValues.remove(dwgItemIdx);
  if (debugPtr) {
    debugPtr->print("forwardDwgSelectedItemIdx after remove"); debugPtr->println();
    debugListDwgValues();
  }
  // rename file just removed
  String pathItemIdx = createItemFilePath(dwgItemIdx);
  String tempPath = getTempPathName();
  renameFile(pathItemIdx.c_str(), tempPath.c_str());

  // dwgItemIdx+1 now moved down to dwgItemIdx
  dwgItemIdx++;
  String pathItemIdx_1 = createItemFilePath(dwgItemIdx); // now + 1
  renameFile(pathItemIdx_1.c_str(), pathItemIdx.c_str()); // +1 move down to
  listDwgValues.insertAt(itemPtr, dwgItemIdx);
  if (debugPtr) {
    debugPtr->print("forwardDwgSelectedItemIdx after insertAt"); debugPtr->println();
    debugListDwgValues();
  }

  renameFile(tempPath.c_str(), pathItemIdx_1.c_str()); // current item move up to +1
  setDwgItemIdxUpdated();
  setDwgItemIdx(dwgItemIdx);  // updated touchActionInput and saves if necessary, returns true if saved
  // if current control is touchActionInput that will do unnecessray save
  setDisplayIdx(); // could just swap idx but..
}

void Items::backwardDwgSelectedItemIdx() {
  // remove current item and add back in higher up  i.e. lower layer
  // decrease dwgItemIdx
  if (listDwgValues.size() <= 1) {
    return; //nothing to do
  }
  // at least 2 items
  if (dwgItemIdx == 0) {
    return; // nothing to do as at bottom
  }
  if (getType(dwgItemIdx) == pfodTOUCH_ZONE) {
    // do not move
    return;
  }
  //dwgItemIdx > 0
  if (debugPtr) {
    debugPtr->print("backwardDwgSelectedItemIdx before remove"); debugPtr->println();
    debugListDwgValues();
  }

  ValuesClass *itemPtr = listDwgValues.remove(dwgItemIdx);
  if (debugPtr) {
    debugPtr->print("backwardDwgSelectedItemIdx after remove"); debugPtr->println();
    debugListDwgValues();
  }

  String pathItemIdx = createItemFilePath(dwgItemIdx);
  String tempPath = getTempPathName();
  renameFile(pathItemIdx.c_str(), tempPath.c_str());

  // dwgItemIdx now moved down to dwgItemIdx
  dwgItemIdx--;
  String pathItemIdx_1 = createItemFilePath(dwgItemIdx); // now -1
  renameFile(pathItemIdx_1.c_str(), pathItemIdx.c_str()); // +1 move down to

  listDwgValues.insertAt(itemPtr, dwgItemIdx); // insert before previous dwgItemIdx
  if (debugPtr) {
    debugPtr->print("backwardDwgSelectedItemIdx after insertAt"); debugPtr->println();
    debugListDwgValues();
  }
  renameFile(tempPath.c_str(), pathItemIdx_1.c_str()); // current item move up to +1

  setDwgItemIdxUpdated();
  setDwgItemIdx(dwgItemIdx);  // updated touchActionInput and saves if necessary, returns true if saved
  // if current control is touchActionInput that will do unnecessray save
  setDisplayIdx(); // could just swap idx but..
}

bool Items::savePushZero() {
  String path = createPushZeroPath();
  if (debugPtr) {
    debugPtr->print("::savePushZero to "); debugPtr->println(path);
  }
  File out = openFileForRewrite(path.c_str());
  if (!out) {
    if (debugPtr) {
      debugPtr->println("savePushZero failed ");
    }
    return false;
  }

  out.print("pushZero("); out.print(DwgPanel::getXCenter(), 2); out.print(", ");
  out.print(DwgPanel::getYCenter(), 2); out.print(", "); out.print(DwgPanel::getScale(), 2); out.print(");");
  //out.close(); will close on going out of scope
  return true;
}


void Items::clearIdxNos() {
  // call this before each output
  //  idxNo = 0;
  //  touchZoneCmdNo = 0;
  //writeIdxNo = 0;
  seenUpdateIdx = false;
}

bool Items::getTouchZoneCmdName(SafeString & sfResult, ValuesClass * valuesClassPtr) {
  sfResult.clear();
  if (valuesClassPtr->type != pfodTOUCH_ZONE) {
    return !sfResult.isEmpty(); // empty
  }
  //else
  sfResult = touchZoneCmdPrefix;
  sfResult += valuesClassPtr->getCmdNo(); // all touchzones have cmdNo on load create
  return !sfResult.isEmpty();
}

// returns true in the currently selected item has an idx
// loops from start to dwgItemIdx
bool Items::getIdxName(SafeString & sfResult, ValuesClass * valuesClassPtr) {
  sfResult.clear();
  if (valuesClassPtr->isIndexed)  {
    setSeenUpdateIdx(); // need all the following to be indexed
    sfResult = updateIdxPrefix;
    sfResult += valuesClassPtr->getIndexNo(); // get this unique idx perhaps set by load
  } else if (valuesClassPtr->isReferenced) {
    setSeenUpdateIdx(); // need all the following to be indexed
    sfResult = updateIdxPrefix;
    sfResult += valuesClassPtr->getIndexNo(); // get this unique idx perhaps set by load
    //    setSeenUpdateIdx(); // need all the following to be indexed
    //    sfResult = idxPrefix;
    //    size_t itemIndex = getIndexOfDwgItem(valuesClassPtr);
    //    sfResult += itemIndex; // just use next available if not explicitly indexed
  } else if (haveSeenUpdateIdx()) {
    sfResult = idxPrefix;
    size_t itemIndex = getIndexOfDwgItem(valuesClassPtr);
    sfResult += itemIndex; // just use next available if not explicitly indexed
  }
  return !sfResult.isEmpty();
}


bool Items::saveItems() {
  if (debugPtr) {
    debugPtr->println("::saveItems  +++++++++ SaveItems ++++++++");
  }
  savePushZero();
  if (!haveItems()) {
    return true; // nothing to do
  }
  clearIdxNos(); // call this first
  cleanUpReferences();
  cSF(sfIdxName, 30);
  size_t idx = 0;
  for (size_t i = 0; i < listDwgValues.size(); i++) {
    ValuesClass *valuesClassPtr = listDwgValues.get(i);
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      getIdxName(sfIdxName, valuesClassPtr);
    } else {
      getTouchZoneCmdName(sfIdxName, valuesClassPtr);
    }
    saveItem(idx++, sfIdxName);
  }
  return true;
}

// due to indices need to rewrite ALL the times
bool Items::saveSelectedItem() {
  return saveItems();
}

bool Items::saveItem(size_t idx, SafeString & sfIdxName) {
  ValuesClass *valuesClassPtr = getDwgValuesClassPtr(idx); // never null
  if (valuesClassPtr->type == pfodNONE) {
    return false; // don't create empty file
  }
  String path = createItemFilePath(idx);
  if (debugPtr) {
    debugPtr->print("::saveItem to "); debugPtr->println(path);
  }
  File out = openFileForRewrite(path.c_str());
  if (!out) {
    if (debugPtr) {
      debugPtr->println("saveItem failed");
    }
    return false;
  }
  bool rtn = writeItem((Stream*)&out, idx, sfIdxName);
  out.close();
  //out.close(); will close on going out of scope  return rtn;
  if (debugPtr) {
    debugPtr->println();
  }
  return rtn;
}

size_t Items::getListDwgValuesSize() {
  return listDwgValues.size();
}

void Items::debugListDwgValues(const char *title ) { // defaults to NULL
  if (!debugPtr) {
    return;
  }
  if (title) {
    debugPtr->print(" ========== "); debugPtr->print(title); debugPtr->println(" ========== ");
  }
  debugPtr->print(" dwgItemIdx:"); debugPtr->print(dwgItemIdx);
  debugPtr->print(" controlItemIdx:"); debugPtr->print(controlItemIdx); debugPtr->println();
  for (size_t i = 0; i < Items::getListDwgValuesSize(); i++) {
    ValuesClass *valuesClassPtr = getDwgValuesClassPtr(i);
    printDwgValues("", valuesClassPtr); // never null
    if (valuesClassPtr->type == pfodTOUCH_ZONE) {
      ValuesClass *actionPtr = valuesClassPtr->getActionsPtr();
      while (actionPtr) {
        printDwgValues("", actionPtr); // never null
        actionPtr = actionPtr->getActionsPtr();
      }
    }
  }
  if (title) {
    debugPtr->print(" ========== End of "); debugPtr->print(title); debugPtr->println(" ========== ");
  }
}

void Items::formatLabel(ValuesClass * labelPtr, SafeString & sfFullText) {
  sfFullText.clear();
  if (labelPtr->type != pfodLABEL) {
    return; // empty
  }
  struct pfodDwgVALUES* valuesPtr = labelPtr->valuesPtr;
  if (!valuesPtr) {
    return;
  }
  if (valuesPtr->text != NULL) { // encode
    pfodDwgsBase::encodeText(&sfFullText, 1, valuesPtr->text);
  }
  if (valuesPtr->haveReading != 0) {
    float f = valuesPtr->reading;
    if (f < 0.0f) {
      sfFullText.print('-');
      f = -f;
    }
    pfodDwgsBase::printFloatDecimals(&sfFullText, f, valuesPtr->decPlaces);
    if (valuesPtr->units != NULL) { // encode
      pfodDwgsBase::encodeText(&sfFullText, 1, valuesPtr->units);
    }
  }
}

void Items::print_pfodBasePtr(const char* title, pfodDwgsBase * basePtr) {
  if (!debugPtr) {
    return;
  }
  if ((title) && (*title)) {
    debugPtr->print(title); debugPtr->print(" ");
  }
  if (!basePtr) {
    debugPtr->println("NULL");
    return;
  }
  basePtr->out = debugPtr;
  if (basePtr->getValuesPtr()->actionPtr) {
    basePtr->getValuesPtr()->actionPtr->out = debugPtr;
  }
  basePtr->send();
  debugPtr->println();
  basePtr->out = parserPtr;
  if (basePtr->getValuesPtr()->actionPtr) {
    basePtr->getValuesPtr()->actionPtr->out = parserPtr;
  }
}

/**
   sendDwgHideSelectedItemAction
   params
   buttonCmd - the cmd to send when this action triggered

   This method sends the actions to hide/show the current item when the (S) button pressed<br>
   touchZones use their valuesPtr->idx, + 1, + 2 <br>
   When idx's are assigned touchZones get an increment of 3 to allow for this<p>

   touchAction actions the have references use the idx of their reference<br>
   Unreferenced touchAction actions used the unReferencedTouchItemIdx (i.e. 1010)<br>
   If just hiding an item (non-touchZone) then make (S) show the hidden item

*/
void Items::sendDwgHideSelectedItemAction(const char* buttonCmd) {

  ValuesClass *valuesClassPtr = dwgSelectedItemPtr;
  if (!valuesClassPtr) {
    return;
  }
  printDwgValues("::sendDwgHideSelectedItemAction dwgSelectedItemPtr", dwgSelectedItemPtr);
  dwgsPtr->touchAction().cmd(buttonCmd).action(
    dwgsPtr->hide().idx(valuesClassPtr->valuesPtr->idx)
  ).send();
  pfodDwgsBase *dwgBasePtr = NULL;

  if (getDwgSelectedItemType() == pfodTOUCH_ZONE) { // could be editing either a touchZone or touchActionInput or touchAction action
    dwgsPtr->touchAction().cmd(buttonCmd).action(
      dwgsPtr->hide().idx(valuesClassPtr->valuesPtr->idx + 1)
    ).send();
    dwgsPtr->touchAction().cmd(buttonCmd).action(
      dwgsPtr->hide().idx(valuesClassPtr->valuesPtr->idx + 2)
    ).send();
    printDwgValues("    dwgSelectedItemPtr==pfodTOUCH_ZONE, getControlDisplayedItem() ", getControlDisplayedItem());

    if (getControlSelectedItem()->type == pfodTOUCH_ACTION) {
      if (debugPtr) {
        debugPtr->print("    getControlSelectedItem==pfodTOUCH_ACTION, Editing a touchAction, since dwg item is a touchZone, then should be unrefrenced action just hide it");
      }
      dwgBasePtr = &(dwgsPtr->touchAction().cmd(buttonCmd).action(dwgsPtr->hide().idx(unReferencedTouchItemIdx)));
      if (dwgBasePtr) {
        const char *orgtouchCmdStr = dwgBasePtr->getValuesPtr()->touchCmdStr; // touchActionInput used touchCmdStr for link to touchZone
        dwgBasePtr->getValuesPtr()->touchCmdStr = buttonCmd;
        dwgBasePtr->send();
        print_pfodBasePtr("   editing touchAction send action ", dwgBasePtr);
        // restore orgCmdStr
        dwgBasePtr->getValuesPtr()->touchCmdStr = orgtouchCmdStr;
      }

    } else if (getControlDisplayedItem()->type == pfodTOUCH_ACTION_INPUT) {
      printDwgValues("   getControlDisplayedItem==pfodTOUCH_ACTION_INPUT, editing a touchActionInput ", getControlDisplayedItem());
      ValuesClass *actionPtr = valuesClassPtr->getActionInputPtr();
      // send actionInput for buttonCmd
      dwgBasePtr = createDwgItemPtr(actionPtr); // initializes pfodDwgs values struct but then replace it with actionInputPtr->valuesPtr
      if (dwgBasePtr) {
        // find idx to display default text
        size_t refIdx = 0;
        if (actionPtr->getIndexNo() != 0) {
          if (debugPtr) {
            debugPtr->print(" error editing a touchActionInput while touchZone selected in dwgPanel but have a reference indexNo : ");
            debugPtr->println(actionPtr->getIndexNo());
          }
          ValuesClass *referredPtr = getReferencedPtr(actionPtr->getIndexNo());
          if ((referredPtr) && (referredPtr->type == pfodLABEL)) {
            refIdx = referredPtr->valuesPtr->idx;  // idx ref in dwgPanel
          }
        }
        ((pfodTouchActionInput*)dwgBasePtr)->textIdx(refIdx); //  may be zero
        if (dwgBasePtr) {
          const char *orgtouchCmdStr = dwgBasePtr->getValuesPtr()->touchCmdStr; // touchActionInput used touchCmdStr for link to touchZone
          dwgBasePtr->getValuesPtr()->touchCmdStr = buttonCmd;
          dwgBasePtr->send();
          print_pfodBasePtr(" send actionInput ", dwgBasePtr);
          // restore orgCmdStr
          dwgBasePtr->getValuesPtr()->touchCmdStr = orgtouchCmdStr;
        }
      }
    } else { // getControlSelectedItem is NOT a touchZone, getControlDisplayedItem is the actionAction NOT the
      printDwgValues("  editing a touchAction action ", getControlDisplayedItem());
      ValuesClass *actionPtr = valuesClassPtr->getActionsPtr();
      while (actionPtr) {
        pfodDwgsBase *dwgBasePtr = NULL;
        if (actionPtr->type == pfodTOUCH_ACTION_INPUT) {
          // send actionInput for buttonCmd
          printDwgValues("  touchActionInput", actionPtr);
          dwgBasePtr = createDwgItemPtr(actionPtr); // initializes pfodDwgs values struct but then replace it with actionInputPtr->valuesPtr
          if (dwgBasePtr) {
            // find idx to display default text
            size_t refIdx = 0;
            if (actionPtr->getIndexNo() != 0) {
              ValuesClass *referredPtr = getReferencedPtr(actionPtr->getIndexNo());
              if ((referredPtr) && (referredPtr->type == pfodLABEL)) {
                refIdx = referredPtr->valuesPtr->idx;  // idx ref in dwgPanel
              }
            }
            ((pfodTouchActionInput*)dwgBasePtr)->textIdx(refIdx); // may be zero
          }
        } else if (actionPtr->type == pfodTOUCH_ACTION) {
          printDwgValues("  touchAction", actionPtr);
          // get the action
          ValuesClass *actionActionPtr = actionPtr->getActionActionPtr();
          if (actionActionPtr) {
            size_t refIdx = 0;
            if (actionPtr->getIndexNo() != 0) {
              ValuesClass *referredPtr = getReferencedPtr(actionPtr->getIndexNo());
              if (referredPtr)  {
                refIdx = referredPtr->valuesPtr->idx;  // idx ref in dwgPanel
              }
            }
            actionActionPtr->valuesPtr->idx = refIdx; // may be zero
            dwgBasePtr = createDwgItemPtr(actionPtr); // initializes pfodDwgs values struct but then replace it with actionInputPtr->valuesPtr
            if (refIdx == 0) {
              if (actionActionPtr->type == pfodHIDE) {
                // skip this one
                dwgBasePtr = NULL;
              } else {
                refIdx = unReferencedTouchItemIdx;
              }
            }
          } //          if (actionActionPtr) {
        } else { //  } else if (actionPtr->type == pfodTOUCH_ACTION) {
          if (debugPtr) {
            debugPtr->print(" Invalid type for Action : "); debugPtr->println(actionPtr->getTypeAsStr());
          }
        }
        if (dwgBasePtr) {
          const char *orgtouchCmdStr = dwgBasePtr->getValuesPtr()->touchCmdStr; // touchActionInput used touchCmdStr for link to touchZone
          dwgBasePtr->getValuesPtr()->touchCmdStr = buttonCmd;
          dwgBasePtr->send();
          print_pfodBasePtr(" send action ", dwgBasePtr);
          // restore orgCmdStr
          dwgBasePtr->getValuesPtr()->touchCmdStr = orgtouchCmdStr;
        }
        actionPtr = actionPtr->getActionsPtr();
      } //      while (actionPtr) {
    }
  } else {
    // if (getDwgSelectedItemType() != pfodTOUCH_ZONE)
    printDwgValues("  getDwgSelectedItemType()!= pfodTOUCH_ZONE, getControlSelectedItem()", getControlSelectedItem());
    if (getControlSelectedItem()->type == pfodTOUCH_ZONE) {
      if (debugPtr) {
        debugPtr->println(" Err Dwg Selected Item not a touchZone AND control Selected Item a touchZone");
      }

    } else if (getControlSelectedItem()->type == pfodTOUCH_ACTION) {
      printDwgValues("  getControlSelectedItem()==pfodTOUCH_ACTION, dwgPanel selected not a touchZone and control selected item a touchAction, getControlDisplayedItem()", getControlDisplayedItem());
      // here we need to set action to show the currently hiden item
      if (getControlSelectedItem()->getIndexNo() != 0) {
        ValuesClass *referredPtr = getReferencedPtr(getControlSelectedItem()->getIndexNo());
        printDwgValues(" referredPtr", referredPtr);
        if (referredPtr)  {
          // action idx should equal this idx
          size_t actionIdx = getControlDisplayedItem()->valuesPtr->idx;
          size_t refIdx = referredPtr->valuesPtr->idx;  // idx ref in dwgPanel
          if (actionIdx != refIdx) {
            if (debugPtr) {
              debugPtr->print(" error action idx:"); debugPtr->print(actionIdx); debugPtr->print(" not equal to refIdx:"); debugPtr->println(refIdx);
            }
          }
          if (debugPtr) {
            debugPtr->println(" send show for refreenced item");
          }
          pfodDwgsBase *dwgBasePtr = createDwgItemPtr(referredPtr);
          if (dwgBasePtr) {
            pfodTouchAction action = dwgsPtr->touchAction().cmd(buttonCmd).action(*dwgBasePtr);
            action.send();
            print_pfodBasePtr(" sending action ", &action);
          }
        }
      } // else no referenced item to show
    } else if (getControlSelectedItem()->type == pfodTOUCH_ACTION_INPUT) {
       printDwgValues("  getControlSelectedItem()== pfodTOUCH_ACTION_INPUT, ", getControlSelectedItem());
      // here we need to set action to show the currently hiden item
      if (getControlDisplayedItem()->getIndexNo() != 0) {
        printDwgValues("  getControlDisplayedItem(), ", getControlDisplayedItem());
        ValuesClass *referredPtr = getReferencedPtr(getControlDisplayedItem()->getIndexNo());
        printDwgValues(" touchActionInput referredPtr", referredPtr);
        if (referredPtr)  {
          // action idx should equal this idx
          size_t actionIdx = getControlDisplayedItem()->valuesPtr->idx;
          size_t refIdx = referredPtr->valuesPtr->idx;  // idx ref in dwgPanel
          if (actionIdx != refIdx) {
            if (debugPtr) {
              debugPtr->print(" error actionInput idx:"); debugPtr->print(actionIdx); debugPtr->print(" not equal to refIdx:"); debugPtr->println(refIdx);
            }
          }
        }
      }
      pfodDwgsBase *dwgBasePtr = createDwgItemPtr(getControlSelectedItem());
      if (dwgBasePtr) {
        const char *orgtouchCmdStr = dwgBasePtr->getValuesPtr()->touchCmdStr; // touchActionInput used touchCmdStr for link to touchZone
        uint16_t origTextIdx = dwgBasePtr->getValuesPtr()->idx;
        dwgBasePtr->getValuesPtr()->idx = unReferencedTouchItemIdx;
        dwgBasePtr->getValuesPtr()->touchCmdStr = buttonCmd;
        dwgBasePtr->send();
        print_pfodBasePtr(" send actionInput ", dwgBasePtr);
        dwgBasePtr->getValuesPtr()->idx = origTextIdx;
        // restore orgCmdStr
        dwgBasePtr->getValuesPtr()->touchCmdStr = orgtouchCmdStr;
      }

    } else {
      if (debugPtr) {
        debugPtr->println(" Dwg Selected Item not a touchZone AND control Selected Item not a touchZone, touchAction, touchActionInput, just hide item");
      }
    }
  }
}

void Items::sendDwgUnhideSelectedItem(const char* buttonCmd) {
  (void)(buttonCmd);

  uint16_t selectedIdx = getDwgSelectedItem()->valuesPtr->idx;
  dwgsPtr->unhide().idx(selectedIdx).send();
  if (getDwgSelectedItemType() == pfodTOUCH_ZONE) {
    dwgsPtr->unhide().idx(selectedIdx + 1).send();
    dwgsPtr->unhide().idx(selectedIdx + 2).send();
  }
}

/**
   findReferredIdx.
   returns the listDwgValues idx of the found item OR listDwgValues.size() if not found
   The listIdx reference arguement is updated with the listDwgValues idx of the last non-touch item OR the first touch item if there are no non-touch items
   Note: current iterator is saved and restored

   params
   indexNoToBeFound - the indexNo to search for, 0 never matches and always returns listDwgValues.size()
   listIdx - returned - updated with foundIdx if found else set to the listDwgValues idx of the last non-touch item OR the first touch item if there are no non-touch items
*/
// returns the list index of the item with the matching indexNo if found, else listDwgValues.size() if not found
// NOTE: this depends on setDisplayIdx() being kept upto date after adds/removes/moveUp/moveDown
// if the match indexNo is NOT a touchZone the listIdx is the same as the return, else it is the last non-touchZone index, or the first touchZone if not non-touchZone items
size_t Items::findReferredIdx(size_t indexNoToBeFound, size_t &listIdx) {
  size_t idx = 0;
  listIdx = 0;
  size_t idxFound = listDwgValues.size();
  //  if (debugPtr) {
  //    debugPtr->print("::findReferredIdx(");    debugPtr->print(indexNoToBeFound); debugPtr->print(")  initial idxFound:");    debugPtr->println(idxFound);
  //  }
  // else
  ValuesClass *currentPtr = listDwgValues.getCurrentIterator();
  ValuesClass *itemIterator = listDwgValues.getFirst();
  while (itemIterator) {
    //    if ((debugPtr)) { // && (indexNoToBeFound != 0)) {
    //      debugPtr->print("at idx:"); debugPtr->print(idx); debugPtr->print(" compare :");
    //      debugPtr->print(itemIterator->getIndexNo()); debugPtr->print("  to :");    debugPtr->println(indexNoToBeFound);
    //      debugPtr->print(" idxFound:"); debugPtr->println(idxFound);
    //    }
    if (itemIterator->getIndexNo() == indexNoToBeFound) {
      idxFound = idx;
      //      if ((debugPtr)) { // && (indexNoToBeFound != 0)) {
      //        debugPtr->print("found idx idx:"); debugPtr->print(idx); debugPtr->print(" idxFound:");
      //        debugPtr->println(idxFound);
      //      }
      break;
    }
    ValuesClass *nextItem = listDwgValues.getNext();
    idx++;
    if ((nextItem) && (nextItem->type != pfodTOUCH_ZONE)) {
      listIdx++;  // stop idx at first touch zone for return
      // but keep looking for touchZone matches for hide etc
    }
    // else
    itemIterator = nextItem;
  }
  //  if (debugPtr) {
  //    debugPtr->print("listIdx:");    debugPtr->print(listIdx);
  //    debugPtr->print(" idxFound:");    debugPtr->println(idxFound);
  //  }

  listDwgValues.setCurrentIterator(currentPtr);
  return indexNoToBeFound ? idxFound : listDwgValues.size(); // return listDwgValues.size(); if searching for 0
}

/**
   addTouchAction_1or2
   returns true if action added/exists else false
   params
   isOne - true is this is for action_1, false if for action_2

   On initial create of touchAction, set hide() action with no reference and display current touchZone in dwgPanel.<br>
   On re-open if referred item not found and action is hide() display current touchZone in dwgPanel<br>
   If action is not hide() make selected dwgPanel item the first touchZone item and then replace it with this action item for editing<br>
   sendDwgSelectedItem() does the replacement.<p>
   NOTE: here must have at least one item this touchZone!!

*/
//returns true if Input action added or Opened else false
bool Items::addTouchAction_1or2(size_t i) { // 1 to 5
  if (debugPtr) {
    debugPtr->print("::addTouchAction_1or2 i="); debugPtr->println(i);
    debugListDwgValues();
  }

  // default is touchAction().cmd(touchZoneCmd).action(hide().cmd(touchZoneCmd)).send()
  ValuesClass *valuesClassPtr = getControlSelectedItem();
  if (valuesClassPtr->type != pfodTOUCH_ZONE) {
    if (debugPtr) {
      debugPtr->print("return false valuesClassPtr->type :"); debugPtr->println(valuesClassPtr->getTypeAsStr());
    }
    return false; // to current Control selected Item
  }
  bool doNotHaveAction = false; // i.e. have action
  if (i == 1) {
    doNotHaveAction = !(valuesClassPtr->getAction1Ptr());
  } else if (i == 2) {
    doNotHaveAction = !(valuesClassPtr->getAction2Ptr());
  } else if (i == 3) {
    doNotHaveAction = !(valuesClassPtr->getAction3Ptr());
  } else if (i == 4) {
    doNotHaveAction = !(valuesClassPtr->getAction4Ptr());
  } else if (i == 5) {
    doNotHaveAction = !(valuesClassPtr->getAction5Ptr());
  }
  if (doNotHaveAction) {
    if (debugPtr) {
      debugPtr->print("doNotHaveAction i="); debugPtr->println(i);
    }

    // add one then position dwg panel to suit
    ValuesClass *actionValuesClassPtr = createDwgItemValues(pfodTOUCH_ACTION); // no indexNo for new actionInput
    if (!actionValuesClassPtr) {
      if (debugPtr) {
        debugPtr->println("return false createDwgItemValues(pfodTOUCH_ACTION) failed");
      }
      return false;
    }
    if (!(actionValuesClassPtr->valuesPtr)) {
      delete actionValuesClassPtr;
      actionValuesClassPtr = NULL;
      if (debugPtr) {
        debugPtr->println("return false createDwgItemValues(pfodTOUCH_ACTION) failed");
      }
      return false;
    }
    // set cmdStr to match TouchZone
    cSFA(sfTouchCmdStr, actionValuesClassPtr->touchCmdStr);
    sfTouchCmdStr.clear();
    sfTouchCmdStr = valuesClassPtr->cmdStr;

    //   printDwgValues(" touchAction before create hide ", actionValuesClassPtr);
    ValuesClass *actionActionClassPtr = createDwgItemValues(pfodHIDE); // no indexNo for new actionInput
    if (debugPtr) {
      debugPtr->println(" after create hide");
    }

    if (!actionActionClassPtr) {
      if (debugPtr) {
        debugPtr->println("return false createDwgItemValues(pfodTOUCH_ACTION) failed");
      }
      delete actionValuesClassPtr;
      actionValuesClassPtr = NULL;
      return false;
    }

    actionValuesClassPtr->actionActionPtr = actionActionClassPtr; // delete will now handle this as well
    //    printDwgValues(" after add action pointer ", actionValuesClassPtr);

    if (!(actionActionClassPtr->valuesPtr)) {
      delete actionActionClassPtr;
      actionValuesClassPtr = NULL;
      if (debugPtr) {
        debugPtr->println("return false createDwgItemValues(pfodTOUCH_ACTION) failed");
      }
      return false;
    }
    //    size_t listIdx = 0; // either listDwgValues idx of match found OR the last non-touchZone if not found
    //    findReferredIdx(0, listIdx);  // returns idx() of dwgItem of dwg item found if match else 0,
    //    // called findRerredIdx to find last time that is NOT a touchZone, i.e. listIdx
    //    ValuesClass *listIdxPtr = listDwgValues.get(listIdx);
    //    if ((listIdxPtr) && (listIdxPtr->type != pfodTOUCH_ZONE)) {
    //      // set idxNo to this one
    //      actionValuesClassPtr->setIndexNo(listIdxPtr->getIndexNo());
    //    }
    actionValuesClassPtr->setIndexNo(0);
    actionActionClassPtr->setIndexNo(0);

    if (valuesClassPtr->appendAction(actionValuesClassPtr, i)) {
      // returns false if not added/updated
      if (i == 1) {
        controlSelectedItemPtr = valuesClassPtr->getAction1Ptr();
      } else if (i == 2) {
        controlSelectedItemPtr = valuesClassPtr->getAction2Ptr();
      } else if (i == 3) {
        controlSelectedItemPtr = valuesClassPtr->getAction3Ptr();
      } else if (i == 4) {
        controlSelectedItemPtr = valuesClassPtr->getAction4Ptr();
      } else if (i == 5) {
        controlSelectedItemPtr = valuesClassPtr->getAction5Ptr();
      }
      if (debugPtr) {
        debugPtr->print("set  controlSelectedItemPtr to :"); debugPtr->println(controlSelectedItemPtr->getTypeAsStr());
      }
      // will save below at setDwgItemIdx
      if (debugPtr) {
        debugPtr->println("return true. added/updated");
      }
    } else {
      if (debugPtr) {
        debugPtr->println("return false appendAction failed");
      }
      return false;
    }
    saveSelectedItem();
  } // end of if (doNotHaveAction)

  // already have one or just added one just open it
  if (i == 1) {
    controlSelectedItemPtr = valuesClassPtr->getAction1Ptr();
  } else if (i == 2) {
    controlSelectedItemPtr = valuesClassPtr->getAction2Ptr();
  } else if (i == 3) {
    controlSelectedItemPtr = valuesClassPtr->getAction3Ptr();
  } else if (i == 4) {
    controlSelectedItemPtr = valuesClassPtr->getAction4Ptr();
  } else if (i == 5) {
    controlSelectedItemPtr = valuesClassPtr->getAction5Ptr();
  }
  if (debugPtr) {
    if (doNotHaveAction) {
      debugPtr->println("created new Action");
    } else {
      debugPtr->println("using existing Action");
    }
  }
  if (!doNotHaveAction) { // use existing Action
    // search for referenc and set dwgItem idx
    // printDwgValues(" action1 controlSelectedItemPtr ", controlSelectedItemPtr);
    // update dwgPanel to this refNo if it exists
    //returns either idx found if match else 0, idxFound is non-zero if the indexNo is found
    size_t listIdx = 0;
    // findReferredIdx updates listIdx to either foundIdx if found OR the last non-touchZone if not found
    size_t foundIdx = findReferredIdx(controlSelectedItemPtr->getIndexNo(), listIdx);  // returns idx() of dwgItem of dwg item found if match else 0,
    if (foundIdx == getListDwgValuesSize()) {
      // missing so set this touchZone
      controlSelectedItemPtr->setIndexNo(0); // clear this indexNo in the actionAction whatever that is
      controlSelectedItemPtr->getActionActionPtr()->setIndexNo(0); // clear this indexNo in the actionAction whatever that is
      saveSelectedItem();
    } else {
      listIdx = foundIdx;
      // here could be matching a touchZone if action is hide (default)
      if (!setDwgItemIdx(listIdx, false)) { // skip touchActionIndexNo update and save
        //saveSelectedItem(); // no need to save as just open existing action referencing existing dwgItem
      }
    }
  } // else leave dwgSelected item unchanged should be a touchZone

  if (debugPtr) {
    debugPtr->println("  at end of : add TouchAction()");
    debugListDwgValues();
    debugPtr->println("  =====================");
  }

  return true;
}

// NOTE here must have at least one item this touchZone!!
//returns true if Input action added or Opened else false
bool Items::addTouchAction_1() {
  if (debugPtr) {
    debugPtr->println("::addTouchAction_1");
  }
  return addTouchAction_1or2(1); // isOne;
}

// NOTE here must have at least one item this touchZone!!
//returns true if Input action added or Opened else false
bool Items::addTouchAction_2() {
  if (debugPtr) {
    debugPtr->println("::addTouchAction_2");
  }
  return addTouchAction_1or2(2); // isOne;
}

// NOTE here must have at least one item this touchZone!!
//returns true if Input action added or Opened else false
bool Items::addTouchAction_3() {
  if (debugPtr) {
    debugPtr->println("::addTouchAction_3");
  }
  return addTouchAction_1or2(3); // isOne;
}

// NOTE here must have at least one item this touchZone!!
//returns true if Input action added or Opened else false
bool Items::addTouchAction_4() {
  if (debugPtr) {
    debugPtr->println("::addTouchAction_4");
  }
  return addTouchAction_1or2(4); // isOne;
}

// NOTE here must have at least one item this touchZone!!
//returns true if Input action added or Opened else false
bool Items::addTouchAction_5() {
  if (debugPtr) {
    debugPtr->println("::addTouchAction_5");
  }
  return addTouchAction_1or2(5); // isOne;
}

// NOTE here must have at least one item this touchZone!!
//returns true if Input action added or Opened else false
// on create have a hide() with no reference
// in dwgPanel remove all but first touchZone
// when add replacement item to hide with no index, replace touchZone with that item
// when show pressed just hide the replacement item this avoids handling/showing multipe touchzones.
// once move down on dwgPanel, remove item but show it on pressing show
// when replacement item added display that item for edit but show original one on show.
bool Items::addTouchActionInput() {
  //  if (debugPtr) {
  //    debugPtr->println("::addTouchActionInput");
  //  }
  bool haveExistingTouchActionInput = false;
  ValuesClass *valuesClassPtr = getControlSelectedItem();
  printDwgValues("addTouchActionInput  getControlSelectedItem() ", getControlSelectedItem());
  if (valuesClassPtr->type != pfodTOUCH_ZONE) {
    if (debugPtr) {
      debugPtr->print("return false valuesClassPtr->type :"); debugPtr->println(valuesClassPtr->getTypeAsStr());
    }
    return false; // to current Control selected Item
  }
  if (!valuesClassPtr->getActionInputPtr()) {
    // add one then position dwg panel to suit
    if (debugPtr) {
      debugPtr->println("create new touchActionInput ");
    }
    ValuesClass *actionValuesClassPtr = createDwgItemValues(pfodTOUCH_ACTION_INPUT); // no indexNo for new actionInput
    if (!actionValuesClassPtr) {
      if (debugPtr) {
        debugPtr->println("return false createDwgItemValues(pfodTOUCH_ACTION_INPUT) failed");
      }
      return false;
    }
    if (!(actionValuesClassPtr->valuesPtr)) {
      delete actionValuesClassPtr;
      actionValuesClassPtr = NULL;
      if (debugPtr) {
        debugPtr->println("return false createDwgItemValues(pfodTOUCH_ACTION_INPUT) failed");
      }
      return false;
    }

    actionValuesClassPtr->valuesPtr->align = 'L'; // default to left for prompts
    {
      cSFA(sfText, actionValuesClassPtr->text);
      sfText = "Enter text for Prompt"; // default value
    }
    // set prompt background to white so default black text is visible
    actionValuesClassPtr->valuesPtr->backgroundColor = dwgsPtr->WHITE;

    // set cmdStr to match TouchZone
    cSFA(sfTouchCmdStr, actionValuesClassPtr->touchCmdStr);
    sfTouchCmdStr.clear();
    sfTouchCmdStr = valuesClassPtr->cmdStr;
    if (valuesClassPtr->appendAction(actionValuesClassPtr, 0)) {
      // returns false if not added/updated
      controlSelectedItemPtr = actionValuesClassPtr;
      if (debugPtr) {
        debugPtr->print("set  controlSelectedItemPtr to :"); debugPtr->println(controlSelectedItemPtr->getTypeAsStr());
      }
      // will save below at setDwgItemIdx
      if (debugPtr) {
        debugPtr->println("return true. added/updated");
      }
    } else {
      if (debugPtr) {
        debugPtr->println("return false appendAction failed");
      }
      return false;
    }

  } else {
    if (debugPtr) {
      debugPtr->println("already have touchActionInput");
    }
    haveExistingTouchActionInput = true;
  } // end of   if (!valuesClassPtr->getActionInput()) {

  // already have one or just added one just open it
  controlSelectedItemPtr = valuesClassPtr->getActionInputPtr();
  if (debugPtr) {
    debugPtr->println("return  existing ActionInput ");
  }
  // update dwgPanel to this refNo if it exists
  //returns either idx found if match else listDwgValues.size(), idxFound is < listDwgValues.size() if the indexNo is found
  size_t listIdx = 0; // either listDwgValues idx of match found OR the last non-touchZone if not found
  size_t foundIdx = findReferredIdx(controlSelectedItemPtr->getIndexNo(), listIdx);  // returns idx() of dwgItem of dwg item found if match else listDwgValues.size(),
  // NOTE here we are looking to match a dwg Item  so use listIdx return instead of foundIdx
  // if not found remove display of all touchzones, listIdx is idx of last non-touchZone
  if (foundIdx == getListDwgValuesSize()) {
    // not found reset to 0
    listIdx = dwgItemIdx; // << this is the touchZone that the input was added to
  }
  if (!setDwgItemIdx(listIdx)) { // updates touchActionInput if necessary and saves returns true if saved
    if (!haveExistingTouchActionInput) {
      // need to save this new one
      saveSelectedItem();
    }
  }
  if (debugPtr) {
    debugPtr->println("  end addTouchActionInput");
  }
  return true;
}

// this pointer is held by the list ONLY used in append
ValuesClass *Items::createDwgItemValues(pfodDwgTypeEnum selectedDwgItemType, bool addIndex, bool initDwgValues) {
  if (debugPtr) {
    debugPtr->print("::createDwgItemValues "); debugPtr->println(ValuesClass::typeToStr(selectedDwgItemType));
  }
  //default for addIndex is false IOSupport does not add one on read, initDwgValues is false so IOSupport does not set dwg defaults on read
  ValuesClass *valuesClassPtr = new ValuesClass; // alway use new  this created new valuePtr and does init()
  if (!valuesClassPtr) {
    return NULL;
  }
  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr; // allocated by ValuesClass but may be NULL if OOM
  if (!valuesPtr) {
    delete valuesClassPtr;
    valuesClassPtr = NULL;
    return NULL;
  }

  //    if (debugPtr) {
  //      debugPtr->print(" new ValuesClass ");   debugPtr->print(valuesClassPtr->cmdStr); debugPtr->print("  valuesPtr->cmdStr "); debugPtr->println(valuesClassPtr->valuesPtr->cmdStr);
  //    }
  printDwgValues(" new ValuesClass ", valuesClassPtr);

  if (selectedDwgItemType == pfodTOUCH_ZONE) {
    valuesClassPtr->setNextCmdNo(); // no indexNo
  } else if ((selectedDwgItemType == pfodTOUCH_ACTION) || (selectedDwgItemType == pfodTOUCH_ACTION_INPUT) || (selectedDwgItemType == pfodNONE)) {
    // skip adding nextIndexNo
  } else {
    if (addIndex) {
      valuesClassPtr->setNextIndexNo(); // for appends, load uses idx if set, touchAction and touchActionInput use indexNo to link to action item
      // so leave as zero to start with
      // IOSupport also does not addIndex on initial create, but assigns one if found and init() cleans up later
    }
  }
  valuesClassPtr->type = selectedDwgItemType;
  if ((selectedDwgItemType == pfodTOUCH_ACTION_INPUT) || (selectedDwgItemType == pfodTOUCH_ACTION)) {
    cSFA(sftouchCmdStr, valuesClassPtr->touchCmdStr);
    sftouchCmdStr = "tz_1";
  }
  if (selectedDwgItemType == pfodTOUCH_ZONE) {
    cSFA(sfCmdStr, valuesClassPtr->cmdStr);
    sfCmdStr = "tz_1";
  }

  // always add next idx
  if (initDwgValues) {
    if (debugPtr) {
      debugPtr->println("initDwgValues");
    }
    {
      cSFA(sfText, valuesClassPtr->text);
      sfText = "Label"; // default value
    }
    // set some defaults

    valuesPtr->color = 0; // black
    valuesPtr->backgroundColor = 15; // white for prompts
    valuesPtr->height = 10;
    valuesPtr->width = 10;
    valuesPtr->radius = 5;
    valuesPtr->startAngle = 0;
    valuesPtr->arcAngle = 45;
    printDwgValues(" after init ", valuesClassPtr);
  }
  //  if (debugPtr) {
  //    debugPtr->print(" new ValuesClass return ");   debugPtr->print(valuesClassPtr->cmdStr); debugPtr->print("  valuesPtr->cmdStr "); debugPtr->println(valuesClassPtr->valuesPtr->cmdStr);
  //    if (selectedDwgItemType == pfodTOUCH_ACTION_INPUT) {
  //      pfodTouchActionInput actionInput = dwgsPtr->touchActionInput();
  //      struct pfodDwgVALUES *actionValuesPtr = actionInput.getValuesPtr();
  //      actionInput.setValuesPtr(valuesClassPtr->valuesPtr);
  //      actionInput.out = debugPtr;
  //      actionInput.send();
  //      debugPtr->println();
  //      actionInput.out = parserPtr;
  //      actionInput.setValuesPtr(actionValuesPtr);
  //    }
  //  }

  return valuesClassPtr;
}

/**
   sendDwgSelectedItem
   returns true if something sent, else false (no items)

   params
   withDragIdx - default true, add idx to let user drag selected item around screen, false when adjusting the zero

  on create have a hide() with no reference
  in dwgPanel remove all but first touchZone
  when add replacement item to hide with no index, replace touchZone with that item
  when show pressed just hide the replacement item this avoids handling/showing multipe touchzones.
  once move down on dwgPanel, remove item but show it on pressing show
  when replacement item added display that item for edit but show original one on show.
*/
bool Items::sendDwgSelectedItem(bool withDragIdx) {
  ValuesClass *valuesClassPtr = getDwgSelectedItem(); // never NULL
  if (debugPtr) {
    debugPtr->print("::sendDwgSelectedItem "); debugPtr->print(valuesClassPtr->getTypeAsStr());
    debugPtr->print(" indexNo:"); debugPtr->print(valuesClassPtr->getIndexNo());
    debugPtr->print(" idx:"); debugPtr->println(valuesClassPtr->valuesPtr->idx);
  }


  struct pfodDwgVALUES *valuesPtr = getDwgSelectedItem()->valuesPtr; // never NULL
  if (valuesClassPtr->type == pfodNONE) {
    return false;
  }
  dwgsPtr->pushZero(DwgPanel::getXCenter(), DwgPanel::getYCenter(), DwgPanel::getScale());

  if (debugPtr) {
    debugPtr->print(" getControlSelectedItem() ");
    debugPtr->println(getControlSelectedItem() == NULL ? " NULL" : "not null");
    if (getControlSelectedItem()) {
      debugPtr->println(getControlSelectedItem()->getTypeAsStr());
    }
  }
  if (getControlSelectedItem()->type == pfodTOUCH_ACTION) {
    // need to display that instead of the item
    // send touch action if editing it
    ValuesClass *controlDisplayPtr = getControlDisplayedItem();
    if (debugPtr) {
      debugPtr->print(" getControlDisplayedItem() ");
      debugPtr->println(getControlDisplayedItem() == NULL ? " NULL" : "not null");
      if (getControlDisplayedItem()) {
        debugPtr->println(getControlDisplayedItem()->getTypeAsStr());
      }
    }
    uint16_t displayIdx = controlDisplayPtr->valuesPtr->idx;
    if (debugPtr) {
      debugPtr->print(" calling getReferencedPtr ");
    }
    ValuesClass *referredPtr = getReferencedPtr(getControlSelectedItem()->getIndexNo());
    if (!referredPtr)  {
      controlDisplayPtr->valuesPtr->idx = unReferencedTouchItemIdx;
    } else {
      controlDisplayPtr->valuesPtr->idx = unReferencedTouchItemIdx;
      // controlDisplayPtr->valuesPtr->idx = referredPtr->valuesPtr->idx;
    }
    if (controlDisplayPtr->type == pfodHIDE) {
      dwgsPtr->index().idx(controlDisplayPtr->valuesPtr->idx).send();
    }

    pfodDwgsBase *dwgItemPtr = createDwgItemPtr(controlDisplayPtr);
    if (dwgItemPtr) {
      dwgItemPtr->send();
      print_pfodBasePtr("send selected Item ", dwgItemPtr);
    }
    controlDisplayPtr->valuesPtr->idx = displayIdx;  // restore
  } else {
    if (debugPtr) {
      debugPtr->print(" controlSelected not a touchAction ");
      debugPtr->println();
    }

    // NOTE:  do not return before the popZero() below.
    uint16_t idx = valuesPtr->idx;
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      if (debugPtr) {
        debugPtr->print(" dwgSelected not a touchZone ");
        debugPtr->println();
      }

      ValuesClass *controlItemPtr = getControlSelectedItem();
      if (controlItemPtr->type == pfodTOUCH_ACTION) {
        // send touch action if editing it
        ValuesClass *controlDisplayPtr = getControlDisplayedItem();
        uint16_t displayIdx = controlDisplayPtr->valuesPtr->idx;
        ValuesClass *referredPtr = getReferencedPtr(controlItemPtr->getIndexNo());
        if (!referredPtr)  {
          controlDisplayPtr->valuesPtr->idx = unReferencedTouchItemIdx;
        } else {
          //      controlDisplayPtr->valuesPtr->idx = referredPtr->valuesPtr->idx;
          controlDisplayPtr->valuesPtr->idx = unReferencedTouchItemIdx;
        }
        if (controlDisplayPtr->type == pfodHIDE) {
          dwgsPtr->index().idx(controlDisplayPtr->valuesPtr->idx).send();
        }

        pfodDwgsBase *dwgItemPtr = createDwgItemPtr(controlDisplayPtr);
        if (dwgItemPtr) {
          dwgItemPtr->send();
          print_pfodBasePtr("send selected Item ", dwgItemPtr);
        }
        controlDisplayPtr->valuesPtr->idx = displayIdx;  // restore
      } else { // not touchAction just send normal one
        if (debugPtr) {
          debugPtr->print(" control Item not a touchAction ");
          debugPtr->println();
        }

        pfodDwgsBase *dwgItemPtr = createDwgItemPtr(valuesClassPtr);
        uint16_t orgIdx = dwgItemPtr->getValuesPtr()->idx;
        if (debugPtr) {
          debugPtr->print(" set idx to :"); debugPtr->print(unReferencedTouchItemIdx);
          debugPtr->println();
        }
        dwgItemPtr->getValuesPtr()->idx = unReferencedTouchItemIdx;
        if (dwgItemPtr) {
          dwgItemPtr->send();
          print_pfodBasePtr("send selected Item ", dwgItemPtr);
        }
        dwgItemPtr->getValuesPtr()->idx = orgIdx;
      }
    } else { // pfodTOUCH_ZONE
      sendTouchZoneOutline(valuesClassPtr, idx);
    }
    valuesPtr->idx = idx; // restore
  }
  dwgsPtr->popZero();
  return true;
}

// send item with idx from start(inclusive) to end(excluding)
// indices start from 0
// returns true if something sent
bool Items::sendDwgItems(size_t startIdx, size_t endIdx) {
  if (startIdx >= dwgItemIdx) {
    // don't send
    return false;
  }
  // limit endIdx to < dwgItemIdx
  if (endIdx < startIdx) {
    endIdx = startIdx;
  }
  if (endIdx >= listDwgValues.size()) {
    endIdx = listDwgValues.size();
  }
  if (endIdx > dwgItemIdx) {
    endIdx = dwgItemIdx; // excluded
  }
  if (debugPtr) {
    debugPtr->print("::sendDwgItems");
    debugPtr->print(" startIdx:"); debugPtr->print(startIdx);
    debugPtr->print(" endIdx:"); debugPtr->print(endIdx);
    debugPtr->print(" dwgItemIdx:"); debugPtr->print(endIdx);
    debugPtr->println();
  }
  if (startIdx >= endIdx) { // limited above
    return false;
  }
  ValuesClass *valuesClassPtr = listDwgValues.get(startIdx); // returns NULL if idx >= size()
  if (!valuesClassPtr) {
    return false;
  }
  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr;
  if (!valuesPtr) {
    return false;
  }

  if (debugPtr) {
    debugPtr->print("sendDwgItems type:");  debugPtr->print(valuesClassPtr->getTypeAsStr());
    debugPtr->print(" indexNo:"); debugPtr->print(valuesClassPtr->getIndexNo());
    debugPtr->print(" idx:"); debugPtr->println(valuesClassPtr->valuesPtr->idx);
  }
  pfodDwgsBase *dwgItemPtr = createDwgItemPtr(valuesClassPtr);
  if (dwgItemPtr) {
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      dwgItemPtr->send();
    } else {
      sendTouchZoneOutline(valuesClassPtr);
    }
  }

  size_t currentIdx = startIdx;
  currentIdx++;
  while ((currentIdx < endIdx) && (valuesClassPtr)) {
    valuesClassPtr = listDwgValues.get(currentIdx);
    if (valuesClassPtr) {
      if (debugPtr) {
        debugPtr->print("sendDwgItems type:");  debugPtr->print(valuesClassPtr->getTypeAsStr());
        debugPtr->print(" indexNo:"); debugPtr->print(valuesClassPtr->getIndexNo());
        debugPtr->print(" idx:"); debugPtr->println(valuesClassPtr->valuesPtr->idx);
      }
      valuesPtr = valuesClassPtr->valuesPtr;
      if (valuesClassPtr->type != pfodTOUCH_ZONE) {
        dwgItemPtr = createDwgItemPtr(valuesClassPtr);
        if (dwgItemPtr) {
          dwgItemPtr->send();
        }
      } else {
        sendTouchZoneOutline(valuesClassPtr);
      }
      currentIdx++;
    }
  }
  //  if (debugPtr) {
  //    debugPtr->print("===============");  debugPtr->println();
  //  }
  return true;
}

void Items::sendTouchZoneIndices() {
  dwgsPtr->index().idx(unReferencedTouchItemIdx).send(); // for touch Actions with no reference
}

/**
   sendTouchZoneOutline
   params
   itemValuesClassPtr - pointer to touchZone item
   idx - dragIdx (default 0), if not zero, else use layer idx in itemValuesClassPtr->valuePtr->idx

   Sends 3 rounded rectangles to indicate the touchzone indexed idx,idx+1,idx+2
*/
void Items::sendTouchZoneOutline(ValuesClass * itemValuesClassPtr, int idx) {
  if (debugPtr) {
    debugPtr->print("::sendTouchZoneOutline idx:"); debugPtr->println(idx);
  }
  // send three rectangles for touch zone
  // if not selected, i.e. idx == 0 then use idx of valuesPtr plus next two for idx
  if (itemValuesClassPtr->type != pfodTOUCH_ZONE) {
    if (debugPtr) {
      debugPtr->print("sendTouchZoneOutline expected touchZone got "); debugPtr->println(itemValuesClassPtr->getTypeAsStr());
    }
    return;
  }
  struct pfodDwgVALUES *valuesPtr = itemValuesClassPtr->valuesPtr;
  if (idx == 0) {
    idx = valuesPtr->idx;
  }
  pfodRectangle rect = dwgsPtr->rectangle();
  // set xoffset and size and color
  rect.offset(valuesPtr->colOffset, valuesPtr->rowOffset);
  rect.size(valuesPtr->width, valuesPtr->height);
  bool centered =  valuesPtr->centered;
  if (centered) {
    rect.centered(); //  valuesClassPtr->valuesPtr->centered = valuesPtr->centered;
  }
  if (itemValuesClassPtr->isCovered()) {
    rect.color(dwgsPtr->SILVER); //valuesClassPtr->valuesPtr->color = dwgsPtr->SILVER;
  } else {
    rect.color(dwgsPtr->BLACK); //valuesClassPtr->valuesPtr->color = dwgsPtr->BLACK;
  }
  rect.idx(idx); // valuesClassPtr->valuesPtr->idx = idx; //pfodAutoIdx_tz_1.idx;
  rect.rounded();//  valuesClassPtr->valuesPtr->rounded = 1;

  rect.send();
  if (!centered) {
    rect.offset( valuesPtr->colOffset - 0.25, valuesPtr->rowOffset - 0.25);
  }
  rect.size(valuesPtr->width + 0.5, valuesPtr->height + 0.5);
  rect.color(dwgsPtr->WHITE);
  idx++;
  rect.idx(idx);
  rect.send();

  if (!centered) {
    rect.offset(valuesPtr->colOffset - 0.5, valuesPtr->rowOffset - 0.5);
  }
  rect.size(valuesPtr->width + 1, valuesPtr->height + 1);
  rect.color(dwgsPtr->SILVER);
  idx++;
  rect.idx(idx);
  rect.send();
}

// current dwg Item with row/col update
void Items::addItemTouchAction(const char* touchCmd) {
  if (debugPtr) {
    debugPtr->println("::addItemTouchAction for moves");
  }
  ValuesClass *valuesClassPtr = getDwgSelectedItem(); // never NULL
  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr; // never null
  if (!valuesPtr) {
    return;
  }
  size_t touchIdx = unReferencedTouchItemIdx;// was valuesPtr->idx; // default

  // check for action
  ValuesClass *controlSelectedPtr = getControlSelectedItem();
  if (controlSelectedPtr->type == pfodTOUCH_ACTION) {
    valuesClassPtr = getControlDisplayedItem();
    ValuesClass *referredItemPtr = getReferencedPtr(controlSelectedPtr->getIndexNo());
    valuesPtr = valuesClassPtr->valuesPtr;
    if (referredItemPtr == 0) {
      if (valuesClassPtr->type == pfodHIDE) {
        // skip this touch action
        return;
      }
      touchIdx = unReferencedTouchItemIdx;// handle
    } else {
      touchIdx = unReferencedTouchItemIdx;// handle
      // touchIdx = referredItemPtr->valuesPtr->idx;
    }
  } else {
    // do normal stuff with dwg selected item
  }
  if (valuesClassPtr->type == pfodNONE) {
    return;
  }
  // save current offsets
  float cols = valuesPtr->colOffset;
  float rows = valuesPtr->rowOffset;
  valuesPtr->colOffset = dwgsPtr->TOUCHED_COL; // update no rounding
  valuesPtr->rowOffset = dwgsPtr->TOUCHED_ROW;
  uint16_t tempIdx = valuesPtr->idx;
  valuesPtr->idx = touchIdx;
  pfodDwgTypeEnum type = valuesClassPtr->type;
  int rounded = valuesPtr->rounded;
  if (type == pfodTOUCH_ZONE) {
    valuesClassPtr->type = pfodRECTANGLE;
    valuesPtr->rounded = 1;  // should perhaps have multiple touch action for touchZone
  }

  pfodTouchAction action = dwgsPtr->touchAction(); // do this first as it clears all the settings
  pfodDwgsBase *dwgItemPtr = createDwgItemPtr(valuesClassPtr);
  if (dwgItemPtr) {
    pfodTouchAction action = dwgsPtr->touchAction(); //*actionPtr = *new pfodTouchAction;
    action.cmd(touchCmd);
    action.action(*dwgItemPtr);
    action.send();
    print_pfodBasePtr("    sending touchItemAction ", &action);
  }

  valuesPtr->colOffset = cols; // restore
  valuesPtr->rowOffset = rows;
  valuesPtr->idx = tempIdx; // restore
  valuesClassPtr->type = type; // restore
  valuesPtr->rounded = rounded;
}

bool Items::haveItems() {
  return listDwgValues.size() != 0;
}


// NEVER returns Null, returns getControlSelectedItem unless getControlSelectedItem is touchAction() in which case returns the action
ValuesClass *Items::getControlDisplayedItem() {
  ValuesClass *rtn = getControlSelectedItem(); //never NULL
  if (rtn->type != pfodTOUCH_ACTION) {
    return rtn;
  } // else return action
  rtn = rtn->getActionActionPtr();
  if (!rtn) {
    rtn = nullValuesClassPtr;
  }
  return rtn;
}

/**
   appendControlItem
  returns new size of list and updates dwgItemIdx to new idx
  params
  type - the pfod type of the item to be added

  If the list has TOUCH_ZONES, non TOUCH_ZONES are inserted below them<br>
  and the selected item idx updated to point to the inserted item<br>
  If current controlItem is a touchAction and the actionAction is hide() then replace with this
*/
size_t Items::appendControlItem(pfodDwgTypeEnum type) {
  // handle touchActions
  ValuesClass *controlPtr = getControlSelectedItem();
  if (controlPtr->type == pfodTOUCH_ACTION) {
    ValuesClass *actionActionPtr = controlPtr->getActionActionPtr();
    if (actionActionPtr->type != pfodHIDE) {
      if (debugPtr) {
        debugPtr->print("::appendControlItem:"); debugPtr->print(ValuesClass::typeToStr(type)); debugPtr->print(" already have action dwgItem :");
        debugPtr->print(actionActionPtr->getTypeAsStr()); debugPtr->println(" ignore append.");
      }
      return dwgItemIdx; // leave unchanged
    }
    // else can add this
    if ((type == pfodTOUCH_ZONE) || (type == pfodTOUCH_ACTION) || (type == pfodTOUCH_ACTION_INPUT)) {
      if (debugPtr) {
        debugPtr->print("appendControlItem:"); debugPtr->print(ValuesClass::typeToStr(type)); debugPtr->print(" invalid action dwgItem, ignore append.");
      }
      return dwgItemIdx; // leave unchanged
    }
    // else replace hide
    delete actionActionPtr;
    actionActionPtr = NULL;
    ValuesClass *valuesClassPtr = createDwgItemValues(type, true, true); // creating new item with new idxNo
    actionActionPtr = valuesClassPtr;
    actionActionPtr->setIndexNo(controlPtr->getIndexNo()); // not really used
    bool rtn = saveSelectedItem();
    if (!rtn) {
      if (debugPtr) {
        debugPtr->print("Error saving new Action item ");  debugPtr->print("  "); debugPtr->println(ValuesClass::typeToStr(type));
      }
    }

    if (debugPtr) {
      debugPtr->print("appendControlItem:"); debugPtr->print(ValuesClass::typeToStr(type)); debugPtr->println();
      debugListDwgValues();
      debugPtr->print("=============="); debugPtr->println();
    }
    if (debugPtr) {
      debugPtr->println();
    }

    return dwgItemIdx; // leave unchanged
  }
  // else
  ValuesClass *valuesClassPtr = createDwgItemValues(type, true, true); // creating new item with new idxNo
  if (listDwgValues.size() == 0)  {
    // add to end
    listDwgValues.append(valuesClassPtr); // could be null if OutOfMem
    dwgItemIdx = listDwgValues.size() - 1;
  } else {
    // find first touchZone size() > 0 here
    // add below any touch zones
    size_t firstTouchZoneIdx = 0;
    bool foundTouchZoneIdx = false;
    for (; firstTouchZoneIdx < listDwgValues.size(); firstTouchZoneIdx++) {
      if (listDwgValues.get(firstTouchZoneIdx)->type == pfodTOUCH_ZONE) {
        foundTouchZoneIdx = true;
        break; // found first pfodTOUCH_ZONE
      }
    }
    // if firstTouchZoneIdx == listDwgValues.size() then NO touch zones
    size_t insertIdx = dwgItemIdx + 1; // where to insert after (higher) then current item
    if (debugPtr) {
      debugPtr->print("dwgItemIdx:"); debugPtr->println(insertIdx);
      debugPtr->print("insertIdx:"); debugPtr->println(insertIdx);
      debugPtr->print("firstTouchZoneIdx:"); debugPtr->println(firstTouchZoneIdx);
    }
    if (type == pfodTOUCH_ZONE) {
      // insert >= first touch zone
      if (insertIdx < firstTouchZoneIdx) {
        insertIdx = firstTouchZoneIdx;
      }
    } else {
      if (foundTouchZoneIdx && (insertIdx > firstTouchZoneIdx)) {
        // have touchZones and trying to insert above first
        // note inserting At first is OK as will push touchZone to higer idx in list
        insertIdx = firstTouchZoneIdx;
      }
    }
    if (debugPtr) {
      debugPtr->print("updated insertIdx:"); debugPtr->println(insertIdx);
    }

    // now insert, pushing item at insertIdx to higher idx in list
    listDwgValues.insertAt(valuesClassPtr, insertIdx);
    dwgItemIdx = insertIdx;
  }
  if (type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered(); // for adding TOUCH_ZONE or first add
  }
  bool rtn = true;
  setDwgItemIdxUpdated();
  if (!setDwgItemIdx(dwgItemIdx)) { // sets dwgSelectedItemPtr and updated touchActionInput if that is current controlSelectedItemPtr
    rtn = saveSelectedItem(); // not save above
  }
  if (debugPtr) {
    debugPtr->print("appendControlItem:"); debugPtr->print(ValuesClass::typeToStr(type)); debugPtr->println();
    debugListDwgValues();
    debugPtr->print("=============="); debugPtr->println();
  }
  if (debugPtr) {
    debugPtr->println();
  }
  if (!rtn) {
    if (debugPtr) {
      debugPtr->print("Error saving item no:");  debugPtr->print(dwgItemIdx + 1); debugPtr->print("  "); debugPtr->println(ValuesClass::typeToStr(type));
    }
  }
  setDisplayIdx();
  return listDwgValues.size();
}

// deletes this file and moves all the later ones down
void Items::deleteItemFile(size_t idx) {
  String path = createItemFilePath(idx);
  deleteFile(path.c_str()); // this one is gone
  // rename all the files above this one
  String path_1 = createItemFilePath(idx + 1);
  while (fileSize(path_1.c_str())) { // have one
    renameFile(path_1.c_str(), path.c_str());
    idx++;
    path = createItemFilePath(idx);
    path_1 = createItemFilePath(idx + 1);
  }
}

// returns new size of list
// ok if list empty
size_t Items::deleteControlItem() {
  if ((controlSelectedItemPtr->type == pfodTOUCH_ACTION) || (controlSelectedItemPtr->type == pfodTOUCH_ACTION_INPUT)) {
    // handle delete of actions
    ValuesClass *valuesClassPtr =  listDwgValues.get(controlItemIdx); // the touchZone parent
    // find the current ptr in the list and remove
    printDwgValues(" removing item ", controlSelectedItemPtr);
    printDwgValues(" from touchZone item ", valuesClassPtr);
    valuesClassPtr->removeAction(controlSelectedItemPtr);
    return listDwgValues.size();
  }
  // else
  ValuesClass *valuesClassPtr =  listDwgValues.get(dwgItemIdx); // the current one
  if (!valuesClassPtr) {
    return listDwgValues.size();
  }
  pfodDwgTypeEnum type = valuesClassPtr->type;
  listDwgValues.remove(valuesClassPtr);
  delete valuesClassPtr; // ok if null

  deleteItemFile(dwgItemIdx);
  if (dwgItemIdx > 0) {
    dwgItemIdx--; // move back one
  }
  setDwgItemIdx(dwgItemIdx); // updated touchActionInput and saves if necessary
  setDwgItemIdxUpdated();
  if (type == pfodTOUCH_ZONE) {
    processTouchZonesForCovered();
  }
  setDisplayIdx();
  return listDwgValues.size();
}


void Items::printDwgValues(const char* title, ValuesClass * valuesClassPtr) {
  if (!debugPtr) {
    return;
  }
  if ((title) && (*title)) {
    debugPtr->print(title); debugPtr->print(" ");
  }
  if (!valuesClassPtr) {
    debugPtr->println("NULL");
    return;
  }

  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr;
  if (!valuesPtr) {
    debugPtr->print(valuesClassPtr->getTypeAsStr());
    debugPtr->println(", values NULL");
    return;
  }
  if (valuesClassPtr->type == pfodNONE) {
    debugPtr->println("  type pfodNONE");
    return;
  }
  pfodDwgsBase *dwgItemPtr = createDwgItemPtr(valuesClassPtr); // sets valuePtr
  if (dwgItemPtr) {
    cSF(sfMsg, 80);
    sfMsg = valuesClassPtr->getTypeAsStr();
    if ((title) && (*title)) {
      // skip padding
    } else {
      while (sfMsg.length() < 20) {
        sfMsg -= ' '; // padd to 20 on left
      }
    }
    debugPtr->print(sfMsg);
    debugPtr->print(" indexNo:"); debugPtr->print(valuesClassPtr->getIndexNo());
    debugPtr->print(" idx:"); debugPtr->print(valuesClassPtr->valuesPtr->idx);
    debugPtr->print(" ref:"); debugPtr->print(valuesClassPtr->isReferenced ? "Y" : "N");
    debugPtr->print(" ");
    struct pfodDwgVALUES *dwgItemValuesPtr = dwgItemPtr->getValuesPtr();
    Print *dwgItemOutPtr = dwgItemPtr->out;
    if (dwgItemValuesPtr->actionPtr) {
      dwgItemValuesPtr->actionPtr->out = debugPtr;
    }
    dwgItemPtr->out = debugPtr;
    dwgItemPtr->send();
    if (dwgItemValuesPtr->actionPtr) {
      dwgItemValuesPtr->actionPtr->out = dwgItemOutPtr;
      debugPtr->print(" ");
      debugPtr->print(valuesClassPtr->getActionActionPtr()->getTypeAsStr());
      debugPtr->print(" indexNo:"); debugPtr->print(valuesClassPtr->getActionActionPtr()->getIndexNo());
      debugPtr->print(" idx:"); debugPtr->print(valuesClassPtr->getActionActionPtr()->valuesPtr->idx);
      debugPtr->print(" ref:"); debugPtr->print(valuesClassPtr->getActionActionPtr()->isReferenced ? "Y" : "N");
      debugPtr->print(" ");
    }
    debugPtr->println();
    dwgItemPtr->out = dwgItemOutPtr;
  } else {
    debugPtr->print("  Unknown type : "); debugPtr->println(valuesClassPtr->getTypeAsStr()); // debugPtr check on entry
  }
}

bool Items::writeItem(Stream * outPtr, size_t idx, SafeString & sfIdxName) {
  ValuesClass *valuesClassPtr = listDwgValues.get(idx);
  if (!valuesClassPtr) {
    return false;
  }
  if (debugPtr) {
    ItemsIOsupport::writeItem(debugPtr, valuesClassPtr, sfIdxName); // returns false if fails
  }
  bool rtn = ItemsIOsupport::writeItem(outPtr, valuesClassPtr, sfIdxName); // returns false if fails
  return rtn;
}

// calls pfodDwgs methods, which initialize the one value sturct
// but then replaces that struct with the one from the valuesClassPtr
pfodDwgsBase *Items::createDwgItemPtr(ValuesClass * valuesClassPtr) {
  if (!valuesClassPtr) {
    return NULL;
  }
  struct pfodDwgVALUES * valuesPtr = valuesClassPtr->valuesPtr;
  if (!valuesPtr) {
    return NULL;
  }
  pfodDwgsBase *dwgItemPtr = NULL;
  switch (valuesClassPtr->type) {
    case pfodRECTANGLE:
      dwgItemPtr = &(dwgsPtr->rectangle());
      break;
    case pfodLINE:
      dwgItemPtr = &(dwgsPtr->line());
      break;
    case pfodCIRCLE:
      dwgItemPtr = &(dwgsPtr->circle());
      break;
    case pfodARC:
      dwgItemPtr = &(dwgsPtr->arc());
      break;
    case pfodLABEL:
      dwgItemPtr = &(dwgsPtr->label());
      break;
    case pfodTOUCH_ZONE:
      dwgItemPtr = &(dwgsPtr->touchZone());
      break;
    case pfodTOUCH_ACTION_INPUT:
      dwgItemPtr = &(dwgsPtr->touchActionInput());
      break;
    case pfodTOUCH_ACTION:
      dwgItemPtr = &(dwgsPtr->touchAction());
      break;
    case pfodHIDE:
      dwgItemPtr = &(dwgsPtr->hide());
      break;
    case pfodNONE:
    default:
      dwgItemPtr = NULL;
      // null pointer
  }
  if (dwgItemPtr) {
    dwgItemPtr->out = parserPtr;
    dwgItemPtr->setValuesPtr(valuesPtr);
    if (valuesClassPtr->type == pfodTOUCH_ACTION) {
      //      if (debugPtr) {
      //        debugPtr->println("touchAction");
      //      }
      ValuesClass *actionClassPtr = valuesClassPtr->actionActionPtr;
      if (actionClassPtr == valuesClassPtr) {
        if (debugPtr) {
          debugPtr->println(" circular reference, action points back to touchAction");
        }
      } else {
        //        if (debugPtr) {
        //          debugPtr->print(" action ");
        //          if (!actionClassPtr) {
        //            debugPtr->println("NULL");
        //          } else {
        //            debugPtr->println(Items::getActionActionPtr(valuesClassPtr)->getTypeAsStr());
        //          }
        //        }
        pfodDwgsBase *actionPtr = createDwgItemPtr(Items::getActionActionPtr(valuesClassPtr));
        valuesPtr->actionPtr = actionPtr;
      }
    }
  }
  return dwgItemPtr;
}

ValuesClass *Items::getDwgValuesClassPtr(size_t idx) {  // NEVER returns NULL
  ValuesClass *valuesClassPtr = listDwgValues.get(idx);
  if (!valuesClassPtr) { // return dummy
    return nullValuesClassPtr;
  }
  return valuesClassPtr;
}

// NEVER returns NULL!!
struct pfodDwgVALUES *Items::getDwgValuesPtr(size_t idx) {
  struct pfodDwgVALUES *rtn = getDwgValuesClassPtr(idx)->valuesPtr;
  if (!rtn) {     // return dummy
    rtn = nullValuesPtr;
  }
  return rtn;
}


ValuesClass *Items::getControlSelectedItem() { // NEVER returns NULL!!
  ValuesClass *rtn = controlSelectedItemPtr;
  if (!rtn) {
    rtn = nullValuesClassPtr;
  }
  return rtn;
}

ValuesClass *Items::getDwgSelectedItem() { // NEVER returns NULL!!
  ValuesClass *rtn = dwgSelectedItemPtr;
  if (!rtn) {
    rtn = nullValuesClassPtr;
  }
  return rtn;
}

size_t Items::getDwgSelectedItemIdx() {
  return dwgItemIdx;
}
