#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#define HASHSIZE (4 * 1024  * 1024)

#define ARC_ALGO_CACHE_PHY_NUMB_OF_MAX_BLOCKS	(256 * 1024 * 1024)

#define MAX(a, b) ( (a) > (b) ? (a) : (b) )
#define MIN(a, b) ( (a) < (b) ? (a) : (b) )

/*
*Global Variable Declarations
*/
unsigned int g_uiMissCount = 0;
unsigned int g_uiHitCount = 0;

/* queue Implementation using Doubly Linked List */
struct page_node
{
	unsigned int m_uiPageNumber;  /*the page number stored in this page_node*/
	struct page_node *next; /*Pointer to Next Node*/
	struct page_node *prev; /*Pointer to Previous Node*/
};

struct page_queue
{
	unsigned int m_uiPage_Count;  /*Number of blocks filled.*/
	unsigned int iNumberOfBlocks; /*Number of max blocks (c)*/	
	struct page_node *front, *rear;
};

struct ARCCache
{
	unsigned int mui_capacity;	/*c be the cache size in pages*/
	float mf_T1_Size_p;
	struct page_queue mS_T1q; /*Most Recently Used page*/
	struct page_queue mS_B1q; /*Most Recently Used Ghost entries*/
	struct page_queue mS_T2q; /*Most Frequently Used page*/
	struct page_queue mS_B2q; /*Most Frequently Used Ghost entries*/

	unsigned long *pul_HashTable;	/*For testing cache page hits*/
};

struct ARCCache cache = {0};

/*Initialize ARCcache structure*/
void Create_Cache(unsigned int in_uiCapacity)
{
	if(in_uiCapacity > ARC_ALGO_CACHE_PHY_NUMB_OF_MAX_BLOCKS)
	{
		printf("Cache number of blocks limited to %d\r\n", ARC_ALGO_CACHE_PHY_NUMB_OF_MAX_BLOCKS);
		return;
	}

	memset(&cache, 0, sizeof(cache));

	cache.mf_T1_Size_p = 0;
	cache.mui_capacity = in_uiCapacity;
	
	cache.mS_T1q.iNumberOfBlocks = in_uiCapacity;
	cache.mS_B1q.iNumberOfBlocks = in_uiCapacity;
	cache.mS_T2q.iNumberOfBlocks = in_uiCapacity;
	cache.mS_B2q.iNumberOfBlocks = in_uiCapacity;

	cache.pul_HashTable = (unsigned long *)malloc(HASHSIZE * sizeof(unsigned long));
	memset(cache.pul_HashTable, 0, (HASHSIZE * sizeof(unsigned long)));
}

/*function to check if queue is empty*/
int isQueueEmpty(struct page_queue* queue)
{
	return queue->rear == NULL;
}

/* function adds a page with given 'pageNumber' to queue(front). */
void enqueue_Page(struct page_queue* queue, unsigned int pageNumber)
{
	/*Create new node and store page number*/
	struct page_node* NewNode = (struct page_node *)malloc(sizeof(struct page_node));
	NewNode->m_uiPageNumber = pageNumber;

	/*Init prev and next as NULL*/
	NewNode->prev = NULL;
	NewNode->next = NULL;

	NewNode->next = queue->front;

	/*If queue is empty, change both front and rear pointers*/
	if (isQueueEmpty(queue))
	{
		queue->front = NewNode;
		queue->rear = queue->front;
	}
	/*Else change the front*/
	else
	{
		queue->front->prev = NewNode;
		queue->front = NewNode;
	}

	/*increment number of full frames*/
	queue->m_uiPage_Count++;
}

/*function to delete a node from queue*/
void remove_page(struct page_queue* queue, struct page_node* in_pS_PageNode)
{
	struct page_node* pS_TempNode = NULL;

	/* base case */
	if (in_pS_PageNode == NULL)
	{
		printf("Requested Page is Not Available\n");
		return;
	}

	if ((queue->front == in_pS_PageNode) && (queue->rear == in_pS_PageNode))
	{
		/*Removing the only node available in the queue*/
		queue->front = NULL;
		queue->rear = NULL;
	}
	else if ((queue->front == in_pS_PageNode) && (queue->rear != in_pS_PageNode))
	{
		/*Removing the front node from the queue*/
		queue->front = in_pS_PageNode->next;
		queue->front->prev = NULL;
	}
	else if ((queue->front != in_pS_PageNode) && (queue->rear == in_pS_PageNode))
	{
		/*Removing the last node from the queue*/
		queue->rear = in_pS_PageNode->prev;
		queue->rear->next = NULL;
	}
	else
	{
		in_pS_PageNode->prev->next = in_pS_PageNode->next;
		in_pS_PageNode->next->prev = in_pS_PageNode->prev;
	}

	/*Free the node after dequeuing it from the queue*/
	free(in_pS_PageNode);

	if(queue->m_uiPage_Count > 0)
	{
		queue->m_uiPage_Count--;
	}
}

