
# make 编译并下载
# make VERBOSE=1 显示编译详细过程
# make clean 清除编译临时文件
#
# 注意： Linux 下编译方式：
#     1. 从 http://pkgman.jieliapp.com/doc/all 处找到下载链接
#     2. 下载后，解压到 /opt/jieli 目录下，保证
#       /opt/jieli/common/bin/clang 存在（注意目录层次）
#     3. 确认 ulimit -n 的结果足够大（建议大于8096），否则链接可能会因为打开文件太多而失败
#       可以通过 ulimit -n 8096 来设置一个较大的值
#

# 工具路径设置
ifeq ($(OS), Windows_NT)
# Windows 下工具链位置
TOOL_DIR := C:/JL/pi32/bin
CC    := clang.exe
CXX   := clang.exe
LD    := pi32v2-lto-wrapper.exe
AR    := llvm-ar.exe
MKDIR := mkdir_win -p
RM    := rm -rf

SYS_LIB_DIR := C:/JL/pi32/pi32v2-lib/r3
SYS_INC_DIR := C:/JL/pi32/pi32v2-include
EXT_CFLAGS  := # Windows 下不需要 -D__SHELL__
export PATH:=$(TOOL_DIR);$(PATH)

## 后处理脚本
FIXBAT          := ..\..\..\..\tools\utils\fixbat.exe # 用于处理 utf8->gbk 编码问题
POST_SCRIPT     := ../../../../cpu/br23/tools/download.bat
RUN_POST_SCRIPT := ..\..\..\..\cpu\br23\tools\download.bat
else
# Linux 下工具链位置
TOOL_DIR := /opt/jieli/pi32v2/bin
CC    := clang
CXX   := clang
LD    := lto-wrapper
AR    := lto-ar
MKDIR := mkdir -p
RM    := rm -rf
export OBJDUMP := $(TOOL_DIR)/objdump
export OBJCOPY := $(TOOL_DIR)/objcopy
export OBJSIZEDUMP := $(TOOL_DIR)/objsizedump

SYS_LIB_DIR := $(TOOL_DIR)/../lib/r3
SYS_INC_DIR := $(TOOL_DIR)/../include
EXT_CFLAGS  := -D__SHELL__ # Linux 下需要这个保证正确处理 download.c
export PATH:=$(TOOL_DIR):$(PATH)

## 后处理脚本
FIXBAT          := touch # Linux下不需要处理 bat 编码问题
POST_SCRIPT     := ../../../../cpu/br23/tools/download.sh
RUN_POST_SCRIPT := bash $(POST_SCRIPT)
endif

CC  := $(TOOL_DIR)/$(CC)
CXX := $(TOOL_DIR)/$(CXX)
LD  := $(TOOL_DIR)/$(LD)
AR  := $(TOOL_DIR)/$(AR)
# 输出文件设置
OUT_ELF   := ../../../../cpu/br23/tools/sdk.elf
OBJ_FILE  := $(OUT_ELF).objs.txt
# 编译路径设置
BUILD_DIR := objs
# 工程路径前缀
ROOT_PREFIX := ../../../..

# 编译参数设置
CFLAGS := \
	-target pi32v2 \
	-mcpu=r3 \
	-integrated-as \
	-flto \
	-Wuninitialized \
	-Wno-invalid-noreturn \
	-fno-common \
	-integrated-as \
	-Oz \
	-g \
	-flto \
	-fallow-pointer-null \
	-fprefer-gnu-section \
	-Wno-shift-negative-value \
	-Wundef \
	-Wframe-larger-than=256 \
	-Wincompatible-pointer-types \
	-Wreturn-type \
	-Wimplicit-function-declaration \
	-fms-extensions \
	-w \


# C++额外的编译参数
CXXFLAGS :=


