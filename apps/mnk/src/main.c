/*
 * See LICENSE for details
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <os/mynewt.h>

#include <bsp/bsp.h>
#include <hal/hal_gpio.h>

#include <console/console.h>
#include <shell/shell.h>
#include <log/log.h>
#include <imgmgr/imgmgr.h>

/* BLE */
#include <nimble/ble.h>
#include <host/ble_hs.h>
#include <services/gap/ble_svc_gap.h>
#include <services/bas/ble_svc_bas.h>
#include <services/dis/ble_svc_dis.h>

/* Newtmgr include */
#include <newtmgr/newtmgr.h>
#include <nmgrble/newtmgr_ble.h>

/* RAM persistence layer. */
#include <store/ram/ble_store_ram.h>

/* Adafruit libraries and helpers */
#include <adafruit/adautil.h>
#include <adafruit/bledis.h>
#include <adafruit/bleuart.h>

/** Default device name */
#define CFG_GAP_DEVICE_NAME     "Adafruit Mynewt"

/*------------------------------------------------------------------*/
/* Global values
 *------------------------------------------------------------------*/
#define LED_RED   LED_BLINK_PIN
#define LED_BLUE  LED_2

/*------------------------------------------------------------------*/
/* TASK Settings
 *------------------------------------------------------------------*/

/* BLEUART to UART bridge task */
#define BLEUART_BRIDGE_NAME           "bleuart"
#define BLEUART_BRIDGE_TASK_PRIO      5
#define BLEUART_BRIDGE_STACK_SIZE     OS_STACK_ALIGN(256)

struct os_task bleuart_bridge_task;
os_stack_t bleuart_bridge_stack[BLEUART_BRIDGE_STACK_SIZE];

/*------------------------------------------------------------------*/
/* ADA Config
 *------------------------------------------------------------------*/
/* Group config Data to one struct */
struct
{
  char devname[32];
}cfgdata =
{
    .devname = CFG_GAP_DEVICE_NAME
};

/* Note when saving to flash group name will be added as prefix to each variable name
 * e.g
 * - group = "adafruit", variable is "devname"
 * - "adafruit/devname" will be save to flash
 */
const adacfg_info_t cfg_info[] =
{
    /* Name, Type, Size, Buffer */
    { "devname", CONF_STRING, 32, cfgdata.devname },

    { 0 } /* Zero for null-terminator */
};

/*------------------------------------------------------------------*/
/* Functions prototypes
 *------------------------------------------------------------------*/
static int btle_gap_event(struct ble_gap_event *event, void *arg);

/**
 * Enables advertising with the following parameters:
 *     o General discoverable mode.
 *     o Undirected connectable mode.
 */
static void btle_advertise(void)
{
  struct ble_hs_adv_fields fields =
  {
      .flags                 = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP,

      /* Indicate that the TX power level field should be included; have the
       * stack fill this one automatically as well.  This is done by assiging the
       * special value BLE_HS_ADV_TX_PWR_LVL_AUTO. */
      .tx_pwr_lvl_is_present = 1,
      .tx_pwr_lvl            = BLE_HS_ADV_TX_PWR_LVL_AUTO,

      .uuids128              = (ble_uuid128_t*) &BLEUART_UUID_SERVICE,
      .num_uuids128          = 1,
      .uuids128_is_complete  = 0,
  };

  VERIFY_STATUS(ble_gap_adv_set_fields(&fields), RETURN_VOID);

  /*------------- Scan response data -------------*/
  const char *name = ble_svc_gap_device_name();
  struct ble_hs_adv_fields rsp_fields =
  {
      .name             = (uint8_t*) name,
      .name_len         = strlen(name),
      .name_is_complete = 1
  };
  ble_gap_adv_rsp_set_fields(&rsp_fields);

  /* Begin advertising. */
  struct ble_gap_adv_params adv_params;
  memset(&adv_params, 0, sizeof adv_params);
  adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
  adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
  VERIFY_STATUS(ble_gap_adv_start(BLE_OWN_ADDR_PUBLIC, NULL, BLE_HS_FOREVER, &adv_params, btle_gap_event, NULL),
                RETURN_VOID);
}