/*! This Function searches the given queue for the page number presence.
	Returns 1 and the page node if the given page number is present in the given queue.*/
struct page_node* Is_Page_Available_In_Queue(struct page_queue* queue, unsigned int m_uiPageNumber)
{
	struct page_node* pS_PageNode = NULL;

	/*Check if the page or the frame being referenced is already present in the queue of nodes.*/
	struct page_node* tempReqPage = NULL;
	for (tempReqPage = queue->front; tempReqPage; tempReqPage = tempReqPage->next)
	{
		if (tempReqPage->m_uiPageNumber == m_uiPageNumber)
		{
			pS_PageNode = tempReqPage;
			return pS_PageNode;
		}
	}
	return pS_PageNode;
}

/* This function moves given page node fromQueue to the top of the toQueue*/
void Move_Page(struct page_queue* fromQueue, struct page_queue* toQueue, struct page_node* in_pS_PageNode)
{
	if ((fromQueue->front == in_pS_PageNode) && (fromQueue->rear == in_pS_PageNode))
	{
		/*if this is the onlly node, then remove from the queue*/
		fromQueue->front = NULL;
		fromQueue->rear = NULL;
	}
	else if ((fromQueue->front == in_pS_PageNode) && (fromQueue->rear != in_pS_PageNode))
	{
		/*Remove Front node*/
		fromQueue->front = in_pS_PageNode->next;
		fromQueue->front->prev = NULL;
	}
	else if ((fromQueue->front != in_pS_PageNode) && (fromQueue->rear == in_pS_PageNode))
	{
		/*Remove rear node*/
		fromQueue->rear = in_pS_PageNode->prev;
		fromQueue->rear->next = NULL;
	}
	else
	{
		in_pS_PageNode->prev->next = in_pS_PageNode->next;
		in_pS_PageNode->next->prev = in_pS_PageNode->prev;
	}
	
	if(fromQueue->m_uiPage_Count > 0)
	{
		fromQueue->m_uiPage_Count--;
	}

	in_pS_PageNode->next = toQueue->front;

	/*If queue is empty, change both front and rear pointers*/
	if (isQueueEmpty(toQueue))
	{
		toQueue->front = in_pS_PageNode;
		toQueue->rear = toQueue->front;
	}
	else  
	{
		toQueue->front->prev = in_pS_PageNode;
		toQueue->front = in_pS_PageNode;
	}
	
	toQueue->m_uiPage_Count++;
}

/*ARC Algorithm Replace Sub routine*/
void replace(const unsigned int m_uiPageNumber, const float mf_T1_Size_p)
{
	if(	(cache.mS_T1q.m_uiPage_Count >= 1) && 
		((cache.mS_T1q.m_uiPage_Count > mf_T1_Size_p) || 
		 ((Is_Page_Available_In_Queue(&cache.mS_B2q, m_uiPageNumber) != NULL) && (mf_T1_Size_p == cache.mS_T1q.m_uiPage_Count))))
	{
		if(cache.mS_T1q.rear)
		{
			Move_Page(&cache.mS_T1q, &cache.mS_B1q, cache.mS_T1q.rear);
		}
	}
	else
	{
		if(cache.mS_T2q.rear)
		{
			Move_Page(&cache.mS_T2q, &cache.mS_B2q, cache.mS_T2q.rear);
		}
	}
}

