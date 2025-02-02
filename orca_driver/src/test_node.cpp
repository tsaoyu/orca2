#include "rclcpp/rclcpp.hpp"

#include "orca_msgs/msg/control.hpp"

namespace orca_driver
{
  //=============================================================================
  // TestNode
  // Test Orca hardware by sending control messages
  //=============================================================================

  class TestNode : public rclcpp::Node
  {
    const int THRUST_DIFF = 50;

    rclcpp::TimerBase::SharedPtr spin_timer_;
    rclcpp::Publisher<orca_msgs::msg::Control>::SharedPtr control_pub_;

    void spin_once()
    {
      orca_msgs::msg::Control msg;
      msg.header.stamp = now();
      msg.stability = 1.0;
      msg.odom_lag = 0.0;
      msg.mode = msg.MANUAL;
      msg.camera_tilt_pwm = msg.TILT_0;
      msg.brightness_pwm = msg.LIGHTS_OFF;

      // Rotate through all 6 thrusters, and send fwd and rev signals
      // Each thruster gets 5s, so a cycle is 30s
      int cycle = msg.header.stamp.sec % 30;
      int thruster = cycle / 5;
      int pwm;
      switch (cycle % 5) {
        case 1:
          pwm = msg.THRUST_STOP + THRUST_DIFF;
          break;
        case 3:
          pwm = msg.THRUST_STOP - THRUST_DIFF;
          break;
        default:
          pwm = msg.THRUST_STOP;
          break;
      }

      for (int i = 0; i < 6; ++i) {
        msg.thruster_pwm.push_back(i == thruster ? pwm : msg.THRUST_STOP);
      }

      control_pub_->publish(msg);
    }

  public:
    explicit TestNode() :
      Node{"self_test"}
    {
      // Suppress IDE warning
      (void) spin_timer_;

      // Publish control messages
      control_pub_ = create_publisher<orca_msgs::msg::Control>("control", 1);

      // Spin timer
      using namespace std::chrono_literals;
      spin_timer_ = create_wall_timer(100ms, std::bind(&TestNode::spin_once, this));
    }

    ~TestNode()
    {
    }
  };

} // namespace orca_driver

//=============================================================================
// Main
//=============================================================================

int main(int argc, char **argv)
{
  // Force flush of the stdout buffer
  setvbuf(stdout, NULL, _IONBF, BUFSIZ);

  // Init ROS
  rclcpp::init(argc, argv);

  // Init node
  auto node = std::make_shared<orca_driver::TestNode>();

  // Spin
  rclcpp::spin(node);

  // Shut down ROS
  rclcpp::shutdown();

  return 0;
}
