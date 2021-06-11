// 2018102150 ȫ����
#include <stdio.h> // fopen, fgets, fclose �Լ��� ����� ��� ����
#include <stdlib.h> // atoi ����� ���
#include "schedule.h"

// ��������
struct Queue* process_list = NULL; // txt������ ���μ��� ������ ��Ƶδ� ����ü
struct Queue* ready_queue = NULL; // ready queue, �� �տ� �ִ� ���μ����� CPU�� ��
struct Queue* CPU = NULL; // ���μ����� �Ҵ��� �� �ִ� CPU
int scheudling_method = -1; // �����ٸ� �޼ҵ� ���� 1�̸� FCFS, 2�̸� SJF, 3�̸� SRTF, 4�̸� RR
int num_of_process = -1; // txt���Ͽ� �����ִ� �� ���μ����� ����, �� ù �ٿ� �ش�
int time_quantum = 2; // RR�� ���̴� ���� �� time quantum�� �ش�. ����Ʈ ������ 2�� ���������� ���Ѵٸ� �߰������� ���� ����
int usage = 0; // RR�� ���̴� ������ CPU�� 1tick��ŭ ����ϸ� �ش� CPU�� usage�� ���� 1 �����Ѵ�. usage == time_quantum�̸� context switching�߻�

// ������ ������ ���� ����ü
struct Queue {
    int pid; // pid
    int arrival; // ������ �ð�
    int burst; // �ش� ���μ����� ���� CPU�� �Ҵ��ؾ� �ϴ� �ð�
    int finish; // �ش� ���μ����� �Ϸ�� ������ �ð�
    int first_CPU_allocated; // �ش� ���μ����� ó������ CPU�� �Ҵ�� �ð�
    struct Queue* next; // �ش� ����ü�� linked list����, location = location->next ���·� ���� �����Ϳ� ������ �� �ִ�.
};

void allocate_CPU(int _tick); // CPU�� ���μ����� �Ҵ��ϴ� �Լ�, ready_queue�� ����ִ� ���μ��� �� ���� �տ� �ִ� ���μ����� CPU�� �Ҵ� �� �� ready_queue�� �մ�� �����Ѵ�.
void insert(struct Queue* item); // ready_queue�� �����ؾ� �� process�� ����ִ� �Լ�
struct Queue* search(struct Queue* q, struct Queue* item); // ���μ����� ��Ƶ� q��� linked list���� item�̶�� ���μ����� ã���ִ� �Լ�, q��� linked list���� �ش��ϴ� ���μ����� �����͸� ��ȯ�Ѵ�.
void burst(struct Queue* _CPU); // CPU�� �Ҵ�� ���μ����� �����ϴ� �Լ�, 1tick�� 1�� �۾��� ����
int end_processing(struct Queue* _ready_queue, int _tick); // �����ؾ��� ���μ����� �� ������ ���α׷��� ���������� �����ϴ� �Լ�, �ٸ� �ִ� tick�� 100�� ����� �����ؾ��� ���μ����� �� �������� ���ϴ��� ���α׷��� ����� �� ����
void free_var(); // �����Ҵ�� ������ �������� �����������ִ� �Լ�, �޸� ���� ����
char* my_strtok(char* str, const char* delimiters); // char ���ڿ� �迭�� split���ִ� �Լ�, delimiters�� �������� ���ڿ��� �߶��ش�.

void read_proc_list(const char* file_name) {
    FILE* process_list_txt; // �ؽ�Ʈ ���� ���� ���� ����
    process_list_txt = fopen(file_name, "r"); // �ؽ�Ʈ ���� ����
    char process[20]; // �ִ� ũ�Ⱑ 20�� char�� ����, �̰ɷ� �ؽ�Ʈ ������ ���ڿ��� ����
    fgets(process, 20, process_list_txt); // �� ����(���μ��� ����) �б�
    num_of_process = atoi(process);  // ���μ��� ���� ����
    for (int i = 0; i < num_of_process; i++) {
        fgets(process, 20, process_list_txt); // �ؽ�Ʈ���� �� �پ� �б�
        if (process_list == NULL) {
            process_list = malloc(sizeof(struct Queue));
            // atoi�Լ��� char->int�� �������ִ� �Լ�, ���⸦ �������� pid, �����ð�, �����ؾ��ϴ� �ð��� process_list�� ����
            process_list->pid = atoi(my_strtok(process," "));
            process_list->arrival = atoi(my_strtok(NULL, " "));
            process_list->burst = atoi(my_strtok(NULL, " "));
            process_list->finish = -1; // ���μ��� �Ϸ� ����, �Ϸ���� ���� �ʱⰪ�� -1�� ����
            process_list->first_CPU_allocated = -1; // ���μ����� ó������ CPU�� �Ҵ�� ����, �Ҵ�� �� ���� �ʱⰪ�� -1�� ����
            process_list->next = NULL; // next�� NULL�̸� queue���� ������ ���Ҷ�� ���� ����
        }
        else {
            struct Queue* location = process_list;
            while (location->next != NULL) {
                location = location->next;
            }
            location->next = malloc(sizeof(struct Queue));
            location->next->pid = atoi(my_strtok(process, " "));
            location->next->arrival = atoi(my_strtok(NULL, " "));
            location->next->burst = atoi(my_strtok(NULL, " "));
            location->next->finish = -1;
            location->next->first_CPU_allocated = -1;
            location->next->next = NULL;
        }      
    }
}

