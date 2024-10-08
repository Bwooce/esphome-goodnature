#include "esphome/core/log.h"
#include "../goodnature_ble_listener.h"
#include "esphome/components/esp32_ble_tracker/esp32_ble_tracker.h"

#include <unity.h>

using namespace esphome;
using namespace esphome::goodnature_ble;

// Mock sensor class for testing
class MockSensor : public sensor::Sensor {
public:
    float last_state;
    void publish_state(float state) override { last_state = state; }
};

void test_parse_kill_info() {
    GoodnatureBleListener listener;
    MockSensor kill_count_sensor;
    MockSensor battery_level_sensor;
    MockSensor last_activation_sensor;
    MockSensor total_activations_sensor;
    MockSensor days_since_activation_sensor;

    listener.set_kill_count_sensor(&kill_count_sensor);
    listener.set_battery_level_sensor(&battery_level_sensor);
    listener.set_last_activation_sensor(&last_activation_sensor);
    listener.set_total_activations_sensor(&total_activations_sensor);
    listener.set_days_since_activation_sensor(&days_since_activation_sensor);

    // Mock kill info data
    // Format: AAbbbbbbbbCdddEEEEEEEfGG
    // AA: Unknown value
    // bbbbbbbb: Serial number (reversed)
    // C: Unknown value
    // ddd: Unknown value
    // EEEEEEE: Unknown value
    // f: Kill count
    // GG: Unknown value
    std::string mock_data = "7700FEDCBA1a87bda3010600";

    esp32_ble_tracker::ESPBTDevice mock_device;
    mock_device.set_name("GN");
    esp32_ble_tracker::ServiceData mock_service_data;
    mock_service_data.uuid = esp32_ble_tracker::ESPBTUUID::from_uint16(0xD00D);
    mock_service_data.data = mock_data;
    mock_device.set_service_datas({mock_service_data});

    bool result = listener.parse_device(mock_device);

    TEST_ASSERT_TRUE(result);
    TEST_ASSERT_EQUAL_FLOAT(6, kill_count_sensor.last_state);
    // Add assertions for other sensors as needed
    // TEST_ASSERT_EQUAL_FLOAT(expected_battery_level, battery_level_sensor.last_state);
    // TEST_ASSERT_EQUAL_FLOAT(expected_last_activation, last_activation_sensor.last_state);
    // TEST_ASSERT_EQUAL_FLOAT(expected_total_activations, total_activations_sensor.last_state);
    // Note: Days since activation might be tricky to test as it depends on the current time
}

void test_reverse_serial() {
    GoodnatureBleListener listener;
    std::string reversed = listener.reverse_serial("ABCDEF00");
    TEST_ASSERT_EQUAL_STRING("00EFCDAB", reversed.c_str());
}

void test_parse_timestamp() {
    GoodnatureBleListener listener;
    uint32_t timestamp = listener.parse_timestamp("\x01\x02\x03\x04");
    TEST_ASSERT_EQUAL_UINT32(0x04030201, timestamp);
}

void test_add_device() {
    GoodnatureBleListener listener;
    listener.add_device("Test Device", "ABCDEF00");
    
    // Mock kill info data for the added device
    std::string mock_data = "7700FEDCBA1a87bda3010600";

    esp32_ble_tracker::ESPBTDevice mock_device;
    mock_device.set_name("GN");
    esp32_ble_tracker::ServiceData mock_service_data;
    mock_service_data.uuid = esp32_ble_tracker::ESPBTUUID::from_uint16(0xD00D);
    mock_service_data.data = mock_data;
    mock_device.set_service_datas({mock_service_data});

    listener.parse_device(mock_device);

    // Check if the device name is correctly associated
    // Note: This assumes you've made last_seen_device_ public or added a getter method
    TEST_ASSERT_EQUAL_STRING("Test Device", listener.last_seen_device_.c_str());
}

void setup() {
    // Initialize the logger (needed for Unity test runner)
    esp_log_level_set("*", ESP_LOG_DEBUG);
}

int runUnityTests(void) {
    UNITY_BEGIN();
    RUN_TEST(test_parse_kill_info);
    RUN_TEST(test_reverse_serial);
    RUN_TEST(test_parse_timestamp);
    RUN_TEST(test_add_device);
    return UNITY_END();
}

void app_main() {
    setup();
    runUnityTests();
}

extern "C" {
    void app_main();
}