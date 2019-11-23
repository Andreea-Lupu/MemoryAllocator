#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint32_t N;
unsigned char *v;
uint32_t indice_start;

void INITIALIZE(uint32_t size)
{
	v = calloc (size, sizeof(unsigned char));
	indice_start=0;
}

void FINALIZE()
{
	free(v);
}

void DUMP()
{	uint32_t i, j;
	for(i=0; i<N; i=i+16) {
		printf("%08X\t", i);
		for(j=0; j<16; j++) {
			if(i+j<N) {
				printf("%02X", v[i+j]);
				if(i+j+1<N) {
					if(j<15)
						printf(" ");
					if(j==7)
						printf(" ");
				}
			}
		}
		printf("\n");
	}
}

uint32_t index_next(uint32_t index_start)
{
	uint32_t i;
	i=(v[index_start]<<0) + (v[index_start+1]<<8) + (v[index_start+2]<<16) 
		+ (v[index_start+3]<<24);
	return i;
}

uint32_t index_back(uint32_t index_start)
{
	uint32_t i;
	i=(v[index_start+4]<<0) + (v[index_start+5]<<8) + (v[index_start+6]<<16) 
		+ (v[index_start+7]<<24);
	return i;
}

uint32_t index_dim(uint32_t index_start)
{
	uint32_t i;
	i=(v[index_start+8]<<0) + (v[index_start+9]<<8) + (v[index_start+10]<<16) 
		+ (v[index_start+11]<<24);
	return i;
}

void f(uint32_t nr, uint32_t start)
{
	v[start] = nr>>0;
	v[start+1] = nr>>8;
	v[start+2] = nr>>16;
	v[start+3] = nr>>24;
}

void ALLOC(uint32_t dim)
{	
	if(dim<N) {
//cazul in care arena nu are nici un bloc alocat
	if((indice_start == 0) && (index_next(0) == 0) && (index_dim(0) == 0) 
		&& (dim+12 <= N)) {
			f(dim, 8);
			printf("12\n");
//cazul in care putem aloca un bloc inainte de primul bloc din arena
	} else if(dim + 12 <= indice_start) {
			f(0, indice_start + 4);
			f(indice_start, 0);
			indice_start = 0; 
			f(dim, 8);
			printf("12\n");
/*cazul in care arena contine un singur bloc si nu e spatiu suficient pentru
alocare inaintea lui, ci doar dupa el*/
	} else if((index_next(indice_start) == 0) &&
				 (2*12+dim+indice_start+index_dim(indice_start) <= N)) {
			f(indice_start, 4+12+indice_start+index_dim(indice_start));
			f(dim, 8+12+indice_start+index_dim(indice_start));
			f(indice_start + 12 + index_dim(indice_start), indice_start);
			printf("%d\n", 2*12+indice_start+index_dim(indice_start));
/*cazul in care cautam un loc suficient pentru alocare intre blocurile existente
sau dupa ultimul bloc*/			
	} else {
		uint32_t indice_curent=indice_start;
		uint32_t indice_urmator=index_next(indice_start);
		int ok=0;
		while((ok == 0) && (indice_urmator != 0)) {
			if(dim+12 <= (indice_urmator-indice_curent-12
				-index_dim(indice_curent))) {
			ok=1;
			f(indice_curent+index_dim(indice_curent)+12, indice_curent);
			f(indice_curent+index_dim(indice_curent)+12, indice_urmator+4);
			f(indice_urmator, indice_curent+index_dim(indice_curent)+12);
			f(indice_curent, indice_curent+index_dim(indice_curent)+12+4);
			f(dim, indice_curent+index_dim(indice_curent)+12+8);
			printf("%d\n", indice_curent+index_dim(indice_curent)+12+12);

			} else {
				indice_curent=indice_urmator;
				indice_urmator=index_next(indice_curent);
			}
		}
		if((ok == 0) && (index_next(indice_curent) == 0) && 
				(indice_curent + index_dim(indice_curent) +dim + 2*12 <= N)) {
			f(indice_curent, 4+12+indice_curent+index_dim(indice_curent));
			f(dim, 8+12+indice_curent+index_dim(indice_curent));
			f(indice_curent+12+index_dim(indice_curent), indice_curent);
			printf("%d\n", 2*12+indice_curent+index_dim(indice_curent));	
			ok=1;			
		}
		if(ok == 0)
			printf("0\n");
	}
	} else {
			printf("0\n");
	}

}

