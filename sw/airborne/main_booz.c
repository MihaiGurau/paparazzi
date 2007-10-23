#include "std.h"
#include "init_hw.h"
#include "interrupt_hw.h"
#include "sys_time.h"
#include "led.h"

#include "commands.h"
#include "actuators.h"
#include "radio_control.h"

#include "spi.h"
#include "link_imu.h"
#include "booz_estimator.h"
#include "booz_control.h"
#include "booz_autopilot.h"

#include "uart.h"
#include "messages.h"
#include "downlink.h"
#include "booz_telemetry.h"
#include "datalink.h"


static inline void main_init( void );
static inline void main_periodic_task( void );
static inline void main_event_task( void );

int16_t trim_p = 0;
int16_t trim_q = 0;
int16_t trim_r = 0;

int main( void ) {
  main_init();
  while(1) {
    if (sys_time_periodic())
      main_periodic_task();
    main_event_task();
  }
  return 0;
}

static inline void main_init( void ) {
  hw_init();
  led_init();
  sys_time_init();

  actuators_init();
  SetCommands(commands_failsafe);

  ppm_init();
  radio_control_init();

  spi_init();
  link_imu_init();
  
  booz_estimator_init();
  booz_control_init();
  booz_autopilot_init();

  uart1_init_tx();

  int_enable();
}

static inline void main_periodic_task( void ) {
  
  link_imu_periodic_task();
  
  booz_autopilot_periodic_task();

  SetActuatorsFromCommands(commands);

  static uint8_t _50hz = 0;
  _50hz++;
  if (_50hz > 5) _50hz = 0;
  switch (_50hz) {
  case 0:
    LED_TOGGLE(1);
    break;
  case 1:
    radio_control_periodic_task();
    if (rc_status != RC_OK)
      booz_autopilot_mode = BOOZ_AP_MODE_FAILSAFE;
    break;
  case 2:
    booz_telemetry_periodic_task();
    break;
  case 3:
    break;
  case 4:
    break;
  }


}

static inline  void main_event_task( void ) {
  
  DlEventCheckAndHandle();

  LinkImuEventCheckAndHandle();

  if (ppm_valid) {
    ppm_valid = FALSE;
    radio_control_event_task();
    //    if (rc_values_contains_avg_channels) {
    //      fbw_mode = FBW_MODE_OF_PPRZ(rc_values[RADIO_MODE]);
    //    }
    booz_autopilot_mode = BOOZ_AP_MODE_RATE;
    /* setpoints */
    booz_control_rate_compute_setpoints();
  }

}
