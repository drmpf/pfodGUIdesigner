#ifndef DWG_NAME_H
#define DWG_NAME_H
/*
   (c)2022 Forward Computing and Control Pty. Ltd.
   NSW Australia, www.forward.com.au
   This code is not warranted to be fit for any purpose. You may only use it at your own risk.
   This code may be freely used for both private and commercial use
   Provide this copyright is maintained.
*/

#include <stddef.h>

class DwgName {
  public:
  DwgName();
  DwgName(const char* _name);
  const char* name();
  bool haveName();
  static const size_t MAX_DWG_BASE_NAME_LEN = 12;
  static const size_t MAX_DWG_NAME_LEN = MAX_DWG_BASE_NAME_LEN+6+1; // allow for / ...  (999) 

 private:
  char dwgName[MAX_DWG_NAME_LEN+1]; // hold the dwg name
};

#endif
