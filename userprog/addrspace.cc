// addrspace.cc
//	Routines to manage address spaces (executing user programs).
//
// Originally Copyright (c) 1992-1993 The Regents of the University of
// California.  All rights reserved.  See copyright.h for copyright notice and
// limitation of liability and disclaimer of warranty provisions.
//
// However, I'm not sure that any of the original UC code is left.  This file
// was rewritten to provide native COFF (instead of NOFF) support, then
// rewritten again to provide ELF support, both by Jerry James.  Most likely,
// this file is now entirely copyright 2004 University of Kansas (but I'm not
// sure).

#pragma implementation "userprog/addrspace.h"

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "nerrno.h"
#include "memmgr.h"
#include "../threads/utility.h"

#define SWAPSHORT(x) x = ShortToHost (x)
#define SWAPWORD(x) x = WordToHost (x)

//----------------------------------------------------------------------
// SwapSection
// 	Do little endian to big endian conversion on the bytes in a
//	section header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
static void
SwapSection (Elf32_Section_header *section)
{
  SWAPWORD (section->sh_name);
  SWAPWORD (section->sh_type);
  SWAPWORD (section->sh_flags);
  SWAPWORD (section->sh_addr);
  SWAPWORD (section->sh_offset);
  SWAPWORD (section->sh_size);
  SWAPWORD (section->sh_link);
  SWAPWORD (section->sh_info);
  SWAPWORD (section->sh_addralign);
  SWAPWORD (section->sh_addralign);
  SWAPWORD (section->sh_entsize);
}

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------
static void
SwapElfHeader (NachosHeader *nachosH)
{
  SWAPSHORT (nachosH->file_header.e_type);
  SWAPSHORT (nachosH->file_header.e_machine);
  SWAPWORD  (nachosH->file_header.e_version);
  SWAPWORD  (nachosH->file_header.e_entry);
  SWAPWORD  (nachosH->file_header.e_phoff);
  SWAPWORD  (nachosH->file_header.e_shoff);
  SWAPWORD  (nachosH->file_header.e_flags);
  SWAPSHORT (nachosH->file_header.e_ehsize);
  SWAPSHORT (nachosH->file_header.e_phentsize);
  SWAPSHORT (nachosH->file_header.e_phnum);
  SWAPSHORT (nachosH->file_header.e_shentsize);
  SWAPSHORT (nachosH->file_header.e_shnum);
  SWAPSHORT (nachosH->file_header.e_shstrndx);
}

/* FIXME : SUMMARY - Throw some error instead of exiting
  In each of the places exit(1) is called within ReadHeaders routine,
  throw an exception instead.
  Throw an exception if there's some problem with the executable
  Advantages of throwing exception:
	a) If user program has been invoked under nachos shell,
	   then after an exception is thrown, user gets
	   the nachos shell back.
	b) whenever nachos user programs are invoked,
	   both under the nachos shell and without shell invocation,
	   throwing exception ensures proper termination
	   and relevant error message to the user.
*/

