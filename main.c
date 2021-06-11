#include <stdio.h>
#include <stdlib.h>
#include "schedule.h"

// global variables
int tick = 0;

int main(int argc, char* argv[])
{
	//char file_name[1024] = "proc_list.txt";
	//char file_name[1024] = "rr.txt";
	//int schedule_method = 4;

	char file_name[1024] = "";
	int schedule_method = -1;

	printf("Enter process file name:");
	scanf("%s", file_name);
	printf("Enter process scheduling mehtod \n");
	printf("1: FCFS: First Come First Served (Non-preemptive) \n");
	printf("2: SJF: Shortest Job First (Non-preemptive) \n");
	printf("3: SRTF: Shortest Remaining Time First (Preemptive) \n");
	printf("4: RR: Round Robin (Preemptive) \n");
	scanf("%d", &schedule_method);

	set_schedule(schedule_method);
	// set processes
	read_proc_list(file_name);

	while(1){
		int res = do_schedule(tick);
		if (res == 0 || tick > 100) break;
		tick++;
	}
	print_performance();

	return 0;

}



