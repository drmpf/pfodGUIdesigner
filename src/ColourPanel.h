#ifndef ColourPanel_h
#define ColourPanel_h
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

//#include <pfodControl.h>
#include <pfodParser.h>
#include <SafeString.h>

// note size of main panel 0,0 to 100,100 is hard code in controlPanel
class ColourPanel {
  public:
    ColourPanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd, const char* _touchCmd);
    bool processTouchCmd(); 
    bool processDwgLoad();
    int getSelectedColor();
    const char *getDwgLoadCmd();
    void setDebug(Stream* debugOutPtr);
  private:
    void sendDrawingUpdates(); // colour panel
    void sendDrawing(long l);
    const char* loadCmd;
    const char* touchCmd;
    
    pfodDwgs *dwgsPtr;
    pfodParser *parserPtr;
    int color;
};
#endif // ColourPanel_h
