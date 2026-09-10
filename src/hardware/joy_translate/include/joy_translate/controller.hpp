#ifndef JOY_BASE_HPP
#define JOY_BASE_HPP

#include "sensor_msgs/msg/joy.hpp"
#include <string>

class Controller
{
    public:
        /*Axis*/
        //joystic
        float joy_left_x = 0.0;
        float joy_left_y = 0.0; 
        float joy_right_x = 0.0;
        float joy_right_y = 0.0;

        //方向キー(左十字キー)
        float  left_right_btn = 0.0;
        float  up_down_btn = 0.0;

        /*Button*/
        //bottan
        bool A_btn;  
        bool B_btn;  
        bool Y_btn;  
        bool X_btn;  

        //側面ボタン
        bool LB_btn;
        bool RB_btn;
        bool LT_btn;
        bool RT_btn;
    
        //etc botttan
        bool back_btn;
        bool start_btn;

        bool R3_btn;
        bool L3_btn;

        void update_key_value(const sensor_msgs::msg::Joy &msg)
        {
            //Axis
            joy_left_x = msg.axes[0];
            joy_left_y = msg.axes[1];
            joy_right_x = msg.axes[2];
            joy_right_y = msg.axes[3];
            
            left_right_btn = msg.axes[4];
            up_down_btn = msg.axes[5];

            //Button
            X_btn = msg.buttons[0];
            A_btn = msg.buttons[1];
            B_btn = msg.buttons[2];
            Y_btn = msg.buttons[3];

            LB_btn          = msg.buttons[4];
            RB_btn          = msg.buttons[5];
            LT_btn          = msg.buttons[6];
            RT_btn          = msg.buttons[7];

            back_btn      = msg.buttons[8];
            start_btn     = msg.buttons[9];

            L3_btn = msg.buttons[10];
            R3_btn = msg.buttons[11];
            apply_deadzones();
        }

        void init_data()
        {
            /*Axis*/
            joy_left_x = 0;
            joy_left_y = 0; 
            joy_right_x = 0;
            joy_right_y = 0;

            left_right_btn = 0;
            up_down_btn = 0;

            /*Button*/
            X_btn = false;
            A_btn = false;
            B_btn = false;
            Y_btn = false;

            LB_btn = false;
            RB_btn = false;
            LT_btn = false;
            RT_btn = false;

            back_btn = false; 
            start_btn = false;
            L3_btn = false;
            R3_btn = false;
        }
                
        void apply_deadzone(float &value, float deadzone = 0.1) {
            if (std::abs(value) < deadzone) {
                value = 0.0;
            }
        }

        void apply_deadzones() {
            apply_deadzone(joy_left_x);
            apply_deadzone(joy_left_y);
            apply_deadzone(joy_right_x);
            apply_deadzone(joy_right_y);
        }
};
#endif
