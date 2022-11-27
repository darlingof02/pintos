/* This file is derived from source code for the Nachos
   instructional operating system.  The Nachos copyright notice
   is reproduced in full below. */

/* Copyright (c) 1992-1996 The Regents of the University of California.
   All rights reserved.

   Permission to use, copy, modify, and distribute this software
   and its documentation for any purpose, without fee, and
   without written agreement is hereby granted, provided that the
   above copyright notice and the following two paragraphs appear
   in all copies of this software.

   IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO
   ANY PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR
   CONSEQUENTIAL DAMAGES ARISING OUT OF THE USE OF THIS SOFTWARE
   AND ITS DOCUMENTATION, EVEN IF THE UNIVERSITY OF CALIFORNIA
   HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

   THE UNIVERSITY OF CALIFORNIA SPECIFICALLY DISCLAIMS ANY
   WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
   WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
   PURPOSE.  THE SOFTWARE PROVIDED HEREUNDER IS ON AN "AS IS"
   BASIS, AND THE UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO
   PROVIDE MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR
   MODIFICATIONS.
*/

#include "threads/synch.h"
#include <stdio.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

/* (add code)(2)
 * used to compare the priority of threads
 */
static bool comparator_greater_thread_priority(const struct list_elem*, const struct list_elem*, void *);
/* (add code)(2)
 * used to compare the priority of threads
 */
static bool comparator_greater_lock_priority(const struct list_elem* a, const struct list_elem *b, void* aux UNUSED);
/*
 * (add code)(2)
 * used to compare the priority of condition waiter
 */
static bool comparator_greater_condition_priority(const struct list_elem* a, const struct list_elem *b, void* aux UNUSED);
/* Initializes semaphore SEMA to VALUE.  A semaphore is a
   nonnegative integer along with two atomic operators for
   manipulating it:

   - down or "P": wait for the value to become positive, then
     decrement it.

   - up or "V": increment the value (and wake up one waiting
     thread, if any). */
void
sema_init (struct semaphore *sema, unsigned value) 
{
  ASSERT (sema != NULL);

  sema->value = value;
  list_init (&sema->waiters);
}

/* Down or "P" operation on a semaphore.  Waits for SEMA's value
   to become positive and then atomically decrements it.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but if it sleeps then the next scheduled
   thread will probably turn interrupts back on. */
void
sema_down (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);
  ASSERT (!intr_context ());

  old_level = intr_disable ();
  while (sema->value == 0) 
    {
/* (add code)(2)
   * insert thread at waiters list in order of priority
   */
      list_insert_ordered (&sema->waiters, &thread_current()->elem, comparator_greater_thread_priority, NULL);
      thread_block ();
    }
  sema->value--;
  intr_set_level (old_level);

  
}

/* Down or "P" operation on a semaphore, but only if the
   semaphore is not already 0.  Returns true if the semaphore is
   decremented, false otherwise.

   This function may be called from an interrupt handler. */
bool
sema_try_down (struct semaphore *sema) 
{
  enum intr_level old_level;
  bool success;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (sema->value > 0) 
    {
      sema->value--;
      success = true; 
    }
  else
    success = false;
  intr_set_level (old_level);

  return success;
}

/* Up or "V" operation on a semaphore.  Increments SEMA's value
   and wakes up one thread of those waiting for SEMA, if any.

   This function may be called from an interrupt handler. */
void
sema_up (struct semaphore *sema) 
{
  enum intr_level old_level;

  ASSERT (sema != NULL);

  old_level = intr_disable ();
  if (!list_empty (&sema->waiters)) {
/* (add code)(2)
   * it is to consider the case of changing priority of threads in waiters list
   * the thread of highest priority should wake up
   */
  list_sort(&(sema->waiters), comparator_greater_thread_priority, NULL);
   thread_unblock (list_entry (list_pop_front (&sema->waiters), struct thread, elem));
  }
  sema->value++;
  intr_set_level (old_level);

  thread_yield();
  
}

static void sema_test_helper (void *sema_);

/* Self-test for semaphores that makes control "ping-pong"
   between a pair of threads.  Insert calls to printf() to see
   what's going on. */
void
sema_self_test (void) 
{
  struct semaphore sema[2];
  int i;

  printf ("Testing semaphores...");
  sema_init (&sema[0], 0);
  sema_init (&sema[1], 0);
  thread_create ("sema-test", PRI_DEFAULT, sema_test_helper, &sema);
  for (i = 0; i < 10; i++) 
    {
      sema_up (&sema[0]);
      sema_down (&sema[1]);
    }
  printf ("done.\n");
}