void set_schedule(int method) {
    scheudling_method = method; // �����층 �޼ҵ� ���ϱ�, 1�̸� FCFS, 2�̸� SJF, 3�̸� SRTF, 4�̸� RR
    if (scheudling_method == 1) {
        printf("Scheudling method: FCFS: First Come First Served (Non-preemptive)\n\n");
    }
    else if (scheudling_method == 2) {
        printf("Scheudling method: SJF: Shortest Job First (Non-preemptive)\n\n");
    }
    else if (scheudling_method == 3) {
        printf("Scheudling method: SRTF: Shortest Remaining Time First (Preemptive)\n\n");
    }
    else if (scheudling_method == 4) {
        printf("Scheudling method: RR: Round Robin (Preemptive)\n\n");
    }
}


int do_schedule(int tick) {
    if (!end_processing(ready_queue, tick)) { // �������� ������ ��, �����ؾ��� ���μ����� �� ����ƴ����� �˻�, ��������� ���α׷� ����
        return 0;
    }

    struct Queue* location = process_list; // process_list�� ���� ����ü ����
    // ���� �ش� tick���� ������ process�� �ִ����� �˻�, ������ ���� �ִٸ� ��� ��, Insert�Լ��� ���� ready_queue�� �ش� process �Ҵ�
    while (location != NULL) {
        if (tick == location->arrival) {
            insert(location);
            printf("[tick: %02d] New Process (ID: %d) newly joins to ready queue\n", tick, location->pid);
        }
        location = location->next;
    }

    // FCFS
    if (scheudling_method == 1) {
        if (CPU == NULL) { // CPU�� ����ְ� ready_queue�� ���μ����� �ִٸ� allocate_CPU�� ���� ready_queue�� �ִ� �� ���� ���μ����� CPU�� �Ҵ� ��, ���μ��� ����(burst)
            if (ready_queue != NULL) {
                allocate_CPU(tick);
                burst(CPU);
            }
            else { // CPU�� ���� �Ҵ��� process�� ���ٸ� �Ѿ��(CPU �޽�)
                return 1;
            }
        }
        else {
            if (CPU->burst == 0 && ready_queue != NULL) { // CPU�� ����ִ� ���μ����� �Ϸ�ư� ������ ���μ����� �ִٸ� context switching�� ���� �� ���μ��� ����
                allocate_CPU(tick);
                burst(CPU);
            }
            else if (CPU->burst == 0 && ready_queue == NULL) { // CPU�� ����ִ� ���μ����� �Ϸ�ư� ������ ���μ����� ���ٸ� CPU�� �ִ� ���μ��� ������ CPU �޽�
                allocate_CPU(tick);
                return 1;
            }
            else { // CPU�� �ִ� ���μ����� �Ϸ���� �ʾҴٸ� ���μ��� ����ؼ� ����
                burst(CPU);
            }
        }
    }
    // SJF
    // FCFS�� �������� �����, �ٸ� Insert �Լ��� ���� FCFS�� unsorted linked list�� �ݸ�, SJF�� sorted linked list��
    else if (scheudling_method == 2) {
        if (CPU == NULL) { // CPU�� ����ְ� ready_queue�� ���μ����� �ִٸ� allocate_CPU�� ���� ready_queue�� �ִ� �� ���� ���μ����� CPU�� �Ҵ� ��, ���μ��� ����(burst)
            allocate_CPU(tick);
            burst(CPU);
        }
        else {
            if (CPU->burst == 0 && ready_queue != NULL) { // CPU�� ����ִ� ���μ����� �Ϸ�ư� ������ ���μ����� �ִٸ� context switching�� ���� �� ���μ��� ����
                allocate_CPU(tick);
                burst(CPU);
            }
            else { // CPU�� �ִ� ���μ����� �Ϸ���� �ʾҴٸ� ���μ��� ����ؼ� ����
                burst(CPU);
            }
        }
    }
    // SRTF
    else if (scheudling_method == 3) {
        if (CPU == NULL) { // CPU�� ����ְ� ready_queue�� ���μ����� �ִٸ� allocate_CPU�� ���� ready_queue�� �ִ� �� ���� ���μ����� CPU�� �Ҵ� ��, ���μ��� ����(burst)
            allocate_CPU(tick);
            burst(CPU);
        }
        else {
            if (CPU->burst != 0) {
                if (ready_queue != NULL) { // CPU�� ����ִ� ���μ����� �Ϸ�� �� ������ ������ ���μ��� �� ����ð��� ���� CPU�� �Ҵ�� ���μ����� ���� ����ð����� ���� ���μ����� �ִٸ� context switching�� ���� �� ���μ��� ����
                    if (ready_queue->burst < CPU->burst) {
                        insert(CPU);
                        allocate_CPU(tick);
                        burst(CPU);
                    } // ���� ����� ���μ����� ���ٸ� CPU�� �Ҵ�Ǿ� �ִ� ���μ��� ����
                    else {
                        burst(CPU);
                    }
                }
                else { // ���� ����� ���μ����� ���ٸ� CPU�� �Ҵ�Ǿ� �ִ� ���μ��� ����
                    burst(CPU);
                }
            } else if (CPU->burst == 0 && ready_queue != NULL) { // CPU�� ����ִ� ���μ����� �Ϸ�ư� ������ ���μ����� �ִٸ� context switching�� ���� �� ���μ��� ����
                allocate_CPU(tick);
                burst(CPU);
            }
            else { // ���� ����� ���μ����� ���ٸ� CPU�� �Ҵ�Ǿ� �ִ� ���μ��� ����
                burst(CPU);
            }
        }
    }
    // RR
    else if (scheudling_method == 4) {
        if (CPU == NULL) { // CPU�� ����ְ� ready_queue�� ���μ����� �ִٸ� allocate_CPU�� ���� ready_queue�� �ִ� �� ���� ���μ����� CPU�� �Ҵ� ��, ���μ��� ����(burst)
            allocate_CPU(tick);
            burst(CPU);
            usage++;
        }
        else {
            if (CPU->burst != 0 && usage < time_quantum) { // CPU�� �Ҵ�� ���μ����� �ȷ���� �ʾҰ� ��뷮(usage)�� �Ѱ跮(time_quantum)�� �Ѿ�� ������ CPU�� �Ҵ�� ���μ��� ����
                burst(CPU);
                usage++; // ��뷮 1�߰�(=����� �� �ִ� �� 1 ����)
            }
            else if (CPU->burst == 0 && ready_queue != NULL) { // CPU�� ����ִ� ���μ����� �Ϸ�ư� ������ ���μ����� �ִٸ� context switching�� ���� �� ���μ��� ����
                allocate_CPU(tick);
                usage = 0; // ��뷮 �ʱ�ȭ
                burst(CPU);
                usage++; // ��뷮 1�߰�(=����� �� �ִ� �� 1 ����)
            }
            else if (usage == time_quantum) { // ��뷮�� �� ���� �� ready_queue�� CPU�� �Ҵ�� ���μ��� �־��ְ� context switching�� ���� �� ���μ��� ����
                insert(CPU);
                allocate_CPU(tick);
                usage = 0; // ��뷮 �ʱ�ȭ
                burst(CPU);
                usage++; // ��뷮 1�߰�(=����� �� �ִ� �� 1 ����)
            }
            else { // ��뷮�� �� �� ���� CPU�� �Ҵ�� ���μ����� �� �� �����ٸ� CPU�� �Ҵ�� ���μ��� ����
                burst(CPU);
                usage++; // ��뷮 1�߰�(=����� �� �ִ� �� 1 ����)
            }
        }
    }

    return 1;
}

