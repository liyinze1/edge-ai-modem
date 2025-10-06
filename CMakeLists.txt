# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

find_package(Zephyr REQUIRED HINTS $ENV{ZEPHYR_BASE})
project(battery_free_roadrunner)
include_directories("include/")
# target_sources(app PRIVATE src/main.c src/modem.c src/battery.c src/i2c_sensor4.c src/switch1.c src/lowpower.c src/opencircuit.c src/switch_divider.c src/read_solar.c)
target_sources(app PRIVATE src/main.c src/roadruner_wakeup.c src/lowpower.c src/serial_interface.c src/nrf_wakeup.c)