# 宏定义
DEFINES := \
	-DSUPPORT_MS_EXTENSIONS \
	-DCONFIG_RELEASE_ENABLE \
	-DCONFIG_CPU_BR23 \
	-DCONFIG_EQ_SUPPORT_ASYNC \
	-DCONFIG_MIXER_CYCLIC \
	-DCONFIG_FREE_RTOS_ENABLE \
	-DCONFIG_DNS_ENABLE \
	-DCONFIG_MMU_ENABLE \
	-DCONFIG_SBC_CODEC_HW \
	-DCONFIG_MSBC_CODEC_HW \
	-DCONFIG_AEC_M=640 \
	-DCONFIG_AUDIO_ONCHIP \
	-DCONFIG_MEDIA_DEVELOP_ENABLE \
	-D__GCC_PI32V2__ \
	-DCONFIG_NEW_ECC_ENABLE \
	-DEVENT_HANDLER_NUM_CONFIG=2 \
	-DEVENT_TOUCH_ENABLE_CONFIG=0 \
	-DEVENT_POOL_SIZE_CONFIG=256 \
	-DCONFIG_EVENT_KEY_MAP_ENABLE=0 \
	-DTIMER_POOL_NUM_CONFIG=10 \
	-DAPP_ASYNC_POOL_NUM_CONFIG=0 \
	-DSDK_VERSION_CFG_DEFINE=0x240001 \
	-DSDK_VERSION_DATE_DEFINE=20241231 \
	-DVFS_ENABLE=1 \
	-DUSE_SDFILE_NEW=1 \
	-DVFS_FILE_POOL_NUM_CONFIG=0 \
	-DSDFILE_VERSION=0x020000 \
	-DVM_MAX_SIZE_CONFIG=16*1024 \
	-DVM_ITEM_MAX_NUM=256 \
	-DCONFIG_TWS_ENABLE \
	-DCONFIG_HID_CASE_ENABLE \
	-DCONFIG_TRANSFER_ENABLE \
	-DCONFIG_LITE_AEC_ENABLE=0 \
	-DCONFIG_LITE_AUDIO \
	-DCONFIG_LMP_RESET_ENABLE \
	-DCONFIG_LMP_REFRESH_ENCRYPTION_KEY_ENABLE \
	-DCONFIG_LINK_DISTURB_SCAN_ENABLE=0 \
	-DCONFIG_BTCTRLER_TASK_DEL_ENABLE \
	-DCONFIG_UPDATA_ENABLE \
	-DCONFIG_OTA_UPDATA_ENABLE \
	-DCONFIG_ITEM_FORMAT_VM \
	-DTCFG_APP_BT_EN=1 \
	-DUSB_PC_NO_APP_MODE \


DEFINES += $(EXT_CFLAGS) # 额外的一些定义

# 头文件搜索路径
INCLUDES := \
	-I../../../../include_lib \
	-I../../../../include_lib/driver \
	-I../../../../include_lib/driver/device \
	-I../../../../include_lib/driver/cpu/br23 \
	-I../../../../include_lib/system \
	-I../../../../include_lib/system/generic \
	-I../../../../include_lib/system/device \
	-I../../../../include_lib/system/fs \
	-I../../../../include_lib/system/ui \
	-I../../../../include_lib/system/math/cpu/br23 \
	-I../../../../include_lib/btctrler \
	-I../../../../include_lib/btctrler/port/br23 \
	-I../../../../include_lib/update \
	-I../../../../include_lib/agreement \
	-I../../../../include_lib/btstack/third_party/common \
	-I../../../../include_lib/btstack/third_party/rcsp \
	-I../../../../include_lib/media/media_develop \
	-I../../../../include_lib/media/media_develop/media \
	-I../../../../include_lib/media/media_develop/media/cpu/br23 \
	-I../../../../include_lib/media/media_develop/media/cpu/br23/asm \
	-I../../../../apps/hid/include \
	-I../../../../apps/hid/board/br23 \
	-I../../../../apps/common \
	-I../../../../apps/common/include \
	-I../../../../apps/common/third_party_profile/jieli \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata \
	-I../../../../apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data \
	-I../../../../apps/common/third_party_profile/jieli/gatt_common \
	-I../../../../apps/common/third_party_profile/jieli/online_db \
	-I../../../../apps/common/third_party_profile/jieli/trans_data_demo \
	-I../../../../apps/common/third_party_profile/common \
	-I../../../../apps/common/device \
	-I../../../../apps/common/device/usb \
	-I../../../../include_lib/ai_stack/JL_rcsp \
	-I../../../../include_lib/btstack \
	-I../../../../cpu/br23 \
	-I../../../../include_lib/system/ex_mcu \
	-I../../../../apps/common/code_switch/include \
	-I../../../../apps/common/device/optical_mouse_sensor/include \
	-I../../../../apps/common/device/touch_pad \
	-I../../../../apps/common/audio \
	-I../../../../cpu/br23/audio_dec \
	-I../../../../cpu/br23/audio_enc \
	-I../../../../apps/common/device/usb/device \
	-I../../../../apps/common/device/usb/host \
	-I$(SYS_INC_DIR) \


