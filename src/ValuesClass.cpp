/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "ValuesClass.h"
#include "Items.h"
#include <dwgs/pfodDwgsBase.h>

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;

//round x,y w,h to 2dec for storage.
//  float width;
//  float height;
//  float colOffset;
//  float rowOffset;

//enum pfodDwgTypeEnum{pfodRECTANGLE,pfodCIRCLE,pfodLABEL,pfodTOUCH_ZONE,pfodTOUCH_ACTION,pfodTOUCH_ACTION_INPUT,pfodINSERT_DWG,pfodLINE,pfodARC,pfodERASE,pfodHIDE,pfodUNHIDE,pfodINDEX,pfodPUSH_ZERO,pfodNONE};
//static const size_t numberOfpfodDwgTypeEnums = static_cast<int>((pfodDwgTypeEnum)pfodNONE)+1;
//const char ValuesClass::typeEnumNames[numberOfpfodDwgTypeEnums][25] = {"RECTANGLE", "CIRCLE", "LABEL", "TOUCH_ZONE", "TOUCH_ACTION", "TOUCH_ACTION_INPUT", "INSERT_DWG", "LINE", "ARC", "ERASE", "HIDE", "UNHIDE", "INDEX", "PUSH_ZERO", "NONE"};
const char ValuesClass::typeEnumNames[numberOfpfodDwgTypeEnums][25] = {"rectangle()", "circle()", "label()", "touchZone()", "touchAction()", "touchActionInput()", "insertDwg()", "line()", "arc()", "erase()", "hide()", "unhide()", "index()", "pushZero()", "NONE"};
const char ValuesClass::typeEnumOutputNames[numberOfpfodDwgTypeEnums][25] = {"rectangle", "circle", "label", "touchZone", "touchAction", "touchActionInput", "insertDwg", "line", "arc", "erase", "hide", "unhide", "index", "pushZero", "NONE"};

size_t ValuesClass::nextIndexNo = 0; // increment before returning nextIndexNo
size_t ValuesClass::nextCmdNo = 0; //// increment before returning nextCmdNo

ValuesClass::ValuesClass() {
  type = pfodNONE;
  actionActionPtr = NULL; //only for touchAction action( )
  actionValuesClassPtr = NULL;
  action_1Ptr = NULL;
  action_2Ptr = NULL;
  action_3Ptr = NULL;
  action_4Ptr = NULL;
  action_5Ptr = NULL;
  action_Input_Ptr = NULL;
  valuesPtr = new struct pfodDwgVALUES;
  pfodDwgsBase::initValues(valuesPtr); // all but touchCmdStr and actionPtr
  text[0] = '\0';
  units[0] = '\0';
  cmdStr[0] = '\0';
  loadCmdStr[0] = '\0';
  touchCmdStr[0] = '\0';
  valuesPtr->text = text;
  valuesPtr->units = units;
  valuesPtr->cmdStr = cmdStr;
  valuesPtr->loadCmdStr = loadCmdStr;
  valuesPtr->touchCmdStr = touchCmdStr;
  valuesPtr->actionPtr = NULL; // no action yet
  isIndexed = false;
  isReferenced = false;
  covered = false;
  indexNo = 0; // not set update from load or on cleanup after load
  cmdNo = 0;
  //  if (debugPtr) {
  //    debugPtr->print(" new ValuesClass ");   debugPtr->print(cmdStr); debugPtr->print("  valuesPtr->cmdStr "); debugPtr->println(valuesPtr->cmdStr);
  //  }
}

ValuesClass::~ValuesClass() {
  if ((debugPtr) && (actionValuesClassPtr)) {
    Items::printDwgValues("delete actionValuesClassPtr", actionValuesClassPtr);
  }
  delete actionValuesClassPtr; // ok to do if still NULL
  actionValuesClassPtr = NULL;
  if ((debugPtr)  && (actionActionPtr)) {
    Items::printDwgValues("delete actionActionPtr", actionActionPtr);
  }
  delete actionActionPtr;
  actionActionPtr = NULL;
  if ((debugPtr) && (valuesPtr)) {
    debugPtr->print("delete valuesPtr in "); debugPtr->println(getTypeAsStr());
  }
  delete valuesPtr;
  valuesPtr = NULL;
}

void ValuesClass::setNextIndexNo() {
  nextIndexNo++;
  setIndexNo(nextIndexNo); // will set on action if this is a touchAction
}

void ValuesClass::setIndexNo(size_t _idxNo) {
  indexNo = _idxNo;
}

