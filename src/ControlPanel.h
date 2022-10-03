#ifndef ControlPanel_h
#define ControlPanel_h
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodParser.h>
#include <SafeString.h>

class ControlPanel {
  public:
    ControlPanel(pfodParser *_parserPtr, pfodDwgs *_dwgsPtr, const char* _loadCmd);
    bool processDwgLoad();
    bool processDwgCmds();
    const char *getDwgLoadCmd();
    //size_t getDwgItemIdx(); // call if isDwgSelectedItemIdxUpdated returns true to update item deleted/moved 
    void setDwgSize(unsigned int size);
    static int getDwgSize();
    bool getShowColorPanelFlag();
    void setShowColorPanelFlag(bool flag);
    bool getShowChoicePanelFlag();       // should move this to Items:: and iterlock with add dwg items
    static bool showTouchZoneOnChoicePanel(); // should move this to Items::  and iterlock with add dwg items
    bool getShowBackgroundColorPanelFlag(); // flag cleared on each call
    void setDebug(Stream* debugOutPtr);
    void setErrMsg(char *msg);
    const char* getFontSizeSliderCmd();
    void disable(bool flag);
    static void reload();


  private:
    const char* loadCmd;
    pfodDwgs *dwgsPtr;
    pfodParser *parserPtr;
    void sendDrawingUpdates();
    void sendDrawing(long l);
    void update();
    static bool forceReload;

    pfodAutoIdx xLabel_idx;
    pfodAutoIdx yLabel_idx;
    pfodAutoIdx rLabel_idx; // radius
    pfodAutoIdx txLabel_idx; // idx for current label text
    pfodAutoIdx fsLabel_idx; // idx for current label text

    pfodAutoIdx IdxbgLabel_idx; // idx backgroung
    pfodAutoIdx IdxLabel_idx;  // idx 

    pfodAutoIdx FbgLabel_idx; // filled backgroung
    pfodAutoIdx FLabel_idx;  // filled
    pfodAutoIdx CbgLabel_idx; // centered backgroung
    pfodAutoIdx CLabel_idx;  // centered
    pfodAutoIdx RbgLabel_idx; // rounded backgroung
    pfodAutoIdx RLabel_idx;  // rounded

    pfodAutoIdx LBbgLabel_idx; // Bold background
    pfodAutoIdx LBLabel_idx; // Bold
    pfodAutoIdx LIbgLabel_idx; // Italic background
    pfodAutoIdx LILabel_idx; // Italic
    pfodAutoIdx LUbgLabel_idx; // Underline background
    pfodAutoIdx LULabel_idx; // Underline

    pfodAutoIdx AbgLabel_idx; // Aligh background
    pfodAutoIdx ALLabel_idx; // Align L
    pfodAutoIdx ACLabel_idx; // Align C
    pfodAutoIdx ARLabel_idx; // Align R
    pfodAutoIdx ARectLabel_idx; // align rect
    
    pfodAutoIdx LVLabel_idx; // value label 
    pfodAutoIdx fullLVLabel_idx; // full value 
    
    pfodAutoIdx LDecLabel_idx; // decimals label
    pfodAutoIdx LuLabel_idx; // unit label hidden
    pfodAutoIdx X1_Label_idx; // cross out dec units
    pfodAutoIdx X2_Label_idx; // cross out dec units
    
    pfodAutoIdx wLabel_idx;
    pfodAutoIdx hLabel_idx;
    pfodAutoIdx SLabel_idx; // start
    pfodAutoIdx ALabel_idx; // angle
    pfodAutoIdx DLabel_idx; // Delete
    pfodAutoIdx DLabelY_N_idx; // Delete Y/N
    pfodAutoIdx CloseLabel_idx; // Close
    pfodAutoIdx colLabelRect_idx; // color rect
    pfodAutoIdx colLabelRectOutline_idx; // color rect outline
    pfodAutoIdx colLabel_idx; // color label
    
    pfodAutoIdx backgroundColLabelRect_idx;
    pfodAutoIdx backgroundColLabelRectOutline_idx;
    pfodAutoIdx backgroundColLabel_idx;

    pfodAutoIdx cLabel_idx;

    pfodAutoIdx Tabg1Label_idx; // TouchAction background
    pfodAutoIdx Ta1Label_idx; // TouchAction 
    
    pfodAutoIdx Tabg2Label_idx; // TouchAction background
    pfodAutoIdx Ta2Label_idx; // TouchAction
    
    pfodAutoIdx Tabg3Label_idx; // TouchAction background
    pfodAutoIdx Ta3Label_idx; // TouchAction
    
    pfodAutoIdx Tabg4Label_idx; // TouchAction background
    pfodAutoIdx Ta4Label_idx; // TouchAction
    
    pfodAutoIdx Tabg5Label_idx; // TouchAction background
    pfodAutoIdx Ta5Label_idx; // TouchAction
     
    pfodAutoIdx TibgLabel_idx; // TouchInput background
    pfodAutoIdx TiLabel_idx; // TouchInput 

    pfodAutoIdx TiPromptLabel_idx; // TouchInput prompt 
    pfodAutoIdx TiPromptShortLabel_idx; // TouchInput prompt shortened 
    pfodAutoIdx shortDefaultInputLabel_idx; // update from dwgPanel if is a label
    static const char TiPCmd[4];//  = "TiP";
    
    static const char xPosCmd[3];//  = "cx";
    static const char yPosCmd[3];//  = "cy";
    static const char rCmd[3];//  = "cr"; // radius
    static const char wCmd[3];//  = "cw";
    static const char hCmd[3];//  = "ch";
    static const char colorCmd[3];// = "cc";
    static const char backgroundColorCmd[3];// = "cb";
    static const char textEditCmd[3];// = "ce";
    static const char fontSizeCmd[3];// = "cs";
    static const char IdxCmd[3];//  = "cI"; // idx
    static const char FCmd[3];//  = "cF"; // filled
    static const char CCmd[3];// = "cC"; // center
    static const char RCmd[3];// = "cR"; // rounded
    static const char SCmd[3];//  = "cS"; // start
    static const char ACmd[3];//  = "cA"; // angle
    static const char DCmd[3];// = "cD"; // delete
    static const char CloseCmd[3];// = "ok"; // close
    static const char LBCmd[3];// = "LB"; // label bold
    static const char LICmd[3];// = "LI"; // label italic
    static const char LUCmd[3];// = "LU"; // label underline
    static const char LACmd[3];// = "LA"; // label align
    static const char LVCmd[3];// = "LV"; // label value
    static const char LDCmd[3];// = "LD"; // label decimals
    static const char LuCmd[3];// = "Lu"; // label units
    
    static const char Ta1Cmd[4];// = "Ta1"; // touchAction
    static const char Ta2Cmd[4];// = "Ta2"; // touchAction
    static const char Ta3Cmd[4];// = "Ta3"; // touchAction
    static const char Ta4Cmd[4];// = "Ta4"; // touchAction
    static const char Ta5Cmd[4];// = "Ta5"; // touchAction
    static const char TiCmd[3];// = "Ti"; // touchInput

    static const char disabledCmd[5];//  = "none";
    bool showColorPanel;
    bool showBackgroundColorPanel; // set true for background color cleared on get by display.cpp

    static int dwgSize;
    bool disablePanel;

    char errMsg[51];
    float limitFloat(float in, float lowerLimit, float upperLimit);
};
#endif // ControlPanel_h
