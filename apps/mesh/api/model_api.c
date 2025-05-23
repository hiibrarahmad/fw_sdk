#include "btstack/bluetooth.h"
#include "system/includes.h"
#include "bt_common.h"
#include "api/sig_mesh_api.h"
#include "model_api.h"

#define LOG_TAG             "[Mesh-ModelApi]"
#define LOG_ERROR_ENABLE
#define LOG_DEBUG_ENABLE
#define LOG_INFO_ENABLE
/* #define LOG_DUMP_ENABLE */
#define LOG_CLI_ENABLE
#include "debug.h"


static u16_t primary_addr;
static u16_t primary_net_idx;
static u32_t led_blink_timer;

extern char *bd_addr_to_str(u8 addr[6]);
extern const u8 led_use_port[2];

void prov_complete(u16_t net_idx, u16_t addr)
{
    log_info("provisioning complete for net_idx 0x%04x addr 0x%04x\n",
             net_idx, addr);
    primary_addr = addr;
    primary_net_idx = net_idx;
}

u16_t get_primary_addr(void)
{
    return primary_addr;
}

void prov_reset(void)
{
    bt_mesh_prov_enable(BT_MESH_PROV_ADV | BT_MESH_PROV_GATT);
}

static void relay_sent(u16 *delay, u16 *duration, void *cb_data)
{
    u16 delay_ms;

    pseudo_random_genrate(&delay_ms, 2);

    delay_ms = btctler_get_rand_from_assign_range(delay_ms, 50, 200);

    *delay = delay_ms;

    log_info("Relay_delay = %u ms", delay_ms);
}

const struct bt_mesh_send_cb relay_sent_cb = {
    // .user_intercept = relay_sent,//for compiler, not used now.
};

void gpio_pin_write(u8_t led_index, u8_t onoff)
{
    if (led_index >= ARRAY_SIZE(led_use_port)) {
        log_info("led_index over realy led set");
        return;
    }

    onoff ? gpio_direction_output(led_use_port[led_index], 1) : gpio_direction_output(led_use_port[led_index], 0);
}

void toggle_led(void *gpio)
{
    static u8 led_state = 0;
    gpio_direction_output((u32)gpio, led_state);
    led_state = !led_state;
}

void led_blink_worker_on(u32 gpio)
{
    printf("led_blink_worker_on");
    if (led_blink_timer) {
        sys_timer_del(led_blink_timer);
    }
    led_blink_timer = sys_timer_add((void *)gpio, toggle_led, 100);
}

void led_blink_worker_off(u32 gpio)
{
    printf("led_blink_worker_off");
    if (led_blink_timer) {
        sys_timer_del(led_blink_timer);
        led_blink_timer = 0;
    }
    gpio_set_direction(gpio, 1);
}

static void generate_bt_address(u8 addr[6])
{
    u8 i;

    for (i = 0; i < 6;) {
        addr[i++] = JL_RAND->R64L;
        addr[i++] = JL_RAND->R64H;
    }
}

void bt_mac_addr_set(u8 *bt_addr)
{
    int err;
    u8 mac_addr[6];

    if (!bt_addr) {
        memcpy(mac_addr, (void *)bt_get_mac_addr(), 6);
        le_controller_set_mac((void *)mac_addr);
        log_info("bt MAC from flash: %s", bd_addr_to_str(mac_addr));
    } else {
        le_controller_set_mac(bt_addr);
        log_info("bt MAC from default: %s", bd_addr_to_str(bt_addr));
    }
}

void bt_ble_adv_enable(u8 enable)
{
    if (0 == enable) {
        ble_user_cmd_prepare(BLE_CMD_ADV_ENABLE, 1, enable);
        ble_user_cmd_prepare(BLE_CMD_SCAN_ENABLE, 1, enable);
    }
}

void bt_ble_exit(void)
{
    bt_ble_adv_enable(0);
}

void ble_module_enable(u8 en)
{
    log_info("mode_en:%d\n", en);
    if (en) {
        bt_ble_adv_enable(1);
    } else {
        ble_app_disconnect();
        bt_ble_adv_enable(0);
    }
}
/* void ble_module_enable(u8 en) */
/* { */
/*     bt_ble_adv_enable(en); */
/* } */

void ble_profile_init(void)
{
    log_info("--func=%s", __FUNCTION__);
}

