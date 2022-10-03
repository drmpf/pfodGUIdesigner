#ifndef ITEMS_IO_SUPPORT_H
#define ITEMS_IO_SUPPORT_H
/*
   (c)2014-2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <pfodDwgs.h>
#include <SafeString.h>
#include "ValuesClass.h"
#include "ControlPanel.h"

class ItemsIOsupport {
  public:
    static bool writeItem(Print *outPtr, ValuesClass *valuesClassPtr, SafeString &sfIdxName, bool withSend = true, const char* outputPrefix = NULL); // returns false if fails
    static ValuesClass *readItem(Stream &in, int dwgSize);
    static bool readPushZero(Stream &in, float &xCenter, float &yCenter, float &scale);

    static void setDebug(Stream* debugOutPtr);
    static bool skipRoundBracketSemiColon(Stream &in);
  private:
    static bool skipRoundBracketDot(Stream &in);
    static bool processMethods(Stream &in, ValuesClass *valuesClassPtr, int dwgSize); // returns false if fails
    static bool readStringArg(Stream &in, SafeString &arg, const char delim);
    static bool readFloatArg(Stream &in, SafeString &arg, const char delim, float &result); // accept floats for all numberic args as code methods will
    static bool parseStringArg(Stream & in, SafeString & sfArg); // parses " ... ") handles \" and \\ only
    static void writeEscaped(Print &out, const char *str);
    static float limitFloat(float in, float lowerLimit, float upperLimit);
    static bool skipDot(Stream &in);
    static bool skipSemiColon(Stream & in);
    static bool skipRoundBracket(Stream &in);
};

#endif
