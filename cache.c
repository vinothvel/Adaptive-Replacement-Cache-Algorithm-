#include "cache.h"

#include <stdio.h>
#include <stdlib.h>

#define MIN(a, b)  (((a) < (b)) ? (a) : (b))
#define MAX(a, b)  (((a) > (b)) ? (a) : (b))  

cache *cache_create (unsigned int ui_capacity)
{
	cache *c = malloc(sizeof(cache));

	memset(c, 0, sizeof(cache));

	/*Creates list to store T1, T2, B1 and B2*/
	c->t1 = (list*)malloc(sizeof(list));
	c->t2 = (list*)malloc(sizeof(list));
	c->b1 = (list*)malloc(sizeof(list));
	c->b2 = (list*)malloc(sizeof(list));

	memset(c->t1, 0, sizeof(list));
	memset(c->t2, 0, sizeof(list));
	memset(c->b1, 0, sizeof(list));
	memset(c->b2, 0, sizeof(list));
	
	c->pages = create_cache_memory(ui_capacity);

	/*ghost entries are created to store the pages available in B1 and B2*/
	c->ghosts = create_cache_memory(ui_capacity);

	//printf("Cache memory created for c : %d\r\n", ui_capacity);
	
	c->p = 0.0;
	c->ui_capacity = ui_capacity;
	
	c->requests = 0;
	c->hits = 0;
	
	return c;
}

void cache_free(cache *pS_Cache)
{
	list_free(pS_Cache->t1);
	list_free(pS_Cache->t2);
	list_free(pS_Cache->b1);
	list_free(pS_Cache->b2);
	
	release_cache_mem(pS_Cache->pages);
	release_cache_mem(pS_Cache->ghosts);
	
	free(pS_Cache);
}

/*function creates page and initialize the data*/
page *page_create (unsigned int ui_PageNum)
{
	page *p = (page*)malloc(sizeof(page));
	p->addr = ui_PageNum;
	p->next = NULL;
	p->prev = NULL;
	p->l = NULL;
	return p;
}

void page_free(page *p)
{
	free(p);
}

void list_free(list *l)
{
	page *temp = l->head, *next;
	
	while (temp != NULL)
	{
		next = temp->next;
		page_free(temp);
		temp = next;
	}
	
	free(l);
}

/*Function creates cache memory.
1. Cache memory is created with 2D block array. each block will be capable to store c pages
2. Cache block stores the page number and its memory location.*/
cache_memory *create_cache_memory(unsigned int ui_cacheCapacity)
{
	unsigned int ui_blockIndex = 0;
	cache_memory *m = (cache_memory*)malloc(sizeof(cache_memory));
	m->uiBlockCount = 0;
	m->n = ui_cacheCapacity;
	m->b = (block**)malloc(sizeof(block*) * ui_cacheCapacity);
	for (ui_blockIndex = 0; ui_blockIndex < m->n; ui_blockIndex++)
	{
		m->b[ui_blockIndex] = NULL;
	}
	return m;
}

void release_cache_mem(cache_memory *m)
{
	unsigned int ui_blockIndex = 0;
	for (ui_blockIndex = 0; ui_blockIndex < m->n; ui_blockIndex++)
	{
		block *temp = m->b[ui_blockIndex], *next;
		while (temp != NULL)
		{
			next = temp->next;
			free(temp);
			temp = next;
		}
	}
	free(m->b);
	free(m);
}

/*Add page in cache memory*/
void add_page(cache_memory *m, unsigned int uiPageNumber, void *value)
{
	unsigned int ui_blockIndex = (uiPageNumber % m->n);
	block *temp = m->b[ui_blockIndex], *b;
	while (temp != NULL)
	{
		if (temp->key == uiPageNumber)
		{
			temp->value = value;
			return;
		}
		temp = temp->next;
	}
	b = (block*)malloc(sizeof(block));
	b->value = value;	/*Stores the page address*/
	b->key = uiPageNumber;
	b->next = m->b[ui_blockIndex];
	m->b[ui_blockIndex] = b;
	m->uiBlockCount++;
}

