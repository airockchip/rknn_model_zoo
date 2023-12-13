#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

class TIMER{
    private:
        struct timeval start_time, stop_time;
        double __get_us(struct timeval t) { return (t.tv_sec * 1000000 + t.tv_usec); }
        char indent[40];

    public:
        TIMER(){}
        ~TIMER(){}

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
};