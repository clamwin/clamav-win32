/*
 * pthread_win32_attach_detach_np.c
 *
 * Description:
 * This translation unit implements non-portable thread functions.
 *
 * --------------------------------------------------------------------------
 *
 *      Pthreads-win32 - POSIX Threads Library for Win32
 *      Copyright(C) 1998 John E. Bossom
 *      Copyright(C) 1999,2005 Pthreads-win32 contributors
 * 
 *      Contact Email: rpj@callisto.canberra.edu.au
 * 
 *      The current list of contributors is contained
 *      in the file CONTRIBUTORS included with the source
 *      code distribution. The list can also be seen at the
 *      following World Wide Web location:
 *      http://sources.redhat.com/pthreads-win32/contributors.html
 * 
 *      This library is free software; you can redistribute it and/or
 *      modify it under the terms of the GNU Lesser General Public
 *      License as published by the Free Software Foundation; either
 *      version 2 of the License, or (at your option) any later version.
 * 
 *      This library is distributed in the hope that it will be useful,
 *      but WITHOUT ANY WARRANTY; without even the implied warranty of
 *      MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *      Lesser General Public License for more details.
 * 
 *      You should have received a copy of the GNU Lesser General Public
 *      License along with this library in the file COPYING.LIB;
 *      if not, write to the Free Software Foundation, Inc.,
 *      59 Temple Place - Suite 330, Boston, MA 02111-1307, USA
 */

#include "pthread.h"
#include "implement.h"

BOOL
pthread_win32_process_attach_np ()
{
  BOOL result = TRUE;

  result = ptw32_processInitialize ();

#if defined(_UWIN)
  pthread_count++;
#endif

#if defined(__GNUC__)
  ptw32_features = 0;
#else
  /*
   * This is obsolete now.
   */
  ptw32_features = PTW32_SYSTEM_INTERLOCKED_COMPARE_EXCHANGE;
#endif

	  ptw32_register_cancelation = ptw32_RegisterCancelation;

  return result;
}


BOOL
pthread_win32_process_detach_np ()
{
  if (ptw32_processInitialized)
    {
      ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

      if (sp != NULL)
	{
	  /*
	   * Detached threads have their resources automatically
	   * cleaned up upon exit (others must be 'joined').
	   */
	  if (sp->detachState == PTHREAD_CREATE_DETACHED)
	    {
	      ptw32_threadDestroy (sp->ptHandle);
	      TlsSetValue (ptw32_selfThreadKey->key, NULL);
	    }
	}

      /*
       * The DLL is being unmapped from the process's address space
       */
      ptw32_processTerminate ();
    }

  return TRUE;
}

BOOL
pthread_win32_thread_attach_np ()
{
  return TRUE;
}

BOOL
pthread_win32_thread_detach_np ()
{
  if (ptw32_processInitialized)
    {
      /*
       * Don't use pthread_self() - to avoid creating an implicit POSIX thread handle
       * unnecessarily.
       */
      ptw32_thread_t * sp = (ptw32_thread_t *) pthread_getspecific (ptw32_selfThreadKey);

      if (sp != NULL) // otherwise Win32 thread with no implicit POSIX handle.
	{
          ptw32_mcs_local_node_t stateLock;
	  ptw32_callUserDestroyRoutines (sp->ptHandle);

	  ptw32_mcs_lock_acquire (&sp->stateLock, &stateLock);
	  sp->state = PThreadStateLast;
	  /*
	   * If the thread is joinable at this point then it MUST be joined
	   * or detached explicitly by the application.
	   */
	  ptw32_mcs_lock_release (&stateLock);

          /*
           * Robust Mutexes
           */
          while (sp->robustMxList != NULL)
            {
              pthread_mutex_t mx = sp->robustMxList->mx;
              ptw32_robust_mutex_remove(&mx, sp);
              (void) PTW32_INTERLOCKED_EXCHANGE_LONG(
                       (PTW32_INTERLOCKED_LONGPTR)&mx->robustNode->stateInconsistent,
                       (PTW32_INTERLOCKED_LONG)-1);
              /*
               * If there are no waiters then the next thread to block will
               * sleep, wakeup immediately and then go back to sleep.
               * See pthread_mutex_lock.c.
               */
              SetEvent(mx->event);
            }


	  if (sp->detachState == PTHREAD_CREATE_DETACHED)
	    {
	      ptw32_threadDestroy (sp->ptHandle);

	      TlsSetValue (ptw32_selfThreadKey->key, NULL);
	    }
	}
    }

  return TRUE;
}

BOOL
pthread_win32_test_features_np (int feature_mask)
{
  return ((ptw32_features & feature_mask) == feature_mask);
}
