#ifndef _RKNN_DEMO_TIMER_H
#define _RKNN_DEMO_TIMER_H

#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TIME_COLLECT_LIMIT 10000

/*
    Custom time step
*/
enum TIME_STEP{
    ENCODER_PRE_PROCESS =0,
    ENCODER_RUN,
    ENCODER_POST_PROCESS,
    DECODER_PRE_PROCESS,
    DECODER_RUN,
    DECODER_POST_PROCESS,

    _STEP_HOLDER,
};


class TIMER_COLLECT{
    private:
        float times[_STEP_HOLDER][TIME_COLLECT_LIMIT];
        int time_num[_STEP_HOLDER];

    public:
        TIMER_COLLECT(){
            for (int i=0; i<_STEP_HOLDER; i++){
                memset(times[i], 0, sizeof(times[i]));
            }
            memset(time_num, 0, sizeof(time_num));
        }
        ~TIMER_COLLECT(){}

        void record_time(TIME_STEP step, float time){
            times[step][time_num[step]] = time;
            time_num[step]++;
        }

        int get_time_num(TIME_STEP step){
            return time_num[step]-1;
        }

        float get_time(TIME_STEP step, int index){
            return times[step][index];
        }

};


class TIMER{
    private:
        struct timeval start_time, stop_time;
        double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }
        char indent[40];

    public:
        TIMER(){}
        ~TIMER(){}

        TIMER_COLLECT tc = TIMER_COLLECT();
        
        void indent_set(char* s){
            strcpy(indent, s);
        }
        void indent_set(const char* s){
            strcpy(indent, s);
        }

        void tik(){
            gettimeofday(&start_time, NULL);
        }

        void tok(){
            gettimeofday(&stop_time, NULL);
        }

        void t_reset(){
            memset(&start_time, 0, sizeof(start_time));
            memset(&stop_time, 0, sizeof(stop_time));
        }

        void print_time(char* str){
            printf("%s", indent);
            printf("%s use: %f ms\n", str, get_time());
        }
        void print_time(const char* str){
            printf("%s", indent);
            printf("%s use: %f ms\n", str, get_time());
        }

        float get_time(){
            return (__get_us(stop_time) - __get_us(start_time))/1000;
        }

        void record_time(TIME_STEP step){
            tc.record_time(step, get_time());
        }

        void record_time(TIME_STEP step, float time){
            tc.record_time(step, time);
        }

        void dump_time_to_file(const char* file_name, TIME_STEP step){
            FILE* fp = fopen(file_name, "w");
            if (fp == NULL){
                printf("open file %s failed\n", file_name);
                return;
            }
            for (int i=0; i<tc.get_time_num(step); i++){
                fprintf(fp, "%f\n", tc.get_time(step, i));
            }
            fclose(fp);
        }
};

#endif // _RKNN_DEMO_TIMER_H