#if (TMALL_UPDATE_TOOL == 1)

typedef struct __flash_of_ali_para_head {
    s16 crc;
    u16 string_len;
    const u8 para_string[];
} __attribute__((packed)) _flash_of_ali_para_head;

static u32 tmall_license_ptr(void)
{
    u32 flash_capacity = sdfile_get_disk_capacity();
    u32 auth_addr = flash_capacity - 256 + 80;
    log_debug("flash capacity:%x \n", flash_capacity);
    return sdfile_flash_addr2cpu_addr(auth_addr);
}

static bool tmall_ali_para_head_check(const u8 *para)
{
    _flash_of_ali_para_head *head;

    //fill head
    head = (_flash_of_ali_para_head *)para;

    ///crc check
    u8 *crc_data = (u8 *)(para + sizeof(((_flash_of_ali_para_head *)0)->crc));
    u32 crc_len = sizeof(_flash_of_ali_para_head) - sizeof(((_flash_of_ali_para_head *)0)->crc)/*head crc*/ + (head->string_len)/*content crc,include end character '\0'*/;
    s16 crc_sum = 0;

    crc_sum = CRC16(crc_data, crc_len);

    if (crc_sum != head->crc) {
        log_debug("gma crc error !!! %x %x \n", (u32)crc_sum, (u32)head->crc);
        return false;
    }

    return true;
}

static void tmall_triad_set(u8 *uuid)
{
    u8 *auth_ptr = (u8 *)tmall_license_ptr();

    _flash_of_ali_para_head *head;

    put_buf(auth_ptr, 128);

    //head length check
    head = (_flash_of_ali_para_head *)auth_ptr;
    if (head->string_len >= 0xff) {
        log_debug("gma license length error !!! \n");
        return;
    }

    ////crc check
    if (tmall_ali_para_head_check(auth_ptr) == (false)) {
        return;
    }

    ///jump to context
    auth_ptr += sizeof(_flash_of_ali_para_head);

    int i;
    u32_t pid = 0;
    u8 *auth_ptr_store = auth_ptr;

    //printf("--0");
    //printf_buf(auth_ptr, 8);

    u8 hex;
    for (i = 0; i < 8; i++) {
        hex = *(auth_ptr + i);
        if ((hex >= '0') && (hex <= '9')) {
            hex -= '0';
        } else if ((hex >= 'a') && (hex <= 'f')) {
            hex = hex - 'a' + 10;
        } else {

        }

        pid |= hex << (28 - 4 * i);
    }

    //printf("pid = 0x%x\n", pid);

    auth_ptr += 8 + 1;

    //printf("--1");
    //printf_buf(auth_ptr, 12);

    u8_t mac[6] = {0};
    u8 hex_b;
    for (i = 0; i < 6; i++) {
        hex = *(auth_ptr + i * 2);
        hex_b = *(auth_ptr + i * 2 + 1);

        if ((hex >= '0') && (hex <= '9')) {
            hex -= '0';
        } else if ((hex >= 'a') && (hex <= 'f')) {
            hex = hex - 'a' + 10;
        }

        if ((hex_b >= '0') && (hex_b <= '9')) {
            hex_b -= '0';
        } else if ((hex_b >= 'a') && (hex_b <= 'f')) {
            hex_b = hex_b - 'a' + 10;
        }

        mac[i] = (hex << 4) | hex_b;
    }
    //printf("before change\n");
    //printf_buf(mac, 6);

    pid = little_endian_read_32((u8 *)&pid, 0);

    u8 mac_temp[6];
    reverse_bd_addr(mac, mac_temp);
    bt_mac_addr_set(mac_temp);

    //printf("after change\n");
    memcpy(uuid + 3, &pid, 4);
    memcpy(uuid + 7, mac_temp,  6);

    //printf("pid = 0x%x\n", pid);
    //printf_buf(mac_temp, 6);

    //tmall_auth_value_calculate
    u8 digest[SHA256_DIGEST_SIZE];

    sha256Compute(auth_ptr_store, (4 + 6 + 16) * 2 + 2, digest);

    //printf_buf(digest, SHA256_DIGEST_SIZE);

    extern void auth_data_change(u8 * c_auth_data);
    auth_data_change(digest);
}

void set_triad(u8 *uuid)
{
    tmall_triad_set(uuid);
}

#endif

