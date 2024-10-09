import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, esp32_ble_tracker
from esphome.const import CONF_ID

#from . import goodnature_ble_ns, GoodnatureBleListener

DEPENDENCIES = ["esp32_ble_tracker"]

GoodnatureBle_ns = cg.esphome_ns.namespace("GoodnatureBle")
GoodnatureBle = GoodnatureBle_ns.class_(
    "GoodnatureBle", esp32_ble_tracker.ESPBTDeviceListener, cg.Component
)

CONF_KILL_COUNT = 'kill_count'
CONF_BATTERY_LEVEL = 'battery_level'
CONF_LAST_ACTIVATION = 'last_activation'
CONF_TOTAL_ACTIVATIONS = 'total_activations'
CONF_DAYS_SINCE_ACTIVATION = 'days_since_activation'

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GoodnatureBle),
    cv.Optional(CONF_KILL_COUNT): sensor.sensor_schema(
        accuracy_decimals=0
    ),
    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        accuracy_decimals=0
    ),
    cv.Optional(CONF_LAST_ACTIVATION): sensor.sensor_schema(
        accuracy_decimals=0
    ),
    cv.Optional(CONF_TOTAL_ACTIVATIONS): sensor.sensor_schema(
        accuracy_decimals=0
    ),
    cv.Optional(CONF_DAYS_SINCE_ACTIVATION): sensor.sensor_schema(
        accuracy_decimals=1
    ),
}).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    #await sensor.register_sensor(var, config)
    if CONF_KILL_COUNT in config:
        sens = await sensor.new_sensor(config[CONF_KILL_COUNT])
        cg.add(var.set_kill_count_sensor(sens))
    
    if CONF_BATTERY_LEVEL in config:
        sens = await sensor.new_sensor(config[CONF_BATTERY_LEVEL])
        cg.add(var.set_battery_level_sensor(sens))
    
    if CONF_LAST_ACTIVATION in config:
        sens = await sensor.new_sensor(config[CONF_LAST_ACTIVATION])
        cg.add(var.set_last_activation_sensor(sens))
    
    if CONF_TOTAL_ACTIVATIONS in config:
        sens = await sensor.new_sensor(config[CONF_TOTAL_ACTIVATIONS])
        cg.add(var.set_total_activations_sensor(sens))
    
    if CONF_DAYS_SINCE_ACTIVATION in config:
        sens = await sensor.new_sensor(config[CONF_DAYS_SINCE_ACTIVATION])
        cg.add(var.set_days_since_activation_sensor(sens))