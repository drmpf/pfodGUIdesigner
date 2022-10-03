/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include "DwgPanel.h"
#include "pfodGUIdisplay.h"
#include <SafeString.h>
#include "Items.h"

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;
// for later use
void DwgPanel::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

static uint16_t dwgControlsIdx = Items::unReferencedTouchItemIdx - 500; // make sure selected item on top of cross hairs
static uint16_t dwgDisableControlsIdx = Items::dwgReservedIdxs + 2000;

const char DwgPanel::posCmd[5] = "cP"; // center pos command
const char DwgPanel::touchCmd[5] =  "tP";
const char DwgPanel::disabledCmd[5] = "n"; // disabled touch zone never sent
const char DwgPanel::itemUpCmd[5] =  "iU"; // increment selected item idx up
const char DwgPanel::itemDownCmd[5] =  "iD"; // decrement selected item idx
const char DwgPanel::itemForwardCmd[5] =  "iF"; // move forward
const char DwgPanel::itemBackwardCmd[5] =  "iB"; // move backward
const char DwgPanel::showSelectedItemCmd[5] =  "iS"; // hide selected item on touch this cmd NOT processed

float DwgPanel::xCenter = 50;
float DwgPanel::yCenter = 50;
float DwgPanel::scale = 1;
int DwgPanel::dwgSize = Items::initDwgSize;
bool DwgPanel::forceReload = true;


pfodDwgs *DwgPanel::dwgsPtr = NULL;
pfodParser *DwgPanel::parserPtr = NULL;


float DwgPanel::getScale() {
  return scale;
}

void DwgPanel::setZero(float _xCenter, float _yCenter, float _scale) {
  xCenter = limitFloat(_xCenter, -dwgSize, dwgSize);
  yCenter = limitFloat(_yCenter, -dwgSize, dwgSize);
  scale = limitFloat(_scale, 0.01, 100);
  reload();
}


void DwgPanel::reload() {
  forceReload = true;
}

float DwgPanel::limitFloat(float in, float lowerLimit, float upperLimit) {
  if (upperLimit < lowerLimit) { // swap
    float temp = lowerLimit;
    lowerLimit = upperLimit;
    upperLimit = temp;
  }
  if (in < lowerLimit) {
    in = lowerLimit;
  }
  if (in > upperLimit) {
    in = upperLimit;
  }
  return in;
}

DwgPanel::DwgPanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd) {
  dwgsPtr = _dwgsPtr;
  parserPtr = _parserPtr;
  loadCmd = _loadCmd;
  dwgSize = 50;
  adjustCenter = false;
  xCenter = dwgSize / 2.0; yCenter = dwgSize / 2.0;
}

void DwgPanel::setDwgSize(int size) {
  dwgSize = size;
  fullReload();
}

const char *DwgPanel::getDwgLoadCmd() {
  return loadCmd;
}

void DwgPanel::setShowMoveTouch(bool flag) {
  moveTouch = flag;
  //  if (debugPtr) {
  //    debugPtr->print("setShow..:"); debugPtr->println(moveTouch ? "true" : "false");
  //  }
}

bool DwgPanel::processDwgLoad() {
  if (!parserPtr->cmdEquals(loadCmd)) { // pfodApp is asking to load dwg "cp"
    return false; // not handled
  }
  if (parserPtr->isRefresh() && !forceReload) { // refresh and not forceReload just send update
    sendDrawingUpdates();
  } else {
    forceReload = false; //
    long pfodLongRtn = 0;
    uint8_t* pfodFirstArg = parserPtr->getFirstArg(); // may point to \0 if no arguments in this msg.
    parserPtr->parseLong(pfodFirstArg, &pfodLongRtn); // parse first arg as a long
    sendDrawing(pfodLongRtn);
  }
  return true;
}

bool DwgPanel::isAdjustCenter() {
  return adjustCenter;
}

bool DwgPanel::processItemShowCmd() {
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // filter forced to TOUCH if have added a touchActionInput
  if (parserPtr->dwgCmdEquals(showSelectedItemCmd) && (parserPtr->isUp() || parserPtr->isTouch())) {
    // only process finger UP
    return true;
  } // else not our cmd or not finger up
  return false;
}