void ARC_Lookup_Algorithm(const unsigned int in_uiPageNumber)
{
	struct page_node* pS_PageNode = NULL;
	unsigned int uiL1_Count = 0, uiL2_Count = 0;
	
	if (cache.pul_HashTable[in_uiPageNumber%HASHSIZE] == 1)
	{
		/*Case 1: page available in T1 or T2 queue*/
		if((pS_PageNode = Is_Page_Available_In_Queue(&cache.mS_T1q, in_uiPageNumber)) != NULL)
		{
			g_uiHitCount++;

			/*move page x to the top of T2*/
			Move_Page(&cache.mS_T1q, &cache.mS_T2q, pS_PageNode);
		}
		/*Case 1: page available in T2 queue*/
		else if((pS_PageNode = Is_Page_Available_In_Queue(&cache.mS_T2q, in_uiPageNumber)) != NULL)
		{
			g_uiHitCount++;
			
			/*move page x to the top of T2*/
			Move_Page(&cache.mS_T2q, &cache.mS_T2q, pS_PageNode);
		}
		/*Case 2: page available in B1 queue*/
		else if((pS_PageNode = Is_Page_Available_In_Queue(&cache.mS_B1q, in_uiPageNumber)) != NULL)
		{
			g_uiMissCount++;
			
			/*Adapt p = min{ c, p+ max{ |B2|/ | B1|, 1} }*/
			cache.mf_T1_Size_p = (float)MIN((float)cache.mui_capacity, (cache.mf_T1_Size_p + MAX(((float)cache.mS_B2q.m_uiPage_Count) / cache.mS_B1q.m_uiPage_Count, 1.0)));

			replace(in_uiPageNumber, cache.mf_T1_Size_p);
			
			/*move page x to the top of T2*/
			Move_Page(&cache.mS_B1q, &cache.mS_T2q, pS_PageNode);

			cache.pul_HashTable[pS_PageNode->m_uiPageNumber % HASHSIZE] = 1;
		}
		/*Case 2: page available in B2 queue*/
		else if (NULL != (pS_PageNode = Is_Page_Available_In_Queue(&cache.mS_B2q, in_uiPageNumber)))
		{
			g_uiMissCount++;
			
			/*Adapt p = max{ 0, p - max{ |B1|/ |B2|, 1} }*/
			cache.mf_T1_Size_p = (float)MAX(0.0, (float)(cache.mf_T1_Size_p - MAX((cache.mS_B1q.m_uiPage_Count*1.0) / cache.mS_B2q.m_uiPage_Count, 1.0)));
			
			replace(in_uiPageNumber, cache.mf_T1_Size_p);
			
			/*move page x to the top of T2*/
			Move_Page(&cache.mS_B2q, &cache.mS_T2q, pS_PageNode);

			cache.pul_HashTable[pS_PageNode->m_uiPageNumber % HASHSIZE] = 1;
		}
		/*Case 4: page not available in any of the queue.*/
		else
		{
			g_uiMissCount++;
				
			uiL1_Count = cache.mS_T1q.m_uiPage_Count + cache.mS_B1q.m_uiPage_Count;
			uiL2_Count = cache.mS_T2q.m_uiPage_Count + cache.mS_B2q.m_uiPage_Count;
			
			if (uiL1_Count == cache.mui_capacity)
			{
				if (cache.mS_T1q.m_uiPage_Count < cache.mui_capacity)
				{
					/*delete LRU page of B1 and remove it from the cache.*/
					//cache.pul_HashTable[cache.mS_B1q.rear->m_uiPageNumber % HASHSIZE]--;
					cache.pul_HashTable[cache.mS_B1q.rear->m_uiPageNumber % HASHSIZE] = 0;
					
					remove_page(&cache.mS_B1q, cache.mS_B1q.rear);

					replace(in_uiPageNumber, cache.mf_T1_Size_p);
				}
				else
				{
					/*delete LRU page of T1 and remove it from the cache.*/
					//cache.pul_HashTable[cache.mS_T1q.rear->m_uiPageNumber % HASHSIZE]--;
					cache.pul_HashTable[cache.mS_T1q.rear->m_uiPageNumber % HASHSIZE] = 0;

					remove_page(&cache.mS_T1q, cache.mS_T1q.rear);
				}
			}
			else if (uiL1_Count < cache.mui_capacity)
			{
				if ((uiL1_Count + uiL2_Count) >= cache.mui_capacity)
				{
					if ((uiL1_Count + uiL2_Count) == (2 * cache.mui_capacity))
					{
						/*delete LRU page of B1 and remove it from the cache.*/
						//cache.pul_HashTable[cache.mS_B2q.rear->m_uiPageNumber % HASHSIZE]--;
						cache.pul_HashTable[cache.mS_B2q.rear->m_uiPageNumber % HASHSIZE] = 0;
						
						remove_page(&cache.mS_B2q, cache.mS_B2q.rear);					
					}

					replace(in_uiPageNumber, cache.mf_T1_Size_p);
				}
			}
			
			/*Put x at the top of T1 and place it in the cache.*/
			enqueue_Page(&cache.mS_T1q, in_uiPageNumber);
			//cache.pul_HashTable[in_uiPageNumber % HASHSIZE]++;
			cache.pul_HashTable[in_uiPageNumber % HASHSIZE] = 1;
		}
	}
	else
	{
		g_uiMissCount++;
		
		uiL1_Count = cache.mS_T1q.m_uiPage_Count + cache.mS_B1q.m_uiPage_Count;
		uiL2_Count = cache.mS_T2q.m_uiPage_Count + cache.mS_B2q.m_uiPage_Count;
		
		if (uiL1_Count == cache.mui_capacity)
		{
			if (cache.mS_T1q.m_uiPage_Count < cache.mui_capacity)
			{
				/*delete LRU page of B1 and remove it from the cache.*/
				//cache.pul_HashTable[cache.mS_B1q.rear->m_uiPageNumber % HASHSIZE]--;
				cache.pul_HashTable[cache.mS_B1q.rear->m_uiPageNumber % HASHSIZE] = 0;
				
				remove_page(&cache.mS_B1q, cache.mS_B1q.rear);
				
				replace(in_uiPageNumber, cache.mf_T1_Size_p);
			}
			else
			{
				/*delete LRU page of T1 and remove it from the cache.*/
				//cache.pul_HashTable[cache.mS_T1q.rear->m_uiPageNumber % HASHSIZE]--;
				cache.pul_HashTable[cache.mS_T1q.rear->m_uiPageNumber % HASHSIZE] = 0;
				
				remove_page(&cache.mS_T1q, cache.mS_T1q.rear);
			}
		}
		else if (uiL1_Count < cache.mui_capacity)
		{
			if ((uiL1_Count + uiL2_Count) >= cache.mui_capacity)
			{
				if ((uiL1_Count + uiL2_Count) == (2 * cache.mui_capacity))
				{
					/*delete LRU page of B1 and remove it from the cache.*/
					//cache.pul_HashTable[cache.mS_B2q.rear->m_uiPageNumber % HASHSIZE]--;
					cache.pul_HashTable[cache.mS_B2q.rear->m_uiPageNumber % HASHSIZE] = 0;
					
					remove_page(&cache.mS_B2q, cache.mS_B2q.rear);					
				}
				
				replace(in_uiPageNumber, cache.mf_T1_Size_p);
			}
		}
		
		/*Put x at the top of T1 and place it in the cache.*/
		enqueue_Page(&cache.mS_T1q, in_uiPageNumber);
		cache.pul_HashTable[in_uiPageNumber % HASHSIZE]++;
		cache.pul_HashTable[in_uiPageNumber % HASHSIZE] = 1;
	}
}

