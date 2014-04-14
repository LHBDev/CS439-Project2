#include "userprog/exception.h"
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <debug.h>
#include "userprog/gdt.h"
#include "userprog/syscall.h"
#include "userprog/process.h"
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"

/* Number of page faults processed. */
static long long page_fault_cnt;

static void kill (struct intr_frame *);
static void page_fault (struct intr_frame *);

/* Registers handlers for interrupts that can be caused by user
	 programs.

	 In a real Unix-like OS, most of these interrupts would be
	 passed along to the user process in the form of signals, as
	 described in [SV-386] 3-24 and 3-25, but we don't implement
	 signals.  Instead, we'll make them simply kill the user
	 process.

	 Page faults are an exception.  Here they are treated the same
	 way as other exceptions, but this will need to change to
	 implement virtual memory.

	 Refer to [IA32-v3a] section 5.15 "Exception and Interrupt
	 Reference" for a description of each of these exceptions. */
void
exception_init (void) 
{
	/* These exceptions can be raised explicitly by a user program,
		 e.g. via the INT, INT3, INTO, and BOUND instructions.  Thus,
		 we set DPL==3, meaning that user programs are allowed to
		 invoke them via these instructions. */
	intr_register_int (3, 3, INTR_ON, kill, "#BP Breakpoint Exception");
	intr_register_int (4, 3, INTR_ON, kill, "#OF Overflow Exception");
	intr_register_int (5, 3, INTR_ON, kill,
										 "#BR BOUND Range Exceeded Exception");

	/* These exceptions have DPL==0, preventing user processes from
		 invoking them via the INT instruction.  They can still be
		 caused indirectly, e.g. #DE can be caused by dividing by
		 0.  */
	intr_register_int (0, 0, INTR_ON, kill, "#DE Divide Error");
	intr_register_int (1, 0, INTR_ON, kill, "#DB Debug Exception");
	intr_register_int (6, 0, INTR_ON, kill, "#UD Invalid Opcode Exception");
	intr_register_int (7, 0, INTR_ON, kill,
										 "#NM Device Not Available Exception");
	intr_register_int (11, 0, INTR_ON, kill, "#NP Segment Not Present");
	intr_register_int (12, 0, INTR_ON, kill, "#SS Stack Fault Exception");
	intr_register_int (13, 0, INTR_ON, kill, "#GP General Protection Exception");
	intr_register_int (16, 0, INTR_ON, kill, "#MF x87 FPU Floating-Point Error");
	intr_register_int (19, 0, INTR_ON, kill,
										 "#XF SIMD Floating-Point Exception");

	/* Most exceptions can be handled with interrupts turned on.
		 We need to disable interrupts for page faults because the
		 fault address is stored in CR2 and needs to be preserved. */
	intr_register_int (14, 0, INTR_OFF, page_fault, "#PF Page-Fault Exception");
}

/* Prints exception statistics. */
void
exception_print_stats (void) 
{
	printf ("Exception: %lld page faults\n", page_fault_cnt);
}

/* Handler for an exception (probably) caused by a user process. */
static void
kill (struct intr_frame *f) 
{
	/* This interrupt is one (probably) caused by a user process.
		 For example, the process might have tried to access unmapped
		 virtual memory (a page fault).  For now, we simply kill the
		 user process.  Later, we'll want to handle page faults in
		 the kernel.  Real Unix-like operating systems pass most
		 exceptions back to the process via signals, but we don't
		 implement them. */
		 
	/* The interrupt frame's code segment value tells us where the
		 exception originated. */
	switch (f->cs)
		{
		case SEL_UCSEG:
			/* User's code segment, so it's a user exception, as we
				 expected.  Kill the user process.  */
			printf ("%s: dying due to interrupt %#04x (%s).\n",
							thread_name (), f->vec_no, intr_name (f->vec_no));
			intr_dump_frame (f);
			thread_exit (); 

		case SEL_KCSEG:
			/* Kernel's code segment, which indicates a kernel bug.
				 Kernel code shouldn't throw exceptions.  (Page faults
				 may cause kernel exceptions--but they shouldn't arrive
				 here.)  Panic the kernel to make the point.  */
			intr_dump_frame (f);
			PANIC ("Kernel bug - unexpected interrupt in kernel"); 

		default:
			/* Some other code segment?  Shouldn't happen.  Panic the
				 kernel. */
			printf ("Interrupt %#04x (%s) in unknown segment %04x\n",
						 f->vec_no, intr_name (f->vec_no), f->cs);
			thread_exit ();
		}
}

/* Page fault handler.  This is a skeleton that must be filled in
	 to implement virtual memory.  Some solutions to project 2 may
	 also require modifying this code.

	 At entry, the address that faulted is in CR2 (Control Register
	 2) and information about the fault, formatted as described in
	 the PF_* macros in exception.h, is in F's error_code member.  The
	 example code here shows how to parse that information.  You
	 can find more information about both of these in the
	 description of "Interrupt 14--Page Fault Exception (#PF)" in
	 [IA32-v3a] section 5.15 "Exception and Interrupt Reference". */

