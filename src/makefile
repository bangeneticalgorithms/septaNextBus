CC=g++
CFLAGS=-c -g -std=c++0x
LDFLAGS=-lcurl -ljsoncpp -lmysqlclient #-lfann
INCLUDES=
CPPFILES = main.cpp functions.cpp arguments.cpp objects.cpp mysql_db.cpp trainer_base.cpp trainer_octave.cpp trainer_linear.cpp trainer_linear_sigmoid.cpp trainer_fann.cpp gtfs.cpp weather.cpp settings.cpp prediction_cache.cpp google_directions.cpp performance_recorder.cpp
EXENAME = next_bus
OFILES = $(CPPFILES:.cpp=.o)
DATE = `date +%m-%d-%y`


default: $(OFILES) 
	$(CC) -o $(EXENAME) $(OFILES) $(LDFLAGS)
	
prototype:
	g++ -o next_bus_proto -std=c++0x -g prototype.cpp -lcurl -lsqlite3 -ljsoncpp
	
clean:
	rm -f $(EXENAME) $(OFILES)

#cpp files -----------------------------------

main.o: main.cpp functions.h arguments.h objects.h mysql_db.h gtfs.h settings.h prediction_cache.h google_directions.h
	$(CC) $(CFLAGS) $(INCLUDES) main.cpp
	
functions.o: functions.cpp functions.h objects.h weather.h mysql_db.h trainer_base.h trainer_octave.h trainer_fann.h trainer_linear.h google_directions.h performance_recorder.h
	$(CC) $(CFLAGS) $(INCLUDES) functions.cpp
	
gtfs.o: gtfs.cpp gtfs.h mysql_db.h functions.h
	$(CC) $(CFLAGS) $(INCLUDES) gtfs.cpp
	
weather.o: weather.cpp weather.h mysql_db.h functions.h objects.h settings.h
	$(CC) $(CFLAGS) $(INCLUDES) weather.cpp
	
google_directions.o: google_directions.cpp google_directions.h functions.h objects.h mysql_db.h settings.h
	$(CC) $(CFLAGS) $(INCLUDES) google_directions.cpp
	
settings.o: settings.cpp settings.h
	$(CC) $(CFLAGS) $(INCLUDES) settings.cpp
	
arguments.o: arguments.cpp arguments.h
	$(CC) $(CFLAGS) $(INCLUDES) arguments.cpp

objects.o: objects.cpp objects.h
	$(CC) $(CFLAGS) $(INCLUDES) objects.cpp
	
mysql_db.o: mysql_db.cpp mysql_db.h objects.h settings.h functions.h
	$(CC) $(CFLAGS) $(INCLUDES) mysql_db.cpp
	
prediction_cache.o: prediction_cache.cpp prediction_cache.h objects.h functions.h mysql_db.h google_directions.h
	$(CC) $(CFLAGS) $(INCLUDES) prediction_cache.cpp
	
performance_recorder.o: performance_recorder.cpp performance_recorder.h mysql_db.h objects.h functions.h
	$(CC) $(CFLAGS) $(INCLUDES) performance_recorder.cpp

trainer_base.o: trainer_base.cpp trainer_base.h functions.h
	$(CC) $(CFLAGS) $(INCLUDES) trainer_base.cpp
	
trainer_octave.o: trainer_octave.cpp trainer_octave.h trainer_base.h
	$(CC) $(CFLAGS) $(INCLUDES) trainer_octave.cpp
	
trainer_linear.o: trainer_linear.cpp trainer_linear.h trainer_base.h functions.h
	$(CC) $(CFLAGS) $(INCLUDES) trainer_linear.cpp
	
trainer_linear_sigmoid.o: trainer_linear_sigmoid.cpp trainer_linear_sigmoid.h trainer_linear.h trainer_base.h
	$(CC) $(CFLAGS) $(INCLUDES) trainer_linear_sigmoid.cpp
	
trainer_fann.o: trainer_fann.cpp trainer_fann.h trainer_base.h
	$(CC) $(CFLAGS) $(INCLUDES) trainer_fann.cpp
