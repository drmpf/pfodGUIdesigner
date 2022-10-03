/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <Arduino.h>
#include "ChoicePanel.h"
#include "ControlPanel.h"
#include "Items.h"
#include <SafeString.h>
#include "pfodGUIdisplay.h"
// note size of main panel 0,0 to 100,100 is hard code in controlPanel

const char ChoicePanel::rectCmd[3]  = "r";
const char ChoicePanel::lineCmd[3]  = "l";
const char ChoicePanel::circleCmd[3]  = "c";
const char ChoicePanel::arcCmd[3]  = "a";
const char ChoicePanel::labelCmd[3]  = "L";
const char ChoicePanel::touchZoneCmd[3]  = "tZ";
const char ChoicePanel::disabledCmd[5] = "n"; // disabled touch zone never sent
const char ChoicePanel::generateCodeCmd[3] = "cg";
const char ChoicePanel::saveAsCmd[3] = "sA";
const char ChoicePanel::openCmd[3] = "O";
const char ChoicePanel::newCmd[3] = "N";
const char ChoicePanel::deleteCmd[3] = "D"; // delete



static uint16_t dwgDisableControlsIdx = Items::dwgReservedIdxs + 2000; // actually this unique to color panel but make it the same as dwg panel

static Stream *debugPtr = NULL;
// for later use
void ChoicePanel::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}


float ChoicePanel::limitFloat(float in, float lowerLimit, float upperLimit) {
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

const char *ChoicePanel::getDwgLoadCmd() {
  return loadCmd;
}

pfodDwgTypeEnum ChoicePanel::getSelectedDwgItemType() {
  return selectedDwgItemType; // the item to display can be null
}

//void ChoicePanel::setSelectedDwgItemType(pfodDwgTypeEnum _type) {
//  selectedDwgItemType = _type;
//}

ChoicePanel::ChoicePanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd) {
  dwgsPtr = _dwgsPtr;
  parserPtr = _parserPtr;
  loadCmd = _loadCmd;
  selectedDwgItemType = pfodNONE;
  disablePanel = false;
  forceReload = true; // for testing force complete reload on every reboot
}


void ChoicePanel::reload() {
  forceReload = true;
}

bool ChoicePanel::processSaveAsCmd() {
  cSFA(sfErrMsg, errMsg); // wrap error msg char[]
  sfErrMsg.clear();
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  if (parserPtr->dwgCmdEquals(saveAsCmd)) {
    if (debugPtr) {
      debugPtr->print(" process saveAs  ");
    }
    // pickup new name
    cSF(sfNewName, 80);
    sfNewName.readFrom((const char*)parserPtr->getEditedText()); // may be empty, read as much as we can into sfNumber truncating the rest.
    sfNewName.trim();
    if (debugPtr) {
      debugPtr->println(sfNewName);
    }
    Items::trimDedupText(sfNewName); // remove any (xxx) from the end
    if (sfNewName.length() > DwgName::MAX_DWG_BASE_NAME_LEN) {
      sfErrMsg = "New dwg name longer than ";
      sfErrMsg += DwgName::MAX_DWG_BASE_NAME_LEN;
      sfErrMsg += " chars.";
      if (debugPtr) {
        debugPtr->println(sfErrMsg);
      }
      sendDrawingUpdates();
      return true;
    } // else
    if (!Items::cleanDwgName(sfNewName, sfErrMsg)) { // change spaces to _
      sfErrMsg -= "New dwg name ";
      sendDrawingUpdates();
      return true;
    }
    // else have cleaned up dwg name
    Items::saveDwgAs(sfNewName, sfErrMsg); // also updates currentName
    sendDrawingUpdates(); // display new name, other panels are unchanged
    fullReload();
    return true;
  }
  return false;
}

bool ChoicePanel::processGenerateCmd() {
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  if (parserPtr->dwgCmdEquals(generateCodeCmd)) {
    return true;
  }
  return false;
}

bool ChoicePanel::processOpenCmd() {
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  if (parserPtr->dwgCmdEquals(openCmd)) {
    return true;
  }
  return false;
}

