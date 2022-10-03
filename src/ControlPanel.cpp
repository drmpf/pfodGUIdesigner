/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <Arduino.h>

#include "pfodGUIdisplay.h"
#include "ControlPanel.h"
#include "DwgPanel.h"
#include <SafeString.h>
#include "Items.h"

//static Stream *debugPtr = &Serial;
static Stream *debugPtr = NULL;
// for later use
void ControlPanel::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

bool ControlPanel::forceReload = true;
const char ControlPanel::TiPCmd[4] = "TiP";

const char ControlPanel::xPosCmd[3]  = "cx";
const char ControlPanel::yPosCmd[3]  = "cy";
const char ControlPanel::rCmd[3]  = "cr";
const char ControlPanel::wCmd[3]  = "cw";
const char ControlPanel::hCmd[3]  = "ch";
const char ControlPanel::FCmd[3] = "cF";
const char ControlPanel::CCmd[3] = "cC";
const char ControlPanel::RCmd[3] = "cR";
const char ControlPanel::SCmd[3] = "cS";
const char ControlPanel::ACmd[3] = "cA";
const char ControlPanel::DCmd[3] = "cD"; // delete
const char ControlPanel::CloseCmd[3] = "ok"; // close
const char ControlPanel::IdxCmd[3] = "cI"; // idx
const char ControlPanel::colorCmd[3] = "cc";
const char ControlPanel::backgroundColorCmd[3] = "cb";
const char ControlPanel::textEditCmd[3] = "ce";
const char ControlPanel::fontSizeCmd[3] = "cs";

const char ControlPanel::LBCmd[3] = "LB"; // label bold
const char ControlPanel::LICmd[3] = "LI"; // label italic
const char ControlPanel::LUCmd[3] = "LU"; // label underline
const char ControlPanel::LACmd[3] = "LA"; // label align
const char ControlPanel::LVCmd[3] = "LV"; // label value
const char ControlPanel::LDCmd[3] = "LD"; // label decimals
const char ControlPanel::LuCmd[3] = "Lu"; // label units

const char ControlPanel::Ta1Cmd[4] = "Ta1"; // touchAction
const char ControlPanel::Ta2Cmd[4] = "Ta2"; // touchAction
const char ControlPanel::Ta3Cmd[4] = "Ta3"; // touchAction
const char ControlPanel::Ta4Cmd[4] = "Ta4"; // touchAction
const char ControlPanel::Ta5Cmd[4] = "Ta5"; // touchAction
const char ControlPanel::TiCmd[3] = "Ti"; // touchInput

const char ControlPanel::disabledCmd[5] = "n"; // disabled touch zone never sent

static uint16_t dwgDisableControlsIdx = Items::dwgReservedIdxs + 2000; // actually this unique to control panel but make it the same as dwg panel
static uint16_t errMsg_idx = Items::dwgReservedIdxs + 1500;

int ControlPanel::dwgSize = Items::initDwgSize;

void ControlPanel::reload() {
  forceReload = true;
}

float ControlPanel::limitFloat(float in, float lowerLimit, float upperLimit) {
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

const char *ControlPanel::getDwgLoadCmd() {
  return loadCmd;
}

void ControlPanel::setDwgSize(unsigned int size) {
  if (size > 255) {
    size = 255;
  }
  dwgSize = size;
  fullReload();
}

int ControlPanel::getDwgSize() {
  return dwgSize;
}

bool ControlPanel::getShowColorPanelFlag() {
  return showColorPanel;
}

/**
   getShowChoicePanelFlag
   returns true if choicePanel should be shown, else false

   ChoicePanel is hidded when editing a TouchAction that has a dwgAction<br>
   or when editing a TouchActionInput, this disables adding other dwg items
*/
bool ControlPanel::getShowChoicePanelFlag() {
  if (Items::getControlSelectedItem()->type == pfodTOUCH_ACTION_INPUT) {
    return false;
  }
  if ((Items::getControlSelectedItem()->type == pfodTOUCH_ACTION) &&
      (Items::getControlDisplayedItem()->type != pfodHIDE) ) {
    return false;
  }
  // else
  return true;
}

/**
   showTouchZoneOnChoicePanel
   returns true is should show TouchZone option on ChoicePanel, else false

   If editing a touchZone touchAction or touchActionInput then disable adding touchZone
*/
bool ControlPanel::showTouchZoneOnChoicePanel() {
  if (Items::getControlSelectedItem()->type == pfodTOUCH_ACTION_INPUT) {
    return false;
  }
  if (Items::getControlSelectedItem()->type == pfodTOUCH_ACTION) {
    return false;
  }
  return true;
}

// flag cleared on each call
bool ControlPanel::getShowBackgroundColorPanelFlag() {
  bool rtn = showBackgroundColorPanel;
  showBackgroundColorPanel = false;
  return rtn;
}

void ControlPanel::setShowColorPanelFlag(bool flag) {
  showColorPanel = flag;
}

ControlPanel::ControlPanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd) {
  dwgsPtr = _dwgsPtr;
  parserPtr = _parserPtr;
  loadCmd = _loadCmd;
  dwgSize = 50;
  disablePanel = false;
}