// ���� ����� �����ִ� �Լ�
void print_performance() {
    float sum_turn_around_time = 0.0; // �� ���λ��� ���� Turn around time�� ������ ����
    float sum_waiting_time = 0.0; // �� ���λ��� ���� Waiting time�� ������ ����
    float sum_responss_time = 0.0; // �� ���λ��� ���� Response time�� ������ ����

    printf("==============================================================================\n");
    printf("  PID  arrival  finish  burst  Turn around time  Waiting time  Response tiem\n");
    printf("==============================================================================\n");

    struct Queue* location = process_list;
    while (location != NULL) {
        int turn_around_time = location->finish - location->arrival; // finish tick - arrival tick
        int waiting_time = location->finish - location->arrival - location->burst; // finish tick - arrival tick - burst tick
        int responss_time = location->first_CPU_allocated - location->arrival; // first tick the CPU was first allocated - arrival tick
        sum_turn_around_time += turn_around_time; // �� ���λ��� ���� Turn around time�� ����
        sum_waiting_time += waiting_time; // �� ���λ��� ���� Waiting time�� ����
        sum_responss_time += responss_time; // �� ���λ��� ���� Response time�� ����
        printf("  %2d     %2d       %2d     %2d          %2d               %2d             %2d\n", location->pid, location->arrival, location->finish, location->burst, turn_around_time, waiting_time, responss_time);
        location = location->next;
    }

    printf("------------------------------------------------------------------------------\n");
    printf(" average:                           %0.2f              %0.2f           %0.2f\n", sum_turn_around_time / num_of_process, sum_waiting_time / num_of_process, sum_responss_time / num_of_process); // �� ������ ���
    printf("==============================================================================");

    free_var(); // �����Ҵ� ����, �޸� ���� ����
}

