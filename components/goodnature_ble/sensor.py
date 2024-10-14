import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, esp32_ble_tracker
from esphome.const import (
    CONF_BATTERY_LEVEL,
    CONF_MAC_ADDRESS,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_EMPTY,
    ICON_EMPTY,
    ICON_SECURITY,
    ICON_BATTERY,
    UNIT_PERCENT,
    UNIT_TIMESTAMP,
    DEVICE_CLASS_EMPTY,
    DEVICE_CLASS_BATTERY,
    DEVICE_CLASS_TIMESTAMP,
    CONF_ID,
    CONF_NAME,
)

DEPENDENCIES = ['esp32_ble_tracker']
AUTO_LOAD = ['sensor']

GoodnatureBle_ns = cg.esphome_ns.namespace("goodnature_ble")
GoodnatureBle = GoodnatureBle_ns.class_(
    "GoodnatureBleListener", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

CONF_KILL_COUNT = 'kill_count'
CONF_LAST_ACTIVATION = 'last_activation'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GoodnatureBle),
    cv.Optional(CONF_NAME): cv.valid_name,
    cv.Optional(CONF_MAC_ADDRESS): cv.mac_address,
    cv.Optional(CONF_KILL_COUNT): sensor.sensor_schema(
        unit_of_measurement=UNIT_EMPTY,
        icon=ICON_SECURITY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_EMPTY,
        tate_class=STATE_CLASS_TOTAL_INCREASING,
    ),
    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        Uunit_of_measurement=UNIT_PERCENT,
        icon=ICON_BATTERY,
        accuracy_decimals=0,
        device_class=DEVICE_CLASS_BATTERY,
        tate_class=STATE_CLASS_MEASUREMENT,
    ),
    cv.Optional(CONF_LAST_ACTIVATION): sensor.sensor_schema(
        unit_of_measurement=UNIT_TIMESTAMP,
        icon=ICON_EMPTY,
        device_class=DEVICE_CLASS_TIMESTAMP,
        tate_class=STATE_CLASS_MEASUREMENT
    ),
}).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    if CONF_MAC_ADDRESS in config:
        cg.add(var.set_address(config[CONF_MAC_ADDRESS].as_hex))

    if CONF_NAME in config:
        cg.add(var.set_name(config[CONF_NAME]))

    if CONF_KILL_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_KILL_COUNT])
        cg.add(var.set_kill_count_sensor(sens))

    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level_sensor(sens))
    
    if CONF_LAST_ACTIVATION in config:
        sens = await sensor.new_sensor(config[CONF_LAST_ACTIVATION])
        cg.add(var.set_last_activation_sensor(sens))
    