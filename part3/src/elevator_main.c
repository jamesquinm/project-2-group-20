#include <linux/err.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "pet_elevator.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("James Mosely");
MODULE_DESCRIPTION("Pet elevator scheduler");

extern int (*STUB_start_elevator)(void);
extern int (*STUB_issue_request)(int, int, int);
extern int (*STUB_stop_elevator)(void);

struct pet_elevator elevator_data;
static struct task_struct *elevator_thread;

int pet_weight(int type)
{
	static const int weights[PET_TYPE_COUNT] = { 3, 14, 10, 16 };

	return weights[type];
}

char pet_letter(int type)
{
	static const char letters[PET_TYPE_COUNT] = { 'C', 'P', 'H', 'D' };

	return letters[type];
}

static int start_elevator(void)
{
	struct pet_elevator *e = &elevator_data;
	int result = 0;

	mutex_lock(&e->lock);
	if (e->state != STATE_OFFLINE) {
		result = 1;
	} else {
		e->state = STATE_IDLE;
		e->current_floor = MIN_FLOOR;
		e->direction = 1;
		e->load_lbs = 0;
		e->pet_count = 0;
	}
	mutex_unlock(&e->lock);

	return result;
}

static bool valid_floor(int floor)
{
	return floor >= MIN_FLOOR && floor <= MAX_FLOOR;
}

static int issue_request(int start_floor, int destination_floor, int type)
{
	struct pet_elevator *e = &elevator_data;
	struct pet *pet;

	if (!valid_floor(start_floor) || !valid_floor(destination_floor) ||
	    start_floor == destination_floor || type < 0 || type >= PET_TYPE_COUNT)
		return 1;

	pet = kmalloc(sizeof(*pet), GFP_KERNEL);
	if (!pet)
		return -ENOMEM;

	pet->type = type;
	pet->destination_floor = destination_floor;

	mutex_lock(&e->lock);
	list_add_tail(&pet->node, &e->floors[start_floor - MIN_FLOOR]);
	e->waiting_count++;
	mutex_unlock(&e->lock);

	return 0;
}

static int stop_elevator(void)
{
	struct pet_elevator *e = &elevator_data;
	int result = 0;

	mutex_lock(&e->lock);
	if (e->stopping)
		result = 1;
	else if (e->state != STATE_OFFLINE)
		e->stopping = true;
	mutex_unlock(&e->lock);

	return result;
}

static void free_pet_list(struct list_head *pets)
{
	struct pet *pet;
	struct pet *next;

	list_for_each_entry_safe(pet, next, pets, node) {
		list_del(&pet->node);
		kfree(pet);
	}
}

static int __init pet_elevator_init(void)
{
	struct pet_elevator *e = &elevator_data;
	int floor;
	int result;

	mutex_init(&e->lock);
	e->state = STATE_OFFLINE;
	e->current_floor = MIN_FLOOR;
	e->direction = 1;
	INIT_LIST_HEAD(&e->riders);
	for (floor = 0; floor < NUM_FLOORS; floor++)
		INIT_LIST_HEAD(&e->floors[floor]);

	result = elevator_proc_create();
	if (result)
		return result;

	elevator_thread = kthread_run(elevator_thread_fn, NULL, "pet_elevator");
	if (IS_ERR(elevator_thread)) {
		elevator_proc_remove();
		return PTR_ERR(elevator_thread);
	}

	STUB_start_elevator = start_elevator;
	STUB_issue_request = issue_request;
	STUB_stop_elevator = stop_elevator;
	return 0;
}

static void __exit pet_elevator_exit(void)
{
	struct pet_elevator *e = &elevator_data;
	int floor;

	STUB_start_elevator = NULL;
	STUB_issue_request = NULL;
	STUB_stop_elevator = NULL;

	kthread_stop(elevator_thread);
	elevator_proc_remove();

	free_pet_list(&e->riders);
	for (floor = 0; floor < NUM_FLOORS; floor++)
		free_pet_list(&e->floors[floor]);
}

module_init(pet_elevator_init);
module_exit(pet_elevator_exit);