#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "cache.h"

#define ARC_ALGO_CACHE_PHY_PAGE_MAX_COUNT	(256 * 1024 * 1024)
#define ARC_ALGO_DEFAULT_CACHE_PAGE_COUNT	(256)

typedef struct 
{
  int c;        // cache size in number of pages
  char fname[1024];
  FILE *fptr;

} Test_input;

cache *g_pS_CacheARC = NULL;

/* Parse Input arguments*/
int parse_args(int argc, char **argv, Test_input *pS_test_input)
{
	if (argc == 3)
	{
		/*argv[1] - cache size*/
		/*argv[2] - Trace File Name*/
		pS_test_input->c = strtol(argv[1], NULL, 10);
		strcpy(pS_test_input->fname, argv[2]);
		
		if(pS_test_input->c > ARC_ALGO_CACHE_PHY_PAGE_MAX_COUNT)
		{
			printf("Cache number of blocks limited to %d\r\n", ARC_ALGO_CACHE_PHY_PAGE_MAX_COUNT);
			printf("Cache Size not supplied properly.\nProgram using default test inputs\r\n");	
			pS_test_input->c = ARC_ALGO_DEFAULT_CACHE_PAGE_COUNT;
		}
	}
	else
	{
		printf("Cache Size and Trace File Name not supplied properly.\nProgram using default test inputs.\r\n");
		pS_test_input->c = ARC_ALGO_DEFAULT_CACHE_PAGE_COUNT;
		sprintf(pS_test_input->fname, "P3.lis");
	}

	pS_test_input->fptr = fopen(pS_test_input->fname, "r");
	if (pS_test_input->fptr == NULL)
	{
		printf("Error: \"%s\" is not a valid path\n", argv[2]);
		return 0;
	}
	
	return 0;
}

int main (int argc, char **argv) 
{
	Test_input mS_test_input = {0};
	time_t start = 0, stop = 0;
	unsigned int uiStartingBlock = 0, uiNumberOfBlocks = 0, uiIgnore = 0, uiRequestNumber = 0;
	unsigned int uiPageNumber = 0;
	unsigned int uiTotLines = 0;

	cache *pS_Cache_ARC = NULL; 
	
	/*Get Cache size and test file name from input arguments*/
	parse_args(argc, argv, &mS_test_input);

	/*Initialize cache to hold c pages*/
	pS_Cache_ARC = cache_create(mS_test_input.c);

	if(mS_test_input.fptr == NULL)
	{
		return 0;
	}

	/*To test the ARC implementation*/
	time(&start);
	while (1)
	{
		if(fscanf(mS_test_input.fptr, "%u %u %u %u", &uiStartingBlock, &uiNumberOfBlocks, &uiIgnore, &uiRequestNumber) != -1)
		{
			uiTotLines++;

			for (uiPageNumber = uiStartingBlock; uiPageNumber < (uiStartingBlock + uiNumberOfBlocks); uiPageNumber++)
			{
				ARC_Lookup_Algorithm(pS_Cache_ARC, uiPageNumber);
			}
		}
		else
		{
			break;
		}

		if (uiTotLines % 100000 == 0) 
		{
			fprintf(stderr, "File : %s (c = %d): processed %d lines\r", mS_test_input.fname, mS_test_input.c, uiTotLines);
			fflush(stderr);
		}
	}

	time(&stop);
	
	printf("\r\n\n");
	printf("Elapsed Time : %.0fs\r\n\n", difftime(stop, start));

	if(pS_Cache_ARC->requests > 0)
	{
		printf("\rCache Access Statistics :\n\nRequests:\t%8d\r\nhits:\t\t%8d\r\nratio:\t\t%5.2f\n",
		pS_Cache_ARC->requests, pS_Cache_ARC->hits, ((pS_Cache_ARC->hits*100)/(float)pS_Cache_ARC->requests));
	}

	cache_free(pS_Cache_ARC);
	
	if(mS_test_input.fptr != NULL)
	{
		fclose(mS_test_input.fptr);
	}
	
	getchar();

	return 0;
}