bool DwgPanel::processDwgCmds() {
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  if (parserPtr->dwgCmdEquals(posCmd)) {
    // toggle adjust
    adjustCenter = !adjustCenter;
    reload(); // new center
    return true;

  } else if (parserPtr->dwgCmdEquals(touchCmd)) {
    if (!parserPtr->isUp()) { // ignore if still draging
      return false;
    }
    if (adjustCenter) {
      // change offset
      xCenter += parserPtr->getTouchedX();
      yCenter += parserPtr->getTouchedY();
      //            if (debugPtr) {
      //              debugPtr->print("DwgPanel touch xCenter:"); debugPtr->print(xCenter); debugPtr->print(" yCenter:"); debugPtr->print(yCenter); debugPtr->println();
      //            }
      Items::savePushZero();
      reload(); // new center
    } else {
      // get x y
      if (Items::setDwgSelectedOffset(parserPtr->getTouchedX(), parserPtr->getTouchedY())) { //rounds to 2 decs NEVER returns NULL!!
        //fullReload();
      }
      Items::saveSelectedItem();
      //      if (debugPtr) {
      //        debugPtr->print("DwgPanel touch x:"); debugPtr->print(parserPtr->getTouchedX()); debugPtr->print(" y:"); debugPtr->print(parserPtr->getTouchedY()); debugPtr->println();
      //      }
    }
    return true;
  } else if (parserPtr->dwgCmdEquals(itemUpCmd)) {
    Items::increaseDwgSelectedItemIdx();
    return true;
  } else if (parserPtr->dwgCmdEquals(itemDownCmd)) {
    Items::decreaseDwgSelectedItemIdx();
    return true;

  } else if (parserPtr->dwgCmdEquals(itemForwardCmd)) {
    if ((Items::getControlSelectedItem()->type == pfodTOUCH_ACTION) || (Items::getControlSelectedItem()->type == pfodTOUCH_ACTION_INPUT)) {
      // ignore this cmd when editing actions
      return false;
    }
    fullReload(); // after move Note: touchZones can only amoung themselves
    Items::forwardDwgSelectedItemIdx(); // just swaps file names and move dwgItem in list
    //Items::saveSelectedItem(); // NO NEED TO force full save
    return true;

  } else if (parserPtr->dwgCmdEquals(itemBackwardCmd)) {
    if ((Items::getControlSelectedItem()->type == pfodTOUCH_ACTION) || (Items::getControlSelectedItem()->type == pfodTOUCH_ACTION_INPUT)) {
      // ignore this cmd when editing actions
      return false;
    }
    fullReload(); // after move Note: touchZones can only amoung themselves
    Items::backwardDwgSelectedItemIdx(); // just swaps file names and move dwgItem in list
    //Items::saveSelectedItem(); // NO NEED TO  force full save
    return true;
  }
  return false;  // not processed
}

void DwgPanel::hideUpDownOnTouch(const char* _cmd) {
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().idx(dwgControlsIdx + 5)
  ).send();
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().idx(dwgControlsIdx + 7)
  ).send();
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().cmd(itemDownCmd)
  ).send();
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().cmd(itemUpCmd)
  ).send();
}

void DwgPanel::hideLayerOnTouch(const char* _cmd) {
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().idx(dwgControlsIdx + 10)
  ).send();
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().idx(dwgControlsIdx + 11)
  ).send();
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().cmd(itemForwardCmd)
  ).send();
  dwgsPtr->touchAction().cmd(_cmd).action(
    dwgsPtr->hide().cmd(itemBackwardCmd)
  ).send();
}

