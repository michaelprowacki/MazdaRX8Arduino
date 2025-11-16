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
 * These specification were pieced together through multiple sources
 * https://forums.codemasters.com/topic/16703-d-box-and-udp-telemetry-information/
 * https://steamcommunity.com/app/310560/discussions/0/481115363869500839/
 * https://www.scribd.com/document/350826037/UDP-output-setup
 *
 * More details are provided in the readme
 */

 typedef struct DirtData_s {
    float time;
    float lap_time;
    float lap_distance;
    float total_distance;
    float x; // World space position
    float y; // World space position
    float z; // World space position
    float speed;
    float xv; // Velocity in world space
    float yv; // Velocity in world space
    float zv; // Velocity in world space
    float xr; // World space right direction
    float yr; // World space right direction
    float zr; // World space right direction
    float xd; // World space forward direction
    float yd; // World space forward direction
    float zd; // World space forward direction
    float susp_pos_rl;
    float susp_pos_rr;
    float susp_pos_fl;
    float susp_pos_fr;
    float susp_vel_rl;
    float susp_vel_rr;
    float susp_vel_fl;
    float susp_vel_fr;
    float wheel_speed_rl;
    float wheel_speed_rr;
    float wheel_speed_fl;
    float wheel_speed_fr;
    float throttle;
    float steer;
    float brake;
    float clutch;
    float gear;
    float gforce_lat;
    float gforce_lon;
    float lap;
    float engine_rate;
    float sli_pro_native_support; // SLI Pro support
    float car_position; // car race position
    float kers_level; // kers energy left
    float kers_max_level; // kers maximum energy
    float drs; // 0 = off, 1 = on
    float traction_control; // 0 (off) - 2 (high)
    float anti_lock_brakes; // 0 (off) - 1 (on)
    float fuel_in_tank; // current fuel mass
    float fuel_capacity; // fuel capacity
    float in_pits; // 0 = none, 1 = pitting, 2 = in pit area
    float sector; // 0 = sector1, 1 = sector2; 2 = sector3
    float sector1_time; // time of sector1 (or 0)
    float sector2_time; // time of sector2 (or 0)
    float brakes_temp_rl; // brakes temperature (centigrade)
    float brakes_temp_rr; // brakes temperature (centigrade)
    float brakes_temp_fl; // brakes temperature (centigrade)
    float brakes_temp_fr; // brakes temperature (centigrade)
    float wheels_pressure_rl; // wheels pressure PSI
    float wheels_pressure_rr; // wheels pressure PSI
    float wheels_pressure_fl; // wheels pressure PSI
    float wheels_pressure_fr; // wheels pressure PSI
    float team_info; // team ID
    float total_laps; // total number of laps in this race
    float track_size; // track size meters
    float last_lap_time; // last lap time
    float max_rpm; // cars max RPM, at which point the rev limiter will kick in
    float idle_rpm; // cars idle RPM
    float max_gears; // maximum number of gears
    float session_type; // 0 = unknown, 1 = practice, 2 = qualifying, 3 = race
    float drs_allowed; // 0 = not allowed, 1 = allowed, -1 = invalid / unknown
    float track_number; // -1 for unknown, 0-21 for tracks
    float vehicle_fia_flags; // -1 = invalid/unknown, 0 = none, 1 = green, 2 = blue, 3 = yellow, 4 = red   
} DirtData_t;
