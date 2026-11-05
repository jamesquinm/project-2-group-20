#include <linux/delay.h>
#include <linux/kthread.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/slab.h>

#include "pet_elevator.h"

#define MOVE_SECONDS 2
#define LOAD_SECONDS 1
#define IDLE_POLL_MS 100

static struct list_head *current_floor_pets(struct pet_elevator *e)
{
	return &e->floors[e->current_floor - MIN_FLOOR];
}

static bool pet_fits(struct pet_elevator *e, struct pet *pet)
{
	return e->pet_count < MAX_PETS &&
	       e->load_lbs + pet_weight(pet->type) <= MAX_WEIGHT_LBS;
}

static bool should_unload(struct pet_elevator *e)
{
	struct pet *pet;

	list_for_each_entry(pet, &e->riders, node) {
		if (pet->destination_floor == e->current_floor)
			return true;
	}
	return false;
}

/* Boarding is FIFO, so only the pet at the front of the line matters. */
static bool should_load(struct pet_elevator *e)
{
	struct pet *pet;

	if (e->stopping)
		return false;

	pet = list_first_entry_or_null(current_floor_pets(e), struct pet, node);
	return pet && pet_fits(e, pet);
}

static void unload_pets(struct pet_elevator *e)
{
	struct pet *pet;
	struct pet *next;

	list_for_each_entry_safe(pet, next, &e->riders, node) {
		if (pet->destination_floor != e->current_floor)
			continue;

		list_del(&pet->node);
		e->load_lbs -= pet_weight(pet->type);
		e->pet_count--;
		e->serviced_count++;
		kfree(pet);
	}
}

static void load_pets(struct pet_elevator *e)
{
	while (should_load(e)) {
		struct pet *pet = list_first_entry(current_floor_pets(e), struct pet, node);

		list_move_tail(&pet->node, &e->riders);
		e->load_lbs += pet_weight(pet->type);
		e->pet_count++;
		e->waiting_count--;
	}
}

/* Returns true if a rider or waiting pet requires going further this way. */
static bool has_work_toward(struct pet_elevator *e, int direction)
{
	struct pet *pet;
	int floor;

	list_for_each_entry(pet, &e->riders, node) {
		if ((pet->destination_floor - e->current_floor) * direction > 0)
			return true;
	}

	if (e->stopping)
		return false;

	for (floor = e->current_floor + direction;
	     floor >= MIN_FLOOR && floor <= MAX_FLOOR; floor += direction) {
		if (!list_empty(&e->floors[floor - MIN_FLOOR]))
			return true;
	}
	return false;
}

/* Runs one action. The lock is never held while sleeping. */
static void elevator_step(struct pet_elevator *e)
{
	mutex_lock(&e->lock);

	if (e->state == STATE_OFFLINE) {
		mutex_unlock(&e->lock);
		msleep(IDLE_POLL_MS);
		return;
	}

	if (should_unload(e) || should_load(e)) {
		e->state = STATE_LOADING;
		mutex_unlock(&e->lock);
		ssleep(LOAD_SECONDS);

		mutex_lock(&e->lock);
		unload_pets(e);
		load_pets(e);
		mutex_unlock(&e->lock);
		return;
	}

	/* LOOK scheduling: keep going until nothing is ahead, then reverse. */
	if (!has_work_toward(e, e->direction))
		e->direction = -e->direction;

	if (has_work_toward(e, e->direction)) {
		e->state = e->direction > 0 ? STATE_UP : STATE_DOWN;
		mutex_unlock(&e->lock);
		ssleep(MOVE_SECONDS);

		mutex_lock(&e->lock);
		e->current_floor += e->direction;
		mutex_unlock(&e->lock);
		return;
	}

	if (e->stopping) {
		e->state = STATE_OFFLINE;
		e->stopping = false;
	} else {
		e->state = STATE_IDLE;
	}
	mutex_unlock(&e->lock);
	msleep(IDLE_POLL_MS);
}

int elevator_thread_fn(void *data)
{
	while (!kthread_should_stop())
		elevator_step(&elevator_data);

	return 0;
}