//----------------------------------------------------------------------
// ReadHeaders
// 	Read in the ELF header and the section headers of an executable
// 	file.
//----------------------------------------------------------------------
static unsigned int
ReadHeaders (OpenFile *executable, NachosHeader *nachosH)
{
  size_t offset;
  char *string_table;
  Elf32_Section_header header;

  // Get the file and a.out headers
  executable->ReadAt (nachosH, sizeof (Elf32_Elf_header), 0);
  SwapElfHeader (nachosH);

  // Check the magic number for this file
  if (nachosH->file_header.e_ident[EI_MAG0] != ELFMAG0 ||
      nachosH->file_header.e_ident[EI_MAG1] != ELFMAG1 ||
      nachosH->file_header.e_ident[EI_MAG2] != ELFMAG2 ||
      nachosH->file_header.e_ident[EI_MAG3] != ELFMAG3)
    {
      fputs ("Nachos can only execute MIPS ELF executables\n", stderr);
/* FIXME: Throw some kind of error instead of exiting --
 	  Will be useful when command is run under Nachos shell instead
	  of terminating the Nachos process itself, throw an error */
      exit (1);
    }

  if (nachosH->file_header.e_ident[EI_CLASS] != ELFCLASS32)
    {
      fputs ("Nachos can only execute 32-bit MIPS ELF executables\n", stderr);
/* FIXME: Throw some kind of error instead of exiting --
          Will be useful when command is run under Nachos shell instead
          of terminating the Nachos process itself, throw an error */
	exit (1);
    }

  if (nachosH->file_header.e_ident[EI_DATA] != ELFDATA2LSB)
    {
      fputs ("Nachos can only execute little endian MIPS ELF executables\n",
	     stderr);
 /* FIXME: Throw some kind of error instead of exiting --
	   Will be useful when command is run under Nachos shell instead
	   of terminating the Nachos process itself, throw an error */
      exit (1);
    }

  if (nachosH->file_header.e_ident[EI_VERSION] != 1)
    {
      fputs ("Nachos can only execute verion 1 MIPS ELF executables\n",
	     stderr);
 /* FIXME: Throw some kind of error instead of exiting --
           Will be useful when command is run under Nachos shell instead
           of terminating the Nachos process itself, throw an error */
      exit (1);
    }

  if (nachosH->file_header.e_ident[EI_OSABI] != ELFOSABI_NONE)
    {
      fputs ("Nachos can only execute SysV ABI MIPS ELF executables\n",
	     stderr);
 /* FIXME: Throw some kind of error instead of exiting --
           Will be useful when command is run under Nachos shell instead
           of terminating the Nachos process itself, throw an error */
      exit (1);
    }

  if (nachosH->file_header.e_ident[EI_ABIVERSION] != 0)
    {
      fputs ("Nachos can only execute SysV ABI version 0 executables\n",
	     stderr);
 /* FIXME: Throw some kind of error instead of exiting --
           Will be useful when command is run under Nachos shell instead
           of terminating the Nachos process itself, throw an error */

      exit (1);
    }

  for (register int i = EI_PAD; i < 16; i++)
    {
      if (nachosH->file_header.e_ident[i] != 0)
	{
	  fputs ("File header corrupted.\n", stderr);
 /* FIXME: Throw some kind of error instead of exiting --
           Will be useful when command is run under Nachos shell instead
           of terminating the Nachos process itself, throw an error */
      exit (1);
	}
    }

  // Make sure the file is not an object file, core file, etc.
  if (nachosH->file_header.e_type != ET_EXEC)
    {
      fputs ("This file is not executable.\n", stderr);
      exit (1);
    }

  // Make sure this is a MIPS file
  if (nachosH->file_header.e_machine != EM_MIPS)
    {
      fputs ("Nachos can only execute MIPS ELF executables\n", stderr);
      exit (1);
    }

  // Check the object file version
  if (nachosH->file_header.e_version != EV_CURRENT)
    {
      fprintf (stderr,
	       "Nachos can only execute version %d MIPS ELF executables\n",
	       EV_CURRENT);
      exit (1);
    }

  // Get the section name string table
  executable->ReadAt (&header, sizeof (header),
		      nachosH->file_header.e_shoff +
		      nachosH->file_header.e_shstrndx *
		      nachosH->file_header.e_shentsize);
  SwapSection (&header);
  string_table = new char[header.sh_size];
  executable->ReadAt (string_table, header.sh_size, header.sh_offset);

  // Get the section headers
  offset = nachosH->file_header.e_shoff;
  memset (&nachosH->section, 0, sizeof (nachosH->section));
  for (register uint16_t i = 0U; i < nachosH->file_header.e_shnum; i++)
    {
      executable->ReadAt (&header, sizeof (header), offset);
      offset += nachosH->file_header.e_shentsize;
      SwapSection (&header);
      if (strcmp (&string_table[header.sh_name], ".reginfo") == 0)
	memcpy (&nachosH->section[REGINFO], &header,
		nachosH->file_header.e_shentsize);
      else if (strcmp (&string_table[header.sh_name], ".text") == 0)
	memcpy (&nachosH->section[TEXT], &header,
		nachosH->file_header.e_shentsize);
      else if (strcmp (&string_table[header.sh_name], ".rodata") == 0)
	memcpy (&nachosH->section[RDATA], &header,
		nachosH->file_header.e_shentsize);
      else if (strcmp (&string_table[header.sh_name], ".data") == 0)
	memcpy (&nachosH->section[DATA], &header,
		nachosH->file_header.e_shentsize);
      else if (strcmp (&string_table[header.sh_name], ".bss") == 0)
	memcpy (&nachosH->section[BSS], &header,
		nachosH->file_header.e_shentsize);
      else if (strcmp (&string_table[header.sh_name], ".sbss") == 0)
	memcpy (&nachosH->section[SBSS], &header,
		nachosH->file_header.e_shentsize);
      //FIXME :  else it is some section we know nothing about.
    }

  delete [] string_table;

  // Return the start address
  return nachosH->file_header.e_entry;
}

