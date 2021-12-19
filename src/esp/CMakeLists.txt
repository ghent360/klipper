idf_component_register(
    SRCS "main.c" "timer.c" "../sched.c" "../basecmd.c" "../command.c" "../generic/alloc.c" ${PROJECT_BINARY_DIR}/compile_time_request.c
    INCLUDE_DIRS "." "..")

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
    DEPENDS ${PROJECT_BINARY_DIR}/ctr_process.sh
)