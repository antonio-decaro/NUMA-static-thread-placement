#define malloc_chk(ptr, size) do{ptr = malloc(size); if(ptr == NULL){perror("malloc");}} while(0)
#define realloc_chk(ptr, size) do{if((ptr = realloc(ptr,size)) == NULL){perror("realloc");}} while(0)
#define ts_diff(te, ts) (1000000000 * (te.tv_sec - ts.tv_sec) + (te.tv_nsec - ts.tv_nsec))
#define swap_ptr(a,b) do{void * tmp = a; a=b; b=tmp;} while(0)