// --------------------------------------------------------------------------
// AddrSpace::CopyPageTable (TranslationEntry *oldPT, TranslationEntry
//			       *newPT, int numpages)
//
// Purpose: This function will copy one pageTable into another.  It is
//          used by things like Fork.
// Arguments:
//          oldPT               This is a pointer to the old PageTable
//                              (i.e. the one we're copying).
//          newPT               This is a pointer to the new PageTable
//                              (i.e. the one we're copying into.)
//          numpages            This is the number of pages in both
//                              pages (i.e. they should have the same
//                              number of pages).
// --------------------------------------------------------------------------
void AddrSpace::CopyPageTable (TranslationEntry *oldPT, TranslationEntry
			       *newPT, int numpages)
{
  for (register int i = 0; i < numpages; i++)
    {
      newPT[i].virtualPage = oldPT[i].virtualPage;
      newPT[i].physicalPage = oldPT[i].physicalPage;
      newPT[i].valid = oldPT[i].valid;
      newPT[i].readOnly = oldPT[i].readOnly;
      newPT[i].use = oldPT[i].use;
      newPT[i].dirty = oldPT[i].dirty;
      newPT[i].File = oldPT[i].File;
      newPT[i].offset = oldPT[i].offset;
      newPT[i].zero = oldPT[i].zero;
      newPT[i].cow = oldPT[i].cow;
      newPT[i].clearSC ();
      newPT[i].setTime (oldPT[i].getTime());
      newPT[i].clearRefHistory ();
    }
}


// --------------------------------------------------------------------------
// AddrSpace::ModifySpace(OpenFile *executable)
// Purpose: Given a thread with an already existing address space, and a
//          new executable, we use this to read in the executable.  Note
//          that there are three cases for what we need to do:
//            1) current file is the same size as the new file, so
//               our address space will be big enough, just read it in.
//            2) current file is larger than new file: so return unneeded
//               pages to the MMU.
//            3) current file is smaller than the new file, so request
//               the necessary pages from the MMU.
// Arguments:
//          executable           This is the new file to load into the
//                               user address space.
// --------------------------------------------------------------------------
int AddrSpace::ModifySpace(OpenFile *executable)
{
  NachosHeader nachosH;
  TranslationEntry *oldPageTable = pageTable;
  unsigned int oldnumpages = numPages;
  OpenFile *swapfile;
  uint32_t max_address = 0U, section_size = 0U;

  // Get the file and section headers, and retrieve the start address
  startAddress = ReadHeaders (executable, &nachosH);

  // How big is the address space? VIRTUAL_MEMORY
  // Find the section with the largest starting address, add on its size, and
  // compute the number of pages needed from that.
  for (register int i = 0; i < NUM_SECTIONS; i++)
    {
      if (nachosH.section[i].sh_addr > max_address)
	{
	  max_address = nachosH.section[i].sh_addr;
	  section_size = nachosH.section[i].sh_size;
	}
    }
  numPages = divRoundUp (max_address + section_size, PageSize)
    + divRoundUp (UserStackSize, PageSize);

  // get a new set of empty pages
  ASSERT (SetupTable() == 0);
  Setup_Load(executable, &nachosH);
  swapfile = swap->file();
  for (unsigned int i = 0U; i < oldnumpages; i++)
    {
      if (oldPageTable[i].valid)
	{
	  memory->release_page (oldPageTable[i].physicalPage, this);
	  if (memory->getNumOwners (oldPageTable->physicalPage) == 1)
	    {
	      Frame *oldFrame = memory->get_frame (oldPageTable->physicalPage);
	      oldFrame->owners[0]->pageTable[oldFrame->owners_page_number].cow
		= false;
	    }
	}
      if (oldPageTable[i].File == swapfile)
	{
	  swap->release_frame (oldPageTable[i].offset, this);
	}
    }
  delete [] oldPageTable;

  return 0;
}


