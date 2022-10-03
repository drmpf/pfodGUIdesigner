/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <Arduino.h>
#include "ColourPanel.h"
#include <SafeString.h>

static bool forceReload = true; // for testing force complete reload on every reboot

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;
// for later use
void ColourPanel::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

ColourPanel::ColourPanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd, const char* _touchCmd) {
  dwgsPtr = _dwgsPtr;
  parserPtr = _parserPtr;
  loadCmd = _loadCmd;
  touchCmd = _touchCmd;
  color = 0; // black
}

const char *ColourPanel::getDwgLoadCmd() {
  return loadCmd;
}
int ColourPanel::getSelectedColor() {
  return color;
}

bool ColourPanel::processDwgLoad() {
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


bool ColourPanel::processTouchCmd() {
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  if (!parserPtr->dwgCmdEquals(touchCmd)) {
    return false;
  }
  // get x y
  int xPos = parserPtr->getTouchedX();
  int yPos = parserPtr->getTouchedY();
//  if (debugPtr) {
//    debugPtr->print("x:"); debugPtr->print(xPos); debugPtr->print(" y:"); debugPtr->print(yPos); debugPtr->println();
//  }
  int c = 0;
  if (yPos >= 10) {
    c += 8;
  }
  c += xPos / 10;  // touchZone goes from 0 to 79 only
  color = c;
  if (debugPtr) {
    debugPtr->print("color:"); debugPtr->print(color); debugPtr->println();
  }
  return true;
}

void ColourPanel::sendDrawing(long dwgIdx) {
  if (dwgIdx == 0) {
    dwgsPtr->start(90, 30, dwgsPtr->WHITE, false); //true means more to come,  background defaults to WHITE if omitted i.e. dwgs.start(50,30);
    parserPtr->sendRefreshAndVersion(0); //need to set a version number for image refresh to work!! send the parser version to cache this image, refreshes every 10sec
    dwgsPtr->pushZero(5, 5);
    // draw colour squares
    for (int i = 0; i < 2; i++) {
      for (int k = 0; k < 8; k++) {
        dwgsPtr->rectangle().filled().color(i * 8 + k).size(10, 10).offset(k * 10, i * 10).send();
      }
    }
    // draw grid
    for (int i = 0; i < 3; i++) {
      dwgsPtr->line().offset(0, 10 * i).size(80, 0).send();
    }
    for (int i = 0; i < 9; i++) {
      dwgsPtr->line().offset(10 * i, 0).size(0, 20).send();
    }
    dwgsPtr->touchZone().cmd(touchCmd).size(78, 19).send(); // 0 to 79
    dwgsPtr->touchAction().cmd(touchCmd).action(
      dwgsPtr->rectangle().filled().color(dwgsPtr->WHITE).size(80, 20)
    ).send();
    dwgsPtr->popZero();
    dwgsPtr->end();
  } else {
    parserPtr->print(F("{}")); // always return somthing
  }
}


void ColourPanel::sendDrawingUpdates() {
  dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
  dwgsPtr->end();
}