size_t ValuesClass::getIndexNo() {
  return indexNo;
}

void ValuesClass::setNextCmdNo() {
  nextCmdNo++;
  cmdNo = nextCmdNo;
}

void ValuesClass::setCmdNo(size_t _cmdNo) {
  cmdNo = _cmdNo;
  String cmdString('_'); // released at the end of this method
  cmdString += cmdNo;
  if ((type == pfodTOUCH_ACTION) || (type == pfodTOUCH_ACTION_INPUT)) {
    strncpy((char*)touchCmdStr, cmdString.c_str(), sizeof(touchCmdStr));
    ((char*)touchCmdStr)[(sizeof(touchCmdStr) - 1)] = '\0'; // always terminate
    //    if (debugPtr) {
    //      debugPtr->print(getTypeAsStr());
    //      debugPtr->print(" setCmdNo ");   debugPtr->print(touchCmdStr); debugPtr->print("  valuesPtr->touchCmdStr "); debugPtr->println(valuesPtr->touchCmdStr);
    //    }
  } else { // normal cmd( )
    strncpy((char*)cmdStr, cmdString.c_str(), sizeof(cmdStr));
    ((char*)cmdStr)[(sizeof(cmdStr) - 1)] = '\0'; // always terminate
    //    if (debugPtr) {
    //      debugPtr->print(getTypeAsStr());
    //      debugPtr->print(" setCmdNo ");   debugPtr->print(cmdStr); debugPtr->print("  valuesPtr->cmdStr "); debugPtr->println(valuesPtr->cmdStr);
    //    }
  }
}

size_t ValuesClass::getCmdNo() {
  return cmdNo;
}

void ValuesClass::setCovered(bool _isCovered) {
  if (type != pfodTOUCH_ZONE) {
    covered = false;
    return;
  }
  // else
  covered = _isCovered;
}

bool ValuesClass::isCovered() {  // is this touchZone completely covered
  if (type != pfodTOUCH_ZONE) {
    covered = false;
  }
  // else
  return covered;
}

float roundTo2decs(float f) {
  f = floor(f * 100 + 0.5); // add 0.005 and round to 2 dec
  f = f / 100.0;
  return f;
}

//round x,y w,h to 2dec for storage.
//  float width;
//  float height;
//  float colOffset;
//  float rowOffset;
void ValuesClass::setColOffset(float f) {
  if (!valuesPtr) {
    return;
  }
  valuesPtr->colOffset = roundTo2decs(f);
  //  if (debugPtr) {
  //    debugPtr->print("set colsOffset:"); debugPtr->println(valuesPtr->colOffset);
  //  }
}
void ValuesClass::setRowOffset(float f) {
  if (!valuesPtr) {
    return;
  }
  valuesPtr->rowOffset = roundTo2decs(f);
  //  if (debugPtr) {
  //    debugPtr->print("set rowOffset:"); debugPtr->println(valuesPtr->rowOffset);
  //  }
}
void ValuesClass::setWidth(float f) {
  if (!valuesPtr) {
    return;
  }
  valuesPtr->width = roundTo2decs(f);
  //  if (debugPtr) {
  //    debugPtr->print("set width:"); debugPtr->println(valuesPtr->width);
  //  }
}
void ValuesClass::setHeight(float f) {
  if (!valuesPtr) {
    return;
  }
  valuesPtr->height = roundTo2decs(f);
  //  if (debugPtr) {
  //    debugPtr->print("set height:"); debugPtr->println(valuesPtr->height);
  //  }
}

// only for touchZone, true if have any action that is not an actionInput
bool ValuesClass::hasAction() {
  return (hasAction1() || hasAction2() || hasAction3() || hasAction4() || hasAction5()); 
}


bool ValuesClass::hasAction1() {
  if ((type != pfodTOUCH_ZONE) || (!actionValuesClassPtr)) {
    return false; // only for touchZones
  }
  // else
  return (getAction1Ptr() != NULL);
}

bool ValuesClass::hasAction2() {
  if ((type != pfodTOUCH_ZONE) || (!actionValuesClassPtr)) {
    return false; // only for touchZones
  }
  // else
  return (getAction2Ptr() != NULL);
}

bool ValuesClass::hasAction3() {
  if ((type != pfodTOUCH_ZONE) || (!actionValuesClassPtr)) {
    return false; // only for touchZones
  }
  // else
  return (getAction3Ptr() != NULL);
}

bool ValuesClass::hasAction4() {
  if ((type != pfodTOUCH_ZONE) || (!actionValuesClassPtr)) {
    return false; // only for touchZones
  }
  // else
  return (getAction4Ptr() != NULL);
}

