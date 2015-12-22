#ifndef ARGUMENTS_H
#define ARGUMENTS_H

#include <string>

using namespace std;

class Arguments
{
    public:
        Arguments() { clearArguments(); }

        void clearArguments();
        bool readArguments(int argc, char *argv[]);
        void printHelp();
        void printMiniHelp();
        void printSetup();

        string exe_command;
        string route_id;
        bool route_all;
        int stop_id;
        bool stop_all;
        bool read_bus_data;
        bool read_stop_data;
        bool create_coeff_data;
        bool print_help;
        bool print_mini_help;
        bool reload_gtfs;
        bool optimize_front_end;
        int iterations;
        int retrain_time;
};

#endif
