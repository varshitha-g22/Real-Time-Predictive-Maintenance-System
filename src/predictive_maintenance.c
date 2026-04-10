#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h> // used for setting thread scheduling priority
#include <sys/neutrino.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>

// SIMPLE LOGGING MACROS (for printing system messages)

#define LOG_LEVEL_INFO  "INFO"
#define LOG_LEVEL_CRIT  "CRIT"
#define SYS_LOG(level, module, msg) printf("[%s] [%s]: " msg "\n", level, module)


// STATES USED BY ACTUATOR (basic FSM)

typedef enum {
    FSM_IDLE,
    FSM_ENGAGING,
    FSM_ACTIVE
} ActuatorState;


// THREAD PRIORITY VALUES
// higher number means higher priority in QNX scheduler

#define PRIO_SERVER      50
#define PRIO_AI          45
#define PRIO_ACTUATOR    40
#define PRIO_SENSOR      30
#define PRIO_DASHBOARD   20


// MESSAGE TYPES AND DATA STRUCTURES

#define MSG_TEMP    0
#define MSG_VOLT    1
#define MSG_AI      2
#define MSG_ACT     3
#define MSG_UI      4
#define MSG_EXIT    5

typedef struct {
    uint16_t msg_type;
    float float_val;
    bool bool_flag;
    ActuatorState fsm_state;
} Msg;

typedef struct {
    float current_temp;
    float current_volt;
    float ai_threshold;
    float fluid_density;
    bool alarm_active;
    ActuatorState cooling_fsm;
    bool system_shutdown;
} Reply;

int server_chid;


// HELPER FOR CREATING THREADS WITH PRIORITY

int spawn_priority_thread(pthread_t *tid, void *(*func)(void *), int prio) {
    pthread_attr_t attr;
    struct sched_param param;

    pthread_attr_init(&attr);
    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);
    pthread_attr_setschedpolicy(&attr, SCHED_RR);

    param.sched_priority = prio;
    pthread_attr_setschedparam(&attr, &param);

    int ret = pthread_create(tid, &attr, func, NULL);
    pthread_attr_destroy(&attr);
    return ret;
}


// DISPATCH FUNCTIONS

void handle_temp(Msg* msg, Reply* sys) {
    sys->current_temp = msg->float_val;
    sys->fluid_density = 1000.0 - (sys->current_temp * 0.45);
}
void handle_volt(Msg* msg, Reply* sys) {
    sys->current_volt = msg->float_val;
}
void handle_ai(Msg* msg, Reply* sys) {
    sys->ai_threshold = msg->float_val;
    sys->alarm_active = msg->bool_flag;
}
void handle_actuator(Msg* msg, Reply* sys) {
    sys->cooling_fsm = msg->fsm_state;
}
void handle_dashboard(Msg* msg, Reply* sys) {
}
void handle_shutdown(Msg* msg, Reply* sys) {
    sys->system_shutdown = true;
}

typedef void (*HandlerFn)(Msg*, Reply*);
HandlerFn dispatcher[6] = {
    handle_temp, handle_volt, handle_ai, handle_actuator, handle_dashboard, handle_shutdown
};


// SERVER THREAD

void* server_process(void* arg) {
    server_chid = ChannelCreate(0);
    if (server_chid == -1) { perror("Channel create failed"); exit(EXIT_FAILURE); }

    int rcvid;
    Msg msg;
    Reply sys = {
        .current_temp = 30.0, .current_volt = 220.0,
        .ai_threshold = 50.0, .fluid_density = 1000.0,
        .alarm_active = false, .cooling_fsm = FSM_IDLE,
        .system_shutdown = false
    };

    SYS_LOG(LOG_LEVEL_INFO, "CORE", "Dispatcher Framework Online. Priority 50 Set.");

    while (!sys.system_shutdown) {
        rcvid = MsgReceive(server_chid, &msg, sizeof(msg), NULL);
        if (rcvid == -1) break;

        if (msg.msg_type <= MSG_EXIT) {
            dispatcher[msg.msg_type](&msg, &sys);
        }

        MsgReply(rcvid, 0, &sys, sizeof(sys));
    }
    return NULL;
}


// CLIENT 1

void* client_thermal(void* arg) {
    usleep(100000);
    int coid = ConnectAttach(0, 0, server_chid, _NTO_SIDE_CHANNEL, 0);

    Msg msg = { .msg_type = MSG_TEMP };
    Reply reply;
    float temp = 30.0;
    srand(time(NULL));

    while (1) {
        msg.float_val = temp;
        MsgSend(coid, &msg, sizeof(msg), &reply, sizeof(reply));
        if (reply.system_shutdown) break;

        if (reply.cooling_fsm == FSM_ACTIVE) {
            temp -= 3.0;
            if (temp < 30.0) temp = 30.0;
        } else {
            temp += ((float)rand() / RAND_MAX) * 1.8;
        }
        usleep(300000);
    }
    ConnectDetach(coid); return NULL;
}


// CLIENT 2

