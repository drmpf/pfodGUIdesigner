#ifndef FILE_SUPPORT_H
#define FILE_SUPPORT_H
/*
   LittleFSsupport.cpp
   by Matthew Ford,  2021/12/06
   (c)2021-22 Forward Computing and Control Pty. Ltd.
   NSW, Australia  www.forward.com.au
   This code may be freely used for both private and commerical use.
   Provide this copyright is maintained.
*/

#include <FS.h>
#include <LittleFS.h>

/* ===================
r   Open a file for reading. If a file is in reading mode, then no data is deleted if a file is already present on a system.
r+  open for reading and writing from beginning

w   Open a file for writing. If a file is in writing mode, then a new file is created if a file doesn’t exist at all. 
    If a file is already present on a system, then all the data inside the file is truncated, and it is opened for writing purposes.
w+  open for reading and writing, overwriting a file

a   Open a file in append mode. If a file is in append mode, then the file is opened. The content within the file doesn’t change.
a+  open for reading and writing, appending to file
============== */

bool initializeFileSystem(); // returns false if fails
void listDir(const char * dirname, Print* outPtr);
void listDir(const char * dirname, uint8_t levels, Print* outPtr);
File exists(const char* path); // returns file/dir if it exists else NULL
bool createDir( const char * path);
File openFileForRewrite(const char * path);
File openFileForRead(const char *path); // need to close / delete File object should happen when File goes out of scope
size_t fileSize(const char * path);
size_t readCharFile(const char * path, char *buffer, size_t bufferSize);
bool writeCharFile(const char * path, const char *buffer, size_t len);
bool renameFile(const char * fromPath, const char * toPath); // returns false if fails
bool deleteFile(const char * path); // returns false if fails
bool deleteDir(const char *dirPath);

File openDir(const char *dirName);
File getNextDir(File dir);
const char* getPath(File f);


void listDir(const char * dirname, Print* outPtr);
void listDir(const char * dirname, uint8_t levels, Print* outPtr);
extern const char DIR_SEPARATOR[];// = "/";

void setFileSupportDebug(Print *debugPtr);
#endif
