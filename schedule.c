#include <stdio.h> // fopen, fgets, fclose 함수가 선언된 헤더 파일
#include <stdlib.h> // atoi 선언된 헤더
#include "schedule.h"

// 전역변수
struct Queue* process_list = NULL; // txt파일의 프로세스 정보를 담아두는 저장체
struct Queue* ready_queue = NULL; // ready queue, 맨 앞에 있는 프로세스가 CPU에 들어감
struct Queue* CPU = NULL; // 프로세스를 할당할 수 있는 CPU
int scheudling_method = -1; // 스케줄링 메소드 저장 1이면 FCFS, 2이면 SJF, 3이면 SRTF, 4이면 RR
int num_of_process = -1; // txt파일에 적혀있는 총 프로세스의 개수, 맨 첫 줄에 해당
int time_quantum = 2; // RR에 쓰이는 변수 중 time quantum에 해당. 디폴트 값으로 2로 설정했지만 원한다면 추가적으로 변경 가능
int usage = 0; // RR에 쓰이는 변수로 CPU를 1tick만큼 사용하면 해당 CPU의 usage의 값이 1 증가한다. usage == time_quantum이면 context switching발생

// 데이터 저장을 위한 구조체
struct Queue {
    int pid; // pid
    int arrival; // 도착한 시간
    int burst; // 해당 프로세스를 위해 CPU에 할당해야 하는 시간
    int finish; // 해당 프로세스가 완료된 시점의 시간
    int first_CPU_allocated; // 해당 프로세스가 처음으로 CPU에 할당된 시간
    struct Queue* next; // 해당 구조체는 linked list형태, location = location->next 형태로 다음 데이터에 접근할 수 있다.
};

void allocate_CPU(int _tick); // CPU에 프로세스를 할당하는 함수, ready_queue에 들어있는 프로세스 중 가장 앞에 있는 프로세스를 CPU에 할당 그 후 ready_queue를 앞당겨 정렬한다.
void insert(struct Queue* item); // ready_queue에 수행해야 할 process를 집어넣는 함수
struct Queue* search(struct Queue* q, struct Queue* item); // 프로세스를 모아둔 q라는 linked list에서 item이라는 프로세스를 찾아주는 함수, q라는 linked list에서 해당하는 프로세스의 포인터를 반환한다.
void burst(struct Queue* _CPU); // CPU에 할당된 프로세스를 수행하는 함수, 1tick당 1의 작업을 수행
int end_processing(struct Queue* _ready_queue, int _tick); // 수행해야할 프로세스를 다 끝내서 프로그램을 종료할지를 결정하는 함수, 다만 최대 tick이 100인 관계로 수행해야할 프로세스를 다 수행하지 못하더라도 프로그램이 종료될 수 있음
void free_var(); // 동적할당된 변수들 마지막에 동적해제해주는 함수, 메모리 누수 방지
char* my_strtok(char* str, const char* delimiters); // char 문자열 배열을 split해주는 함수, delimiters를 기준으로 문자열을 잘라준다.

