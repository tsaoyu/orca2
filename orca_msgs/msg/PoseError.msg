# Diagnostic message for evaluating path errors

# Header
std_msgs/Header header

# Planned pose
orca_msgs/Pose plan

# Estimated pose
orca_msgs/Pose estimate

# Error (estimate - plan)
orca_msgs/Pose error

# Sum of squared errors
orca_msgs/Pose sse
int64 steps

# RMS
orca_msgs/Pose rms
