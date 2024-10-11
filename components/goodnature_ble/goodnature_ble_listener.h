#pragma once
#include <map>
#include "esphome/core/component.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace goodnature_ble {

class GoodnatureBleListener : public esp32_ble_tracker::ESPBTDeviceListener, public Component {
 public:
  void set_kill_count_sensor(sensor::Sensor *kill_count_sensor) { kill_count_sensor_ = kill_count_sensor; }
  void set_battery_level_sensor(sensor::Sensor *battery_level_sensor) { battery_level_sensor_ = battery_level_sensor; }
  void set_last_activation_sensor(sensor::Sensor *last_activation_sensor) { last_activation_sensor_ = last_activation_sensor; }
  void set_total_activations_sensor(sensor::Sensor *total_activations_sensor) { total_activations_sensor_ = total_activations_sensor; }
  void set_days_since_activation_sensor(sensor::Sensor *days_since_activation_sensor) { days_since_activation_sensor_ = days_since_activation_sensor; }
  void add_device(const std::string &name, const std::string &serial);
  bool parse_device(const esp32_ble_tracker::ESPBTDevice &device) override;
  void dump_config() override;

 protected:
  void parse_kill_info(const std::vector<unsigned char> &data);
  std::string reverse_serial(const std::string &serial);
  uint32_t parse_timestamp(const std::vector<unsigned char> &data);

 private:
  sensor::Sensor *kill_count_sensor_{nullptr};
  sensor::Sensor *battery_level_sensor_{nullptr};
  sensor::Sensor *last_activation_sensor_{nullptr};
  sensor::Sensor *total_activations_sensor_{nullptr};
  sensor::Sensor *days_since_activation_sensor_{nullptr};
  std::map<std::string, std::string> devices_;
  int kill_count_ = 0;
  int battery_level_ = 0;
  uint32_t last_activation_ = 0;
  int total_activations_ = 0;
  std::string last_seen_device_;
  std::string last_seen_serial_;
};

} // namespace goodnature_ble
} // namespace esphome