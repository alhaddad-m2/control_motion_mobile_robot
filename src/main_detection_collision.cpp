#include <ros/ros.h>
#include <std_msgs/Bool.h>
#include <tf2_ros/transform_listener.h>
#include <tf/transform_listener.h>
#include "nav_msgs/OccupancyGrid.h"
#include <control_mobile_robot/SimulatecollisionAction.h>
#include <actionlib/server/simple_action_server.h>
#include <vector>
#include "visualization_msgs/Marker.h"


typedef actionlib::SimpleActionServer<control_mobile_robot::SimulatecollisionAction> Server;
control_mobile_robot::SimulatecollisionResult result_msg;

ros::Subscriber     grid_map_sub;

ros::Publisher      pub_collision_rotation , pub_collision_backward , pub_collision_position , pub_marker_footprint;

// ROS messages
visualization_msgs::Marker      footprint_mk;

geometry_msgs::Point            footprint_pt;

bool map_received;
double dist_tol = 0;
double angle_rotation = 0;
nav_msgs::OccupancyGrid  grid_map;
double x_robot , y_robot , theta_robot;
std_msgs::Bool collision_rotation , collision_backward , collision_position;
bool robot_in_passage = true;
bool StartCheck = false;
double x1_path , y1_path;
bool check_position = false , goto_position_better = false;
int index_cell_at_coordintes(double x_s , double y_s);
double get_yaw(geometry_msgs::Quaternion q);
void publish_result(Server* as);
bool collision_footprint(double theta_sim , bool all_sides, double x_sim , double y_sim);
bool collision_simulated_rotation(bool all_side , double x_sim , double y_sim);
bool collision_backward_motion();

// action server recieve command to start checking

void execute(const control_mobile_robot::SimulatecollisionGoal::ConstPtr& goal, Server* as)
{
       // angle_rotation=goal->angle;
        x1_path = goal->x;
        y1_path = goal->y;
        ros::Rate feedback_rate(2);
        StartCheck = true;   
        check_position = true;  
        goto_position_better = false;   
        result_msg.status = true;
        as->setSucceeded(result_msg);
        ROS_INFO("Simulation Rotation Action Received");
        feedback_rate.sleep();
}

// callback for local map
// store map in grid_map, activate flag map_received and calculate distance tolerance
void grid_map_back(const nav_msgs::OccupancyGrid& msg)
{
        map_received = true;
        grid_map = msg;
        dist_tol = msg.info.resolution - 0.1 * msg.info.resolution;
}
// check collision footprint robot at rotation in specefic position x_sim, y_sim 
// increase theta from theta robot and angle rotation
// return true if there is collision, false if no collision
bool collision_simulated_rotation(bool all_side , double x_sim , double y_sim)
{
    bool collision_status = false;
    if(angle_rotation >= theta_robot)
    {
        for(double theta_sim = theta_robot; theta_sim<angle_rotation ; theta_sim= theta_sim + 0.1)
        {
            ros::Rate loop_rate(20);
            collision_status = collision_footprint(theta_sim , all_side , x_sim , y_sim);
            if(collision_status)
            break;
            loop_rate.sleep();

        }
    }
    else
    {
        for(double theta_sim = theta_robot; theta_sim>angle_rotation ; theta_sim=theta_sim-0.1)
        {
            ros::Rate loop_rate(20);
            collision_status = collision_footprint(theta_sim , all_side , x_sim , y_sim);
            if(collision_status)
            break;
            loop_rate.sleep();
        }
    }
    return collision_status;
}