# 需要编译的 .c 文件
c_SRC_FILES := \
	../../../../apps/common/audio/audio_utils.c \
	../../../../apps/common/audio/decode/decode.c \
	../../../../apps/common/audio/sine_make.c \
	../../../../apps/common/bt_common/bt_test_api.c \
	../../../../apps/common/code_switch/code_switch.c \
	../../../../apps/common/debug/debug.c \
	../../../../apps/common/device/key/adkey.c \
	../../../../apps/common/device/key/ctmu_touch_key.c \
	../../../../apps/common/device/key/iokey.c \
	../../../../apps/common/device/key/irkey.c \
	../../../../apps/common/device/key/key_driver.c \
	../../../../apps/common/device/key/matrix_keyboard.c \
	../../../../apps/common/device/key/matrix_keyboard_ex_mcu.c \
	../../../../apps/common/device/key/touch_key.c \
	../../../../apps/common/device/optical_mouse_sensor/OMSensor_manage.c \
	../../../../apps/common/device/optical_mouse_sensor/hal3205/hal3205.c \
	../../../../apps/common/device/optical_mouse_sensor/hal3212/hal3212.c \
	../../../../apps/common/device/touch_pad/SYD9557M.c \
	../../../../apps/common/device/usb/device/cdc.c \
	../../../../apps/common/device/usb/device/custom_hid.c \
	../../../../apps/common/device/usb/device/descriptor.c \
	../../../../apps/common/device/usb/device/hid.c \
	../../../../apps/common/device/usb/device/msd.c \
	../../../../apps/common/device/usb/device/msd_upgrade.c \
	../../../../apps/common/device/usb/device/task_pc.c \
	../../../../apps/common/device/usb/device/uac1.c \
	../../../../apps/common/device/usb/device/uac_stream.c \
	../../../../apps/common/device/usb/device/usb_device.c \
	../../../../apps/common/device/usb/device/user_setup.c \
	../../../../apps/common/device/usb/host/adb.c \
	../../../../apps/common/device/usb/host/aoa.c \
	../../../../apps/common/device/usb/host/audio.c \
	../../../../apps/common/device/usb/host/audio_demo.c \
	../../../../apps/common/device/usb/host/hid.c \
	../../../../apps/common/device/usb/host/usb_bulk_transfer.c \
	../../../../apps/common/device/usb/host/usb_ctrl_transfer.c \
	../../../../apps/common/device/usb/host/usb_host.c \
	../../../../apps/common/device/usb/host/usb_storage.c \
	../../../../apps/common/device/usb/usb_config.c \
	../../../../apps/common/device/usb/usb_host_config.c \
	../../../../apps/common/ex_mcu/ex_mcu_uart.c \
	../../../../apps/common/jl_kws/jl_kws_algo.c \
	../../../../apps/common/jl_kws/jl_kws_audio.c \
	../../../../apps/common/jl_kws/jl_kws_event.c \
	../../../../apps/common/jl_kws/jl_kws_main.c \
	../../../../apps/common/temp_trim/dtemp_pll_trim.c \
	../../../../apps/common/third_party_profile/common/custom_cfg.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/bt_trans_data/rcsp_hid_inter.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_bluetooth.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_ch_loader_download.c \
	../../../../apps/common/third_party_profile/jieli/JL_rcsp/rcsp_updata/rcsp_user_update.c \
	../../../../apps/common/third_party_profile/jieli/gatt_common/le_gatt_client.c \
	../../../../apps/common/third_party_profile/jieli/gatt_common/le_gatt_common.c \
	../../../../apps/common/third_party_profile/jieli/gatt_common/le_gatt_server.c \
	../../../../apps/common/third_party_profile/jieli/online_db/online_db_deal.c \
	../../../../apps/common/third_party_profile/jieli/online_db/spp_online_db.c \
	../../../../apps/common/update/testbox_update.c \
	../../../../apps/common/update/uart_update.c \
	../../../../apps/common/update/uart_update_master.c \
	../../../../apps/common/update/update.c \
	../../../../apps/hid/app_main.c \
	../../../../apps/hid/board/br23/board_ac6351d_keyboard.c \
	../../../../apps/hid/board/br23/board_ac635n_demo.c \
	../../../../apps/hid/config/lib_btctrler_config.c \
	../../../../apps/hid/config/lib_btstack_config.c \
	../../../../apps/hid/config/lib_driver_config.c \
	../../../../apps/hid/config/lib_media_config.c \
	../../../../apps/hid/config/lib_profile_config.c \
	../../../../apps/hid/config/lib_system_config.c \
	../../../../apps/hid/config/lib_update_config.c \
	../../../../apps/hid/config/log_config.c \
	../../../../apps/hid/examples/gamebox/app_gamebox.c \
	../../../../apps/hid/examples/gamebox/gamebox.c \
	../../../../apps/hid/examples/gamebox/key_mapping.c \
	../../../../apps/hid/examples/idle/app_idle.c \
	../../../../apps/hid/examples/keyboard/app_keyboard.c \
	../../../../apps/hid/examples/keyfob/app_keyfob.c \
	../../../../apps/hid/examples/keypage/app_keypage.c \
	../../../../apps/hid/examples/mouse_dual/app_mouse_dual.c \
	../../../../apps/hid/examples/mouse_single/app_mouse.c \
	../../../../apps/hid/examples/standard_keyboard/app_standard_keyboard.c \
	../../../../apps/hid/examples/standard_keyboard/usb_hid_devices.c \
	../../../../apps/hid/examples/voice_remote_control/app_remote_control.c \
	../../../../apps/hid/examples/voice_remote_control/audio_codec_demo.c \
	../../../../apps/hid/modules/aec/br23/audio_aec.c \
	../../../../apps/hid/modules/bt/app_comm_ble.c \
	../../../../apps/hid/modules/bt/app_comm_edr.c \
	../../../../apps/hid/modules/bt/ble_hogp.c \
	../../../../apps/hid/modules/bt/edr_hid_user.c \
	../../../../apps/hid/modules/misc.c \
	../../../../apps/hid/modules/power/app_charge.c \
	../../../../apps/hid/modules/power/app_chargestore.c \
	../../../../apps/hid/modules/power/app_handshake.c \
	../../../../apps/hid/modules/power/app_power_manage.c \
	../../../../apps/hid/modules/rtc_alarm.c \
	../../../../apps/hid/modules/user_cfg.c \
	../../../../apps/hid/version.c \
	../../../../cpu/br23/adc_api.c \
	../../../../cpu/br23/audio_common/app_audio.c \
	../../../../cpu/br23/audio_dec/audio_dec.c \
	../../../../cpu/br23/audio_dec/audio_dec_midi_ctrl.c \
	../../../../cpu/br23/audio_dec/audio_dec_pc.c \
	../../../../cpu/br23/audio_dec/audio_dec_tone.c \
	../../../../cpu/br23/audio_dec/audio_usb_mic.c \
	../../../../cpu/br23/audio_dec/tone_player.c \
	../../../../cpu/br23/audio_demo/audio_dac_io_demo.c \
	../../../../cpu/br23/audio_demo/audio_fft_demo.c \
	../../../../cpu/br23/audio_enc/audio_adc_demo.c \
	../../../../cpu/br23/audio_enc/audio_enc.c \
	../../../../cpu/br23/audio_enc/audio_mic_codec.c \
	../../../../cpu/br23/charge.c \
	../../../../cpu/br23/chargestore.c \
	../../../../cpu/br23/clock_manager.c \
	../../../../cpu/br23/ctmu.c \
	../../../../cpu/br23/handshake_timer.c \
	../../../../cpu/br23/iic_eeprom_test.c \
	../../../../cpu/br23/iic_hw.c \
	../../../../cpu/br23/iic_slave_test.c \
	../../../../cpu/br23/iic_soft.c \
	../../../../cpu/br23/irflt.c \
	../../../../cpu/br23/led_spi.c \
	../../../../cpu/br23/ledc_test.c \
	../../../../cpu/br23/mcpwm.c \
	../../../../cpu/br23/pap.c \
	../../../../cpu/br23/plcnt.c \
	../../../../cpu/br23/port_wkup.c \
	../../../../cpu/br23/pwm_led.c \
	../../../../cpu/br23/setup.c \
	../../../../cpu/br23/spi.c \
	../../../../cpu/br23/spi_test.c \
	../../../../cpu/br23/uart_dev.c \
	../../../../cpu/br23/uart_test.c \


