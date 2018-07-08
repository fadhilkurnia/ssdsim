#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <sys/types.h>
#include "initialize.h"
#include "flash.h"
#include "pagemap.h"

#define MAX_INT64  0x7fffffffffffffffll
#define FLAG_NO_REQUEST_ADDED -1
#define FLAG_NEW_REQUEST_ADDED 1

struct ssd_info *simulate(struct ssd_info *);
int get_requests(struct ssd_info *);
struct ssd_info *buffer_management(struct ssd_info *);
unsigned int lpn2ppn(struct ssd_info * ,unsigned int lsn);
struct ssd_info *distribute(struct ssd_info *);
int64_t trace_output(struct ssd_info* );
void statistic_output(struct ssd_info *);
unsigned int size(unsigned int);
unsigned int transfer_size(struct ssd_info *,int,unsigned int,struct request *);
int64_t find_nearest_event(struct ssd_info *);
void free_all_node(struct ssd_info *);
struct ssd_info *make_aged(struct ssd_info *);
struct ssd_info *no_buffer_distribute(struct ssd_info *);

struct ssd_info *parse_args(struct ssd_info *, int, char *[]);
struct request *read_request(struct ssd_info *, struct request *);
int64_t process_io(struct ssd_info *, struct request **, struct request *);
int try_insert_request(struct ssd_info *, struct request *);
void prep_output_for_simulation(struct ssd_info *);
void close_file(struct ssd_info *);

/********************************************
* Function to display info and help
* Added by Fadhil Imam (fadhilimamk@gmail.com) 
* to print help and usage info | 29/06/2018
*********************************************/
void display_title();
void display_help();
void display_simulation_intro(struct ssd_info *);
void display_freepage(struct ssd_info *);