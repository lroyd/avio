SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)
SET(TOOLCHAIN_DIR "/opt/hisi-linux/x86-arm/arm-hisiv300-linux/target/bin")

SET(CMAKE_C_COMPILER   ${TOOLCHAIN_DIR}/arm-hisiv300-linux-gcc)
SET(CMAKE_CXX_COMPILER ${TOOLCHAIN_DIR}/arm-hisiv300-linux-g++)

SET(CMAKE_FIND_ROOT_PATH  ${TOOLCHAIN_DIR})
SET(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

#设定MPP相关（绝对路径）
#SET(MPP_PATH	"/home/lroyd/pjsip/mpp")
SET(MPP_PATH	"/home/lroyd/haisi/Hi3518E_SDK_V1.0.3.0/mpp")
SET(COMMON_DIR  "${MPP_PATH}/sample/common")
SET(LIBS_DIR	"${MPP_PATH}/lib")

SET(CHIP_FLAGS	"-DCHIP_ID=CHIP_HI3518E_V200 -DHI_ACODEC_TYPE_INNER -DSENSOR_TYPE=APTINA_AR0130_DC_720P_30FPS")


SET(LIB_AUDIO	"-lVoiceEngine -lupvqe -ldnvqe ")
SET(LIB_SENSOR	"-lisp -l_iniparser -l_hiae -l_hiawb -l_hiaf -l_hidefog -lsns_ar0130 ")
SET(LIB_MPI		"-lmpi -live -lmd")
SET(LIBS_FLAGS	"${LIB_AUDIO} ${LIB_SENSOR} ${LIB_MPI}")




