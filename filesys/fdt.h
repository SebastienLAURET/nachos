#ifdef USER_PROGRAM
// $Id: fdt.h,v 1.1.1.1 2006/02/20 14:32:11 caveenj Exp $ 
//
// This file contains the definition of the FDTEntry structure. The entries in
// the file descriptor tables of threads are simply instances of this 
// structure.
//
// Authors: Matt Peters, Robert Hill, Doug McClendon, Shyam Pather

#ifndef __FDT_H
#define __FDT_H

#include "openfile.h"

// FileDescriptorType is an enumeration that will be used in the FDTEntry to 
// distinguish different types of files (eg the Console input file descriptor
// is different from a file descriptor referring to a disk file).

enum FileDescriptorType { DiskFile, ConsoleFile };

typedef struct FDTEntry {
  FileDescriptorType type;
  OpenFile *openfile;  // If this entry is a disk file, 
                       // this should be non-NULL.
};


#endif // __FDT_H
#endif // USER_PROGRAM
