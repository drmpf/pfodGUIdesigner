#ifndef VALUES_CLASS_h
#define VALUES_CLASS_h
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

// for struct pfodDwgVALUES
#include <pfodParser.h>
#include <SafeString.h>

// NONE is the last one
// use static_cast<int>(NONE) to get last valid idx (0 to NONE) inclusive
typedef enum {pfodRECTANGLE, pfodCIRCLE, pfodLABEL, pfodTOUCH_ZONE, pfodTOUCH_ACTION, pfodTOUCH_ACTION_INPUT,
              pfodINSERT_DWG, pfodLINE, pfodARC, pfodERASE, pfodHIDE, pfodUNHIDE, pfodINDEX, pfodPUSH_ZERO, pfodPOP_ZERO,
              pfodNONE // always the last entry
             } pfodDwgTypeEnum;


class ValuesClass {
  public:
    ValuesClass();
    ~ValuesClass();

    bool hasAction1(); // only for touchZones
    bool hasAction2(); // only for touchZones
    bool hasAction3(); // only for touchZones
    bool hasAction4(); // only for touchZones
    bool hasAction5(); // only for touchZones
    bool hasAction(); // only for touchZone, true if have any action that is not an actionInput
    bool hasActionInput(); // only for touchZones
    ValuesClass *getAction1Ptr();
    ValuesClass *getAction2Ptr();
    ValuesClass *getAction3Ptr();
    ValuesClass *getAction4Ptr();
    ValuesClass *getAction5Ptr();
    ValuesClass *getActionInputPtr();
    bool appendAction(ValuesClass *actionPtr, size_t no); // returns false if did not add/update, no = 0 for touchActionInput, 1 for touchAction1, 2 for touchAction2
    bool removeAction(ValuesClass *actionPtr); // removes this ptr from the action list, returns false if failed
    ValuesClass *getActionsPtr(); // pointer to linked list of actions for a touchZone also used for linking action to action in the list
    ValuesClass *getActionActionPtr(); // return ptr to touchAction's action, or NULL if not touchAction
    void setCovered(bool _isCovered);
    bool isCovered();  // is this touchZone completely covered
    
    // set cols rows w, h rounded to 2 dec this makes checking enclosed touchzones easier/more reliable
    void setColOffset(float f);
    void setRowOffset(float f);
    void setWidth(float f);
    void setHeight(float f);

    // pfodNONE is the last one
    // use static_cast<int>(pfodNONE) to get last valid idx (0 to pfodNONE) inclusive
    pfodDwgTypeEnum type;
    const char* getTypeAsStr();
    static const char* typeToStr(pfodDwgTypeEnum type);
    static const char* getTypeAsMethodName(pfodDwgTypeEnum type);
    static pfodDwgTypeEnum sfStrToPfodDwgTypeEnum(SafeString &sfTypeStr);
    static const size_t textSize = 64;
    static const size_t cmdSize = 10;
    struct pfodDwgVALUES* valuesPtr;
    char text[textSize];
    char units[textSize];
    char cmdStr[cmdSize];
    char loadCmdStr[cmdSize];
    char touchCmdStr[cmdSize]; // link to touchZone
    bool isIndexed; // true if this item needs an update idx
    bool isReferenced; // true is this item is referenced by an action and so needs an idx
    void setNextIndexNo();
    void setIndexNo(size_t _idxNo);
    size_t getIndexNo();
    void setNextCmdNo();
    void setCmdNo(size_t _cmdNo);
    size_t getCmdNo();
    
    static size_t nextIndexNo; // for use by Items::init()
    static size_t nextCmdNo;
    ValuesClass *actionActionPtr; // for touchAction only points to action

  private:
    ValuesClass *actionValuesClassPtr; // initially NULL
    ValuesClass *action_1Ptr; // initially NULL
    ValuesClass *action_2Ptr; // initially NULL
    ValuesClass *action_3Ptr; // initially NULL
    ValuesClass *action_4Ptr; // initially NULL
    ValuesClass *action_5Ptr; // initially NULL
    ValuesClass *action_Input_Ptr; // initially NULL
    bool covered; // if touchZone completely covered by another touchZone
    static const size_t numberOfpfodDwgTypeEnums = static_cast<int>((pfodDwgTypeEnum)pfodNONE) + 1;
    static const char typeEnumNames[numberOfpfodDwgTypeEnums][25];
    static const char typeEnumOutputNames[numberOfpfodDwgTypeEnums][25];
    // variables for handling index references
    //size_t actionRefNo; // for touchActions and touchActionInputs indexNo is used to ref dwg Item
    size_t indexNo; // the trailing number from _update_idx_ found on load.
    size_t cmdNo; // for touchZone cmds 
};

#endif
