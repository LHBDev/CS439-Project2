#include "frame.h"
#include "swap.h"
#include "page.h"
#include "lib/kernel/hash.h"
#include "threads/palloc.h"
#include "threads/thread.h"
#include "threads/malloc.h"

//Header prototypes
struct hash frame_table;
struct lock ft_lock;

/* Returns a hash value for page p. */
unsigned
frame_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct frame *p = hash_entry (p_, struct frame, hash_elem);
  return hash_bytes (&p->pte, sizeof p->pte);
}

/* Returns true if page a precedes page b. */
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
  void *page;
	hash_init(&frame_table, frame_hash, frame_less, NULL);
  lock_init(&ft_lock);

  //Get every single page
  // while((page = palloc_get_page(PAL_USER)))
  //   {
  //     struct frame *frame;
  //     frame = (struct frame *)malloc(sizeof(struct frame));
  //     frame->owner = thread_current();
  //     frame->pte = page;
  //     hash_insert(&frame_table, &frame->hash_elem);
  //   }
}

//need to allocate all the user pages, also need to free them sometime
//free_frames and alloc_frames?

void
add_frame (uint32_t frame_address, uint32_t *pt_entry)
{
  struct frame *f;

  f = malloc(sizeof(struct frame));
  f->owner = thread_current();
  f->frame_addr = frame_address;
  f->pte = pt_entry;
  //might need to put info about page table entry
  //this insert might not insert if there is a collision
  //should this insert be synchronized??
  hash_insert(&frame_table, &f->hash_elem);
}

//lookup a frame
struct frame *
lookup_frame (uint32_t frame_addr)
{
  struct hash_iterator i;
  struct frame *f;

  hash_first (&i, &frame_table);
  while (hash_next (&i))
    {
      f = hash_entry (hash_cur (&i), struct frame, hash_elem);
      if(f->frame_addr == frame_addr)
        return f;
    }

  return NULL;
}

//no clock algo so far, just random
void
evict_frame (void)
{
  //use lookup_frame here?
}
//any more???
//clock algorithm that jumps over pinned frames
