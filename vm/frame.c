#include "frame.h"
#include "swap.h"
#include "page.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"

//Global Vars
struct hash frame_table;
struct lock ft_lock;

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
}

//need to allocate all the user pages, also need to free them sometime
//free_frames and alloc_frames?

//combine allocing of frames and getting a frame
void *
obtain_frame (uint8_t *pt_entry)
{
  struct frame *f;
  void *palloc_frame;

  palloc_frame = palloc_get_page(PAL_USER);

  //Returned frame is not null, put the user page in the free frame
  if(palloc_frame)
    {
      f = malloc(sizeof(struct frame));
      f->owner = thread_current();
      f->frame = palloc_frame;
      f->pte = pt_entry;
      //might need to put info about page table entry
      //this insert might not insert if there is a collision
      //should this insert be synchronized??
      // lock_acquire(&ft_lock);
      hash_insert(&frame_table, &f->hash_elem);
      // lock_release(&ft_lock);
    }
  //no more frames, need to evict and swap frames
  else
    {
      PANIC ("Nope. Don't even.");
    }

    return palloc_frame;
}

//TODO: synch needed
void
free_frame (uint8_t *pt_entry)
{
  struct frame lookup;
  struct frame *target;
  struct hash_elem *e;

  lookup.pte = pt_entry;
  e = hash_find(&frame_table, &lookup.hash_elem);
  if(e)
    {
      target = hash_entry(e, struct frame, hash_elem);
      hash_delete(&frame_table, &target->hash_elem);
      free(target);
      palloc_free_page(pt_entry);
    }
}

//lookup a frame, just find something empty (null pointer/bits)
//do we need synch here?
struct frame *
lookup_frame (uint8_t *pte)
{
  struct frame lookup;
  struct hash_elem *e;

  lookup.pte = pte;
  e = hash_find(&frame_table, &lookup.hash_elem);
  return e ? hash_entry(e, struct frame, hash_elem) : NULL;
}

//no clock algo so far, just random
void
evict_frame (void)
{
  //implement clock algo for choosing frame//
  //iterate through hash table
  //check reference bit-clear it
  //check if page in frame is dirty - pagedir.c pagedir_is_dirty()?
  struct hash_iterator i;
  struct frame *f;
  
  hash_first(&i, &frame_table);
  while(hash_next (&i))
  {
    f = hash_entry(hash_cur(&i), struct frame, hash_elem);
    //check/modify reference bit
    //free the page
  }
}
//any more???
//clock algorithm that jumps over pinned frames
