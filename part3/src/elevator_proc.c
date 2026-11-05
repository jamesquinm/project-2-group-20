#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/list.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "pet_elevator.h"

#define PROC_NAME "elevator"
#define PROC_PERMS 0444

static struct proc_dir_entry *elevator_entry;

static const char *state_name(enum elevator_state state)
{
	static const char *const names[] = { "OFFLINE", "IDLE", "LOADING", "UP", "DOWN" };

	return names[state];
}

static void show_pets(struct seq_file *m, struct list_head *pets)
{
	struct pet *pet;

	list_for_each_entry(pet, pets, node)
		seq_printf(m, " %c%d", pet_letter(pet->type), pet->destination_floor);
}

static void show_floor(struct seq_file *m, struct pet_elevator *e, int floor)
{
	struct list_head *pets = &e->floors[floor - MIN_FLOOR];

	seq_printf(m, "[%c] Floor %d: %zu", floor == e->current_floor ? '*' : ' ',
		   floor, list_count_nodes(pets));
	show_pets(m, pets);
	seq_putc(m, '\n');
}

static int elevator_show(struct seq_file *m, void *unused)
{
	struct pet_elevator *e = &elevator_data;
	int floor;

	mutex_lock(&e->lock);

	seq_printf(m, "Elevator state: %s\n", state_name(e->state));
	seq_printf(m, "Current floor: %d\n", e->current_floor);
	seq_printf(m, "Current load: %d lbs\n", e->load_lbs);
	seq_puts(m, "Elevator status:");
	show_pets(m, &e->riders);
	seq_puts(m, "\n\n");

	for (floor = MAX_FLOOR; floor >= MIN_FLOOR; floor--)
		show_floor(m, e, floor);

	seq_printf(m, "\nNumber of pets: %d\n", e->pet_count);
	seq_printf(m, "Number of pets waiting: %d\n", e->waiting_count);
	seq_printf(m, "Number of pets serviced: %d\n", e->serviced_count);

	mutex_unlock(&e->lock);
	return 0;
}

static int elevator_open(struct inode *inode, struct file *file)
{
	return single_open(file, elevator_show, NULL);
}

static const struct proc_ops elevator_proc_ops = {
	.proc_open = elevator_open,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

int elevator_proc_create(void)
{
	elevator_entry = proc_create(PROC_NAME, PROC_PERMS, NULL, &elevator_proc_ops);
	return elevator_entry ? 0 : -ENOMEM;
}

void elevator_proc_remove(void)
{
	proc_remove(elevator_entry);
}