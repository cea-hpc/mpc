#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>

int res = 0;

typedef struct {
  long i;
  double total;
  double min;
  double max;
} entry_t;

typedef struct {
  double i;
  double total;
  double min;
  double max;
} error_t;

error_t error={0.5,0.5,0.5,0.5};

#define get_next()do {				\
    while(line[0] != '\0'){			\
      line ++;					\
    }						\
    while(line[0] == '\0'){			\
      line ++;					\
    }						\
  }while(0)

double convert_unit(char* line){
  if(strcmp(line,"ns") == 0){
    return 1;
  }
  if(strcmp(line,"us") == 0){
    return 1000.0;
  }
  if(strcmp(line,"ms") == 0){
    return 1000000.0;
  }
  if(strcmp(line,"s") == 0){
    return 1000000000.0;
  }
  assert(0);
}

#define get_unit(a) entry->a=entry->a*convert_unit(line)

void read_entry(entry_t * entry, char* line){
  while(line[0] == '\0'){
    line ++;
  }
  entry->i = atol(line);
  get_next();
  entry->total = atof(line);
  get_next();
  get_unit(total);
  get_next();
  entry->min = atof(line);
  get_next();
  get_unit(total);
  get_next();
  entry->max = atof(line);
  get_next();
  get_unit(max);
}

#define check_jitter(a) do{status = 0;if(abs(ref.a-cur.a) > error.a*ref.a){ status = 1;res = 1;}}while(0)


void check_entry(entry_t ref, entry_t cur, char* func){
  int status;
  fprintf(stderr, "Func %s:\n",func);
  check_jitter(i);
  fprintf(stderr, "\ti     : %8d (%ld,%ld)\n",status,ref.i,cur.i);
  check_jitter(total);
  fprintf(stderr, "\ttotal : %8d (%e,%e)\n",status,ref.total,cur.total);
  check_jitter(min);
  fprintf(stderr, "\tmin   : %8d (%e,%e)\n",status,ref.min,cur.min);
  check_jitter(max);
  fprintf(stderr, "\tmax   : %8d (%e,%e)\n",status,ref.max,cur.max);
}

int main (int argc, char** argv){
  char *line = NULL;
  size_t len = 0;
  ssize_t i = 0;
  ssize_t read;

  char *func_name;
  char* body;
  char cur[4096];
  entry_t ref_entry;
  entry_t cur_entry;

  cur[0]='\0';
  while ((read = getline(&line, &len, stdin)) != -1) {
    int done = 0; 
    for(i = 0; i < read; i++){
      if((line[i] == ' ') && (done == 1)){
	line[i] = '\0';
      }
      if((line[i] == '|') && (done == 0)){
	line[i] = '\0';
	done = 1;
	body=&(line[i+1]);
      }
    }
    func_name = line;
    if(strcmp(func_name,cur) != 0){
      strcpy(cur,func_name);
      read_entry(&ref_entry,body);
    } else {
      read_entry(&cur_entry,body);
      check_entry(ref_entry,cur_entry,func_name);
    }
  }

  free(line);

  
  return res;
}
