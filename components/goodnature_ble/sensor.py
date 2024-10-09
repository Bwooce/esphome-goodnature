import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor
from esphome.const import CONF_ID, UNIT_EMPTY, ICON_COUNTER

from . import goodnature_ble_ns, GoodnatureBleListener

CONF_KILL_COUNT = "kill_count"

CONFIG_SCHEMA = sensor.sensor_schema(
    unit_of_measurement=UNIT_EMPTY,
    icon=ICON_COUNTER,
    accuracy_decimals=0
).extend({
    cv.GenerateID(): cv.declare_id(GoodnatureBleListener),
})

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await sensor.register_sensor(var, config)