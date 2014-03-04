#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "lib/user/syscall.h"

static void syscall_handler (struct intr_frame *);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int call_num = * (int *) (f->esp);
	int first, second, third;

	//0 argument system calls
  if(call_num == SYS_HALT) {
  	halt();
  }

  first = * (int *) (++(f->esp));
  // printf("%d\n", * (int *) f->esp);
  //1 argument system calls
  if (call_num == SYS_EXIT) {
  	exit(first);
  } else if (call_num == SYS_EXEC) {

  } else if (call_num == SYS_WAIT) {

  } else if (call_num == SYS_REMOVE) {

  } else if (call_num == SYS_OPEN) {

  } else if (call_num == SYS_FILESIZE) {

  } else if (call_num == SYS_TELL) {

  } else if (call_num == SYS_CLOSE) {

  }

  second = * (int *) (++(f->esp));
  //2 argument system calls
  if (call_num == SYS_SEEK) {

  } else if (call_num == SYS_CREATE) {

  } 

  third = * (int *) (++(f->esp));
  //3 argument system calls
  if (call_num == SYS_READ) {

  } else if (call_num == SYS_WRITE) {

  } else {
  	printf("Invalid Call.\n");
  }

  thread_exit ();
}

void halt (void) {
	shutdown_power_off();
}

void exit (int status) {

}

// pid_t exec (const char *cmd_line) {
// 	return 0;
// }

// int wait (pid_t pid) {
// 	return 0;
// }

// bool create (const char *file, unsigned initial_size) {
// 	return false;
// }

// bool remove (const char *file) {
// 	return false;
// }

// int open (const char *file) {
// 	return 0;
// }

// int filesize (int fd) {
// 	return 0;
// }

// int read (int fd, void *buffer, unsigned size) {
// 	return 0;
// }

// int write (int fd, const void *buffer, unsigned size) {
// 	return 0;
// }

// void seek (int fd, unsigned position) {

// }

// unsigned tell (int fd) {
// 	return 0;
// }

// void close (int fd) {

// }