# 需要编译的 .S 文件
S_SRC_FILES :=


# 需要编译的 .s 文件
s_SRC_FILES :=


# 需要编译的 .cpp 文件
cpp_SRC_FILES :=


# 需要编译的 .cc 文件
cc_SRC_FILES :=


# 需要编译的 .cxx 文件
cxx_SRC_FILES :=


# 链接参数
LFLAGS := \
	--plugin-opt=-pi32v2-always-use-itblock=false \
	--plugin-opt=-enable-ipra=true \
	--plugin-opt=-pi32v2-merge-max-offset=4096 \
	--plugin-opt=-pi32v2-enable-simd=true \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-global-merge-on-const \
	--plugin-opt=-inline-threshold=5 \
	--plugin-opt=-inline-max-allocated-size=32 \
	--plugin-opt=-inline-normal-into-special-section=true \
	--plugin-opt=-dont-used-symbol-list=malloc,free,sprintf,printf,puts,putchar \
	--plugin-opt=save-temps \
	--plugin-opt=-pi32v2-enable-rep-memop \
	--plugin-opt=-warn-stack-size=256 \
	--sort-common \
	--dont-complain-call-overflow \
	--plugin-opt=-used-symbol-file=../../../../cpu/br23/sdk_used_list.used \
	--gc-sections \
	--start-group \
	../../../../cpu/br23/liba/cpu.a \
	../../../../cpu/br23/liba/system.a \
	../../../../cpu/br23/liba/btstack.a \
	../../../../cpu/br23/liba/btctrler.a \
	../../../../cpu/br23/liba/crypto_toolbox_Osize.a \
	../../../../cpu/br23/liba/rcsp_stack.a \
	../../../../cpu/br23/liba/agreement.a \
	../../../../cpu/br23/liba/aec.a \
	../../../../cpu/br23/liba/media.a \
	../../../../cpu/br23/liba/media_app.a \
	../../../../cpu/br23/liba/libAptFilt_pi32v2_OnChip.a \
	../../../../cpu/br23/liba/libEchoSuppress_pi32v2_OnChip.a \
	../../../../cpu/br23/liba/libNoiseSuppress_pi32v2_OnChip.a \
	../../../../cpu/br23/liba/libSplittingFilter_pi32v2_OnChip.a \
	../../../../cpu/br23/liba/libFFT_pi32v2_OnChip.a \
	../../../../cpu/br23/liba/libOpcore_maskrom_pi32v2_OnChip.a \
	../../../../cpu/br23/liba/lib_resample_cal.a \
	../../../../cpu/br23/liba/lib_resample_fast_cal.a \
	../../../../cpu/br23/liba/opus_enc_lib.a \
	../../../../cpu/br23/liba/speex_enc_lib.a \
	../../../../cpu/br23/liba/lib_adpcm_ima_code.a \
	../../../../cpu/br23/liba/lib_adpcm_code.a \
	../../../../cpu/br23/liba/lc3_codec_lib.a \
	../../../../cpu/br23/liba/wav_dec_lib.a \
	../../../../cpu/br23/liba/bfilterfun_lib.a \
	../../../../cpu/br23/liba/wtg_dec_lib.a \
	../../../../cpu/br23/liba/wtgv2_dec_lib.a \
	../../../../cpu/br23/liba/wav_dec_lib.a  \
	../../../../cpu/br23/liba/mp3_dec_lib.a \
	../../../../cpu/br23/liba/lib_pitch_speed.a \
	../../../../cpu/br23/liba/lib_dns.a \
	../../../../cpu/br23/liba/lib_midi_dec.a \
	../../../../cpu/br23/liba/update.a \
	--end-group \
	-T../../../../cpu/br23/sdk.ld \
	-M=../../../../cpu/br23/tools/sdk.map \
	--plugin-opt=mcpu=r3 \
	--plugin-opt=-mattr=+fprev1 \


