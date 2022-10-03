/*
  pfodGUIdisplay.h for pfodGUIdesigner
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/
// download SafeString V4.1.5+ library from the Arduino Library manager or from
// https://www.forward.com.au/pfod/ArduinoProgramming/SafeString/index.html
#include "pfodGUIdisplay.h"
#include "SafeString.h"
#include "millisDelay.h"
#include <WiFiClient.h>
#include "ControlPanel.h"
#include "ColourPanel.h"
#include "ChoicePanel.h"
#include "DwgPanel.h"
#include "CodeSupport.h"
#include "DwgsList.h"

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;

void setDisplayDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

// download the libraries from http://www.forward.com.au/pfod/pfodParserLibraries/index.html

#include <ESPBufferedClient.h>
#include <pfodParser.h>
#include "Items.h"

pfodParser parser("V1"); // create a parser with menu version string to handle the pfod messages
ESPBufferedClient bufferedClient;


void sendMainMenu();
void sendMainMenuUpdate();
void closeConnection(Stream *io);

static bool forceReload = true; // for testing force complete reload on every reboot
//static pfodLinkedPointerList<struct pfodDwgVALUES> listDwgValues;
//bool Items::haveItems() = false;
//static struct pfodDwgVALUES *dwgItemValuesPtr = NULL;
//static size_t itemIdx;

// add your pfod Password here for 128bit security
// eg "b0Ux9akSiwKkwCtcnjTnpWp" but generate your own key, "" means no pfod password
#define pfodSecurityCode ""
// see http://www.forward.com.au/pfod/ArduinoWiFi_simple_pfodDevice/index.html for more information and an example
// and QR image key generator.

static bool showColorPanel = false;
static bool showDwgList = false;
static bool showChoicePanel = true;
static bool showControlPanel = true;

pfodDwgs dwgs(&parser);  // drawing support
// note size of main panel 0,0 to 100,100 is hard code in controlPanel
Items items(&parser, &dwgs);
ColourPanel colourPanel(&parser, &dwgs, "a", "t");
DwgPanel dwgPanel(&parser, &dwgs, "b");
ControlPanel controlPanel(&parser, &dwgs, "c");
ChoicePanel choicePanel(&parser, &dwgs, "d");
DwgsList dwgsList(&parser, &dwgs, "e", "t");

static const int dwgSize = 50;
//pfodRectangle rect;
//pfodLine line;
//pfodCircle circle;
//pfodArc arc;

static bool displayInitialized = false;
//static pfodDwgTypeEnum selectedDwgItemType = pfodNONE;

void fullReload() {
  dwgPanel.reload();
  controlPanel.reload();
  choicePanel.reload();
  dwgsList.reload();
  forceReload = true;
}

void disableControls(bool flag) {
  // enable/disable control choice
  controlPanel.disable(flag);
  choicePanel.disable(flag);
  //  if (debugPtr) {
  //    debugPtr->print("disableControls:"); debugPtr->print(flag ? "true" : "false"); debugPtr->println();
  //  }
}

void initializeDisplay(Print *serialOutPtr) {
  if (displayInitialized) {
    return;
  }
  dwgs.reserveIdx(Items::dwgReservedIdxs); // reserve the first 1 to 1000 index numbers for use by dwg items add/edits
  displayInitialized = true;
  dwgPanel.setDwgSize(dwgSize);
  controlPanel.setDwgSize(dwgSize);
  Items::init(); // false if file system init fails
  dwgPanel.setShowMoveTouch(Items::haveItems());
  CodeSupport::init(&parser, serialOutPtr); //
}


void connect_pfodParser(WiFiClient * client) {
  parser.connect(bufferedClient.connect(client)); // sets new io stream to read from and write to
}

// called from main .ino when connected
void handle_pfodParser() {
  if (!displayInitialized) {
    if (debugPtr) {
      debugPtr->println("Error: Call initializeDisplay() first.");
    }
    return;
  }
  uint8_t cmd = parser.parse(); // parse incoming data from connection
  // parser returns non-zero when a pfod command is fully parsed
  if (cmd != 0) { // have parsed a complete msg { to }
    byte dwgCmd = parser.parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd
    pfod_MAYBE_UNUSED(dwgCmd); // may not be used, just suppress warning
    uint8_t* pfodFirstArg = parser.getFirstArg(); // may point to \0 if no arguments in this msg.
    pfod_MAYBE_UNUSED(pfodFirstArg); // may not be used, just suppress warning
    long pfodLongRtn; // used for parsing long return arguments, if any
    pfod_MAYBE_UNUSED(pfodLongRtn); // may not be used, just suppress warning

    if ('.' == cmd) {
      // pfodApp has connected and sent {.} , it is asking for the main menu
      if (parser.isRefresh() && !forceReload) {
        sendMainMenuUpdate(); // menu is cached just send update
      } else {
        sendMainMenu(); // send back the menu designed
      }

      // handle {@} request
    } else if ('@' == cmd) { // pfodApp requested 'current' time
      parser.print(F("{@`0}")); // return `0 as 'current' raw data milliseconds

    } else if (colourPanel.processDwgLoad()) { // false if not load dwg "a"
    } else if (dwgPanel.processDwgLoad()) {  // false if not load dwg "b"
    } else if (controlPanel.processDwgLoad()) { // false if not load dwg "c"
    } else if (choicePanel.processDwgLoad()) { // false if not load dwg "d"
    } else if (dwgsList.processDwgLoad()) { // false if not load dwg "e"

    } else if (parser.cmdEquals('A')) { // handle colour panel
      showDwgList = false;
      showColorPanel = false;
      if (colourPanel.processTouchCmd()) {
        int color = colourPanel.getSelectedColor();
        bool showBackgroundColor = controlPanel.getShowBackgroundColorPanelFlag();
//        if (debugPtr) {
//          debugPtr->print("controlPanel.getShowBackgroundColorPanelFlag() : "); debugPtr->println(showBackgroundColor);
//        }
        if (showBackgroundColor) {
          Items::getControlDisplayedItem()->valuesPtr->backgroundColor = color;
        } else {
          Items::getControlDisplayedItem()->valuesPtr->color = color;
        }
        Items::saveSelectedItem();
      }
      showColorPanel = false;
      controlPanel.setShowColorPanelFlag(showColorPanel);
      dwgPanel.setShowMoveTouch(!showColorPanel);
      fullReload();
      // update the panels
      sendMainMenuUpdate();

    } else if (parser.cmdEquals('B')) { // user pressed dwg panel
      showDwgList = false;
      showColorPanel = false;
      if (dwgPanel.processItemShowCmd()) {
        // have sent unhide update
        if ( 
            ((Items::getControlDisplayedItem()->type == pfodTOUCH_ZONE) &&  (Items::getControlDisplayedItem()->hasActionInput()))
            && (Items::getControlDisplayedItem()->hasAction()) 
            ) { 
            //(Items::getControlDisplayedItem()->getActionInputPtr() != NULL))  ) {
          delay(500); // small delay to let user see the actions is there is an actionInput AND there are actions
          // dwgPanel.reload();
        }
        sendMainMenuUpdate();
      } else if (dwgPanel.processDwgCmds()) {
        if (Items::isDwgSelectedItemIdxUpdated() || Items::isControlSelectedItemIdxUpdated()) { // update selection
          fullReload(); // update
        }
        disableControls(dwgPanel.isAdjustCenter());
        if (dwgPanel.isAdjustCenter()) {
          showChoicePanel = false;
          showControlPanel = false;
        } else {
          showChoicePanel = true;
          showControlPanel = true;
        }
        // disable/enable control and choice
        sendMainMenuUpdate();
      } else {
        parser.print("{}");
      }

    } else if (parser.cmdEquals('C')) { // user pressed control
      showDwgList = false;
      showColorPanel = false;
      if (controlPanel.processDwgCmds()) { // handle control panel cmds
        if (Items::isDwgSelectedItemIdxUpdated() || Items::isControlSelectedItemIdxUpdated()) { // update selection
          fullReload(); // update
        }
        showColorPanel = controlPanel.getShowColorPanelFlag();
        showChoicePanel = controlPanel.getShowChoicePanelFlag();
        dwgPanel.setShowMoveTouch(!showColorPanel);
        sendMainMenuUpdate(); // update the drawing
      } else {
        parser.print("{}");
      }
    } else if (parser.cmdEquals('D')) { // user pressed choice panel
      showDwgList = false;
      showColorPanel = false;
      pfodDwgTypeEnum selectedDwgItemType = pfodNONE;
      // first check generate code
      if (choicePanel.processGenerateCmd()) {
        if (Items::haveItems()) {
          Items::writeCode();
          //          // open raw data screen
        } else {
          sendMainMenuUpdate(); // update the drawing
        }
      } else if (choicePanel.processSaveAsCmd()) {
        choicePanel.reload();
        choicePanel.processDwgLoad(); // update the drawing
        // on other changes needed
        
      } else if (choicePanel.processNewCmd()) {
        fullReload();
        sendMainMenuUpdate(); // update the drawing
        // on other changes needed

      } else if (choicePanel.processDwgDelete()) {
        if (Items::deleteDwg()) {
          fullReload();
          sendMainMenuUpdate(); // update the drawing
        } else {
          choicePanel.processDwgLoad(); // update the drawing
        }
      } else if (choicePanel.processOpenCmd()) {
        showDwgList = true;
        fullReload();
        sendMainMenuUpdate(); // update the drawing

      } else if (choicePanel.processDwgCmds()) { // handle control panel cmds
        selectedDwgItemType = choicePanel.getSelectedDwgItemType();
        Items::appendControlItem(selectedDwgItemType); // updated itemIdx
        if ((selectedDwgItemType == pfodTOUCH_ZONE) || (Items::getControlSelectedItemType() == pfodTOUCH_ACTION)) {
          fullReload();
        }
        showColorPanel = false;
        dwgPanel.setShowMoveTouch(!showColorPanel);
        if (Items::isDwgSelectedItemIdxUpdated() || Items::isControlSelectedItemIdxUpdated()) { // update selection
          fullReload();
        }
        sendMainMenuUpdate(); // update the drawing

      } else {
        sendMainMenuUpdate(); // update the drawing
      }

    } else if (parser.cmdEquals('E')) { // user pressed dwgList panel
      showDwgList = false;
      showColorPanel = false;
      // first check generate code
      if (dwgsList.processDwgCmds()) {
        showDwgList = false;
        fullReload();
        sendMainMenuUpdate(); // update the drawing
      } else {
        // can show error msg here
        dwgsList.sendDrawingUpdates();
      }

    } else if ('!' == cmd) {
      // CloseConnection command
      closeConnection(parser.getPfodAppStream());
    } else {
      // unknown command
      parser.print(F("{}")); // always send back a pfod msg otherwise pfodApp will disconnect.
    }
  }
}


void closeConnection(Stream * io) {
  (void)(io); // unused
  // add any special code here to force connection to be dropped
  parser.closeConnection(); // nulls io stream
  bufferedClient.stop(); // clears client reference
  //  clientPtr->stop();
  //  clientPtr = NULL;
}

void sendMainMenu() {
  // !! Remember to change the parser version string
  //    every time you edit this method
  parser.print(F("{,"));  // start a Menu screen pfod message
  parser.print("~"); // no prompt
  parser.sendRefreshAndVersion(0);
  // send menu items
  parser.print(F("|+A")); // colorPanel
  if ((!showColorPanel) || showDwgList) {
    parser.print('-'); // hide
  }
  parser.print('~'); parser.print(colourPanel.getDwgLoadCmd());

  parser.print(F("|+B")); //dwgPanel
  if ((!Items::haveItems()) || showDwgList) {
    parser.print('-'); // hide
  }
  parser.print('~'); parser.print(dwgPanel.getDwgLoadCmd());

  parser.print(F("|+C")); // control Panel
  if ((showColorPanel)  || (!showControlPanel) || showDwgList || (!Items::haveItems())) {
    parser.print('-'); // hide
  }
  parser.print('~');  parser.print(controlPanel.getDwgLoadCmd());

  parser.print(F("|+D")); // choicePanel
  if (showColorPanel ||  (!showChoicePanel) || showDwgList) {
    parser.print('-'); // hide
  }
  parser.print('~');   parser.print(choicePanel.getDwgLoadCmd());

  parser.print(F("|+E")); //dwgsList
  if (showColorPanel || (!showDwgList)) {
    parser.print('-'); // hide
  }
  parser.print('~');   parser.print(dwgsList.getDwgLoadCmd());

  parser.print(F("}"));  // close pfod message
}

void sendMainMenuUpdate() {
  parser.print(F("{;"));  // start an Update Menu pfod message
  parser.print(F("~")); // no change to colours and size
  // send menu items
  
  parser.print(F("|+A")); // colorPanel
  if ((!showColorPanel) || showDwgList) {
    parser.print('-'); // hide
  }

  parser.print(F("|+B")); // dwgPanel
  if ((!Items::haveItems()) || showDwgList) {
    parser.print('-'); // hide
  }

  parser.print(F("|+C")); // controlPanel
  if ((showColorPanel) || (!showControlPanel) || showDwgList || (!Items::haveItems())) {
    parser.print('-'); // hide
  }

  parser.print(F("|+D")); // choicePanel
  if (showColorPanel ||  (!showChoicePanel) || showDwgList) {
    parser.print('-'); // hide
  }

  parser.print(F("|+E")); // dwgsList
  if (showColorPanel || (!showDwgList)) {
    parser.print('-'); // hide
  }

  parser.print(F("}"));  // close pfod message
  // ============ end of menu ===========
}