void read_proc_list(const char* file_name) {
    FILE* process_list_txt; // 텍스트 파일 담을 변수 선언
    process_list_txt = fopen(file_name, "r"); // 텍스트 파일 열기
    char process[20]; // 최대 크기가 20인 char형 선언, 이걸로 텍스트 파일의 문자열을 담음
    fgets(process, 20, process_list_txt); // 맨 윗줄(프로세스 개수) 읽기
    num_of_process = atoi(process);  // 프로세수 개수 저장
    for (int i = 0; i < num_of_process; i++) {
        fgets(process, 20, process_list_txt); // 텍스트파일 한 줄씩 읽기
        if (process_list == NULL) {
            process_list = malloc(sizeof(struct Queue));
            // atoi함수는 char->int로 변형해주는 함수, 띄어쓰기를 기준으로 pid, 도착시간, 수행해야하는 시간을 process_list에 저장
            process_list->pid = atoi(my_strtok(process," "));
            process_list->arrival = atoi(my_strtok(NULL, " "));
            process_list->burst = atoi(my_strtok(NULL, " "));
            process_list->finish = -1; // 프로세스 완료 시점, 완료되지 않은 초기값은 -1로 설정
            process_list->first_CPU_allocated = -1; // 프로세스가 처음으로 CPU에 할당된 시점, 할당된 적 없는 초기값은 -1로 설정
            process_list->next = NULL; // next가 NULL이면 queue에서 마지막 원소라는 것을 뜻함
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
    scheudling_method = method; // 스케쥴링 메소드 정하기, 1이면 FCFS, 2이면 SJF, 3이면 SRTF, 4이면 RR
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
    if (!end_processing(ready_queue, tick)) { // 스케쥴을 시작할 때, 수행해야할 프로세스가 다 수행됐는지를 검사, 수행됐으면 프로그램 종료
        return 0;
    }

    struct Queue* location = process_list; // process_list를 읽을 구조체 선언
    // 먼저 해당 tick에서 도착한 process가 있는지를 검사, 도착한 것이 있다면 명시 후, Insert함수를 통해 ready_queue에 해당 process 할당
    while (location != NULL) {
        if (tick == location->arrival) {
            insert(location);
            printf("[tick: %02d] New Process (ID: %d) newly joins to ready queue\n", tick, location->pid);
        }
        location = location->next;
    }

    // FCFS
    if (scheudling_method == 1) {
        if (CPU == NULL) { // CPU가 비어있고 ready_queue에 프로세스가 있다면 allocate_CPU를 통해 ready_queue에 있는 맨 앞의 프로세스를 CPU에 할당 후, 프로세스 수행(burst)
            if (ready_queue != NULL) {
                allocate_CPU(tick);
                burst(CPU);
            }
            else { // CPU가 비었어도 할당할 process가 없다면 넘어가기(CPU 휴식)
                return 1;
            }
        }
        else {
            if (CPU->burst == 0 && ready_queue != NULL) { // CPU에 들어있는 프로세스가 완료됐고 수행할 프로세스가 있다면 context switching을 진행 후 프로세스 수행
                allocate_CPU(tick);
                burst(CPU);
            }
            else if (CPU->burst == 0 && ready_queue == NULL) { // CPU에 들어있는 프로세스가 완료됐고 수행할 프로세스가 없다면 CPU에 있는 프로세스 꺼내고 CPU 휴식
                allocate_CPU(tick);
                return 1;
            }
            else { // CPU에 있는 프로세스가 완료되지 않았다면 프로세스 계속해서 수행
                burst(CPU);
            }
        }
    }
    // SJF
    // FCFS와 수행방법은 비슷함, 다만 Insert 함수를 보면 FCFS는 unsorted linked list인 반면, SJF는 sorted linked list임
    else if (scheudling_method == 2) {
        if (CPU == NULL) { // CPU가 비어있고 ready_queue에 프로세스가 있다면 allocate_CPU를 통해 ready_queue에 있는 맨 앞의 프로세스를 CPU에 할당 후, 프로세스 수행(burst)
            allocate_CPU(tick);
            burst(CPU);
        }
        else {
            if (CPU->burst == 0 && ready_queue != NULL) { // CPU에 들어있는 프로세스가 완료됐고 수행할 프로세스가 있다면 context switching을 진행 후 프로세스 수행
                allocate_CPU(tick);
                burst(CPU);
            }
            else { // CPU에 있는 프로세스가 완료되지 않았다면 프로세스 계속해서 수행
                burst(CPU);
            }
        }
    }
    // SRTF
    else if (scheudling_method == 3) {
        if (CPU == NULL) { // CPU가 비어있고 ready_queue에 프로세스가 있다면 allocate_CPU를 통해 ready_queue에 있는 맨 앞의 프로세스를 CPU에 할당 후, 프로세스 수행(burst)
            allocate_CPU(tick);
            burst(CPU);
        }
        else {
            if (CPU->burst != 0) {
                if (ready_queue != NULL) { // CPU에 들어있는 프로세스가 완료는 안 됐지만 수행할 프로세스 중 수행시간이 현재 CPU에 할당된 프로세스의 남은 수행시간보다 작은 프로세스가 있다면 context switching을 진행 후 프로세스 수행
                    if (ready_queue->burst < CPU->burst) {
                        insert(CPU);
                        allocate_CPU(tick);
                        burst(CPU);
                    } // 위에 경우의 프로세스가 없다면 CPU에 할당되어 있는 프로세스 수행
                    else {
                        burst(CPU);
                    }
                }
                else { // 위에 경우의 프로세스가 없다면 CPU에 할당되어 있는 프로세스 수행
                    burst(CPU);
                }
            } else if (CPU->burst == 0 && ready_queue != NULL) { // CPU에 들어있는 프로세스가 완료됐고 수행할 프로세스가 있다면 context switching을 진행 후 프로세스 수행
                allocate_CPU(tick);
                burst(CPU);
            }
            else { // 위에 경우의 프로세스가 없다면 CPU에 할당되어 있는 프로세스 수행
                burst(CPU);
            }
        }
    }
    // RR
    else if (scheudling_method == 4) {
        if (CPU == NULL) { // CPU가 비어있고 ready_queue에 프로세스가 있다면 allocate_CPU를 통해 ready_queue에 있는 맨 앞의 프로세스를 CPU에 할당 후, 프로세스 수행(burst)
            allocate_CPU(tick);
            burst(CPU);
            usage++;
        }
        else {
            if (CPU->burst != 0 && usage < time_quantum) { // CPU에 할당된 프로세스가 안료되지 않았고 사용량(usage)가 한계량(time_quantum)을 넘어서지 않으면 CPU에 할당된 프로세스 수행
                burst(CPU);
                usage++; // 사용량 1추가(=사용할 수 있는 양 1 차감)
            }
            else if (CPU->burst == 0 && ready_queue != NULL) { // CPU에 들어있는 프로세스가 완료됐고 수행할 프로세스가 있다면 context switching을 진행 후 프로세스 수행
                allocate_CPU(tick);
                usage = 0; // 사용량 초기화
                burst(CPU);
                usage++; // 사용량 1추가(=사용할 수 있는 양 1 차감)
            }
            else if (usage == time_quantum) { // 사용량을 다 썼을 때 ready_queue에 CPU에 할당된 프로세스 넣어주고 context switching을 진행 후 프로세스 수행
                insert(CPU);
                allocate_CPU(tick);
                usage = 0; // 사용량 초기화
                burst(CPU);
                usage++; // 사용량 1추가(=사용할 수 있는 양 1 차감)
            }
            else { // 사용량도 다 안 ㎞ CPU에 할당된 프로세스가 다 안 끝났다면 CPU에 할당된 프로세스 수행
                burst(CPU);
                usage++; // 사용량 1추가(=사용할 수 있는 양 1 차감)
            }
        }
    }

    return 1;
}

// 수행 결과를 보여주는 함수
void print_performance() {
    float sum_turn_around_time = 0.0; // 각 프로새스 들의 Turn around time을 총합한 변수
    float sum_waiting_time = 0.0; // 각 프로새스 들의 Waiting time을 총합한 변수
    float sum_responss_time = 0.0; // 각 프로새스 들의 Response time을 총합한 변수

    printf("==============================================================================\n");
    printf("  PID  arrival  finish  burst  Turn around time  Waiting time  Response tiem\n");
    printf("==============================================================================\n");

    struct Queue* location = process_list;
    while (location != NULL) {
        int turn_around_time = location->finish - location->arrival; // finish tick - arrival tick
        int waiting_time = location->finish - location->arrival - location->burst; // finish tick - arrival tick - burst tick
        int responss_time = location->first_CPU_allocated - location->arrival; // first tick the CPU was first allocated - arrival tick
        sum_turn_around_time += turn_around_time; // 각 프로새스 들의 Turn around time을 더힘
        sum_waiting_time += waiting_time; // 각 프로새스 들의 Waiting time을 더함
        sum_responss_time += responss_time; // 각 프로새스 들의 Response time을 더함
        printf("  %2d     %2d       %2d     %2d          %2d               %2d             %2d\n", location->pid, location->arrival, location->finish, location->burst, turn_around_time, waiting_time, responss_time);
        location = location->next;
    }

    printf("------------------------------------------------------------------------------\n");
    printf(" average:                           %0.2f              %0.2f           %0.2f\n", sum_turn_around_time / num_of_process, sum_waiting_time / num_of_process, sum_responss_time / num_of_process); // 각 변수들 평균
    printf("==============================================================================");

    free_var(); // 동적할당 해제, 메모리 누수 방지
}

// CPU에 프로세스 할당하는 함수, 할당한 프로세스는 ready_queue에서 가져온다.
void allocate_CPU(int _tick) {
    if (CPU != NULL) { // CPU가 비어있지 않았을 상태일 때 context switching이 발생하는 경우, 해당 프로세스가 완료됐으면 완료된 시간 저장
        struct Queue* location = search(process_list, CPU);
        if(location->finish == -1 && CPU->burst == 0) // 프로세스 완료된 시간 중복 저장 방지
            location->finish = _tick;
    }

    if (ready_queue != NULL) { // ready_queue가 비어있지 않을 때 CPU에 프로세스 할당 가능
        CPU = malloc(sizeof(struct Queue));
        CPU->pid = ready_queue->pid;
        CPU->arrival = ready_queue->arrival;
        CPU->burst = ready_queue->burst;
        CPU->finish = ready_queue->finish;
        CPU->first_CPU_allocated = ready_queue->first_CPU_allocated;
        CPU->next = NULL;

        ready_queue = ready_queue->next; // 맨 앞이 빠져나가니 앞당김
        printf("[tick: %02d] Dispatch to Process (ID: %d)\n", _tick, CPU->pid);

        if (CPU->first_CPU_allocated == -1) { // 해당 프로세스가 CPU에 할당된 이력이 없으면 해당 프로세스가 CPU에 할당되는 현재 tick을 저장
            struct Queue* location = search(process_list, CPU);
            location->first_CPU_allocated = _tick;
            CPU->first_CPU_allocated = _tick;
        }
    }
}

// ready_queue에 프로세스를 넣는 함수, 스케쥴 method에 따라 넣는 알고리즘이 상이, FCFS와 RR은 정렬하지 않고 프로세스를 넣고, SJF, SRTF는 오름차순 정렬로 ready_queue에 프로세스 저장
void insert(struct Queue* item) {
    if (ready_queue == NULL) { // ready_queue가 비어 있으면 정렬이든 아니든 ready_queue 맨 앞에다 프로세스를 넣는다.
        ready_queue = malloc(sizeof(struct Queue));
        ready_queue->pid = item->pid;
        ready_queue->arrival = item->arrival;
        ready_queue->burst = item->burst;
        ready_queue->finish = item->finish;
        ready_queue->first_CPU_allocated = item->first_CPU_allocated;
        ready_queue->next = NULL;
        return;
    }

    struct Queue* location = ready_queue; // ready_queue를 돌아다니며 저장에 필요한 변수
    if (scheudling_method == 1 || scheudling_method == 4) { // 비정렬 넣기
        while (location->next != NULL) { // 맨 마지막에 프로세스를 저장한다.
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
    else if (scheudling_method == 2 || scheudling_method == 3) { // 정렬 넣기
        if(location != NULL) {
            if (location->burst > item->burst) { // 오름차순으로 프로세스 저장, 저장 위치가 중간쯤이라면 그 사이 비집고 들어가 서로를 있는 작업 수행
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

// q라는 linked list에서 item이라는 프로세스를 찾아 q에서 item에 해당하는 프로세스의 포인터를 반환하는 함수
struct Queue* search(struct Queue* q, struct Queue* item) {
    struct Queue* location = q;
    while (location->pid != item->pid) {
        location = location->next;
    }
    return location;
}

// CPU에 할당된 프로세스 수행하는 함수
void burst(struct Queue* _CPU) {
    if (_CPU != NULL) { // CPU가 비어있지 않을 경우에 수행
        if (_CPU->burst > 0)
            _CPU->burst--;
    }
}

// 프로그램 종료 여부 판단 함수, 텍스트 파일에 있던 프로세스들 모두 수행이 완료되면 프로그램 종료(return 0), 아니면 계속해서 프로그램 재재(return 1)
int end_processing(struct Queue* _ready_queue, int _tick) {
    if (CPU != NULL) {
        if (_ready_queue == NULL && CPU->burst == 0) { // 프로세스가 다 끝났으면 프로세스 끝난 현재 시간(tick)저장 후 프로그램 종료
            struct Queue* location = search(process_list, CPU);
            if(location->finish == -1) // 중복저장 방지
                location->finish = _tick;

            struct Queue* location2 = process_list;
            while (location2 != NULL) {
                if (location2->finish == -1) { // CPU에 할당된 프로세스가 다 끝났고 ready_queue에도 프로세스가 없으면 모든 프로세스가 종료됐다고 생각할지 모르지만 텍스트 파일에 있는 모든 프로세스가 다 끝난게 아니라면 프로그램 계속해서 수행
                    return 1;
                }
                else {
                    location2 = location2->next;
                }
            }
            printf("[tick: %02d] All processes are terminated.\n\n", _tick);
            return 0; // 프로그램 종료
        }
    } 
    return 1;
}

// process_list, ready_queue, CPU에 동적할당한 것들 해재해주는 함수
void free_var() {
    struct Queue* location = process_list; // process_list 동적 할당 해제
    struct Queue* pre;
    do {
        pre = location;
        if (location != NULL)
            location = location->next;
        if (pre != NULL)
            free(pre);
    } while (location != NULL);

    location = ready_queue; // ready_queue 동적 할당 해제
    do {
        pre = location;
        if (location != NULL)
            location = location->next;
        if (pre != NULL)
            free(pre);
    } while (location != NULL);

    location = CPU; // CPU 동적 할당 해제
    do {
        pre = location;
        if (location != NULL)
            location = location->next;
        if (pre != NULL)
            free(pre);
    } while (location != NULL);
}

// string.h에 있는 strtok 함수 구현, str을 delimiters 기준으로 모두 잘라낸다.
char* my_strtok(char* str, const char* delimiters) {
    static char* pCurrent;
    char* pDelimit;

    if (str != NULL)pCurrent = str;
    else str = pCurrent;

    if (*pCurrent == NULL) return NULL; // str이 NULL이면 NULL반환

    //문자열 점검
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
    // 더이상 자를 수 없다면 NULL반환
    return str;
}
