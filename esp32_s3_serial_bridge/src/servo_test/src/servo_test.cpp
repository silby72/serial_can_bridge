#include <algorithm>
#include <atomic>
#include <chrono>
#include <iostream>
#include <limits>
#include <mutex>
#include <thread>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include <std_msgs/msg/int16_multi_array.hpp>

// 以前の構成に合わせ、配列数は24
constexpr size_t TX16NUM = 24; 

class ServoKeyboardHandler {
public:
    ServoKeyboardHandler(rclcpp::Node::SharedPtr node, uint8_t device_id)
        : node_(node), running_(true), device_id_(device_id) {

        // トピック名は以前の定義[cite: 5]に従い serial_tx_{device_id}
        publisher_ = node_->create_publisher<std_msgs::msg::Int16MultiArray>(
            "serial_tx_" + std::to_string(device_id_), 10);

        data_.assign(TX16NUM, 0);

        // 100ms周期で常時パブリッシュ[cite: 6]
        timer_ = node_->create_wall_timer(
            std::chrono::milliseconds(100),
            std::bind(&ServoKeyboardHandler::timer_callback, this));

        input_thread_ = std::thread(&ServoKeyboardHandler::keyboard_input_loop, this);
    }

    ~ServoKeyboardHandler() {
        running_ = false;
        if (input_thread_.joinable()) {
            input_thread_.join();
        }
    }

private:
    void keyboard_input_loop() {
        while (running_) {
            int index, value;
            std::cout << "\n--- Servo Control ---" << std::endl;
            std::cout << "Target Index: 9 (Servo1) or 10 (Servo2)" << std::endl;
            std::cout << "Enter index (0-23) and value (0-180): ";

            if (!(std::cin >> index >> value)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                continue;
            }

            if (index < 0 || index >= static_cast<int>(TX16NUM)) {
                std::cerr << "Error: Index out of range!" << std::endl;
                continue;
            }

            {
                std::lock_guard<std::mutex> lock(data_mutex_);
                data_[index] = static_cast<int16_t>(value);
            }
            
            std::cout << ">> Set index [" << index << "] to " << value << std::endl;
        }
    }

    void timer_callback() {
        std_msgs::msg::Int16MultiArray msg;
        {
            std::lock_guard<std::mutex> lock(data_mutex_);
            msg.data = data_;
        }
        publisher_->publish(msg);
    }

    rclcpp::Node::SharedPtr node_;
    rclcpp::Publisher<std_msgs::msg::Int16MultiArray>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;

    std::vector<int16_t> data_;
    std::mutex data_mutex_;
    std::thread input_thread_;
    std::atomic<bool> running_;
    uint8_t device_id_;
};

int main(int argc, char *argv[]) {
    rclcpp::init(argc, argv);
    auto node = rclcpp::Node::make_shared("servo_keyboard_tester");

    // masterのデバイスIDに合わせる（例として 1 に設定）[cite: 5]
    uint8_t device_id = 1; 
    
    ServoKeyboardHandler handler(node, device_id);

    RCLCPP_INFO(node->get_logger(), "Publishing to serial_tx_%d. Use index 9/10 for Servos.", device_id);

    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}