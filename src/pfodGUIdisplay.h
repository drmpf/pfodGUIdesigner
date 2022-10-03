#ifndef PFOD_GUI_DISPLAY_H
#define PFOD_GUI_DISPLAY_H
/*
  pfodGUIdisplay.h for pfodGUIdesigner
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <WiFiClient.h>

void initializeDisplay(Print *serialOut);
void connect_pfodParser(WiFiClient * client);
void handle_pfodParser();

void fullReload(); // force reload of all dwgs

void setDisplayDebug(Stream* debugOutPtr);

#endif
