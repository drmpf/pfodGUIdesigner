/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "DwgsList.h"
#include "DwgName.h"
#include "Items.h"
#include <SafeString.h>

const char DwgsList::disabledCmd[5] = "n"; // disabled touch zone never sent
const char DwgsList::closeCmd[3] = "C";
static uint16_t dwgDisableControlsIdx = Items::dwgReservedIdxs + 2000; // actually this unique to color panel but make it the same as dwg panel

static int yStep = 1;

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;
// for later use
void DwgsList::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

DwgsList::DwgsList(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd, const char* _touchCmd) {
  dwgsPtr = _dwgsPtr;
  parserPtr = _parserPtr;
  loadCmd = _loadCmd;
  touchCmd = _touchCmd;
  disablePanel = false;
  forceReload = true;
}

const char *DwgsList::getDwgLoadCmd() {
  return loadCmd;
}

void DwgsList::reload() {
  forceReload = true;
}

bool DwgsList::processDwgLoad() {
  if (!parserPtr->cmdEquals(loadCmd)) { // pfodApp is asking to load dwg "ch"
    return false; // not handled
  }
  if (parserPtr->isRefresh() && !forceReload) { // refresh and not forceReload just send update
    sendDrawingUpdates();
  } else {
    //forceReload = false; //
    long pfodLongRtn = 0;
    uint8_t* pfodFirstArg = parserPtr->getFirstArg(); // may point to \0 if no arguments in this msg.
    parserPtr->parseLong(pfodFirstArg, &pfodLongRtn); // parse first arg as a long
    sendDrawing(pfodLongRtn);
  }
  return true;
}

bool DwgsList::processDwgCmds() {
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  if (parserPtr->dwgCmdEquals(closeCmd)) {
    return true;  // just leave current dwg unchanged

  } else if (parserPtr->dwgCmdEquals(touchCmd)) {
    // get x y
    int xPos = parserPtr->getTouchedX();
    int yPos = parserPtr->getTouchedY();
    (void)(xPos);
    //    if (debugPtr) {
    //      debugPtr->print("x:"); debugPtr->print(xPos); debugPtr->print(" y:"); debugPtr->print(yPos); debugPtr->println();
    //    }
    // caculate idx
    int nameIdx = (yPos + (yStep / 2)) / yStep; // round by half step
    DwgName *selectedNamePtr = Items::listOfDwgs.get(nameIdx);

    if (debugPtr) {
      if (selectedNamePtr) {
        debugPtr->print("Selected name "); debugPtr->println(selectedNamePtr->name());
      } else {
        debugPtr->println("Selected name NULL");
      }
    }
    if (!selectedNamePtr) {
      // create new Dwg_1 dwg
      Items::createDefaultDwg();
    } else {
      if (strlen(selectedNamePtr->name()) > DwgName::MAX_DWG_NAME_LEN) {
        // error name too long, should not happen
        return true;
      }
      cSF(sfCurrentDwgName, DwgName::MAX_DWG_NAME_LEN);
      sfCurrentDwgName = selectedNamePtr->name(); // truncates if too long
      if (Items::writeDwgName(sfCurrentDwgName)) {
        Items::openDwg();
      }
    }
    return true;
  }// else {
  return false;
}

// assumes dwg is 100 wide
void DwgsList::sendDrawing(long dwgIdx) {
  if (dwgIdx == 0) {
    dwgsPtr->start(12, 16, dwgsPtr->WHITE, true); //true means more to come,  background defaults to WHITE if omitted i.e. dwgsPtr->start(50,30);
    parserPtr->sendRefreshAndVersion(0); //need to set a version number for image refresh to work!! send the parser version to cache this image, refreshes every 10sec
    dwgsPtr->touchZone().cmd(disabledCmd).idx(dwgDisableControlsIdx).filter(dwgsPtr->TOUCH_DISABLED).size(100, 130).send();
    dwgsPtr->hide().cmd(disabledCmd).send();

    dwgsPtr->pushZero(6, 0.5);
    dwgsPtr->label().fontSize(-12).italic().text("Choose Dwg to Open").send();
    dwgsPtr->popZero();

    // add close button
    dwgsPtr->pushZero(11.5, 0.5);
    dwgsPtr->label().fontSize(-9).bold().text("X").send();
    dwgsPtr->circle().idx(closeButton_idx).radius(0.5).send();
    dwgsPtr->touchZone().cmd(closeCmd).centered().size(1, 1).filter(dwgsPtr->DOWN_UP).send(); // slight longer than screen
    dwgsPtr->touchAction().cmd(closeCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->touchAction().cmd(closeCmd).action(
      dwgsPtr->circle().idx(closeButton_idx).filled().radius(0.5)
    ).send();
    dwgsPtr->popZero();


    int nameIdx = 0;
    dwgsPtr->pushZero(0, 1);
    dwgsPtr->index().idx(Selection_idx).send();
    dwgsPtr->touchZone().cmd(touchCmd).size(12, 16).filter(dwgsPtr->DOWN_UP).send(); // slight longer than screen
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->rectangle().idx(Selection_idx).offset(0.1, dwgsPtr->TOUCHED_Y).size(12 - 0.3, 1).color(dwgsPtr->BLUE)
    ).send();
    dwgsPtr->popZero();

    dwgsPtr->pushZero(0.2, 1.5);

    DwgName *dwgNamePtr = Items::listOfDwgs.getFirst();
    if (!dwgNamePtr) {
      dwgsPtr->label().fontSize(-11).bold().left().text(" No Dwgs found").send();
    } else {
      int currentDwgIdx = -1; // not found
      cSF(sfCurrentDwg, DwgName::MAX_DWG_NAME_LEN);
      sfCurrentDwg = Items::getCurrentDwgName();
      size_t listIdx = 0;
      while (dwgNamePtr) {
        if (sfCurrentDwg == dwgNamePtr->name()) {
          currentDwgIdx = listIdx;
        }
        dwgsPtr->label().fontSize(-11).offset(0, nameIdx * yStep).bold().left().text(dwgNamePtr->name()).send();
        nameIdx++;
        if (nameIdx >= MAX_DWG_NAMES) {
          break;
        }
        dwgNamePtr = Items::listOfDwgs.getNext();
        listIdx++;
      }

      // add outline  NOTE: idx(Selection_idx) force this rectangle to use a different Zero, the one set above when the index() cmd sent
      if (currentDwgIdx >= 0) {
        dwgsPtr->rectangle().idx(Selection_idx).offset(0.1, currentDwgIdx).size(12 - 0.3, 1).color(dwgsPtr->BLUE).send();
      }

    }
    dwgsPtr->popZero();
    dwgsPtr->end();

  } else if (dwgIdx == 1) {
    dwgsPtr->startUpdate(false);

    update(); // update with current state

    dwgsPtr->end();

  } else {
    parserPtr->print(F("{}")); // always return somthing
  }
}

void DwgsList::sendDrawingUpdates() {
  dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
  update();
  dwgsPtr->end();
}


void DwgsList::update() {
  dwgsPtr->hide().cmd(disabledCmd).send();
}