void* client_ai(void* arg) {
    usleep(100000);
    int coid = ConnectAttach(0, 0, server_chid, _NTO_SIDE_CHANNEL, 0);

    Msg msg = { .msg_type = MSG_AI };
    Reply reply;

    while (1) {
        MsgSend(coid, &msg, sizeof(msg), &reply, sizeof(reply));
        if (reply.system_shutdown) break;

        float t = reply.current_temp;
        float thresh = reply.ai_threshold;
        bool alarm = reply.alarm_active;

        if (reply.cooling_fsm == FSM_IDLE && t < thresh) {
            thresh = (thresh * 0.95) + ((t + 10.0) * 0.05);
        }

        if (t > thresh && !alarm) alarm = true;
        else if (t < (thresh - 3.0) && alarm) alarm = false;

        msg.float_val = thresh;
        msg.bool_flag = alarm;
        usleep(400000);
    }
    ConnectDetach(coid); return NULL;
}


// CLIENT 3

void* client_actuator(void* arg) {
    usleep(100000);
    int coid = ConnectAttach(0, 0, server_chid, _NTO_SIDE_CHANNEL, 0);

    ActuatorState my_state = FSM_IDLE;
    Msg msg = { .msg_type = MSG_ACT, .fsm_state = my_state };
    Reply reply;

    while (1) {
        MsgSend(coid, &msg, sizeof(msg), &reply, sizeof(reply));
        if (reply.system_shutdown) break;

        switch(my_state) {
            case FSM_IDLE:
                if (reply.alarm_active) my_state = FSM_ENGAGING;
                break;
            case FSM_ENGAGING:
                usleep(200000);
                my_state = FSM_ACTIVE;
                break;
            case FSM_ACTIVE:
                if (!reply.alarm_active) my_state = FSM_IDLE;
                break;
        }

        msg.fsm_state = my_state;
        usleep(200000);
    }
    ConnectDetach(coid); return NULL;
}


// CLIENT 4

void* client_dashboard(void* arg) {
    usleep(200000);
    int coid = ConnectAttach(0, 0, server_chid, _NTO_SIDE_CHANNEL, 0);
    Msg msg = { .msg_type = MSG_UI };
    Reply reply;
    int cycles = 0;

    while (1) {
        MsgSend(coid, &msg, sizeof(msg), &reply, sizeof(reply));
        if (reply.system_shutdown) break;

        if (cycles % 5 == 0) {
            printf("\n|   TEMP   | AI THRESHOLD | VOLTAGE | DENSITY | ALARM | COOLING SYSTEM |\n");
            printf("|----------|--------------|---------|---------|-------|----------------|\n");
        }

        char* cooling_str;
        if (reply.cooling_fsm == FSM_IDLE) cooling_str = "  IDLE  ";
        else if (reply.cooling_fsm == FSM_ENGAGING) cooling_str = "ENGAGING";
        else cooling_str = " ACTIVE ";

        printf("| %05.2f°C |    %05.2f°C  | %06.2fV | %06.2f  |  %s  |    %s    |\n",
               reply.current_temp, reply.ai_threshold, reply.current_volt, reply.fluid_density,
               reply.alarm_active ? "!!! " : " OK ", cooling_str);

        cycles++;
        sleep(1);
    }
    ConnectDetach(coid); return NULL;
}


// MAIN

int main() {
    SYS_LOG(LOG_LEVEL_INFO, "SYS", "Initializing QNX Framework-Driven RTOS...");
    SYS_LOG(LOG_LEVEL_INFO, "SYS", "Enforcing SCHED_RR Real-Time Priority Policies...");

    pthread_t t_server, t_sens, t_ai, t_act, t_dash;

    spawn_priority_thread(&t_server, server_process,   PRIO_SERVER);
    spawn_priority_thread(&t_sens,   client_thermal,   PRIO_SENSOR);
    spawn_priority_thread(&t_ai,     client_ai,        PRIO_AI);
    spawn_priority_thread(&t_act,    client_actuator,  PRIO_ACTUATOR);
    spawn_priority_thread(&t_dash,   client_dashboard, PRIO_DASHBOARD);

    printf("*************************************************************\n");
    printf("* SYSTEM ONLINE. Type 'q' and press ENTER to safely stop.   *\n");
    printf("*************************************************************\n");

    while (1) {
        char command = getchar();
        if (command == 'q' || command == 'Q') {
            SYS_LOG(LOG_LEVEL_CRIT, "CTRL", "Shutdown command received. Dispatching teardown...");

            int coid = ConnectAttach(0, 0, server_chid, _NTO_SIDE_CHANNEL, 0);
            Msg kill_msg = { .msg_type = MSG_EXIT };
            Reply ignore;
            MsgSend(coid, &kill_msg, sizeof(kill_msg), &ignore, sizeof(ignore));
            ConnectDetach(coid);
            break;
        }
    }

    pthread_join(t_dash, NULL);
    pthread_join(t_sens, NULL);
    pthread_join(t_ai, NULL);
    pthread_join(t_act, NULL);
    pthread_join(t_server, NULL);

    SYS_LOG(LOG_LEVEL_INFO, "CORE", "All modules cleanly detached. Goodbye.");
    return 0;
}