/* Thread function used by sema_self_test(). */
static void
sema_test_helper (void *sema_) 
{
  struct semaphore *sema = sema_;
  int i;

  for (i = 0; i < 10; i++) 
    {
      sema_down (&sema[0]);
      sema_up (&sema[1]);
    }
}

/* Initializes LOCK.  A lock can be held by at most a single
   thread at any given time.  Our locks are not "recursive", that
   is, it is an error for the thread currently holding a lock to
   try to acquire that lock.

   A lock is a specialization of a semaphore with an initial
   value of 1.  The difference between a lock and such a
   semaphore is twofold.  First, a semaphore can have a value
   greater than 1, but a lock can only be owned by a single
   thread at a time.  Second, a semaphore does not have an owner,
   meaning that one thread can "down" the semaphore and then
   another one "up" it, but with a lock the same thread must both
   acquire and release it.  When these restrictions prove
   onerous, it's a good sign that a semaphore should be used,
   instead of a lock. */
void
lock_init (struct lock *lock)
{
  ASSERT (lock != NULL);

  lock->holder = NULL;
  sema_init (&lock->semaphore, 1);
}

/* Acquires LOCK, sleeping until it becomes available if
   necessary.  The lock must not already be held by the current
   thread.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
lock_acquire (struct lock *lock)
{
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (!lock_held_by_current_thread (lock));

  struct thread* t = thread_current();
  t->wait_lock = lock;
  if (lock->holder == NULL) { // no holder for required lock
    lock->lock_priority = t->priority;
  }

  // nested donation
  struct lock *cur_lock = lock;
  while (cur_lock->holder != NULL && cur_lock->holder->priority < t->priority) { //donation is needed
    cur_lock->holder->priority = t->priority; //donation

    cur_lock->lock_priority = (cur_lock->lock_priority < t->priority) ? t->priority : cur_lock->lock_priority; //keep the priority of the lock
    
    // t= cur_lock->holder;
    cur_lock = cur_lock->holder->wait_lock;
    if (cur_lock == NULL) break;
  }

  sema_down (&lock->semaphore);
  lock->holder = thread_current ();

  lock->holder->wait_lock = NULL;
  list_insert_ordered(&(lock->holder->locks), &(lock->lockelem),comparator_greater_lock_priority, NULL);
  // list_push_back (&lock->holder->locks, &lock->lockelem);
  // list_sort(&lock->holder->locks, comparator_greater_lock_priority, NULL);
  
}

/* Tries to acquires LOCK and returns true if successful or false
   on failure.  The lock must not already be held by the current
   thread.

   This function will not sleep, so it may be called within an
   interrupt handler. */
bool
lock_try_acquire (struct lock *lock)
{
  bool success;

  ASSERT (lock != NULL);
  ASSERT (!lock_held_by_current_thread (lock));

  success = sema_try_down (&lock->semaphore);
  if (success)
    lock->holder = thread_current ();
  return success;
}

/* Releases LOCK, which must be owned by the current thread.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to release a lock within an interrupt
   handler. */
void
lock_release (struct lock *lock) 
{
  ASSERT (lock != NULL);
  ASSERT (lock_held_by_current_thread (lock));

  

  lock->holder = NULL;
  sema_up (&lock->semaphore);

  struct thread *t = thread_current();
  list_remove(&lock->lockelem);
  if (list_empty(&t->locks)) {
    t->priority = t->prepriority;
  }
  else {
    // list_sort(&(t->locks), comparator_greater_lock_priority, NULL);
    int next_priority = list_entry(list_front(&(t->locks)), struct lock, lockelem)->lock_priority;
    // if (next_priority > t->prepriority) {
      t->priority = next_priority;
    // }
    // else {
      // t->priority = t->prepriority; 
    // }
    
  }

  thread_priority_donate(t, t->priority);

  // thread_yield();


  // if (!list_empty(&ready_list)) {
  //     struct thread *next_thread = list_entry(list_begin(&ready_list), struct thread, elem);
  //     if (next_thread != NULL && next_thread -> priority > t->priority) {
  //       thread_yield();
  //     }
  // }
}

/* Returns true if the current thread holds LOCK, false
   otherwise.  (Note that testing whether some other thread holds
   a lock would be racy.) */
bool
lock_held_by_current_thread (const struct lock *lock) 
{
  ASSERT (lock != NULL);

  return lock->holder == thread_current ();
}

/* One semaphore in a list. */
struct semaphore_elem 
  {
    struct list_elem elem;              /* List element. */
    struct semaphore semaphore;         /* This semaphore. */
  };

/* Initializes condition variable COND.  A condition variable
   allows one piece of code to signal a condition and cooperating
   code to receive the signal and act upon it. */