// --------------------------------------------------------------------------
// AddrSpace::InitSpace(OpenFile *executable)
// Purpose: This function will initialize a user address space with the
//          executable parameter.  We use this version of InitSpace in
//          the system call Exec, which is passed a filename and needs
//          to load it into memory.
//          The difference between this InitSpace and the one that follows
//          below is that this one uses the executable image to calculate
//          the size of the page table.
// Arguments:
//         executable          pointer to the open executable image
//                             which will be loaded into memory.
// --------------------------------------------------------------------------
int AddrSpace::InitSpace(OpenFile *executable)
{
  NachosHeader nachosH;
  int retval;
  uint32_t max_address = 0U, section_size = 0U;

  // Get the file and section headers, and retrieve the start address
  startAddress = ReadHeaders (executable, &nachosH);

  // How big is the address space?  VIRTUAL_MEMORY
  // Find the section with the largest starting address, add on its size, and
  // compute the number of pages needed from that.
  for (register int i = 0; i < NUM_SECTIONS; i++)
    {
      if (nachosH.section[i].sh_addr > max_address)
	{
	  max_address = nachosH.section[i].sh_addr;
	  section_size = nachosH.section[i].sh_size;
	}
    }
  numPages = divRoundUp (max_address + section_size, PageSize)
    + divRoundUp (UserStackSize, PageSize);

  // Allocate a page table
  if ((retval = SetupTable()) < 0)
    {
      return retval;
    }

  // Copy the code and data segments into memory
  Setup_Load (executable, &nachosH);

  return 0;
}


// --------------------------------------------------------------------------
//  AddrSpace::InitSpace( int numpages )
//  Purpose: We initialize a user address space in this function.  Note
//           that we don't load an executable image in this version of
//           InitSpace, we simply set the number of pages, and then get
//           them from the MMU.
// Arguments:
//           numpages        The number of pages to make the memory space.
// --------------------------------------------------------------------------
int
AddrSpace::InitSpace(int numpages)
{
  numPages = numpages;
  return SetupTable();
}


// --------------------------------------------------------------------------
// AddrSpace::SetupTable
// Purpose: Given a number of pages we allocate a pageTable of the correct
//          size, we then run through each page, requesting a physical
//          page from the MMU to assign to each virtual page.  If we
//          don't have enough physical pages, we return -ENOMEM.
//          Note that this function is called by both versions of initSpace
//          and is the standard way to allocate and setup an empty memory
//          space for a user process.
// Arguments: None.
// --------------------------------------------------------------------------
int AddrSpace::SetupTable() {
  unsigned int size, i;

  size = numPages * PageSize;
  DEBUG('a', "Initializing address space, num pages %d, size %d\n",
	numPages, size);

  // first, set up the translation
  pageTable = new TranslationEntry[numPages];
  if (pageTable == NULL) {
    return -ENOMEM;
  }

  for (i = 0; i < numPages; i++) {
    pageTable[i].virtualPage = i;
    pageTable[i].physicalPage = 0U;
    pageTable[i].valid = false;
    pageTable[i].readOnly = false;
    pageTable[i].use = false;
    pageTable[i].dirty = false;
    pageTable[i].File = NULL;
    pageTable[i].offset = 0U;
    pageTable[i].zero = false;
    pageTable[i].cow = false;
    pageTable[i].clearSC ();
    pageTable[i].setTime (0U);
    pageTable[i].clearRefHistory ();
  }

  return 0;
}


