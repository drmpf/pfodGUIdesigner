/*
   Filesupport.cpp
   by Matthew Ford,  2021/12/06
   (c)2021-22 Forward Computing and Control Pty. Ltd.
   NSW, Australia  www.forward.com.au
   This code may be freely used for both private and commerical use.
   Provide this copyright is maintained.

*/

#include "FileSupport.h"

#define FORMAT_IF_FAILED true
const char DIR_SEPARATOR[] = "/";
static Stream *debugPtr = NULL;

void setFileSupportDebug(Stream* debugOutPtr) {
  debugPtr = debugOutPtr;
}

static bool fileSystemInitialized = false;
/* ===================
  r   Open a file for reading. If a file is in reading mode, then no data is deleted if a file is already present on a system.
  r+  open for reading and writing from beginning

  w   Open a file for writing. If a file is in writing mode, then a new file is created if a file doesn’t exist at all.
    If a file is already present on a system, then all the data inside the file is truncated, and it is opened for writing purposes.
  w+  open for reading and writing, overwriting a file

  a   Open a file in append mode. If a file is in append mode, then the file is opened. The content within the file doesn’t change.
  a+  open for reading and writing, appending to file
  ============== */

bool initializeFileSystem() {
  fs::FS fs = LittleFS;

  if (fileSystemInitialized) {
    return fileSystemInitialized;
  }

  if (debugPtr) {
    debugPtr->print("Mount File System");
  }
  // esp32
#if defined (ESP32)
  if (!LittleFS.begin(true)) { // for ESP32 if(!LittleFS.begin(true)) FORMAT_LITTLEFS_IF_FAILED
#elif defined (ESP8266)
  if (!LittleFS.begin()) {
#else
#error "Only ESP32 and ESP8266 supported"
#endif
    if (debugPtr) {
      debugPtr->println(" - mount failed");
    }
    return fileSystemInitialized;
  }
  if (debugPtr) {
    debugPtr->println(" mounted");
  }
  // else
  fileSystemInitialized = true;
  listDir(DIR_SEPARATOR, debugPtr);
  return fileSystemInitialized;
}

void listDir(const char * dirname, Print* outPtr) {
  listDir(dirname, uint8_t(-1), outPtr);
}

/**
  openDir
  returns dir (File), caller needs to close
  
  params
  dirName - if this is a directory open it, else return null
  
*/
File openDir(const char *dirName) {
  File emptyRtn;
  fs::FS fs = LittleFS;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return emptyRtn;
  }

  File root = fs.open(dirName,"r");
  if (!root) {
    if (debugPtr) {
      debugPtr->println("- failed to open directory");
    }
    return emptyRtn;
  }
  if (!root.isDirectory()) {
    if (debugPtr) {
      debugPtr->println("- not a directory");
    }
    root.close();
    return emptyRtn;
  }
  return root;
}

const char* getPath(File file) {
#if defined (ESP32)
     return file.path();
#elif defined (ESP8266)
     return file.fullName();
#else
#error "Only ESP32 and ESP8266 supported"
#endif
}

/**
 getNextDir
 returns next dir as file (opened), caller needs to close after use, unopened file returned if none found
 
 params
 dir - root dir to scan
*/
File getNextDir(File dir) {
  File file;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return file;
  }
  if (!dir.isDirectory()) {
  	return file;
  }
  file = dir.openNextFile();
  while(file) {
   if (file.isDirectory()) {
     return file;
   } // else
   file = dir.openNextFile();
  }
  return file;
}
	
void listDir(const char * dirname, uint8_t levels, Print* outPtr) {
  fs::FS fs = LittleFS;
//  if (!outPtr) {
//    return;
//  }
  if (!fileSystemInitialized) {
    if (outPtr) {
      outPtr->println("FS not initialized yet");
    }
    return;
  }
  if (outPtr) {
    outPtr->print("++ Listing directory: "); outPtr->println(dirname);
  }

  File root = fs.open(dirname,"r");
  if (!root) {
    if (outPtr) {
      outPtr->println("- failed to open directory");
    }
    return;
  }
  if (!root.isDirectory()) {
    if (outPtr) {
      outPtr->println("- not a directory");
    }
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      if (outPtr) {
        outPtr->print("  DIR : ");
        outPtr->println(file.name());
      }
      if (levels) {
#if defined (ESP32)
        listDir(file.path(), levels - 1, outPtr);
#elif defined (ESP8266)
        listDir(file.fullName(), levels - 1, outPtr);
#else
#error "Only ESP32 and ESP8266 supported"
#endif
      }
     if (outPtr) {
        outPtr->print("-- "); outPtr->println(getPath(file)); 
     }

    } else {
      if (outPtr) {
        outPtr->print("  FILE: ");
        outPtr->print(file.name());
        outPtr->print("\tSIZE: ");
        outPtr->println(file.size());
      }
    }
    file = root.openNextFile();
  }
}
/**
 *   //#if defined (ESP32)
  // ESP32

  #elif defined (ESP8266)
  // ESP8266
  Dir root = LittleFS.openDir(dirname);
  while (root.next()) {
    File file = root.openFile("r");
    out.print("  FILE: ");
    out.print(root.fileName());
    out.print("  SIZE: ");
    out.println(file.size());
    file.close();
  }
  out.println();
  }
  #endif
**/