// CPU�� ���μ��� �Ҵ��ϴ� �Լ�, �Ҵ��� ���μ����� ready_queue���� �����´�.
void allocate_CPU(int _tick) {
    if (CPU != NULL) { // CPU�� ������� �ʾ��� ������ �� context switching�� �߻��ϴ� ���, �ش� ���μ����� �Ϸ������ �Ϸ�� �ð� ����
        struct Queue* location = search(process_list, CPU);
        if(location->finish == -1 && CPU->burst == 0) // ���μ��� �Ϸ�� �ð� �ߺ� ���� ����
            location->finish = _tick;
    }

    if (ready_queue != NULL) { // ready_queue�� ������� ���� �� CPU�� ���μ��� �Ҵ� ����
        CPU = malloc(sizeof(struct Queue));
        CPU->pid = ready_queue->pid;
        CPU->arrival = ready_queue->arrival;
        CPU->burst = ready_queue->burst;
        CPU->finish = ready_queue->finish;
        CPU->first_CPU_allocated = ready_queue->first_CPU_allocated;
        CPU->next = NULL;

        ready_queue = ready_queue->next; // �� ���� ���������� �մ��
        printf("[tick: %02d] Dispatch to Process (ID: %d)\n", _tick, CPU->pid);

        if (CPU->first_CPU_allocated == -1) { // �ش� ���μ����� CPU�� �Ҵ�� �̷��� ������ �ش� ���μ����� CPU�� �Ҵ�Ǵ� ���� tick�� ����
            struct Queue* location = search(process_list, CPU);
            location->first_CPU_allocated = _tick;
            CPU->first_CPU_allocated = _tick;
        }
    }
}

// ready_queue�� ���μ����� �ִ� �Լ�, ������ method�� ���� �ִ� �˰����� ����, FCFS�� RR�� �������� �ʰ� ���μ����� �ְ�, SJF, SRTF�� �������� ���ķ� ready_queue�� ���μ��� ����
void insert(struct Queue* item) {
    if (ready_queue == NULL) { // ready_queue�� ��� ������ �����̵� �ƴϵ� ready_queue �� �տ��� ���μ����� �ִ´�.
        ready_queue = malloc(sizeof(struct Queue));
        ready_queue->pid = item->pid;
        ready_queue->arrival = item->arrival;
        ready_queue->burst = item->burst;
        ready_queue->finish = item->finish;
        ready_queue->first_CPU_allocated = item->first_CPU_allocated;
        ready_queue->next = NULL;
        return;
    }

    struct Queue* location = ready_queue; // ready_queue�� ���ƴٴϸ� ���忡 �ʿ��� ����
    if (scheudling_method == 1 || scheudling_method == 4) { // ������ �ֱ�
        while (location->next != NULL) { // �� �������� ���μ����� �����Ѵ�.
            location = location->next;
        }
        location->next = malloc(sizeof(struct Queue));
        location->next->pid = item->pid;
        location->next->arrival = item->arrival;
        location->next->burst = item->burst;
        location->next->finish = item->finish;
        location->next->first_CPU_allocated = item->first_CPU_allocated;
        location->next->next = NULL;
    }
    else if (scheudling_method == 2 || scheudling_method == 3) { // ���� �ֱ�
        if(location != NULL) {
            if (location->burst > item->burst) { // ������������ ���μ��� ����, ���� ��ġ�� �߰����̶�� �� ���� ������ �� ���θ� �ִ� �۾� ����
                ready_queue = malloc(sizeof(struct Queue));
                ready_queue->pid = item->pid;
                ready_queue->arrival = item->arrival;
                ready_queue->burst = item->burst;
                ready_queue->finish = item->finish;
                ready_queue->first_CPU_allocated = item->first_CPU_allocated;
                ready_queue->next = location;
                return;
            }
        }
        while (location->next != NULL) {
            if (location->next->burst > item->burst) {
                break;
            }
            location = location->next;
        }
        if (location->next != NULL) {
            struct Queue* location_back = location->next;
            location->next = malloc(sizeof(struct Queue));
            location->next->pid = item->pid;
            location->next->arrival = item->arrival;
            location->next->burst = item->burst;
            location->next->finish = item->finish;
            location->next->first_CPU_allocated = item->first_CPU_allocated;
            location->next->next = location_back;

        }
        else {
            location->next = malloc(sizeof(struct Queue));
            location->next->pid = item->pid;
            location->next->arrival = item->arrival;
            location->next->burst = item->burst;
            location->next->finish = item->finish;
            location->next->first_CPU_allocated = item->first_CPU_allocated;
            location->next->next = NULL;
        }
    }
}

