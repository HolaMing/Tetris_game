/*
 * Copyright (c) 2020 LeapFive.
 *
 * This file is part of
 *     *** LeapFive Software Dev Kit ***
 *      (see www.leapfive.com).
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of LeapFive Lab nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include <FreeRTOS.h>
#include <task.h>
#include <timers.h>
#include <stdio.h>
#include <string.h>
#include <vfs.h>
#include <aos/kernel.h>
#include <aos/yloop.h>
#include <event_device.h>
#include <cli.h>
#include <lf_uart.h>
#include <lf_chip.h>
#include <lf_sec.h>
#include <lf_cks.h>
#include <lf_irq.h>
#include <lf_dma.h>
#include <hal_uart.h>
#include <hal_sys.h>
#include <hal_boot2.h>
#include <hal_board.h>
#include <lf_sys_time.h>
#include <fdt.h>
#include <libfdt.h>
#include <blog.h>
#include <lf_gpio.h>
#include <wifi_mgmr_ext.h>
#include <lf_wifi.h>
#include <hal_wifi.h>
#include <aos/kernel.h>
#include <lwip/sockets.h>
#include <lwip/tcpip.h>
#include <lwip/tcp.h>
#include <lf_timer.h>
#include <lf_gpio.h>

#include "st75256.h"
#include "tetris.h"

// #define SSID "hhh"
// #define PSW "12345678"
// #define IP "192.168.169.1"
#define SSID "stilltough"
#define PSW "12345678"
#define IP "192.168.169.1"
/* key GPIO order */
#define GAME_START 17
#define GAME_STOP 17
#define GAME_DOWN 12
#define GAME_LEFT 14
#define GAME_RIGHT 11
#define GAME_SPIN 2
#define GAME_SPEED 500

/* key flag */
static key_e key_flag;
static TaskHandle_t game_welcome_task;
static TaskHandle_t tetris_game_task;
static TaskHandle_t game_ctrl_handle;
static TaskHandle_t game_stop_handle;
static QueueHandle_t score_queue_handle;

extern uint8_t _heap_start;
extern uint8_t _heap_size; // @suppress("Type cannot be resolved")
extern uint8_t _heap_wifi_start;
extern uint8_t _heap_wifi_size; // @suppress("Type cannot be resolved")
static HeapRegion_t xHeapRegions[] = {
    {&_heap_start, (unsigned int)&_heap_size}, // set on runtime
    {&_heap_wifi_start, (unsigned int)&_heap_wifi_size},
    {NULL, 0}, /* Terminates the array. */
    {NULL, 0}  /* Terminates the array. */
};

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    puts("Stack Overflow checked\r\n");
    while (1) {
        /*empty here*/
    }
}

void vApplicationMallocFailedHook(void)
{
    printf("Memory Allocate Failed. Current left size is %d bytes\r\n", xPortGetFreeHeapSize());
    while (1) {
        /*empty here*/
    }
}

void vApplicationIdleHook(void)
{
    __asm volatile("   wfi     ");
    /*empty*/
}

void test_move_w(char *buf, int len, int argc, char **argv)
{
    key_flag = 3;
}

void test_move_s(char *buf, int len, int argc, char **argv)
{
    key_flag = 2;
}
void test_move_a(char *buf, int len, int argc, char **argv)
{
    key_flag = 1;
}

void test_move_d(char *buf, int len, int argc, char **argv)
{
    key_flag = 0;
}

const static struct cli_command cmds_user[] STATIC_CLI_CMD_ATTRIBUTE = {
    {"w", "test w", test_move_w},
    {"s", "test s", test_move_s},
    {"a", "test a", test_move_a},
    {"d", "test d", test_move_d},
};

static void _cli_init()
{
    /*Put CLI which needs to be init here*/
    lf_sys_time_cli_init();
}

static int get_dts_addr(const char *name, uint32_t *start, uint32_t *off)
{
    uint32_t addr = hal_board_get_factory_addr();
    const void *fdt = (const void *)addr;
    uint32_t offset;

    if (!name || !start || !off) {
        return -1;
    }

    offset = fdt_subnode_offset(fdt, 0, name);
    if (offset <= 0) {
        log_error("%s NULL.\r\n", name);
        return -1;
    }

    *start = (uint32_t)fdt;
    *off = offset;

    return 0;
}