bool ValuesClass::hasAction5() {
  if ((type != pfodTOUCH_ZONE) || (!actionValuesClassPtr)) {
    return false; // only for touchZones
  }
  // else
  return (getAction5Ptr() != NULL);
}

bool ValuesClass::hasActionInput() {
  if ((type != pfodTOUCH_ZONE) || (!actionValuesClassPtr)) {
    return false; // only for touchZones
  }
  // else
  return (getActionInputPtr() != NULL);
}

ValuesClass *ValuesClass::getActionActionPtr() {
  if (type != pfodTOUCH_ACTION) {
    return NULL;
  }
  return actionActionPtr; // may be null
}

ValuesClass *ValuesClass::getActionsPtr() {
  return actionValuesClassPtr; // may be null
}

ValuesClass *ValuesClass::getActionInputPtr() {
  return action_Input_Ptr; // may be null
}

ValuesClass *ValuesClass::getAction1Ptr() {
  return action_1Ptr; // may be null
}
ValuesClass *ValuesClass::getAction2Ptr() {
  return action_2Ptr; // may be null
}
ValuesClass *ValuesClass::getAction3Ptr() {
  return action_3Ptr; // may be null
}
ValuesClass *ValuesClass::getAction4Ptr() {
  return action_4Ptr; // may be null
}
ValuesClass *ValuesClass::getAction5Ptr() {
  return action_5Ptr; // may be null
}

// removes this ptr from the action list, returns false if failed
bool ValuesClass::removeAction(ValuesClass *actionPtr) {
  if (debugPtr) {
    debugPtr->println("removeAction");
  }
  if (!actionPtr) {
    if (debugPtr) {
      debugPtr->println(" removeAction: NULL actionPtr");
    }
    return false;
  }
  if (type != pfodTOUCH_ZONE) {
    if (debugPtr) {
      debugPtr->println(" removeAction this is not a touchZone");
    }
    return false; // not removing from touch zone
  }
  if ((actionPtr->type != pfodTOUCH_ACTION) && (actionPtr->type != pfodTOUCH_ACTION_INPUT)) {
    if (debugPtr) {
      debugPtr->println(" removeAction: actionPtr not an action");
    }
    return false; // not removing a touch action
  }

  ValuesClass *lastActionClassPtr = this;
  ValuesClass *actionClassPtr = actionValuesClassPtr;
  while (actionClassPtr) {
    if (actionClassPtr == actionPtr) {
      // found it remove it
      Items::printDwgValues(" found actionPtr in list", actionClassPtr);
      // remove from the list
      lastActionClassPtr->actionValuesClassPtr = actionPtr->actionValuesClassPtr;
      Items::printDwgValues(" lastActionClassPtr ", lastActionClassPtr);
      Items::printDwgValues(" now pointing to  ", lastActionClassPtr->actionValuesClassPtr);
      actionPtr->actionValuesClassPtr = NULL;
      if (actionPtr == action_1Ptr) {
        action_1Ptr = NULL;
      }
      if (actionPtr == action_2Ptr) {
        action_2Ptr = NULL;
      }
      if (actionPtr == action_3Ptr) {
        action_3Ptr = NULL;
      }
      if (actionPtr == action_4Ptr) {
        action_4Ptr = NULL;
      }
      if (actionPtr == action_5Ptr) {
        action_5Ptr = NULL;
      }
      if (actionPtr == action_Input_Ptr) {
        action_Input_Ptr = NULL;
      }
      delete actionPtr;
      actionPtr = NULL;
      return true;
    } // else keep looking
    lastActionClassPtr = actionClassPtr;
    actionClassPtr = actionClassPtr->actionValuesClassPtr; // next one in the chain
  }
  if (debugPtr) {
    debugPtr->println(" removeAction: actionPtr not found");
  }
  return false;
}