LIBPATHS := \
	-L$(SYS_LIB_DIR) \


LIBS := \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libc.a \
	$(SYS_LIB_DIR)/libm.a \
	$(SYS_LIB_DIR)/libcompiler-rt.a \



c_OBJS    := $(c_SRC_FILES:%.c=%.c.o)
S_OBJS    := $(S_SRC_FILES:%.S=%.S.o)
s_OBJS    := $(s_SRC_FILES:%.s=%.s.o)
cpp_OBJS  := $(cpp_SRC_FILES:%.cpp=%.cpp.o)
cxx_OBJS  := $(cxx_SRC_FILES:%.cxx=%.cxx.o)
cc_OBJS   := $(cc_SRC_FILES:%.cc=%.cc.o)

OBJS      := $(c_OBJS) $(S_OBJS) $(s_OBJS) $(cpp_OBJS) $(cxx_OBJS) $(cc_OBJS)
DEP_FILES := $(OBJS:%.o=%.d)


OBJS      := $(addprefix $(BUILD_DIR)/, $(OBJS:$(ROOT_PREFIX)/%=%))
DEP_FILES := $(addprefix $(BUILD_DIR)/, $(DEP_FILES:$(ROOT_PREFIX)/%=%))


VERBOSE ?= 0
ifeq ($(VERBOSE), 1)
QUITE :=
else
QUITE := @
endif

