#include "vm/swap.h"
#include "vm/frame.h"
#include "vm/page.h"
#include "devices/block.h"
#include "threads/synch.h"

struct bitmap *swap_table;
// struct block swap_block;
struct lock swap_lock;

void
swap_init (void)
{
	//change size of bitmap later
	// swap_table = bitmap_create(block_size(swap_block));  //bitmap set to false
	// swap_block = block_get_role(BLOCK_SWAP);
	// lock_init(&swap_lock);
}

void
swap_out (void)
{

}

void
swap_in (void)
{

}
