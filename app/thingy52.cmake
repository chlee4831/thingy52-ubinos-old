set_cache(NRF5SDK__BOARD_NAME                                                   "PCA10040"  STRING)

set_cache(NRF5SDK__SWI_DISABLE0                                                 TRUE    BOOL)

set_cache(UBINOS__BSP__LINK_MEMMAP_RAM_ORIGIN                                   0x20004400     STRING)
set_cache(UBINOS__BSP__LINK_MEMMAP_RAM_LENGTH                                   0x0000BC00     STRING)

include(${PROJECT_UBINOS_DIR}/config/ubinos_nrf52dk_softdevice.cmake)

include(${PROJECT_LIBRARY_DIR}/nrf5sdk_wrapper/config/nrf5sdk.cmake)

####
add_compile_definitions(BLE_STACK_SUPPORT_REQD NRF52_PAN_12 NRF52_PAN_15 NRF52_PAN_20 NRF52_PAN_30 NRF52_PAN_31 NRF52_PAN_36 NRF52_PAN_51 NRF52_PAN_53 NRF52_PAN_54 NRF52_PAN_55 NRF52_PAN_58 NRF52_PAN_62 NRF52_PAN_63 NRF52_PAN_64 S132 NRF_LOG_USES_RTT=1 NRF52 SOFTDEVICE_PRESENT SWI_DISABLE0 DEBUG ARM_MATH_CM4 MPU9250 EMPL USE_DMP EMPL_TARGET_NRF52 NO_VTOR_CONFIG)

set(INCLUDE__APP TRUE)
set(APP__NAME "usrmain")

get_filename_component(_tmpdir "${CMAKE_CURRENT_LIST_DIR}" ABSOLUTE)

file(GLOB_RECURSE _tmpsrc
    "${_tmpdir}/*.c"
    "${_tmpdir}/*.cpp"
    "${_tmpdir}/*.S"
    "${_tmpdir}/*.s")

set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmpsrc})

include_directories(${_tmpdir})

include_directories(${_tmpdir}/lib_thingy/config)

include_directories(${_tmpdir}/lib_thingy/libs/bv32fp-1.2)
include_directories(${_tmpdir}/lib_thingy/libs/dvi_adpcm)
include_directories(${_tmpdir}/lib_thingy/libs/jlink_monitor)
#include_directories(${_tmpdir}/lib_thingy/libs/sr3_audio)
include_directories(${_tmpdir}/lib_thingy/libs/vocal_anr)

include_directories(${_tmpdir}/lib_thingy/libs/eMD6/core/driver/eMPL)
include_directories(${_tmpdir}/lib_thingy/libs/eMD6/core/driver/include)
include_directories(${_tmpdir}/lib_thingy/libs/eMD6/core/driver/nRF52)
include_directories(${_tmpdir}/lib_thingy/libs/eMD6/core/eMPL-hal)
include_directories(${_tmpdir}/lib_thingy/libs/eMD6/core/mllite)
include_directories(${_tmpdir}/lib_thingy/libs/eMD6/core/mpl)

include_directories(${_tmpdir}/lib_thingy/include/board)
include_directories(${_tmpdir}/lib_thingy/include/macros)
include_directories(${_tmpdir}/lib_thingy/include/util)
include_directories(${_tmpdir}/lib_thingy/include/drivers)

include_directories(${_tmpdir}/modules)

include_directories(${_tmpdir}/../library/nrf5sdk_v17.00.00_lite/components/toolchain/cmsis/dsp/Include)

get_filename_component(_tmp_source_dir "${NRF5SDK__BASE_DIR}" ABSOLUTE)

set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/integration/nrfx/legacy/nrf_drv_ppi.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/integration/nrfx/legacy/nrf_drv_spi.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/integration/nrfx/legacy/nrf_drv_twi.c)

set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/modules/nrfx/drivers/src/nrfx_timer.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/modules/nrfx/drivers/src/nrfx_ppi.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/modules/nrfx/drivers/src/nrfx_spi.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/modules/nrfx/drivers/src/nrfx_twi.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/modules/nrfx/drivers/src/nrfx_saadc.c)
set(PROJECT_APP_SOURCES ${PROJECT_APP_SOURCES} ${_tmp_source_dir}/modules/nrfx/drivers/src/nrfx_pdm.c)

#MPL library
find_library(LIBMPLLIB mpllib ${_tmpdir}/lib_thingy/libs/eMD6/core/mpl)
set(PROJECT_LIBRARIES ${LIBMPLLIB} ${PROJECT_LIBRARIES})