void DwgPanel::addDwgControls_1() {
  dwgsPtr->touchZone().cmd(disabledCmd).idx(dwgDisableControlsIdx).filter(dwgsPtr->TOUCH_DISABLED).size(dwgSize, dwgSize).send();

  // define location for showSelectedItem for later touchZone update
  dwgsPtr->circle().idx(dwgControlsIdx + 15).offset(2.5, dwgSize - 2.5).radius(2.5).color(dwgsPtr->BLACK).send();
  dwgsPtr->label().idx(dwgControlsIdx + 16).offset(2.5, dwgSize - 2.5).text("S").fontSize(1).color(dwgsPtr->BLACK).send();
  dwgsPtr->touchZone().cmd(showSelectedItemCmd).idx(dwgControlsIdx + 17).send();
  dwgsPtr->hide().idx(dwgControlsIdx + 15).send();
  dwgsPtr->hide().idx(dwgControlsIdx + 16).send();
  dwgsPtr->hide().cmd(showSelectedItemCmd).send();

  // dwgsPtr->hide().cmd(disabledCmd).send(); done at end of update
  dwgsPtr->pushZero(xCenter, yCenter);
  Items::sendTouchZoneIndices();

  dwgsPtr->line().idx(dwgControlsIdx - 2).offset(0, -dwgSize * 2).size(0, dwgSize * 4).color(dwgsPtr->BLACK).send();
  dwgsPtr->line().idx(dwgControlsIdx - 1).offset(-dwgSize * 2, 0).size(dwgSize * 4, 0).color(dwgsPtr->BLACK).send();
  dwgsPtr->popZero();

  dwgsPtr->hide().idx(dwgControlsIdx - 2).send();
  dwgsPtr->hide().idx(dwgControlsIdx - 1).send();

  // zero position control dwgControlsIdx+1 to 4
  dwgsPtr->circle().idx(dwgControlsIdx + 4).offset(2.5, 2.5).radius(2.5).color(dwgsPtr->BLACK).send();
  if (adjustCenter) {
    dwgsPtr->circle().idx(dwgControlsIdx + 1).offset(2.5, 2.5).radius(2.5).filled().color(dwgsPtr->BLACK).send();
    dwgsPtr->line().idx(dwgControlsIdx + 2).offset(0, 2.5).size(5, 0).color(dwgsPtr->WHITE).send();
    dwgsPtr->line().idx(dwgControlsIdx + 3).offset(2.5, 5).size(0, -5).color(dwgsPtr->WHITE).send();
  } else {
    dwgsPtr->circle().idx(dwgControlsIdx + 1).offset(2.5, 2.5).radius(2.5).filled().color(dwgsPtr->WHITE).send();
    dwgsPtr->line().idx(dwgControlsIdx + 2).offset(0, 2.5).size(5, 0).color(dwgsPtr->BLACK).send();
    dwgsPtr->line().idx(dwgControlsIdx + 3).offset(2.5, 5).size(0, -5).color(dwgsPtr->BLACK).send();
  }
  dwgsPtr->touchZone().cmd(posCmd).idx(dwgControlsIdx).offset(2.5, 2.5).centered().size(1, 1).send();
  dwgsPtr->touchAction().cmd(posCmd).action(
    dwgsPtr->circle().idx(dwgControlsIdx + 1).offset(2.5, 2.5).radius(2.5).color(dwgsPtr->BLACK)
  ).send();
  dwgsPtr->touchAction().cmd(posCmd).action(
    dwgsPtr->unhide().cmd(disabledCmd)
  ).send();

  dwgsPtr->pushZero(10.5, dwgSize - 2.5);
  dwgsPtr->index().idx(posXLabel_idx).send();
  dwgsPtr->index().idx(posYLabel_idx).send();
  dwgsPtr->popZero();

  // item show control idx's dwgControlsIdx+5 to 9
  dwgsPtr->arc().idx(dwgControlsIdx + 5).offset(dwgSize - 12, dwgSize - 4).radius(3.5).start(-70).angle(-40).filled().color(dwgsPtr->BLACK).send(); // UP
  { // release String after this block
    String itemIdxStr(Items::getDwgSelectedItemIdx() + 1);
    if (!Items::haveItems()) {
      itemIdxStr = "0/0";
    } else {
      itemIdxStr += "/";
      itemIdxStr += Items::getListDwgValuesSize();
    }
    dwgsPtr->label().idx(dwgControlsIdx + 6).offset(dwgSize - 7, dwgSize - 2).fontSize(0).text(itemIdxStr.c_str()).color(dwgsPtr->BLACK).send();
  }
  dwgsPtr->arc().idx(dwgControlsIdx + 7).offset(dwgSize - 2, dwgSize - 0.5).radius(3.5).start(70).angle(40).filled().color(dwgsPtr->BLACK).send(); // Down

  dwgsPtr->touchZone().cmd(itemUpCmd).idx(dwgControlsIdx + 8).offset(dwgSize - 12, dwgSize - 2).centered().size(1, 1).send();
  hideUpDownOnTouch(itemUpCmd);
  dwgsPtr->touchZone().cmd(itemDownCmd).idx(dwgControlsIdx + 9).offset(dwgSize - 2, dwgSize - 2).centered().size(1, 1).send();
  hideUpDownOnTouch(itemDownCmd);

  if (Items::haveItems()) {
    dwgsPtr->unhide().idx(dwgControlsIdx + 7).send();
    dwgsPtr->unhide().cmd(itemDownCmd).send();
    dwgsPtr->unhide().idx(dwgControlsIdx + 5).send();
    dwgsPtr->unhide().cmd(itemUpCmd).send();
    if (Items::getDwgSelectedItemIdx() == 0) {
      dwgsPtr->hide().idx(dwgControlsIdx + 7).send();
      dwgsPtr->hide().cmd(itemDownCmd).send();
    }
    if (Items::getDwgSelectedItemIdx() >= Items::getListDwgValuesSize() - 1) {
      dwgsPtr->hide().idx(dwgControlsIdx + 5).send();
      dwgsPtr->hide().cmd(itemUpCmd).send();
    }
  }

  if (adjustCenter) { // skip item control
    dwgsPtr->hide().idx(dwgControlsIdx + 5).send();
    dwgsPtr->hide().idx(dwgControlsIdx + 6).send();
    dwgsPtr->hide().idx(dwgControlsIdx + 7).send();
    dwgsPtr->hide().cmd(itemDownCmd).send();
    dwgsPtr->hide().cmd(itemUpCmd).send();
    dwgsPtr->hide().idx(posXLabel_idx).send();
    dwgsPtr->hide().idx(posYLabel_idx).send();

  } else {
    dwgsPtr->unhide().idx(posXLabel_idx).send();
    dwgsPtr->unhide().idx(posYLabel_idx).send();
  }

  // move layer  position control dwgControlsIdx+10 to 14
  dwgsPtr->arc().idx(dwgControlsIdx + 10).offset(dwgSize - 13, 2).radius(3.5).start(-20).angle(40).filled().color(dwgsPtr->BLACK).send(); // Backward
  dwgsPtr->arc().idx(dwgControlsIdx + 11).offset(dwgSize - 0.5, 2).radius(3.5).start(200).angle(-40).filled().color(dwgsPtr->BLACK).send(); // Forward
  dwgsPtr->label().idx(dwgControlsIdx + 12).offset(dwgSize - 6.75, 2 ).fontSize(0).text("B/F").color(dwgsPtr->BLACK).send();

  dwgsPtr->touchZone().cmd(itemBackwardCmd).idx(dwgControlsIdx + 13).offset(dwgSize - 11, 1).centered().size(1, 1).send();
  hideLayerOnTouch(itemBackwardCmd);
  dwgsPtr->touchZone().cmd(itemForwardCmd).idx(dwgControlsIdx + 14).offset(dwgSize - 2, 1).centered().size(1, 1).send();
  hideLayerOnTouch(itemForwardCmd);

  if (Items::haveItems()) {
    dwgsPtr->unhide().idx(dwgControlsIdx + 10).send();
    dwgsPtr->unhide().cmd(itemForwardCmd).send();
    dwgsPtr->unhide().idx(dwgControlsIdx + 11).send();
    dwgsPtr->unhide().cmd(itemBackwardCmd).send();
    dwgsPtr->unhide().idx(dwgControlsIdx + 12).send();
    if ((Items::getDwgSelectedItemIdx() == 0) || (Items::getDwgSelectedItemType() == pfodTOUCH_ZONE)) {
      dwgsPtr->hide().idx(dwgControlsIdx + 10).send();
      dwgsPtr->hide().cmd(itemBackwardCmd).send();
    }
    if ((Items::getDwgSelectedItemIdx() >= Items::getListDwgValuesSize() - 1)  // at top
        || ((Items::getDwgSelectedItemIdx() < Items::getListDwgValuesSize() - 1) && (Items::getType(Items::getDwgSelectedItemIdx() + 1) == pfodTOUCH_ZONE))) { // next one a pfodTOUCH_ZONE
      dwgsPtr->hide().idx(dwgControlsIdx + 11).send();
      dwgsPtr->hide().cmd(itemForwardCmd).send();
    }
  }

  if (adjustCenter) { // skip item control
    dwgsPtr->hide().idx(dwgControlsIdx + 10).send();
    dwgsPtr->hide().idx(dwgControlsIdx + 11).send();
    dwgsPtr->hide().idx(dwgControlsIdx + 12).send();
    dwgsPtr->hide().cmd(itemForwardCmd).send();
    dwgsPtr->hide().cmd(itemBackwardCmd).send();
  } else {
  }

}

