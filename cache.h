#ifndef CACHE_H
#define CACHE_H



typedef struct block
{
	unsigned int key;	/*Pagenumber*/
	void *value;
	struct block *next;
}block;

typedef struct
{
	block **b;		// array of block pointers
	unsigned int n;    // number of buckets
	unsigned int uiBlockCount; // how many element are currently stored
}cache_memory;

struct list;

typedef struct page
{
	struct page *next;
	struct page *prev;
	struct list *l;	/*to identify in which list this page is added (T1, T2, B1, B2)*/
	unsigned int addr;	/*Page number*/
}page;

typedef struct list
{
	page *head;
	page *tail;
	unsigned int ui_pagecount;
}list;

typedef struct
{
	list *t1, *t2, *b1, *b2;
	cache_memory *ghosts, *pages;
	double p;
	unsigned int ui_capacity;
	unsigned long requests;
	unsigned long hits;

}cache;

page *page_create(unsigned int ui_addr);
void page_free(page *p);

void list_free(list *l);
page *list_remove(list *l, page *p);
page *remove_page_from_rear(list *l);
void add_page_to_front(list *l, page *p);

cache_memory *map_create();
cache_memory *create_cache_memory(unsigned int n);
void release_cache_mem(cache_memory *m);
void add_page(cache_memory *m, unsigned int uiPageNumber, void *value);
void remove_page(cache_memory *m, unsigned int uiPageNumber);
void *Is_Page_Available_inCache(cache_memory *m, unsigned int ui_PageNumber);

cache *cache_create(unsigned int capacity);
void cache_free (cache *c);
int ARC_Lookup_Algorithm(cache *pS_Cache, int ui_pageNumber);

#endif