# 一些旧的 make 不支持 file 函数，需要 make 的时候指定 LINK_AT=0 make
LINK_AT ?= 1

# 表示下面的不是一个文件的名字，无论是否存在 all, clean, pre_build 这样的文件
# 还是要执行命令
# see: https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean pre_build

# 不要使用 make 预设置的规则
# see: https://www.gnu.org/software/make/manual/html_node/Suffix-Rules.html
.SUFFIXES:

all: pre_build $(OUT_ELF)
	$(info +POST-BUILD)
	$(QUITE) $(RUN_POST_SCRIPT) sdk

pre_build:
	$(info +PRE-BUILD)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br23/sdk_used_list.c -o ../../../../cpu/br23/sdk_used_list.used
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br23/sdk_ld.c -o ../../../../cpu/br23/sdk.ld
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br23/tools/download.c -o $(POST_SCRIPT)
	$(QUITE) $(FIXBAT) $(POST_SCRIPT)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -D__LD__ -E -P ../../../../cpu/br23/tools/isd_config_rule.c -o ../../../../cpu/br23/tools/isd_config.ini

clean:
	$(QUITE) $(RM) $(OUT_ELF)
	$(QUITE) $(RM) $(BUILD_DIR)



ifeq ($(LINK_AT), 1)
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(file >$(OBJ_FILE), $(OBJS))
	$(QUITE) $(LD) -o $(OUT_ELF) @$(OBJ_FILE) $(LFLAGS) $(LIBPATHS) $(LIBS)
else
$(OUT_ELF): $(OBJS)
	$(info +LINK $@)
	$(shell $(MKDIR) $(@D))
	$(QUITE) $(LD) -o $(OUT_ELF) $(OBJS) $(LFLAGS) $(LIBPATHS) $(LIBS)
endif


$(BUILD_DIR)/%.c.o : $(ROOT_PREFIX)/%.c
	$(info +CC $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.S.o : $(ROOT_PREFIX)/%.S
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.s.o : $(ROOT_PREFIX)/%.s
	$(info +AS $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CC) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cpp.o : $(ROOT_PREFIX)/%.cpp
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cxx.o : $(ROOT_PREFIX)/%.cxx
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

$(BUILD_DIR)/%.cc.o : $(ROOT_PREFIX)/%.cc
	$(info +CXX $<)
	$(QUITE) $(MKDIR) $(@D)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -MM -MT $@ $< -o $(@:.o=.d)
	$(QUITE) $(CXX) $(CXXFLAGS) $(CFLAGS) $(DEFINES) $(INCLUDES) -c $< -o $@

-include $(DEP_FILES)