bool collision_footprint(double theta_sim , bool all_sides , double x_sim , double y_sim)
{
    bool result = false;

    double x_scan , y_scan;
    double x_start = 0 , y_start = 0;
    int index_scan;
    double x1 , y1;
    x1 = x_sim - 0.5*cos(theta_sim);
    y1 = y_sim - 0.5*sin(theta_sim);
    footprint_mk.points.clear();
    footprint_mk.color.r = 0.0;
    footprint_mk.color.g = 1.0; 
    int mk=0;

    // check collision at sides of robot only
    for(double k = -0.05; k<=1.1; k=k+0.02)
    {
        x_start = x1+k*cos(theta_sim);
        y_start = y1+k*sin(theta_sim);
        if(!all_sides)
        {
            double j = 0.38;
            x_scan = x_start + j*cos(theta_sim-M_PI/2);
            y_scan = y_start + j*sin(theta_sim-M_PI/2);
            if(mk%5==0)
            {
                footprint_pt.x = x_scan;
                footprint_pt.y = y_scan;
                footprint_pt.z = 0;
                footprint_mk.points.push_back(footprint_pt);
            }
            index_scan = index_cell_at_coordintes(x_scan , y_scan);
            if (index_scan > 0)
            {
                result = true;
                footprint_mk.color.r = 1.0; 
                footprint_mk.color.g = 0.0; 
                break;                  
            }
            x_scan = x_start + j*cos(theta_sim+M_PI/2);
            y_scan = y_start + j*sin(theta_sim+M_PI/2);
            if(mk%5==0)
            {
                footprint_pt.x = x_scan;
                footprint_pt.y = y_scan;
                footprint_pt.z = 0;
                footprint_mk.points.push_back(footprint_pt);
            }
            
            index_scan = index_cell_at_coordintes(x_scan , y_scan);
            if (index_scan > 0)
            {
                result = true;
                footprint_mk.color.r = 1.0;
                footprint_mk.color.g = 0.0; 
                break;                  
            }
            mk++;
        }
        else // check collision at all points of footprint
        {
            for(double j = 0; j<=0.36 ; j+=0.06)
            {
                x_scan = x_start + j*cos(theta_sim-M_PI/2);
                y_scan = y_start + j*sin(theta_sim-M_PI/2);
                if(mk%5==0)
                {
                    footprint_pt.x = x_scan;
                    footprint_pt.y = y_scan;
                    footprint_pt.z = 0;
                    footprint_mk.points.push_back(footprint_pt);
                }
                index_scan = index_cell_at_coordintes(x_scan , y_scan);
                if (index_scan > 0)
                {
                    result = true;
                    footprint_mk.color.r = 1.0; 
                    footprint_mk.color.g = 0.0; 
                    break;                  
                }
                x_scan = x_start + j*cos(theta_sim+M_PI/2);
                y_scan = y_start + j*sin(theta_sim+M_PI/2);
                if(mk%5==0)
                {
                    footprint_pt.x = x_scan;
                    footprint_pt.y = y_scan;
                    footprint_pt.z = 0;
                    footprint_mk.points.push_back(footprint_pt);
                }
                
                index_scan = index_cell_at_coordintes(x_scan , y_scan);
                if (index_scan > 0)
                {
                    result = true;
                    footprint_mk.color.r = 1.0;
                    footprint_mk.color.g = 0.0; 
                    break;                  
                }
                mk++;
            }
        }
    }
   
    pub_marker_footprint.publish(footprint_mk);
    return result;
}
// to check status collision for motion backward
// return true is there is collision
bool collision_backward_motion()
{
    bool result = false;

    double x_scan , y_scan;
    double x_start = 0 , y_start = 0;
    int index_scan;
    double x1 , y1;
    x1 = x_robot - 0.5*cos(theta_robot);
    y1 = y_robot - 0.5*sin(theta_robot);
    for(double k = -0.25; k<=0.1; k+=dist_tol)
    {
        x_start = x1+k*cos(theta_robot);
        y_start = y1+k*sin(theta_robot);
        for(double j=0;j<=0.36;j+=dist_tol)
        {
            x_scan = x_start + j*cos(theta_robot-M_PI/2);
            y_scan = y_start + j*sin(theta_robot-M_PI/2);
            index_scan = index_cell_at_coordintes(x_scan , y_scan);
            if (index_scan > 0)
            {
                result = true;
                break;                  
            }
            x_scan = x_start + j*cos(theta_robot+M_PI/2);
            y_scan = y_start + j*sin(theta_robot+M_PI/2);
            index_scan = index_cell_at_coordintes(x_scan , y_scan);
            if (index_scan > 0)
            {
                result = true;
                break;                  
            }
        }
        if(result)
            break;
    }
    return result;
}


int index_cell_at_coordintes(double x_s , double y_s)
{
    int col = round((x_s - grid_map.info.origin.position.x)/grid_map.info.resolution);
    int row = round((y_s - grid_map.info.origin.position.y)/grid_map.info.resolution);
    int index_s = row * grid_map.info.width + col;
    if (grid_map.data[index_s]==100)
        return index_s;
    else
        return 0;
}



