# Forza Horizon 5

An overview of how to configure the game and details of the data

## Configure Forza Horizon 5 Data Output

Enabling/disabling UDP data output is done by toggling the following:

`Options -> HUD and Gameplay -> Data Out`

Additionally, just under this configuration set the **ip address** and **port** to point to the Arduino.

The **port** is found in [forza.ino](src/forza/forza.ino) as `local_port`.

The **ip** may be found by using this sketch [ip_finder](https://gitlab.com/christiangroleau/arduino-tools/-/tree/main/ip_finder), or through your network gateway.


## Forza Data Specification

This table describes the data represented in [forza_data.h](src/forza/forza_data.h) with a description for labels that may not be obvious.

Name | Type | Description |
--- | --- | --- |
is_race_on | int32 | Boolean where **1** is `race started` or **0** is `in menus` or `race ended` |
timestamp_ms | uint32 | Game's internal timestamp in milliseconds |
| | |
engine_max_rpm | float | Current car's maximum RPM |
engine_idle_rpm | float |  |
current_engine_rpm | float |  |
| | |
acceleration_x | float |  |
acceleration_y | float |  |
acceleration_z | float |  |
| | |
velocity_x | float |  |
velocity_y | float |  |
velocity_z | float |  |
| | |
angular_velocity_x | float |  |
angular_velocity_y | float |  |
angular_velocity_z | float |  |
| | |
yaw | float |  |
pitch | float |  |
roll | float |  |
| | |
normalized_suspension_travel_front_left | float | **0.0** is `full stretch`, **1.0** is `full compression` |
normalized_suspension_travel_front_right | float |  |
normalized_suspension_travel_rear_left | float |  |
normalized_suspension_travel_rear_right | float |  |
| | |
tire_slip_ratio_front_left | float | **0.0** is `100% grip`, >**1.0** is `loss of grip` |
tire_slip_ratio_front_right | float |  |
tire_slip_ratio_rear_left | float |  |
tire_slip_ratio_rear_right | float |  |
| | |
wheel_rotation_speed_front_left | float | radians per second |
wheel_rotation_speed_front_right | float |  |
wheel_rotation_speed_rear_left | float |  |
wheel_rotation_speed_rear_right | float |  |
| | |
wheel_on_rumble_strip_front_left | int32 | Boolean tire is on a `rumble strip` where **1** is `yes` or **0**  is `no` |
wheel_on_rumble_strip_front_right | int32 |  |
wheel_on_rumble_strip_rear_left | int32 |  |
wheel_on_rumble_strip_rear_right | int32 |  |
| | |
wheel_in_puddle_depth_front_left | float | Range from **0** to **1** whether the tire is in a puddle (**1** is the deepest puddle) |
wheel_in_puddle_depth_front_right | float |  |
wheel_in_puddle_depth_rear_left | float |  |
wheel_in_puddle_depth_rear_right | float |  |
| | |
surface_rumble_front_left | float | Non-dimensional surface rumble value for the tire (passed to controller force feedback) |
surface_rumble_front_right | float |  |
surface_rumble_rear_left | float |  |
surface_rumble_rear_right | float |  |
| | |
tire_slip_angle_front_left | float | Normalized slip angle for the tire where **0** is `100% grip`, >**1.0** is `loss of grip` |
tire_slip_angle_front_right | float |  |
tire_slip_angle_rear_left | float |  |
tire_slip_angle_rear_right | float |  |
| | |
tire_combined_slip_front_left | float | Normalized combined slip for the tire where **0** is `100% grip`, >**1.0** is `loss of grip` |
tire_combined_slip_front_right | float |  |
tire_combined_slip_rear_left | float |  |
tire_combined_slip_rear_right | float |  |
| | |
suspension_travel_meters_front_left | float | Suspension travel (in meters) |
suspension_travel_meters_front_right | float |  |
suspension_travel_meters_rear_left | float |  |
suspension_travel_meters_rear_right | float |  |
| | |
car_ordinal | int32 | Unique identifier of the car make and model |
car_class | int32 | Car class from **0** `D` to **7** `X` (inclusive) |
car_performance_index | int32 | Range from **100** `slowest` to **999** `fastest` (inclusive) |
| | |
drivetrain_type | int32 | **0** is `front-wheel drive`, **1** is `rear-wheel drive`, **2** is `all-wheel drive` |
num_cylinders | int32 |  |
| | |
offset0 | float | 4 byte offset (V1 and V2 of this protocol are separated by 12 bytes) |
offset1 | float | 4 byte offset |
offset2 | float | 4 byte offset |
| | |
position_x | float | In meters |
position_y | float |  |
position_z | float |  |
| | |
speed | float | Current speed in meters per second |
power | float | Current power output of the engine in watts |
torque | float | Current torque of the engine in Newton meters |
| | |
tire_temp_front_left | float | Tire temperature |
tire_temp_front_right | float |  |
tire_temp_rear_left | float |  |
tire_temp_rear_right | float |  |
| | |
boost | float | Boost pressure |
fuel | float | Amount of fuel remaining |
distance_traveled | float |  |
best_lap | float | Best lap time of the current race/session (in seconds) |
last_lap | float | Lap time of the previous lap |
current_lap | float | Current time of the current lap |
current_race_time | float | Current total race time |
| | |
lap_number | uint16 |  |
race_position | uint8 |  |
| | |
accel | uint8 | Acceleration input range from **0** to **255** |
brake | uint8 | Amount of brake input |
clutch | uint8 | Amount of clutch input |
hand_brake | uint8 | Amount of handbarke input |
gear | uint8 | Current gear |
steer | int8 | Amount of steering input |
| | |
normalized_driving_line | int8 |  |
normalized_ai_brake_difference | int8 |  |

### Sources
 - [2018 forzamotorsport.net post](https://web.archive.org/web/20200808065339/https://forums.forzamotorsport.net/turn10_postsm926839_Forza-Motorsport-7--Data-Out--feature-details.aspx/)
 - [nettrom's list of parameters](https://github.com/nettrom/forza_motorsport/blob/master/configuration_file.md)