bool ChoicePanel::processNewCmd() {
  cSFA(sfErrMsg, errMsg); // wrap error msg char[]
  sfErrMsg.clear();
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  if (parserPtr->dwgCmdEquals(newCmd)) {
    if (debugPtr) {
      debugPtr->print(" process New Dwg  ");
    }
    // pickup new name
    cSF(sfNewName, 80);
    sfNewName.readFrom((const char*)parserPtr->getEditedText()); // may be empty, read as much as we can into sfNumber truncating the rest.
    sfNewName.trim();
    if (debugPtr) {
      debugPtr->println(sfNewName);
    }
    Items::trimDedupText(sfNewName); // remove any (xxx) from the end
    if (sfNewName.length() > DwgName::MAX_DWG_BASE_NAME_LEN) {
      sfErrMsg = "New dwg name longer than ";
      sfErrMsg += DwgName::MAX_DWG_BASE_NAME_LEN;
      sfErrMsg += " chars.";
      if (debugPtr) {
        debugPtr->println(sfErrMsg);
      }
      return true;
    } // else
    if (!Items::cleanDwgName(sfNewName, sfErrMsg)) { // leave in space
      sfErrMsg -= "New dwg name ";
      return true;
    }
    // else have cleaned up dwg name
    Items::newDwg(sfNewName, sfErrMsg); // also updates currentName
    return true;
  }
  return false;
}

bool ChoicePanel::processDwgDelete() {
  cSFA(sfErrMsg, errMsg); // wrap error msg char[]
  sfErrMsg.clear();

  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  if (parserPtr->dwgCmdEquals(deleteCmd)) {
    cSF(sfText, 20);
    sfText.readFrom((const char*)parserPtr->getEditedText()); // may be empty, read as much as we can into sfNumber truncating the rest.

    sfText.toUpperCase();
    if (sfText == 'Y') {
      // delete item
      //itemIdxUpdated = true;
      if (debugPtr) {
        debugPtr->print("before delete"); debugPtr->println();
        Items::debugListDwgValues();
      }
      if (Items::showDwgDelete()) {
        return true;
      } else {
        sfErrMsg = "Need to delete all the dwg elements first";
        sendDrawingUpdates();
        return false;
      }
    } // else return false below
  }
  return false;
}

// return true if handled else false
bool ChoicePanel::processDwgCmds() {
  cSFA(sfErrMsg, errMsg); // wrap error msg char[]
  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
  if (!dwgCmd) {
    return false; // not handled
  }
  // else
  if (parserPtr->dwgCmdEquals(rectCmd)) {
    selectedDwgItemType = pfodRECTANGLE;
    return true;
  } else if (parserPtr->dwgCmdEquals(lineCmd)) {
    selectedDwgItemType = pfodLINE;
    return true;
  } else if (parserPtr->dwgCmdEquals(circleCmd)) {
    selectedDwgItemType = pfodCIRCLE;
    return true;
  } else if (parserPtr->dwgCmdEquals(arcCmd)) {
    selectedDwgItemType = pfodARC;
    return true;
  } else if (parserPtr->dwgCmdEquals(labelCmd)) {
    selectedDwgItemType = pfodLABEL;
    return true;
  } else if (parserPtr->dwgCmdEquals(touchZoneCmd)) {
    selectedDwgItemType = pfodTOUCH_ZONE;
    return true;
  } // else
  return false;
}

void ChoicePanel::setErrMsg(char *msg) {
  cSFA(sfErrMsg, errMsg);
  sfErrMsg.clear();
  sfErrMsg.readFrom(msg); // take the first 50 char only
}

void ChoicePanel::disable(bool flag) {
  disablePanel = flag;
}