void DwgPanel::addDwgControls_2() {
}

void DwgPanel::sendDrawing(long dwgIdx) {
  if (dwgIdx == 0) {
    dwgsPtr->start(dwgSize, dwgSize, dwgsPtr->WHITE, true); //true means more to come,  background defaults to WHITE if omitted i.e. dwgsPtr->start(50,30);
    parserPtr->sendRefreshAndVersion(0); //need to set a version number for image refresh to work!! send the parser version to cache this image, refreshes every 10sec
    addDwgControls_1();
    dwgsPtr->end();

  } else if (dwgIdx == 1) {
    dwgsPtr->startUpdate(true);  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
    //addDwgControls_2();
    dwgsPtr->pushZero(xCenter, yCenter);
    dwgsPtr->popZero();
    dwgsPtr->end();

  } else if (dwgIdx == 2) {
    dwgsPtr->startUpdate(true);  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
    if (adjustCenter) {
      dwgsPtr->unhide().idx(dwgControlsIdx - 2).send();
      dwgsPtr->unhide().idx(dwgControlsIdx - 1).send();
    }
    dwgsPtr->pushZero(xCenter, yCenter); // add zero axies
    dwgsPtr->line().offset(0, -dwgSize * 2).size(0, dwgSize * 4).color(dwgsPtr->SILVER).send();
    dwgsPtr->line().offset(-dwgSize * 2, 0).size(dwgSize * 4, 0).color(dwgsPtr->SILVER).send();
    dwgsPtr->popZero();

    dwgsPtr->pushZero(xCenter, yCenter + 10); // add offset here so finger does not cover the item being moved
    dwgsPtr->touchZone().offset(-xCenter, -yCenter).size(dwgSize, dwgSize).cmd(touchCmd).filter(dwgsPtr->DOWN_UP).send(); // cover whole area only update dwg on UP
    dwgsPtr->popZero();

    dwgsPtr->pushZero(xCenter, yCenter);
    Items::sendDwgItems(0, size_t(-1)); // stops at itemIdx
    dwgsPtr->popZero();
    dwgsPtr->end();

  } else if (dwgIdx == 3) {
    dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
    update();
    dwgsPtr->end();

  } else {
    parserPtr->print(F("{}")); // always return somthing
  }
}


