#ifndef Items_h
#define Items_h
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodParser.h>
#include <SafeString.h>
#include "DwgName.h"
#include "ValuesClass.h"
#include "pfodLinkedPointerList.h" // iterable linked list of pointers to objects

// This Item class wraps the listDwgValues linked list of drawing items.
//
// The DwgPanel and ControlPanel are two view on this Item 'control' class
// There are two sets of get/set
// one for the dwgPanel and on for the controlPanel
//
class Items {
  public:
    static const char versionNo[10];// = "1.0.36";
    Items(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr); // set static vars
    static bool init(); // open files load existing items
    void setDebug(Stream* debugOutPtr); // set static var
    static void printDwgValues(const char* title, ValuesClass *valuesClassPtr);
    static void print_pfodBasePtr(const char* title, pfodDwgsBase *basePtr);
    static void debugListDwgValues(const char *title = NULL);

    static const uint16_t dwgReservedIdxs = 999;
    static const uint16_t unReferencedTouchItemIdx = dwgReservedIdxs+1500; // for action items that do not have a reference

    static ValuesClass *getReferencedPtr(size_t indexNo); // return dwg item referenced by indexNo OR NULL if not found or indexNo is 0;
    static size_t getIndexOfDwgItem(ValuesClass *dwgItem);
    static pfodLinkedPointerList<DwgName> listOfDwgs; // list of dwg names

    static const char* getCurrentDwgName();
    static const char* getCurrentDwgNameBase(); // trimmed at first space
    static bool renameDwg(String &newName);
    static bool saveDwgAs(SafeString &sfNewName, SafeString &sfErrMsg); // also updates currentName
    static bool newDwg(SafeString &sfNewName, SafeString &sfErrMsg); // also updates currentName
    static bool openDwg(); // opens/load currentNameDwg
    static bool writeDwgName(SafeString &dwgName); // used by DwgsList

    static bool cleanDwgName(SafeString &sfDwgName, SafeString &sfErrMsg); // returns false on error with errMsg, quitely removes non-alpha number non_ nonSpace chars
    static void trimDedupText(SafeString &sfName); // removes trailing (xxx)
    static bool showDwgDelete();
    static bool deleteDwg(); // deletes current dwg iff it has no items;
    static const int initDwgSize = 50;



    // -- this return true if a forceReload needed
    static bool setDwgSelectedOffset(float fx, float fy);
    // --
    static void forwardDwgSelectedItemIdx();  // force reload if moving touchZone
    static void backwardDwgSelectedItemIdx(); // force reload if moving touchZone
    static void increaseDwgSelectedItemIdx();
    static void decreaseDwgSelectedItemIdx();
    static ValuesClass *getDwgSelectedItem(); // NEVER returns NULL!!
    static size_t getDwgSelectedItemIdx(); // call this if isDwgSelectedItemIdxUpdated returns true to update item deleted/moved
    static bool isDwgSelectedItemIdxUpdated(); // resets flag on each call
    static bool sendDwgItems(size_t startIdx, size_t endIdx); // stops at selected item
    static bool sendDwgSelectedItem(bool withDragIdx = true); // normally add drag idx
    static pfodDwgTypeEnum getDwgSelectedItemType();
    static void sendDwgHideSelectedItemAction(const char* buttonCmd);
    static void sendDwgUnhideSelectedItem(const char* buttonCmd);

    static void setControlSelectedItemFromItemIdx();
    static void cleanUpActionReferenceNos(ValuesClass *touchActionPtr); // cleans up multiple actions referencing the same dwgItem and Saves
    static void closeAction(ValuesClass *touchActionPtr); // for debugging only

    // -- these return true if a forceReload needed
    static bool setControlSelectedColOffset(float f);
    static bool setControlSelectedRowOffset(float f);
    static bool setControlSelectedWidth(float f);
    static bool setControlSelectedHeight(float f);
    static bool setControlSelectedCentered(bool _centered);
    // --
    static size_t deleteControlItem(); // returns new size of list force reload if deleting touchZone
    static size_t appendControlItem(pfodDwgTypeEnum _type); // returns new size of list force reload if deleting touchZone
    static ValuesClass *getControlSelectedItem(); // NEVER returns NULL!!
    static ValuesClass *getControlDisplayedItem(); // NEVER returns Null, returns getControlSelectedItem unless getControlSelectedItem is touchAction() in which case returns the action
    static ValuesClass *getActionActionPtr(ValuesClass *valuesClassPtr);  // NEVER returns NULL!!
    static bool isControlSelectedItemIdxUpdated(); // resets flag on each call
    static pfodDwgTypeEnum getControlSelectedItemType();

    // static methods
    static bool writeCode();
    static bool saveSelectedItem();
    static bool savePushZero();
    static size_t getListDwgValuesSize();
    static pfodDwgTypeEnum getType(size_t idx);

    static size_t getListDwgsValuesSize();
    static bool haveItems();
    static void addItemTouchAction(const char* touchCmd); // current dwg Item with row/col update
    static const char* getTypeAsString(pfodDwgTypeEnum type);
    static const char* getItemTypeAsString(ValuesClass *valuesClassPtr);