// q��� linked list���� item�̶�� ���μ����� ã�� q���� item�� �ش��ϴ� ���μ����� �����͸� ��ȯ�ϴ� �Լ�
struct Queue* search(struct Queue* q, struct Queue* item) {
    struct Queue* location = q;
    while (location->pid != item->pid) {
        location = location->next;
    }
    return location;
}

// CPU�� �Ҵ�� ���μ��� �����ϴ� �Լ�
void burst(struct Queue* _CPU) {
    if (_CPU != NULL) { // CPU�� ������� ���� ��쿡 ����
        if (_CPU->burst > 0)
            _CPU->burst--;
    }
}

// ���α׷� ���� ���� �Ǵ� �Լ�, �ؽ�Ʈ ���Ͽ� �ִ� ���μ����� ��� ������ �Ϸ�Ǹ� ���α׷� ����(return 0), �ƴϸ� ����ؼ� ���α׷� ����(return 1)
int end_processing(struct Queue* _ready_queue, int _tick) {
    if (CPU != NULL) {
        if (_ready_queue == NULL && CPU->burst == 0) { // ���μ����� �� �������� ���μ��� ���� ���� �ð�(tick)���� �� ���α׷� ����
            struct Queue* location = search(process_list, CPU);
            if(location->finish == -1) // �ߺ����� ����
                location->finish = _tick;

            struct Queue* location2 = process_list;
            while (location2 != NULL) {
                if (location2->finish == -1) { // CPU�� �Ҵ�� ���μ����� �� ������ ready_queue���� ���μ����� ������ ��� ���μ����� ����ƴٰ� �������� ������ �ؽ�Ʈ ���Ͽ� �ִ� ��� ���μ����� �� ������ �ƴ϶�� ���α׷� ����ؼ� ����
                    return 1;
                }
                else {
                    location2 = location2->next;
                }
            }
            printf("[tick: %02d] All processes are terminated.\n\n", _tick);
            return 0; // ���α׷� ����
        }
    } 
    return 1;
}

// process_list, ready_queue, CPU�� �����Ҵ��� �͵� �������ִ� �Լ�
void free_var() {
    struct Queue* location = process_list; // process_list ���� �Ҵ� ����
    struct Queue* pre;
    do {
        pre = location;
        if (location != NULL)
            location = location->next;
        if (pre != NULL)
            free(pre);
    } while (location != NULL);

    location = ready_queue; // ready_queue ���� �Ҵ� ����
    do {
        pre = location;
        if (location != NULL)
            location = location->next;
        if (pre != NULL)
            free(pre);
    } while (location != NULL);

    location = CPU; // CPU ���� �Ҵ� ����
    do {
        pre = location;
        if (location != NULL)
            location = location->next;
        if (pre != NULL)
            free(pre);
    } while (location != NULL);
}

// string.h�� �ִ� strtok �Լ� ����, str�� delimiters �������� ��� �߶󳽴�.
char* my_strtok(char* str, const char* delimiters) {
    static char* pCurrent;
    char* pDelimit;

    if (str != NULL)pCurrent = str;
    else str = pCurrent;

    if (*pCurrent == NULL) return NULL; // str�� NULL�̸� NULL��ȯ

    //���ڿ� ����
    while (*pCurrent)
    {
        pDelimit = (char*)delimiters;

        while (*pDelimit) {
            if (*pCurrent == *pDelimit) {
                *pCurrent = NULL;
                ++pCurrent;
                return str;
            }
            ++pDelimit;
        }
        ++pCurrent;
    }
    // ���̻� �ڸ� �� ���ٸ� NULL��ȯ
    return str;
}