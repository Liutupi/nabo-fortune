cmake_minimum_required(VERSION 3.16)
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(nabo-fortune-passport)
# assets 分区由 tools/pack_assets_fortune.py 生成，随 flash / merge-bin 一起写入
esptool_py_flash_to_partition(flash "assets" "${CMAKE_SOURCE_DIR}/assets/fortune_assets.bin")
