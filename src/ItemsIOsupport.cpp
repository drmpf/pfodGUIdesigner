/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/
#include "ItemsIOsupport.h"
#include "Items.h"
#include <SafeString.h>
#include <SafeStringStream.h>

//extern Stream *debugPtr;
static Stream *debugPtr = NULL;
// for later use
void ItemsIOsupport::setDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}


// for touchAction need to handle ))
bool ItemsIOsupport::skipSemiColon(Stream & in) {
  if (in.peek() != ';') {  // expecting ; after ) no space allowed
    if (debugPtr) {
      debugPtr->print("missing ';' found "); debugPtr->println((char)in.peek());
    }
    return false;
  }
  in.read(); //skip ;
  return true;
}

// for touchAction need to handle ))
bool ItemsIOsupport::skipDot(Stream & in) {
  if (in.peek() != '.') {  // must have . after ) no space allowed
    if (debugPtr) {
      debugPtr->print("missing '.' found "); debugPtr->println((char)in.peek());
    }
    return false;
  }
  in.read(); //skip .
  return true;
}

bool ItemsIOsupport::skipRoundBracket(Stream &in) {
  //  skip )  // break if error
  cSF(sfToken, 80);
  if (!sfToken.readUntil(in, ')')) {
    if (debugPtr) {
      debugPtr->println("Missing closing ')' Input:"); debugPtr->println(sfToken);
    }
    return false;
  }
  // else full OR found )

  if (sfToken[sfToken.length() - 1] != ')') { // check first because trim() may make sfToken empty
    sfToken.trim();
    if (!sfToken.isEmpty()) {
      if (debugPtr) {
        debugPtr->print("Unexpected chars before closing ')' Input:"); debugPtr->println(sfToken);
      }
      return false;
    } else {
      if (debugPtr) {
        debugPtr->println("More then 79 whitespace chars before closing ')' Input:"); debugPtr->println(sfToken);
      }
      return false;
    }
  }
  sfToken.removeLast(1);
  sfToken.trim();
  if (!sfToken.isEmpty()) {
    if (debugPtr) {
      debugPtr->print("Unexpected chars before closing ')' Input:"); debugPtr->println(sfToken);
    }
    return false;
  }
  return true;
}

bool ItemsIOsupport::skipRoundBracketDot(Stream &in) {
  if (!skipRoundBracket(in)) {
    return false;
  }
  // else whitespace ) which is OK
  // check for following .
  if (!skipDot(in)) {
    return false;
  }
  return true; // done
}

bool ItemsIOsupport::skipRoundBracketSemiColon(Stream &in) {
  if (!skipRoundBracket(in)) {
    return false;
  }
  // else whitespace ) which is OK
  // check for following ;
  if (!skipSemiColon(in)) {
    return false;
  }
  return true; // done
}

bool ItemsIOsupport::readPushZero(Stream &in, float &xCenter, float &yCenter, float &scale) {
  cSF(sfToken, 80);
  if (!sfToken.readUntil(in, '(')) { // returns true if found token OR sfToken full else false (EOF before first ( )
    if (debugPtr) {
      debugPtr->print("Leading dwg type not found. Input:'"); debugPtr->print(sfToken); debugPtr->println("'");
    }
    return false; // could not find rectangle( .. type etc
  }
  // here either sfToken full or found delimiter
  // clear white space
  if (sfToken[sfToken.length() - 1] != '(') { // check first because trim() may make sfToken empty
    sfToken.trim();
    if (!sfToken.isEmpty()) {
      if (debugPtr) {
        debugPtr->print("Dwg type not found. Input:'"); debugPtr->print(sfToken); debugPtr->println("'");
      }
      return false; // could not find rectangle( .. type etc
    }
  } else {
    //  found (
    sfToken.trim(); // remove leading white space
    sfToken.removeLast(1); // remove trailing (
  }
  if (debugPtr) {
    debugPtr->print(" "); debugPtr->print(sfToken); debugPtr->print("() ");
  }
  pfodDwgTypeEnum type = ValuesClass::sfStrToPfodDwgTypeEnum(sfToken);
  if (type != pfodPUSH_ZERO) {
    if (debugPtr) {
      debugPtr->print("Expected pushZero but found '"); debugPtr->print(sfToken); debugPtr->println("'");
    }
    return false;
  }
  // parse 2 to 3 args
  cSF(sfArg, 80);
  if (!ItemsIOsupport::readFloatArg(in, sfArg, ',', xCenter)) {
    return false;
  }
  if (!ItemsIOsupport::readFloatArg(in, sfArg, ',', yCenter)) {
    return false; // not , or
  }
  if (!ItemsIOsupport::readFloatArg(in, sfArg, ')', scale)) {
    return false; // not , or
  }
  if (debugPtr) {
    debugPtr->print(" found pushZero("); debugPtr->print(xCenter); debugPtr->print(", ");
    debugPtr->print(yCenter); debugPtr->print(", ");
    debugPtr->print(scale); debugPtr->println(")");
  }
  return true;
}


