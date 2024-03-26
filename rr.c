#include <errno.h>
#include <fcntl.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

typedef uint32_t u32;
typedef int32_t i32;

struct process
{
  u32 pid;
  u32 arrival_time;
  u32 burst_time;

  TAILQ_ENTRY(process) pointers;

  /* Additional fields here */
  u32 first_run; //is this the first time the process has been run
  u32 first_run_time; //time the first process runs
  u32 time_left; //process time remaining
  u32 response_time; //time until ran
  u32 wait_time; //queue time
  /* End of "Additional fields here" */
};

TAILQ_HEAD(process_list, process);

u32 next_int(const char **data, const char *data_end)
{
  u32 current = 0;
  bool started = false;
  while (*data != data_end)
  {
    char c = **data;

    if (c < 0x30 || c > 0x39)
    {
      if (started)
      {
        return current;
      }
    }
    else
    {
      if (!started)
      {
        current = (c - 0x30);
        started = true;
      }
      else
      {
        current *= 10;
        current += (c - 0x30);
      }
    }

    ++(*data);
  }

  printf("Reached end of file while looking for another integer\n");
  exit(EINVAL);
}

u32 next_int_from_c_str(const char *data)
{
  char c;
  u32 i = 0;
  u32 current = 0;
  bool started = false;
  while ((c = data[i++]))
  {
    if (c < 0x30 || c > 0x39)
    {
      exit(EINVAL);
    }
    if (!started)
    {
      current = (c - 0x30);
      started = true;
    }
    else
    {
      current *= 10;
      current += (c - 0x30);
    }
  }
  return current;
}

void init_processes(const char *path,
                    struct process **process_data,
                    u32 *process_size)
{
  int fd = open(path, O_RDONLY);
  if (fd == -1)
  {
    int err = errno;
    perror("open");
    exit(err);
  }

  struct stat st;
  if (fstat(fd, &st) == -1)
  {
    int err = errno;
    perror("stat");
    exit(err);
  }

  u32 size = st.st_size;
  const char *data_start = mmap(NULL, size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (data_start == MAP_FAILED)
  {
    int err = errno;
    perror("mmap");
    exit(err);
  }

  const char *data_end = data_start + size;
  const char *data = data_start;

  *process_size = next_int(&data, data_end);

  *process_data = calloc(sizeof(struct process), *process_size);
  if (*process_data == NULL)
  {
    int err = errno;
    perror("calloc");
    exit(err);
  }

  for (u32 i = 0; i < *process_size; ++i)
  {
    (*process_data)[i].pid = next_int(&data, data_end);
    (*process_data)[i].arrival_time = next_int(&data, data_end);
    (*process_data)[i].burst_time = next_int(&data, data_end);
  }

  munmap((void *)data, size);
  close(fd);
}

int main(int argc, char *argv[])
{
  if (argc != 3)
  {
    return EINVAL;
  }
  struct process *data;
  u32 size;
  init_processes(argv[1], &data, &size);

  u32 quantum_length = next_int_from_c_str(argv[2]);

  struct process_list list;
  TAILQ_INIT(&list);

  u32 total_waiting_time = 0;
  u32 total_response_time = 0;

  /* Your code here */
  u32 curr_time = 0;
  u32 q_left = quantum_length;
  struct process *current_process = NULL;
  u32 total_process_time = 0; //When total burst time reaches 0 all of the processes have finished
  for(u32 i=0; i<size; i++)
  {
    total_process_time += data[i].burst_time;
    data[i].first_run = 0;
  }
  if(size==0)
  {
    total_waiting_time = 0;
    total_response_time = 0;
  }

  while(total_process_time!=0)
  {
    //if any process came in at this time insert it into queue, set remaining time to burst time
    for(u32 i=0; i<size; i++)
    {
      struct process *p;
      p = &data[i];
      if(p->arrival_time == curr_time)
      {
	    TAILQ_INSERT_TAIL(&list, p, pointers);
	    p->time_left = p->burst_time;
      }
    }
    //if no process is running and q has stuff, set first process as current
    if(current_process==NULL && !TAILQ_EMPTY(&list))
    {
      struct process *head = TAILQ_FIRST(&list);
      TAILQ_REMOVE(&list,head,pointers);
      current_process=head;
      if(!current_process->first_run)
      {
	      current_process->first_run=1;
	      current_process->first_run_time=curr_time;
      }
    }
    
    //if quantum expired or current process is done
    if(q_left == 0 || (current_process!=NULL && current_process->time_left==0))
    {
      //if current process is done update wait/response time
      if(current_process!=NULL && current_process->time_left==0)
      {
        current_process->response_time = current_process->first_run_time - current_process->arrival_time;
	      current_process->wait_time = curr_time - current_process->arrival_time - current_process->burst_time;
      }
      //else if current process needs more time add to back of queue
      else if(current_process!=NULL && current_process->time_left!=0)
      {
	      TAILQ_INSERT_TAIL(&list, current_process, pointers);
      }
      //set first process in queue as current
      if(!TAILQ_EMPTY(&list))
      {
        struct process *head = TAILQ_FIRST(&list);
        TAILQ_REMOVE(&list, head, pointers);
        current_process = head;
        if(!current_process->first_run){
          current_process->first_run=1;
          current_process->first_run_time = curr_time;
        }
        q_left = quantum_length;
      }
      else
      {
	      current_process = NULL;
	      q_left = quantum_length;
      }
    }

    curr_time++;
    if(current_process!=NULL)
    {
      total_process_time--;
      current_process->time_left--;
      q_left--;
    }
  }
  if(current_process!=NULL) //last process wait/response time, equations from discussion
  {
    current_process->response_time = current_process->first_run_time - current_process->arrival_time;
    current_process->wait_time = curr_time - current_process->arrival_time - current_process->burst_time;
  }
  
  for(u32 i=0; i<size; i++)
  {
    struct process *p = &data[i];
    total_response_time += p->response_time;
    total_waiting_time += p->wait_time;
  }
  /* End of "Your code here" */

  printf("Average waiting time: %.2f\n", (float)total_waiting_time / (float)size);
  printf("Average response time: %.2f\n", (float)total_response_time / (float)size);

  free(data);
  return 0;
}
