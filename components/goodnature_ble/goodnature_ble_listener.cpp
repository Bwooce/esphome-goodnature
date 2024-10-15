#include "goodnature_ble_listener.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"

namespace esphome {
namespace goodnature_ble {

static const char *TAG = "goodnature_ble";

void GoodnatureBleListener::set_name(const std::string &name) {
  this->name_ = name;
}

bool GoodnatureBleListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {

  if (strcmp(device.get_name().c_str(),"GN") != 0) {
    //ESP_LOGE(TAG, "Not Goodnature device, got %s", device.get_name().c_str());
    return false;
  }

  ESP_LOGD(TAG, "Found Goodnature device: %s", device.address_str().c_str());

  auto mfg_datas = device.get_manufacturer_datas();
  if (mfg_datas.empty()) {
    ESP_LOGE(TAG, "parse_device(): no mfg data");
  }
  if (mfg_datas.size() > 0) {
    ESP_LOGD(TAG, "mfg data");
    for (auto data : mfg_datas) {
      ESP_LOGW(TAG, " mfg adv datas - %s: (length %i)", data.uuid.to_string().c_str(), data.data.size());
      //ESP_LOG_BUFFER_HEX_LEVEL(TAG, &data.data[0], data.data.size(), ESP_LOG_ERROR);
      std::string hex;
      for(int i=0;i<data.data.size();i++) {
        hex << data[i];
      }
      ESP_LOGW(TAG, "DATA: %s",hex.c_str());
    }
  }

  auto services = device.get_service_datas();
  for (auto &service_data : services) {
    ESP_LOGD(TAG, "service data");
    //if (service_data.uuid.contains(0xD3, 0x0D)) {
    //  auto kill_info = service_data.data;
    //  parse_kill_info(device.address_uint64(), kill_info);
    //  return true;
    //}
    ESP_LOGW(TAG, " mfg svc datas - %s: (length %i)", service_data.uuid.to_string().c_str(), service_data.data.size());
    //ESP_LOG_BUFFER_HEX_LEVEL(TAG, &service_data.data[0], service_data.data.size(), ESP_LOG_ERROR);
  }

  return false;
}

void GoodnatureBleListener::parse_kill_info(uint64_t address, const std::vector<unsigned char> &data) {
  if (data.size() < 22) {
    ESP_LOGE(TAG, "Invalid kill info data length");
    return;
  }

  // Extract serial number (positions 2-9, reversed)
  std::string serial(data.begin() + 2, data.begin() + 10);
  serial = reverse_serial(serial);

  // if a mac address is configured then use it, otherwise just try this one
  if(address == this->mac_address_ || this->mac_address_ == 0) {
    
    // Extract kill count (position 20)
    char kill_count_char = data[20];
    this->kill_count_ = kill_count_char - '0';

    // Extract battery level (example: position 10, this might need adjustment)
    this->battery_level_ = static_cast<uint8_t>(data[10]);

    // Extract last activation timestamp (positions 11-14, this might need adjustment)
    std::vector<uint8_t> timestamp(data.begin()+11, data.begin()+15);
    this->last_activation_ = parse_timestamp(timestamp);

    ESP_LOGI(TAG, "Goodnature device: %x (Serial: %s), Kill count: %d, Battery: %d%%, Last activation: %u",
            this->mac_address_, this->serial_.c_str(), this->kill_count_, this->battery_level_, this->last_activation_);

    if (kill_count_sensor_ != nullptr) {
      kill_count_sensor_->publish_state(this->kill_count_);
    }

    if (battery_level_sensor_ != nullptr) {
      battery_level_sensor_->publish_state(this->battery_level_);
    }

    if (last_activation_sensor_ != nullptr) {
      last_activation_sensor_->publish_state(this->last_activation_);
    }
  }
  this->last_seen_serial_ = serial;
  this->last_seen_mac_address_ = address;

  if (this->last_seen_mac_address_ == 0) {
    this->last_seen_serial_ = "Unknown";
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
  ESP_LOGCONFIG(TAG, "Goodnature BLE");
  LOG_SENSOR("  ", "Kill Count", kill_count_sensor_);
  LOG_SENSOR("  ", "Battery Level", battery_level_sensor_);
  LOG_SENSOR("  ", "Last Activation", last_activation_sensor_);
}

} // namespace goodnature_ble
} // namespace esphome