// returns NULL if fails in which case delete the file to clean up;
ValuesClass *ItemsIOsupport::readItem(Stream & in,  int dwgSize) {
  ValuesClass *valuesClassPtr = NULL;
  pfodDwgTypeEnum type = pfodNONE;
  if (debugPtr) {
    debugPtr->print(" readItem - ");
  }

  { // this block is for sfToken, release stack at end of block
    cSF(sfToken, 80); // max text space is 64 char in valuesClassPtr->text
    // read first token
    while (1) { // loop until get first token the type
      if (!sfToken.readUntil(in, '(')) { // returns true if found token OR sfToken full else false (EOF before first ( )
        if (debugPtr) {
          debugPtr->print("Leading dwg type not found, returning NULL. Input:'"); debugPtr->print(sfToken); debugPtr->println("'");
        }
        return NULL; // could not find rectangle( .. type etc
      }
      // here either sfToken full or found delimiter
      // clear white space
      if (sfToken[sfToken.length() - 1] != '(') { // check first because trim() may make sfToken empty
        sfToken.trim();
        if (!sfToken.isEmpty()) {
          if (debugPtr) {
            debugPtr->print("Dwg type not found. Input:'"); debugPtr->print(sfToken); debugPtr->println("'");
          }
          return NULL; // could not find rectangle( .. type etc
        }
      } else {
        //  found (
        sfToken.trim(); // remove leading white space
        sfToken.removeLast(1); // remove trailing (
        break;
      }
    }
    if (debugPtr) {
      debugPtr->print("Found "); debugPtr->print(sfToken); debugPtr->print("() ");
    }
    type = ValuesClass::sfStrToPfodDwgTypeEnum(sfToken);
    if (type == pfodNONE) {
      if (debugPtr) {
        debugPtr->println("Invalid dwg type Input:"); debugPtr->println(sfToken);
      }
      return NULL;
    }
    if ((type == pfodINSERT_DWG ) || (type == pfodERASE ) || (type == pfodUNHIDE ) || (type == pfodINDEX)) {
      if (debugPtr) {
        debugPtr->println("Ignoring dwg type :"); debugPtr->println(sfToken);
      }
      return NULL;
    }

    // here have valid type
    valuesClassPtr = Items::createDwgItemValues(type); // use indexNo if found else add one later, clean up after load
    if (valuesClassPtr == NULL) {
      if (debugPtr) {
        debugPtr->println("Out of Memory");
      }
      return NULL;
    }


    Items::printDwgValues(" created : ", valuesClassPtr);

    // from here on just ignore error and use values defaults
    if (!skipRoundBracketDot(in)) {
      if (debugPtr) {
        debugPtr->print("Missing ). after "); debugPtr->println(sfToken);
      }
      return valuesClassPtr;
    }
  }
  // loop here until we find "send" or fail on error
  while (processMethods(in, valuesClassPtr, dwgSize)) {
    // loop
  }
  if (debugPtr) {
    debugPtr->println();
  }
  //Items::printDwgValues(" after processMethods ",valuesClassPtr);
  // found send( or EOF or some major parse error like embedded space
  return valuesClassPtr;
}


float ItemsIOsupport::limitFloat(float in, float lowerLimit, float upperLimit) {
  if (upperLimit < lowerLimit) { // swap
    float temp = lowerLimit;
    lowerLimit = upperLimit;
    upperLimit = temp;
  }
  if (in < lowerLimit) {
    in = lowerLimit;
  }
  if (in > upperLimit) {
    in = upperLimit;
  }
  return in;
}