/*function to remove page from cache memory*/
void remove_page(cache_memory *m, unsigned int uiPageNumber)
{
	unsigned int ui_blockIndex = (uiPageNumber % m->n);
	block *temp = m->b[ui_blockIndex], *prev = temp;
	while (temp != NULL)
	{
		if (temp->key == uiPageNumber)
		{
			if (temp == m->b[ui_blockIndex])
			{
				m->b[ui_blockIndex] = temp->next;
			}
			else
			{
				prev->next = temp->next;
			}
			free(temp);
			m->uiBlockCount--;
			return;
		}
		prev = temp;
		temp = temp->next;
	}
}

/*function checks the page is available in cache memory.
If available, returns page location, else return NULL.*/
void *Is_Page_Available_inCache(cache_memory *m, unsigned int ui_PageNumber)
{
	block *temp = m->b[(ui_PageNumber % m->n)];
	while (temp != NULL)
	{
		if (temp->key == ui_PageNumber)
		{
			return temp->value;
		}
		temp = temp->next;
	}
	
	return NULL;
}

/*removes page node from list*/
page *list_remove(list *l, page *p)
{
	if(l->ui_pagecount == 1)
	{
		l->head = NULL;
		l->tail = NULL;
	}
	else
	{
		if(l->tail == p)
		{
			l->tail = p->prev;
			l->tail->next = NULL;
		}
		else if(l->head == p)
		{
			l->head = p->next;
			l->head->prev = NULL;
		}
		else
		{
			p->prev->next = p->next;
			p->next->prev = p->prev;
		}
	}
	
	p->l = NULL;
	
	l->ui_pagecount--;
	return p;
}

page *remove_page_from_rear(list *l)
{
	if(l->ui_pagecount == 0)
	{
		return NULL;
	}
	else
	{
		return list_remove(l, l->tail);
	}
}

void add_page_to_front(list *l, page *p)
{
	// insert at front
	if (l->ui_pagecount == 0)
	{
		l->head = p;
		l->tail = p;
	}
	else
	{
		p->next = l->head;
		l->head->prev = p;
		l->head = p;
		p->prev = NULL;
	}
	
	p->l = l;
	l->ui_pagecount++;
}

void replace(cache *pS_Cache, unsigned int ui_pageNumber)
{
	page *ghost = Is_Page_Available_inCache(pS_Cache->ghosts, ui_pageNumber);
	
	if ((pS_Cache->t1->ui_pagecount >= 1) &&                       //(|T1| = 1
		((pS_Cache->t1->ui_pagecount > pS_Cache->p) ||            //(|T1| > p)
		(((ghost != NULL) && (ghost->l == pS_Cache->b2)) &&	 // (x ? B2 and
		(pS_Cache->t1->ui_pagecount == pS_Cache->p))))       // |T1| = p
	{
		/*move the LRU page of T1 to the top of B1 and remove it from the cache.*/
		
		// delete LRU in t1
		page *p = remove_page_from_rear(pS_Cache->t1);
		if(p != NULL)
		{
			// remove LRU page of T1 from cache memory
			remove_page(pS_Cache->pages, p->addr);
			
			/*Move LRU page of T1 to the top of B1*/
			add_page_to_front(pS_Cache->b1, p);
			add_page(pS_Cache->ghosts, p->addr, p);
		}
	}
	else
	{
		/*move the LRU page in T2 to the top of B2 and remove it from the cache.*/
		// delete LRU in t2
		page *p = remove_page_from_rear(pS_Cache->t2);
		if(p != NULL)
		{
			// remove LRU page of T2 from cache memory
			remove_page(pS_Cache->pages, p->addr);
			
			/*Move LRU page of T2 to the top of B2*/
			add_page_to_front(pS_Cache->b2, p);
			add_page(pS_Cache->ghosts, p->addr, p);
		}
	}
}