//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Deallocate an address space.  We return all the pages of
//      memory owned by our threads user AddrSpace.
//   Note that this is called in some cases because we were unable to
//   allocate the total number of pages we wanted.  In this case,
//   we reset numPages to be the value we originally allocated so that
//   we don't try to release pages we never owned.
//----------------------------------------------------------------------
AddrSpace::~AddrSpace()
{
  OpenFile *swapFile = swap->file ();

  for (unsigned int i = 0 ; i < numPages; i++)
    {
      if (pageTable[i].valid)
	{
	  memory->release_page(pageTable[i].physicalPage, this);
	}
      if (memory->getNumOwners (pageTable->physicalPage) == 1)
	{
	  Frame *frame = memory->get_frame(pageTable->physicalPage);
	  frame->owners[0]->pageTable[frame->owners_page_number].cow = false;
	}

      if (pageTable[i].File == swapFile)
	{
	  swap->release_frame (pageTable[i].offset, this);
	}
    }
  // if (execFile != NULL) {
  //   delete execFile;
  // }
  delete [] pageTable;
}


//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegistdivRoundUp   ers
//	when this thread is context switched out.
//----------------------------------------------------------------------
void
AddrSpace::InitRegisters()
{
  int i;

  for (i = 0; i < NumTotalRegs; i++)
    machine->WriteRegister(i, 0);

  // Initial program counter -- must be location of "Start"
  machine->WriteRegister(PCReg, startAddress);

  // Need to also tell MIPS where next instruction is, because
  // of branch delay possibility
  machine->WriteRegister(NextPCReg, startAddress + 4);

  // Set the stack register to the end of the address space, where we
  // allocated the stack; but subtract off a bit, to make sure we don't
  // accidentally reference off the end!
  machine->WriteRegister(StackReg, numPages * PageSize - 16);
  DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}


//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------
void AddrSpace::SaveState()
{
  unsigned char mask;

  switch (wsDeltaSize) { // On applique le bon masque en fonction du DeltaSize en entrée.
    case 1:
      mask = 0x80;
    break;
    case 2:
      mask = 0xC0;
    break;
    case 4:
      mask = 0xF0;
    break;
    default:
      mask = 0XFF;
    break;
  }

   for (int i = 0; i < numPages; i++) {
     pageTable[i].historyPage = (pageTable[i].historyPage >> 1) & mask;
   }
   numPages = machine->pageTableSize;
}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------
void AddrSpace::RestoreState()
{
  // FIXME: Shouldn't this function flush the TLB if there is one?
  machine->pageTable = pageTable;
  machine->pageTableSize = numPages;
}