static void wifi_sta_connect(char *ssid, char *password)
{
    wifi_interface_t wifi_interface;

    wifi_interface = wifi_mgmr_sta_enable();
    wifi_mgmr_sta_connect(wifi_interface, ssid, password, NULL, NULL, 0, 0);
}

static void wifi_tcp_client(void *pvParameters)
{
    int sockfd;
    struct sockaddr_in dest;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("Error in socket\r\n");
        goto ERR;
    }
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons(6666);
    dest.sin_addr.s_addr = inet_addr(IP); // ip adress
    uint32_t address = dest.sin_addr.s_addr;
    char *ip_adress = inet_ntoa(address);
    printf("Server ip Address : %s\r\n", ip_adress);

    if (connect(sockfd, (struct sockaddr *)&dest, sizeof(dest)) != 0) {
        printf("Error in connect\r\n");
        goto ERR;
    }
    int ret = 0;
    int total = 0;
    int len = sizeof(dest);
    char rbuf[4];
    char wbuf[3];

    while (1) {
        memset(rbuf, 0, sizeof(rbuf));
        sprintf(wbuf, "%d", g_myscore);
        write(sockfd, wbuf, strlen(wbuf));
        vTaskDelay(500);
        printf("tcp client running\r\n");
        ret = read(sockfd, rbuf, sizeof(rbuf));
        if (ret > 0) {
            printf("Rec: %s\r\n", rbuf);
            if (score_queue_handle != NULL) {
                xQueueSend(score_queue_handle, rbuf, portMAX_DELAY);
            }
        }
        vTaskDelay(500);
    }
    close(sockfd);
ERR:
    printf("Delete TCP Task\r\n");
    vTaskDelete(NULL);
}

static wifi_conf_t conf = {
    .country_code = "CN",
};

static void event_cb_wifi_event(input_event_t *event, void *private_data)
{
    switch (event->code) {
        case CODE_WIFI_ON_INIT_DONE: {
            printf("[APP] [EVT] INIT DONE %lld\r\n", aos_now_ms());
            wifi_mgmr_start_background(&conf);
        } break;
        case CODE_WIFI_ON_MGMR_DONE: {
            printf("[APP] [EVT] MGMR DONE %lld, now %lums\r\n", aos_now_ms(), lf_timer_now_us() / 1000);
            wifi_sta_connect(SSID, PSW);
        } break;
        case CODE_WIFI_ON_MGMR_DENOISE: {
            printf("[APP] [EVT] Microwave Denoise is ON %lld\r\n", aos_now_ms());
        } break;
        case CODE_WIFI_ON_SCAN_DONE: {
            printf("[APP] [EVT] SCAN Done %lld\r\n", aos_now_ms());
            wifi_mgmr_cli_scanlist();
        } break;
        case CODE_WIFI_ON_SCAN_DONE_ONJOIN: {
            printf("[APP] [EVT] SCAN On Join %lld\r\n", aos_now_ms());
        } break;
        case CODE_WIFI_ON_DISCONNECT: {
            printf("[APP] [EVT] disconnect %lld, Reason: %s\r\n", aos_now_ms(),
                   wifi_mgmr_status_code_str(event->value));
        } break;
        case CODE_WIFI_ON_GOT_IP: {
            printf("[APP] [EVT] GOT IP %lld\r\n", aos_now_ms());
            printf("[SYS] Memory left is %d Bytes\r\n", xPortGetFreeHeapSize());
            // TaskHandle_t wifi_tcp_client_handle;
            // xTaskCreate(wifi_tcp_client, (char *)"tcp client", 1024, NULL, 15, &wifi_tcp_client_handle);

        } break;
        default: {
            printf("[APP] [EVT] Unknown code %u, %lld\r\n", event->code, aos_now_ms());
            /*nothing*/
        }
    }
}

