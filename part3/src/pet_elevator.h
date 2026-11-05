#ifndef PET_ELEVATOR_H
#define PET_ELEVATOR_H

#include <linux/list.h>
#include <linux/mutex.h>

#define MIN_FLOOR 1
#define MAX_FLOOR 5
#define NUM_FLOORS (MAX_FLOOR - MIN_FLOOR + 1)
#define MAX_PETS 5
#define MAX_WEIGHT_LBS 50

enum elevator_state {
	STATE_OFFLINE,
	STATE_IDLE,
	STATE_LOADING,
	STATE_UP,
	STATE_DOWN,
};

enum pet_type {
	PET_CHIHUAHUA,
	PET_PUG,
	PET_PUGHUAHUA,
	PET_DACHSHUND,
	PET_TYPE_COUNT,
};

struct pet {
	int type;
	int destination_floor;
	struct list_head node;
};

/* All fields except lock are protected by lock. */
struct pet_elevator {
	struct mutex lock;
	enum elevator_state state;
	bool stopping;
	int current_floor;
	int direction;			/* +1 is up, -1 is down */
	int load_lbs;
	int pet_count;
	int waiting_count;
	int serviced_count;
	struct list_head riders;
	struct list_head floors[NUM_FLOORS];	/* FIFO; index 0 is MIN_FLOOR */
};

extern struct pet_elevator elevator_data;

int pet_weight(int type);
char pet_letter(int type);

int elevator_thread_fn(void *data);

int elevator_proc_create(void);
void elevator_proc_remove(void);

#endif