// if Input just replace it
// if action check cmd and replace any with same cmd
// else add
// no == 0 for touchActionInput, 1 for action1, 2 for action2, 3, 4, 5 etc
bool ValuesClass::appendAction(ValuesClass *actionPtr, size_t no) {
  if (debugPtr) {
    debugPtr->println("appendAction");
  }
  if (!actionPtr) {
    return false;
  }
  if (type != pfodTOUCH_ZONE) {
    return false; // not adding to touchZone
  }

  pfodDwgTypeEnum actionType = actionPtr->type;
  if ((actionType != pfodTOUCH_ACTION) && (actionType != pfodTOUCH_ACTION_INPUT)) {
    if (debugPtr) {
      debugPtr->print("return false not action or input  type : ");   debugPtr->println(typeToStr(actionType));
    }
    return false;
  }
  cSFP(sfTouchActionCmd, actionPtr->touchCmdStr);
  cSFP(sfTouchZoneCmd, cmdStr);
  if (sfTouchActionCmd != sfTouchZoneCmd) {
    if (debugPtr) {
      debugPtr->print("return false not action cmd :"); debugPtr->print(sfTouchActionCmd); debugPtr->print(" does not match touchZone cmd :");   debugPtr->println(sfTouchZoneCmd);
    }
    return false; // missmatched cmds
  }
  if ((no != 0) && (no != 1) && (no != 2) && (no != 3) && (no != 4) && (no != 5)) {
    if (debugPtr) {
      debugPtr->print("return false invalid no:");   debugPtr->println(no);
    }
    return false;
  }
  if ( ((no == 0) && (actionPtr->type != pfodTOUCH_ACTION_INPUT)) ||
       ((no != 0) && (actionPtr->type == pfodTOUCH_ACTION_INPUT)) ) {
    if (debugPtr) {
      debugPtr->print("return false missmatch no:");   debugPtr->print(no); debugPtr->print(" and "); debugPtr->println(typeToStr(actionType));
    }
    return false;
  }
  if ( (actionPtr->type == pfodTOUCH_ACTION) && (! ((no >= 1) && (no <= 5)))) {
    if (debugPtr) {
      debugPtr->print("return false missmatch no:");   debugPtr->print(no); debugPtr->print(" and "); debugPtr->println(typeToStr(actionType));
    }
    return false;
  }
  if (no == 0) {
    if (action_Input_Ptr) {
      if (debugPtr) {
        debugPtr->println("return false already have touchActionInput");
      }
      return false;
    } else {
      action_Input_Ptr = actionPtr;
    }
  } else if (no == 1) {
    if (action_1Ptr) {
      if (debugPtr) {
        debugPtr->println("return false already have touchAction 1");
      }
      return false;
    } else {
      action_1Ptr = actionPtr;
    }
  } else if (no == 2) {
    if (action_2Ptr) {
      if (debugPtr) {
        debugPtr->println("return false already have touchAction 2");
      }
      return false;
    } else {
      action_2Ptr = actionPtr;
    }
  } else if (no == 3) {
    if (action_3Ptr) {
      if (debugPtr) {
        debugPtr->println("return false already have touchAction 3");
      }
      return false;
    } else {
      action_3Ptr = actionPtr;
    }
  } else if (no == 4) {
    if (action_4Ptr) {
      if (debugPtr) {
        debugPtr->println("return false already have touchAction 4");
      }
      return false;
    } else {
      action_4Ptr = actionPtr;
    }
  } else if (no == 5) {
    if (action_5Ptr) {
      if (debugPtr) {
        debugPtr->println("return false already have touchAction 5");
      }
      return false;
    } else {
      action_5Ptr = actionPtr;
    }
  }
  // add to front of list
  actionPtr->actionValuesClassPtr = actionValuesClassPtr;
  actionValuesClassPtr = actionPtr;
  return true;
}

// returns pfodNONE if no match
pfodDwgTypeEnum ValuesClass::sfStrToPfodDwgTypeEnum(SafeString & sfTypeStr) {
  for (size_t i = 0; i < numberOfpfodDwgTypeEnums; i++) {
    if (sfTypeStr == typeEnumOutputNames[i]) {
      return static_cast<pfodDwgTypeEnum>(i);
    }
  }
  return pfodNONE;
}

const char* ValuesClass::getTypeAsStr() {
  if ( (((int)type) >= numberOfpfodDwgTypeEnums) || (((int)type) < 0)) {
    return typeEnumNames[numberOfpfodDwgTypeEnums-1];
  }
  return typeEnumNames[type];
}

const char* ValuesClass::typeToStr(pfodDwgTypeEnum type) {
  if ( (((int)type) >= numberOfpfodDwgTypeEnums) || (((int)type) < 0)) {
    return typeEnumNames[numberOfpfodDwgTypeEnums-1];
  }
  return typeEnumNames[type];
}


const char* ValuesClass::getTypeAsMethodName(pfodDwgTypeEnum type) {
  return typeEnumOutputNames[type];
}

//pfodDwgTypeEnum ValuesClass::getActionType() {
//  if (actionValuesClassPtr == NULL) {
//    return pfodNONE
//  } //else {
//   actionValuesClassPtr->getType();
//}
