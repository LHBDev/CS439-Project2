#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "userprog/exception.h"
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
#include "vm/page.h"

static void syscall_handler (struct intr_frame *);

/* Helper Functions */
bool esp_valid (void *esp);
bool pointer_valid (void * given_addr);
bool fd_valid (int fd);
struct file * fd_to_file (int fd);

/* Added global variables */
struct lock file_lock;

void
syscall_init (void) 
{
	//Above (header file also) added methods/declarations are driven by both
	//Ruben started driving
	lock_init(&file_lock);
	intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

/*Syscall Handler- We first check the validity of the esp location.
  Then, we obtain the call num that is esp is pointing to. We
  structured it such that we obtain arguments only as we need them in
  accordance with the appropriate system call. As we obtain the arguments,
  we check if their position validity as well.*/
static void
syscall_handler (struct intr_frame *f UNUSED) 
{
	int call_num, first_arg, second_arg, third_arg;

	thread_current()->user_esp = f->esp;
	//Checks the validity of the esp location
	if(!esp_valid(f->esp))
		exit(-1);

	//Get the system call number
	call_num =  * (int *) (f->esp);

	//0 argument system calls
	if(call_num == SYS_HALT)
		halt();

	//Get 1st argument and check it's validity
	first_arg = * (int *) (f->esp + WORD_LENGTH);
	if(!pointer_valid(f->esp + WORD_LENGTH))
		exit(-1);

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

	//Ruben stopped driving
	//Siva started driving

	//Get 2nd argument and check it's validity
	second_arg = * (int *) (f->esp + (2 * WORD_LENGTH));
	if(!pointer_valid(f->esp + 2* WORD_LENGTH))
		exit(-1);

	//2 argument system calls
	if (call_num == SYS_CREATE)
		{
			f->eax = create((const char *) first_arg, (unsigned) second_arg);
		}
	else if (call_num == SYS_SEEK)
		{
			seek(first_arg, (unsigned) second_arg);
		}

	//Get 3rd argument and check it's validity
	third_arg = * (int *) (f->esp + (3 * WORD_LENGTH));
	if(!pointer_valid(f->esp + 3 * WORD_LENGTH))
		exit(-1);

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
//Siva stopped driving

/*Halt system call- Simply calls the pow off method*/
//Ruben started driving
void
halt (void)
{
	shutdown_power_off();
}
//Ruben stopped driving

/*Exit system call - Closes the thread's exec file to reenable
  writing to it and saves the status directly to the child's 
  parent. We call sema down on exit sema to make the exiting
  child wait for the parent to start waiting. This prevents
  premature exiting of the children, a possible race condition.
  Then, wakes up the waiting parent and prints the proper 
  exit message, if it's a user process.*/

//Siva started driving
void
exit (int status)
{
	struct thread *cur = thread_current();

	lock_acquire(&file_lock);
	file_close(cur->exec_file);
	lock_release(&file_lock);

	sema_down(&cur->exit_sema);

	cur->parent->child_exit_status = status;
	cur->is_alive = false;

	sema_up(&cur->parent->wait_sema);
	if(cur->is_user_process)
		printf("%s: exit(%d)\n", cur->name, status);
	sup_table_free();

	thread_exit();
}

/*Exec system call - First checks the validity of the passed
  in argument and then calls the process execute method. This
  is done under a lock to prevent perturbing race conditions.
  Returns the value returned by process_execute(). */
pid_t
exec (const char *cmd_line)
{
	pid_t new_pid;

	if(!pointer_valid((void *) cmd_line))
		exit(-1);

	lock_acquire(&file_lock);
	new_pid = process_execute(cmd_line);
	lock_release(&file_lock);

	return new_pid;
}

//Siva stopped driving
//Ruben started driving

/*Wait system call - Implemented in terms of process_wait().
  Therefore, simply returns what process_wait returns.*/
int
wait (pid_t pid)
{
	return process_wait(pid);
}

/*Create system call - First checks validity of the char *
  file and thread is killed if necessary. Calls the provided
  filesys_create() method in filesys dir and this is done
  within a lock to ensure synchronization. Returns the result
  returned by filesys_create().*/
bool
create (const char *file, unsigned initial_size)
{
	bool success;

	if(!pointer_valid((void *) file))
		exit(-1);

	lock_acquire(&file_lock);
	success = filesys_create(file, initial_size);
	lock_release(&file_lock);
	return success;
}

/*Remove system call - First checks validity of the char *
  file. Calls the provided filesys_remove() method and this
  is done within a lock to ensure synchronization. 
  Returns the result returned by filesys_remove().*/
bool
remove (const char *file)
{
	bool success;

	if(!pointer_valid((void *) file))
		exit(-1);

	lock_acquire(&file_lock);
	success = filesys_remove(file);
	lock_release(&file_lock);
	return success;
}

//Ruben stopped driving
//Siva started driving

/*Open system call - First checks validity of the char *
  file. Then calls the provided filesys_open() method 
  and this is done within a lock to ensure synchronization.
 	If the file isn't NULL, the open file is stored in the 
 	thread's file_list and the fd is the index of the 
 	file_list (an array) that is returned. Otherwise, -1
 	is returned.*/
int
open (const char *file)
{
	struct file *actual_file;
	struct thread *cur = thread_current();
	int i, fd;
	
	if(!pointer_valid((void *) file))
		exit(-1);

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

/*Filesize system call - First checks validity of fd which is
	converted into a file by the helper method (done within a lock).
  Then returns the result of provided file_length() method 
  if the file is proper, else the ret val is 0.*/
int
filesize (int fd)
{
	struct file *fd_file;

	if(!fd_valid(fd))
		exit(-1);

	lock_acquire(&file_lock);
	fd_file = fd_to_file(fd);
	lock_release(&file_lock);

	if(!fd_file)
		return 0;

	return (int) file_length(fd_file);
}

/*Read system call - First checks validity of the buffer and fd. 
  If fd == STDIN_FILENO, we read a char at a time from stdin and store
  it in the buffer, returning size. Otherwise, we convert fd to its
  referenced file and use the provided file_read method to read from
  it. Both of the above operations are performed within a lock.*/
int
read (int fd, void *buffer, unsigned size)
{
	struct file *fd_file;
	int bytes_read = 0;
	unsigned size_copy = size;

	if(!pointer_valid(buffer) || !fd_valid(fd))
		exit(-1);

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

//Siva stopped driving
//Ruben started driving

/*Write system call - First checks validity of the buffer and fd.
	If fd == STDOUT_FILENO, use the putbuf method to write the entire
	buffer and return size. Otherwise, convert fd to its referenced
	file and call the provided file_write method to write to the file.
	Both again are done within a lock to ensure synchronization.*/
int
write (int fd, const void *buffer, unsigned size)
{
	struct file *fd_file;
	int bytes_written = 0;

	if(!pointer_valid((void *) buffer) || !fd_valid(fd))
		exit(-1);

	lock_acquire(&file_lock);
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

/*Seek system call - First checks the validity of fd. After 
	converting it to its referenced file, call the provided
	method file_seek within a lock.*/
void
seek (int fd, unsigned position)
{
	struct file *fd_file;

	if(!fd_valid(fd))
		exit(-1);

	lock_acquire(&file_lock);
	fd_file = fd_to_file(fd);
	if(fd_file)
		file_seek(fd_file, position);
	lock_release(&file_lock);
}

//Ruben stopped driving
//Siva started driving

/*Tell system call - First checks the validity of fd. After 
	converting it to its referenced file, call the provided
	method file_tell within a lock.*/
unsigned
tell (int fd)
{
	struct file *fd_file;
	unsigned result_pos = 0;

	if(!fd_valid(fd))
		exit(-1);

	lock_acquire(&file_lock);
	fd_file = fd_to_file(fd);
	if(fd_file)
		result_pos = file_tell(fd_file);
	lock_release(&file_lock);

	return result_pos;
}

/*Close system call - First checks the validity of fd. After 
	converting it to its referenced file, call the provided
	method file_close within a lock. Then NULL out the proper
	entry from the thread's file's list.*/
void
close (int fd)
{
	struct file *fd_file;
	struct thread *cur = thread_current();
 
	if(!fd_valid(fd))
		exit(-1);

	lock_acquire(&file_lock);
	fd_file = fd_to_file(fd);
	file_close(fd_file);
	lock_release(&file_lock);

	cur->file_list[fd] = NULL;
}
//Siva stopped driving

/*Start of helper methods*/

/*Checks the validity of a passed in user address. Can't 
	trust those foos.*/
bool
esp_valid (void *esp)
{
	struct page *spt_entry;
	void *fault_vpage, *kpage;

	if(!esp || is_kernel_vaddr(esp))
		return false;

	fault_vpage = pg_round_down(esp);
	spt_entry = lookup_page(fault_vpage, thread_current());

	if(!spt_entry)
	{
		if(fault_vpage >= esp - 32 && 
			 fault_vpage >= (PHYS_BASE - STACK_LIMIT))
			stack_growth (fault_vpage, kpage);
		else
			return false;
	}
	return true;
}

//Ruben started driving
bool
pointer_valid (void *given_addr)
{
	// struct page *spt_entry = lookup_page(pg_round_down(given_addr));

	if(!(given_addr) || is_kernel_vaddr(given_addr))
		return false;

	if(!pagedir_get_page(thread_current()->pagedir, given_addr))
		return false;

	return true;
}
//Ruben stopped driving

/*Checks the validity of a passed in nm. Still can't trust
	those foos.*/
//Siva started driving
bool
fd_valid (int fd)
{
	if(fd < 0 || fd >= MAX_FILES)
		return false;
	return true;
}

/*Returns a file pointer (struct file *) given a fd number.
	Basically looks through the thread's file list and obtains
	the entry associate with fd. If fd is not present, returns NULL.*/
struct file *
fd_to_file (int fd)
{
	struct thread *cur = thread_current();
	struct file *fd_file = NULL;
	int i;

	for(i = 2; i < MAX_FILES; i++)
			if(fd == i)
				fd_file = cur->file_list[i];

	return fd_file;
}
//Siva stopped driving

/*End of helper methods*/