void DwgPanel::sendDrawingUpdates() {
  if (debugPtr) {
    debugPtr->print("DwgPanel::sendDrawingUpdates:");  debugPtr->println();
  }
  dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
  update();
  dwgsPtr->end();
}

void DwgPanel::update() {
  if (adjustCenter) {
    Items::sendDwgSelectedItem(false); // no special idx
    dwgsPtr->touchZone().offset(-xCenter, -yCenter).size(dwgSize, dwgSize).cmd(touchCmd).filter(dwgsPtr->DOWN_UP).send(); // cover whole area only update dwg on UP
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->line().idx(dwgControlsIdx - 2).offset(dwgsPtr->TOUCHED_X, -dwgSize * 2).size(0, dwgSize * 4).color(dwgsPtr->BLACK)
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->line().idx(dwgControlsIdx - 1).offset(-dwgSize * 2, dwgsPtr->TOUCHED_Y).size(dwgSize * 4, 0).color(dwgsPtr->BLACK)
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->label().idx(posXLabel_idx).bold().text("(").decimals(0).intValue(dwgsPtr->TOUCHED_X).units(",").right().fontSize(-1) // scale to center
      .displayMax(dwgSize).displayMin(0).maxValue(dwgSize - xCenter).minValue(-xCenter) // scale to allow for pushZero to current center
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->label().idx(posYLabel_idx).bold().decimals(0).intValue(dwgsPtr->TOUCHED_Y).units(")").left().fontSize(-1)
      .displayMax(dwgSize).displayMin(0).maxValue(dwgSize - yCenter).minValue(-yCenter) // scale to allow for pushZero to current center
    ).send();

    //    dwgsPtr->hide().idx(posXLabel_idx).send();
    //    dwgsPtr->hide().idx(posYLabel_idx).send();
    dwgsPtr->hide().idx(dwgControlsIdx + 15).send();
    dwgsPtr->hide().idx(dwgControlsIdx + 16).send();
    dwgsPtr->hide().cmd(showSelectedItemCmd).send();

  } else {
    Items::sendDwgSelectedItem();

    struct pfodDwgVALUES *valuesPtr = Items::getDwgSelectedItem()->valuesPtr;
    dwgsPtr->label().idx(posXLabel_idx).right().bold().text("(").floatReading(valuesPtr->colOffset).decimals(0).units(",").fontSize(-1).send();
    dwgsPtr->label().idx(posYLabel_idx).left().bold().floatReading(valuesPtr->rowOffset).decimals(0).units(")").fontSize(-1).send();

    dwgsPtr->touchZone().cmd(showSelectedItemCmd).idx(dwgControlsIdx + 17).offset(2.5, dwgSize - 2.5).centered().size(2.5, 2.5)
    .filter(dwgsPtr->DOWN_UP).send(); // finger sends msg for DOWN_UP filter screen when cmd processed above
    // NOTE: filter will be forced to TOUCH if a touchActionInput added
    dwgsPtr->touchAction().cmd(showSelectedItemCmd).action(
      dwgsPtr->hide().idx(Items::unReferencedTouchItemIdx)
    ).send();
    Items::sendDwgHideSelectedItemAction(showSelectedItemCmd);
    dwgsPtr->touchAction().cmd(showSelectedItemCmd).action(
      dwgsPtr->hide().idx(dwgControlsIdx + 15)
    ).send();
    dwgsPtr->touchAction().cmd(showSelectedItemCmd).action(
      dwgsPtr->hide().idx(dwgControlsIdx + 16)
    ).send();

    dwgsPtr->touchZone().offset(-xCenter, -yCenter).size(dwgSize, dwgSize).cmd(touchCmd).filter(dwgsPtr->DOWN_UP).send(); // cover whole area only update dwg on UP
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->unhide().idx(dwgControlsIdx - 2)
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->unhide().idx(dwgControlsIdx - 1)
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->line().idx(dwgControlsIdx - 2).offset(dwgsPtr->TOUCHED_X, -dwgSize * 2).size(0, dwgSize * 4).color(255) // very light line
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->line().idx(dwgControlsIdx - 1).offset(-dwgSize * 2, dwgsPtr->TOUCHED_Y).size(dwgSize * 4, 0).color(255) // very light line
    ).send();

    Items::addItemTouchAction(touchCmd);
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->label().idx(posXLabel_idx).bold().text("(").decimals(0).intValue(dwgsPtr->TOUCHED_X).units(",").right().fontSize(-1)
    ).send();
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->label().idx(posYLabel_idx).bold().decimals(0).intValue(dwgsPtr->TOUCHED_Y).units(")").left().fontSize(-1)
    ).send();

    if (Items::haveItems()) {
      dwgsPtr->unhide().idx(posXLabel_idx).send();
      dwgsPtr->unhide().idx(posYLabel_idx).send();
      dwgsPtr->unhide().idx(dwgControlsIdx + 15).send();
      dwgsPtr->unhide().idx(dwgControlsIdx + 16).send();
      dwgsPtr->unhide().cmd(showSelectedItemCmd).send();

      Items::printDwgValues(" Panel getControlSelectedItem()", Items::getControlSelectedItem());
      if ((Items::getControlSelectedItem()->type == pfodTOUCH_ACTION) || (Items::getControlSelectedItem()->type == pfodTOUCH_ACTION)) {
        // disable forward backward
        dwgsPtr->hide().idx(dwgControlsIdx + 10).send();
        dwgsPtr->hide().cmd(itemForwardCmd).send();
        dwgsPtr->hide().idx(dwgControlsIdx + 11).send();
        dwgsPtr->hide().cmd(itemBackwardCmd).send();
        dwgsPtr->hide().idx(dwgControlsIdx + 12).send();
      }

    } else {
      dwgsPtr->hide().idx(posXLabel_idx).send();
      dwgsPtr->hide().idx(posYLabel_idx).send();
      dwgsPtr->hide().idx(dwgControlsIdx + 15).send();
      dwgsPtr->hide().idx(dwgControlsIdx + 16).send();
      dwgsPtr->hide().cmd(showSelectedItemCmd).send();
    }
  }

  if (moveTouch) {
    dwgsPtr->unhide().cmd(touchCmd).send();
  } else {
    dwgsPtr->hide().cmd(touchCmd).send();
  }
  dwgsPtr->hide().cmd(disabledCmd).send();
}

float DwgPanel::getXCenter() {
  return xCenter;
}
float DwgPanel::getYCenter() {
  return yCenter;
}
float DwgPanel::getXPos() {
  return xPos;
}
float DwgPanel::getYPos() {
  return yPos;
}