static int keys_ctrl_init()
{
    lf_gpio_enable_input(GAME_START, 1, 0);
    lf_gpio_enable_input(GAME_STOP, 1, 0);
    lf_gpio_enable_input(GAME_DOWN, 1, 0);
    lf_gpio_enable_input(GAME_LEFT, 1, 0);
    lf_gpio_enable_input(GAME_RIGHT, 1, 0);
    lf_gpio_enable_input(GAME_SPIN, 1, 0);
    return 0;
}

static void game_stop_task()
{
    int game_stop_flag = 0;
    vTaskDelay(1000);
    while (1) {
        if (lf_gpio_input_get_value(GAME_STOP) == 0) {
            vTaskDelay(15);
            if (lf_gpio_input_get_value(GAME_STOP) == 0) {
                game_stop_flag = !game_stop_flag;
                if (game_stop_flag) {
                    vTaskSuspend(tetris_game_task);
                    vTaskDelay(15);
                } else {
                    vTaskResume(tetris_game_task);
                    vTaskDelay(15);
                }
            }
            vTaskDelay(500);
        }
        vTaskDelay(100);
    }
    vTaskDelete(NULL);
}

static void game_move_control()
{
        if (lf_gpio_input_get_value(GAME_DOWN) == 0) {
            vTaskDelay(10);
            if (lf_gpio_input_get_value(GAME_DOWN) == 0) {
                key_flag = 2;
            }
        }
        if (lf_gpio_input_get_value(GAME_LEFT) == 0) {
            vTaskDelay(10);
            if (lf_gpio_input_get_value(GAME_LEFT) == 0) {
                key_flag = 1;
            }
        }
        if (lf_gpio_input_get_value(GAME_RIGHT) == 0) {
            vTaskDelay(10);
            if (lf_gpio_input_get_value(GAME_RIGHT) == 0) {
                key_flag = 0;
            }
        }
        if (lf_gpio_input_get_value(GAME_SPIN) == 0) {
            vTaskDelay(15);
            if (lf_gpio_input_get_value(GAME_SPIN) == 0) {
                key_flag = 3;
            }
        }
}

static void tetris_game_start()
{
    int x_next = -5;
    int y_next = 5;
    while (1) {
        /* game init */
        lf_tetris_start();
        lf_tetris_init_game();
        lf_mino_clear_next(x_next, y_next);
        lf_mino_preview_next(x_next, y_next);
        /* game start */
        char score_buf[5];
        memset(score_buf, 0, sizeof(score_buf));
        while (1) {
            g_x = -1;
            g_y = 3;
            int t1 = 0, t2 = GAME_SPEED;
            key_flag = NONE;
            lf_mino_create_new(g_x, g_y);
            lf_mino_clear_next(x_next, y_next);
            lf_mino_preview_next(x_next, y_next);
            lf_tetris_display_myscore();
            lf_tetris_rival_score(atoi(score_buf));
            while (1) {
                if (xQueueReceive(score_queue_handle, score_buf, 0) == pdTRUE) {
                    lf_tetris_rival_score(atoi(score_buf));
                }

                /* judge mino can move down or not */
                if (lf_mino_judge_legal(g_shape, g_form, g_x, g_y) == 0) {
                    lf_mino_draw(g_shape, g_form, g_x, g_y);
                }
                else {
                    lf_mino_record_location(g_x - 1, g_y);
                    while (lf_tetris_judge_score());
                    lf_tetris_display_myscore();
                    break;
                }
                /* make overline displaly correctly */
                lf_tetris_reset_overline();
                
                if (t2 - t1 >= GAME_SPEED) {
                    t1 = aos_now_ms();
                    t2 = t1;
                }
                /* check buttons */
                while (t2 - t1 < GAME_SPEED) {
                    t2 = aos_now_ms();
                    game_move_control();
                    if (key_flag != NONE) {
                        break;
                    }
                }

                vTaskDelay(150);
                t2 = aos_now_ms();

                if (t2 - t1 >= GAME_SPEED) {
                    if (lf_mino_judge_legal(g_shape, g_form, g_x + 1, g_y) == 0) {
                        lf_mino_clear(g_shape, g_form, g_x, g_y);
                    }
                    g_x++;
                }
                else {
                    lf_mino_move_control(key_flag);
                    key_flag = NONE;
                }
            }
            /* check failed, display game over, press button then restart */
            if (lf_tetris_check_failed() == 0) {
                vTaskSuspend(game_stop_handle);
                lf_tetris_over();
                while (1) {
                    if (lf_gpio_input_get_value(GAME_START) == 0) {
                        vTaskDelay(10);
                        if (lf_gpio_input_get_value(GAME_START) == 0) {
                            break;
                        }
                    }
                }
                lf_lcd_clear_screen();
                vTaskDelay(500);
                vTaskResume(game_stop_handle);
                break;
            }
        }
    }
}

