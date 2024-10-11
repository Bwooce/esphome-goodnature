#include "goodnature_ble_listener.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"

namespace esphome {
namespace goodnature_ble {

static const char *TAG = "goodnature_ble";

void GoodnatureBleListener::add_device(const std::string &name, const std::string &serial) {
  this->devices_[serial] = name;
}

bool GoodnatureBleListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {
  if (device.get_name() != "GN") {
    ESP_LOGE(TAG, "Not Goodnature device");
    return false;
  }

  ESP_LOGD(TAG, "Found Goodnature device: %s", device.address_str().c_str());

  auto services = device.get_service_datas();
  for (auto &service_data : services) {
    if (service_data.uuid.contains(0xD3, 0x0D)) {
      auto kill_info = service_data.data;
      parse_kill_info(kill_info);
      return true;
    }
  }

  return false;
}

void GoodnatureBleListener::parse_kill_info(const std::vector<unsigned char> &data) {
  if (data.size() < 22) {
    ESP_LOGE(TAG, "Invalid kill info data length");
    return;
  }

  // Extract serial number (positions 2-9, reversed)
  std::string serial(data.begin() + 2, data.begin() + 10);
  serial = reverse_serial(serial);
  
  // Extract kill count (position 20)
  char kill_count_char = data[20];
  kill_count_ = kill_count_char - '0';

  // Extract battery level (example: position 10, this might need adjustment)
  battery_level_ = static_cast<uint8_t>(data[10]);

  // Extract last activation timestamp (positions 11-14, this might need adjustment)
  std::vector<uint8_t> timestamp(data.begin()+11, data.begin()+15);
  last_activation_ = parse_timestamp(timestamp);

  // Calculate total activations (cumulative kill count, this might need adjustment)
  total_activations_ += kill_count_;

  last_seen_serial_ = serial;
  last_seen_device_ = devices_[serial];

  if (last_seen_device_.empty()) {
    last_seen_device_ = "Unknown";
  }

  ESP_LOGI(TAG, "Goodnature device: %s (Serial: %s), Kill count: %d, Battery: %d%%, Last activation: %u",
           last_seen_device_.c_str(), serial.c_str(), kill_count_, battery_level_, last_activation_);

  if (kill_count_sensor_ != nullptr) {
    kill_count_sensor_->publish_state(kill_count_);
  }

  if (battery_level_sensor_ != nullptr) {
    battery_level_sensor_->publish_state(battery_level_);
  }

  if (last_activation_sensor_ != nullptr) {
    last_activation_sensor_->publish_state(last_activation_);
  }

  if (total_activations_sensor_ != nullptr) {
    total_activations_sensor_->publish_state(total_activations_);
  }

  if (days_since_activation_sensor_ != nullptr) {
    time_t now;
    time(&now);
    float days_since = (now - last_activation_) / (24.0 * 60 * 60);
    days_since_activation_sensor_->publish_state(days_since);
  }
}

std::string GoodnatureBleListener::reverse_serial(const std::string &serial) {
  std::string reversed;
  for (int i = 6; i >= 0; i -= 2) {
    reversed += serial.substr(i, 2);
  }
  return reversed;
}

uint32_t GoodnatureBleListener::parse_timestamp(const std::vector<unsigned char> &data) {
  // This is a placeholder implementation. You might need to adjust this based on the actual data format.
  uint32_t timestamp = 0;
  for (int i = 0; i < 4; i++) {
    timestamp |= static_cast<uint32_t>(data[i]) << (8 * i);
  }
  return timestamp;
}

void GoodnatureBleListener::dump_config() {
  ESP_LOGCONFIG(TAG, "Goodnature BLE Listener:");
  ESP_LOGCONFIG(TAG, "  Registered devices:");
  for (const auto &device : devices_) {
    ESP_LOGCONFIG(TAG, "    %s: %s", device.second.c_str(), device.first.c_str());
  }
}

} // namespace goodnature_ble
} // namespace esphome