//----------------------------------------------------------------------
// Function: AddrSpace::CopyFrom(AddrSpace *source)
// Purpose: This function creates a new address space in the calling
//          thread and then copies into that address space the contents
//          of *source.  (another thread's address space).
// Arguments:
//          source          a pointer to the address space to be copied.
//----------------------------------------------------------------------
// VIRTUAL_MEMORY (copy on write)
int AddrSpace::CopyFrom(Thread *ourThread)
{
  unsigned int i;
  OpenFile *swapfile = swap->file ();

  numPages = currentThread->space->numPages;
  pageTable = new TranslationEntry[numPages];

  CopyPageTable (currentThread->space->pageTable, pageTable, numPages);

  for (i = 0; i < numPages; i++)
    {
      if (pageTable[i].File == swapfile)
	{
	  swap->add_frame_owner (pageTable[i].offset, ourThread, i);

	  // Only set COW if the page is already in swap
	  pageTable[i].cow = true;
	  currentThread->space->pageTable[i].cow = true;
	}
      if (pageTable[i].valid)
	{
	  memory->add_frame_owner (pageTable[i].physicalPage, ourThread, i);

	  // Only set COW if the page is already in memory
	  pageTable[i].cow = true;
	  currentThread->space->pageTable[i].cow = true;
	}
    }

  return 0;
}


void AddrSpace::duplicatePage (int virtPageNumber) {
  TranslationEntry* local;
  int dest_page;
  OpenFile *swapfile = swap->file ();

  local = get_page_ptr (virtPageNumber);

  if ((dest_page = memory->get_next_free_page ()) < 0)
    {
      dest_page = memory->Choose_Victim (local->physicalPage);
      memory->pageout (dest_page);
    }

  memory->add_frame_owner (dest_page, currentThread, virtPageNumber);

  memcpy (&(machine->mainMemory [dest_page * PageSize]),
	  &(machine->mainMemory [local->physicalPage * PageSize]),
	  PageSize);

  memory->release_page (local->physicalPage, this);
  if (local->File == swapfile)
    {
      swap->release_frame (local->offset, this);
      local->File = NULL;
    }

  if (memory->getNumOwners (local->physicalPage) == 1)
    {
      Frame *oldFrame = memory->get_frame(local->physicalPage);
      oldFrame->owners[0]->pageTable[oldFrame->owners_page_number].cow = false;
    }

  local->cow = false;
  local->valid = true;
  local->physicalPage = dest_page;
  local->clearSC ();
  local->setTime (0);
  local->clearRefHistory ();
}


