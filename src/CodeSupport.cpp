/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/
#include "CodeSupport.h"
#include "Items.h"
#include "ItemsIOsupport.h"
#include <SafeString.h>

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;

// for later use
void CodeSupport::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

Print *CodeSupport::serialOutPtr = NULL;
pfodParser* CodeSupport::parserPtr = NULL;


//// call this before each output
void CodeSupport::clearIdxNos() {
  Items::clearIdxNos();
}


void CodeSupport::init(pfodParser *_parserPtr, Print *_serialOutPtr)  {
  parserPtr = _parserPtr;
  serialOutPtr = _serialOutPtr;
}

bool CodeSupport::write_H(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print *outPtr) {
  if (!outPtr) {
    return false;
  }
  String str = "\
// split here for .h file\n\
\n\
// =========  Start of ";
  str += Items::getCurrentDwgNameBase();
  str += ".h generated code ==========\n\
";

  cSF(sfName, strlen(Items::getCurrentDwgNameBase()));
  sfName = Items::getCurrentDwgNameBase();
  sfName.toUpperCase();

  str += "#ifndef ";
  str += sfName.c_str();
  str += "_H\n\
#define ";
  str += sfName.c_str();
  str += "_H\n\
\n\
//  ";
  str += Items::getCurrentDwgNameBase();
  str += ".h from ";
  str += Items::getCurrentDwgName();
  str += " by pfodGUIdesigner V";
  str += Items::versionNo;
  str += "\n\
";
  outPtr->print(str);
  copyright(outPtr);

  str = "\n\
#include <pfodParser.h>\n\
#include <pfodDwgs.h>\n\
#include <pfodDrawing.h>\n\
\n\
class ";
  str += Items::getCurrentDwgNameBase();
  str += " : public pfodDrawing {\n\
  public:\n\
    ";
  str += Items::getCurrentDwgNameBase();
  str += "(pfodParser &parser, pfodDwgs& dwgs);\n\
    bool sendDwg(); // returns is dwg sent else false i.e. not this dwg's loadCmd\n\
    bool processDwgCmds(); // return true if handled else false\n\
\n\
  private:\n\
    void sendFullDrawing(long dwgIdx);\n\
    void sendUpdate();\n\
    void drawDwg0();\n\
    void updateDwg();\n\
    bool forceReload;\n\
    void printDwgCmdReceived(Print *outPtr);\n";
  outPtr->print(str);

  writeVars(listDwgValuesPtr, outPtr);
  str = "};\n\
#endif\n\
// =========  End of ";
  str += Items::getCurrentDwgNameBase();
  str += ".h generated code ==========\n\
\n";
  outPtr->print(str);
  return true;
}

