#include "frame.h"
#include "swap.h"
#include "page.h"
#include "lib/kernel/hash.h"

//Header prototypes
struct hash frame_table;


//Put the hash functions directly in the header file ????
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
	hash_init(&frame_table, frame_hash, frame_less, NULL);
}

//lookup a frame
void
frame_lookup (void)
{
  struct hash_elem *target;

  target = hash_find(&frame_table, );
}

//need for later purposes
//evicting a frame?
//any more???
