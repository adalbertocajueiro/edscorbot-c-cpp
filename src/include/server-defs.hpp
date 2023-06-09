#include <string>
#include <list>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

/**
 * The name of the controller
 *
 * TODO: Replace with the convenient value for your robot
 **/
const std::string ROBOT_NAME = "EDScorbotSim";

// channel names
const std::string META_INFO = "metainfo";
const std::string COMMANDS = "commands";
const std::string MOVED = "moved";

// commands and moved topics are built from controller name and channel names
std::string COMMANDS_TOPIC;
std::string MOVED_TOPIC;

/**
 * Function to convert angles into reference values
 * DOUBT: angles in DEGREES or RADIANS?
 **/
int angle_to_ref(int motor, float angle);

/**
 * Function to converto reference values into angles
 * DOUBT: angles in DEGREES or RADIANS?
 **/
double ref_to_angle(int motor, int ref);

/**
 * A global flag representing is the arm is in a error state or not.
 * This allows one to notify clients that an internal problem has
 * happened with the arm. The controller implementation should
 * consider mechanisms of detecting arm's errors to update this value,
 * as well as some efficient ways to recover from the failure
 **/
static bool error_state = false;

/**
 * The possible codes for a metainfo object
 **/
enum MetaInfoSignal
{
    ARM_GET_METAINFO = 1,
    ARM_METAINFO = 2
};

/**
 * The possible codes for a commands object
 **/
enum CommandsSignal
{
    ARM_CHECK_STATUS = 3,
    ARM_STATUS = 4,
    ARM_CONNECT = 5,
    ARM_CONNECTED = 6,
    ARM_MOVE_TO_POINT = 7,
    ARM_APPLY_TRAJECTORY = 8,
    ARM_CANCEL_TRAJECTORY = 9,
    ARM_CANCELED_TRAJECTORY = 10,
    ARM_DISCONNECT = 11,
    ARM_DISCONNECTED = 12,
    ARM_HOME_SEARCHED = 13
};

/**
 * A class modelling the information about a robot joint.
 * It contains the minimum and que maximum values assumed
 * by a joint in DEGREES (angles)
 **/
class JointInfo
{
public:
    double minimum;
    double maximum;

    JointInfo()
    {
        this->minimum = 0.0;
        this->maximum = 0.0;
    }

    JointInfo(double min, double max)
    {
        minimum = min;
        maximum = max;
    }

    /**
     * The equality operation for JointInfo
     **/
    bool operator==(JointInfo other)
    {
        return this->maximum == other.maximum && this->minimum == other.minimum;
    }

    /**
     * Method to convert a  JointInfo object into a
     * Niels Lohmann library JSON object
     **/
    json to_json()
    {
        json result;

        result["minimum"] = this->minimum;
        result["maximum"] = this->maximum;

        return result;
    }

    /**
     * Static method to recover a JointInfo object from a
     * Niels Lohmann library JSON object
     **/
    static JointInfo from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    /**
     * Static method to recover a JointInfo object from a
     * string containing a suitable Jsonified JointInfo object
     **/
    static JointInfo from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);

        JointInfo result = JointInfo();
        result.maximum = json_obj["maximum"];
        result.minimum = json_obj["minimum"];

        return result;
    }
};

/**
 * A class representing the meta info object of an arm
 **/
class MetaInfoObject
{
public:
    MetaInfoSignal signal;
    std::string name;
    std::list<JointInfo> joints;

    MetaInfoObject()
    {
        this->signal = ARM_METAINFO;
        this->name = ROBOT_NAME;
        this->joints = std::list<JointInfo>();
    }

    MetaInfoObject(MetaInfoSignal sig, std::string n, std::list<JointInfo> jts)
    {
        signal = sig;
        name = n;
        joints = std::list<JointInfo>(jts);
    }