// returns true if arg read as String
// this called after opening (
// parses " ... ") handles \" and \\ only
bool ItemsIOsupport::parseStringArg(Stream & in, SafeString & sfArg) {
  sfArg.clear();
  cSF(sfToken, 80);
  char delim = '"';
  sfToken.clear();
  if (!sfToken.readUntil(in, delim) && (sfToken[sfToken.length() - 1] == delim)) {
    if (debugPtr) {
      debugPtr->print("Error reading string argument. Unexpected EOF?");
      return false;
    }
    //   else {
    sfToken.removeLast(1);
    sfToken.trim();
    if (!sfToken.isEmpty()) {
      if (debugPtr) {
        debugPtr->print("Error reading string argument. Unexpected chars before opening \". Input:"); debugPtr->print(sfToken);
      }
      return false;
    }
  }
  // now search for closing " handle \\ and \"
  sfArg.clear();
  uint8_t c = '\0';
  while ( (!sfArg.isFull()) && (in.peek() != -1) && (in.peek() != 0)) { // have space and not eof and not '\0'
    uint8_t nextC = in.peek();
    c = in.read();
    if (c == '"') {
      break; // found terminating "
    }
    if ((c == '\\') && ((nextC == '\\') || (nextC == '"')) ) {
      // \ followed by \ or "
      sfArg += (char)in.read(); // skip \ add following char,
    } else {
      sfArg += (char)c; // just add it
    }
  }
  // either found " or full or eof or '\0'
  if (c != '"') {
    return false; // did not find "
  }
  // scan for )
  delim = ')';
  sfToken.clear();
  if (!sfToken.readUntil(in, delim) && (sfToken[sfToken.length() - 1] == delim)) {
    if (debugPtr) {
      debugPtr->print("Error reading string argument. Unexpected EOF?");
    }
    return false;
  }
  //   else {
  sfToken.removeLast(1);
  sfToken.trim();
  if (!sfToken.isEmpty()) {
    if (debugPtr) {
      debugPtr->print("Error reading string argument. Unexpected chars before ). Input:"); debugPtr->print(sfToken);
    }
    return false;
  }
  return true;
}


// returns true if arg read as String
bool ItemsIOsupport::readStringArg(Stream & in, SafeString & sfArg, const char delim) {
  sfArg.clear();
  if (sfArg.readUntil(in, delim) && (sfArg[sfArg.length() - 1] == delim)) {
    sfArg.removeLast(1);
    return true;
  }
  if (debugPtr) {
    debugPtr->print("Error reading text argument. Input: "); debugPtr->print(sfArg);
  }
  return false;
}

// returns true if arg read as Float, not some args are actually ints or unsigned, but
// the method calls will accept float and convert so allow floats here
bool ItemsIOsupport::readFloatArg(Stream & in, SafeString & sfArg, const char delim, float & result) {
  sfArg.clear();
  if (sfArg.readUntil(in, delim) && (sfArg[sfArg.length() - 1] == delim)) {
    sfArg.removeLast(1);
    if (sfArg.toFloat(result)) {
      return true;
    } else {
      if (debugPtr) {
        debugPtr->print("Error reading numeric argument. Input: "); debugPtr->print(sfArg);
      }
      return false;
    }
  }
  if (debugPtr) {
    debugPtr->print("Error reading numeric argument. Input: "); debugPtr->print(sfArg);
  }
  return false;
}

