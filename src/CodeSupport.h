#ifndef CODE_SUPPORT_H
#define CODE_SUPPORT_H
/*
   (c)2014-2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodParser.h>
#include <pfodDwgs.h>
#include <SafeString.h>
#include "ValuesClass.h"
#include "pfodLinkedPointerList.h" // iterable linked list of pointers to objects


class CodeSupport {
  public:
    static void init(pfodParser *_parserPtr, Print *serialOutPtr);
    static bool writeCode(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, float xCenter, float yCenter, float scale);
    static void setDebug(Stream* debugOutPtr);
  private:
    static Print *serialOutPtr;
    static pfodParser* parserPtr;
    static bool write_H(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print *out);
    static bool write_CPP(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print *out);
    static bool writeHeader(Print *out);
    static bool writeFooter(Print* out, float xCenter, float yCenter, float scale);
    static bool outputDisplayCode(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print* outPtr, float xCenter, float yCenter, float scale);
    static bool output_dwgCmdProcessor(ValuesClass *valuesClassPtr, Print* outPtr, SafeString &sfName);
    static bool output_printDwgCmdReceived(Print * outPtr);
    static bool output_drawDwgCmds(ValuesClass *valuesClassPtr, Print* outPtr, SafeString &sfName);
    static bool output_updateDwg(ValuesClass *valuesClassPtr, Print* outPtr);
    static void clearIdxNos(); // call this before each output
    static bool writeVars(pfodLinkedPointerList<ValuesClass> *listDwgValuesPtr, Print* outPtr);
    static bool copyright(Print *outPtr);
};

#endif
