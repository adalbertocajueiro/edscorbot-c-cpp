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

/**
 * The broker host. There is an instance of Mosquitto running at 192.168.1.104
 * For the simulated server we suggest to use a local instance of Mosquitto
 *
 * TODO: Adjust the value to point to your mosquitto host
 **/
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

/**
 * A global variable maintaining the current point to move the arm
 * This point is initially empty and its value is updated when the user
 * requests via ARM_MOVE_TO_POINT signal. After the robotc arm is moved
 * its value is reset to an empty point again.
**/
Point current_point;

/**
 * A global variable maintaining the current trajectory to be executed
 * This trajectory is initially empty and its value is updated when the user
 * requests via ARM_APPLY_TRAJECTORY signal. After the trajectory is applied 
 * its value is reset to and empty trajectory again.
 **/
Trajectory current_trajectory;

/**
 * Global flag informing that a trajectory is being executed or not 
**/
bool executing_trajectory = false;


/**
 * Function to move the arm to home position. It must be executed into a 
 * thread to avoid blocking the main process and disconnect from the broker.
 * Before calling this function all pre-conditions have already been validated
*/
void* search_home_threaded_function(void* arg){
	//the answer
	CommandObject output = CommandObject(ARM_HOME_SEARCHED);
	output.client = owner;
	output.error = error_state;

	/**
	 * A sleep time representing the action of moving the arm to home position
	 * 
	 * TODO: Replace this code with the real procedure to move the arm to home
	 **/
	usleep(4000000);
	
	//publish message notifying that home has been reached
	publish_message(COMMANDS_TOPIC,output.to_json().dump().c_str());
	std::cout << "Home position reached" << std::endl;
	return NULL;
}

/**
 * Function to move the arm to a single point. It must be executed into a
 * thread to avoid blocking the main process and disconnect from the broker.
 * Before calling this function all pre-conditions have already been validated
 */
void* move_to_point_threaded_function(void* arg){
	//the answer
	MovedObject output = MovedObject();
	output.client = owner;
	output.error = error_state;

	/**
	 * A sleep time representing the action of moving the arm to home position
	 *
	 * TODO: Replace this code with the real procedure to move the arm to a point
	 * The values for each joint are stored in the global variable "current_point"
	 * (current_point.coordinates)
	**/
	usleep(2000000);

	/**
	 * TODO: After moving the arm, get the counter value for each joint and convert
	 * them to angles. Then use the variable realPoint bellow to assign these
	 * values (realPoint.coordinates[index] = count_to_angle(value)). Check if the
	 * number of coordinates  matched the number of joints in the global variable
	 * METAINFOS
	 *
	 * For the moment we simply publish the the current_point
	 **/
	Point realPoint = current_point;

	//sets the content of the answer
	output.content = realPoint;
	
	//publish message notifying that the point has been published
	publish_message(MOVED_TOPIC,output.to_json().dump().c_str());
	std::cout << "Arm moved to point " << output.content.to_json().dump().c_str() << std::endl;

	//after publishing the message, the current_point must be re-instantiated with empty point
	current_point = Point();

	return NULL;
}



//the function below is intented to be used
/**
 * Function to move the arm to a single point to be used in trajectory execution because the execution of
 * each point of a trajectory cannot be threaded (different points would be executed concurrently,
 * causing a terrible side-effect)
 */
void move_to_point(Point point){
	//the answer to communicate each point
	MovedObject output = MovedObject();
	output.client = owner;
	output.error = error_state;

	/**
	 * A sleep time representing the action of moving the arm to home position
	 *
	 * TODO: Replace this code with the real procedure to move the arm to a point
	 * The values for each joint are stored in the global variable "current_point"
	 * (current_point.coordinates)
	 **/
	usleep(2000000);

	/**
	 * TODO: After moving the arm, get the counter value for each joint and convert
	 * them to angles. Then use the variable realPoint bellow to assign these
	 * values (realPoint.coordinates[index] = count_to_angle(value)). Check if the
	 * number of coordinates  matched the number of joints in the global variable
	 * METAINFOS
	 *
	 * For the moment we simply publish the same point we have received
	 **/
	Point realPoint = point;

	// sets the content of the answer
	output.content = realPoint;

	// publish message notifying that the point has been published
	publish_message(MOVED_TOPIC, output.to_json().dump().c_str());
	std::cout << "Arm moved to point " << output.content.to_json().dump().c_str() << std::endl;
	
	return;
}

