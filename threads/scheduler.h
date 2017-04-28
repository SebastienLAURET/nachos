// -*- C++ -*-
// scheduler.h 
//	Data structures for the thread dispatcher and scheduler.
//	Primarily, the list of threads that are ready to run.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SCHEDULER_H
#define SCHEDULER_H

#pragma interface "threads/scheduler.h"

#include "copyright.h"
#include "list.h"
#include "thread.h"
#include "system.h"
#include "nachos-gdb.h"

class Thread;

// The following class defines the scheduler/dispatcher abstraction -- 
// the data structures and operations needed to keep track of which 
// thread is running, and which threads are ready but not running.

extern bool ThreadOrder(void *, void *); // Order two threads by priority

class Scheduler {
public:
  Scheduler();				// Initialize a scheduler

  void ReadyToRun(Thread* thread);	// Thread can be dispatched.
  Thread* FindNextToRun();		// Dequeue first thread on the ready 
					// list, if any, and return thread.
  void Run(Thread* nextThread);		// Cause nextThread to start running
#ifdef RDN_DEBUG_HELP
  void PrintReadyList();		// Print contents of ready list
#endif

  int addToList(Thread * thread);	// Add a thread to the list
  void removeFromList(Thread * thread);	// Remove a thread from the list
#ifdef SMARTGDB
  Thread * GetThis( Thread * thread );
#endif
  void Switch_To( Thread * newthread );	// Switch to execute newthread.
private:
  SortedList readyList;			// Queue of threads that are ready to
					// run, but are not running
  int threadid;
};

#endif // SCHEDULER_H
