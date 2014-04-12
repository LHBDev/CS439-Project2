#include "frame.h"
#include "swap.h"
#include "page.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"
#include "userprog/pagedir.h"

//Global Vars
struct hash frame_table;
struct lock ft_lock;
struct hash_elem *hand;

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

void
frame_init (void)
{
	hash_init(&frame_table, frame_hash, frame_less, NULL);
	lock_init(&ft_lock);
	hand = NULL;
}

//combine allocing of frames and getting a frame
void *
obtain_frame (uint8_t *pt_entry, bool zero_page)
{
	struct frame *f;
	void *palloc_frame;

	palloc_frame = zero_page ? palloc_get_page(PAL_USER | PAL_ZERO)
													 : palloc_get_page(PAL_USER);
	//Returned frame is null, need to evict
	if(!palloc_frame)
		palloc_frame = evict_frame(zero_page);
	else
		{
			lock_acquire(&ft_lock);
			f = malloc(sizeof(struct frame));
			f->owner = thread_current();
			f->pte = pt_entry;
			f->frame_addr = palloc_frame;
			hash_insert(&frame_table, &f->hash_elem);
			lock_release(&ft_lock);
		}

	return palloc_frame;
}

void
free_frame (uint8_t *pt_entry, void *frame_addr)
{
	struct frame *target;

	lock_acquire(&ft_lock);
	target = lookup_frame(pt_entry);
	if(target)
		{
			// frame_addr = target->frame_addr;
			// printf("Free: %08x\n", (unsigned int) pt_entry);
			// printf("frame_addr %08x\n", frame_addr);
			hash_delete(&frame_table, &target->hash_elem);
			free(target);
			palloc_free_page(frame_addr);
		}
	lock_release(&ft_lock);
}

//lookup a frame, just find something empty (null pointer/bits)
struct frame *
lookup_frame (uint8_t *pte)
{
	struct frame lookup;
	struct hash_elem *e;

	lookup.pte = pte;
	e = hash_find(&frame_table, &lookup.hash_elem);
	return e ? hash_entry(e, struct frame, hash_elem) : NULL;
}

void *
evict_frame (bool zero_page)
{
	struct hash_iterator it;
  struct frame *f;
  struct page *spt_entry;
  void *vpage = NULL, *palloc_frame;

  if(hand)
    it.elem = hand;
  else
    hash_first(&it, &frame_table);

  lock_acquire(&ft_lock);
  while(!vpage)
  {
    f = hash_entry(hash_cur(&it), struct frame, hash_elem);
    spt_entry = lookup_page(f->pte, f->owner);

    if(!pagedir_is_accessed(f->owner->pagedir, &f->pte))
    	{
        if(!pagedir_is_dirty(f->owner->pagedir, &f->pte))
	        {
	          if(f->remember_dirty)
	            swap_out(f->pte);

	          vpage = &f->pte;
	          spt_entry->has_loaded = false;
	          spt_entry->type = IN_SWAP;
	          f->remember_dirty = false;

	          pagedir_clear_page(f->owner->pagedir, &f->pte);
	          palloc_free_page(&f->pte);
	          palloc_frame = zero_page ? palloc_get_page(PAL_USER | PAL_ZERO)
	                    							 : palloc_get_page(PAL_USER);
	          lock_release(&ft_lock);
	          return palloc_frame;
	        }
        else
	        {
	          pagedir_set_dirty(f->owner->pagedir, &f->pte, false);
	          f->remember_dirty = true;
	        }
   	 }
    else
    	{
      	pagedir_set_accessed(f->owner->pagedir, &f->pte, false);
    	}
    	
    hand = hash_cur(&it);
    if(!hash_next(&it))
      hash_first(&it, &frame_table);
  }

  NOT_REACHED();
}

