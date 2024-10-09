import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.const import CONF_ID, CONF_NAME, UNIT_EMPTY, ICON_COUNTER, UNIT_PERCENT, ICON_BATTERY, ICON_CLOCK, UNIT_DAYS, ICON_CALENDAR_CLOCK
from esphome.components import esp32_ble_tracker, sensor

DEPENDENCIES = ['esp32_ble_tracker']
AUTO_LOAD = ['sensor']

goodnature_ns = cg.esphome_ns.namespace('goodnature')
GoodnatureListener = goodnature_ns.class_('GoodnatureListener', esp32_ble_tracker.ESPBTDeviceListener, cg.Component)

CONF_DEVICES = 'devices'
CONF_SERIAL = 'serial'
CONF_KILL_COUNT = 'kill_count'
CONF_BATTERY_LEVEL = 'battery_level'
CONF_LAST_ACTIVATION = 'last_activation'
CONF_TOTAL_ACTIVATIONS = 'total_activations'
CONF_DAYS_SINCE_ACTIVATION = 'days_since_activation'

DEVICE_SCHEMA = cv.Schema({
    cv.Required(CONF_NAME): cv.string,
    cv.Required(CONF_SERIAL): cv.string,
})

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(GoodnatureListener),
    cv.Optional(CONF_DEVICES): cv.ensure_list(DEVICE_SCHEMA),
    cv.Optional(CONF_KILL_COUNT): sensor.sensor_schema(
        unit_of_measurement=UNIT_EMPTY,
        icon=ICON_COUNTER,
        accuracy_decimals=0
    ),
    cv.Optional(CONF_BATTERY_LEVEL): sensor.sensor_schema(
        unit_of_measurement=UNIT_PERCENT,
        icon=ICON_BATTERY,
        accuracy_decimals=0
    ),
    cv.Optional(CONF_LAST_ACTIVATION): sensor.sensor_schema(
        unit_of_measurement=UNIT_EMPTY,
        icon=ICON_CLOCK,
        accuracy_decimals=0
    ),
    cv.Optional(CONF_TOTAL_ACTIVATIONS): sensor.sensor_schema(
        unit_of_measurement=UNIT_EMPTY,
        icon=ICON_COUNTER,
        accuracy_decimals=0
    ),
    cv.Optional(CONF_DAYS_SINCE_ACTIVATION): sensor.sensor_schema(
        unit_of_measurement=UNIT_DAYS,
        icon=ICON_CALENDAR_CLOCK,
        accuracy_decimals=1
    ),
}).extend(esp32_ble_tracker.ESP_BLE_DEVICE_SCHEMA).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await esp32_ble_tracker.register_ble_device(var, config)

    if CONF_DEVICES in config:
        for device in config[CONF_DEVICES]:
            cg.add(var.add_device(device[CONF_NAME], device[CONF_SERIAL]))

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
