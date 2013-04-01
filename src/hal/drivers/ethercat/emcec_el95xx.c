//
//    Copyright (C) 2011 Sascha Ittner <sascha.ittner@modusoft.de>
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
//

#include <linux/ctype.h>

#include "rtapi.h"
#include "rtapi_string.h"

#include "hal.h"

#include "emcec.h"
#include "emcec_el95xx.h"

typedef struct {
  hal_bit_t *power_ok;
  hal_bit_t *overload;
  int power_ok_pdo_os;
  int power_ok_pdo_bp;
  int overload_pdo_os;
  int overload_pdo_bp;
} emcec_el95xx_data_t;

void emcec_el95xx_read(struct emcec_slave *slave, long period);

int emcec_el95xx_init(int comp_id, struct emcec_slave *slave, ec_pdo_entry_reg_t *pdo_entry_regs) {
  emcec_master_t *master = slave->master;
  emcec_el95xx_data_t *hal_data;
  int err;

  // initialize callbacks
  slave->proc_read = emcec_el95xx_read;

  // alloc hal memory
  if ((hal_data = hal_malloc(sizeof(emcec_el95xx_data_t))) == NULL) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "hal_malloc() for slave %s.%s failed\n", master->name, slave->name);
    return -EIO;
  }
  memset(hal_data, 0, sizeof(emcec_el95xx_data_t));
  slave->hal_data = hal_data;

  // initialize POD entries
  EMCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x6000, 0x01, &hal_data->power_ok_pdo_os, &hal_data->power_ok_pdo_bp);
  EMCEC_PDO_INIT(pdo_entry_regs, slave->index, slave->vid, slave->pid, 0x6000, 0x02, &hal_data->overload_pdo_os, &hal_data->overload_pdo_bp);

  // export pins
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->power_ok), comp_id, "%s.%s.%s.power-ok", EMCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.power-ok failed\n", EMCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }
  if ((err = hal_pin_bit_newf(HAL_OUT, &(hal_data->overload), comp_id, "%s.%s.%s.overload", EMCEC_MODULE_NAME, master->name, slave->name)) != 0) {
    rtapi_print_msg(RTAPI_MSG_ERR, EMCEC_MSG_PFX "exporting pin %s.%s.%s.overload failed\n", EMCEC_MODULE_NAME, master->name, slave->name);
    return err;
  }

  // initialize pins
  *(hal_data->power_ok) = 0;
  *(hal_data->overload) = 0;

  return 0;
}

void emcec_el95xx_read(struct emcec_slave *slave, long period) {
  emcec_master_t *master = slave->master;
  emcec_el95xx_data_t *hal_data = (emcec_el95xx_data_t *) slave->hal_data;
  uint8_t *pd = master->process_data;

  // wait for slave to be operational
  if (!slave->state.operational) {
    return;
  }

  // check inputs
  *(hal_data->power_ok) = EC_READ_BIT(&pd[hal_data->power_ok_pdo_os], hal_data->power_ok_pdo_bp);
  *(hal_data->overload) = EC_READ_BIT(&pd[hal_data->overload_pdo_os], hal_data->overload_pdo_bp);
}
