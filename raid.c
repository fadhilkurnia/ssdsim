#include <stdio.h>
#include "pagemap.h"
#include "raid.h"
#include "ssd.h"

// initialize_raid function initializes raid struct and all ssd connected to the struct
struct raid_info* initialize_raid(struct raid_info* raid, struct user_args* uargs) {
    struct ssd_info *ssd_pointer;

    raid->num_disk = uargs->num_disk;
    strcpy(raid->tracefilename, uargs->trace_filename);

    raid->connected_ssd=malloc(sizeof(struct ssd_info) * raid->num_disk);
    alloc_assert(raid->connected_ssd, "connected ssd");
    for (int i = 0; i < raid->num_disk; i++) {
        ssd_pointer=(struct ssd_info*) malloc(sizeof(struct ssd_info));
        alloc_assert(ssd_pointer, "one of connected ssd");
        memset(ssd_pointer,0,sizeof(struct ssd_info));
        raid->connected_ssd[i] = ssd_pointer;
    }


    // initialize each ssd in raid
    for (int i = 0; i < raid->num_disk; i++) {
        printf("Initializing disk%d\n", i);

        raid->connected_ssd[i] = initialize_ssd(raid->connected_ssd[i], uargs);
        raid->connected_ssd[i] = initiation(raid->connected_ssd[i]);
        raid->connected_ssd[i] = make_aged(raid->connected_ssd[i]);
        raid->connected_ssd[i] = pre_process_page(raid->connected_ssd[i]);
    }

    return raid;
}

int simulate_raid(struct user_args* uargs) {
    struct raid_info *raid;

    raid=(struct raid_info*)malloc(sizeof(struct raid_info));
    alloc_assert(raid, "raid");
    memset(raid,0,sizeof(struct raid_info));

    raid = initialize_raid(raid, uargs);

    raid->raid_type = uargs->raid_type;

    if (raid->raid_type == RAID_0) {
        simulate_raid0(raid);
    } else if (raid->raid_type == RAID_5) {
        simulate_raid5(raid);
    } else {
        printf("Error! unknown raid version\n");
        exit(1);
    }
    
    free_raid_ssd(raid);
    free(raid);
    return 0;
}

void free_raid_ssd(struct raid_info* raid) {
    for (int i = 0; i < raid->num_disk; i++) {
        free(raid->connected_ssd[i]);
    }
}

struct raid_info* simulate_raid0(struct raid_info* raid) {
    printf("Work in progress for simualting raid 0 ...\n");

    raid->tracefile = fopen(raid->tracefilename, "r");

    fclose(raid->tracefile);
    return raid;
}


struct raid_info* simulate_raid5(struct raid_info* raid) {
    printf("Work in progress for simualting raid 5 ...\n");

    // controller's request queue
    // get_raid_request(raid)
    return raid;
}