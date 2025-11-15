# Dirt Rally 2.0

An overview of how to configure the game and details of the data

## Configure Dirt Rally 2 Data Output

Data output is enabled by updating a configuration file

1. In Windows, go to this path (or similar) substiting your username

    `C:\Users\{username}\Documents\My Games\DiRT Rally 2.0`

2. Copy a back up of `hardware_settings_config`

3. Edit the file `hardware_settings_config`

    `<udp enabled=”false” extradata=”0” ip=”127.0.0.1″ port=”20777″ delay=”1″ />`

    And set `enabled` to `true`, `extradata` to `3`

4. Edit the `ip` and `port` to point to the Arduino

    The **port** is found in [dirt.ino](src/dirt/dirt.ino) as `local_port`.

    The **ip** may be found by using this sketch [ip_finder](https://gitlab.com/christiangroleau/arduino-tools/-/tree/main/ip_finder), or through your network gateway.


## Dirt Data Specification

This table describes the data represented in [dirt_data.h](src/forza/dirt_data.h) with a description for labels that may not be obvious.

Name | Type | Description |
--- | --- | --- |
time | float | Total Time (not reset after stage restart) |
lap_time | float | Current Lap/Stage Time (starts on Go!) |
lap_distance | float | Current Lap/Stage Distance (meters) |
total_distance | float | |
 | | |
x | float | Position in the world |
y | float | |
z | float | |
 | | |
speed | float | Current traveling speed |
xv | float | Velicity on a given direction |
yv | float | |
zv | float | |
 | | |
xr | float | Roll direction |
yr | float | |
zr | float | |
 | | |
xd | float | Pitch vector |
yd | float | |
zd | float | |
 | | |
susp_pos_rl | float | Position of suspension |
susp_pos_rr | float | |
susp_pos_fl | float | |
susp_pos_fr | float | |
 | | |
susp_vel_rl | float | Velocity of suspension |
susp_vel_rr | float | |
susp_vel_fl | float | |
susp_vel_fr | float | |
 | | |
wheel_speed_rl | float | Wheel rotation speed |
wheel_speed_rr | float | |
wheel_speed_fl | float | |
wheel_speed_fr | float | |
 | | |
throttle | float | |
steer | float | |
brake | float | |
clutch | float | |
gear | float | Gear [0 = Neutral, 1 = 1, 2 = 2, ..., 10 = Reverse] |
 | | |
gforce_lat | float | |
gforce_lon | float | |
 | | |
lap | float | Current lap |
engine_rate | float | Engine RPM |
sli_pro_native_support | float | ? (always 1) |
car_position | float | |
kers_level | float | ? (always 0) |
kers_max_level | float | ? (always 0) |
drs | float | ? (always 0) |
traction_control | float | ? (always 0) |
anti_lock_brakes | float | ? (always 0) |
fuel_in_tank | float | ? (always 0) |
fuel_capacity | float | ? (always 0) |
in_pits | float | ? (always 0) |
sector | float | ? (always 0) |
sector1_time | float | ? |
sector2_time | float | ? |
 | | |
brakes_temp_rl | float | Temperature Brake in C |
brakes_temp_rr | float | |
brakes_temp_fl | float | |
brakes_temp_fr | float | |
 | | |
wheels_pressure_rl | float | In PSI |
wheels_pressure_rr | float | |
wheels_pressure_fl | float | |
wheels_pressure_fr | float | |
 | | |
team_info | float | Team ID |
total_laps | float | Total number of laps in this race |
track_size | float | Track size (in meters) |
last_lap_time | float | Last lap time |
 | | |
max_rpm | float | |
idle_rpm | float | |
max_gears | float | Maximum gears for the current car |
 | | |
session_type | float | |
drs_allowed | float | |
track_number | float | |
vehicle_fia_flags | float | |

### Sources
 - [Code Masters Forum Post](https://forums.codemasters.com/topic/16703-d-box-and-udp-telemetry-information/)
 - [Steam Forum Post](https://steamcommunity.com/app/310560/discussions/0/481115363869500839/)
 - [Scribd PDF](https://www.scribd.com/document/350826037/UDP-output-setup)