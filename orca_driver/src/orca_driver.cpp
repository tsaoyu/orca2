#include "orca_driver/orca_driver.hpp"

using std::placeholders::_1;

namespace orca_driver {

// Message publish rate in Hz
constexpr int SPIN_RATE = 50;

OrcaDriver::OrcaDriver():
  Node{"orca_driver"}
{
  // TODO parameters
#if 0
  nh_priv_.param<std::string>("maestro_port", maestro_port_, "/dev/ttyACM0");
  RCLCPP_INFO(get_logger(), "Expecting Maestro on port %s", maestro_port_.c_str());

  nh_priv_.param("num_thrusters", num_thrusters_, 6);
  RCLCPP_INFO(get_logger(), "Configuring for %d thrusters:", num_thrusters_);
  for (int i = 0; i < num_thrusters_; ++i)
  {
    Thruster t;
    nh_priv_.param("thruster_" + std::to_string(i + 1) + "_channel", t.channel_, i); // No checks for channel conflicts!
    nh_priv_.param("thruster_" + std::to_string(i + 1) + "_reverse", t.reverse_, false);
    thrusters_.push_back(t);
    RCLCPP_INFO(get_logger(), "Thruster %d on channel %d %s", i + 1, t.channel_, t.reverse_ ? "(reversed)" : "");
  }

  nh_priv_.param("lights_channel", lights_channel_, 8);
  RCLCPP_INFO(get_logger(), "Lights on channel %d", lights_channel_);

  nh_priv_.param("tilt_channel", tilt_channel_, 9);
  RCLCPP_INFO(get_logger(), "Camera servo on channel %d", tilt_channel_);

  nh_priv_.param("voltage_channel", voltage_channel_, 11); // Must be analog input
  nh_priv_.param("voltage_multiplier", voltage_multiplier_, 4.7);
  nh_priv_.param("voltage_min", voltage_min_, 12.0);
  RCLCPP_INFO(get_logger(), "Voltage sensor on channel %d, multiplier is %g, minimum is %g", voltage_channel_, voltage_multiplier_, voltage_min_);

  nh_priv_.param("leak_channel", leak_channel_, 12); // Must be digital input
  RCLCPP_INFO(get_logger(), "Leak sensor on channel %d", leak_channel_);
#endif

  // Set up subscription
  control_sub_ = create_subscription<orca_msgs::msg::Control>("/orca_base/control", std::bind(&OrcaDriver::controlCallback, this, _1));
  
  // Advertise topics that we'll publish on
  battery_pub_ = create_publisher<orca_msgs::msg::Battery>("battery", 1);
  leak_pub_ = create_publisher<orca_msgs::msg::Leak>("leak", 1);
}

void OrcaDriver::controlCallback(const orca_msgs::msg::Control::SharedPtr msg)
{
  if (maestro_.ready())
  {
    maestro_.setPWM(static_cast<uint8_t>(tilt_channel_), msg->camera_tilt_pwm);
    maestro_.setPWM(static_cast<uint8_t>(lights_channel_), msg->brightness_pwm);

    for (int i = 0; i < thrusters_.size(); ++i)
    {
      uint16_t pwm = msg->thruster_pwm[i];

      // Compensate for ESC programming errors
      if (thrusters_[i].reverse_) pwm = static_cast<uint16_t>(3000 - pwm);

      maestro_.setPWM(static_cast<uint8_t>(thrusters_[i].channel_), pwm);
    }
  }
  else
  {
    RCLCPP_ERROR(get_logger(), "Maestro not ready, ignoring control message");
  }
}

bool OrcaDriver::readBattery()
{
  battery_msg_.header.stamp = now();

  double value;
  if (maestro_.ready() && maestro_.getAnalog(static_cast<uint8_t>(voltage_channel_), value))
  {
    battery_msg_.voltage = value * voltage_multiplier_;
    battery_msg_.low_battery = static_cast<uint8_t>(battery_msg_.voltage < voltage_min_);
    return true;
  }
  else
  {
    RCLCPP_ERROR(get_logger(), "Can't read battery");
    battery_msg_.voltage = 0;
    battery_msg_.low_battery = 1;
    return false;  
  }
}

bool OrcaDriver::readLeak()
{
  leak_msg_.header.stamp = now();

  bool value;
  if (maestro_.ready() && maestro_.getDigital(static_cast<uint8_t>(leak_channel_), value))
  {
    leak_msg_.leak_detected = static_cast<uint8_t>(value);
    return true;
  }
  else
  {
    RCLCPP_ERROR(get_logger(), "Can't read leak sensor");
    leak_msg_.leak_detected = 1;
    return false;  
  }
}

void OrcaDriver::spinOnce()
{
  battery_pub_->publish(battery_msg_);
  leak_pub_->publish(leak_msg_);
}

// Run a bunch of pre-dive checks, return true if everything looks good
bool OrcaDriver::preDive()
{
  RCLCPP_INFO(get_logger(), "Running pre-dive checks...");

  if (!readBattery() || !readLeak())
  {
    maestro_.disconnect();
    return false;
  }

  RCLCPP_INFO(get_logger(), "Voltage is %g, leak status is %d", battery_msg_.voltage, leak_msg_.leak_detected);

  if (leak_msg_.leak_detected)
  {
    RCLCPP_ERROR(get_logger(), "Leak detected");
    maestro_.disconnect();
    return false;
  }

  if (battery_msg_.voltage < voltage_min_)
  {
    RCLCPP_ERROR(get_logger(), "Battery voltage %g is below minimum %g", battery_msg_.voltage, voltage_min_);
    maestro_.disconnect();
    return false;
  }

  // When the Maestro boots, it should set all thruster channels to 1500.
  // But on a system restart it might be a bad state. Force an all-stop.
  for (int i = 0; i < thrusters_.size(); ++i)
  {
    maestro_.setPWM(static_cast<uint8_t>(thrusters_[i].channel_), 1500);
  }

  // Check to see that all thrusters are stopped.
  for (int i = 0; i < thrusters_.size(); ++i)
  {
    uint16_t value;
    maestro_.getPWM(static_cast<uint8_t>(thrusters_[i].channel_), value);
    RCLCPP_INFO(get_logger(), "Thruster %d is set at %d", i + 1, value);
    if (value != 1500)
    {
      RCLCPP_ERROR(get_logger(), "Thruster %d didn't initialize properly (and possibly others)", i + 1);
      maestro_.disconnect();
      return false;
    }
  }

  RCLCPP_INFO(get_logger(), "Pre-dive checks passed");
  return true;
}

// Connect to Maestro
bool OrcaDriver::connect()
{
  std::string port = maestro_port_;
  RCLCPP_INFO(get_logger(), "Opening port %s...", port.c_str());
  maestro_.connect(port);
  if (!maestro_.ready())
  {
    RCLCPP_ERROR(get_logger(), "Can't open port %s, are you root?", port.c_str());
    return false;
  }
  RCLCPP_INFO(get_logger(), "Port %s open", port.c_str());

  return preDive();
}

void OrcaDriver::disconnect()
{
  maestro_.disconnect();
}

} // namespace orca_driver

int main(int argc, char **argv)
{
  // Force flush of the stdout buffer
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Init ROS
  rclcpp::init(argc, argv);

  // Init node
  auto node = std::make_shared<orca_driver::OrcaDriver>();
  if (node->connect())
  {
    RCLCPP_INFO(node->get_logger(), "Entering main loop");
    rclcpp::Rate r(orca_driver::SPIN_RATE);

    while (rclcpp::ok())
    {
      // Do our work
      node->spinOnce();

      // Respond to incoming messages
      rclcpp::spin_some(node);

      // Wait
      r.sleep();
    }
  }

  node->disconnect();
  rclcpp::shutdown();
  return 0;
}
