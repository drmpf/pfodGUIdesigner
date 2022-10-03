#include "DwgName.h"
#include <SafeString.h>
#include <string.h>

DwgName::DwgName() {
  dwgName[0] = '\0';
}

DwgName::DwgName(const char* _name) {
  dwgName[0] = '\0';
  if (strlen(_name) < MAX_DWG_NAME_LEN) {
    cSFA(sfName, dwgName);
    sfName = _name;
  }
}
const char* DwgName::name() {
  return dwgName;
}

bool DwgName::haveName() {
  return (dwgName[0] != '\0');
}
