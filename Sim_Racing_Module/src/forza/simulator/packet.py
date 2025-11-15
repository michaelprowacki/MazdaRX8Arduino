import struct

from dataclasses import dataclass, astuple


@dataclass
class DashPacket:
    # V1 'sled'
    is_race_on: int = 0

    timestamp_m_s: int = 0

    engine_max_rpm: float = 0.0
    engine_idle_rpm: float = 0.0
    current_engine_rpm: float = 0.0

    acceleration_x: float = 0.0
    acceleration_y: float = 0.0
    acceleration_z: float = 0.0

    velocity_x: float = 0.0
    velocity_y: float = 0.0
    velocity_z: float = 0.0

    angular_velocity_x: float = 0.0
    angular_velocity_y: float = 0.0
    angular_velocity_z: float = 0.0

    yaw: float = 0.0
    pitch: float = 0.0
    roll: float = 0.0

    normalized_suspension_travel_front_left: float = 0.0
    normalized_suspension_travel_front_right: float = 0.0
    normalized_suspension_travel_rear_left: float = 0.0
    normalized_suspension_travel_rear_right: float = 0.0

    tire_slip_ratio_front_left: float = 0.0
    tire_slip_ratio_front_right: float = 0.0
    tire_slip_ratio_rear_left: float = 0.0
    tire_slip_ratio_rear_right: float = 0.0

    wheel_rotation_speed_front_left: float = 0.0
    wheel_rotation_speed_front_right: float = 0.0
    wheel_rotation_speed_rear_left: float = 0.0
    wheel_rotation_speed_rear_right: float = 0.0

    wheel_on_rumble_strip_front_left: float = 0.0
    wheel_on_rumble_strip_front_right: float = 0.0
    wheel_on_rumble_strip_rear_left: float = 0.0
    wheel_on_rumble_strip_rear_right: float = 0.0

    wheel_in_puddle_depth_front_left: float = 0.0
    wheel_in_puddle_depth_front_right: float = 0.0
    wheel_in_puddle_depth_rear_left: float = 0.0
    wheel_in_puddle_depth_rear_right: float = 0.0

    surface_rumble_front_left: float = 0.0
    surface_rumble_front_right: float = 0.0
    surface_rumble_rear_left: float = 0.0
    surface_rumble_rear_right: float = 0.0

    tire_slip_angle_front_left: float = 0.0
    tire_slip_angle_front_right: float = 0.0
    tire_slip_angle_rear_left: float = 0.0
    tire_slip_angle_rear_right: float = 0.0

    tire_combined_slip_front_left: float = 0.0
    tire_combined_slip_front_right: float = 0.0
    tire_combined_slip_rear_left: float = 0.0
    tire_combined_slip_rear_right: float = 0.0

    suspension_travel_meters_front_left: float = 0.0
    suspension_travel_meters_front_right: float = 0.0
    suspension_travel_meters_rear_left: float = 0.0
    suspension_travel_meters_rear_right: float = 0.0

    car_ordinal: int = 0
    car_class: int = 0
    car_performance_index: int = 0
    drivetrain_type: int = 0
    num_cylinders: int = 0

    # V1 and V2 of this protocol is separated by 12 bytes offset
    offset0: float = 0.0
    offset1: float = 0.0
    offset2: float = 0.0

    # V2 'car dash'
    position_x: float = 0.0
    position_y: float = 0.0
    position_z: float = 0.0

    speed: float = 0.0
    power: float = 0.0
    torque: float = 0.0

    tire_temp_front_left: float = 0.0
    tire_temp_front_right: float = 0.0
    tire_temp_rear_left: float = 0.0
    tire_temp_rear_right: float = 0.0

    boost: float = 0.0
    fuel: float = 0.0
    distance_traveled: float = 0.0
    best_lap: float = 0.0
    last_lap: float = 0.0
    current_lap: float = 0.0
    current_race_time: float = 0.0

    lap_number: int = 0
    race_position: int = 0

    accel: int = 0
    brake: int = 0
    clutch: int = 0
    hand_brake: int = 0
    gear: int = 0
    steer: int = 0

    normalized_driving_line: int = 0
    normalized_a_i_brake_difference: int = 0

    # convenience to positionally reference index of unpacked data
    labels = ['is_race_on',
              'timestamp_m_s',
              'engine_max_rpm',
              'engine_idle_rpm',
              'current_engine_rpm',
              'acceleration_x',
              'acceleration_y',
              'acceleration_z',
              'velocity_x',
              'velocity_y',
              'velocity_z',
              'angular_velocity_x',
              'angular_velocity_y',
              'angular_velocity_z',
              'yaw',
              'pitch',
              'roll',
              'normalized_suspension_travel_front_left',
              'normalized_suspension_travel_front_right',
              'normalized_suspension_travel_rear_left',
              'normalized_suspension_travel_rear_right',
              'tire_slip_ratio_front_left',
              'tire_slip_ratio_front_right',
              'tire_slip_ratio_rear_left',
              'tire_slip_ratio_rear_right',
              'wheel_rotation_speed_front_left',
              'wheel_rotation_speed_front_right',
              'wheel_rotation_speed_rear_left',
              'wheel_rotation_speed_rear_right',
              'wheel_on_rumble_strip_front_left',
              'wheel_on_rumble_strip_front_right',
              'wheel_on_rumble_strip_rear_left',
              'wheel_on_rumble_strip_rear_right',
              'wheel_in_puddle_depth_front_left',
              'wheel_in_puddle_depth_front_right',
              'wheel_in_puddle_depth_rear_left',
              'wheel_in_puddle_depth_rear_right',
              'surface_rumble_front_left',
              'surface_rumble_front_right',
              'surface_rumble_rear_left',
              'surface_rumble_rear_right',
              'tire_slip_angle_front_left',
              'tire_slip_angle_front_right',
              'tire_slip_angle_rear_left',
              'tire_slip_angle_rear_right',
              'tire_combined_slip_front_left',
              'tire_combined_slip_front_right',
              'tire_combined_slip_rear_left',
              'tire_combined_slip_rear_right',
              'suspension_travel_meters_front_left',
              'suspension_travel_meters_front_right',
              'suspension_travel_meters_rear_left',
              'suspension_travel_meters_rear_right',
              'car_ordinal',
              'car_class',
              'car_performance_index',
              'drivetrain_type',
              'num_cylinders',
              'position_x',
              'position_y',
              'position_z',
              'offset0',
              'offset1',
              'offset2',
              'speed',
              'power',
              'torque',
              'tire_temp_front_left',
              'tire_temp_front_right',
              'tire_temp_rear_left',
              'tire_temp_rear_right',
              'boost',
              'fuel',
              'distance_traveled',
              'best_lap',
              'last_lap',
              'current_lap',
              'current_race_time',
              'lap_number',
              'race_position',
              'accel',
              'brake',
              'clutch',
              'hand_brake',
              'gear',
              'steer',
              'normalized_driving_line',
              'normalized_a_i_brake_difference']

    def __str__(self):
        return "current_engine_rpm: {rpm:.2f}, speed: {speed:.2f}".format(rpm=self.current_engine_rpm, speed=self.speed)

    def to_tuple(self):
        return astuple(self)


class DashCodec:
    format: str = '<iIfffffffffffffffffffffffffffffffffffffffffffffffffffiiiiiffffffffffffffffffffHBBBBBBbbb'

    def buffer_size(self):
        return struct.calcsize(self.format)

    def unpack(self, data) -> DashPacket:
        data = struct.unpack(self.format, data)

        # only care about this data (for now)
        on = data[DashPacket.labels.index('is_race_on')]
        rpm = data[DashPacket.labels.index('current_engine_rpm')]
        speed = data[DashPacket.labels.index('speed')]

        return DashPacket(is_race_on=on, current_engine_rpm=rpm, speed=speed)

    def pack(self, *data) -> DashPacket:
        return struct.pack(self.format, *data)