/**
 * The nimble host executes this callback when a GAP event occurs.  The
 * application associates a GAP event callback with each connection that forms.
 * bleprph uses the same callback for all connections.
 *
 * @param event                 The type of event being signalled.
 * @param ctxt                  Various information pertaining to the event.
 * @param arg                   Application-specified argument; unuesd by
 *                                  bleprph.
 *
 * @return                      0 if the application successfully handled the
 *                                  event; nonzero on failure.  The semantics
 *                                  of the return code is specific to the
 *                                  particular GAP event being signalled.
 */
static int btle_gap_event(struct ble_gap_event *event, void *arg)
{
  switch ( event->type )
  {
    case BLE_GAP_EVENT_CONNECT:
      /* A new connection was established or a connection attempt failed. */
      if ( event->connect.status == 0 )
      {
        bleuart_set_conn_handle(event->connect.conn_handle);
      }
      else
      {
        /* Connection failed; resume advertising. */
        btle_advertise();
      }
      return 0;

    case BLE_GAP_EVENT_DISCONNECT:
      /* Connection terminated; resume advertising. */
      btle_advertise();
      return 0;

  }

  return 0;
}

static void btle_on_sync(void)
{
  /* Begin advertising. */
  btle_advertise();
}

void bleuart_bridge_task_handler(void* arg)
{
  timeout_t blinky_tm;

  // Configure timeout = 1000 ms
  timeout_set(&blinky_tm, 1000);

  hal_gpio_init_out(LED_RED, 0);

  while(1)
  {
    int ch;

    // Get data from bleuart to hwuart
    if ( (ch = bleuart_getc()) != EOF )
    {
      console_write( (char*)&ch, 1);
    }

    // Blink LED if timer expired
    if ( timeout_expired(&blinky_tm) )
    {
      hal_gpio_toggle(LED_RED);
      timeout_periodic_reset(&blinky_tm);
    }

    // Sleep (should be yield)
    os_time_delay(1);
  }
}


int main(void)
{
  /* Initialize OS */
  sysinit();

  /* Set initial BLE device address. */
  /* memcpy(g_dev_addr, (uint8_t[6]){0xAD, 0xAF, 0xAD, 0xAF, 0xAD, 0xAF}, 6); */
  /* Copy from controller: Is this even required? */
  /* ble_hs_id_copy_addr(BLE_ADDR_PUBLIC,&g_dev_addr,NULL); */

  /* Init Config & NFFS */
  adacfg_init("adafruit");
  adacfg_add(cfg_info);

  //------------- Task Init -------------//
  os_task_init(&bleuart_bridge_task, BLEUART_BRIDGE_NAME, bleuart_bridge_task_handler, NULL,
               BLEUART_BRIDGE_TASK_PRIO, OS_WAIT_FOREVER, bleuart_bridge_stack, BLEUART_BRIDGE_STACK_SIZE);

  /* Initialize the BLE host. */
  ble_hs_cfg.sync_cb        = btle_on_sync;

  /* Init BLE Device Information Service */
  bledis_init();

  /* Nordic UART service (NUS) settings */
  bleuart_init();

  /* BLE device information service init */
  ble_svc_dis_model_number_set("mnk.01");
  ble_svc_dis_serial_number_set("00000001");
  ble_svc_dis_firmware_revision_set("0.0.1");
  ble_svc_dis_hardware_revision_set("0.0.1");
  ble_svc_dis_software_revision_set("0.0.1");
  ble_svc_dis_manufacturer_name_set("samveen.in");
  ble_svc_dis_init();

  /* BLE battery service init */
  ble_svc_bas_init();

  /* Set the default device name. */
  VERIFY_STATUS( ble_svc_gap_device_name_set(cfgdata.devname) );

  while (1) {
    os_eventq_run(os_eventq_dflt_get());
  }
  /* Never exit */

  return 0;
}
