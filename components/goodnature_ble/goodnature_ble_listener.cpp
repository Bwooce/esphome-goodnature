#include "goodnature_ble_listener.h"
#include "esphome/core/log.h"
#include "esphome/core/time.h"

namespace esphome {
namespace goodnature_ble {

void print_buffer(const uint8_t *data, size_t length);


static const char *TAG = "goodnature_ble";

void GoodnatureBleListener::set_name(const std::string &name) {
  this->name_ = name;
}

bool GoodnatureBleListener::parse_device(const esp32_ble_tracker::ESPBTDevice &device) {

  if (strcmp(device.get_name().c_str(),"GN") != 0) {
    return false;
  }

  ESP_LOGD(TAG, "Found Goodnature device: %s", device.address_str().c_str());

  // Parse manufacturer data for iBeacon format
  auto mfg_datas = device.get_manufacturer_datas();
  for (auto &mfg_data : mfg_datas) {
    ESP_LOGD(TAG, "Manufacturer data UUID: %s, length: %zu",
             mfg_data.uuid.to_string().c_str(), mfg_data.data.size());

    // Check for Apple company ID (0x004C) and correct iBeacon length (23 bytes)
    if (mfg_data.uuid.to_string() == "0x004C" && mfg_data.data.size() == 23) {

      // For debugging: print the raw iBeacon data
      ESP_LOGD(TAG, "Found Apple manufacturer data (iBeacon):");
      print_buffer(&mfg_data.data[0], mfg_data.data.size());

      // Verify iBeacon prefix (0x02 0x15)
      if (mfg_data.data[0] != 0x02 || mfg_data.data[1] != 0x15) {
        ESP_LOGD(TAG, "Not iBeacon format (prefix mismatch)");
        continue;
      }

      // Verify Goodnature UUID: B0B0EEE7-B9B0-4BC5-B5E4-F5CB610EB700
      const uint8_t goodnature_uuid[16] = {
        0xB0, 0xB0, 0xEE, 0xE7, 0xB9, 0xB0, 0x4B, 0xC5,
        0xB5, 0xE4, 0xF5, 0xCB, 0x61, 0x0E, 0xB7, 0x00
      };

      if (memcmp(&mfg_data.data[2], goodnature_uuid, 16) != 0) {
        ESP_LOGD(TAG, "Not Goodnature UUID");
        continue;
      }

      // Extract kill count from byte 18 (0-based index 17)
      uint8_t kill_count = mfg_data.data[17];

      // Extract serial number from iBeacon Major and Minor
      // Major = bytes 18-19, Minor = bytes 20-21
      uint16_t major = (mfg_data.data[18] << 8) | mfg_data.data[19];
      uint16_t minor = (mfg_data.data[20] << 8) | mfg_data.data[21];

      // Construct serial: Minor (high) + Major (low)
      // Example: minor=0xE33B, major=0xC98F → serial=E33BC98F
      char serial_str[9];
      snprintf(serial_str, sizeof(serial_str), "%04X%04X", minor, major);
      this->serial_ = serial_str;

      // TX Power at byte 22 (optional, for RSSI calculations)
      int8_t tx_power = (int8_t)mfg_data.data[22];

      ESP_LOGI(TAG, "Goodnature iBeacon: %s (Serial: %s), Kill count: %d, TX Power: %d dBm",
               device.address_str().c_str(), this->serial_.c_str(), kill_count, tx_power);

      // If MAC address matches or not configured, process this device
      if (device.address_uint64() == this->mac_address_ || this->mac_address_ == 0) {
        this->kill_count_ = kill_count;

        if (kill_count_sensor_ != nullptr) {
          kill_count_sensor_->publish_state(this->kill_count_);
        }

        return true;
      } else {
        ESP_LOGD(TAG, "MAC address mismatch (configured: %llx, found: %llx)",
                 this->mac_address_, device.address_uint64());
      }
    }
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

// TEMP from weikai.cpp
 void print_buffer(const uint8_t *data, size_t length) {
   char hex_buffer[100];
   hex_buffer[(3 * 32) + 1] = 0;
   for (size_t i = 0; i < length; i++) {
     snprintf(&hex_buffer[3 * (i % 32)], sizeof(hex_buffer), "%02X ", data[i]);
     if (i % 32 == 31) {
       ESP_LOGD(TAG, "   %s", hex_buffer);
     }
   }
   if (length % 32) {
     // null terminate if incomplete line
     hex_buffer[3 * (length % 32) + 2] = 0;
     ESP_LOGD(TAG, "   %s", hex_buffer);
   }
 }

} // namespace goodnature_ble
} // namespace esphome