bool ChoicePanel::processDwgLoad() {
  if (!parserPtr->cmdEquals(loadCmd)) { // pfodApp is asking to load dwg "ch"
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





// assumes dwg is 100 wide
void ChoicePanel::sendDrawing(long dwgIdx) {
  if (dwgIdx == 0) {
    dwgsPtr->start(100, 60, dwgsPtr->WHITE, true); //true means more to come,  background defaults to WHITE if omitted i.e. dwgsPtr->start(50,30);
    parserPtr->sendRefreshAndVersion(0); //need to set a version number for image refresh to work!! send the parser version to cache this image, refreshes every 10sec

    dwgsPtr->touchZone().cmd(disabledCmd).idx(dwgDisableControlsIdx).filter(dwgsPtr->TOUCH_DISABLED).size(100, 60).send();
    dwgsPtr->hide().cmd(disabledCmd).send();

    dwgsPtr->label().idx(new_idx).offset(30, 55).text("New Dwg").fontSize(6).underline().send();
    dwgsPtr->touchZone().cmd(newCmd).offset(30, 55).centered().size(30, 8).send();
    cSF(sfPrompt, 120);
    sfPrompt = "Enter New Dwg Name\n<-2><i>A letter then letters, numbers, _ or <space>\nMax length of name : ";
    sfPrompt += DwgName::MAX_DWG_BASE_NAME_LEN;
    dwgsPtr->touchActionInput().cmd(newCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt(sfPrompt.c_str()).send();
    dwgsPtr->touchAction().cmd(newCmd).action(
      dwgsPtr->label().idx(new_idx).offset(30, 55).text("New Dwg").fontSize(6).italic().color(dwgsPtr->RED)
    ).send();
    dwgsPtr->touchAction().cmd(newCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();

    dwgsPtr->index().idx(dwgNameHidden_idx).send(); //   dwgPtr->label().idx(dwgName_idx).offset(5,30).text(Items::getCurrentDwgName()).send();
    dwgsPtr->hide().idx(dwgNameHidden_idx).send();
    dwgsPtr->index().idx(dwgName_idx).send(); //   dwgPtr->label().idx(dwgName_idx).offset(5,30).text(Items::getCurrentDwgName()).send();
    // error msg label
    dwgsPtr->index().idx(errMsg_idx).send(); //dwgsPtr->label().idx(errMsg_idx).fontSize(6).offset(10, 20).bold().italic().color(dwgsPtr->RED).text(errMsg).send();

    dwgsPtr->label().idx(open_idx).offset(70, 55).text("Open dwgs list").fontSize(6).underline().send();
    dwgsPtr->touchZone().cmd(openCmd).offset(70, 55).centered().size(30, 8).send();
    dwgsPtr->touchAction().cmd(openCmd).action(
      dwgsPtr->label().idx(open_idx).offset(70, 55).text("Open dwgs list").fontSize(6).italic().color(dwgsPtr->RED)
    ).send();
    dwgsPtr->touchAction().cmd(openCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();

    dwgsPtr->pushZero(0, 6);

    // place X Pos, Y Pos
    int yPos = 12;
    int step = 20;
    int xPos = 0;
    xPos += step;
    dwgsPtr->pushZero(xPos, yPos);
    dwgsPtr->rectangle().centered().size(8, 8).send();
    dwgsPtr->index().idx(rect_idx).send(); // dwgsPtr->rectangle().idx(rect_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
    dwgsPtr->touchZone().cmd(rectCmd).centered().size(15, 14).send();
    dwgsPtr->touchAction().cmd(rectCmd).action(
      dwgsPtr->rectangle().idx(rect_idx).centered().color(dwgsPtr->RED).size(15, 14)
    ).send();
    dwgsPtr->touchAction().cmd(rectCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();
    xPos += step;
    dwgsPtr->pushZero(xPos, yPos);
    dwgsPtr->line().offset(-4, -4).size(8, 8).send();
    dwgsPtr->index().idx(line_idx).send(); // dwgsPtr->rectangle().idx(line_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
    dwgsPtr->touchZone().cmd(lineCmd).centered().size(15, 14).send();
    dwgsPtr->touchAction().cmd(lineCmd).action(
      dwgsPtr->rectangle().idx(line_idx).centered().color(dwgsPtr->RED).size(15, 14)
    ).send();
    dwgsPtr->touchAction().cmd(lineCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();
    xPos += step;
    dwgsPtr->pushZero(xPos, yPos);
    dwgsPtr->circle().radius(4).send();
    dwgsPtr->index().idx(circle_idx).send(); //dwgsPtr->rectangle().idx(circle_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
    dwgsPtr->touchZone().cmd(circleCmd).centered().size(15, 14).send();
    dwgsPtr->touchAction().cmd(circleCmd).action(
      dwgsPtr->rectangle().idx(circle_idx).centered().color(dwgsPtr->RED).size(15, 14)
    ).send();
    dwgsPtr->touchAction().cmd(circleCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();
    xPos += step;
    dwgsPtr->pushZero(xPos, yPos);
    dwgsPtr->arc().offset(-4, 2).radius(8).start(0).angle(45).send();
    dwgsPtr->index().idx(arc_idx).send(); //dwgsPtr->rectangle().idx(arc_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
    dwgsPtr->touchZone().cmd(arcCmd).centered().size(15, 14).send();
    dwgsPtr->touchAction().cmd(arcCmd).action(
      dwgsPtr->rectangle().idx(arc_idx).centered().color(dwgsPtr->RED).size(15, 14)
    ).send();
    dwgsPtr->touchAction().cmd(arcCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();

    dwgsPtr->popZero(); //pushZero(0,10);

    dwgsPtr->end();



  } else if (dwgIdx == 1) {
    dwgsPtr->startUpdate(false);
    dwgsPtr->pushZero(0, 6);

    dwgsPtr->pushZero(20, 30);
    dwgsPtr->rectangle().centered().size(20, 8).send();
    dwgsPtr->label().idx(label_idx).fontSize(6).bold().text("Label").send();
    dwgsPtr->touchZone().cmd(labelCmd).centered().size(20, 8).send();
    dwgsPtr->touchAction().cmd(labelCmd).action(
      dwgsPtr->label().idx(label_idx).fontSize(6).bold().color(dwgsPtr->RED).text("Label")
    ).send();
    dwgsPtr->touchAction().cmd(labelCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();

    dwgsPtr->pushZero(50, 30);
    dwgsPtr->rectangle().idx(touchZoneRectangle_idx).centered().size(32, 8).send();
    dwgsPtr->label().idx(touchZone_idx).bold().fontSize(6).text("TouchZone").send();
    dwgsPtr->touchZone().cmd(touchZoneCmd).centered().size(30, 8).send();
    dwgsPtr->touchAction().cmd(touchZoneCmd).action(
      dwgsPtr->label().idx(touchZone_idx).bold().fontSize(6).color(dwgsPtr->RED).text("TouchZone")
    ).send();
    dwgsPtr->touchAction().cmd(touchZoneCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();

    dwgsPtr->pushZero(90, 30);
    // always add Y/N label for delete and hide it
    dwgsPtr->label().idx(deleteLabelY_N_idx).text("N").send();
    dwgsPtr->hide().idx(deleteLabelY_N_idx).send();

    dwgsPtr->rectangle().idx(deleteLabelRectangle_idx).filled().rounded().centered().color(dwgsPtr->RED).size(18, 8).send();
    dwgsPtr->label().idx(deleteLabel_idx).fontSize(6).color(dwgsPtr->WHITE).text("Delete").bold().send();
    dwgsPtr->touchZone().cmd(deleteCmd).centered().filter(dwgsPtr->DOWN_UP).size(18, 8).send();
    if (Items::showDwgDelete()) {
      dwgsPtr->touchActionInput().cmd(deleteCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt("Delete dwg\n<-2> <b>y / N").textIdx(deleteLabelY_N_idx).send();
      dwgsPtr->touchAction().cmd(deleteCmd).action(
        dwgsPtr->rectangle().idx(deleteLabel_idx).filled().centered().color(dwgsPtr->BLACK).size(18, 8)
      ).send();
      dwgsPtr->touchAction().cmd(deleteCmd).action(
        dwgsPtr->unhide().cmd(disabledCmd)
      ).send();
    } else {
      dwgsPtr->touchAction().cmd(deleteCmd).action(
        dwgsPtr->label().idx(errMsg_idx).fontSize(6).offset(1, 5).bold().left().italic().color(dwgsPtr->RED).text("Need to delete all the dwg elements first.")
      ).send();
      dwgsPtr->touchAction().cmd(deleteCmd).action(
        dwgsPtr->hide().idx(dwgName_idx)
      ).send();
    }
    dwgsPtr->popZero();

    dwgsPtr->pushZero(70, 41);
    dwgsPtr->label().idx(generateCode_idx).underline().fontSize(6).text("Generate Code").send();
    dwgsPtr->touchZone().cmd(generateCodeCmd).centered().size(30, 8).send();
    dwgsPtr->touchAction().cmd(generateCodeCmd).action(
      dwgsPtr->label().idx(generateCode_idx).italic().fontSize(6).color(dwgsPtr->RED).text("Generate Code")
    ).send();
    dwgsPtr->touchAction().cmd(generateCodeCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();

    dwgsPtr->pushZero(30, 41);
    dwgsPtr->label().idx(saveAs_idx).underline().fontSize(6).text("Save As").send();
    dwgsPtr->touchZone().cmd(saveAsCmd).offset(0, 4).centered().size(30, 8).send();
    cSF(sfPrompt, 120);
    sfPrompt = "Enter Dwg Name\n<-2><i>A letter then letters, numbers, _ or <space>\nMax length of name : ";
    sfPrompt += DwgName::MAX_DWG_BASE_NAME_LEN;
    dwgsPtr->touchActionInput().cmd(saveAsCmd).fontSize(3).color(dwgsPtr->RED).backgroundColor(dwgsPtr->SILVER).prompt(sfPrompt.c_str()).textIdx(dwgNameHidden_idx).send();
    dwgsPtr->touchAction().cmd(saveAsCmd).action(
      dwgsPtr->label().idx(saveAs_idx).italic().fontSize(6).color(dwgsPtr->RED).text("Save As")
    ).send();
    dwgsPtr->touchAction().cmd(saveAsCmd).action(
      dwgsPtr->unhide().cmd(disabledCmd)
    ).send();
    dwgsPtr->popZero();


    update(); // update with current state

    dwgsPtr->popZero(); //pushZero(0,10);
    dwgsPtr->end();

  } else {
    parserPtr->print(F("{}")); // always return somthing
  }

}

void ChoicePanel::sendDrawingUpdates() {
  dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg
  update();
  dwgsPtr->end();
}


void ChoicePanel::update() {
  // control updates
  // update enclosing rectangles
  dwgsPtr->rectangle().idx(rect_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
  dwgsPtr->rectangle().idx(line_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
  dwgsPtr->rectangle().idx(circle_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();
  dwgsPtr->rectangle().idx(arc_idx).centered().color(dwgsPtr->BLACK).size(15, 14).send();

  if (ControlPanel::showTouchZoneOnChoicePanel()) {
    dwgsPtr->unhide().idx(touchZone_idx).send();
    dwgsPtr->unhide().idx(touchZoneRectangle_idx).send();
    dwgsPtr->unhide().cmd(touchZoneCmd).send();
  } else {
    dwgsPtr->hide().idx(touchZone_idx).send();
    dwgsPtr->hide().idx(touchZoneRectangle_idx).send();
    dwgsPtr->hide().cmd(touchZoneCmd).send();
  }

  //  if (Items::showDwgDelete()) {
  //    dwgsPtr->unhide().idx(deleteLabel_idx).send();
  //    dwgsPtr->unhide().idx(deleteLabelRectangle_idx).send();
  //    dwgsPtr->unhide().cmd(deleteCmd).send();
  //  } else {
  //    dwgsPtr->hide().idx(deleteLabel_idx).send();
  //    dwgsPtr->hide().idx(deleteLabelRectangle_idx).send();
  //    dwgsPtr->hide().cmd(deleteCmd).send();
  //  }

  dwgsPtr->label().idx(dwgName_idx).fontSize(9).offset(50, 5).bold().text(Items::getCurrentDwgName()).send();
  dwgsPtr->label().idx(dwgNameHidden_idx).offset(50, 5).text(Items::getCurrentDwgNameBase()).send(); // for touchActionInput
  // error msg label
  if (errMsg[0] != '\0') {
    dwgsPtr->hide().idx(dwgName_idx).send();
    dwgsPtr->unhide().cmd(disabledCmd).send();
  } else {
    dwgsPtr->unhide().idx(dwgName_idx).send();
    dwgsPtr->hide().cmd(disabledCmd).send();
  }
  dwgsPtr->label().idx(errMsg_idx).fontSize(6).offset(2, 5).bold().left().italic().color(dwgsPtr->RED).text(errMsg).send();
  errMsg[0] = '\0'; //clear(); // only display this once
  dwgsPtr->hide().cmd(disabledCmd).send();
  if (disablePanel) {
    dwgsPtr->unhide().cmd(disabledCmd).send();
  }
}