// -----------------------------------------------------------------------
// Setup_Load
// Purpose: This function will initialize the thread's page table and
//          then swap in some of its pages.
// -----------------------------------------------------------------------
int
AddrSpace::Setup_Load(OpenFile *executable, NachosHeader *nachosH)
{
  // setup page table
  register unsigned int i = 0, oldi;
  unsigned int stack_start = 0U;
#define STACK_START_MAX(name)						\
  if (stack_start <							\
      nachosH->section[name].sh_addr + nachosH->section[name].sh_size)	\
    stack_start =							\
      nachosH->section[name].sh_addr + nachosH->section[name].sh_size

  execFile = executable;

  // register info setup
  i = divRoundUp (nachosH->section[REGINFO].sh_addr, PageSize);
  DEBUG('a', "REGINFO section at page 0x%x, VA 0x%x, 0x%x bytes\n", i,
	nachosH->section[REGINFO].sh_addr, nachosH->section[REGINFO].sh_size);
  STACK_START_MAX(REGINFO);
  for (oldi = i;
       i < oldi + divRoundUp (nachosH->section[REGINFO].sh_size, PageSize);
       i++)
    {
      pageTable[i].File = executable;
      pageTable[i].offset =
	nachosH->section[REGINFO].sh_offset + (i - oldi) * PageSize;
      pageTable[i].readOnly = true;
      pageTable[i].zero = false;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
      pageTable[i].clearRefHistory ();
    }

  // code setup
  i = divRoundUp (nachosH->section[TEXT].sh_addr, PageSize);
  DEBUG('a', "TEXT section at page 0x%x, VA 0x%x, 0x%x bytes\n", i,
	nachosH->section[TEXT].sh_addr, nachosH->section[TEXT].sh_size);
  STACK_START_MAX(TEXT);
  for (oldi = i;
       i < oldi + divRoundUp (nachosH->section[TEXT].sh_size, PageSize);
       i++)
    {
      pageTable[i].File = executable;
      pageTable[i].offset =
	nachosH->section[TEXT].sh_offset + (i - oldi) * PageSize;
      pageTable[i].readOnly = true;
      pageTable[i].zero = false;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
      pageTable[i].clearRefHistory ();
    }

  // initialized read-only data setup
  i = divRoundUp (nachosH->section[RDATA].sh_addr, PageSize);
  DEBUG('a', "RDATA section at page 0x%x, VA 0x%x, 0x%x bytes\n", i,
	nachosH->section[RDATA].sh_addr, nachosH->section[RDATA].sh_size);
  STACK_START_MAX(RDATA);
  for (oldi = i;
       i < oldi + divRoundUp (nachosH->section[RDATA].sh_size, PageSize);
       i++)
    {
      pageTable[i].File = executable;
      pageTable[i].offset =
	nachosH->section[RDATA].sh_offset + (i - oldi) * PageSize;
      pageTable[i].readOnly = true;
      pageTable[i].zero = false;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
      pageTable[i].clearRefHistory ();
    }

  // initialized data setup
  i = divRoundUp (nachosH->section[DATA].sh_addr, PageSize);
  DEBUG('a', "DATA section at page 0x%x, VA 0x%x, 0x%x bytes\n", i,
	nachosH->section[DATA].sh_addr, nachosH->section[DATA].sh_size);
  STACK_START_MAX(DATA);
  for (oldi = i;
       i < oldi + divRoundUp (nachosH->section[DATA].sh_size, PageSize);
       i++)
    {
      pageTable[i].File = executable;
      pageTable[i].offset =
	nachosH->section[DATA].sh_offset + (i - oldi) * PageSize;
      pageTable[i].readOnly = false;
      pageTable[i].zero = false;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
      pageTable[i].clearRefHistory ();
    }

  // uninitialized data setup
  i = divRoundUp (nachosH->section[BSS].sh_addr, PageSize);
  DEBUG('a', "BSS section at page 0x%x, VA 0x%x, 0x%x bytes\n", i,
	nachosH->section[BSS].sh_addr, nachosH->section[BSS].sh_size);
  STACK_START_MAX(BSS);
  for (oldi = i;
       i < oldi + divRoundUp (nachosH->section[BSS].sh_size, PageSize);
       i++)
    {
      pageTable[i].File = NULL;
      pageTable[i].offset = 0;
      pageTable[i].readOnly = false;
      pageTable[i].zero = true;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
      pageTable[i].clearRefHistory ();
    }

  // uninitialized short data setup
  i = divRoundUp (nachosH->section[SBSS].sh_addr, PageSize);
  DEBUG('a', "SBSS section at page 0x%x, VA 0x%x, 0x%x bytes\n", i,
	nachosH->section[SBSS].sh_addr, nachosH->section[SBSS].sh_size);
  STACK_START_MAX(SBSS);
  for (oldi = i;
       i < oldi + divRoundUp (nachosH->section[SBSS].sh_size, PageSize);
       i++)
    {
      pageTable[i].File = NULL;
      pageTable[i].offset = 0;
      pageTable[i].readOnly = false;
      pageTable[i].zero = true;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
       pageTable[i].setTimeUse(stats->totalTicks);
      pageTable[i].clearRefHistory ();
    }

  // stack setup
  i = divRoundUp (stack_start, PageSize);
  DEBUG('a', "Stack section at page 0x%x, 0x%x bytes\n", i, UserStackSize);
  for (oldi = i; i < oldi + divRoundUp (UserStackSize, PageSize); i++)
    {
      pageTable[i].File = NULL;
      pageTable[i].offset = 0;
      pageTable[i].readOnly = false;
      pageTable[i].zero = true;
      pageTable[i].clearSC ();
      pageTable[i].setTime (0);
      pageTable[i].clearRefHistory ();
    }

  // To start the program, we page in the first page of program text and the
  // last (stack) page of the address space.
  memory->pagein (divRoundUp (nachosH->section[TEXT].sh_addr, PageSize), this);
  memory->pagein (numPages - 1, this);

  return 0;
}


