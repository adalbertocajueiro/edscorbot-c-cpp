#include <signal.h>
#include <iostream>
#include <string>
#include <thread>
#include <unistd.h>
#include "../impl/server-impls.cpp"
#include "mosquitto.h"
#include "include/devmem.hpp"
#include <pthread.h>


#define DEFAULT_SLEEP 125000 //microseconds
// the server with all implementations
//#define mqtt_host "192.168.1.104"
#define mqtt_host "localhost"
#define mqtt_port 1883

static int run = 1;
struct mosquitto *mosq;
pthread_t search_home_thread;
pthread_t move_to_point_thread;
pthread_t apply_trajectory_thread;

typedef struct
{
	int type;
	char mode;
	char *payload;
	int last;
} progress_info;

progress_info progress;

//variable storing the cnext point to move the arm
Point current_point;

bool executing_trajectory = false;


/**
 * The function to send arm to home. This must be executed into a thread
*/
void* search_home_threaded_function(void* arg){
	//to execute search home we need the suitable signal, the owner and the error state
	// the owner has already been validated to allow this execution in the callback
	CommandObject output = CommandObject(ARM_HOME_SEARCHED);
	output.client = owner;
	output.error = error_state; 

	usleep(4000000);
	
	//publish message notifying that home has been reached
	publish_message(COMMANDS_TOPIC,output.to_json().dump().c_str());
	std::cout << "Home position reached" << std::endl;
	return NULL;
}

void* move_to_point_threaded_function(void* arg){
	//to move to a single point we need the owner, the error state and the point
	// the owner has already been validated to allow this execution in the callback
	MovedObject output = MovedObject();
	output.client = owner;
	output.error = error_state; 	

	//call the low level function to move to a single point considering 
	//the coordinates contain angles in degrees and are previously stored in 
	//current_point (current_point.coordinates) 

	//this command is representing the arm is moving to a point. replate it with your code
	usleep(2000000);

	//after movind get the counters from the arm and convert them into angles in degrees
	//put them into realPoint variable 
	//it is important to fill the coordinates with the number of joints
	//informed in the metainfo.
	// for the moment we publish the same point we have received
	Point realPoint = current_point;

	//for the moment we use current_point to notify the user
	output.content = realPoint;
	
	//publish message notifying that the point has been published
	publish_message(MOVED_TOPIC,output.to_json().dump().c_str());
	std::cout << "Arm moved to point " << output.content.to_json().dump().c_str() << std::endl;

	//after publishing the message, the current_point must be re-instantiated with empty point
	current_point = Point();

	return NULL;
}



//the function below is intented to be used in trajectory execution because the execution of 
//alll points of a trajectory cannot be threaded (different points would be executed concurrently,
//causing a terrible side-effect)
void move_to_point(Point point){
	MovedObject output = MovedObject();
	output.client = owner;
	output.error = error_state; 

	//call the low level function to move to a single point considering 
	//the coordinates contain angles in degrees and are previously stored in 
	//current_point (current_point.coordinates) 

	//this command is representing the arm is moving to a point. replate it with your code
	usleep(2000000);

	//after movind get the counters from the arm and convert them into angles in degrees
	//put them into realPoint variable 
	//it is important to fill the coordinates with the number of joints
	//informed in the metainfo.
	// for the moment we publish the same point we have received
	Point realPoint = point;

	//for the moment we use current_point to notify the user
	output.content = realPoint;
	
	//publish message notifying that the point has been published
	publish_message(MOVED_TOPIC,output.to_json().dump().c_str());
	std::cout << "Arm moved to point " << output.content.to_json().dump().c_str() << std::endl;

	//after publishing the message, the current_point must be re-instantiated with empty point
	current_point = Point();
	
	return;
}

//the current trajectory to be executed
Trajectory current_trajectory;