bool CodeSupport::write_CPP(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print *outPtr) {
  if (!outPtr) {
    return false;
  }

  String str = "\
// split here for .cpp file\n\
\n\
// =========  Start of ";
  str += Items::getCurrentDwgNameBase();
  str += ".cpp generated code ==========\n\
//  ";
  str += Items::getCurrentDwgNameBase();
  str += ".cpp from ";
  str += Items::getCurrentDwgName();
  str += " by pfodGUIdesigner V";
  str += Items::versionNo;
  str += "\n\
";
  outPtr->print(str);
  copyright(outPtr);

  cSF(sfName, strlen(Items::getCurrentDwgNameBase()));
  sfName = Items::getCurrentDwgNameBase();
  sfName.toUpperCase();

  str = "\n\
#ifndef ";
  str += sfName.c_str();
  str += "_H\n\
#include \"";
  str += Items::getCurrentDwgNameBase();
  str += ".h\"\n\
#endif\n\n";

  str += Items::getCurrentDwgNameBase();
  str += "::";
  str += Items::getCurrentDwgNameBase();
  str += "(pfodParser &parser, pfodDwgs& dwgs) : pfodDrawing(parser, dwgs) {\n\
  forceReload = true; // force reload on reboot, e.g. reprogrammed\n\
}\n";
  outPtr->print(str);

  // proc header
  clearIdxNos(); // call this first
  outPtr->println();
  outPtr->print("bool "); outPtr->print(Items::getCurrentDwgNameBase()); outPtr->println("::processDwgCmds() { // return true if handled else false");
  outPtr->println("  byte dwgCmd = parserPtr->parseDwgCmd(); // dwgCmd is '\0' if this is NOT a dwg cmd");
  outPtr->println("  if (!dwgCmd) {");
  outPtr->println("    return false; // not dwg cmd not handled");
  outPtr->println("  }");
  {
    cSF(sfName, 30);
    for (size_t i = 0; i < listDwgValuesPtr->size(); i++) {
      ValuesClass *valuesClassPtr = listDwgValuesPtr->get(i);
      if (valuesClassPtr->type == pfodTOUCH_ZONE) {
        Items::getTouchZoneCmdName(sfName, valuesClassPtr);
        output_dwgCmdProcessor(valuesClassPtr, outPtr, sfName);
        // ignore errors when sending to debug/serial
      }
    }
    outPtr->println("  return false; // not handled");
    outPtr->println("}");
    outPtr->println();
  }

  // proc header
  clearIdxNos(); // call this first
  outPtr->println();
  outPtr->print("void "); outPtr->print(Items::getCurrentDwgNameBase()); outPtr->println("::drawDwg0() {");
  {
    // loop through list by idx to avoid changes in the iterator position
    for (size_t i = 0; i < listDwgValuesPtr->size(); i++) {
      ValuesClass *valuesClassPtr = listDwgValuesPtr->get(i);
      cSF(sfName, 30);
      if (valuesClassPtr->type != pfodTOUCH_ZONE) {
        Items::getIdxName(sfName, valuesClassPtr);
      } else { // touch zone
        Items::getTouchZoneCmdName(sfName, valuesClassPtr);
      }
      output_drawDwgCmds(valuesClassPtr, outPtr, sfName);
    }
    //  outPtr->println("  dwgsPtr->popZero();");
    outPtr->println("}");
    outPtr->println();
  }

  {
    // proc header
    clearIdxNos(); // call this first
    outPtr->print("void "); outPtr->print(Items::getCurrentDwgNameBase());  outPtr->println("::updateDwg() {");
    // loop through list by idx to avoid changes in the iterator position
    for (size_t i = 0; i < listDwgValuesPtr->size(); i++) {
      ValuesClass *valuesClassPtr = listDwgValuesPtr->get(i);
      output_updateDwg(valuesClassPtr, outPtr);
    }
    outPtr->println("}\n");
  }

  str = "bool ";
  str += Items::getCurrentDwgNameBase();
  str += "::sendDwg() {\n\
  if (!parserPtr->cmdEquals(*this)) {\n\
    return false; // not this dwg's loadCmd\n\
  }  // else\n\
  // ======== begin debugging\n\
  Serial.print(\" sendDwg() cmd \"); Serial.print((char*)parserPtr->getCmd());\n\
  if (parserPtr->isRefresh()) {\n\
    Serial.print(\" isRefresh\");\n\
  }\n\
  byte* argPtr = parserPtr->getFirstArg();\n\
  if (*argPtr) {\n\
    Serial.print(\"  args: \");\n\
    while (*argPtr) {\n\
      Serial.print(\"  \"); Serial.print((char*)argPtr);\n\
      argPtr = parserPtr->getNextArg(argPtr);\n\
    }\n\
  }\n\
  Serial.println();\n\
  // ======= end debugging =======\n\
  if (parserPtr->isRefresh() && !forceReload) { // refresh and not forceReload just send update\n\
    sendUpdate();\n\
  } else {\n\
    forceReload = false; //\n\
    long pfodLongRtn = 0;\n\
    uint8_t* pfodFirstArg = parserPtr->getFirstArg(); // may point to \\0 if no arguments in this msg.\n\
    parserPtr->parseLong(pfodFirstArg, &pfodLongRtn); // parse first arg as a long\n\
    sendFullDrawing(pfodLongRtn);\n\
  }\n\
  return true;\n\
}\n\n";
  outPtr->print(str);

  str = "void ";
  str += Items::getCurrentDwgNameBase();
  str += "::sendFullDrawing(long dwgIdx) {\n\
  if (dwgIdx == 0) {\n\
    dwgsPtr->start(50, 50, dwgsPtr->WHITE, true);  // the dwg size 50x50 and color (WHITE) are ignored when this drawing is inserted in the main dwg\n\
    //true means more to come\n\
    parserPtr->sendRefreshAndVersion(0); //need to set a version number for image refresh to work!!\n\
    drawDwg0(); //\n\
    dwgsPtr->end();\n\
\n\
    // if drawing msg is >1023 split it in to two or more parts and add extra else if (dwgIdx == .. ) blocks here\n\
\n\
  } else if (dwgIdx == 1) {\n\
    dwgsPtr->startUpdate(false);  // always send update last so this is the last msg => false\n\
    updateDwg(); // update with current state, // defined in display.cpp\n\
    dwgsPtr->end();\n\
\n\
  } else {\n\
    parserPtr->print(F(\"{}\")); // always return somthing\n\
  }\n\
}\n\
\n";
  outPtr->print(str);

  str = "void ";
  str += Items::getCurrentDwgNameBase();
  str += "::sendUpdate() {\n\
  dwgsPtr->startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg\n\
  updateDwg();\n\
  dwgsPtr->end();\n\
}\n\
\n";
  outPtr->print(str);


  output_printDwgCmdReceived(outPtr);

  str = "// =========  End of ";
  str += Items::getCurrentDwgNameBase();
  str += ".cpp generated code ==========\n\
\n\
// end of ";
  str += Items::getCurrentDwgNameBase();
str += " class files\n\
\n";
  outPtr->print(str);
  return true;
}

