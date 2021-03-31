/**
 * @file    sample_rpmsg.c
 * @brief   rpmsg sample program for FreeRTOS
 * @date    2020.03.24
 * @author  Copyright (c) 2020, eForce Co., Ltd. All rights reserved.
 *
 ****************************************************************************
 * @par     History
 *          - rev 1.0 (2020.01.28) Imada
 *            Initial version.
 *          - rev 1.1 (2020.03.24) Imada
 *            Employed a dedicated function to set a string for Shared memory API.
 ****************************************************************************
 */

#include <metal/sleep.h>
#include "FreeRTOS.h"
#include "openamp/open_amp.h"
#include "platform_info.h"
#include "rsc_table.h"

extern int init_system(void);
extern void cleanup_system(void);

#define SHUTDOWN_MSG    (0xEF56A55A)
#define RECONNECT_FLG   (1) /* 1:reconnect after exit, 0:disconnect after exit */
#define RECONNECT_DLY   (10000u)

/* Local variables */

/*-----------------------------------------------------------------------------*
 *  RPMSG callbacks setup by remoteproc_resource_init()
 *-----------------------------------------------------------------------------*/
/* Local variables */

static struct rpmsg_endpoint rp_ept[CFG_RPMSG_SVCNO] = { 0 };

volatile static int evt_svc_unbind[CFG_RPMSG_SVCNO] = { 0 };


/**
 *  Callback Function: rpmsg_endpoint_cb
 *
 *  @param[in] rp_svc
 *  @param[in] data
 *  @param[in] len
 *  @param[in] priv
 *  @param[in] src
 */
static int rpmsg_endpoint_cb0(struct rpmsg_endpoint *cb_rp_ept, void *data, size_t len, uint32_t src, void *priv)
{
    /* service 0 */
    (void)priv;
    int ret = 0;
    struct rpmsg_endpoint *cb_rp_ept_test;
    src = 0x501;
    uint32_t dst = 0x502;
    /* On reception of a shutdown we signal the application to terminate */
    if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
        evt_svc_unbind[0] = 1;
        return RPMSG_SUCCESS;
    }

    /* $$$$ Test Point 7.2.2 $$$$ */
    cb_rp_ept_test->rdev = NULL;
    ret = rpmsg_trysend_offchannel(cb_rp_ept_test, src, dst, data, (int)len);
    if (ret < 0) {
        LPERROR("rdev = NULL -> rpmsg_trysend_offchannel failed %d\n", ret);
        if (ret == RPMSG_ERR_PARAM)
            printf_raw("TC 7.2.2 is PASS\n");
        else 
            printf_raw("TC 7.2.2 is FAILED\n");
    }

    /* $$$$ Test Point $$$$ */
    ret = rpmsg_trysend_offchannel(cb_rp_ept, src, RPMSG_ADDR_ANY, data, (int)len);
    if (ret < 0) {
        LPERROR("dst = RPMSG_ADDR_ANY -> rpmsg_trysend_offchannel failed %d\n", ret);
        if (ret == RPMSG_ERR_PARAM)
            printf_raw("TC 7.2.2 is PASS\n");
        else 
            printf_raw("TC 7.2.2 is FAILED\n");
    }

    /* $$$$ Test Point $$$$ */
    ret = rpmsg_trysend_offchannel(cb_rp_ept, src, dst, NULL, (int)len);
    if (ret < 0) {
        LPERROR("data = NULL -> rpmsg_trysend_offchannel failed %d\n", ret);
        if (ret == RPMSG_ERR_PARAM)
            printf_raw("TC 7.2.3 is PASS\n");
        else 
            printf_raw("TC 7.2.3 is FAILED\n");
    }

    /* $$$$ Test Point 1.2.1 $$$$ */
    /* Send data back to master */
    if (rpmsg_trysend_offchannel(cb_rp_ept, src, dst, data, (int)len) < 0) {
        LPERROR("rpmsg_trysend_offchannel failed\n");
        return -1;
    }
    else
        printf_raw("rpmsg_trysend_offchannel -> return RPMSG_SUCCESS \n TC 7.2.1 is PASS\n");

    return RPMSG_SUCCESS;
}