int main(int argc, char **argv)
{
	unsigned int iStartingBlock = 0, iNumberOfBlocks = 0, iIgnore = 0, iRequestNumber = 0;
	unsigned int iIndex = 0;
	unsigned int iTotalRequests = 0;
	unsigned int uiCacheSize = 0;
	char *fileName = "p3.lis";
	FILE *fptr;
	time_t start, stop;

	if (argc == 3)
	{
		/*argv[1] is the cache size*/
		/*argv[2] is the Trace File Name*/
		uiCacheSize = strtol(argv[1], NULL, 10);
		fileName = argv[2];
	}
	else
	{
		printf("Cache Size and Trace File Name not supplied properly.\nProgram using default test inputs\n");
		uiCacheSize = (8 * 1024);
	}

	printf("Cache Size = %d\n", uiCacheSize);
	printf("TraceFile Name = %s\n", fileName);

	/*Initialize cache can hold c pages*/
	Create_Cache(uiCacheSize);

	fptr = fopen(fileName, "r");
	if (fptr == NULL)
	{
		printf("Error while opening the file: %s\n", fileName);
		getchar();
		exit(EXIT_FAILURE);
	}
	
	time(&start);
	
	while (1)
	{
		if(fscanf(fptr, "%u %u %u %u", &iStartingBlock, &iNumberOfBlocks, &iIgnore, &iRequestNumber) != -1)
		{
			/*if((iTotalRequests % 10000) == 0)
			{
				printf("T1 : %d, B1 : %d, T2 : %d, B2 : %d\r\n", cache.mS_T1q.m_uiPage_Count, cache.mS_B1q.m_uiPage_Count, cache.mS_T2q.m_uiPage_Count, cache.mS_B2q.m_uiPage_Count);
			}*/
			iTotalRequests++;

			/*printf("%u\t\t%u/%u\t\t%u/%u\t\t%u/%u\t\t%u/%u\n", iRequestNumber, cache->mS_T1q.m_uiPage_Count, cache->mS_T1q.iNumberOfBlocks, cache->mS_B1q.m_uiPage_Count, cache->mS_B1q.iNumberOfBlocks, cache->mS_T2q.m_uiPage_Count, cache->mS_T2q.iNumberOfBlocks, cache->mS_B2q.m_uiPage_Count, cache->mS_B2q.iNumberOfBlocks);*/
			for (iIndex = iStartingBlock; iIndex < (iStartingBlock + iNumberOfBlocks); iIndex++)
			{
				if(iStartingBlock + iNumberOfBlocks > 1024)
				{
					//continue;
				}

				ARC_Lookup_Algorithm(iIndex);
			}
		}
		else
		{
			break;
		}
	}
	
	time(&stop);

	printf("Miss Count = %u, Hit Count = %u\n", g_uiMissCount, g_uiHitCount);
	printf("Hit Ratio = %5.4f %%\n", ((float)(g_uiHitCount * 100) / (g_uiHitCount + g_uiMissCount)));
	printf("Elapsed Time : %.0f seconds. \n", difftime(stop, start));

	if(fptr != NULL)
	{
		fclose(fptr);
	}

	getchar();
	return 0;
}
