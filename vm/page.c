#include "vm/page.h"
#include "vm/frame.h"
#include "vm/swap.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

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

void
sup_table_free (void)
{
	struct thread *cur = thread_current();

	hash_destroy(&cur->sup_table, page_action_func);
}

struct page *
insert_page (void *virt_address)
{
	struct page *p;
	struct thread *cur = thread_current();

	p = malloc(sizeof(struct page));
	p->vaddr = pg_round_down(virt_address);
	// printf("%08x\n", p->vaddr);
	hash_insert(&cur->sup_table, &p->hash_elem);

	return p;
}

void
free_page (struct page *p)
{
	struct thread *cur = thread_current();
	// void *frame_addr = lookup_frame(p->vaddr)->frame_addr;

	if(p->has_loaded) {
		// free_frame(p->vaddr);
		// printf("10 free %08x\n", pagedir_get_page(cur->pagedir, p->vaddr));
		pagedir_clear_page(cur->pagedir, p->vaddr);
	}
	free(p);
}

//NOTE: The following function is adapted from the hash
//table example in the pintos document sheet (Section A.8.5)

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

//END NOTE
