// scheduler.cc
//	Routines to choose the next thread to run, and to dispatch to
//	that thread.
//
// 	These routines assume that interrupts are already disabled.
//	If interrupts are disabled, we can assume mutual exclusion
//	(since we are on a uniprocessor).
//
// 	NOTE: We can't use Locks to provide mutual exclusion here, since
// 	if we needed to wait for a lock, and the lock was busy, we would
//	end up calling FindNextToRun(), and that would put us in an
//	infinite loop.
//
// 	Very simple implementation -- no priorities, straight FIFO.
//	Might need to be improved in later assignments.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation
// of liability and disclaimer of warranty provisions.

#pragma implementation "threads/scheduler.h"

#include "copyright.h"
#include "scheduler.h"
#include "system.h"

//----------------------------------------------------------------------
// Scheduler::Scheduler
// 	Initialize a new scheduler
//----------------------------------------------------------------------

Scheduler::Scheduler ()
{
  threadid = 1;
  readyList.setComparator (ThreadOrder);
}

//----------------------------------------------------------------------
// Scheduler::ThreadOrder
// 	Order two threads by priority.
//----------------------------------------------------------------------
#include <iostream>
bool
ThreadOrder(void *thread1, void *thread2)
{
//  std::cout << "TOTO" << std::endl;
  Thread *t1 = static_cast<Thread *>(thread1);
  Thread *t2 = static_cast<Thread *>(thread2);
  /* Return true if the priority of thread t1 is BETTER than the priority of
     thread t2; return false otherwise. */
//  std::cout <<t1->getPriority()<< "<" << t2->getPriority()<<std::endl;
  return (t1->getPriority() < t2->getPriority());
}

//----------------------------------------------------------------------
// Scheduler::ReadyToRun
// 	Mark a thread as ready, but not running.
//	Put it on the ready list, for later scheduling onto the CPU.
//
//	"thread" is the thread to be put on the ready list.
//----------------------------------------------------------------------

void
Scheduler::ReadyToRun (Thread *thread)
{
    DEBUG('t', "Putting thread %s on ready list.\n", thread->getName());

    if (thread != currentThread) {
#ifdef RDN_DEBUG_HELP
	//
	// When first figuring out how often this
	// happens and why I found the printf's useful
	//
	printf("\n\n Non-current thread ");
	thread->Print();
	printf(" added to ready \n\n");
	fflush(stdout);
#endif
	interrupt->setNeedResched();
    }

    thread->setStatus(READY);
    readyList.Insert(thread);

#ifdef RDN_DEBUG_HELP
    PrintReadyList();
#endif
}


//----------------------------------------------------------------------
// Scheduler::FindNextToRun
// 	Return the next thread to be scheduled onto the CPU.
//	If there are no ready threads, return NULL.
// Side effect:
//	Thread is removed from the ready list.
//----------------------------------------------------------------------

Thread *
Scheduler::FindNextToRun ()
{
  Thread *lastThread, *nextThread;

  //
  // Check to see if all threads have run to completion
  //
  if ((lastThread = (Thread *) allThreads.head->thread) ==
      (Thread *) allThreads.tail->thread &&
      lastThread->getStatus() == ZOMBIE) {
    //
    // If so, halt the system
    //
    interrupt->Halt();
  }

#ifdef RDN_DEBUG_HELP
  printf("\n");
  PrintReadyList();
  printf("Current: ");
  currentThread->Print();
  printf("\n");
  fflush(stdout);
#endif

  // Choose a thread to run
  //
  nextThread = static_cast<Thread *>(readyList.Remove());

#ifdef RDN_DEBUG_HELP
  if (nextThread != NULL) {
    printf("Chose: ");
    nextThread->Print();
    printf("\n\n");
    fflush(stdout);
  }
#endif

  return nextThread;
}


//----------------------------------------------------------------------
// Scheduler::Run
// 	Dispatch the CPU to nextThread.  Save the state of the old thread,
//	and load the state of the new thread, by calling the machine
//	dependent context switch routine, SWITCH.
//
//      Note: we assume the state of the previously running thread has
//	already been changed from running to blocked or ready (depending).
// Side effect:
//	The global variable currentThread becomes nextThread.
//
//	"nextThread" is the thread to be put into the CPU.
//----------------------------------------------------------------------

