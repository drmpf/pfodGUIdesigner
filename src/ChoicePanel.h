#ifndef ChoicePanel_h
#define ChoicePanel_h
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodParser.h>
#include <SafeString.h>
#include "ValuesClass.h"

// note size of main panel 0,0 to 100,100 is hard code in controlPanel
class ChoicePanel {
  public:
    ChoicePanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd);
    bool processDwgLoad();
    bool processDwgCmds();
    bool processSaveAsCmd();
    bool processGenerateCmd();
    bool processOpenCmd();
    bool processNewCmd();
    bool processDwgDelete();
    void reload();


    const char *getDwgLoadCmd();
    void setDebug(Stream* debugOutPtr);
    void setErrMsg(char *msg);
    pfodDwgTypeEnum getSelectedDwgItemType();
    void disable(bool flag);
   // void setSelectedDwgItemType(pfodDwgTypeEnum _type);

  private:
    pfodDwgTypeEnum selectedDwgItemType; // the item to display
    bool forceReload; // = true; // for testing force complete reload on every reboot

    const char* loadCmd;
    pfodDwgs *dwgsPtr;
    pfodParser *parserPtr;
    void sendDrawingUpdates();
    void sendDrawing(long l);
    void update();

    pfodAutoIdx rect_idx;
    pfodAutoIdx line_idx;
    pfodAutoIdx circle_idx;
    pfodAutoIdx arc_idx;
    pfodAutoIdx label_idx;
    pfodAutoIdx touchZone_idx;
    pfodAutoIdx touchZoneRectangle_idx;
    pfodAutoIdx generateCode_idx;
    pfodAutoIdx saveAs_idx;
    pfodAutoIdx open_idx;
    pfodAutoIdx new_idx;
    pfodAutoIdx dwgName_idx;
    pfodAutoIdx dwgNameHidden_idx;
    pfodAutoIdx deleteLabel_idx;
    pfodAutoIdx deleteLabelY_N_idx; // Delete Y/N
    pfodAutoIdx deleteLabelRectangle_idx;

    
    static const char rectCmd[3];//  = "r";
    static const char lineCmd[3];//  = "l";
    static const char circleCmd[3];//  = "c";
    static const char arcCmd[3];//  = "a";
    static const char labelCmd[3];//  = "L";
    static const char touchZoneCmd[3];//  = "tZ";
    static const char disabledCmd[5];//  = "n";
    static const char generateCodeCmd[3];//  = "cg";
    static const char saveAsCmd[3]; // = "sA"; 
    static const char openCmd[3]; // = "O"; 
    static const char newCmd[3];// = "N"; 
    static const char deleteCmd[3];// = "D"; // delete


    pfodAutoIdx errMsg_idx;
    bool showColorPanel;
    bool disablePanel;

    char errMsg[51];
    float limitFloat(float in, float lowerLimit, float upperLimit);
};
#endif // ChoicePanel_h
