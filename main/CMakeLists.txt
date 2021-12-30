SET(BASE_FILES 
  "../src/sched.c"
  "../src/basecmd.c"
  "../src/command.c"
  "../src/debugcmds.c")

SET(BASE_GPIO_FILES 
  "../src/initial_pins.c"
  "../src/gpiocmds.c"
  "../src/stepper.c"
  "../src/endstop.c"
  "../src/trsync.c")

SET(BASE_ADC_FILES "../src/adccmds.c")
SET(BASE_SPI_FILES "../src/spicmds.c" "../src/thermocouple.c")
SET(BASE_I2C_FILES "../src/i2ccmds.c")
SET(BASE_PWM_FILES "../src/pwmcmds.c")
SET(BASE_BB_SPI_FILES "../src/spi_software.c" "../src/sensor_adxl345.c")
SET(BASE_BB_GPIO_FILES
  "../src/lcd_st7920.c"
  "../src/lcd_hd44780.c"
  "../src/buttons.c"
  "../src/tmcuart.c"
  "../src/neopixel.c"
  "../src/pulse_counter.c")

#${BASE_ADC_FILES}
#${BASE_SPI_FILES}
#${BASE_I2C_FILES}
#${BASE_PWM_FILES}
#${BASE_BB_SPI_FILES}
#${BASE_BB_GPIO_FILES}

SET(SRC_FILES
  "../src/esp/main.c"
  "../src/esp/timer.c"
  "../src/esp/console.c"
  "../src/esp/gpio.c"
  "../src/esp/usb_cdc.c"
  ${BASE_FILES}
  ${BASE_GPIO_FILES}
  "../src/generic/alloc.c"
  "../src/generic/crc16_ccitt.c")

idf_component_register(
    SRCS 
      ${SRC_FILES}
      ${PROJECT_BINARY_DIR}/compile_time_request.c
    INCLUDE_DIRS "../src" "../src/esp")

file(GLOB_RECURSE O_FILES ${PROJECT_BINARY_DIR}/*.obj)
file(WRITE ${PROJECT_BINARY_DIR}/ctr_process.sh 
"#!/bin/bash
src_dir=$1
shift
dest_dir=$1
shift
obj_files=$*
ctr_files=
for o in $obj_files
do
  xtensa-esp32-elf-objcopy -j '.compile_time_request' -O binary $o $o.ctr
  ctr_files=\"$ctr_files $o.ctr\"
done
cat $ctr_files | tr -s '\\0' '\\n' >$dest_dir/compile_time_request.txt
echo cp $src_dir/out/compile_time_request.txt $dest_dir/compile_time_request.txt
cd $src_dir
python2 $src_dir/scripts/buildcommands.py -d $dest_dir/klipper.dict -t \"xtensa-esp32-elf-gcc;xtensa-esp32-elf-as;xtensa-esp32-elf-ld;xtensa-esp32-elf-objcopy;xtensa-esp32-elf-objdump;xtensa-esp32-elf-strip\" $dest_dir/compile_time_request.txt $dest_dir/compile_time_request.c.in
sed 's/\\\"board\\//\\\"esp\\//g' $dest_dir/compile_time_request.c.in > $dest_dir/compile_time_request.c
"
)
add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/compile_time_request.c
    COMMAND bash ARGS ${PROJECT_BINARY_DIR}/ctr_process.sh ${CMAKE_SOURCE_DIR} ${PROJECT_BINARY_DIR} ${O_FILES}
    DEPENDS ${SRC_FILES} ${PROJECT_BINARY_DIR}/ctr_process.sh
)