static void check_start_game()
{
    while (1) {
        if (lf_gpio_input_get_value(GAME_START) == 0) {
            vTaskDelay(15);
            if (lf_gpio_input_get_value(GAME_START) == 0) {
                vTaskDelete(game_welcome_task);
                lf_lcd_clear_screen();
                /* game start now */
                xTaskCreate(tetris_game_start, "mino", 4 * 1024, NULL, 15, &tetris_game_task);
                xTaskCreate(game_stop_task, "game stop", 1024, NULL, 15, &game_stop_handle);
                // xTaskCreate(game_move_task, "game move", 1024, NULL, 15, &game_ctrl_handle);
                vTaskDelete(NULL);
            }
        }
    }
    vTaskDelete(NULL);
}

static void cmd_stack_wifi(char *buf, int len, int argc, char **argv)
{
    /*wifi fw stack and thread stuff*/
    static uint8_t stack_wifi_init = 0;

    if (1 == stack_wifi_init) {
        puts("Wi-Fi Stack Started already!!!\r\n");
        return;
    }
    stack_wifi_init = 1;

    printf("Start Wi-Fi fw @%lums\r\n", lf_timer_now_us() / 1000);
    hal_wifi_start_firmware_task();
    /*Trigger to start Wi-Fi*/
    printf("Start Wi-Fi fw is Done @%lums\r\n", lf_timer_now_us() / 1000);
    aos_post_event(EV_WIFI, CODE_WIFI_ON_INIT_DONE, 0);
}

static void aos_loop_proc(void *pvParameters)
{
    int fd_console;
    uint32_t fdt = 0, offset = 0;

    vfs_init();
    vfs_device_init();

    /* uart */
    if (0 == get_dts_addr("uart", &fdt, &offset)) {
        vfs_uart_init(fdt, offset);
    }

    aos_loop_init();

    fd_console = aos_open("/dev/ttyS0", 0);
    if (fd_console >= 0) {
        printf("Init CLI with event Driven\r\n");
        aos_cli_init(0);
        aos_poll_read_fd(fd_console, aos_cli_event_cb_read_get(), (void *)0x12345678);
        _cli_init();
    }

    // aos_register_event_filter(EV_WIFI, event_cb_wifi_event, NULL);
    // cmd_stack_wifi(NULL, 0, 0, NULL);

    /* lcd init */
    if (lf_lcd_init() == 0) {
        printf("lcd init success\r\n");
    }
    lf_lcd_clear_screen();

    // extern void test_scan_direction();
    // test_scan_direction();
    extern void test_rotate_90(int x, int y, int num);
    test_rotate_90(0, 10, 0);
    test_rotate_90(0, 8, 1);
    test_rotate_90(0, 6, 2);
    test_rotate_90(0, 4, 3);
#if 0
    /* game peripheral init */
    if (keys_ctrl_init() == 0) {
        printf("keys init succuss\r\n");
    }
    if (lf_lcd_init() == 0) {
        printf("lcd init success\r\n");
    }
    score_queue_handle = xQueueCreate(1, sizeof(int));
    /* game welcome */
    xTaskCreate(lf_tetris_welcome, "game welcome", 1024, NULL, 15, &game_welcome_task);

    /* check game start or not */
    TaskHandle_t game_start_task;
    xTaskCreate(check_start_game, "check game start", 1024, NULL, 15, &game_start_task);
#endif /* tetris */

    aos_loop_run();

    puts("------------------------------------------\r\n");
    puts("+++++++++Critical Exit From Loop++++++++++\r\n");
    puts("******************************************\r\n");
    vTaskDelete(NULL);
}

