#ifndef DWGS_LIST_H
#define DWGS_LIST_H
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodParser.h>
#include <SafeString.h>

class DwgsList {
  public:
    DwgsList(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd, const char* _touchCmd);
    bool processDwgLoad();
    bool processDwgCmds();
    const char *getDwgLoadCmd();
    void sendDrawingUpdates();
    static const size_t MAX_DWG_NAMES = 15; // the most that will fit on the screen

    void reload();
    void setDebug(Stream* debugOutPtr);


  private:
    const char* loadCmd;
    pfodDwgs *dwgsPtr;
    pfodParser *parserPtr;
    void sendDrawing(long l);
    void update();
    bool disablePanel;
    const char* touchCmd;
    bool forceReload;
    pfodAutoIdx Selection_idx;
    pfodAutoIdx closeButton_idx;
    static const char closeCmd[3];//  = "C";
    static const char disabledCmd[5];// = "n"; // disabled touch zone never sent
};

#endif