bool CodeSupport::output_dwgCmdProcessor(ValuesClass * valuesClassPtr, Print * outPtr, SafeString & sfName) {
  if (!outPtr) {
    return false;
  }
  (void)(valuesClassPtr); // to avoid warning about unused
  if (Items::haveSeenUpdateIdx()) {
    outPtr->print("  else ");
  } else {
    outPtr->print("  ");
  }
  outPtr->print("if (parserPtr->dwgCmdEquals("); outPtr->print(sfName); outPtr->println(")) { // handle _touchZone_cmd_..");
  outPtr->print("    Serial.print(\" Got pfodAutoCmd:\"); Serial.println(\""); outPtr->print(sfName); outPtr->println("\");");
  outPtr->println("    printDwgCmdReceived(&Serial); // does nothing if passed NULL");
  outPtr->println("    // add your cmd handling code here");
  outPtr->println("    // send update");
  outPtr->println("    sendUpdate();");
  outPtr->println("    return true;");
  outPtr->println("  }");
  Items::setSeenUpdateIdx();
  return true;
}

bool CodeSupport::output_printDwgCmdReceived(Print * outPtr) {
  if (!outPtr) {
    return false;
  }
  String str = "void ";
  str += Items::getCurrentDwgNameBase();
  str += "::printDwgCmdReceived(Print *outPtr) {\n\
  if (!outPtr) {\n\
    return;\n\
  }\n\
  outPtr->print(\"     touchZone cmd \"); outPtr->print((const char*)parserPtr->getDwgCmd()); outPtr->print(\" at \");\n\
  outPtr->print(\"(\"); outPtr->print(parserPtr->getTouchedX()); outPtr->print(\",\");\n\
  outPtr->print(parserPtr->getTouchedY()); outPtr->print(\") \");\n\
  if (parserPtr->isTouch()) {\n\
    outPtr->print(\"touch type:TOUCHED\");\n\
  }\n\
  outPtr->println();\n\
  if (*parserPtr->getEditedText()) { // first char not '\\0'\n\
    outPtr->print(\"     Edited text '\"); outPtr->print((const char*)parserPtr->getEditedText()); outPtr->println(\"'\");\n\
  }\n\
}\n";
  outPtr->print(str);
  return true;
}

bool CodeSupport::output_drawDwgCmds(ValuesClass * valuesClassPtr, Print * outPtr, SafeString & sfName) {
  bool rtn = true;
  if (!outPtr) {
    return false;
  }
  if (!valuesClassPtr) {
    return false;
  }
  if (valuesClassPtr->isIndexed) {
    // just output idx if not a touchZone
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      outPtr->print("  dwgsPtr->index().idx("); outPtr->print(sfName); outPtr->print(").send();");
    } else {
      outPtr->print("  dwgsPtr->touchZone().cmd("); outPtr->print(sfName); outPtr->print(").send();");
    }
  } else {
    rtn = ItemsIOsupport::writeItem(outPtr, valuesClassPtr, sfName, true, "  dwgsPtr->");
  }
  outPtr->println();
  return rtn;
}