// -----------------------------------------------------------------------
// NumPagesUsed
// Purpose: Calculates the number of physical pages this thread
//          currently owns
// -----------------------------------------------------------------------
unsigned int AddrSpace::NumPagesUsed () {
  int num = 0;
  unsigned int i;

  for (i = 0; i < numPages; i++) {
    if (pageTable[i].valid) {
      num++;
    }
  }
  return num;
}

// -----------------------------------------------------------------------
// TooManyPages
// Purpose: Checks if the current process owns more pages than its
//          working set allows.  Returns 1 if it does.
// -----------------------------------------------------------------------
int AddrSpace::TooManyPages () {
  // return the appropriate value here
  return (NumPagesUsed() > 64);
}




// -----------------------------------------------------------------------
// FIFO_Choose_Victim
// Purpose: This function chooses a victim from the physical pages
//          already allocated to this process.  The value notMe, if not
//          equal to -1, indicates a physical page that you must not
//          choose.  You should return the number of the physical page
//          that you have chosen to release.
//          It uses the First-In First-Out algorithm
// -----------------------------------------------------------------------
int AddrSpace::FIFO_Choose_Victim (int notMe) {
  // You need to return an appropriate value here.
  int tmp = -1;
  for (size_t i = 0; i < numPages; i++) {
    if (i != notMe
        && pageTable[i].valid
        && (tmp == -1 || pageTable[i].timePage <= pageTable[tmp].timePage)) {
          tmp = i;
        }
  }
  return pageTable[tmp].physicalPage;
}


// -----------------------------------------------------------------------
// LRU_Choose_Victim
// Purpose: This function chooses a victim from the physical pages
//          already allocated to this process.  The value notMe, if not
//          equal to -1, indicates a physical page that you must not
//          choose.  You should return the number of the physical page
//          that you have chosen to release.
//          It uses the Least-Recently-Used algorithm
// -----------------------------------------------------------------------
int AddrSpace::LRU_Choose_Victim (int notMe) {
  // You need to return an appropriate value here.
//  printf("lru\n");
  int tmp = -1;
  for (size_t i = 0; i < numPages; i++) {
    if (i != notMe
        && pageTable[i].valid
        && (tmp == -1 || pageTable[i].historyPage <= pageTable[tmp].historyPage)) {
          tmp = i;
        }
  }
  return pageTable[tmp].physicalPage;
}


// -----------------------------------------------------------------------
// SC_Choose_Victim
// Purpose: This function chooses a victim from the physical pages
//          already allocated to this process.  The value notMe, if not
//          equal to -1, indicates a physical page that you must not
//          choose.  You should return the number of the physical page
//          that you have chosen to release.
//          It uses the Enhanced Second Chance algorithm
// -----------------------------------------------------------------------
int AddrSpace::Fifo_next_victim (int act, int notMe) {
  int tmp = -1;
  printf("Numpage : %d \n",numPages );
  for (size_t i = 0; i < numPages; i++) {
    if (i != notMe && i != act
        && pageTable[i].valid
        && pageTable[i].timePage >= pageTable[act].timePage
        && (tmp == -1 || pageTable[i].timePage <= pageTable[tmp].timePage)) {
          tmp = i;
        }
  }
  return i;
}

int AddrSpace::SC_Choose_Victim (int notMe) {
  // You need to return an appropriate value here.
  int i = FIFO_Choose_Victim (notMe);
  while (1) {
    if (pageTable[i].historyPage == 0) {
      return pageTable[i].physicalPage;
    } else if (pageTable[i].historyPage != 0
            && pageTable[i].secondChance == true) {
      pageTable[i].secondChance == false;
    } else {
      return pageTable[i].physicalPage;
    }
    if ((i = Fifo_next_victim (i, notMe)) == -1)
      i = FIFO_Choose_Victim (notMe);
  }
  return 0;
}