int main(int argc, char **argv)
{
    ros::init(argc, argv, "detection_passage");

    ros::NodeHandle n;

    grid_map_sub            = n.subscribe("/occupancy_grid_local_map/grid_map",       10, grid_map_back);  
    pub_collision_rotation =  n.advertise<std_msgs::Bool> ("collision_rotation", 10);
    pub_collision_backward =  n.advertise<std_msgs::Bool> ("collision_backward", 10);
    pub_collision_position =  n.advertise<std_msgs::Bool> ("collision_position", 10);
    pub_marker_footprint   = n.advertise<visualization_msgs::Marker> ("footprint", 10);

    tf2_ros::Buffer tfBuffer;

    tf2_ros::TransformListener tfListener(tfBuffer);
    footprint_mk.points.clear();
    footprint_mk.header.frame_id = "local_map_lidar";
    footprint_mk.header.stamp = ros::Time::now();
    footprint_mk.ns = "footprint_mk" ;
    footprint_mk.action = visualization_msgs::Marker::ADD;;
    footprint_mk.pose.orientation.w = 1.0;
    footprint_mk.type = 8;
    footprint_mk.scale.x = 0.1;
    footprint_mk.scale.y = 0.1;
    footprint_mk.color.a = 1.0;
    footprint_mk.color.r = 0.0;
    footprint_mk.color.g = 1.0;
    footprint_mk.color.b = 0.0;

    Server server(n, "detection_passage_server", boost::bind(&execute, _1, &server), false);
	server.start();

    ros::Rate loop_rate(1);
    while(ros::ok())
    {
        geometry_msgs::TransformStamped transformStamped;
        tf::StampedTransform echo_transform;
        try{
        
            transformStamped = tfBuffer.lookupTransform("local_map_lidar", "base_link",
                               ros::Time(0));
        }
        catch (tf2::TransformException &ex) {
        ROS_WARN("%s",ex.what());
        ros::Duration(1.0).sleep();
        continue;
        }
        x_robot = transformStamped.transform.translation.x;
        y_robot = transformStamped.transform.translation.y; 
        theta_robot = get_yaw(transformStamped.transform.rotation);
        // check collisions if there is map and recieved command
        if(map_received && StartCheck)
        {     
            angle_rotation=atan2((y1_path-y_robot),(x1_path-x_robot)); 
            if(angle_rotation>=M_PI/2 && theta_robot<=-M_PI/2)
                theta_robot = 2*M_PI + theta_robot;
            else if (angle_rotation<=-M_PI/2 && theta_robot>=M_PI/2)
                theta_robot = -2*M_PI + theta_robot;

            

            // 1- check collision at for rotation at robot position
            if(collision_simulated_rotation(false , x_robot , y_robot))
            {
                collision_rotation.data = true;
                if(abs(angle_rotation - theta_robot)<=1 && check_position)
                {
                    // there is collision for rotation at robot position
                    collision_position.data = true;
                    
                    double xp = x_robot + 0.6*cos(theta_robot);
                    double yp = y_robot + 0.6*sin(theta_robot);
                    angle_rotation=atan2((y1_path-yp),(x1_path-xp)); 
                    if(angle_rotation>=M_PI/2 && theta_robot<=-M_PI/2)
                        theta_robot = 2*M_PI + theta_robot;
                    else if (angle_rotation<=-M_PI/2 && theta_robot>=M_PI/2)
                        theta_robot = -2*M_PI + theta_robot;
                    // check collision at forward point (all points of footprint should be taken)
                    if(collision_simulated_rotation(true , xp , yp))
                    {
                        collision_position.data = true;
                        ROS_WARN("Rotation here -> collision , Forward Motion -> collision");
                    }
                    else
                    {
                        ROS_WARN("Rotation here -> collision ,Forward Motion -> No collision");
                        collision_position.data = false;
                        goto_position_better = true;
                    }
                }
                check_position = false;
            }
            else
            {
                ROS_INFO("Rotation here -> No collision");
                collision_rotation.data = false;
                collision_position.data = false;
                StartCheck = false;
            }
            // check collision at motion backward
            if(collision_backward_motion() && !goto_position_better)
            {
                collision_backward.data = true;
                ROS_INFO("Backward Motion -> collision");
                
            }
            else
            {
                collision_backward.data = false;
                ROS_INFO("Backward Motion -> No collision");
            }
            if(!goto_position_better)
            {
                collision_position.data = true;
            }

            pub_collision_rotation.publish(collision_rotation);
            pub_collision_position.publish(collision_position);
            pub_collision_backward.publish(collision_backward);
            
        }

        ros::spinOnce();
        loop_rate.sleep();
    }
    return 0;
}

double get_yaw(geometry_msgs::Quaternion q)
{
    double siny_cosp = 2 * (q.w * q.z + q.x * q.y);
    double cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
    return std::atan2(siny_cosp, cosy_cosp);
}