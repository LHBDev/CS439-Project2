#include "page.h"
#include "frame.h"
#include "swap.h"

//Global vars
struct hash sup_table;

/* Returns a hash value for spte p. */
unsigned
spt_hash (const struct hash_elem *p_, void *aux UNUSED)
{
  const struct spt_entry *p = hash_entry (p_, struct spt_entry, hash_elem);
  return hash_bytes (&p->vpage, sizeof p->vpage);
}

/* Returns true if page a precedes page b. */
bool
spt_less (const struct hash_elem *a_, const struct hash_elem *b_,
           void *aux UNUSED)
{
  const struct spt_entry *a = hash_entry (a_, struct spt_entry, hash_elem);
  const struct spt_entry *b = hash_entry (b_, struct spt_entry, hash_elem);

  return a->vpage < b->vpage;
}

void
sup_table_init (void)
{
	hash_init(&sup_table, spt_hash, spt_less, NULL);
  // lock_init(&ft_lock);	
}