void
cond_init (struct condition *cond)
{
  ASSERT (cond != NULL);

  list_init (&cond->waiters);
}

/* Atomically releases LOCK and waits for COND to be signaled by
   some other piece of code.  After COND is signaled, LOCK is
   reacquired before returning.  LOCK must be held before calling
   this function.

   The monitor implemented by this function is "Mesa" style, not
   "Hoare" style, that is, sending and receiving a signal are not
   an atomic operation.  Thus, typically the caller must recheck
   the condition after the wait completes and, if necessary, wait
   again.

   A given condition variable is associated with only a single
   lock, but one lock may be associated with any number of
   condition variables.  That is, there is a one-to-many mapping
   from locks to condition variables.

   This function may sleep, so it must not be called within an
   interrupt handler.  This function may be called with
   interrupts disabled, but interrupts will be turned back on if
   we need to sleep. */
void
cond_wait (struct condition *cond, struct lock *lock) 
{
  struct semaphore_elem waiter;

  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));
  // enum intr_level old_level;
  // old_level = intr_disable ();
  sema_init (&waiter.semaphore, 0);
/* (add code)(2)
   * insert thread at waiters list in order of priority
*/
  // waiter.semaphore.priority = thread_current() -> priority;
  

  list_push_back (&cond->waiters, &waiter.elem);
  // list_sort(&cond->waiters, comparator_greater_condition_priority, NULL);
  
  lock_release (lock);
  sema_down (&waiter.semaphore); //block
  // list_insert_ordered (&cond->waiters, &waiter.elem, comparator_greater_condition_priority, NULL);
  lock_acquire (lock);
  // intr_set_level (old_level);
  }

/* If any threads are waiting on COND (protected by LOCK), then
   this function signals one of them to wake up from its wait.
   LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_signal (struct condition *cond, struct lock *lock UNUSED) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);
  ASSERT (!intr_context ());
  ASSERT (lock_held_by_current_thread (lock));

  if (!list_empty (&cond->waiters)) {
    list_sort(&cond->waiters, comparator_greater_condition_priority, NULL);
    sema_up (&list_entry (list_pop_front (&cond->waiters),
                          struct semaphore_elem, elem)->semaphore);
  }
  /* (add code)(2)
   * it is to consider the case of changing priority of threads in waiters list
   */
}

/* Wakes up all threads, if any, waiting on COND (protected by
   LOCK).  LOCK must be held before calling this function.

   An interrupt handler cannot acquire a lock, so it does not
   make sense to try to signal a condition variable within an
   interrupt handler. */
void
cond_broadcast (struct condition *cond, struct lock *lock) 
{
  ASSERT (cond != NULL);
  ASSERT (lock != NULL);

  while (!list_empty (&cond->waiters))
    cond_signal (cond, lock);
}

/* (add code)(2)
 * used to compare priorities of threads
 */
static bool comparator_greater_thread_priority(const struct list_elem* a, const struct list_elem *b, void* aux UNUSED)
{
  const struct thread* x = list_entry(a, struct thread, elem);
  const struct thread* y = list_entry(b, struct thread, elem);
  ASSERT(x != NULL && y != NULL);
  return x->priority > y->priority;
}

static bool comparator_greater_condition_priority(const struct list_elem* a, const struct list_elem *b, void* aux UNUSED)
{
  const struct semaphore_elem* x = list_entry (a, struct semaphore_elem, elem);
  const struct semaphore_elem* y = list_entry (b, struct semaphore_elem, elem);
  ASSERT(x != NULL && y != NULL);
  const struct thread* t1 = list_entry(list_front(&x->semaphore.waiters), struct thread, elem);
  const struct thread* t2 = list_entry(list_front(&y->semaphore.waiters), struct thread, elem);

  return t1->priority > t2->priority;
}

static bool comparator_greater_lock_priority(const struct list_elem* a, const struct list_elem *b, void* aux UNUSED)
{
  const struct lock* x = list_entry (a, struct lock, lockelem);
  const struct lock* y = list_entry (b, struct lock, lockelem);
  ASSERT(x != NULL && y != NULL);

  return x->lock_priority > y->lock_priority;
}

/*
 * used to compare priorities of semaphore
 */

// static bool comparator_greater_sema_priority(const struct list_elem* a, const struct list_elem *b, void* aux UNUSED)
// {
//   const struct semaphore_elem* x = list_entry(a, struct semaphore_elem, elem);
//   const struct semaphore_elem* y = list_entry(b, struct semaphore_elem, elem);
//   ASSERT(x != NULL && y != NULL);
//   return x->semaphore.priority > y->semaphore.priority;
// }