void* apply_trajectory_threaded_function(void* arg){
	//to apply a trajectory we need the owner, the error state and the trajectory
	// the owner has already been validated to allow this execution in the callback
	//the current trajectory is maintained in a global variable current_trajectory
	//that is updated befoe starting this thread
	
	//points to be considered
	std::list<Point> points = std::list<Point>(current_trajectory.points);

	//trajectory execution has been started
	executing_trajectory = true;

	while (!points.empty() && executing_trajectory){
        Point p = points.front();
		move_to_point(p); 
		//we need to handle the waiting time after start oving to a point.
		//this time is the last coordinate of the point parameter
		//usleep(p.coordinates.end()) // something like this ??????

		points.erase(points.begin());
	}

	//trajectory execution has finished
	executing_trajectory = false;

	//clean the current trajectory variable
	current_trajectory = Trajectory();

	return NULL;
}

void handle_signal(int s)
{
	run = 0;
}

//subscribes the robot in all relevant topics
//ADJUST THE NAME OF YOUR ROBOT IN THE PREFIX OF THE TOPICS!!!!
void subscribe_all_topics()
{
	build_topics();

	std::cout << "Subscribing on topic "
			  << META_INFO
			  << std::endl;
	
	mosquitto_subscribe(mosq, NULL, META_INFO.c_str(), 0);

	std::cout << "Subscribing on topic "
			  << COMMANDS_TOPIC.c_str()
			  << std::endl;

	mosquitto_subscribe(mosq, NULL, COMMANDS_TOPIC.c_str(), 0);
}

int publish_message(std::string topic, const char *buf)
{
    char *payload = (char *)buf;
	int rc = mosquitto_publish(mosq, NULL, topic.c_str(), strlen(payload), payload, 1, false);
	return rc;
}

//handles messages on channel metainfo
void handle_metainfo_message(std::string mesage){
	MetaInfoObject mi = initial_metainfoobj();
	publish_message(META_INFO,mi.to_json().dump().c_str());
}

//handles messages on channel ROBOTNAME/commands
//ADJUST THE ROBOT NAME IN YOUR IMPLEMENTATION
void handle_commands_message(const struct mosquitto_message *message){
	//extraces the signal from a message on channel ROBOTNAME/commands
	int sig = extract_signal((char *)message->payload);

	//creates a commandobject (a new output)
	CommandObject output = CommandObject(ARM_STATUS);

	//parses the received command from the payload
	CommandObject receivedCommand = CommandObject::from_json_string((char *)message->payload);
	switch (sig)
	{
	case ARM_CHECK_STATUS:
		std::cout << "Request status received. "
				  << " Sending payload " 
				  << output.to_json().dump().c_str() << std::endl;
		publish_message("EDScorbot/commands", output.to_json().dump().c_str());
		break;
	case ARM_CONNECT:
		std::cout << "Request to connect received." << std::endl;

		if (!owner.is_valid())
		{
			owner = receivedCommand.client;
			output.signal = ARM_CONNECTED;
			output.client = owner;
			std::cout 	<< "Arm's owner is " 
						<< owner.to_json().dump().c_str()
						<< std::endl;

			publish_message(COMMANDS_TOPIC, output.to_json().dump().c_str());
			
			std::cout << "Moving arm to home..." << std::endl;

			int err = pthread_create(&search_home_thread, NULL, &search_home_threaded_function, NULL);
			pthread_detach(search_home_thread);

			// handle err?
		}
		else
		{
			std::cout << "Connection refused. Arm is busy" << std::endl;
		}

		break;
	case ARM_MOVE_TO_POINT:
		std::cout << "Move to point request received. " << std::endl;

		if (owner.is_valid())
		{
			Client client = receivedCommand.client;
			if (owner == client)
			{
				Point target = receivedCommand.point;
				if (!target.is_empty())
				{
					current_point = target;
					int err = pthread_create(&move_to_point_thread, NULL, &move_to_point_threaded_function, NULL);
					pthread_detach(move_to_point_thread);

					// handle err?
				}
			}
			else
			{
				// other client is trying to move the arm ==> ignore
			}
		}
		else
		{
			// arm has no owner ==> ignore
		}
		break;
	case ARM_APPLY_TRAJECTORY:
		std::cout << "Apply trajectory received. " << std::endl;
		if (owner.is_valid())
		{
			Client client = receivedCommand.client;
			if (owner == client)
			{
				current_trajectory = receivedCommand.trajectory;
				int err = pthread_create(&apply_trajectory_thread, NULL, &apply_trajectory_threaded_function, NULL);
				pthread_detach(apply_trajectory_thread);

				// handle err?
			}
			else
			{
				// other client is trying to move the arm ==> ignore
			}
		}
		else
		{
			// arm has no owner ==> ignore
		}
		break;
	case ARM_CANCEL_TRAJECTORY:
		std::cout << "Cancel trajectory received. " << std::endl;
		if (owner.is_valid())
		{
			Client client = receivedCommand.client;
			if (owner == client)
			{
				executing_trajectory = false;
				output.signal = ARM_CANCELED_TRAJECTORY;
				output.client = owner;
				output.error = error_state;
				publish_message(COMMANDS_TOPIC, output.to_json().dump().c_str());
				std::cout << "Trajectory cancelled. " << std::endl;
			}
		}
		else
		{
			// arm has no owner ==> ignore
		}
		break;
	case ARM_DISCONNECT:
		
		Client c = receivedCommand.client;
		std::cout << "Request disconnect received. " 
			<< receivedCommand.to_json().dump().c_str()
			<< std::endl;
		if (c == owner)
		{
			owner = Client();
			output.signal = ARM_DISCONNECTED;
			output.client = c;
			publish_message(COMMANDS_TOPIC, output.to_json().dump().c_str());
			std::cout 	<< "Client disconnected " 
						<< output.to_json().dump().c_str()
						<< " on channel "
						<< COMMANDS_TOPIC
						<< std::endl;
		}

		break;
		// incluir default
		// default:
	}
}