static void
page_fault (struct intr_frame *f) 
{
	bool not_present;  /* True: not-present page, false: writing r/o page. */
	bool write;        /* True: access was write, false: access was read. */
	bool user;         /* True: access by user, false: access by kernel. */
	void *fault_addr;  /* Fault address. */

	//Added variables
	void *fault_vpage; /* Virtual page of the fault address. */
	void *actual_esp;  /* The appropriate esp for the user process. */
	struct page *spt_entry; /* Sup. table entry for the fault addr. */
	struct thread *cur = thread_current();  /* The currently running thread */
	bool get_lock = false;

	/* Obtain faulting address, the virtual address that was
		 accessed to cause the fault.  It may point to code or to
		 data.  It is not necessarily the address of the instruction
		 that caused the fault (that's f->eip).
		 See [IA32-v2a] "MOV--Move to/from Control Registers" and
		 [IA32-v3a] 5.15 "Interrupt 14--Page Fault Exception
		 (#PF)". */
	asm ("movl %%cr2, %0" : "=r" (fault_addr));

	/* Turn interrupts back on (they were only off so that we could
		 be assured of reading CR2 before it changed). */
	intr_enable ();

	/* Count page faults. */
	page_fault_cnt++;

	/* Determine cause. */
	not_present = (f->error_code & PF_P) == 0;
	write = (f->error_code & PF_W) != 0;
	user = (f->error_code & PF_U) != 0;

	//Ruben started driving
	if(!not_present || is_kernel_vaddr(fault_addr) || !fault_addr)
		exit(-1);
	//Ruben stopped driving

	//P3 Added Code: Code only considers faulting in not_present pages.
	//We first lookup the sup. table entry for the faulting page and if
	//it is not present, we evaluate a stack heuristic on the fault addr
	//to see if we need to grow the stack and o/w we exit. If the entry is
	//present we load the page from either file or swap.

	//Siva started driving
	if(lock_held_by_current_thread (&file_lock)) {
		get_lock = true;
		lock_release(&file_lock);
	}

	if(not_present)
		{
			fault_vpage = pg_round_down(fault_addr);
			spt_entry = lookup_page(fault_vpage, cur);
			//spt does not contain this page, illegal access or stack growth?
			if(!spt_entry)
				{
					//Obtain current value of user program's stack pointer
					if(!user)
						actual_esp = cur->user_esp;
					else
						actual_esp = f->esp;
					
					//Check if we need to allow stack growth (Stack heuristic)
					if(fault_addr >= actual_esp - 32 &&
						 fault_vpage >= (PHYS_BASE - STACK_LIMIT))
							stack_growth(fault_vpage);
					else
						exit(-1);
				}
			//Siva stopped driving
			//Ruben started driving
			else
				{
					load_file_swap(fault_vpage);
				}
		}

	if(get_lock)
		lock_acquire(&file_lock);
	//Ruben stopped driving

}

//P3 Added code: We record the new page's information in
//the sup. table and then we obtain a frame and install it.

//Siva started driving
void
stack_growth (void *fault_vpage)
{
	struct page *spt_entry;
	struct frame *f;
	void *kpage;

	spt_entry = insert_page(fault_vpage);
	spt_entry->type = IN_SWAP;
	spt_entry->has_loaded = true;
	spt_entry->read_only = false;

	kpage = obtain_frame(fault_vpage, true);
	f = lookup_frame(fault_vpage);

	if (kpage)
		if (!install_page (fault_vpage, kpage, true))
			free_frame(fault_vpage, kpage);

	f->pinned = false;
		
}
//Siva stopped driving

//P3 Added code: We identify the page location from the sup. table and
//we respectively load the page from that source. If the page is in the
//swap, we simply swap in the page and otherwise, we read from the file and
//transfer the data onto the page.

//Ruben started driving
void
load_file_swap (void *fault_vpage)
{
	struct page *spt_entry = lookup_page(fault_vpage, thread_current());
	struct frame *f;
	void *kpage;

	//If it has loaded, do not do it again!
	if(spt_entry->has_loaded)
		return;

	//Load from file
	if(spt_entry->type == IN_FILE)
		{
			kpage = obtain_frame(fault_vpage, spt_entry->zero_page);
			file_seek (spt_entry->load_file, spt_entry->file_ofs);
			if (file_read(spt_entry->load_file, kpage, spt_entry->read_bytes) != (int) spt_entry->read_bytes)
				{
					free_frame(fault_vpage, kpage);
					return;
				}
			memset (kpage + spt_entry->read_bytes, 0, (PGSIZE - spt_entry->read_bytes));

			 // Add the page to the process's address space. 
			if (!install_page (fault_vpage, kpage, (!spt_entry->read_only)))
				{
					free_frame(fault_vpage, kpage);
					return;
				}
		}
	//Load from swap
	else if(spt_entry->type == IN_SWAP)
		{
			kpage = swap_in(spt_entry->vaddr, spt_entry->swp);
			if (!install_page (fault_vpage, kpage, (!spt_entry->read_only)))
				{
					free_frame(fault_vpage, kpage);
					return;
				}
		}

	f = lookup_frame(fault_vpage);

	spt_entry->has_loaded = true;
}
//Ruben stopped driving
