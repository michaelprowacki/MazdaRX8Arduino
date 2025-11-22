import struct

from dataclasses import dataclass, astuple

@dataclass
class DashPacket:
    time: float = 0.0
    lap_time: float = 0.0
    lap_distance: float = 0.0
    total_distance: float = 0.0
    x: float = 0.0  # World space position
    y: float = 0.0  # World space position
    z: float = 0.0  # World space position
    speed: float = 0.0
    xv: float = 0.0  # Velocity in world space
    yv: float = 0.0  # Velocity in world space
    zv: float = 0.0  # Velocity in world space
    xr: float = 0.0  # World space right direction
    yr: float = 0.0  # World space right direction
    zr: float = 0.0  # World space right direction
    xd: float = 0.0  # World space forward direction
    yd: float = 0.0  # World space forward direction
    zd: float = 0.0  # World space forward direction
    susp_pos_rl: float = 0.0
    susp_pos_rr: float = 0.0
    susp_pos_fl: float = 0.0
    susp_pos_fr: float = 0.0
    susp_vel_rl: float = 0.0
    susp_vel_rr: float = 0.0
    susp_vel_fl: float = 0.0
    susp_vel_fr: float = 0.0
    wheel_speed_rl: float = 0.0
    wheel_speed_rr: float = 0.0
    wheel_speed_fl: float = 0.0
    wheel_speed_fr: float = 0.0
    throttle: float = 0.0  # 29
    steer: float = 0.0
    brake: float = 0.0
    clutch: float = 0.0
    gear: float = 0.0
    gforce_lat: float = 0.0
    gforce_lon: float = 0.0
    lap: float = 0.0
    engine_rate: float = 0.0  # 37
    sli_pro_native_support: float = 0.0  # SLI Pro support
    car_position: float = 0.0  # car race position
    kers_level: float = 0.0  # kers energy left
    kers_max_level: float = 0.0  # kers maximum energy
    drs: float = 0.0  # 0 = off, 1 = on
    traction_control: float = 0.0  # 0 (off) - 2 (high)
    anti_lock_brakes: float = 0.0  # 0 (off) - 1 (on)
    fuel_in_tank: float = 0.0  # current fuel mass
    fuel_capacity: float = 0.0  # fuel capacity
    in_pits: float = 0.0  # 0 = none, 1 = pitting, 2 = in pit area
    sector: float = 0.0  # 0 = sector1, 1 = sector2: float = 0.0 2 = sector3
    sector1_time: float = 0.0  # time of sector1 (or 0)
    sector2_time: float = 0.0  # time of sector2 (or 0) # 50
    # brakes_temp[4]: float = 0.0  # brakes temperature (centigrade)
    brakes_temp_rl: float = 0.0  # brakes temperature (centigrade)
    brakes_temp_rr: float = 0.0  # brakes temperature (centigrade)
    brakes_temp_fl: float = 0.0  # brakes temperature (centigrade)
    brakes_temp_fr: float = 0.0  # brakes temperature (centigrade)
    wheels_pressure_rl: float = 0.0  # wheels pressure PSI
    wheels_pressure_rr: float = 0.0  # wheels pressure PSI
    wheels_pressure_fl: float = 0.0  # wheels pressure PSI
    wheels_pressure_fr: float = 0.0  # wheels pressure PSI
    team_info: float = 0.0  # team ID
    total_laps: float = 0.0  # total number of laps in this race
    track_size: float = 0.0  # track size meters
    last_lap_time: float = 0.0  # last lap time
    max_rpm: float = 0.0  # cars max RPM, at which point the rev limiter will kick in
    idle_rpm: float = 0.0  # cars idle RPM
    max_gears: float = 0.0  # maximum number of gears
    session_type: float = 0.0  # 0 = unknown, 1 = practice, 2 = qualifying, 3 = race
    drs_allowed: float = 0.0  # 0 = not allowed, 1 = allowed, -1 = invalid / unknown
    track_number: float = 0.0  # -1 for unknown, 0-21 for tracks
    # -1 = invalid/unknown, 0 = none, 1 = green, 2 = blue, 3 = yellow, 4 = red
    vehicle_fia_flags: float = 0.0

    def __str__(self):
        return "RPM: {rpm:.2f} Speed: {speed:.2f}".format(rpm=(self.engine_rate * 10), speed=(self.speed * 3.6))

    def to_tuple(self):
        return astuple(self)

class DashCodec:
    # TODO: how many floats are expected 60 or 70?
    format: str = '<ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff'

    def buffer_size(self):
        return struct.calcsize(self.format)

    def unpack(self, data) -> DashPacket:
        data = struct.unpack(self.format, data)

        # only care about this data (for now)
        speed = data[7]
        rpm = data[37]

        return DashPacket(engine_rate=rpm, speed=speed)

    def pack(self, *data) -> DashPacket:
        return struct.pack(self.format, *data)
