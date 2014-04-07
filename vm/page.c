#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/malloc.h"

//Global vars
// struct hash sup_table;
struct lock sup_lock;

//NOTE: The following hash functions are adapted from the hash
//table example in the pintos document sheet (Section A.8.5)

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

//END NOTE

/* Initialize supplemental page table for the thread */
void
sup_table_init (struct hash *sup_table)
{
	hash_init(sup_table, page_hash, page_less, NULL);
}

//TODO: ensure synchronization
void
insert_page (void *virt_address)
{
	struct page *p;

	p = malloc(sizeof(struct page));
	p->vaddr = virt_address;
	// p->owner = thread_current();
	//make page point to the frame?
	// p->frame = palloc_frame;
	// p->pte = pt_entry;
	//might need to put info about page table entry
	//this insert might not insert if there is a collision
	//should this insert be synchronized??
	// lock_acquire(&ft_lock);
	hash_insert(&thread_current()->sup_table, &p->hash_elem);
	// lock_release(&ft_lock);
}

//NOTE: The following function is adapted from the hash
//table example in the pintos document sheet (Section A.8.5)

/* Locate the page that faulted */
struct page *
page_lookup (void *virt_address)
{
	struct page lookup;
	struct hash_elem *e;

	lookup.vaddr = virt_address;
	e = hash_find(&thread_current()->sup_table, &lookup.hash_elem);
	return e ? hash_entry(e, struct page, hash_elem) : NULL;
}

//END NOTE

/*
TODO:
locate page that faulted 
exit process if all zero page, within kernel memory, or write to read-only
else return to page fault handler */