    /**
     * The equality operation for MetaInfoObject
     **/
    bool operator==(MetaInfoObject other)
    {
        bool result = true;
        result = result && this->signal == other.signal;
        if (result)
        {
            result = result && strcmp(this->name.c_str(), other.name.c_str()) == 0;
            if (result)
            {
                result = result && this->joints.size() == other.joints.size();
                if (result)
                {
                    std::list<JointInfo> auxThis = std::list<JointInfo>(this->joints);
                    std::list<JointInfo> auxOther = std::list<JointInfo>(other.joints);

                    while (!auxThis.empty() && result)
                    {
                        JointInfo jThis = auxThis.front();
                        JointInfo jOther = auxOther.front();
                        result = result && jThis == jOther;
                        auxThis.erase(auxThis.begin());
                        auxOther.erase(auxOther.begin());
                    }
                }
            }
        }
        return result;
    }

    /**
     * Method to convert a MetaInfoObject into a
     * Niels Lohmann library JSON object
     **/
    json to_json()
    {
        json result;

        result["signal"] = this->signal;
        result["name"] = this->name;
        json js = json::array();

        for (JointInfo j : joints)
        {
            js.push_back(j.to_json());
        }
        result["joints"] = js;

        return result;
    }

    /**
     * Static method to recover a MetaInfoObject from a
     * Niels Lohmann library JSON object
     **/
    static MetaInfoObject from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    /**
     * Static method to recover a MetaInfoObject from a
     * string containing a suitable Jsonified MetaInfoObject
     **/
    static MetaInfoObject from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);

        MetaInfoObject result = MetaInfoObject();
        result.signal = json_obj["signal"];
        result.name = json_obj["name"];

        std::list<JointInfo> joints = std::list<JointInfo>();
        json list = json_obj["joints"];

        for (json::iterator it = list.begin(); it != list.end(); ++it)
        {
            JointInfo ji = JointInfo::from_json(it.value());
            result.joints.push_back(ji);
        }

        return result;
    }
};

/**
 * A class representing a client of the arm
 **/
class Client
{
public:
    std::string id;

    Client(std::string ident)
    {
        this->id = ident;
    }

    Client()
    {
        this->id = "";
    }

    /**
     * Method to validate a client. Valid clients have some
     * content into their id attribute
     **/
    bool is_valid()
    {
        bool ret = (strcmp(this->id.c_str(), "") != 0);
        return ret;
    }

    /**
     * The equality operation for Client object
     **/
    bool operator==(Client other)
    {
        return strcmp(this->id.c_str(), other.id.c_str()) == 0;
    }

    /**
     * Method to convert a Client object into a
     * Niels Lohmann library JSON object
     **/
    json to_json()
    {
        json result;

        result["id"] = this->id;

        return result;
    }

    /**
     * Static method to recover a Client object from a
     * Niels Lohmann library JSON object
     **/
    static Client from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    /**
     * Static method to recover a Client object from a
     * string containing a suitable Jsonified client object
     **/
    static Client from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);

        Client result = Client();
        result.id = json_obj["id"];

        return result;
    }
};

/**
 * A global variable meaning the owner of the arm.
 **/
static Client owner = Client();

/**
 * A class representing a point as a set of coordinates (values for each joint of the arm)
 **/
class Point
{
public:
    std::vector<double> coordinates;
    Point()
    {
        coordinates = std::vector<double>();
    }
    Point(std::vector<double> coords)
    {
        coordinates = coords;
    }

    /**
     * The equality operation for Point object
     **/
    bool operator==(Point other)
    {
        return coordinates == other.coordinates;
    }

    /**
     * Method to say if a point is empty (has coordinates) or not
     **/
    bool is_empty()
    {
        int size = coordinates.size();
        return coordinates.size() == 0;
    }

    /**
     * Method to convert a Point object into a
     * Niels Lohmann library JSON object
     **/
    json to_json()
    {
        json result;
        json coords = json::array();

        if (!is_empty())
        {
            for (double coord : coordinates)
            {
                coords.push_back(coord);
            }
        }

        result["coordinates"] = coords;

        return result;
    }

    /**
     * Static method to recover a Point object from a
     * Niels Lohmann library JSON object
     **/
    static Point from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    /**
     * Static method to recover a Point object from a
     * string containing a suitable Jsonified point object
     **/
    static Point from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);

        Point result = Point();

        json coords = json_obj["coordinates"];

        for (json::iterator it = coords.begin(); it != coords.end(); ++it)
        {
            double c = it.value();
            result.coordinates.push_back(c);
        }

        return result;
    }
};