void
Scheduler::Run (Thread *nextThread)
{
    Thread *oldThread = currentThread;
    ListElement *elem = readyList.Head();
    while (elem) {
        if (elem->item)
          ((Thread*)elem->item)->decrPriority();
        elem = elem->next;
    }
#ifdef USER_PROGRAM			// ignore until running user programs
    if (currentThread->space != NULL) {	// if this thread is a user program,
        currentThread->SaveUserState(); // save the user's CPU registers
	       currentThread->space->SaveState();
    }
#endif

    oldThread->CheckOverflow();		    // check if the old thread
					    // had an undetected stack overflow

    currentThread = nextThread;		    // switch to the next thread
    currentThread->setStatus(RUNNING);      // nextThread is now running

    DEBUG('t', "Switching from thread \"%s\" to thread \"%s\"\n",
	  oldThread->getName(), nextThread->getName());

    // This is a machine-dependent assembly language routine defined
    // in switch.s.  You may have to think
    // a bit to figure out what happens after this, both from the point
    // of view of the thread and from the perspective of the "outside world".

    stats->numContextSwitches++;
    if (currentThread) {
        currentThread->procStats->numContextSwitches++;
        if (wasYieldOnReturn) {
	    currentThread->procStats->numInvolContextSwitches++;
	} else {
	    currentThread->procStats->numVolContextSwitches++;
	}
    }
    wasYieldOnReturn = false;


    SWITCH(oldThread, nextThread);

    DEBUG('t', "Now in thread \"%s\"\n", currentThread->getName());

    // If the old thread gave up the processor because it was finishing,
    // we need to delete its carcass.  Note we cannot delete the thread
    // before now (for example, in Thread::Finish()), because up to this
    // point, we were still running on the old thread's stack!
    if (threadToBeDestroyed != NULL) {
        delete threadToBeDestroyed;
	threadToBeDestroyed = NULL;
    }

#ifdef USER_PROGRAM
    if (currentThread->space != NULL) {		// if there is an address space
        currentThread->RestoreUserState();     // to restore, do it.
	currentThread->space->RestoreState();
    }
#endif
}


#ifdef RDN_DEBUG_HELP
//----------------------------------------------------------------------
// Scheduler::PrintReadyList
// 	Print the scheduler state -- in other words, the contents of
//	the ready list.  For debugging.
//----------------------------------------------------------------------

void
Scheduler::PrintReadyList()
{
    printf(" RL: ");
    readyList.Mapcar((VoidFunctionPtr) ThreadPrint);
    printf("\n");
    fflush(stdout);
}
#endif /* RDN_DEBUG_HELP */


//----------------------------------------------------------------------
// Scheduler::addToList
//       This will add an element to the list of existing threads.
//----------------------------------------------------------------------

int
Scheduler::addToList (Thread* thread)
{
  struct nachos_thread *node = new nachos_thread;

  node->ID = threadid++;
  node->next = NULL;
  node->thread = (void *)thread;

  if (allThreads.tail) {
    allThreads.tail->next = node;
    allThreads.tail = node;
  } else {
    allThreads.tail = allThreads.head = node;
  }

  return node->ID;
}


//----------------------------------------------------------------------
// Scheduler::removeFromList
//           This takes an element off the list of existing threads.
//----------------------------------------------------------------------

void
Scheduler::removeFromList (Thread* thread)
{
  struct nachos_thread *tmp = allThreads.head, *prev = NULL;

  while (tmp) {
    if (tmp->thread == (void *)thread) {
      break;
    }
    prev = tmp;
    tmp = tmp->next;
  }
  if (tmp) {
    if (prev) {
      prev->next = tmp->next;
      if (prev->next == NULL) {
	allThreads.tail = prev;
      }
    } else {
      allThreads.head = tmp->next;
      if (allThreads.head->next == NULL) {
	allThreads.tail = tmp->next;
      }
    }
    delete tmp;
  }
}

#ifdef SMARTGDB
//----------------------------------------------------------------------
// Scheduler::GetThis
//           This takes an element off the list of existing threads.
//----------------------------------------------------------------------

Thread *
Scheduler::GetThis (Thread* thread)
{
  return static_cast<Thread *>(readyList.Remove (thread));
}
#endif


//----------------------------------------------------------------------
// Scheduler::SwitchTo()
//           This will switch to running a particular thread.  It it
//           mostly for use in debugging.  Note that if the thread is
//           null, or if it's not already ready, this function won't do
//           anything.
//----------------------------------------------------------------------
void Scheduler::Switch_To( Thread * newthread ) {

  // If no thread, return.
  if ( newthread == NULL ) {
    return;
  }

  // If thread is not ready to run, return.
  if ( newthread->getStatus() != READY ) {
    return;
  }

  //
  // Remove the thread from the ready list.
  // So we don't have it on there twice.
  //
  readyList.Delete (newthread);

  //
  // Add it to the front of the readylist.
  //
  readyList.Prepend (newthread);

  currentThread->Yield();
}