// returns false if fails
// don't filter on type just update value
// NOTE: if method repeated last one wins which is what pfodApp will do
bool ItemsIOsupport::processMethods(Stream & in, ValuesClass * valuesClassPtr, int dwgSize) {
  if (!valuesClassPtr) {
    return false;
  }
  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr;
  if (!valuesPtr) {
    return false;
  }

  Items::printDwgValues("  processMethods for ", valuesClassPtr);

  cSF(sfToken, 80); // max text space is 64 char in valuesClassPtr->text
  if (!sfToken.readUntil(in, '(')) { // returns true if found token OR sfToken full else false (EOF before first ( )
    if (debugPtr) {
      debugPtr->println("Missing '(',  EOF  Input : "); debugPtr->println(sfToken);
    }
    return false;
  }
  // else
  if (sfToken[sfToken.length() - 1] != '(') {
    // not found ( or more the 80 char token,  note white space not expected here
    if (debugPtr) {
      debugPtr->print("Missing '('  Input: "); debugPtr->println(sfToken);
    }
    return false;
  }
  // else
  sfToken.removeLast(1); // remove '('
  //  cmd, loadCmd, ,filter,prompt,
  //  textIdx,,action,
  //  ,text,units,floatReading,intValue,displayMax,
  //  displayMin,maxValue,minValue,,
  //
  if (debugPtr) {
    debugPtr->print("."); debugPtr->print(sfToken); debugPtr->print(" ");//debugPtr->print(" for "); debugPtr->println(ValuesClass::getTypeAsStr(valuesClassPtr->type));
  }
  // handle the two arg one first
  // size and offset
  if ((sfToken == "size") || (sfToken == "offset")) {
    cSF(sfArg1, 80);
    cSF(sfArg2, 80);
    float arg1 = 0; float arg2 = 0;
    if (!readFloatArg(in, sfArg1, ',', arg1)) {
      if (debugPtr) {
        debugPtr->print("Missing or invalid first arg for "); debugPtr->print(sfToken); debugPtr->print("  Input "); debugPtr->println(sfArg1);
      }
      return false;
    }
    if (!readFloatArg(in, sfArg2, ')', arg2)) {
      if (debugPtr) {
        debugPtr->print("Missing or invalid second arg for "); debugPtr->print(sfToken); debugPtr->print("  Input "); debugPtr->println(sfArg2);
      }
      return false;
    }
    // else limit and save
    arg1 = limitFloat(arg1, -dwgSize, dwgSize);
    arg1 = limitFloat(arg1, -dwgSize, dwgSize);
    if (sfToken == "size") {
      valuesClassPtr->setWidth(arg1);
      valuesClassPtr->setHeight(arg2);
    } else { // offset
      valuesClassPtr->setColOffset(arg1);
      valuesClassPtr->setRowOffset(arg2);
    }
    return skipDot(in);
  }

  // have done
  //  centered, filled,rounded, bold,italic,underline,
  // center,left,right, idx ,color, backgroundColor,decimals
  // encode,fontSize, radius, start,angle,
  // size,offset
  if (sfToken == "send") { // finished
    // skip );
    skipRoundBracketSemiColon(in);
    return false;
  } else if (sfToken == "centered") {
    valuesPtr->centered = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "filled") {
    valuesPtr->filled = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "rounded") {
    valuesPtr->rounded = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "bold") {
    valuesPtr->bold = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "italic") {
    valuesPtr->italic = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "underline") {
    valuesPtr->underline = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "center") {
    valuesPtr->align = 'C';
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "left") {
    valuesPtr->align = 'L';
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "right") {
    valuesPtr->align = 'R';
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  } else if (sfToken == "encode") {
    valuesPtr->encodeOutput = 1;
    if (!skipRoundBracketDot(in)) {
      return false;
    }
    return true;
  }
  if (sfToken == "action") {
    if (valuesClassPtr->type != pfodTOUCH_ACTION) {
      if (debugPtr) {
        debugPtr->print("Invalid action( .. ");  debugPtr->print(") in "); debugPtr->print(valuesClassPtr->getTypeAsStr());
      }
      return false;
    }
    cSF(sfActionStr, 512); // max text space is 64 char in valuesClassPtr->text / units so <128 for text + methods <256 so total <512
    sfActionStr.readUntil(in, "\n\r");
    sfActionStr.trim();
    if (sfActionStr.isEmpty()) {
      if (debugPtr) {
        debugPtr->println(" Invalid empty action( .. ");
      }
      return false;
    } // else
    if (debugPtr) {
      debugPtr->print(" processing sfActionStr :"); debugPtr->println(sfActionStr);
    }
    // search for ).send()
    int endIdx = sfActionStr.indexOf(").send()");
    if (endIdx < 0) {
      if (debugPtr) {
        debugPtr->print("Missing ').send()',  EOF  Input for action( : "); debugPtr->print(sfActionStr);
      }
      return false;
    }
    // else remove ).send() and add back .send() for next parse
    sfActionStr.removeFrom(endIdx);
    sfActionStr += ".send();";
    if (debugPtr) {
      debugPtr->print("Parsing action : "); debugPtr->println(sfActionStr);
    }
    // put in a SafeStringStream and pass to readItem again
    SafeStringStream sfStream(sfActionStr);
    sfStream.begin(); // all data available no baud rate delays
    if (debugPtr) {
      debugPtr->println(" calling readItem for actionAction");
    }
    valuesClassPtr->actionActionPtr = readItem(sfStream, dwgSize);
    Items::printDwgValues(" action read", valuesClassPtr->actionActionPtr);
    // update indexNo if any
    valuesClassPtr->setIndexNo(valuesClassPtr->actionActionPtr->getIndexNo());
    // set token to send and return false
    sfToken.clear();
    sfToken = "send"; // normal exit
    return false;
  }

  // else
  // all these have at least one arg
  if ((sfToken == "idx") || (sfToken == "cmd") || (sfToken == "textIdx")) {
    sfToken.trim();
    cSF(sfStrArg, 80); // max text space is 64 char in valuesClassPtr->text
    if (!readStringArg(in, sfStrArg, ')')) {
      return false;
    } else {
      sfStrArg.trim();
      // check for updateIdxPrefix/idxPrefix/cmdPrefix
      // do cmd first
      if (sfToken == "cmd") { // for touchZone, touchActionInput, touchAction
        if ((valuesClassPtr->type != pfodTOUCH_ZONE) && (valuesClassPtr->type != pfodTOUCH_ACTION) && (valuesClassPtr->type != pfodTOUCH_ACTION_INPUT)) {
          if (debugPtr) {
            debugPtr->print("Invalid cmd("); debugPtr->println(sfStrArg); debugPtr->print(") on "); debugPtr->print(valuesClassPtr->getTypeAsStr());
          }
          return false;
        }
        if (!sfStrArg.startsWith(Items::touchZoneCmdPrefix)) {
          if (debugPtr) {
            debugPtr->print("Invalid cmd("); debugPtr->print(sfStrArg); debugPtr->println(")");
          }
          return false;
        } // else
        // get cmdNo
        cSFA(sfCmdStr, valuesClassPtr->cmdStr);
        sfCmdStr.readFrom(sfStrArg.c_str()); 
        if (debugPtr) {
          debugPtr->print("sfStrArg:");debugPtr->println(sfStrArg);
        }
        sfStrArg.removeBefore(strlen(Items::touchZoneCmdPrefix));
        if (debugPtr) {
          debugPtr->print("sfStrArg after remove before :");debugPtr->println(sfStrArg);
        }
        long cmdNo = 0;
        if (!sfStrArg.toLong(cmdNo) || (cmdNo <= 0)) {
          if (debugPtr) {
            debugPtr->print("Invalid cmdNo "); debugPtr->println(sfStrArg);
          }
          return false;
        } // else
        //        Items::printDwgValues(" before setcmdNo ",valuesClassPtr);
        valuesClassPtr->setCmdNo(cmdNo);
        //        Items::printDwgValues(" after setcmdNo ",valuesClassPtr);

      } else if (sfToken == "textIdx") {
        // for touchActionInput
        if (valuesClassPtr->type != pfodTOUCH_ACTION_INPUT) { // invalid
          if (debugPtr) {
            debugPtr->print(" Invalid textIdx("); debugPtr->print(sfStrArg); debugPtr->print(") on "); debugPtr->println(valuesClassPtr->getTypeAsStr());
          }
          return false;
        } //else {
        if (sfStrArg.startsWith(Items::updateIdxPrefix)) {
          sfStrArg.removeBefore(strlen(Items::updateIdxPrefix));
          long idxNo = 0;
          if (!sfStrArg.toLong(idxNo) || (idxNo <= 0)) {
            if (debugPtr) {
              debugPtr->print("Invalid update indexNo "); debugPtr->println(sfStrArg);
            }
            return false;
          } // else
          valuesClassPtr->setIndexNo(idxNo);
          valuesClassPtr->isIndexed = true;
        } else if (sfStrArg.startsWith(Items::idxPrefix)) {
          if (debugPtr) {
            debugPtr->print(" Invalid textIdx("); debugPtr->print(sfStrArg); debugPtr->print(") on "); debugPtr->println(valuesClassPtr->getTypeAsStr());
          }
          return false;
        } else {
          if (debugPtr) {
            debugPtr->print(" Invalid textIdx("); debugPtr->print(sfStrArg); debugPtr->print(") on "); debugPtr->println(valuesClassPtr->getTypeAsStr());
          }
          return false;
        }

      } else { // idx
        if (valuesClassPtr->type == pfodTOUCH_ZONE) { // skip idx
          if (debugPtr) {
            debugPtr->print(" Skipping idx("); debugPtr->print(sfStrArg); debugPtr->print(") "); //debugPtr->println(") on touchZone()");
          }
        } //else
        if (sfStrArg.startsWith(Items::updateIdxPrefix)) {
          sfStrArg.removeBefore(strlen(Items::updateIdxPrefix));
          long idxNo = 0;
          if (!sfStrArg.toLong(idxNo) || (idxNo <= 0)) {
            if (debugPtr) {
              debugPtr->print("Invalid update indexNo "); debugPtr->println(sfStrArg);
            }
            return false;
          } // else
          valuesClassPtr->setIndexNo(idxNo);
          valuesClassPtr->isIndexed = true;
        } else if (sfStrArg.startsWith(Items::idxPrefix)) {
          if (debugPtr) {
            debugPtr->print(" Skipping idx("); debugPtr->print(sfStrArg); debugPtr->print(") starting with prefix "); debugPtr->println(Items::idxPrefix);
          }
        } else {
          if (debugPtr) {
            debugPtr->print(" Invalid idx("); debugPtr->print(sfStrArg); debugPtr->println(")");
          }
          return false;
        }

      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "color") {
    float color;
    if (!readFloatArg(in, sfToken, ')', color)) {
      return false;
    } else {
      if ((color >= 0) && (color <= 255)) {
        valuesPtr->color = color;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "backgroundColor") {
    float color;
    if (!readFloatArg(in, sfToken, ')', color)) {
      return false;
    } else {
      if ((color >= 0) && (color <= 255)) {
        valuesPtr->backgroundColor = color;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "decimals") {
    float dec;
    if (!readFloatArg(in, sfToken, ')', dec)) {
      return false;
    } else {
      if ((dec >= -6) && (dec <= 6)) {
        valuesPtr->decPlaces = dec;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if ((sfToken == "floatReading") || (sfToken == "value")) {
    float reading;
    if (!readFloatArg(in, sfToken, ')', reading)) {
      return false;
    } else {
      valuesPtr->reading = reading;
      valuesPtr->haveReading = 1;
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "radius") {
    float radius;
    if (!readFloatArg(in, sfToken, ')', radius)) {
      return false;
    } else {
      if ((radius >= -dwgSize) && (radius <= dwgSize)) {
        valuesPtr->radius = radius;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "start") {
    float start;
    if (!readFloatArg(in, sfToken, ')', start)) {
      return false;
    } else {
      if ((start >= -360) && (start <= 360)) {
        valuesPtr->startAngle = start;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "angle") {
    float angle;
    if (!readFloatArg(in, sfToken, ')', angle)) {
      return false;
    } else {
      if ((angle >= -360) && (angle <= 360)) {
        valuesPtr->arcAngle = angle;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "fontSize") {
    float fontSize;
    if (!readFloatArg(in, sfToken, ')', fontSize)) {
      return false;
    } else {
      if ((fontSize >= -6) && (fontSize <= 30)) {
        valuesPtr->fontSize = fontSize;
      } else {
        if (debugPtr) {
          debugPtr->print("Error Input : "); debugPtr->println(sfToken);
        }
      }
      if (!skipDot(in)) {
        return false;
      }
      return true;
    }
  } else if (sfToken == "text") { // does NOT handle space between ( and first "
    cSF(sfArg, ValuesClass::textSize);
    if (!parseStringArg(in, sfArg)) { // read upto the first 63 char of the text
      return false;
    } // else
    if (debugPtr) {
      debugPtr->print(" text arg Input:"); debugPtr->println(sfArg);
    }
    cSFA(sfText, valuesClassPtr->text);
    sfText = sfArg; // update text
    if (!skipDot(in)) {
      return false;
    }
    return true;

  } else if (sfToken == "prompt") { // does NOT handle space between ( and first "
    cSF(sfArg, ValuesClass::textSize);
    if (!parseStringArg(in, sfArg)) { // read upto the first 63 char of the text
      return false;
    } // else
    if (debugPtr) {
      debugPtr->print(" prompt arg Input:"); debugPtr->println(sfArg);
    }
    cSFA(sfText, valuesClassPtr->text);
    sfText = sfArg; // update text
    if (!skipDot(in)) {
      return false;
    }
    return true;

  } else if (sfToken == "units") { // does NOT handle space between ( and first "
    cSF(sfArg, ValuesClass::textSize);
    if (!parseStringArg(in, sfArg)) { // read upto the first 63 char of the text
      return false;
    } // else
    if (debugPtr) {
      debugPtr->print(" text arg Input:"); debugPtr->println(sfArg);
    }
    cSFA(sfText, valuesClassPtr->units);
    sfText = sfArg; // update text
    if (!skipDot(in)) {
      return false;
    }
    return true;
  }

  return true;
}

void ItemsIOsupport::writeEscaped(Print & out, const char *str) {
  if (!str) {
    return;
  }
  uint8_t c = '\0';
  while ((c = *str++) != 0) {
    if ((c == '\\') || (c == '"')) {
      out.print('\\');
    }
    out.write(c);
  }
}


bool ItemsIOsupport::writeItem(Print * outPtr, ValuesClass * valuesClassPtr, SafeString & sfIdxName, bool withSend, const char* statementPrefix) {
  if (!valuesClassPtr) {
    return false;
  }
  struct pfodDwgVALUES *valuesPtr = valuesClassPtr->valuesPtr;
  if (!valuesPtr) {
    return false;
  }
  pfodDwgTypeEnum type = valuesClassPtr->type;
  if (type == pfodNONE) {
    return false;
  }
  if (!outPtr) {
    return false;
  }

  if (type == pfodTOUCH_ZONE && valuesClassPtr->isIndexed && (!statementPrefix || !statementPrefix[0])) { // send place holder
    outPtr->print("touchZone().cmd(");
    outPtr->print(sfIdxName);
    outPtr->println(").send();");
  }

  if (statementPrefix) {
    outPtr->print(statementPrefix);
  }
  outPtr->print(ValuesClass::getTypeAsMethodName(valuesClassPtr->type)); outPtr->print("()");
  if (valuesClassPtr->type == pfodHIDE) {
    if (!sfIdxName.isEmpty() ) { // have idx
      outPtr->print(".idx("); outPtr->print(sfIdxName); outPtr->print(")");
    } else {
      outPtr->print(".idx(0)");
    }
    if (withSend) {
      outPtr->print(".send();");
    }
    // no send here as Hide is only an action
    return true;
  }

  if (!sfIdxName.isEmpty() ) { // have idx
    if (type != pfodTOUCH_ZONE) { // output idx if not a touchZone
      outPtr->print(".idx("); outPtr->print(sfIdxName); outPtr->print(")");
    } else {
      // touchZone
      outPtr->print(".cmd("); outPtr->print(sfIdxName); outPtr->print(")");
    }
  }
  if (type != pfodTOUCH_ZONE) { // output idx if not a touchZone
    // colour
    outPtr->print(".color(");
    if (valuesPtr->color < 0) { // not set yet use black, ignore BLACK_WHITE setting
      outPtr->print(0);       // will output black
    } else {
      outPtr->print(valuesPtr->color);       // will output black
    }
    outPtr->print(")");
  }
  outPtr->print(".offset("); outPtr->print(valuesPtr->colOffset, 2); outPtr->print(','); outPtr->print(valuesPtr->rowOffset, 2); outPtr->print(')');
  if ((type == pfodLINE) || (type == pfodTOUCH_ZONE) || (type == pfodRECTANGLE)) {
    outPtr->print(".size("); outPtr->print(valuesPtr->width, 2); outPtr->print(','); outPtr->print(valuesPtr->height, 2); outPtr->print(')');
  }
  if ((type == pfodARC) || (type == pfodCIRCLE)) {
    outPtr->print(".radius("); outPtr->print(valuesPtr->radius, 2); outPtr->print(")");
  }
  if (type == pfodARC) {
    outPtr->print(".start("); outPtr->print( valuesPtr->startAngle, 2); outPtr->print(")");
    outPtr->print(".angle("); outPtr->print(valuesPtr->arcAngle, 2); outPtr->print(")");
  }
  if ((type == pfodARC) || (type == pfodCIRCLE) || (type == pfodRECTANGLE)) {
    if (valuesPtr->filled) {
      outPtr->print(".filled()");
    }
  }
  if (type == pfodRECTANGLE) {
    if (valuesPtr->rounded) {
      outPtr->print(".rounded()");
    }
  }
  if ((type == pfodRECTANGLE) || (type == pfodTOUCH_ZONE)) {
    if (valuesPtr->centered) {
      outPtr->print(".centered()");
    }
  }
  if (type == pfodLABEL) {
    if (valuesPtr->text[0] != '\0') {
      outPtr->print(".text(\"");
      // escape \ and "
      writeEscaped(*outPtr, valuesPtr->text);
      outPtr->print("\")");
    }
    outPtr->print(".fontSize("); outPtr->print(valuesPtr->fontSize); outPtr->print(")");
    if (valuesPtr->italic) {
      outPtr->print(".italic()");
    }
    if (valuesPtr->bold) {
      outPtr->print(".bold()");
    }
    if (valuesPtr->underline) {
      outPtr->print(".underline()");
    }
    if (valuesPtr->align == 'L') {
      outPtr->print(".left()");
    } else if (valuesPtr->align == 'R') {
      outPtr->print(".right()");
    } else { // 'C'
      outPtr->print(".center()");
    }
    if (valuesPtr->haveReading) {
      outPtr->print(".value("); // or floatReading
      outPtr->print(valuesPtr->reading, 6);
      outPtr->print(")");
      outPtr->print(".decimals(");
      outPtr->print(valuesPtr->decPlaces);
      outPtr->print(")");
      outPtr->print(".units(\"");
      // escape \ and "
      writeEscaped(*outPtr, valuesPtr->units);
      outPtr->print("\")");
    }
  }
  // end of output
  if (withSend) {
    outPtr->print(".send();"); // close may be
  }

  if (type == pfodTOUCH_ZONE) { // send actions
    ValuesClass *actionClassPtr = valuesClassPtr->getActionsPtr();
    if (!actionClassPtr) {
      return true; // no actions
    }
    // else
    outPtr->println();
    while (actionClassPtr) {
      struct pfodDwgVALUES* valuesPtr = actionClassPtr->valuesPtr;
      if (actionClassPtr->type == pfodTOUCH_ACTION) {
        if (statementPrefix && (actionClassPtr->getActionActionPtr()->type == pfodHIDE) && (actionClassPtr->getIndexNo() == 0)) {
          // skip this one
        } else {
          if (statementPrefix) {
            outPtr->print(statementPrefix);
          }
          outPtr->print("touchAction()");
          outPtr->print(".cmd("); outPtr->print(sfIdxName); outPtr->print(")");
          ValuesClass *actionActionPtr = actionClassPtr->actionActionPtr;
          if (actionActionPtr) {
            outPtr->print(".action(");
            if (statementPrefix) {
              outPtr->println();
            }
            cSF(sfActionIdxName, 30);
            if (actionClassPtr->getIndexNo() != 0) {
              // need to look up that idx and see if it is indexed or just referenced
              // save current place in list
              ValuesClass *referencedPtr = Items::getReferencedPtr(actionClassPtr->getIndexNo());
              if (referencedPtr) {
                if (referencedPtr->isIndexed) {
                  sfActionIdxName = Items::updateIdxPrefix;
                  sfActionIdxName += actionClassPtr->getIndexNo();
                } else { // just referenced
                  sfActionIdxName = Items::updateIdxPrefix;
                  sfActionIdxName += actionClassPtr->getIndexNo();
//                  sfActionIdxName = Items::idxPrefix;
//                  size_t listIdx = Items::getIndexOfDwgItem(referencedPtr);
//                  sfActionIdxName += listIdx;
                }
              } else {
                if (debugPtr) {
                  Items::printDwgValues(" write: ", valuesClassPtr);
                  debugPtr->print(" actionClassPtr->getIndexNo() : "); debugPtr->print(actionClassPtr->getIndexNo()); debugPtr->println(" not found");
                }
              }
            } // else empty
            if (statementPrefix) {
              // add indent
              cSF(sfPrefix, 80);
              sfPrefix = statementPrefix;
              sfPrefix -= "  ";
              writeItem(outPtr, actionActionPtr, sfActionIdxName, false, sfPrefix.c_str()); // no send(); for action( )
            } else {
              writeItem(outPtr, actionActionPtr, sfActionIdxName, false, statementPrefix); // no send(); for action( )
            }
            if (statementPrefix) {
              outPtr->println(); outPtr->print("  ");
            }
            outPtr->print(")");
          }
          if (withSend) {
            outPtr->print(".send();"); // close may be
          }
        }
        // need to write action here
      } else if (actionClassPtr->type == pfodTOUCH_ACTION_INPUT) {
        if (statementPrefix) {
          outPtr->print(statementPrefix);
        }
        outPtr->print("touchActionInput()");
        outPtr->print(".cmd("); outPtr->print(sfIdxName); outPtr->print(")");
        outPtr->print(".backgroundColor("); outPtr->print(valuesPtr->backgroundColor); outPtr->print(")");
        outPtr->print(".color(");
        if (valuesPtr->color < 0) { // not set yet use black, ignore BLACK_WHITE setting
          outPtr->print(0);       // will output black
        } else {
          outPtr->print(valuesPtr->color);       // will output black
        }
        outPtr->print(")");
        outPtr->print(".fontSize("); outPtr->print(valuesPtr->fontSize); outPtr->print(")");
        if (valuesPtr->italic) {
          outPtr->print(".italic()");
        }
        if (valuesPtr->bold) {
          outPtr->print(".bold()");
        }
        if (valuesPtr->underline) {
          outPtr->print(".underline()");
        }
        // not for TouchActinInpt always left()
        //        if (valuesPtr->align == 'L') {
        //          outPtr->print(".left()");
        //        } else if (valuesPtr->align == 'R') {
        //          outPtr->print(".right()");
        //        } else { // 'C'
        //          outPtr->print(".center()");
        //        }
        // print prompt
        if (valuesPtr->text[0] != '\0') {
          outPtr->print(".prompt(\"");
          // escape \ and "
          writeEscaped(*outPtr, valuesPtr->text);
          outPtr->print("\")");
        }
        if (actionClassPtr->getIndexNo() != 0) {
          outPtr->print(".textIdx("); outPtr->print(Items::updateIdxPrefix); outPtr->print(actionClassPtr->getIndexNo()); outPtr->print(")");
        }
        if (withSend) {
          outPtr->print(".send();"); // close may be
        }

      } else {
        // invalid ignore
      }
      actionClassPtr = actionClassPtr->getActionsPtr(); // next one in the chain
      if (actionClassPtr) {
        outPtr->println();
      }
    }
  }

  return true;
}