int ARC_Lookup_Algorithm(cache *pS_Cache, int ui_pageNumber)
{
	unsigned int uiL1_Count = 0, uiL2_Count = 0;
	page *p = NULL;
	float fTemp = 0;
	
	pS_Cache->requests++;
	
	/*Case I. x ? T1 ?T2 (a hit in ARC(c) and DBL(2c)): Move x to the top of T2
	Check page is available in cache memory. If available move to the tope of T2*/
	if((p = Is_Page_Available_inCache(pS_Cache->pages, ui_pageNumber)) != NULL)
	{
		pS_Cache->hits++;
		
		list_remove(p->l, p);
		add_page_to_front(pS_Cache->t2, p);
		
		return 1;
	}
	
	/*Case II. If page is not available and avilable is B1/B2 (ghost lists)
	Move the page to cache memory from ghost memory. */
	if((p = Is_Page_Available_inCache(pS_Cache->ghosts, ui_pageNumber)) != NULL)
	{
		if(p->l == pS_Cache->b1)
		{
			/*Available in B1*/
			/*Adapt p = min{ c, p+ max{ |B2|/ | B1|, 1} }*/
			if(pS_Cache->b1->ui_pagecount < pS_Cache->b2->ui_pagecount)
			{
				fTemp = ((float)pS_Cache->b2->ui_pagecount / (float)pS_Cache->b1->ui_pagecount);
			}
			else
			{
				fTemp = 1;
			}
			pS_Cache->p = MIN((pS_Cache->p + fTemp), pS_Cache->ui_capacity);
		}
		else
		{
			/*Available ib B2*/
			/*Adapt p = max{ 0, p max{ |B1|/ |B2|, 1} }*/
			if(pS_Cache->b2->ui_pagecount < pS_Cache->b1->ui_pagecount)
			{
				fTemp = ((float)pS_Cache->b1->ui_pagecount / (float)pS_Cache->b2->ui_pagecount);
			}
			else
			{
				fTemp = 1;
			}
			pS_Cache->p = MAX((pS_Cache->p - fTemp), 0);
		}
		
		replace(pS_Cache, ui_pageNumber);
		
		// move page into top of t2
		list_remove(p->l, p);
		add_page_to_front(pS_Cache->t2, p);
		
		// add page into cache memory
		remove_page(pS_Cache->ghosts, ui_pageNumber);
		add_page(pS_Cache->pages, ui_pageNumber, p);
		
		return 0;
	}
	
	uiL1_Count = (pS_Cache->t1->ui_pagecount + pS_Cache->b1->ui_pagecount);
	uiL2_Count = (pS_Cache->t2->ui_pagecount + pS_Cache->b2->ui_pagecount);
	
	/* Case IV - Part A*/  
	if(uiL1_Count == pS_Cache->ui_capacity)
	{
		if(pS_Cache->t1->ui_pagecount < pS_Cache->ui_capacity)
		{
			p = remove_page_from_rear(pS_Cache->b1);
			if(p != NULL)
			{
				remove_page(pS_Cache->ghosts, p->addr);
				page_free(p);
			}
			replace(pS_Cache, ui_pageNumber);
		}
		else
		{
			p = remove_page_from_rear(pS_Cache->t1);
			if(p != NULL)
			{
				remove_page(pS_Cache->pages, p->addr);
				page_free(p);
			}
		}
	}
	else
	{
		/* Case IV - Part B*/
		if((uiL1_Count + uiL2_Count) >= pS_Cache->ui_capacity)
		{
			if((uiL1_Count + uiL2_Count) == (2 * pS_Cache->ui_capacity))
			{
				p = remove_page_from_rear(pS_Cache->b2);
				if(p != NULL)
				{
					remove_page(pS_Cache->ghosts, p->addr);
					page_free(p);
				}
			}
			replace(pS_Cache, ui_pageNumber);
		}
	}
	
	p = page_create(ui_pageNumber);
	add_page_to_front(pS_Cache->t1, p);
	add_page(pS_Cache->pages, ui_pageNumber, p);
	
	return 0;
}