/**
 * A class representing a trajectory as a list of points.
 **/
class Trajectory
{
public:
    std::list<Point> points;
    Trajectory()
    {
        points = std::list<Point>();
    }
    Trajectory(std::list<Point> pts)
    {
        points = std::list<Point>(pts);
    };
    bool is_empty()
    {
        return this->points.size() == 0;
    }

    /**
     * The equality operation for this class
     **/
    bool operator==(Trajectory other)
    {
        bool result = true;
        result = result && this->points.size() == other.points.size();
        if (result)
        {
            std::list<Point> auxThis = std::list<Point>(this->points);
            std::list<Point> auxOther = std::list<Point>(other.points);

            while (!auxThis.empty() && result)
            {
                Point pThis = auxThis.front();
                Point pOther = auxOther.front();
                result = result && pThis == pOther;
                auxThis.erase(auxThis.begin());
                auxOther.erase(auxOther.begin());
            }
        }
        return result;
    }

    /**
     * Method to convert a Trajectory object into a
     * Niels Lohmann library JSON object
     **/
    json to_json()
    {
        json result;
        json pts = json::array();

        for (Point point : points)
        {
            json p = point.to_json();
            pts.push_back(p);
        }
        result["points"] = pts;

        return result;
    }

    /**
     * Static method to recover a Trajectory object from a
     * Niels Lohmann library JSON object
     **/
    static Trajectory from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    /**
     * Static method to recover a Trajectory object from a
     * string containing a suitable Jsonified trajectory object
     **/
    static Trajectory from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);

        Trajectory result = Trajectory();

        json points = json_obj["points"];

        for (json::iterator it = points.begin(); it != points.end(); ++it)
        {
            Point p = Point::from_json(it.value());
            result.points.push_back(p);
        }

        return result;
    }
};

/**
 * Class modelling an object to be exchanged on channel ROBOT_NAME/commands.
 **/
class CommandObject
{
public:
    CommandsSignal signal;
    Client client;
    bool error;
    Point point;
    Trajectory trajectory;

    CommandObject(CommandsSignal sig)
    {
        signal = sig;
        client = Client();
        error = error_state;
        point = Point();
        trajectory = Trajectory();
    }

    CommandObject(CommandsSignal sig, Point p)
    {
        signal = sig;
        client = Client();
        error = error_state;
        point = p;
        trajectory = Trajectory();
    }

    CommandObject(CommandsSignal sig, Trajectory t)
    {
        signal = sig;
        client = Client();
        error = error_state;
        point = Point();
        trajectory = t;
    }

    bool operator==(CommandObject other)
    {
        bool result = true;

        result = result && this->signal == other.signal;
        if (result)
        {
            result = result && this->client == other.client;
            if (result)
            {
                if (point.is_empty())
                {
                    result = result && this->trajectory == other.trajectory;
                }
                else
                {
                    result = result &&
                             this->point == other.point;
                }
            }
        }
        return result;
    }

    json to_json()
    {
        json result;
        result["signal"] = signal;
        if (client.is_valid())
        {
            result["client"] = client.to_json();
        }
        result["error"] = error_state;
        if (!point.is_empty())
        {
            result["point"] = this->point.to_json();
        }
        else
        {
            if (!trajectory.is_empty())
            {
                result["trajectory"] = this->trajectory.to_json();
            }
        }

        return result;
    }

    static CommandObject from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    static CommandObject from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);
        CommandObject result = CommandObject(json_obj["signal"]);
        if (json_obj.contains("client"))
        {
            Client cli = Client::from_json(json_obj["client"]);
            result.client = cli;
        }
        if (json_obj.contains("point"))
        { // contains point
            Point p = Point::from_json(json_obj["point"]);
            result.point = p;
        }
        else
        { // constains trajectory
            if (json_obj.contains("trajectory"))
            {
                Trajectory t = Trajectory::from_json(json_obj["trajectory"]);
                result.trajectory = t;
            }
        }
        return result;
    }
};