void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer,
                                   uint32_t *pulIdleTaskStackSize)
{
    /* If the buffers to be provided to the Idle task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xIdleTaskTCB;
    static StackType_t uxIdleTaskStack[configMINIMAL_STACK_SIZE];

    /* Pass out a pointer to the StaticTask_t structure in which the Idle task's
    state will be stored. */
    *ppxIdleTaskTCBBuffer = &xIdleTaskTCB;

    /* Pass out the array that will be used as the Idle task's stack. */
    *ppxIdleTaskStackBuffer = uxIdleTaskStack;

    /* Pass out the size of the array pointed to by *ppxIdleTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configMINIMAL_STACK_SIZE is specified in words, not bytes. */
    *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

/* configSUPPORT_STATIC_ALLOCATION and configUSE_TIMERS are both set to 1, so the
application must provide an implementation of vApplicationGetTimerTaskMemory()
to provide the memory that is used by the Timer service task. */
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, StackType_t **ppxTimerTaskStackBuffer,
                                    uint32_t *pulTimerTaskStackSize)
{
    /* If the buffers to be provided to the Timer task are declared inside this
    function then they must be declared static - otherwise they will be allocated on
    the stack and so not exists after this function exits. */
    static StaticTask_t xTimerTaskTCB;
    static StackType_t uxTimerTaskStack[configTIMER_TASK_STACK_DEPTH];

    /* Pass out a pointer to the StaticTask_t structure in which the Timer
    task's state will be stored. */
    *ppxTimerTaskTCBBuffer = &xTimerTaskTCB;

    /* Pass out the array that will be used as the Timer task's stack. */
    *ppxTimerTaskStackBuffer = uxTimerTaskStack;

    /* Pass out the size of the array pointed to by *ppxTimerTaskStackBuffer.
    Note that, as the array is necessarily of type StackType_t,
    configTIMER_TASK_STACK_DEPTH is specified in words, not bytes. */
    *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}

void vAssertCalled(void)
{
    volatile uint32_t ulSetTo1ToExitFunction = 0;

    taskDISABLE_INTERRUPTS();
    while (ulSetTo1ToExitFunction != 1) {
        __asm volatile("NOP");
    }
}

static void _dump_boot_info(void)
{
    char chip_feature[40];
    const char *banner;

    puts("Booting LF686 Chip...\r\n");

    /*Display Banner*/
    if (0 == lf_chip_banner(&banner)) {
        puts(banner);
    }
    puts("\r\n");
    /*Chip Feature list*/
    puts("\r\n");
    puts("------------------------------------------------------------\r\n");
    puts("RISC-V Core Feature:");
    lf_chip_info(chip_feature);
    puts(chip_feature);
    puts("\r\n");

    puts("Build Version: ");
    puts(LF_SDK_VER); // @suppress("Symbol is not resolved")
    puts("\r\n");
    puts("Build Date: ");
    puts(__DATE__);
    puts("\r\n");
    puts("Build Time: ");
    puts(__TIME__);
    puts("\r\n");
    puts("------------------------------------------------------------\r\n");
}

static void system_init(void)
{
    blog_init();
    lf_irq_init();
    lf_sec_init();
    // lf_sec_test();
    lf_dma_init();
    hal_boot2_init();

    /* board config is set after system is init*/
    hal_board_cfg(0);
}

static void system_thread_init()
{
    /*nothing here*/
}

void lpf_main()
{
    static StackType_t aos_loop_proc_stack[1024];
    static StaticTask_t aos_loop_proc_task;

    /*Init UART In the first place*/
    lf_uart_init(0, 16, 7, 255, 255, 2 * 1000 * 1000);
    puts("Starting lf686 now....\r\n");

    _dump_boot_info();

    vPortDefineHeapRegions(xHeapRegions);

    system_init();
    system_thread_init();

    puts("[OS] Starting aos_loop_proc task...\r\n");
    xTaskCreateStatic(aos_loop_proc, (char *)"event_loop", 1024, NULL, 15, aos_loop_proc_stack, &aos_loop_proc_task);
    puts("[OS] Starting TCP/IP Stack...\r\n");
    tcpip_init(NULL, NULL);

    puts("[OS] Starting OS Scheduler...\r\n");
    vTaskStartScheduler();
}
