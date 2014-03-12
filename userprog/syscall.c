#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "lib/user/syscall.h"
#include "devices/shutdown.h"
#include "devices/input.h"

static void syscall_handler (struct intr_frame *);

/*Helper Functions*/
bool pointer_valid (void * given_addr);
struct file * fd_to_file (int fd);

/*Added global variables*/
struct lock file_lock;

void
syscall_init (void) 
{
  lock_init(&file_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int first_arg, second_arg, third_arg;
  //Get the system call number
  int call_num = * (int *) (f->esp);

	//0 argument system calls
  if(call_num == SYS_HALT)
    halt();

  f->esp += WORD_LENGTH;
  first_arg = * (int *) (f->esp);
  //1 argument system calls
  if (call_num == SYS_EXIT) 
    {
      exit(first_arg);
    } 
  else if (call_num == SYS_EXEC)
    {
      f->eax = exec((const char *) first_arg);
    }
  else if (call_num == SYS_WAIT)
    {
      f->eax = wait((pid_t) first_arg);
    }
  else if (call_num == SYS_REMOVE)
    {
      f->eax = remove((const char *) first_arg);
    }
  else if (call_num == SYS_OPEN)
    {
      f->eax = open((const char *) first_arg);
    }
  else if (call_num == SYS_FILESIZE)
    {
      f->eax = filesize(first_arg);
    }
  else if (call_num == SYS_TELL)
    {
      f->eax = tell(first_arg);
    }
  else if (call_num == SYS_CLOSE)
    {
      close(first_arg);
    }

  f->esp += WORD_LENGTH;
  second_arg = * (int *) (f->esp);
  //2 argument system calls
  if (call_num == SYS_CREATE)
    {
      f->eax = create((const char *) first_arg, (unsigned) second_arg);
    }
  else if (call_num == SYS_SEEK)
    {
      seek(first_arg, (unsigned) second_arg);
    }

  f->esp += WORD_LENGTH;
  third_arg = * (int *) (f->esp);
  //3 argument system calls
  if (call_num == SYS_READ)
    {
      f->eax = read(first_arg, (void *) second_arg, (unsigned) third_arg);
    }
  else if (call_num == SYS_WRITE)
    { 
      f->eax = write(first_arg, (const void *) second_arg, (unsigned) third_arg);
    }
}

void
halt (void)
{
	shutdown_power_off();
}

void
exit (int status)
{
  //Possibly notify a waiting parent process of status
  //TODO
  thread_exit ();
  printf("%s: exit(%d)\n", thread_current()->name, status);
}

pid_t
exec (const char *cmd_line)
{
	return 0;
}

int
wait (pid_t pid)
{
	return 0;
}

bool
create (const char *file, unsigned initial_size)
{
  bool success;
  //ECC here

  lock_acquire(&file_lock);
  success = filesys_create (file, initial_size);
  lock_release(&file_lock);
	return success;
}

bool
remove (const char *file)
{
  bool success;
  //ECC here

  lock_acquire(&file_lock);
  success = filesys_remove(file);
  lock_release(&file_lock);
	return success;
}

int
open (const char *file)
{
  struct file *actual_file;
  struct thread *cur = thread_current();
  int i, fd;
  //ECC here

  lock_acquire(&file_lock);
  actual_file = filesys_open(file);
  lock_release(&file_lock);

  //File cannot be opened, some error
  if(!actual_file)
    return -1;

  //put the file pointer into the thread's file_list
  for(i = 2; i < MAX_FILES; i++)
    if(!cur->file_list[i])
      break;
  //insert the file into the position
  fd = i;
  cur->file_list[fd] = actual_file;
	return fd;
}

int
filesize (int fd)
{
  struct file *fd_file;

  lock_acquire(&file_lock);
  fd_file = fd_to_file(fd);
  lock_release(&file_lock);

  //ECC here, might need to change
  if(!fd_file)
    return 0;

  return (int) file_length(fd_file);
}

int
read (int fd, void *buffer, unsigned size)
{
  struct file *fd_file;
  int bytes_read = 0;
  unsigned size_copy = size;
  //ECC here

  lock_acquire(&file_lock);
  //Read from stdin
  if(fd == STDIN_FILENO)
    {
      while(size_copy-- > 0)
        * (uint8_t *) buffer++ = input_getc();
      bytes_read = size;
    }
  //Read from the fd file
  else
    {
      fd_file = fd_to_file(fd);
      if(!fd_file)
        bytes_read = -1;
      else
        bytes_read = file_read(fd_file, buffer, size);
    }
  lock_release(&file_lock);

	return bytes_read;
}

int
write (int fd, const void *buffer, unsigned size)
{
  struct file *fd_file;
  int bytes_written = 0;
  //ECC here

  lock_acquire(&file_lock);
  //Write to stdout
  if(fd == STDOUT_FILENO)
    {
      putbuf(buffer, size);
      bytes_written = size;
    }
  else
    {
      fd_file = fd_to_file(fd);
      if(fd_file)
        bytes_written = file_write(fd_file, buffer, size);
    }
  lock_release(&file_lock);
  return bytes_written;
}

void
seek (int fd, unsigned position)
{
  struct file *fd_file;

  lock_acquire(&file_lock);
  fd_file = fd_to_file(fd);
  if(fd_file)
    file_seek(fd_file, position);
  lock_release(&file_lock);
}

unsigned
tell (int fd)
{
  struct file *fd_file;
  unsigned result_pos = 0;

  lock_acquire(&file_lock);
  fd_file = fd_to_file(fd);
  if(fd_file)
    result_pos = file_tell(fd_file);
  lock_release(&file_lock);

	return result_pos;
}

void
close (int fd)
{
  struct file *fd_file;
  struct thread *cur = thread_current();
  //ECC here

  lock_acquire(&file_lock);
  fd_file = fd_to_file(fd);
  file_close(fd_file);
  lock_release(&file_lock);

  cur->file_list[fd] = NULL;
}

/*Start of helper methods*/
/*Checks the validity of a passed in user address. Can't 
  trust those foos. */
bool
pointer_valid (void * given_addr)
{
  if(!given_addr || is_kernel_vaddr(given_addr))
    return false;
  //ALSO CHECK IF VADDR IS UNMAPPED OR NOT
  return true;
}

/*Returns a file pointer (struct file *) given a fd number.
  Basically looks through the thread's file list and obtains
  the entry associate with fd. If fd is not present, returns NULL. */
struct file *
fd_to_file (int fd)
{
  struct thread *cur = thread_current();
  struct file *fd_file = NULL;
  int i;

  for(i = 2; i < MAX_FILES; i++)
      if(fd == i)
        fd_file = cur->file_list[i];

  // for (e = list_begin (&cur->file_list); e != list_end (&cur->file_list);
  //      e = list_next (e))
  //   {
  //     f = list_entry (e, struct thread_file, file_elem);
  //     if(f->fd == fd)
  //       fd_file = f->curr_file;
  //   }

  return fd_file;
}