/**
 * A class representing the object to be exchanged on channel ROBOT_NAME/moved
 **/
class MovedObject
{
public:
    Client client;
    bool error;
    Point content;

    MovedObject()
    {
        client = owner;
        error = error_state;
    }

    MovedObject(Point p)
    {
        client = owner;
        error = error_state;
        content = p;
    }

    bool operator==(MovedObject other)
    {
        return content == other.content;
    }

    json to_json()
    {
        json result;
        result["client"] = client.to_json();
        result["error"] = error_state;
        result["content"] = content.to_json();

        return result;
    }
    static MovedObject from_json(json json_obj)
    {
        return from_json_string(json_obj.dump());
    }

    static MovedObject from_json_string(std::string json_string)
    {
        json json_obj = json::parse(json_string);
        MovedObject result = MovedObject();
        Client cli = Client::from_json(json_obj["client"]);
        bool error = json_obj["error"];
        Point p = Point::from_json(json_obj["content"]);
        result.client = cli;
        result.error = error;
        result.content = p;

        return result;
    }
};

/**
 * Function to convert angles into reference values. This function is
 * specific for each arm.
 *
 * TODO: Adjust this implementation according to your arm
 *
 * DOUBT: angles are in DEGREES or in RADIANS?
 **/
int angle_to_ref(int motor, float angle)
{
    switch (motor)
    {
    case 1:
        return int(-3 * angle);
    case 2:
        return int(-9.4 * angle);
    case 3:
        return int(-3.1 * angle);
    case 4:
        return int(-17.61158871 * angle);
    default:
        puts("Maximum actionable joint is J4 for them moment");
        break;
    }
    return 0;
}

/**
 * Function to convert reference values into angles. This function is
 * specific for each arm.
 *
 * TODO: Adjust this implementation according to your arm
 *
 * DOUBT: angles are in DEGREES or in RADIANS?
 **/
double ref_to_angle(int motor, int ref)
{
    switch (motor)
    {
    case 1:
        return ((-1.0 / 3.0) * ref);
    case 2:
        return ((-1 / 9.4) * ref);
    case 3:
        return ((-1 / 3.1) * ref);
    case 4:
        return (-0.056780795 * ref);
    default:
        puts("Maximum actionable joint is J4 for them moment");
        break;
    }
    return 0;
}

/**
 * The global metainfo of this arm.
 *
 * TODO: Adjust only the reference values (the second argument of ref_to_angle)
 * for each motor
 *
 **/
const std::list<JointInfo> METAINFOS =
    {
        JointInfo(ref_to_angle(1, -450), ref_to_angle(1, 500)),
        JointInfo(ref_to_angle(2, -950), ref_to_angle(2, 800)),
        JointInfo(ref_to_angle(3, -350), ref_to_angle(3, 350)),
        JointInfo(ref_to_angle(4, -1500), ref_to_angle(4, 1600)),
        JointInfo(-360, 360),
        JointInfo(0, 100)};

/**
 * Function that build all topics using the robot name. Ex:
 * COMMANDS_TOPIC = ROBOT_NAME/commands
 * MOVED_TOPIC = ROBOT_NAME/moved
 **/
int build_topics();

/**
 * Fuction that publishes a message (the string representation of a JSON object)
 * using a specific topic
 **/
int publish_message(std::string topic, const char *buf);

/**
 * Function that checks if a message has a signal. This is useful to
 * detect if the incoming message follows the async specification
 * where all messages from clients must have a signal code
 **/
bool has_signal(std::string message);

/**
 * Function that extracts the signal of a message
 **/
int extract_signal(std::string message);

/**
 * Function that returns the initial metainfo of this arm
 **/
MetaInfoObject initial_metainfoobj();

/**
 * Function that handles messages delivered on channel metainfo
 **/
void handle_metainfo_message(std::string mesage);

/**
 * Function that handles messages delivered on channel ROBOT_NAME/commands
 **/
void handle_commands_message(std::string mesage);