bool CodeSupport::output_updateDwg(ValuesClass * valuesClassPtr, Print * outPtr) {
  bool rtn = true;
  if (!outPtr) {
    return false;
  }
  if (!valuesClassPtr) {
    return false;
  }

  // see if there are any updates
  cSF(sfName, 30);
  if (valuesClassPtr -> isIndexed) {
    if (valuesClassPtr->type != pfodTOUCH_ZONE) {
      Items::getIdxName(sfName, valuesClassPtr);
      rtn = ItemsIOsupport::writeItem(outPtr, valuesClassPtr, sfName, true, "  dwgsPtr->");
    } else {
      Items::getTouchZoneCmdName(sfName, valuesClassPtr);
      rtn = ItemsIOsupport::writeItem(outPtr, valuesClassPtr, sfName, true, "  dwgsPtr->");
    }
    outPtr->println();
  }
  return rtn;
}



bool CodeSupport::writeVars(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print * outPtr) {
  cSF(sfName, 30);
  clearIdxNos(); // call this first
  for (size_t i = 0; i < listDwgValuesPtr->size(); i++) {
    ValuesClass *valuesClassPtr = listDwgValuesPtr->get(i);
    // Items::printDwgValues(" writeVars", valuesClassPtr);
    if (valuesClassPtr->type == pfodTOUCH_ZONE) {
      Items::getTouchZoneCmdName(sfName, valuesClassPtr);
      outPtr->print("    pfodAutoCmd ");
      outPtr->print(sfName);
      outPtr->println(";");
    } else {
      if (Items::getIdxName(sfName, valuesClassPtr)) {
        outPtr->print("    pfodAutoIdx ");
        outPtr->print(sfName);
        outPtr->println(";");
        //    } else if (Items::getTouchZoneCmdName(sfName, valuesClassPtr)) {
        //      outPtr->print("static pfodAutoCmd ");
        //      outPtr->print(sfName);
        //      outPtr->println(";");
      }
    }
  }
  return true;
}

bool CodeSupport::outputDisplayCode(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print * outPtr, float xCenter, float yCenter, float scale) {
  if (listDwgValuesPtr->size() == 0) {
    outPtr->println("\\No dwg Items");
  } else {
    // outPtr->println("have some items");
    writeHeader(outPtr);
    write_H(listDwgValuesPtr, outPtr);
    write_CPP(listDwgValuesPtr, outPtr);
    writeFooter(outPtr, xCenter, yCenter, scale);
  }
  return true;
}

bool CodeSupport::writeCode(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, float xCenter, float yCenter, float scale) {
  bool rtn = true;
  if (serialOutPtr) {
    outputDisplayCode(listDwgValuesPtr, serialOutPtr, xCenter, yCenter, scale);
  }
  if (parserPtr) {
    if (!outputDisplayCode(listDwgValuesPtr, parserPtr, xCenter, yCenter, scale)) {
      return false;
    }
  }
  return rtn;
}

bool CodeSupport::writeHeader(Print * outPtr) {
  if (!outPtr) {
    return false;
  }
  String str =
    "// for ES32 (e.g Sparkfun Thing ESP32-C3 etc) and ESP8266 (e.g. HUZZAH ESP8266 etc)\n\
\n\
// Network Settings\n\
#include <IPAddress.h>\n\
const char staticIP[] = \"10.1.1.71\";\n\
const char ssid[] = \"ssid\";\n\
const char pw[] = \"password\";\n\
IPAddress dns1(8, 8, 8, 8);\n\
IPAddress dns2(8, 8, 8, 4);\n\
\n\
\n";
  outPtr->println(str);
  return true;
}

