#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <hash.h>
#include <stdint.h>
#include <threads/synch.h>

/* States in a thread's life cycle. */
enum thread_status
	{
		THREAD_RUNNING,     /* Running thread. */
		THREAD_READY,       /* Not running but ready to run. */
		THREAD_BLOCKED,     /* Waiting for an event to trigger. */
		THREAD_DYING        /* About to be destroyed. */
	};

/* Thread identifier type.
	 You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/*Added global constants*/
//Ruben started driving
#define MAX_ARGS  128           /* Maximum number of cmd line args */
#define WORD_LENGTH 4           /* Length of a word in Pintos */
#define STACK_LIMIT (1 << 23)   /* How much stack can grow to - 8MB */
#define MAX_FILES 128			/* Maximum number of files a thread can have */
//Ruben stopped driving

/* A kernel thread or user process.

	 Each thread structure is stored in its own 4 kB page.  The
	 thread structure itself sits at the very bottom of the page
	 (at offset 0).  The rest of the page is reserved for the
	 thread's kernel stack, which grows downward from the top of
	 the page (at offset 4 kB).  Here's an illustration:

				4 kB +---------------------------------+
						 |          kernel stack           |
						 |                |                |
						 |                |                |
						 |                V                |
						 |         grows downward          |
						 |                                 |
						 |                                 |
						 |                                 |
						 |                                 |
						 |                                 |
						 |                                 |
						 |                                 |
						 |                                 |
						 +---------------------------------+
						 |              magic              |
						 |                :                |
						 |                :                |
						 |               name              |
						 |              status             |
				0 kB +---------------------------------+

	 The upshot of this is twofold:

			1. First, `struct thread' must not be allowed to grow too
				 big.  If it does, then there will not be enough room for
				 the kernel stack.  Our base `struct thread' is only a
				 few bytes in size.  It probably should stay well under 1
				 kB.

			2. Second, kernel stacks must not be allowed to grow too
				 large.  If a stack overflows, it will corrupt the thread
				 state.  Thus, kernel functions should not allocate large
				 structures or arrays as non-static local variables.  Use
				 dynamic allocation with malloc() or palloc_get_page()
				 instead.

	 The first symptom of either of these problems will probably be
	 an assertion failure in thread_current(), which checks that
	 the `magic' member of the running thread's `struct thread' is
	 set to THREAD_MAGIC.  Stack overflow will normally change this
	 value, triggering the assertion.  (So don't add elements below 
	 THREAD_MAGIC.)
*/
/* The `elem' member has a dual purpose.  It can be an element in
	 the run queue (thread.c), or it can be an element in a
	 semaphore wait list (synch.c).  It can be used these two ways
	 only because they are mutually exclusive: only a thread in the
	 ready state is on the run queue, whereas only a thread in the
	 blocked state is on a semaphore wait list. */
struct thread
	{
		/* Owned by thread.c. */
		tid_t tid;                          /* Thread identifier. */
		enum thread_status status;          /* Thread state. */
		char name[16];                      /* Name (for debugging purposes). */
		uint8_t *stack;                     /* Saved stack pointer. */
		int priority;                       /* Priority. */
		struct list_elem allelem;           /* List element for all threads list. */

		/* Shared between thread.c and synch.c. */
		struct list_elem elem;              /* List element. */

#ifdef USERPROG
		/* Owned by userprog/process.c. */
		uint32_t *pagedir;                  /* Page directory. */
#endif

		/* Owned by thread.c. */
		unsigned magic;                     /* Detects stack overflow. */

		/* Added Variables */
		//Siva and Ruben started driving
		bool is_user_process;               /* True if a user process*/
		bool has_loaded_process;            /* Has child loaded file */
		bool has_waited;                    /* Has child been waited on before */
		bool is_alive;                      /* Has the child exited or not */
		int child_exit_status;              /* Exit status of A child */
		struct semaphore load_sema;        	/* Waits the parent for child load */
		struct semaphore wait_sema;        	/* Waits the parent for child exit */
		struct semaphore exit_sema;			/* Waits child for parent to reap */
		struct thread *parent;              /* Pointer to the child's parent */
		struct file *file_list[MAX_FILES];  /* List of files that a thread has */
		struct file *exec_file;				/* Exec file that thread is running */

		//Project 3 Code
		struct hash sup_table;				/* Sup. page table for each thread */
		void *user_esp;						/* The current val for esp */
		//Siva and Ruben stopped driving
	};

/* If false (default), use round-robin scheduler.
	 If true, use multi-level feedback queue scheduler.
	 Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_exit (void) NO_RETURN;
void thread_yield (void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

/*Added method*/
struct thread * tid_to_thread(tid_t tid);
struct thread * get_a_thread(tid_t tid);

#endif /* threads/thread.h */