    static ValuesClass *createDwgItemValues(pfodDwgTypeEnum selectedDwgItemType, bool addIndex=false, bool initDwgValues = false);
    static bool addTouchActionInput(); // to current Control selected Item
    static bool addTouchAction_1(); // add touchAction to current control selected Item
    static bool addTouchAction_2(); // add touchAction to current control selected Item
    static bool addTouchAction_3(); // add touchAction to current control selected Item
    static bool addTouchAction_4(); // add touchAction to current control selected Item
    static bool addTouchAction_5(); // add touchAction to current control selected Item
    static const char idxPrefix[];// = "_idx_";
    static const char updateIdxPrefix[];// = "_update_idx_";
    static const char touchZoneCmdPrefix[]; //= "_touchZone_cmd_";
    static void sendTouchZoneIndices();
    static void clearIdxNos(); // public for access by CodeSupport
    static void setSeenUpdateIdx();
    static bool haveSeenUpdateIdx();
    static bool getIdxName(SafeString &sfResult, ValuesClass *valuesClassPtr);
    static bool getTouchZoneCmdName(SafeString & sfResult, ValuesClass * valuesClassPtr);
    static void formatLabel(ValuesClass* labelPtr, SafeString &sfFullText); 
    static bool createDefaultDwg();
    static bool checkForZeroFile(const char* dirPath);
    static void removeEmptyDirs();




  private:
    static bool initListOfDwgs();
    static const char pushZeroFileName[15];// = "item_pushZero"
    //static const char DWG_NAME_SEPERATOR = '^'; // between base name and (999) version number
    static pfodLinkedPointerList<ValuesClass> listDwgValues;
    static void setDisplayIdx();

    static bool addTouchAction_1or2(size_t i); // 1 to 5

    static char currentDwgName[DwgName::MAX_DWG_NAME_LEN+1]; // the name of dwg currently being edited allow for adding (999) + '\0'
    static char currentDwgNameBASE[DwgName::MAX_DWG_NAME_LEN+1]; // the name of dwg currently being edited WITHOUT THE  (999)

    static ValuesClass *dwgSelectedItemPtr; // never null
    static ValuesClass *controlSelectedItemPtr; // never null

    static size_t controlItemIdx;
    static size_t dwgItemIdx;
    static bool dwgItemIdxUpdated;
    static bool controlItemIdxUpdated;

    static void cleanUpReferences(); // call before saveing to update isReferenced
    static void cleanUpAllActionReferenceNos(); // called by init() after loading dwg items from files

    static void setDwgItemIdxUpdated();
    static void setControlItemIdxUpdated();
    static bool updateTouchActionInputIndexNo(); // returns true if updated and saved
    static bool updateTouchActionIndexNo(); // returns true if updated and saves

    static bool setDwgItemIdx(size_t idx, bool doActionUpdates = true); // returns true if save performed
    static bool createNewDwg(SafeString &sfNewName);

    static String createDwgNamePath();
    static String getTempPathName();
    static bool readDwgName(SafeString &dwgName);
    static bool readPushZeroFromStream(Stream *in);
    static bool saveItem(size_t idx, SafeString &sfIdxName); // save this item to file
    static bool writeItem(Stream*, size_t idx, SafeString &sfIdxName);
    static void findUniqueName(SafeString &sfNewName);
    
    static String createItemFilePath(size_t idx);
    static String createPushZeroPath();
    static void deleteItemFile(size_t idx);
    static bool appendDwgItemFromStream(Stream * inPtr);
    static pfodDwgsBase *createDwgItemPtr(ValuesClass *valuesClassPtr);
    static struct pfodDwgVALUES *getDwgValuesPtr(size_t idx);  // NEVER returns NULL
    static ValuesClass *getDwgValuesClassPtr(size_t idx);  // NEVER returns NULL
    static ValuesClass *nullValuesClassPtr; // never null NULL;
    static struct pfodDwgVALUES *nullValuesPtr;
    static pfodDwgs *dwgsPtr;
    static pfodParser *parserPtr;
    static bool seenUpdateIdx;// = false;
    static bool saveItems();
    static void sendTouchZoneOutline(ValuesClass *valuesClassPtr, int idx = 0);
    static void getBounds(struct pfodDwgVALUES *valuesPtr, float &leftX, float &topY, float &rightX, float &bottomY);
    static void checkCovered(ValuesClass *tzHigher, ValuesClass *tzLower);
    static void processTouchZonesForCovered();
    static void cleanupIndices();
    static bool skipSend(Stream &in);
    static size_t writeIdxNo; // used for writeItem getIdxName _idx_..
    static void findMaxIndices(size_t &maxIdx, size_t &maxCmdNo);
    
    static size_t findReferredIdx(size_t indexNo, size_t &listIdx); // returns idx() of dwgItem of dwg item found if match else 0, 
    // if indexNo is found then listIdx is the index into the listDwgValues of that dwg item otherwise listIdx is last non-touchZone idx 
    // if there is one

};
#endif
