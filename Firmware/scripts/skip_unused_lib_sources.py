# Leaves library sources the firmware never uses out of the build.
#
# lib_archive = false (platformio.ini) links every object file of every
# library, and some of them stay in the image even though nothing calls them,
# because they create global objects at start-up. SimpleFOCDrivers' comms/
# folder (register access, telemetry, I2C commander, which also pulls in the
# I2C driver) was ~2.9 KB of a flash budget that is nearly used up (the
# settings page starts at 126 KB, see docs/firmware.md#flash-size-limit).
#
# Boards with no I2C parts (custom_no_i2c = yes in their board.ini) also
# leave out the Wire library and the library sources that use it: an unused
# global `Wire` object otherwise keeps ~4.5 KB of I2C code. Wire's headers
# stay available (SimpleFOC.h includes Wire.h); only its code goes, so using
# Wire on such a board fails at link time rather than silently.
Import("env")


def skip(node):
    return None  # returning None drops the file from the build


env.AddBuildMiddleware(skip, "*/SimpleFOCDrivers/src/comms/*")

if env.GetProjectOption("custom_no_i2c", "no").strip().lower() in ("yes", "true", "1"):
    for pattern in (
        "*/libraries/Wire/src/*",                       # Wire.cpp, utility/twi.c
        "*/src/sensors/MagneticSensorI2C.cpp",          # Simple FOC
        "*/SimpleFOCDrivers/src/encoders/as5600/*",
        "*/SimpleFOCDrivers/src/settings/i2c/*",
    ):
        env.AddBuildMiddleware(skip, pattern)