/**
 * Function to aply a trajectory. It must be executed into a
 * thread to avoid blocking the main process and disconnect from the broker.
 * Before calling this function all pre-conditions have already been validated
 */
void* apply_trajectory_threaded_function(void* arg){
	//points to be considered come from the global variable "current_trajectory"
	std::list<Point> points = std::list<Point>(current_trajectory.points);

	//trajectory execution has been started
	executing_trajectory = true;

	while (!points.empty() && executing_trajectory){
        Point p = points.front();
		
		move_to_point(p); 

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

/**
 * Function that registers the server into the broker using the suitable topics 
**/
void subscribe_all_topics()
{
	//it builds the topic names (META_INFO,COMMANDS_TOPIC,MOVED_TOPIC)
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

/**
 * Fuction that publishes a message (the string representation of a JSON object)
 * using a specific topic
 **/
int publish_message(std::string topic, const char *buf)
{
    char *payload = (char *)buf;
	int rc = mosquitto_publish(mosq, NULL, topic.c_str(), strlen(payload), payload, 1, false);
	return rc;
}

/**
 * Function that handles messages delivered on channel metainfo
 **/
void handle_metainfo_message(std::string mesage)
{
	MetaInfoObject mi = initial_metainfoobj();
	publish_message(META_INFO,mi.to_json().dump().c_str());
}

/**
 * Function that handles messages delivered on channel ROBOT_NAME/commands
 **/
void handle_commands_message(const struct mosquitto_message *message){
	//obtains the signal/code
	int sig = extract_signal((char *)message->payload);

	//the answer
	CommandObject output = CommandObject(ARM_STATUS);

	//parses the received command from the payload
	CommandObject receivedCommand = CommandObject::from_json_string((char *)message->payload);
	switch (sig)
	{
	case ARM_CHECK_STATUS: //user requested arm status
		std::cout << "Request status received. "
				  << " Sending payload " 
				  << output.to_json().dump().c_str() << std::endl;
		publish_message("EDScorbot/commands", output.to_json().dump().c_str());
		break;
	case ARM_CONNECT: //user wants to connect to the arm to become the owner
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
	case ARM_MOVE_TO_POINT: //user requested to move the arm to a single point
		std::cout << "Move to point request received. " << std::endl;

		if (owner.is_valid())
		{
			Client client = receivedCommand.client;
			if (owner == client) // only owner can do that
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
	case ARM_APPLY_TRAJECTORY:  //user requested to apply a trajectory
		std::cout << "Apply trajectory received. " << std::endl;
		if (owner.is_valid())
		{
			Client client = receivedCommand.client;
			if (owner == client) //only owner can do that
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
	case ARM_CANCEL_TRAJECTORY: //user requested to cancel trajectory execution
		std::cout << "Cancel trajectory received. " << std::endl;
		if (owner.is_valid())
		{
			Client client = receivedCommand.client;
			if (owner == client) // only owner can do that
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
	case ARM_DISCONNECT: //user requested to disconnect from the arm
		
		Client c = receivedCommand.client;
		std::cout << "Request disconnect received. " 
			<< receivedCommand.to_json().dump().c_str()
			<< std::endl;
		if (c == owner) // only owner can do that
		{
			owner = Client();
			output.signal = ARM_DISCONNECTED;
			output.client = c;

			publish_message(COMMANDS_TOPIC, output.to_json().dump().c_str());
			std::cout 	<< "Client disconnected " 
						<< output.to_json().dump().c_str()
						<< std::endl;
		}

		break;
		// incluir default
		// default:
	}
}


/**
 * The callback function invoked by mosquitto when notifying all applications subscribed
 * on specific topics  
**/
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{

	/**
	 * If the message contains a signal then it has been sent from some application
	 * the follows the async specification (the new model/flow). Otherwise, the server
	 * must handle the message as before. 
	**/
	bool new_flow = has_signal((char *) message->payload);
	if(new_flow){
		//this is the logic for processing messages in the new model
		bool match = std::strcmp(message->topic,"metainfo") == 0;
		int sig = extract_signal((char *)message->payload);
		if(match){
			if(sig == ARM_GET_METAINFO){
				handle_metainfo_message((char *)message->payload);
			}

		} else {
			match = std::strcmp(message->topic,COMMANDS_TOPIC.c_str()) == 0;
			if (match){
				handle_commands_message(message);
			}
		}

	} else {

		/**
		 * TODO: This is the sapce to put all the old code where the robot
		 * received messages with a different structure of the async API 
		**/
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

		//subscribe on all relevant topics
        subscribe_all_topics();

		//publishes the metainfo of the robot
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