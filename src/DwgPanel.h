#ifndef DwgPanel_h
#define DwgPanel_h
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
//#include "pfodLinkedPointerList.h" // iterable linked list of pointers to objects

// note size of main panel 0,0 to 100,100 is hard code in controlPanel
class DwgPanel {
  public:
    DwgPanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd);
    bool processDwgCmds();
    bool processItemShowCmd();
    bool processDwgLoad();
    bool posUpdated();
    const char *getDwgLoadCmd();
    bool isAdjustCenter(); // true if adjusting zero // disable controlpane and item choice
    void setShowMoveTouch(bool flag);
    void setDebug(Stream* debugOutPtr);
    void setDwgSize(int size);
    float getXPos();
    float getYPos();
    static float getXCenter();
    static float getYCenter();
    static float getScale();
    static void setZero(float _xCenter, float _yCenter, float _scale);
    static void reload();
    static const char showSelectedItemCmd[5];// =  "iS";


  private:
    void sendDrawingUpdates(); // colour panel
    void sendDrawing(long l);
    void update();
    const char* loadCmd;
    static const char posCmd[5];//  = "cP";
    static const char disabledCmd[5];//  = "n";
    static const char touchCmd[5];// =  "tP";
    static const char itemUpCmd[5];// =  "iU";
    static const char itemDownCmd[5];// =  "iD";
    static const char itemForwardCmd[5];// =  "iF";
    static const char itemBackwardCmd[5];// =  "iB";
    bool moveTouch;
    static int dwgSize;
    bool sendDwgItems(size_t startIdx, size_t endIdx);
    static pfodDwgs *dwgsPtr;
    static pfodParser *parserPtr;
    float xPos; float yPos;
    static float scale;
    static float xCenter; static float yCenter;
    static float limitFloat(float in, float lowerLimit, float upperLimit);
    void addDwgControls_1();
    void addDwgControls_2();
    bool adjustCenter;
    static bool forceReload;
    void hideUpDownOnTouch(const char* _cmd);
    void hideLayerOnTouch(const char* _cmd);
    pfodAutoIdx posXLabel_idx;
    pfodAutoIdx posYLabel_idx;
};
#endif // DwgPanel_h