static int rpmsg_endpoint_cb1(struct rpmsg_endpoint *cb_rp_ept, void *data, size_t len, uint32_t src, void *priv)
{
    /* service 1 */
    (void)priv;
    (void)src;

    /* On reception of a shutdown we signal the application to terminate */
    if ((*(unsigned int *)data) == SHUTDOWN_MSG) {
        evt_svc_unbind[1] = 1;
        return RPMSG_SUCCESS;
    }

    /* Send data back to master */
    if (rpmsg_send(cb_rp_ept, data, (int)len) < 0) {
        LPERROR("rpmsg_send failed\n");
        return -1;
    }
    return RPMSG_SUCCESS;
}

/**
 *  Callback Function: rpmsg_service_unbind
 *
 *  @param[in] ept
 */
static void rpmsg_service_unbind0(struct rpmsg_endpoint *ept)
{
    (void)ept;
    /* service 0 */
    rpmsg_destroy_ept(&rp_ept[0]);
    memset(&rp_ept[0], 0x0, sizeof(struct rpmsg_endpoint));
    evt_svc_unbind[0] = 1;
    return ;
}

static void rpmsg_service_unbind1(struct rpmsg_endpoint *ept)
{
    (void)ept;
    /* service 1 */  
    rpmsg_destroy_ept(&rp_ept[1]);
    memset(&rp_ept[1], 0x0, sizeof(struct rpmsg_endpoint));
    evt_svc_unbind[1] = 1;
    return ;
}

/*-----------------------------------------------------------------------------*
 *  Application
 *-----------------------------------------------------------------------------*/
int app(struct rpmsg_device *rdev, void *platform, unsigned long svcno)
{
    (void)platform;
    int ret;

    if (svcno == 0UL) {
        ret = rpmsg_create_ept(&rp_ept[0], rdev, CFG_RPMSG_SVC_NAME0,
                       APP_EPT_ADDR, RPMSG_ADDR_ANY,
                       rpmsg_endpoint_cb0,
                       rpmsg_service_unbind0);
        if (ret) {
            LPERROR("Failed to create endpoint.\n");
            return -1;
        }
    } else {
        ret = rpmsg_create_ept(&rp_ept[1], rdev, CFG_RPMSG_SVC_NAME1,
                       APP_EPT_ADDR, RPMSG_ADDR_ANY,
                       rpmsg_endpoint_cb1,
                       rpmsg_service_unbind1);
        if (ret) {
            LPERROR("Failed to create endpoint.\n");
            return -1;
        }
    }

    LPRINTF("Waiting for events...\n");
    while(1) {
        vTaskDelay(0);
        /* we got a shutdown request, exit */
        if (evt_svc_unbind[svcno]) {
            break;
        }
    }
    /* Clear shutdown flag */
    evt_svc_unbind[svcno] = 0;
    printf_raw("\tTesting CR7: App\n");
    return 0;
}

/*******************************
   @    Main Task
 *******************************/
void MainTask(void* exinf)
{
    unsigned long proc_id = (unsigned long)exinf;
    unsigned long rsc_id = (unsigned long)exinf;
    struct rpmsg_device *rpdev;
    void *platform;
    int ret;
    
    ret = platform_init(proc_id, rsc_id, &platform);
    if (ret) {
        LPERROR("Failed to create remoteproc device.\n");
        goto err1;
    } else {
        do {
            /* RTOS is Master, but this setting must remote in this release. */
            rpdev = platform_create_rpmsg_vdev(platform,
                                               0x0U,
                                               VIRTIO_DEV_SLAVE,
                                               NULL,
                                               NULL);
            if (!rpdev) {
                LPERROR("Fail, platform_create_rpmsg_vdev.\n");
                metal_log(METAL_LOG_INFO, "Fail, platform_create_rpmsg_vdev.");      
                goto err2 ;
            }

            /* Kick the application */
            (void)app(rpdev, platform, proc_id);

            LPRINTF("De-initializating remoteproc\n");
            platform_release_rpmsg_vdev(platform, rpdev);

            vTaskDelay(RECONNECT_DLY);
        } while(RECONNECT_FLG);
    }
err2:
    platform_cleanup(platform);    
err1:
    vTaskDelete(NULL);
    return ;
}

void RpmsgTaskStart(uint16_t usStackSize, UBaseType_t uxPriority)
{
    int ret;

    ret = init_system();
    if (ret) {
        return ;
    }
    
    (void)xTaskCreate(MainTask, "MainTask#0", usStackSize, (void*)0, uxPriority, NULL); /* for RPMSG service #0 */
    (void)xTaskCreate(MainTask, "MainTask#1", usStackSize, (void*)1, uxPriority, NULL); /* for RPMSG service #1 */

    return ;
}