/**
   exists
   returns file/dir if it exists else NULL
   params
   path - the pathname to check 
*/
File exists(const char* path) {
  fs::FS fs = LittleFS;
  File file;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return file;
  }
  if (debugPtr) {
    debugPtr->print("Opening file for read : "); debugPtr->println(path);
  }
  file = fs.open(path, "r");
  return file; 
}	

// if (!file) returned then open failed
File openFileForRead(const char * path) {
  fs::FS fs = LittleFS;
  File file;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return file;
  }
  if (debugPtr) {
    debugPtr->print("Opening file for read : "); debugPtr->println(path);
  }
  file = fs.open(path, "r");
  if ((!file) || file.isDirectory()) {
    if (debugPtr) {
      debugPtr->println(" - failed to open file for reading");
    }
    file.close(); //
  }
  return file;
}

bool createDir( const char * path) {
  fs::FS fs = LittleFS;
  File file;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return false;
  }
  if (debugPtr) {
    debugPtr->print("createDir "); debugPtr->println(path);
  }
  if(!fs.mkdir(path)){
   if (debugPtr) {
     debugPtr->println("createDir failed ");
   }
   return false;
  }
  return true;
}

// if (!file) returned then open failed
File openFileForRewrite(const char * path) {
  fs::FS fs = LittleFS;
  File file;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return file;
  }
  if (debugPtr) {
    debugPtr->print("Opening file for rewriting : "); debugPtr->println(path);
  }
  file = fs.open(path, "w");
  if ((!file) || file.isDirectory()) {
    if (debugPtr) {
      debugPtr->println(" - failed to open file for rewriting");
    }
    file.close(); //
  }
  return file;
}

// read at most bufferSize-1 chars and add terminating '\0'
size_t readCharFile(const char * path, char *buffer, size_t bufferSize) {
  fs::FS fs = LittleFS;
  if (!buffer) {
    return 0;
  }
  if (bufferSize == 0) {
    return 0;
  }
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    buffer[0] = '\0';
    return 0;
  }
  if (debugPtr) {
    debugPtr->print("Reading file: "); debugPtr->print(path);
  }

  File file = fs.open(path,"r");
  if (!file || file.isDirectory()) {
    if (debugPtr) {
      debugPtr->println(" - failed to open file for writing");
    }
    return 0;
  }

  size_t charsRead =  file.readBytes(buffer, bufferSize - 1); // leave space for '\0'
  buffer[charsRead] = '\0'; // always terminate
  if (debugPtr) {
    debugPtr->print(" - charsRead:"); debugPtr->println(charsRead);
  }
  return charsRead;
}


size_t fileSize(const char * path) {
  fs::FS fs = LittleFS;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return 0;
  }
  if (debugPtr) {
    debugPtr->print("Getting size of file: "); debugPtr->print(path);
  }
  File file = fs.open(path,"r");
  if ((!file) || file.isDirectory()) {
    if (debugPtr) {
      debugPtr->println(" - failed to open file");
    }
    return 0;
  }
  size_t fSize = file.size();
  if (debugPtr) {
    debugPtr->print(" = "); debugPtr->println(fSize);
  }
  return fSize;
}
bool writeCharFile(const char * path, const char *buffer, size_t len) {
  fs::FS fs = LittleFS;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return false;
  }
  if (debugPtr) {
    debugPtr->print("Writing file: "); debugPtr->print(path);
  }
  File file = fs.open(path, "w");
  if (!file || file.isDirectory()) {
    if (debugPtr) {
      debugPtr->println(" - failed to open file for writing");
    }
  }
  return false;

  size_t bytesWritten = file.write((uint8_t*)buffer, len);
  if (bytesWritten == len) {
    if (debugPtr) {
      debugPtr->println(" - file written");
    }
    return true;
  } // else
  if (debugPtr) {
    debugPtr->print(" - file write failed, len "); debugPtr->print(len); debugPtr->print("!="); debugPtr->print(bytesWritten); debugPtr->println(" written.");
  }
  return false;
}


bool renameFile(const char * fromPath, const char * toPath) {
  fs::FS fs = LittleFS;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return false;
  }
  if (debugPtr) {
    debugPtr->print("Renaming file "); debugPtr->print(fromPath); debugPtr->print(" to "); debugPtr->print(toPath);
  }
  if (fs.rename(fromPath, toPath)) {
    if (debugPtr) {
      debugPtr->println("- file renamed");
    }
    return true;
  }
  // else {
  if (debugPtr) {
    debugPtr->println("- rename failed");
  }
  return false;
}

bool deleteDir(const char *dirPath) {
  fs::FS fs = LittleFS;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return false;
  }
  if (debugPtr) {
    debugPtr->print("Removing Dir: "); debugPtr->print(dirPath);
  }
  if(!fs.rmdir(dirPath)){
    if (debugPtr) {
      debugPtr->println("- delete failed");
    }
    return false;
  } 
  // else
  if (debugPtr) {
    debugPtr->println("- dir deleted");
  }
  return true;
}

bool deleteFile(const char * path) {
  fs::FS fs = LittleFS;
  if (!fileSystemInitialized) {
    if (debugPtr) {
      debugPtr->println("FS not initialized yet");
    }
    return false;
  }
  if (debugPtr) {
    debugPtr->print("Deleting file: "); debugPtr->print(path);
  }
  if (!fs.remove(path)) {
    if (debugPtr) {
      debugPtr->println("- delete failed");
    }
    return false;
  }
  // else {
  if (debugPtr) {
    debugPtr->println("- file deleted");
  }
  return true;
}
