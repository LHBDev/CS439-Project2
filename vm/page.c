#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

//Header code and above code driven by both Ruben and Siva

//P3 Added Code: (This hash code is slightly adapted from the from the hash
//table example in the pintos document sheet (Section A.8.5)).
//Hash functions for our sup. page table.

//Ruben started driving
/* Returns a hash value for spte p. */
unsigned
page_hash (const struct hash_elem *p_, void *aux UNUSED)
{
	const struct page *p = hash_entry (p_, struct page, hash_elem);
	return hash_bytes (&p->vaddr, sizeof p->vaddr);
}

/* Returns true if page a precedes page b. */
bool
page_less (const struct hash_elem *a_, const struct hash_elem *b_,
					 void *aux UNUSED)
{
	const struct page *a = hash_entry (a_, struct page, hash_elem);
	const struct page *b = hash_entry (b_, struct page, hash_elem);

	return a->vaddr < b->vaddr;
}

/* Destructor function that frees sup. entries */
void
page_action_func (struct hash_elem *a_, void *aux UNUSED)
{
	struct page *a = hash_entry (a_, struct page, hash_elem);
	free_page(a);
}

/* Initialize supplemental page table for the thread */
void
sup_table_init (struct hash *sup_table)
{
	hash_init(sup_table, page_hash, page_less, NULL);
}

//P3 Added code - A destructor function that frees the sup. page table
void
sup_table_free (void)
{
	struct thread *cur = thread_current();

	hash_destroy(&cur->sup_table, page_action_func);
}
//Ruben stopped driving

//P3 Added Code - Simply allocates an entry for the sup. page table and
//inserts into the table. Other properties are added beside where this 
//function is called

//Siva started driving
struct page *
insert_page (void *virt_address)
{
	struct page *p;
	struct thread *cur = thread_current();

	p = malloc(sizeof(struct page));
	p->vaddr = pg_round_down(virt_address);
	hash_insert(&cur->sup_table, &p->hash_elem);

	return p;
}

//P3 Added Code - Called by the destructor hash function above. This simply
//frees the struct allocated to the sup. page table. Also if the frame is
//still loaded in memory, we should free up the allocated frame and clear
//its entry from the page table
void
free_page (struct page *p)
{
	struct thread *cur = thread_current();
	void *frame_addr = pagedir_get_page(cur->pagedir, p->vaddr);

	if(p->has_loaded)
		{
			pagedir_clear_page(cur->pagedir, p->vaddr);
			free_frame(p->vaddr, frame_addr);
		}
	free(p);
}
//Siva stopped driving

//P3 Added Code - lookup the sup. table entry corresponding to the key
//virt_address. The owner specifies which thread's sup. table to look in.
//(Code adapted from hash example)

//Ruben started driving

/* Locate the page that faulted */
struct page *
lookup_page (void *virt_address, struct thread *owner)
{
	struct thread *cur = owner;
	struct page lookup;
	struct hash_elem *e;

	lookup.vaddr = pg_round_down(virt_address);
	e = hash_find(&cur->sup_table, &lookup.hash_elem);
	return e ? hash_entry(e, struct page, hash_elem) : NULL;
}
//Ruben stopped driving
