# One Livox MID-360 point-cloud UDP payload (compact raw form for rosbag recording)

uint8 data_type
uint16 dot_num
uint16 time_interval
uint16 udp_cnt
uint8 frame_cnt
uint8 time_type
uint8[8] livox_timestamp
uint8[] data