void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{

	//if the message contains a signal then it has came from a client (angular)
	//then follows the new communication model. otherwise, it does exactly as before
	bool new_flow = has_signal((char *) message->payload);
	if(new_flow){

		bool match = std::strcmp(message->topic,"metainfo") == 0;
		int sig = extract_signal((char *)message->payload);
		if(match){
			if(sig == ARM_GET_METAINFO){
				handle_metainfo_message((char *)message->payload);
			}

		} else {
			match = std::strcmp(message->topic,"EDScorbot/commands") == 0;
			if (match){
				handle_commands_message(message);
			}
		}

	} else {

		// TODO
		// do everything as before
	}

}


int main(int argc, char *argv[])
{

	uint8_t reconnect = true;
	char clientid[24];
	int rc = 0;

	signal(SIGINT, handle_signal);
	signal(SIGTERM, handle_signal);

	mosquitto_lib_init();

	memset(clientid, 0, 24);
	snprintf(clientid, 23, "simulated_server_%d", getpid());
	mosq = mosquitto_new(clientid, true, NULL);
	progress.last = 0;
	if (mosq)
	{
		mosquitto_message_callback_set(mosq, message_callback);
    
		rc = mosquitto_connect(mosq, mqtt_host, mqtt_port, 60);

        subscribe_all_topics();

		MetaInfoObject mi = initial_metainfoobj();
		publish_message(META_INFO,mi.to_json().dump().c_str());
		std::cout << "Metainfo published " << mi.to_json().dump().c_str() << std::endl;
		
		rc = mosquitto_loop_start(mosq);
		while (run)
		{
			
			if (run && rc)
			{
				printf("connection error!\n");
				sleep(1);
				mosquitto_reconnect(mosq);
			}
            else
            {
                //printf("server conected to mosquitto broker!\n");
            }
		}
		mosquitto_loop_stop(mosq,true);
		mosquitto_destroy(mosq);
	}

	mosquitto_lib_cleanup();

	return rc;
}