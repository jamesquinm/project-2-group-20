#include <linux/fs.h>
#include <linux/init.h>
#include <linux/kernel.h>
#include <linux/ktime.h>
#include <linux/module.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/time64.h>
#include <linux/timekeeping.h>

#define PROC_NAME "timer"
#define PROC_PERMS 0444
#define OUTPUT_SIZE 128

MODULE_LICENSE("GPL");
MODULE_AUTHOR("James Mosely");
MODULE_DESCRIPTION("Reports the current time and the time since the last read");

static struct proc_dir_entry *timer_entry;
static struct timespec64 last_time;
static bool has_last_time;
static DEFINE_MUTEX(timer_lock);

/* Builds the text for one read and records this read's time. */
static int format_timer_output(char *output, size_t size)
{
	struct timespec64 now;
	struct timespec64 elapsed;
	int length;

	ktime_get_real_ts64(&now);

	length = scnprintf(output, size, "current time: %lld.%09ld\n",
			   (long long)now.tv_sec, now.tv_nsec);

	if (has_last_time) {
		elapsed = timespec64_sub(now, last_time);
		length += scnprintf(output + length, size - length,
				    "elapsed time: %lld.%09ld\n",
				    (long long)elapsed.tv_sec, elapsed.tv_nsec);
	}

	last_time = now;
	has_last_time = true;
	return length;
}

static ssize_t timer_read(struct file *file, char __user *user_buffer,
			  size_t count, loff_t *offset)
{
	static char output[OUTPUT_SIZE];
	static int output_length;
	ssize_t bytes_read;

	mutex_lock(&timer_lock);

	/* cat reads until EOF, so only a read from the start counts as a new call. */
	if (*offset == 0)
		output_length = format_timer_output(output, sizeof(output));

	bytes_read = simple_read_from_buffer(user_buffer, count, offset, output,
					     output_length);

	mutex_unlock(&timer_lock);
	return bytes_read;
}

static const struct proc_ops timer_proc_ops = {
	.proc_read = timer_read,
};

static int __init my_timer_init(void)
{
	timer_entry = proc_create(PROC_NAME, PROC_PERMS, NULL, &timer_proc_ops);
	if (!timer_entry)
		return -ENOMEM;

	return 0;
}

static void __exit my_timer_exit(void)
{
	proc_remove(timer_entry);
}

module_init(my_timer_init);
module_exit(my_timer_exit);