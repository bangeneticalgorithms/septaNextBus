#include "arguments.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

void Arguments::clearArguments()
{
    exe_command = "";
    stop_id = 0;
    stop_all = true;
    route_id = "";
    route_all = true;
    read_bus_data = false;
    read_stop_data = false;
    create_coeff_data = false;
    print_help = false;
    print_mini_help = false;
    reload_gtfs = false;
    optimize_front_end = false;
    iterations = 1000;
    retrain_time = 0;
}

bool Arguments::readArguments(int argc, char **argv)
{
    if(argc) exe_command = argv[0];
    if(argc<=1) print_mini_help = true;

    for(int i=1;i<argc;i++)
    {
        if(!strcmp(argv[i], "--collect-bus-data") || !strcmp(argv[i], "-b"))
            read_bus_data = true;
        else if(!strcmp(argv[i], "--collect-stop-data") || !strcmp(argv[i], "-e"))
            read_stop_data = true;
        else if(!strcmp(argv[i], "--create-coeff-data") || !strcmp(argv[i], "-c"))
            create_coeff_data = true;
        else if(!strcmp(argv[i], "--reload-gtfs"))
            reload_gtfs = true;
        else if(!strcmp(argv[i], "--optimize-front-end") || !strcmp(argv[i], "-o"))
            optimize_front_end = true;
        else if(!strcmp(argv[i], "--help") || !strcmp(argv[i], "-h") || !strcmp(argv[i], "?"))
            print_help = true;
        else if(!strcmp(argv[i], "-i") && i+1<argc)
            iterations = atoi(argv[++i]);
        else if(!strcmp(argv[i], "-t") && i+1<argc)
            retrain_time = atoi(argv[++i]);
        else if(!strcmp(argv[i], "-td") && i+1<argc)
            retrain_time = 24 * 60 * 60 * atoi(argv[++i]);
        else if(!strcmp(argv[i], "-th") && i+1<argc)
            retrain_time = 60 * 60 * atoi(argv[++i]);
        else if(!strcmp(argv[i], "-r") && i+1<argc)
        {
            route_id = argv[++i];
            route_all = false;
        }
        else if(!strcmp(argv[i], "-s") && i+1<argc)
        {
            stop_id = atoi(argv[++i]);
            stop_all = false;
        }
    }
}

void Arguments::printHelp()
{
    printf("Septa Next Bus (http://septanextbus.sf.net/)\n");
    printf("     --collect-bus-data or -b         : collect bus data (can be route specific)\n");
    printf("     --collect-stop-data or -e        : collect stop data (can be stop specific, not used anymore)\n");
    printf("     --create-coeff-data or -c        : create coefficient data (can be stop and route specific)\n");
    printf("     --reload-gtfs                    : reload gtfs databases from gtfs_public/\n");
    printf("     --optimize-front-end or -o       : keep recent bus / weather data available for the front end\n");
    printf("     -r route_id                      : set a specific route id (if not set default is all)\n");
    printf("     -s stop_id                       : set a specific stop id (if not set default is all)\n");
    printf("     -i iterations                    : set number of times the learning algorithm should iterate)\n");
    printf("     -t retrain_time                  : only retrain if time since last train is greater than retrain_time)\n");
    printf("     -td retrain_time_days            : same as -t but the value is in days instead of seconds)\n");
    printf("     -th retrain_time_hours           : same as -t but the value is in hours instead of seconds)\n");
    printf("\n");
}

void Arguments::printMiniHelp()
{
    printf("Septa Next Bus (http://septanextbus.sf.net/)\n");
    printf("For more information rerun with --help (ie: %s --help)\n", exe_command.c_str());
}

void Arguments::printSetup()
{
    printf("using commands: ");
    if(read_bus_data) printf("--collect-bus-data ");
    if(read_stop_data) printf("--collect-stop-data ");
    if(create_coeff_data) printf("--create-coeff-data ");
    if(reload_gtfs) printf("--reload-gtfs ");
    if(optimize_front_end) printf("--optimize-front-end ");
    if(route_all) printf("-r ALL ");
    else printf("-r '%s' ", route_id.c_str());
    if(stop_all) printf("-s ALL ");
    else printf("-s %d ", stop_id);
    printf("-i %d ", iterations);
    if(retrain_time > 0) printf("-t %d ", retrain_time);
    printf("\n");
}