bool CodeSupport::writeFooter(Print * outPtr, float xCenter, float yCenter, float scale) {
  if (!outPtr) {
    return false;
  }
  String str =
    "\n\
// =========  Start of Test Code to test above class generated code ==========\n\
\n\
// This template code uses above class and inserts that dwg in the main dwg\n\
/*\n\
   Test code for pfodGUIdesigner\n\
   by Matthew Ford,  2022/10/01\n\
*/\n";
  outPtr->print(str);
  copyright(outPtr);
  str =
    "\n\
#if defined (ESP32)\n\
#include <WiFi.h>\n\
#elif defined (ESP8266)\n\
#include <ESP8266WiFi.h>\n\
#else\n\
#error \"Only ESP32 and ESP8266 supported\"\n\
#endif\n\
#include <pfodEEPROM.h>\n\
\n\
#include <pfodParser.h>\n\
#include <pfodDwgs.h>\n";
  {
    cSF(sfName, strlen(Items::getCurrentDwgNameBase()));
    sfName = Items::getCurrentDwgNameBase();
    sfName.toUpperCase();
    str += "\n\
#ifndef ";
    str += sfName.c_str();
    str += "_H\n\
#include \"";
    str += Items::getCurrentDwgNameBase();
    str += ".h\"\n\
#endif\n";
  }
  str += "\n\
pfodAutoCmd mainDwgLoadCmd; // the main empty dwg\n\
static bool forceMainMenuReload = true; // for testing force complete reload on every reboot\n\
static bool forceMainDwgReload = true; // for testing force complete reload on every reboot\n\
static unsigned long dwgRefresh = 0;//set to 0 to disable auto refresh or 10000; to  refresh every 10sec\n\
// when on the front screen, the mobile back button will also refresh, otherwise goes back to the previous menu/screen\n\
\n\
// install library \"ESPAutoWiFiConfig.h\"  from Arduin or download from  https://www.forward.com.au/pfod/ESPAutoWiFiConfig/index.html\n\
// for ESPBufferedClient\n\
#include <ESPBufferedClient.h>\n\
\n\
const unsigned long BAUD_RATE = 115200ul;\n\
pfodParser parser(\"V1\"); // create a parser with menu version string to handle the pfod messages\n\
pfodDwgs dwgs(parser);  // drawing support\n\
ESPBufferedClient bufferedClient;\n\
\n\
";
  str += Items::getCurrentDwgNameBase();
  //make object name _ lowercase(getCurrentDwgNameBase)
  cSF(sfName, strlen(Items::getCurrentDwgNameBase()));
  sfName = Items::getCurrentDwgNameBase();
  sfName.toLowerCase();
  str += " _"; // add leading _
  str += sfName.c_str();
  str += "(parser, dwgs); // create dwg object and add to parser processing\n\
\n\
// send main dwg that just inserts the designed dwg at the pushZero position and scaling.\n\
bool sendMainDwg() {\n\
  // main dwg just inserts desgined dwg\n\
  dwgs.start(50, 50, dwgs.WHITE, false); //false means NO more to come,  background defaults to WHITE if omitted i.e. dwgs.start(50,30);\n\
  parser.sendRefreshAndVersion(0); //need to set a version number for image refresh to work!! send the parser version to cache this image, refreshes every 10sec\n\
  dwgs.pushZero(";
  str += xCenter;
  str += ",";
  str += yCenter;
  str += ",";
  str += scale;
  str += "); // position and scale the inserted designed dwg\n\
  dwgs.insertDwg().loadCmd(_";
  str += sfName.c_str();
  str += ").send(); // insert the dwg object in the main dwg dwg\n\
  dwgs.popZero();\n\
  dwgs.end();\n\
  return true;\n\
}\n\
\n\
void sendMainDwgUpdate() { // nothing here, but insertedDwgs updated by parser.parse()\n\
  dwgs.startUpdate();  // dwg refresh updates must be <1024 bytes, i.e. can only be 1 msg\n\
  dwgs.end();\n\
}\n\
\n\
void handle_pfodParser() {\n\
  uint8_t cmd = parser.parse(); // parse incoming data from connection\n\
  // parser returns non-zero when a pfod command is fully parsed and not consumed by an inserted dwg\n\
  if (cmd != 0) { // have parsed a complete msg { to }\n\
    // == begin debugging\n\
    if (cmd == '!') {\n\
      Serial.print(\" handle_pfodParser() closeConnection cmd \"); Serial.print((char)cmd);\n\
    } else {\n\
      Serial.print(\" handle_pfodParser() cmd \"); Serial.print((char*)parser.getCmd());\n\
      if (parser.isRefresh()) {\n\
        Serial.print(\" isRefresh\");\n\
      }\n\
      byte* argPtr = parser.getFirstArg();\n\
      if (*argPtr) {\n\
        Serial.print(\"  args: \");\n\
        while (*argPtr) {\n\
          Serial.print(\"  \"); Serial.print((char*)argPtr);\n\
          argPtr = parser.getNextArg(argPtr);\n\
        }\n\
      }\n\
    }\n\
    Serial.println();\n\
    // ========== end debugging\n\
    if ('.' == cmd) {\n\
      // pfodApp has connected and sent {.} , it is asking for the main menu\n\
      if (parser.isRefresh() && !forceMainMenuReload) {\n\
        sendMainMenuUpdate(); // menu is cached just send update\n\
      } else {\n\
        forceMainMenuReload = false;\n\
        sendMainMenu(); // send back the menu designed\n\
      }\n\
\n\
    } else if (parser.cmdEquals(mainDwgLoadCmd)) {\n\
      if (parser.isRefresh() && !forceMainDwgReload) { // refresh and not forceReload just send update\n\
        sendMainDwgUpdate();\n\
      } else {\n\
        forceMainDwgReload = false; //\n\
        sendMainDwg(); // just inserts dwgs and that will update them\n\
      }\n\
\n\
    } else if (parser.cmdEquals('A')) { // click in A menu item that holds the main dwg\n\
      // not used as inserted dwgs handle all the clicks\n\
      sendMainMenuUpdate(); // always reply to the msg.\n\
\n\
      // handle {@} request\n\
    } else if ('@' == cmd) { // pfodApp requested 'current' time\n\
      parser.print(F(\"{@`0}\")); // return `0 as 'current' raw data milliseconds\n\
    } else if ('!' == cmd) {\n\
      // CloseConnection command\n\
      closeConnection(parser.getPfodAppStream());\n\
    } else {\n\
      // unknown command\n\
      parser.print(F(\"{}\")); // always send back a pfod msg otherwise pfodApp will disconnect.\n\
    }\n\
  }\n\
}\n\
\n\
void connect_pfodParser(WiFiClient * client) {\n\
  parser.connect(bufferedClient.connect(client)); // sets new io stream to read from and write to\n\
}\n\
\n\
void closeConnection(Stream * io) {\n\
  (void)(io); // unused\n\
  // add any special code here to force connection to be dropped\n\
  parser.closeConnection(); // nulls io stream\n\
  bufferedClient.stop(); // clears client reference\n\
}\n\
\n\
void sendMainMenu() {\n\
  parser.print(F(\"{,\"));  // start a Menu screen pfod message\n\
  parser.print(\"~\"); // no prompt\n\
  parser.sendRefreshAndVersion(dwgRefresh);\n\
  // send menu items\n\
  parser.print(F(\"|+A~\"));\n\
  parser.print(mainDwgLoadCmd); // the unique load cmd\n\
  parser.print(F(\"}\"));  // close pfod message\n\
}\n\
\n\
void sendMainMenuUpdate() {\n\
  parser.print(F(\"{;\"));  // start an Update Menu pfod message\n\
  parser.print(F(\"~\")); // no change to colours and size\n\
  // send menu items\n\
  parser.print(F(\"|+A\")); // reload A calls for dwg z update\n\
  parser.print(F(\"}\"));  // close pfod message\n\
  // ============ end of menu ===========\n\
}\n\
\n\
void setup() {\n\
  EEPROM.begin(512);  // only use 20bytes for pfodSecurity but reserve 512\n\
  Serial.begin(115200);\n\
  Serial.println();\n\
  for (int i = 10; i > 0; i--) {\n\
    Serial.print(i); Serial.print(' ');\n\
    delay(500);\n\
  }\n\
\n\
  if (!connectToWiFi(staticIP, ssid, pw)) {\n\
    Serial.println(\" Connect to WiFi failed.\");\n\
    while (1) {\n\
      delay(10); // wait here but feed WDT\n\
    }\n\
  } else {\n\
    Serial.print(\" Connected to \"); Serial.print(ssid); Serial.print(\"  with IP:\"); Serial.println(WiFi.localIP());\n\
  }\n\
\n\
  start_pfodServer();\n\
}\n\
\n\
void loop() {\n\
  handle_pfodServerConnection();\n\
}\n\
\n\
// ------------------ trival telnet running on port 4989 server -------------------------\n\
//how many clients should be able to telnet to this\n\
#define MAX_SRV_CLIENTS 1\n\
\n\
WiFiServer pfodServer(4989);\n\
WiFiClient pfodServerClients[MAX_SRV_CLIENTS];\n\
\n\
void start_pfodServer() {\n\
  pfodServer.begin();\n\
  pfodServer.setNoDelay(true);\n\
  Serial.print(\"pfodServer Ready to Use on \");\n\
  Serial.print(WiFi.localIP());\n\
  Serial.println(\":4989\");\n\
}\n\
\n\
void handle_pfodServerConnection() {\n\
  //check if there are any new clients\n\
  uint8_t i;\n\
  if (pfodServer.hasClient()) {\n\
    for (i = 0; i < MAX_SRV_CLIENTS; i++) {\n\
      //find free/disconnected spot\n\
      if (!pfodServerClients[i] || !pfodServerClients[i].connected()) {\n\
        if (pfodServerClients[i]) {\n\
          pfodServerClients[i].stop();\n\
        }\n\
        pfodServerClients[i] = pfodServer.available();\n\
        if (!pfodServerClients[i]) {\n\
          Serial.println(\"available broken\");\n\
        } else {\n\
          Serial.print(\"New client: \");\n\
          Serial.print(i); Serial.print(' ');\n\
          Serial.println(pfodServerClients[i].remoteIP());\n\
          connect_pfodParser(&pfodServerClients[i]);\n\
          break;\n\
        }\n\
      }\n\
    }\n\
    if (i >= MAX_SRV_CLIENTS) {\n\
      //no free/disconnected spot so reject\n\
      pfodServer.available().stop();\n\
    }\n\
  }\n\
  //check clients for data\n\
  for (i = 0; i < MAX_SRV_CLIENTS; i++) {\n\
    if (pfodServerClients[i] && pfodServerClients[i].connected()) {\n\
      handle_pfodParser();\n\
    }\n\
    else {\n\
      if (pfodServerClients[i]) {\n\
        pfodServerClients[i].stop();\n\
      }\n\
    }\n\
  }\n\
}\n\
\n\
// returns false if does not connect\n\
bool connectToWiFi(const char* staticIP, const char* ssid, const char* password) {\n\
  WiFi.mode(WIFI_STA);\n\
  if (staticIP[0] != '\\0') {\n\
    IPAddress ip;\n\
    bool validIp = ip.fromString(staticIP);\n\
    if (validIp) {\n\
      IPAddress gateway(ip[0], ip[1], ip[2], 1); // set gatway to ... 1\n\
      IPAddress subnet_ip = IPAddress(255, 255, 255, 0);\n\
      WiFi.config(ip, gateway, subnet_ip, dns1, dns2);\n\
    } else {\n\
      Serial.print(\"staticIP is invalid: \"); Serial.println(staticIP);\n\
      return false;\n\
    }\n\
  }\n\
  Serial.print(\"   Connecting to WiFi \");\n\
  if (WiFi.status() != WL_CONNECTED) {\n\
    WiFi.begin(ssid, password);\n\
  }\n\
  // Wait for connection for 30sec\n\
  unsigned long pulseCounter = 0;\n\
  unsigned long delay_ms = 100;\n\
  unsigned long maxCount = (30 * 1000) / delay_ms; // delay below\n\
  while ((WiFi.status() != WL_CONNECTED) && (pulseCounter < maxCount)) {\n\
    pulseCounter++;\n\
    delay(delay_ms); // often also prevents WDT timing out\n\
    if ((pulseCounter % 10) == 0) {\n\
      Serial.print(\".\");\n\
    }\n\
  }\n\
  if (pulseCounter >= maxCount) {\n\
    Serial.println(\" Failed to connect.\");\n\
    return false;\n\
  } // else\n\
  Serial.println(\" Connected.\");\n\
  return true;\n\
}\n\
\n\
// =========  End of Test Code  ==========\n\
";
  outPtr->println(str);
  return true;
}

bool CodeSupport::copyright(Print * outPtr) {
  if (!outPtr) {
    return false;
  }
  String str = "\
/*\n\
  (c)2022 Forward Computing and Control Pty. Ltd.\n\
  NSW Australia, www.forward.com.au\n\
  This code is not warranted to be fit for any purpose. You may only use it at your own risk.\n\
  This code may be freely used for both private and commercial use\n\
  Provide this copyright is maintained.\n\
*/\n";

  outPtr->print(str);
  return true;
}