// return true if handled else false
bool ControlPanel::processDwgCmds() {
  cSFA(sfErrMsg, errMsg); // wrap error msg char[]
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  cSF(sfNumber, 20); // for number inputs otherwise ignored
  sfNumber.readFrom((const char*)parserPtr->getEditedText()); // may be empty, read as much as we can into sfNumber truncating the rest.

  if (parserPtr->dwgCmdEquals(TiPCmd)) { // handle input prompt text
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    cSFA(sfText, valuesClassPtr->text); //
    sfText.clear();
    sfText.readFrom((char*)parserPtr->getEditedText()); // read as much as we can
    if (sfText.isFull()) {
      // replace last with ...
      sfText.removeLast(3);
      sfText += "...";
    }
    // save item
    Items::saveSelectedItem();
    reload();
    return true;

  } else if (parserPtr->dwgCmdEquals(CloseCmd)) { // handle action input close
    // clean up double referenced dwgItems
    // if the current action has ref != 0 and another action also has the same refNo
    // the zero the other actions referNo
    Items::closeAction(Items::getControlSelectedItem()); // for debugging only
    Items::cleanUpActionReferenceNos(Items::getControlSelectedItem()); //Items::getControlDisplayedItem() is the actionAction e.g. label
    Items::setControlSelectedItemFromItemIdx();
    return true;


  } else if (parserPtr->dwgCmdEquals(Ta1Cmd)) { // handle action 1
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      return false;
    }
    // else create action and add to this item
    if (Items::addTouchAction_1()) {
      reload();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(Ta2Cmd)) { // handle action 2
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      return false;
    }
    // else create action and add to this item
    if (Items::addTouchAction_2()) {
      reload();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(Ta3Cmd)) { // handle action 3
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      return false;
    }
    // else create action and add to this item
    if (Items::addTouchAction_3()) {
      reload();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(Ta4Cmd)) { // handle action 4
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      return false;
    }
    // else create action and add to this item
    if (Items::addTouchAction_4()) {
      reload();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(Ta5Cmd)) { // handle action 5
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      return false;
    }
    // else create action and add to this item
    if (Items::addTouchAction_5()) {
      reload();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(TiCmd)) { // handle action input
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      return false;
    }
    // else create Input and add to this item
    if (Items::addTouchActionInput()) {
      reload();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(textEditCmd)) { // handle edit text
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    cSFA(sfText, valuesClassPtr->text); //
    sfText.clear();
    sfText.readFrom((char*)parserPtr->getEditedText()); // read as much as we can
    // save item
    Items::saveSelectedItem();
    return true;

  } else if (parserPtr->dwgCmdEquals(LuCmd)) { // handle edit units
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
    cSFA(sfText, valuesClassPtr->units); //
    sfText.clear();
    sfText.readFrom((char*)parserPtr->getEditedText()); // read as much as we can
    // save item
    Items::saveSelectedItem();
    return true;

  } else if (parserPtr->dwgCmdEquals(fontSizeCmd)) { // handle font size
    int fontSize = 0;
    if (!sfNumber.toInt(fontSize)) {
      sfErrMsg = "Invalid fontSize: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      // NEVER returns NULL!!
      Items::getControlDisplayedItem()->valuesPtr->fontSize = fontSize;
      Items::saveSelectedItem();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(IdxCmd)) {  // handle idx
    ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // NEVER returns NULL!!
    valuesClassPtr->isIndexed = (!valuesClassPtr->isIndexed);
    Items::saveSelectedItem(); // force full save
    return true;

  } else if (parserPtr->dwgCmdEquals(DCmd)) {  // handle delete item
    //sfNumber has edited text
    sfNumber.toUpperCase();
    if (sfNumber == 'Y') {
      // delete item
      //itemIdxUpdated = true;
      if (debugPtr) {
        debugPtr->print("before delete"); debugPtr->println();
        Items::debugListDwgValues();
      }
      bool isDeleteingTouchZoneOrAction = ((Items::getControlSelectedItemType() == pfodTOUCH_ZONE)
                                           || (Items::getControlSelectedItemType() == pfodTOUCH_ACTION)
                                           || (Items::getControlSelectedItemType() == pfodTOUCH_ACTION_INPUT));
      Items::deleteControlItem(); // handles action items
      if (isDeleteingTouchZoneOrAction) {
        Items::setControlSelectedItemFromItemIdx(); // close panel
        fullReload(); // after delete
      }
      if (debugPtr) {
        debugPtr->print("after delete"); debugPtr->println();
        Items::debugListDwgValues();
      }
      Items::saveSelectedItem(); // force full save is Y
    } //else ignore delete
    return true;

  } else if (parserPtr->dwgCmdEquals(colorCmd)) {  // handle color
    showColorPanel = true;
    return true;

  } else if (parserPtr->dwgCmdEquals(backgroundColorCmd)) {  // handle backgroundColor
    showColorPanel = true;
    showBackgroundColorPanel = true; // make result color update background of touchActionInput
    return true;

  } else if (parserPtr->dwgCmdEquals(xPosCmd)) {  // handle x pos
    // xPos not updated if number invalid, no need to trim() first
    float xPos = 0.0;
    if (!sfNumber.toFloat(xPos)) {
      sfErrMsg = "Invalid X Offset: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      xPos = limitFloat(xPos, -dwgSize, dwgSize);
      // NEVER returns NULL!!
      if (Items::setControlSelectedColOffset(xPos)) { // rounds to 2 decs
        fullReload();
      }
      Items::saveSelectedItem();
    }
    return true;
  } else if (parserPtr->dwgCmdEquals(yPosCmd)) {  // handle y pos
    float yPos = 0.0;
    if (!sfNumber.toFloat(yPos)) {     // no need to trim() first
      sfErrMsg = "Invalid Y Offset: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      yPos = limitFloat(yPos, -dwgSize, dwgSize);
      // NEVER returns NULL!!
      if (Items::setControlSelectedRowOffset(yPos)) { // round to 2 decs
        fullReload();
      }
      Items::saveSelectedItem();
    }
    return true;
  } else if (parserPtr->dwgCmdEquals(rCmd)) {  // handle radius
    float radius = 1.0;
    if (!sfNumber.toFloat(radius)) {     // no need to trim() first
      sfErrMsg = "Invalid radius: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      radius = limitFloat(radius, -dwgSize, dwgSize);
      // NEVER returns NULL!!
      Items::getControlDisplayedItem()->valuesPtr->radius = radius;
      Items::saveSelectedItem();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(SCmd)) {  // handle start
    float start = 0.0;
    if (!sfNumber.toFloat(start)) {     // no need to trim() first
      sfErrMsg = "Invalid Start angle: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      start = limitFloat(start, -360, 360);
      // NEVER returns NULL!!
      Items::getControlDisplayedItem()->valuesPtr->startAngle = start;
      Items::saveSelectedItem();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(ACmd)) {  // handle angle
    float angle = 180.0;
    if (!sfNumber.toFloat(angle)) {     // no need to trim() first
      sfErrMsg = "Invalid arc Angle: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      angle = limitFloat(angle, -360, 360);
      // NEVER returns NULL!!
      Items::getControlDisplayedItem()->valuesPtr->arcAngle = angle;
      Items::saveSelectedItem();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(wCmd)) {  // handle width
    float width = 1.0;
    if (!sfNumber.toFloat(width)) {     // no need to trim() first
      sfErrMsg = "Invalid width: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      width = limitFloat(width, -dwgSize, dwgSize);
      // NEVER returns NULL!!
      if (Items::setControlSelectedWidth(width)) {  // round to 2 decs
        fullReload();
      }
      Items::saveSelectedItem();
    }
    return true;
  } else if (parserPtr->dwgCmdEquals(hCmd)) {  // handle height
    float height = 1.0;
    if (!sfNumber.toFloat(height)) {     // no need to trim() first
      sfErrMsg = "Invalid height: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      height = limitFloat(height, -dwgSize, dwgSize);
      // NEVER returns NULL!!
      if (Items::setControlSelectedHeight(height)) {
        fullReload();
      }
      Items::saveSelectedItem();
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(LBCmd)) {  // handle bold
    // toggle   // NEVER returns NULL!!
    Items::getControlDisplayedItem()->valuesPtr->bold = !(Items::getControlDisplayedItem()->valuesPtr->bold);
    Items::saveSelectedItem();
    return true;

  } else if (parserPtr->dwgCmdEquals(LICmd)) {  // handle italic
    // toggle   // NEVER returns NULL!!
    Items::getControlDisplayedItem()->valuesPtr->italic = !(Items::getControlDisplayedItem()->valuesPtr->italic);
    Items::saveSelectedItem();
    return true;

  } else if (parserPtr->dwgCmdEquals(LUCmd)) {  // handle underline
    // toggle   // NEVER returns NULL!!
    Items::getControlDisplayedItem()->valuesPtr->underline = !(Items::getControlDisplayedItem()->valuesPtr->underline);
    Items::saveSelectedItem();
    return true;

  } else if (parserPtr->dwgCmdEquals(LACmd)) { // handle alignment
    if (Items::getControlDisplayedItem()->valuesPtr->align == 'R') {
      Items::getControlDisplayedItem()->valuesPtr->align = 'L';
    } else if (Items::getControlDisplayedItem()->valuesPtr->align == 'L') {
      Items::getControlDisplayedItem()->valuesPtr->align = 'C';
    } else { // if (Items::getControlDisplayedItem()->valuesPtr->align = 'C' { // default is C
      Items::getControlDisplayedItem()->valuesPtr->align = 'R';
    }
    Items::saveSelectedItem();
    return true;

  } else if (parserPtr->dwgCmdEquals(LVCmd)) { // handle label value
    cSF(sfReading, 80);
    sfReading.readFrom((char*)parserPtr->getEditedText());
    sfReading.trim();
    if (sfReading.isEmpty()) {
      Items::getControlDisplayedItem()->valuesPtr->haveReading = 0;
    } else {
      float reading = 0.0;
      if (!sfNumber.toFloat(reading)) {     // no need to trim() first
        sfErrMsg = "Invalid value: '";
        sfErrMsg += sfNumber;
        sfErrMsg += "'";
      } else {
        if (isnan(reading)) {
          sfErrMsg = "Invalid value NAN : '";
          sfErrMsg += sfNumber;
          sfErrMsg += "'";
        } else if (isinf(reading)) {
          sfErrMsg = "Invalid value INF : '";
          sfErrMsg += sfNumber;
          sfErrMsg += "'";
        } else {
          cSF(sfNum, 80);
          sfNum.print(reading);
          if (sfNum.indexOf("ovf") != -1) {
            sfErrMsg = "Invalid value overflow : '";
            sfErrMsg += sfNumber;
            sfErrMsg += "'";
          } else {
            Items::getControlDisplayedItem()->valuesPtr->haveReading = 1;
            Items::getControlDisplayedItem()->valuesPtr->reading = reading;
            Items::saveSelectedItem();
          }
        }
      }
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(LDCmd)) { // handle label decimals
    int decimals = 0;
    if (!sfNumber.toInt(decimals)) {     // no need to trim() first
      sfErrMsg = "Invalid decimals: '";
      sfErrMsg += sfNumber;
      sfErrMsg += "'";
    } else {
      if ((decimals < -6) || (decimals > 6)) {
        sfErrMsg = "Decimals not -6 to 6 : '";
        sfErrMsg += sfNumber;
        sfErrMsg += "'";
      } else {
        Items::getControlDisplayedItem()->valuesPtr->decPlaces = decimals;
        Items::saveSelectedItem();
      }
    }
    return true;

  } else if (parserPtr->dwgCmdEquals(FCmd)) {  // handle fill
    // toggle
    // NEVER returns NULL!!
    Items::getControlDisplayedItem()->valuesPtr->filled = !(Items::getControlDisplayedItem()->valuesPtr->filled);
    Items::saveSelectedItem();
    return true;
  } else if (parserPtr->dwgCmdEquals(CCmd)) {  // handle center
    // toggle
    // NEVER returns NULL!!
    if (Items::setControlSelectedCentered(!(Items::getControlDisplayedItem()->valuesPtr->centered)) ) {
      fullReload();
    }
    Items::saveSelectedItem();
    return true;
  } else if (parserPtr->dwgCmdEquals(RCmd)) {  // handle Rounded
    // toggle
    // NEVER returns NULL!!
    Items::getControlDisplayedItem()->valuesPtr->rounded = !(Items::getControlDisplayedItem()->valuesPtr->rounded);
    Items::saveSelectedItem();
    return true;

  } // else
  return false;
}

void ControlPanel::setErrMsg(char *msg) {
  cSFA(sfErrMsg, errMsg);
  sfErrMsg.clear();
  sfErrMsg.readFrom(msg); // take the first 50 char only
}

bool ControlPanel::processDwgLoad() {
  if (!parserPtr->cmdEquals(loadCmd)) { // pfodApp is asking to load dwg "c"
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

void ControlPanel::disable(bool flag) {
  disablePanel = flag;
}


// assumes dwg is 100 wide
void ControlPanel::sendDrawing(long dwgIdx) {
  pfodDwgTypeEnum type = Items::getControlDisplayedItem()->type; // pfodNONE;  returns getControlSelectedItem unless getControlSelectedItem is touchAction() in which case returns the action
  // if selectedItemType is an Action then type is the actual action, e.g.  pfodHIDE or other
  pfodDwgTypeEnum selectedItemType = Items::getControlSelectedItemType(); // this == type unless selected item is an action
  // NOTE for touchActionInput  type == selectedItemType.
  // NOTE type == pfodHIDE only when selectedItemType == pfodTOUCH_ACTION

  if (debugPtr) {
    debugPtr->print("Items::getControlDisplayedItem()->type : "); debugPtr->println(ValuesClass::typeToStr(type));
    debugPtr->print("Items::getControlSelectedItemType() : "); debugPtr->println(ValuesClass::typeToStr(selectedItemType));
  }
  if (dwgIdx == 0) {
    dwgsPtr->start(100, 62, dwgsPtr->WHITE, true); //true means more to come,  background defaults to WHITE if omitted i.e. dwgsPtr->start(50,30);
    parserPtr->sendRefreshAndVersion(0); //need to set a version number for image refresh to work!! send the parser version to cache this image, refreshes every 10sec
    dwgsPtr->touchZone().cmd(disabledCmd).idx(dwgDisableControlsIdx).filter(dwgsPtr->TOUCH_DISABLED).size(100, 40).send();
    dwgsPtr->hide().cmd(disabledCmd).send();

    if (type == pfodTOUCH_ACTION_INPUT) {
      dwgsPtr->pushZero(2, 4); // pos x, y
      dwgsPtr->label().fontSize(6).offset(0, 0).left().text("Input Prompt").bold().send();
      dwgsPtr->index().idx(TiPromptLabel_idx).send();
      dwgsPtr->hide().idx(TiPromptLabel_idx).send(); // hiddend label for full prompt
      dwgsPtr->index().idx(TiPromptShortLabel_idx).send();
      dwgsPtr->rectangle().offset(0, 4).size(57, 8).send();
      dwgsPtr->touchZone().cmd(TiPCmd).offset(0, 4).size(57, 12).send(); // cover label as well
      cSF(sfPrompt, 50);
      sfPrompt = "Prompt for touchActionInput";
      dwgsPtr->touchActionInput().cmd(TiPCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt(sfPrompt.c_str()).textIdx(TiPromptLabel_idx).send();
      dwgsPtr->popZero();

    } else {
      // place X Pos, Y Pos
      if (type != pfodHIDE) {
        dwgsPtr->pushZero(10, 4); // pos x, y
        dwgsPtr->label().fontSize(6).text("X").bold().send();
        dwgsPtr->index().idx(xLabel_idx).send(); //dwgsPtr->label().idx(xLabel_idx).fontSize(6).offset(0, 8).floatReading(xPos).decimals(2).send();
        dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();

        if ((type != pfodNONE)) { // && (selectedItemType != pfodTOUCH_ACTION)) {
          dwgsPtr->touchZone().cmd(xPosCmd).offset(0, 6).centered().size(20, 12).send(); // cover label as well
          cSF(sfPrompt, 50);
          sfPrompt = "Enter X offset\n<-2><i>";
          sfPrompt.print((float) - dwgSize, 2);
          sfPrompt += " to +";
          sfPrompt.print((float)dwgSize, 2);
          dwgsPtr->touchActionInput().cmd(xPosCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt(sfPrompt.c_str()).textIdx(xLabel_idx).send();
        }

        dwgsPtr->label().fontSize(4).offset(10, 0).text("offset").bold().send();

        dwgsPtr->label().fontSize(6).offset(20, 0).text("Y").bold().send();
        dwgsPtr->index().idx(yLabel_idx).send(); //dwgsPtr->label().idx(yLabel_idx).fontSize(6).offset(20, 8).floatReading(yPos).decimals(2).send();
        dwgsPtr->rectangle().offset(20, 8).centered().size(18, 8).send();

        if ((type != pfodNONE)) {// && (Items::getControlSelectedItemType() != pfodTOUCH_ACTION)) {
          dwgsPtr->touchZone().cmd(yPosCmd).offset(20, 6).centered().size(20, 12).send(); // cover label as well
          cSF(sfPrompt, 50);
          sfPrompt = "Enter Y offset\n<-2><i>";
          sfPrompt.print((float) - dwgSize, 2);
          sfPrompt += " to +";
          sfPrompt.print((float)dwgSize, 2);
          dwgsPtr->touchActionInput().cmd(yPosCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt(sfPrompt.c_str()).textIdx(yLabel_idx).send();
        }
        dwgsPtr->popZero();
      }
    }
    dwgsPtr->pushZero(50, 4); // pos w,h
    //define all the idx for this pushZero
    dwgsPtr->index().idx(wLabel_idx).send();//    dwgsPtr->label().idx(wLabel_idx).fontSize(6).offset(0, 8).floatReading(width).decimals(2).send();
    dwgsPtr->index().idx(hLabel_idx).send(); //dwgsPtr->label().idx(hLabel_idx).fontSize(6).offset(20, 8).floatReading(height).decimals(2).send();
    dwgsPtr->index().idx(rLabel_idx).send();//    dwgsPtr->label().idx(rLabel_idx).fontSize(6).offset(0, 8).floatReading(radius).decimals(2).send();
    dwgsPtr->index().idx(fsLabel_idx).send(); // fontsize
    dwgsPtr->index().idx(txLabel_idx).send(); // idx for current label text
    dwgsPtr->hide().idx(txLabel_idx).send(); // hide
    dwgsPtr->index().idx(LuLabel_idx).send(); // idx for current units text
    dwgsPtr->hide().idx(LuLabel_idx).send(); // hide

    if ((type == pfodRECTANGLE) || (type == pfodLINE) || (type == pfodTOUCH_ZONE)) {
      // place width height
      dwgsPtr->label().fontSize(6).text("Width").bold().send();
      dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(wCmd).offset(0, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(wCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Width +/-\n<-2><i>e.g. 2.65").textIdx(wLabel_idx).send();

      dwgsPtr->label().fontSize(6).offset(20, 0).text("Height").bold().send();
      dwgsPtr->rectangle().offset(20, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(hCmd).offset(20, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(hCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Height +/-\n<-2><i>e.g. 2.65").textIdx(hLabel_idx).send();
    } else  if ((type == pfodARC) || (type == pfodCIRCLE)) {
      // place radius
      dwgsPtr->label().fontSize(6).text("Radius").bold().send();
      dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(rCmd).offset(0, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(rCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Radius +/-\n<-2><i>e.g. 2.65").textIdx(rLabel_idx).send();
    } else  if (type == pfodLABEL) {
      // place Text
      dwgsPtr->label().fontSize(6).offset(0, 8).text("text").bold().send();
      dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(textEditCmd).offset(0, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(textEditCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Label text\n(upto 63 chars").textIdx(txLabel_idx).send();

      dwgsPtr->label().fontSize(6).offset(20, 0).text("fontsize").bold().send();
      dwgsPtr->rectangle().offset(20, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(fontSizeCmd).offset(20, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(fontSizeCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter +/- Font Size \n<-2><i>e.g.-1\nIncrease by 6 to double font size\nDecrease by 6 to half size").textIdx(fsLabel_idx).send();

    } else if (type == pfodTOUCH_ACTION_INPUT) {
      dwgsPtr->label().fontSize(6).offset(20, 0).text("fontSize").bold().send();
      dwgsPtr->rectangle().offset(20, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(fontSizeCmd).offset(20, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(fontSizeCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter +/- Font Size \n<-2><i>e.g.-1\nIncrease by 6 to double font size\nDecrease by 6 to half size").textIdx(fsLabel_idx).send();
    }
    dwgsPtr->popZero();

    // place color height
    if ((type == pfodTOUCH_ZONE) || (type == pfodHIDE)) {
      // skip color
    } else {
      dwgsPtr->pushZero(90, 4); // color
      dwgsPtr->label().fontSize(6).idx(colLabel_idx).text("color").bold().send();
      dwgsPtr->index().idx(colLabelRect_idx).send();
      dwgsPtr->rectangle().idx(colLabelRect_idx).filled().color(Items::getControlDisplayedItem()->valuesPtr->color).offset(0, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(colorCmd).offset(0, 6).centered().size(18, 8).send(); // cover label as well
      dwgsPtr->touchAction().cmd(colorCmd).action(
        dwgsPtr->hide().idx(colLabelRect_idx)
      ).send();
      dwgsPtr->touchAction().cmd(colorCmd).action(
        dwgsPtr->hide().idx(colLabel_idx)
      ).send();
      dwgsPtr->touchAction().cmd(colorCmd).action(
        dwgsPtr->unhide().cmd(disabledCmd)
      ).send();
      dwgsPtr->rectangle().idx(colLabelRectOutline_idx).offset(0, 8).centered().size(18, 8).send(); // always
      // since colLabelRectOutline_idx is AFTER colLabelRect_idx it will be assigned a higher idx and so will be ON TOP
      dwgsPtr->popZero();
    }
    dwgsPtr->end();

  } else if (dwgIdx == 1) {
    dwgsPtr->startUpdate(true);
    // always add Y/N label for delete and hide it
    dwgsPtr->label().idx(DLabelY_N_idx).text("N").send();
    dwgsPtr->hide().idx(DLabelY_N_idx).send();

    // delete button
    dwgsPtr->pushZero(90, 47);
    dwgsPtr->rectangle().filled().rounded().centered().color(dwgsPtr->RED).offset(0, 8).size(18, 8).send();
    dwgsPtr->label().idx(DLabel_idx).fontSize(6).color(dwgsPtr->WHITE).offset(0, 8).text("Delete").bold().send();
    dwgsPtr->touchZone().cmd(DCmd).offset(0, 8).centered().size(18, 8).send();
    dwgsPtr->touchActionInput().cmd(DCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Delete current item\n<-2> <b>y / N").textIdx(DLabelY_N_idx).send();
    dwgsPtr->touchAction().cmd(DCmd).action(
      dwgsPtr->rectangle().idx(DLabel_idx).filled().centered().color(dwgsPtr->BLACK).offset(0, 8).size(18, 8)
    ).send();
    dwgsPtr->touchAction().cmd(DCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();

    // place Delete, idx
    if (type != pfodNONE) {
      if ( (type == pfodHIDE) ||  (selectedItemType == pfodTOUCH_ACTION) || (type == pfodTOUCH_ACTION_INPUT)) {
        // do not add
      } else {
        dwgsPtr->pushZero(90, 28); // idx
        dwgsPtr->index().idx(IdxbgLabel_idx).send();
        dwgsPtr->label().idx(IdxLabel_idx).fontSize(6).text("update").bold().send();
        dwgsPtr->touchZone().cmd(IdxCmd).centered().size(12, 6).send();
        dwgsPtr->popZero();
      }
      if (type == pfodTOUCH_ZONE) {
        dwgsPtr->pushZero(15, 27);
        dwgsPtr->index().idx(CbgLabel_idx).send(); // centered background
        dwgsPtr->label().idx(CLabel_idx).fontSize(6).bold().text("centered").send(); // does not change
        dwgsPtr->touchZone().cmd(CCmd).offset(0, 0).centered().size(20, 1).send();
        dwgsPtr->popZero();

      } else if ((type == pfodRECTANGLE) || (type == pfodARC) || (type == pfodCIRCLE)) {
        dwgsPtr->pushZero(10, 27);
        dwgsPtr->index().idx(FbgLabel_idx).send(); // filled background
        dwgsPtr->label().idx(FLabel_idx).fontSize(6).bold().text("filled").send(); // does not change
        dwgsPtr->touchZone().cmd(FCmd).offset(0, 0).centered().size(20, 1).send();
        dwgsPtr->popZero();
        if ((type == pfodRECTANGLE)) {
          dwgsPtr->pushZero(34, 27);
          dwgsPtr->index().idx(CbgLabel_idx).send(); // centered background
          dwgsPtr->label().idx(CLabel_idx).fontSize(6).bold().text("centered").send(); // does not change
          dwgsPtr->touchZone().cmd(CCmd).offset(0, 0).centered().size(20, 1).send();
          dwgsPtr->popZero();
          dwgsPtr->pushZero(62, 27);
          dwgsPtr->index().idx(RbgLabel_idx).send(); // rounded background
          dwgsPtr->label().idx(RLabel_idx).fontSize(6).bold().text("rounded").send(); // does not change
          dwgsPtr->touchZone().cmd(RCmd).offset(0, 0).centered().size(20, 1).send();
          dwgsPtr->popZero();
        } else if (type == pfodARC) {
          dwgsPtr->pushZero(30, 27);
          dwgsPtr->label().fontSize(6).text("start").bold().send();
          dwgsPtr->index().idx(SLabel_idx).send(); //dwgsPtr->label().idx(SLabel_idx).fontSize(6).offset(20, 8).floatReading(startAngle).decimals(1).send();
          dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();
          dwgsPtr->touchZone().cmd(SCmd).offset(0, 6).centered().size(20, 12).send();
          dwgsPtr->touchActionInput().cmd(SCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Start angle\n<-2>-360.0 to +360.0").textIdx(SLabel_idx).send();
          dwgsPtr->popZero();
          dwgsPtr->pushZero(50, 27);
          dwgsPtr->label().fontSize(6).text("angle").bold().send();
          dwgsPtr->index().idx(ALabel_idx).send(); //dwgsPtr->label().idx(ALabel_idx).fontSize(6).offset(0, 8).floatReading(arcAngle).decimals(1).send();
          dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();
          dwgsPtr->touchZone().cmd(ACmd).offset(0, 6).centered().size(20, 12).send();
          dwgsPtr->touchActionInput().cmd(ACmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter arc Angle\n<-2>-360.0 to +360.0").textIdx(ALabel_idx).send();
          dwgsPtr->popZero();
        }
      } else if ((type == pfodLABEL) || (type == pfodTOUCH_ACTION_INPUT)) {
        // PLACE bold, italic underline left/center/right
        dwgsPtr->pushZero(10, 28);
        dwgsPtr->index().idx(LBbgLabel_idx).send(); // Bold background
        dwgsPtr->label().idx(LBLabel_idx).fontSize(6).bold().text("B").send(); // does not change
        dwgsPtr->touchZone().cmd(LBCmd).send();
        dwgsPtr->popZero();

        dwgsPtr->pushZero(24, 28);
        dwgsPtr->index().idx(LIbgLabel_idx).send(); // Italic background
        dwgsPtr->label().idx(LILabel_idx).fontSize(6).bold().italic().text("I").send(); // does not change
        dwgsPtr->touchZone().cmd(LICmd).send();
        dwgsPtr->popZero();

        dwgsPtr->pushZero(38, 28);
        dwgsPtr->index().idx(LUbgLabel_idx).send(); // Underline background
        dwgsPtr->label().idx(LULabel_idx).fontSize(6).bold().underline().text("U").send(); // does not change
        dwgsPtr->touchZone().cmd(LUCmd).send();
        dwgsPtr->popZero();

        if (type == pfodTOUCH_ACTION_INPUT) {
          dwgsPtr->pushZero(90, 28); //   //       dwgsPtr->pushZero(90, 4);  so +45
          dwgsPtr->label().offset(-10, 0).idx(backgroundColLabel_idx).fontSize(6).text("backgdColor").bold().right().send();
          dwgsPtr->index().idx(backgroundColLabelRect_idx).send();
          dwgsPtr->rectangle().offset(0, 0).centered().idx(backgroundColLabelRect_idx).filled().color(Items::getControlDisplayedItem()->valuesPtr->backgroundColor).size(18, 8).send();
          dwgsPtr->rectangle().offset(0, 0).centered().idx(backgroundColLabelRectOutline_idx).size(18, 8).send(); // always
          // since backgroundColLabelRectOutline_idx is AFTER backgroundColLabelRect_idx it will be assigned a higher idx and so will be ON TOP
          dwgsPtr->touchZone().cmd(backgroundColorCmd).offset(-45, -4).size(45 + 18, 8).send(); // cover label as well
          dwgsPtr->touchAction().cmd(backgroundColorCmd).action(
            dwgsPtr->hide().idx(backgroundColLabelRect_idx)
          ).send();
          dwgsPtr->touchAction().cmd(backgroundColorCmd).action(
            dwgsPtr->hide().idx(backgroundColLabel_idx)
          ).send();
          dwgsPtr->touchAction().cmd(backgroundColorCmd).action(
            dwgsPtr->unhide().cmd(disabledCmd)
          ).send();
          dwgsPtr->popZero();
        }
        if (type == pfodLABEL) {
          dwgsPtr->pushZero(62, 28);
          // background shading
          dwgsPtr->index().idx(AbgLabel_idx).send();
          dwgsPtr->label().idx(ALLabel_idx).fontSize(6).bold().offset(-8, 0).text("L").send();
          dwgsPtr->label().idx(ACLabel_idx).fontSize(6).bold().text("C").send();
          dwgsPtr->label().idx(ARLabel_idx).fontSize(6).bold().offset(+8, 0).text("R").send();
          dwgsPtr->rectangle().idx(ARectLabel_idx).size(26, 10).centered().color(dwgsPtr->SILVER).send();
          dwgsPtr->touchZone().cmd(LACmd).send();
          dwgsPtr->popZero();
        }

      }
    }
    dwgsPtr->end();

  } else if (dwgIdx == 2) {
    dwgsPtr->startUpdate(true); // more to come

    dwgsPtr->pushZero(40, 37);
    dwgsPtr->index().idx(LDecLabel_idx).send();
    dwgsPtr->touchZone().cmd(LDCmd).offset(0, 6).centered().size(10, 12).send();
    dwgsPtr->hide().cmd(LDCmd).send();
    dwgsPtr->popZero();

    dwgsPtr->pushZero(40, 37);
    dwgsPtr->index().idx(X1_Label_idx).send();
    dwgsPtr->index().idx(X2_Label_idx).send();
    dwgsPtr->popZero();

    if (type == pfodTOUCH_ZONE) {
      dwgsPtr->pushZero(75, 37);
      dwgsPtr->index().idx(TibgLabel_idx).send(); // centered background
      dwgsPtr->label().idx(TiLabel_idx).fontSize(6).bold().text("input").send(); // does not change
      dwgsPtr->touchZone().cmd(TiCmd).offset(0, 0).centered().size(20, 1).send();
      dwgsPtr->popZero();

      dwgsPtr->pushZero(15, 37);
      dwgsPtr->index().idx(Tabg1Label_idx).send(); // centered background
      dwgsPtr->label().idx(Ta1Label_idx).fontSize(6).bold().text("action").send(); // does not change
      dwgsPtr->touchZone().cmd(Ta1Cmd).offset(0, 0).centered().size(20, 1).send();
      dwgsPtr->popZero();

      dwgsPtr->pushZero(45, 37);
      dwgsPtr->index().idx(Tabg2Label_idx).send(); // centered background
      dwgsPtr->label().idx(Ta2Label_idx).fontSize(6).bold().text("action").send(); // does not change
      dwgsPtr->touchZone().cmd(Ta2Cmd).offset(0, 0).centered().size(20, 1).send();
      dwgsPtr->popZero();
      
      dwgsPtr->pushZero(15, 46);
      dwgsPtr->index().idx(Tabg3Label_idx).send(); // centered background
      dwgsPtr->label().idx(Ta3Label_idx).fontSize(6).bold().text("action").send(); // does not change
      dwgsPtr->touchZone().cmd(Ta3Cmd).offset(0, 0).centered().size(20, 1).send();
      dwgsPtr->popZero();

      dwgsPtr->pushZero(45, 46);
      dwgsPtr->index().idx(Tabg4Label_idx).send(); // centered background
      dwgsPtr->label().idx(Ta4Label_idx).fontSize(6).bold().text("action").send(); // does not change
      dwgsPtr->touchZone().cmd(Ta4Cmd).offset(0, 0).centered().size(20, 1).send();
      dwgsPtr->popZero();

      dwgsPtr->pushZero(75, 46);
      dwgsPtr->index().idx(Tabg5Label_idx).send(); // centered background
      dwgsPtr->label().idx(Ta5Label_idx).fontSize(6).bold().text("action").send(); // does not change
      dwgsPtr->touchZone().cmd(Ta5Cmd).offset(0, 0).centered().size(20, 1).send();
      dwgsPtr->popZero();


    } else if (type == pfodTOUCH_ACTION_INPUT) { // || (Items::getControlSelectedItemType() == pfodTOUCH_ACTION)) {

      if (type == pfodTOUCH_ACTION_INPUT) {
        dwgsPtr->pushZero(37, 36); // default txt: only if label
        dwgsPtr->label().fontSize(6).text("default input: ").bold().right().italic().send();
        dwgsPtr->label().idx(shortDefaultInputLabel_idx).send();
        // dwgsPtr->label().idx(shortDefaultInputLabel_idx).fontSize(6).text("Label").left().send();
        dwgsPtr->popZero();
      }

    } else if (type == pfodLABEL) {
      // third row
      dwgsPtr->pushZero(15, 37);
      dwgsPtr->index().idx(fullLVLabel_idx).send();
      dwgsPtr->hide().idx(fullLVLabel_idx).send(); // store full value here
      dwgsPtr->label().fontSize(6).text("value").bold().send();
      dwgsPtr->index().idx(LVLabel_idx).send();
      dwgsPtr->rectangle().offset(0, 8).centered().size(28, 8).send();
      dwgsPtr->touchZone().cmd(LVCmd).offset(0, 6).centered().size(20, 12).send();
      dwgsPtr->touchActionInput().cmd(LVCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Value or nothing\n<-2>If empty just the label text displayed").textIdx(fullLVLabel_idx).send();
      dwgsPtr->popZero();


      dwgsPtr->pushZero(40, 37);
      dwgsPtr->label().fontSize(6).text("decimals").bold().send();
      dwgsPtr->index().idx(LDecLabel_idx).send();
      dwgsPtr->rectangle().offset(0, 8).centered().size(10, 8).send();
      dwgsPtr->touchZone().cmd(LDCmd).offset(0, 6).centered().size(10, 12).send();
      dwgsPtr->touchActionInput().cmd(LDCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter number of decimals\n<-2>-6 to 6").textIdx(LDecLabel_idx).send();
      dwgsPtr->popZero();

      dwgsPtr->pushZero(60, 37);
      dwgsPtr->label().fontSize(6).offset(0, 8).text("units").bold().send();
      dwgsPtr->rectangle().offset(0, 8).centered().size(18, 8).send();
      dwgsPtr->touchZone().cmd(LuCmd).offset(0, 6).centered().size(20, 12).send(); // cover label as well
      dwgsPtr->touchActionInput().cmd(LuCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Enter Units text\n(upto 63 chars").textIdx(LuLabel_idx).send();
      dwgsPtr->popZero();

      //    add cross outs
      dwgsPtr->pushZero(40, 37);
      dwgsPtr->line().idx(X1_Label_idx).offset(-8, 0).size(40, 12).send();
      dwgsPtr->line().idx(X2_Label_idx).offset(-8, 12).size(40, -12).send();
      dwgsPtr->popZero();
    }

    // add Close if touchActionInput, touchAction, or actionAction
    // add Close in green for both touchActionInput and touchAction
    bool addClose = false;
    if ((type == pfodTOUCH_ACTION_INPUT) || (Items::getControlSelectedItemType() == pfodTOUCH_ACTION)) {
      dwgsPtr->pushZero(12, 47); // pos x, y // touchActionLabel
      addClose = true;
      //      dwgsPtr->pushZero(12, 37); // pos x, y
      //      addClose = true;
      //    } else if (Items::getControlSelectedItemType() == pfodTOUCH_ACTION) {
      //      // add
      //      if (Items::getControlDisplayedItem()->type != pfodLABEL) {
      //        dwgsPtr->pushZero(12, 29); // pos x, y
      //        addClose = true;
      //      } else {
      //        dwgsPtr->pushZero(12, 47); // pos x, y // touchActionLabel
      //        addClose = true;
      //      }
    }
    if (addClose) { // add and pop
      dwgsPtr->rectangle().filled().rounded().centered().color(dwgsPtr->GREEN).offset(0, 8).size(18, 8).send();
      dwgsPtr->label().idx(CloseLabel_idx).fontSize(6).color(dwgsPtr->WHITE).offset(0, 8).text("Close").bold().send();
      dwgsPtr->touchZone().cmd(CloseCmd).offset(0, 8).centered().size(18, 8).send();
      dwgsPtr->touchAction().cmd(CloseCmd).action(
        dwgsPtr->rectangle().idx(CloseLabel_idx).filled().centered().color(dwgsPtr->GREEN).offset(0, 8).size(18, 8)
      ).send();
      dwgsPtr->touchAction().cmd(CloseCmd).action(
        dwgsPtr->unhide().cmd(disabledCmd)
      ).send();
      dwgsPtr->popZero();
    }
    // error msg label
    dwgsPtr->end();

  } else if (dwgIdx == 3) {
    dwgsPtr->startUpdate(false); // no more to come
    dwgsPtr->index().idx(errMsg_idx).send(); //dwgsPtr->label().idx(errMsg_idx).fontSize(6).offset(10, 20).bold().italic().color(dwgsPtr->RED).text(errMsg).send();
    update(); // update with current state
    dwgsPtr->end();

  } else {
    parserPtr->print(F("{}")); // always return somthing
  }

}

void ControlPanel::sendDrawingUpdates() {
  dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
  update();
  dwgsPtr->end();
}


void ControlPanel::update() {
  // control updates
   pfodDwgTypeEnum type = Items::getControlDisplayedItem()->type; // pfodNONE;  returns getControlSelectedItem unless getControlSelectedItem is touchAction() in which case returns the action
  // if selectedItemType is an Action then type is the actual action, e.g.  pfodHIDE or other
  pfodDwgTypeEnum selectedItemType = Items::getControlSelectedItemType(); // this == type unless selected item is an action
  // NOTE for touchActionInput  type == selectedItemType.
  // NOTE type == pfodHIDE only when selectedItemType == pfodTOUCH_ACTION

  ValuesClass *valuesClassPtr = Items::getControlDisplayedItem(); // never null
  Items::printDwgValues("  update controlDisplayedItem", valuesClassPtr);
  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr;  // never null
//  pfodDwgTypeEnum type = valuesClassPtr->type; // pfodNONE; if no items

  if (type != pfodNONE) {
    if ((type == pfodTOUCH_ZONE) || (type == pfodHIDE))  {
      // do nothing
    } else {
      dwgsPtr->rectangle().idx(colLabelRect_idx).filled().color(valuesPtr->color).offset(0, 8).centered().size(18, 8).send();
    }
    if (type == pfodTOUCH_ACTION_INPUT) {
      dwgsPtr->rectangle().offset(0, 0).centered().idx(backgroundColLabelRect_idx).filled().color(Items::getControlDisplayedItem()->valuesPtr->backgroundColor).size(18, 8).send();
    }
    if ((type == pfodHIDE) || (type == pfodTOUCH_ACTION_INPUT)) {
      // do nothing
    } else {
      dwgsPtr->label().idx(xLabel_idx).fontSize(6).offset(0, 8).floatReading(valuesPtr->colOffset).decimals(2).send();
      dwgsPtr->label().idx(yLabel_idx).fontSize(6).offset(20, 8).floatReading(valuesPtr->rowOffset).decimals(2).send();
    }

    if ((type == pfodHIDE) || (selectedItemType == pfodTOUCH_ACTION) || (type == pfodTOUCH_ACTION_INPUT))  {
      // do nothing
    } else {
      dwgsPtr->rectangle().idx(IdxbgLabel_idx).filled().centered().size(19, 8).color(valuesClassPtr->isIndexed ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(IdxCmd).centered().size(19, 6).send();
      dwgsPtr->touchAction().cmd(IdxCmd).action(
        dwgsPtr->rectangle().idx(IdxbgLabel_idx).filled().centered().size(19, 8).color(valuesClassPtr->isIndexed ? 15 : 87) // swap color
      ).send();
    }

    if ((type == pfodRECTANGLE) || (type == pfodLINE) || (type == pfodTOUCH_ZONE)) {
      dwgsPtr->label().idx(wLabel_idx).fontSize(6).offset(0, 8).floatReading(valuesPtr->width).decimals(2).send();
      dwgsPtr->label().idx(hLabel_idx).fontSize(6).offset(20, 8).floatReading(valuesPtr->height).decimals(2).send();
    } else if ((type == pfodARC) || (type == pfodCIRCLE)) {
      dwgsPtr->label().idx(rLabel_idx).fontSize(6).offset(0, 8).floatReading(valuesPtr->radius).decimals(2).send();
    } else if ((type == pfodLABEL) || (type == pfodTOUCH_ACTION_INPUT)) {
      dwgsPtr->label().idx(txLabel_idx).fontSize(6).text(valuesPtr->text).send();
      dwgsPtr->label().idx(fsLabel_idx).fontSize(6).offset(20, 8).floatReading(valuesPtr->fontSize).decimals(0).send();
      dwgsPtr->label().idx(LuLabel_idx).fontSize(6).text(valuesPtr->units).send();
    } else if (type == pfodTOUCH_ACTION_INPUT) {
      dwgsPtr->label().idx(fsLabel_idx).fontSize(6).offset(20, 8).floatReading(valuesPtr->fontSize).decimals(0).send();
    }

    if (type == pfodTOUCH_ZONE) {
      bool centered = valuesPtr->centered;
      dwgsPtr->rectangle().idx(CbgLabel_idx).filled().centered().size(25, 8).color(centered ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(CCmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(CCmd).action(
        dwgsPtr->rectangle().idx(CbgLabel_idx).filled().centered().size(25, 8).color(centered ? 15 : 87) // swap color
      ).send();

      bool has1Action = valuesClassPtr->hasAction1();
      dwgsPtr->rectangle().idx(Tabg1Label_idx).filled().centered().size(25, 8).color(has1Action ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(Ta1Cmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(Ta1Cmd).action(
        dwgsPtr->rectangle().idx(Tabg1Label_idx).filled().centered().size(25, 8).color(has1Action ? 15 : 87) // swap color
      ).send();
      bool has2Action = valuesClassPtr->hasAction2();
      dwgsPtr->rectangle().idx(Tabg2Label_idx).filled().centered().size(25, 8).color(has2Action ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(Ta2Cmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(Ta2Cmd).action(
        dwgsPtr->rectangle().idx(Tabg2Label_idx).filled().centered().size(25, 8).color(has2Action ? 15 : 87) // swap color
      ).send();

      bool has3Action = valuesClassPtr->hasAction3();
      dwgsPtr->rectangle().idx(Tabg3Label_idx).filled().centered().size(25, 8).color(has3Action ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(Ta3Cmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(Ta3Cmd).action(
        dwgsPtr->rectangle().idx(Tabg3Label_idx).filled().centered().size(25, 8).color(has3Action ? 15 : 87) // swap color
      ).send();

      bool has4Action = valuesClassPtr->hasAction4();
      dwgsPtr->rectangle().idx(Tabg4Label_idx).filled().centered().size(25, 8).color(has4Action ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(Ta4Cmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(Ta4Cmd).action(
        dwgsPtr->rectangle().idx(Tabg4Label_idx).filled().centered().size(25, 8).color(has4Action ? 15 : 87) // swap color
      ).send();

      bool has5Action = valuesClassPtr->hasAction5();
      dwgsPtr->rectangle().idx(Tabg5Label_idx).filled().centered().size(25, 8).color(has5Action ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(Ta5Cmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(Ta5Cmd).action(
        dwgsPtr->rectangle().idx(Tabg5Label_idx).filled().centered().size(25, 8).color(has5Action ? 15 : 87) // swap color
      ).send();

      bool hasInput = valuesClassPtr->hasActionInput();
      dwgsPtr->rectangle().idx(TibgLabel_idx).filled().centered().size(25, 8).color(hasInput ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(TiCmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(TiCmd).action(
        dwgsPtr->rectangle().idx(TibgLabel_idx).filled().centered().size(25, 8).color(hasInput ? 15 : 87) // swap color
      ).send();


    } else if ((type == pfodRECTANGLE) || (type == pfodARC) || (type == pfodCIRCLE)) {
      bool filled = valuesPtr->filled;
      dwgsPtr->rectangle().idx(FbgLabel_idx).filled().centered().size(16, 8).color(filled ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(FCmd).centered().size(20, 6).send();
      dwgsPtr->touchAction().cmd(FCmd).action(
        dwgsPtr->rectangle().idx(FbgLabel_idx).filled().centered().size(16, 8).color(filled ? 15 : 87) // swap color
      ).send();

      if ((type == pfodRECTANGLE)) {
        bool centered = valuesPtr->centered;
        dwgsPtr->rectangle().idx(CbgLabel_idx).filled().centered().size(25, 8).color(centered ? 87 : 15).send();
        dwgsPtr->touchZone().cmd(CCmd).centered().size(20, 6).send();
        dwgsPtr->touchAction().cmd(CCmd).action(
          dwgsPtr->rectangle().idx(CbgLabel_idx).filled().centered().size(25, 8).color(centered ? 15 : 87) // swap color
        ).send();

        bool rounded = valuesPtr->rounded;
        dwgsPtr->rectangle().idx(RbgLabel_idx).filled().centered().size(24, 8).color(rounded ? 87 : 15).send();
        dwgsPtr->touchZone().cmd(RCmd).centered().size(20, 6).send();
        dwgsPtr->touchAction().cmd(RCmd).action(
          dwgsPtr->rectangle().idx(RbgLabel_idx).filled().centered().size(24, 8).color(rounded ? 15 : 87) // swap color
        ).send();

      } else if (type == pfodARC) {
        dwgsPtr->label().idx(SLabel_idx).fontSize(6).offset(0, 8).floatReading(valuesPtr->startAngle).decimals(1).send();
        dwgsPtr->label().idx(ALabel_idx).fontSize(6).offset(0, 8).floatReading(valuesPtr->arcAngle).decimals(1).send();
      }
    } else if ((type == pfodLABEL) || (type == pfodTOUCH_ACTION_INPUT)) {
      bool bold = valuesPtr->bold;
      dwgsPtr->rectangle().idx(LBbgLabel_idx).filled().centered().size(8, 8).color(bold ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(LBCmd).centered().size(4, 4).send();
      dwgsPtr->touchAction().cmd(LBCmd).action(
        dwgsPtr->rectangle().idx(LBbgLabel_idx).filled().centered().size(8, 8).color(bold ? 15 : 87) // swap color
      ).send();

      bool italic = valuesPtr->italic;
      dwgsPtr->rectangle().idx(LIbgLabel_idx).filled().centered().size(8, 8).color(italic ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(LICmd).centered().size(4, 4).send();
      dwgsPtr->touchAction().cmd(LICmd).action(
        dwgsPtr->rectangle().idx(LIbgLabel_idx).filled().centered().size(8, 8).color(italic ? 15 : 87) // swap color
      ).send();

      bool underline = valuesPtr->underline;
      dwgsPtr->rectangle().idx(LUbgLabel_idx).filled().centered().size(8, 8).color(underline ? 87 : 15).send();
      dwgsPtr->touchZone().cmd(LUCmd).centered().size(4, 4).send();
      dwgsPtr->touchAction().cmd(LUCmd).action(
        dwgsPtr->rectangle().idx(LUbgLabel_idx).filled().centered().size(8, 8).color(underline ? 15 : 87) // swap color
      ).send();

      if (type == pfodLABEL) { // not for touchActionInput
        char align = valuesPtr->align; // update action to show next selection
        if (align == 'L') {
          dwgsPtr->rectangle().idx(AbgLabel_idx).size(8, 8).color(87).offset(-8, 0).filled().centered().send();
          dwgsPtr->touchZone().cmd(LACmd).centered().size(26, 10).send();
          dwgsPtr->touchAction().cmd(LACmd).action(
            dwgsPtr->rectangle().idx(AbgLabel_idx).filled().centered().size(8, 8).color(87)
          ).send();
        } else if (align == 'R') {
          dwgsPtr->rectangle().idx(AbgLabel_idx).size(8, 8).color(87).offset(+8, 0).filled().centered().send();
          dwgsPtr->touchZone().cmd(LACmd).centered().size(26, 10).send();
          dwgsPtr->touchAction().cmd(LACmd).action(
            dwgsPtr->rectangle().idx(AbgLabel_idx).filled().centered().offset(-8, 0).size(8, 8).color(87)
          ).send();
        } else { // 'C'
          dwgsPtr->rectangle().idx(AbgLabel_idx).size(8, 8).color(87).filled().centered().send();
          dwgsPtr->touchZone().cmd(LACmd).centered().size(26, 10).send();
          dwgsPtr->touchAction().cmd(LACmd).action(
            dwgsPtr->rectangle().idx(AbgLabel_idx).filled().centered().offset(+8, 0).size(8, 8).color(87)
          ).send();
        }
      }
      if (type == pfodTOUCH_ACTION_INPUT) {
        {
          cSF(sfShort, 20);
          sfShort.readFrom(valuesPtr->text);
          if (sfShort.isFull()) {
            sfShort.removeLast(3);
            sfShort += "...";
          }
          dwgsPtr->label().idx(TiPromptShortLabel_idx).offset(1, 8).left().fontSize(6).text(sfShort.c_str()).send();
          dwgsPtr->label().idx(TiPromptLabel_idx).text(valuesPtr->text).send();
        }
        ValuesClass *dwgItem = Items::getDwgSelectedItem();
        Items::printDwgValues("dwgItem", dwgItem);
        Items::printDwgValues("getControlSelectedItem", valuesClassPtr);
        cSF(sfShort, 12);
        sfShort.clear(); // nothing to start with
        if ((dwgItem) && (dwgItem->type == pfodLABEL)) { // && (dwgItem->getIndexNo() == valuesClassPtr->getIndexNo())) {
          cSF(sfFullLabel, 255); // bigger than be returned from input
          Items::formatLabel(dwgItem, sfFullLabel);
          sfShort.readFrom(sfFullLabel.c_str());
          if (sfShort.isFull()) {
            sfShort.removeLast(3);
            sfShort += "...";
          }
        } else {
          // valuesClassPtr->setIndexNo(0); // not a label
        }
        dwgsPtr->label().idx(shortDefaultInputLabel_idx).fontSize(6).text(sfShort.c_str()).left().send();
      }
      if (type == pfodLABEL) {
        if (!valuesPtr->haveReading) { // update action to show next selection
          dwgsPtr->label().idx(LVLabel_idx).send(); // empty
          dwgsPtr->unhide().idx(X1_Label_idx).send();
          dwgsPtr->unhide().idx(X2_Label_idx).send();
          dwgsPtr->hide().cmd(LDCmd).send();
          dwgsPtr->hide().cmd(LUCmd).send();
          dwgsPtr->hide().idx(LDecLabel_idx).send();
        } else {
          // limit length to 8 digits
          cSF(sfText, 80);
          if (valuesPtr->reading > 99999999) {
            sfText.print(valuesPtr->reading, 0);
            sfText.remove(6); // to end
            sfText += "..";
          } else {
            sfText.clear();
            sfText.print(valuesPtr->reading, 2, 8);
          }
          dwgsPtr->label().idx(LVLabel_idx).fontSize(6).offset(0, 8).text(sfText.c_str()).send();
          dwgsPtr->label().idx(fullLVLabel_idx).floatReading(valuesPtr->reading).decimals(6).send();
          dwgsPtr->hide().idx(X1_Label_idx).send();
          dwgsPtr->hide().idx(X2_Label_idx).send();
          dwgsPtr->unhide().cmd(LDCmd).send();
          dwgsPtr->unhide().cmd(LUCmd).send();
          dwgsPtr->unhide().idx(LDecLabel_idx).send();
        }
        dwgsPtr->label().idx(LDecLabel_idx).fontSize(6).offset(0, 8).floatReading(valuesPtr->decPlaces).decimals(0).send();
      }
    }

  } else {
  }
  if ((type == pfodTOUCH_ZONE) || (type == pfodHIDE))  {
    // do nothing
  } else {
    dwgsPtr->rectangle().idx(colLabelRectOutline_idx).offset(0, 8).centered().size(18, 8).send(); // always
  }
  if (type == pfodTOUCH_ACTION_INPUT) {
    dwgsPtr->rectangle().offset(0, 0).centered().idx(backgroundColLabelRectOutline_idx).size(18, 8).send(); // always
  }

  if (errMsg[0] == '\0') { // no error add type
    cSF(sfErrMsg, 255);
    ValuesClass *valuesClassPtr = Items::getControlSelectedItem();
    sfErrMsg = Items::getItemTypeAsString(valuesClassPtr);
    if (valuesClassPtr->isCovered() && (type == pfodTOUCH_ZONE)) {
      sfErrMsg += " -- <b><r>Covered, not active";
    } else if (Items::getControlSelectedItemType() == pfodTOUCH_ACTION) {

      Items::printDwgValues("valuesClassPtr", valuesClassPtr);

      // add name of action( )
      sfErrMsg += " ";
      sfErrMsg += Items::getActionActionPtr(valuesClassPtr)->getTypeAsStr();
      if (Items::getActionActionPtr(valuesClassPtr)->type == pfodHIDE) {
        // trim off the ()
        sfErrMsg.removeLast(2);
        sfErrMsg += " ";
        // look up refered idx
        ValuesClass *referencedPtr = Items::getReferencedPtr(valuesClassPtr->getIndexNo());
        Items::printDwgValues("referencedPtr", referencedPtr);
        if ((referencedPtr) && (referencedPtr->type != pfodTOUCH_ZONE)) { // i.e. not NULL add name of item referencedd
          sfErrMsg += referencedPtr->getTypeAsStr();
          // update X Y offset also??
        } else {
          sfErrMsg.println("<i><r>nothing selected");
          sfErrMsg.println("<bk>Use up/down arrows to select item");
          sfErrMsg += "Add dwg item to replace hidden item";
        }
      }
    }
    dwgsPtr->label().idx(errMsg_idx).fontSize(6).offset(2, 20).bold().left().color(dwgsPtr->BLUE).text(sfErrMsg.c_str()).send();
  } else {   // error msg label
    // limit errMsg
    cSF(sfErrMsg, 35);
    sfErrMsg.readFrom(errMsg);
    if (sfErrMsg.isFull()) {
      sfErrMsg.removeLast(3);
      sfErrMsg += "...";
    }
    dwgsPtr->label().idx(errMsg_idx).fontSize(6).offset(2, 20).bold().left().italic().color(dwgsPtr->RED).text(sfErrMsg.c_str()).send();
  }
  errMsg[0] = '\0'; //clear(); // only display this once
  dwgsPtr->hide().cmd(disabledCmd).send();
  if (disablePanel) {
    dwgsPtr->unhide().cmd(disabledCmd).send();
  }
}