void FREE(uint32_t indice)
{
	uint32_t i;
	for(i = indice; i < indice + index_dim(indice-12); i++)
		v[i] = 0;
	if(index_next(indice-12) != 0) {
		if((index_back(indice-12) != 0) || (indice_start == 0)) {
			f(index_next(indice-12), index_back(indice-12));
			f(index_back(indice-12), index_next(indice-12)+4);
		}
		if((index_back(indice-12) == 0) && (indice_start == indice-12)) {
			f(0, index_next(indice-12)+4);
			indice_start = index_next(indice-12);
		}
	} else {
		if((index_back(indice-12) != 0) || (indice_start == 0)) {
			f(0, index_back(indice-12));
		}
		if((index_back(indice-12) == 0) && (indice_start == indice-12)) {
			indice_start = 0;
		}
	}
	for(i = indice-12; i < indice; i++)
		v[i] = 0;
}

void FILL(uint32_t index, uint32_t size, uint32_t value) 
{
	uint32_t i;
	while((size > 0) && (index < N)) {
		if(size <= index_dim(index-12)) {
			for(i = index; i < index+size; i++)
				v[i] = value;
			size = 0;
		} else { 
			for(i = index; i < index+index_dim(index-12); i++)
				v[i] = value;
			if(index_next(index-12) != 0) {
				size=size-index_dim(index-12);
				index=12+index_next(index-12);
			} else {
				size=0;
			}		
		}
	}
}

void parse_command(char* cmd)
{
    const char* delims = " \n";

    char* cmd_name = strtok(cmd, delims);
    if (!cmd_name) {
        goto invalid_command;
    }

    if (strcmp(cmd_name, "INITIALIZE") == 0) {
        char* size_str = strtok(NULL, delims);
        if (!size_str) {
            goto invalid_command;
        }
        uint32_t size = atoi(size_str);
		N=size;
        INITIALIZE(size);

    } else if (strcmp(cmd_name, "FINALIZE") == 0) {
        FINALIZE();

    } else if (strcmp(cmd_name, "DUMP") == 0) {
        DUMP();

    } else if (strcmp(cmd_name, "ALLOC") == 0) {
        char* size_str = strtok(NULL, delims);
        if (!size_str) {
            goto invalid_command;
        }
        uint32_t size = atoi(size_str);
        ALLOC(size);

    } else if (strcmp(cmd_name, "FREE") == 0) {
        char* index_str = strtok(NULL, delims);
        if (!index_str) {
            goto invalid_command;
        }
        uint32_t index = atoi(index_str);
        FREE(index);

    } else if (strcmp(cmd_name, "FILL") == 0) {
        char* index_str = strtok(NULL, delims);
        if (!index_str) {
            goto invalid_command;
        }
        uint32_t index = atoi(index_str);
        char* size_str = strtok(NULL, delims);
        if (!size_str) {
            goto invalid_command;
        }
        uint32_t size = atoi(size_str);
        char* value_str = strtok(NULL, delims);
        if (!value_str) {
            goto invalid_command;
        }
        uint32_t value = atoi(value_str);
        FILL(index, size, value);

    } else {
        goto invalid_command;
    }

    return;

invalid_command:
    printf("Invalid command: %s\n", cmd);
    exit(1);
}

int main(void)
{	
    ssize_t read;
    char* line = NULL;
    size_t len;

    /* parse input line by line */
    while ((read = getline(&line, &len, stdin)) != -1) {
        /* print every command to stdout */
        printf("%s", line);

        parse_command(line);
    }

    free(line);

    return 0;
}
