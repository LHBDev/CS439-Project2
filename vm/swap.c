#include "swap.h"
#include "frame.h"
#include "page.h"

struct bitmap *swap_table;

void
swap_init (void)
{
	//change size of bitmap later
	swap_table = bitmap_create(256);
}
