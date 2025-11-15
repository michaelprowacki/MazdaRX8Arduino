/*
 *  Project     RX-8 Arduino
 *  @author     Christian Groleau
 *  @link       https://gitlab.com/christiangroleau/rx8-arduino
 *  @license    MIT - Copyright (c) 2022 Christian Groleau
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* 
 * Notes:
 * There does not appear to be a current and 'official' specification of 
 * the Forza UDP data output. However, this old post from the forzamotorsport.net 
 * forums from 'Mechberg' is one of the best sources.
 * See here:
 * https://web.archive.org/web/20200808065339/https://forums.forzamotorsport.net/turn10_postsm926839_Forza-Motorsport-7--Data-Out--feature-details.aspx/
 *
 * More details are provided in the readme
 */
 
typedef struct ForzaData_s {

    // V1 'sled'
    int32_t is_race_on; // 1 when race is started, 0 when in menus or race stopped
    
    uint32_t timestamp_ms; // can overflow to 0 eventually
 
    float engine_max_rpm;
    float engine_idle_rpm;
    float current_engine_rpm;
    
    float acceleration_x; // right
    float acceleration_y; // up
    float acceleration_z; // forward
    
    float velocity_x;
    float velocity_y;
    float velocity_z;

    float angular_velocity_x; // pitch
    float angular_velocity_y; // yaw
    float angular_velocity_z; // roll
    
    float yaw;
    float pitch;
    float roll;

    // suspension travel normalized:
    float normalized_suspension_travel_front_left; // 0.0f is max stretch, 1.0 is max compression
    float normalized_suspension_travel_front_right;
    float normalized_suspension_travel_rear_left;
    float normalized_suspension_travel_rear_right;

    // tire normalized slip ratio
    float tire_slip_ratio_front_left; // 0 is 100% grip, |ratio| > 1.0 is loss of grip
    float tire_slip_ratio_front_right;
    float tire_slip_ratio_rear_left;
    float tire_slip_ratio_rear_right;

    // wheel rotation speed radians/sec
    float wheel_rotation_speed_front_left;
    float wheel_rotation_speed_front_right;
    float wheel_rotation_speed_rear_left;
    float wheel_rotation_speed_rear_right;

    int32_t wheel_on_rumble_strip_front_left; // 1 when wheel is on rumble strip, 0 when off
    int32_t wheel_on_rumble_strip_front_right;
    int32_t wheel_on_rumble_strip_rear_left;
    int32_t wheel_on_rumble_strip_rear_right;

    float wheel_in_puddle_depth_front_left; // 0 to 1, where 1 is the deepest puddle
    float wheel_in_puddle_depth_front_right;
    float wheel_in_puddle_depth_rear_left;
    float wheel_in_puddle_depth_rear_right;

    // non-dimensional surface rumble values passed to controller force feedback
    float surface_rumble_front_left;
    float surface_rumble_front_right;
    float surface_rumble_rear_left;
    float surface_rumble_rear_right;

    // tire normalized slip angle
    float tire_slip_angle_front_left; // 0 is 100% grip, |angle| > 1.0 is loss of grip
    float tire_slip_angle_front_right;
    float tire_slip_angle_rear_left;
    float tire_slip_angle_rear_right;

    // tire normalized combined slip
    float tire_combined_slip_front_left; // 0 is 100% grip, |slip| > 1.0 is loss of grip
    float tire_combined_slip_front_right;
    float tire_combined_slip_rear_left;
    float tire_combined_slip_rear_right;

    // suspension travel in meters
    float suspension_travel_meters_front_left;
    float suspension_travel_meters_front_right;
    float suspension_travel_meters_rear_left;
    float suspension_travel_meters_rear_right;

    int32_t car_ordinal;           // unique id of the car make/model
    int32_t car_class;             // between 0 (d -- worst cars) and 7 (_x class -- best cars) inclusive
    int32_t car_performance_index; // between 100 (slowest car) and 999 (fastest car) inclusive
    int32_t drivetrain_type;       // corresponds to e_drivetrain_type; 0 = f_w_d, 1 = _r_w_d, 2 = _a_w_d
    int32_t num_cylinders;         // number of cylinders in the engine

    // V2 known as 'car dash'

    // V1 and V2 is separated by 12 bytes offset
    float offset0;
    float offset1;
    float offset2;

    // position (meters)
    float position_x;
    float position_y;
    float position_z;

    float speed;  // meters per second
    float power;  // watts
    float torque; // newton meter

    float tire_temp_front_left;
    float tire_temp_front_right;
    float tire_temp_rear_left;
    float tire_temp_rear_right;

    float boost;
    float fuel;
    float distance_traveled;
    float best_lap;
    float last_lap;
    float current_lap;
    float current_race_time;

    uint16_t lap_number;
    uint8_t race_position;

    uint8_t accel;
    uint8_t brake;
    uint8_t clutch;
    uint8_t hand_brake;
    uint8_t gear;
    int8_t steer;

    int8_t normalized_driving_line;
    int8_t normalized_ai_brake_difference;

} ForzaData_t;
