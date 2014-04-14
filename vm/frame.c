#include "frame.h"
#include "swap.h"
#include "page.h"
#include "lib/kernel/hash.h"
#include "lib/random.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

//Global Vars
struct hash frame_table;

//Header code and above code driven by both Ruben and Siva

//P3 Added Code: (This hash code is slightly adapted from the from the hash
//table example in the pintos document sheet (Section A.8.5)).
//Hash functions for our frame table.

//Siva started driving
/* Returns a hash value for frame p. */
unsigned
frame_hash (const struct hash_elem *f_, void *aux UNUSED)
{
	const struct frame *f = hash_entry (f_, struct frame, hash_elem);
	return hash_bytes (&f->pte, sizeof f->pte);
}

/* Returns true if frame a precedes frame b. */
bool
frame_less (const struct hash_elem *a_, const struct hash_elem *b_,
					 void *aux UNUSED)
{
	const struct frame *a = hash_entry (a_, struct frame, hash_elem);
	const struct frame *b = hash_entry (b_, struct frame, hash_elem);

	return a->pte < b->pte;
}

//Initialize the hash table and the lock
void
frame_init (void)
{
	hash_init(&frame_table, frame_hash, frame_less, NULL);
	lock_init(&ft_lock);
}
//Siva stopped driving

//P3 Added code; Method first tries to palloc a page from memory and 
//if it's a valid page, we add a frame table entry corresponding to 
//that page. Otherwise, we evict a page and then add the entry. We hash 
//the frame table entry based on the virt. addr. of the corresponding 
//page and the zero-page flag indicates if we have to zero out the palloc.

//Ruben and Siva started driving
void *
obtain_frame (void *pt_entry, bool zero_page)
{
	struct frame *f;
	void *palloc_frame;

	palloc_frame = zero_page ? palloc_get_page(PAL_USER | PAL_ZERO)
													 : palloc_get_page(PAL_USER);

	//Returned frame is null, need to evict from memory
	if(!palloc_frame)
		palloc_frame = evict_frame(zero_page);
	else
		{
			lock_acquire(&ft_lock);
			f = malloc(sizeof(struct frame));
			f->owner = thread_current();
			f->pte = pt_entry;
			f->frame_addr = palloc_frame;
			f->pinned = true;
			hash_insert(&frame_table, &f->hash_elem);
			lock_release(&ft_lock);
		}

	return palloc_frame;
}
//Ruben and Siva stopped driving

//P3 Added Code - To free a frame, we first hash into the frame entry with
//the pt_entry key and delete that entry from the frame table (freeing the
//struct as well). Also, we free the corresponding palloced page as referred
//by frame_addr.

//Ruben started driving
void
free_frame (void *pt_entry, void *frame_addr)
{
	struct frame *target;

	lock_acquire(&ft_lock);
	target = lookup_frame(pt_entry);
	if(target)
		{
			hash_delete(&frame_table, &target->hash_elem);
			free(target);
			palloc_free_page(frame_addr);
		}
	lock_release(&ft_lock);
}
//Ruben stopped driving

//P3 Added Code: With the key *pte, we lookup the corresponding hash
//entry and return the pointer. (Code adapted from hash example)

//Siva started driving
struct frame *
lookup_frame (void *pte)
{
	struct frame lookup;
	struct hash_elem *e;

	lookup.pte = pte;
	e = hash_find(&frame_table, &lookup.hash_elem);
	return e ? hash_entry(e, struct frame, hash_elem) : NULL;
}
//Siva stopped driving

//P3 Added Code - Random frame eviction. We generate a random
//number that is up till 5 and as we evict the page that is 
//present at the location specified by the random number

//Ruben and Siva started driving
void *
evict_frame (bool zero_page)
{
	struct hash_iterator it;
  struct frame *f;
  struct page *spt_entry;
  void *vpage = NULL, *palloc_frame;
  int random = random_ulong() % 5, ind = 0;

  lock_acquire(&ft_lock);
  hash_first(&it, &frame_table);
  while (hash_next (&it))
    {
    	if(ind++ == random)
    		{
		      f = hash_entry (hash_cur (&it), struct frame, hash_elem);
		      spt_entry = lookup_page(f->pte, f->owner);

		      palloc_frame = f->frame_addr;

		      if(!f->pinned)
		      	{
				      swap_out(f->pte);
				      spt_entry->has_loaded = false;
					    spt_entry->type = IN_SWAP;
				     	break;
			    	}
		  	}
    }

  lock_release(&ft_lock);

  return palloc_frame;
}
//Ruben and Siva stopped driving
