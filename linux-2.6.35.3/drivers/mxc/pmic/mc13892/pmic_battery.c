/*
 * Copyright 2009-2011 Freescale Semiconductor, Inc. All Rights Reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
 */

/*
 * Includes
 */
#include <linux/workqueue.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/delay.h>
#include <linux/slab.h>
#include <linux/reboot.h>
#include <asm/mach-types.h>
#include <linux/pmic_battery.h>
#include <linux/pmic_adc.h>
#include <linux/pmic_status.h>
#include <linux/pmic_external.h>

#define BIT_CHG_VOL_LSH		0
#define BIT_CHG_VOL_WID		3

#define BIT_CHG_CURR_LSH		3
#define BIT_CHG_CURR_WID		4

#define BIT_CHG_PLIM_LSH		15
#define BIT_CHG_PLIM_WID		2

#define BIT_CHG_DETS_LSH 6
#define BIT_CHG_DETS_WID 1
#define BIT_CHG_CURRS_LSH 11
#define BIT_CHG_CURRS_WID 1

#define TRICKLE_CHG_EN_LSH	7
#define	LOW_POWER_BOOT_ACK_LSH	8
#define BAT_TH_CHECK_DIS_LSH	9
#define	BATTFET_CTL_EN_LSH	10
#define BATTFET_CTL_LSH		11
#define	REV_MOD_EN_LSH		13
#define PLIM_DIS_LSH		17
#define	CHG_LED_EN_LSH		18
#define CHGTMRRST_LSH		19
#define RESTART_CHG_STAT_LSH	20
#define	AUTO_CHG_DIS_LSH	21
#define CYCLING_DIS_LSH		22
#define	VI_PROGRAM_EN_LSH	23

#define TRICKLE_CHG_EN_WID	1
#define	LOW_POWER_BOOT_ACK_WID	1
#define BAT_TH_CHECK_DIS_WID	1
#define	BATTFET_CTL_EN_WID	1
#define BATTFET_CTL_WID		1
#define	REV_MOD_EN_WID		1
#define PLIM_DIS_WID		1
#define	CHG_LED_EN_WID		1
#define CHGTMRRST_WID		1
#define RESTART_CHG_STAT_WID	1
#define	AUTO_CHG_DIS_WID	1
#define CYCLING_DIS_WID		1
#define	VI_PROGRAM_EN_WID	1

#define ACC_STARTCC_LSH		0
#define ACC_STARTCC_WID		1
#define ACC_RSTCC_LSH		1
#define ACC_RSTCC_WID		1
#define ACC_CCFAULT_LSH		7
#define ACC_CCFAULT_WID		7
#define ACC_CCOUT_LSH		8
#define ACC_CCOUT_WID		16
#define ACC1_ONEC_LSH		0
#define ACC1_ONEC_WID		15



#ifdef CONFIG_MACH_MX50_NIMBUS 
#define ACC_CALIBRATION 0x17
#define ACC_START_COUNTER 0x07
#define ACC_STOP_COUNTER 0x2
#define ACC_CONTROL_BIT_MASK 0x1f
#define ACC_ONEC_VALUE 2621
#define ACC_COULOMB_PER_LSB 1
#define ACC_CALIBRATION_DURATION_MSECS 20

#define BAT_VOLTAGE_UNIT_UV 4688 // 4.8V/1024
#define BAT_VOLTAGE_OFFSET_UV 40000 // 4.8V/1024
#define BAT_CURRENT_UNIT_UA 5865
#define CHG_VOLTAGE_UINT_UV 23474
#define CHG_MIN_CURRENT_UA 3500

#define COULOMB_TO_UAH(c) (10000 * c / 36)
#define BAT_CAP_MAH 1500UL
#define CHG_CUR_MA 400UL // 720 mA setting

// For charge current
#define MC13892_ICHRG_OFF       0
#define MC13892_ICHRG_80mA      1
#define MC13892_ICHRG_240mA     2
#define MC13892_ICHRG_320mA     3
#define MC13892_ICHRG_400mA     4
#define MC13892_ICHRG_480mA     5
#define MC13892_ICHRG_560mA     6
#define MC13892_ICHRG_640mA     7
#define MC13892_ICHRG_720mA     8
#define MC13892_ICHRG_800mA     9
#define MC13892_ICHRG_880mA     10
#define MC13892_ICHRG_960mA     11
#define MC13892_ICHRG_1040mA    12
#define MC13892_ICHRG_1200mA    13
#define MC13892_ICHRG_1600mA    14
#define MC13892_ICHRG_FULLON    15

#define BAT_LOW_LIMIT_POWEROFF  3250
#define BAT_LOW_LIMIT_ALARM1    3300    // For low battery charging
#define BAT_LOWCHARGING_BTM     3400

#define BAT_DISCHARGE_NORMAL_BTM    3600
#define BAT_DISCHARGE_LOW_BTM       3400
#define BAT_DISCHARGE_STATE_TIME    18

#define BAT_CHARGE_NORMAL_BTM       3750
#define BAT_CHARGE_LOW_BTM          3550
#define BAT_CHARGE_STATE_TIME       24



#define BAT_FULL_LIMITM         4100
#define CHGR_VOLTAGE_MIN        4700

//#define NIMBUS_VERY_LOWBAT_WORKAROUND
#else
#define ACC_CALIBRATION 0x17
#define ACC_START_COUNTER 0x07
#define ACC_STOP_COUNTER 0x2
#define ACC_CONTROL_BIT_MASK 0x1f
#define ACC_ONEC_VALUE 2621
#define ACC_COULOMB_PER_LSB 1
#define ACC_CALIBRATION_DURATION_MSECS 20

#define BAT_VOLTAGE_UNIT_UV 4692
#define BAT_CURRENT_UNIT_UA 5870
#define CHG_VOLTAGE_UINT_UV 23474
#define CHG_MIN_CURRENT_UA 3500

#define COULOMB_TO_UAH(c) (10000 * c / 36)
#define BAT_CAP_MAH 1000UL
#define CHG_CUR_MA 400UL
#endif
enum chg_setting {
       TRICKLE_CHG_EN,
       LOW_POWER_BOOT_ACK,
       BAT_TH_CHECK_DIS,
       BATTFET_CTL_EN,
       BATTFET_CTL,
       REV_MOD_EN,
       PLIM_DIS,
       CHG_LED_EN,
       CHGTMRRST,
       RESTART_CHG_STAT,
       AUTO_CHG_DIS,
       CYCLING_DIS,
       VI_PROGRAM_EN
};

enum chg_state {
	CHG_POWER_OFF,
	CHG_RESTART,
	CHG_CHARGING,
	CHG_DISCHARGING_WITH_CHARGER,
	CHG_DISCHARGING,
#ifdef CONFIG_MACH_MX50_NIMBUS 
    CHG_OFF,
#endif
};

/* Flag used to indicate if Charger workaround is active. */
int chg_wa_is_active;
/* Flag used to indicate if Charger workaround timer is on. */
int chg_wa_timer;
int disable_chg_timer;
struct workqueue_struct *chg_wq;
struct delayed_work chg_work;
static unsigned long expire;
static int state=CHG_RESTART;

#ifdef CONFIG_MACH_MX50_NIMBUS 

#define NIMBUS_LOW_BATTERY_CHECKED

#define BATLEVEL_CHARGEFULL     200
#define BATLEVEL_CHARGING       150
#define BATLEVEL_DISCHARGENOW   100
#define BATLEVEL_SYNC           3

#define BATLEVEL_SCALE          12

struct bat_curve {
    int voltage;
    int capacity;
    int life;
};

struct bat_curve battery_map[BATLEVEL_SCALE]={
    {4120,100,0},
    {4100,98,500},
    {4030,90,2100},
    {3910,75,4000},
    {3800,60,5000},
    {3750,40,5500},
    {3720,25,4000},
    {3700,20,2000},
    {3620,15,2200},
    {3550,8,1400},
    {3400,5,600},
    {3300,3,0},
    {3200,1,0}
};

#define BAT_CAP_NORMAL  0
#define BAT_CAP_LOW     1
#define BAT_CAP_ALARM   2
//#define LED_SHOW_3_STATE

unsigned long usb_plugin_timestamp;
unsigned long charging_start_timestamp;
int bat_charging_in_small_current;

int battery_init=6;
int bat_power_now;
int discharge_start;
int bat_voltage_avg;
#ifdef NIMBUS_VERY_LOWBAT_WORKAROUND
int bat_voltage_avg_prev,bat_voltage_avg_down_check;
#endif
int bat_capacity_level;
int bat_capacity_check;
int bat_step_time_start;
int bat_step_old;
int charging_setting,charging_adjust,charging_current[3],charging_checking,charging_step_down;
int charging_voltage,charger_retry,charger_usb_testing;
int lowbat_charging;
static int low_bat_found;
extern void nimbus_led_ctrl(int led,int action,int isOn);
extern nimbus_idle_check_stop();
extern nimbus_idle_check_init();
void battery_update_status(void)
{
    if(battery_init) return;
    if(bat_capacity_level == BAT_CAP_NORMAL)
    {
        // GREEN
        nimbus_led_ctrl(2,0,0);
        nimbus_led_ctrl(3,0,1);
    }
#ifdef LED_SHOW_3_STATE   
    else if(bat_capacity_level == BAT_CAP_LOW)
    {
        // AMBER
        nimbus_led_ctrl(2,0,1);
        nimbus_led_ctrl(3,0,1);
    }
#endif    
    else
    {
        // RED
        nimbus_led_ctrl(2,0,1);
        nimbus_led_ctrl(3,0,0);
    }
}
EXPORT_SYMBOL_GPL(battery_update_status);

#ifdef NIMBUS_LOW_BATTERY_CHECKED
#define LED_BLINK_CHECK_DELAY (HZ/2)
static struct timer_list nimbus_led_timer;
static int led_blink_status,led_blink_period;

static void led_blink_timer(unsigned long ptr)
{
    int *led_blink_ptr=(int *)ptr;

    nimbus_led_timer.expires=jiffies + led_blink_period;
    if(battery_init==0)
    {
        if(*led_blink_ptr==0)
        {
            *led_blink_ptr=1;
            battery_update_status();
        }
        else
        {
            *led_blink_ptr=0;
            // only blinking in normal.
            if(bat_capacity_level == BAT_CAP_NORMAL)
            {
                nimbus_led_ctrl(3,0,0);
                nimbus_led_ctrl(2,0,0);
            }
        }
    }
    add_timer(&nimbus_led_timer);
}
#endif
void battery_map_settting(int voltage)
{
    int i;

    for(i=0; i<BATLEVEL_SCALE; i++)
    {
        if(voltage>=battery_map[i].voltage)
        {
            break;
        }
    }

    bat_power_now= (i==0)?battery_map[i].capacity: battery_map[i-1].capacity;
    
    bat_step_old=i;
    bat_step_time_start=jiffies_to_msecs(jiffies)/1000;
    //printk("SETUP: bat_power_now=%d,bat_step_old=%d,bat_step_time_start=%d\n",bat_power_now,bat_step_old,bat_step_time_start);
}

int battery_voltage_to_power_now(int voltage)
{
    int capacity;

    int i,sec;

    for(i=0; i<BATLEVEL_SCALE; i++)
    {
        if(voltage>=battery_map[i].voltage)
        {
            break;
        }
    }

    capacity= (i==0)?battery_map[i].capacity: battery_map[i-1].capacity;
    
    if(i!=bat_step_old)
    {
        bat_step_time_start=jiffies_to_msecs(jiffies)/1000;
        bat_step_old=i;
    }
    else if(i>0)
    {
        sec=jiffies_to_msecs(jiffies)/1000;

        if(battery_map[i].life > 0 && sec>0)
        {
            int diff,life,gap;

            gap=battery_map[i-1].capacity-battery_map[i].capacity;
            life=battery_map[i].life;
            diff=(sec-bat_step_time_start);
            if( diff>0 && life > diff)
            {   
                capacity-=(diff*gap)/life;
                //printk("capacity=%d,diff:%d,gap=%d,life=%d\n",capacity,diff,gap,life);
            }  
            else
            {
                capacity=battery_map[i].capacity;
            }
        }
        else if(battery_map[i].life == 0)
        {
            capacity=battery_map[i].capacity;
        }
    }
    //printk("CAPACITY: %d (setup:%d,sec:%d)\n",capacity,bat_step_time_start,sec);  

    // force led and precentage alignment.
    if( bat_capacity_level!=BAT_CAP_NORMAL && capacity>10 )
    {
        printk("Align LED and CAPACITY: %d (level:%d) to 10%\n",capacity,bat_capacity_level);
        bat_capacity_level=BAT_CAP_LOW;
        capacity=10;
        battery_update_status();
    }
    
    return(capacity);
}

void battery_reset_charging_current_detection(void)
{
    charging_adjust=1;
    charging_checking=0;
    charging_setting=MC13892_ICHRG_560mA;
    charging_step_down=0;
    lowbat_charging=0;
    charger_retry=3;
    usb_plugin_timestamp=jiffies_to_msecs(jiffies)/1000;
    charging_start_timestamp=usb_plugin_timestamp;
    bat_charging_in_small_current=0;
}
#endif

static int pmic_set_chg_current(unsigned short curr)
{
	unsigned int mask;
	unsigned int value;
    //printk("JOHNSON> pmic_set_chg_current=%d\n",curr);
	value = BITFVAL(BIT_CHG_CURR, curr);
	mask = BITFMASK(BIT_CHG_CURR);
	CHECK_ERROR(pmic_write_reg(REG_CHARGE, value, mask));

	return 0;
}
#ifdef CONFIG_MACH_MX50_NIMBUS 
void pmic_battery_turnoff_charging(void)
{
    state = CHG_OFF;
    battery_update_status();
    charger_usb_testing=1;
    queue_delayed_work(chg_wq, &chg_work, HZ*1);
}

EXPORT_SYMBOL_GPL(pmic_battery_turnoff_charging);

#endif

static int pmic_set_chg_misc(enum chg_setting type, unsigned short flag)
{

	unsigned int reg_value = 0;
	unsigned int mask = 0;

	switch (type) {
	case TRICKLE_CHG_EN:
		reg_value = BITFVAL(TRICKLE_CHG_EN, flag);
		mask = BITFMASK(TRICKLE_CHG_EN);
		break;
	case LOW_POWER_BOOT_ACK:
		reg_value = BITFVAL(LOW_POWER_BOOT_ACK, flag);
		mask = BITFMASK(LOW_POWER_BOOT_ACK);
		break;
	case BAT_TH_CHECK_DIS:
		reg_value = BITFVAL(BAT_TH_CHECK_DIS, flag);
		mask = BITFMASK(BAT_TH_CHECK_DIS);
		break;
	case BATTFET_CTL_EN:
		reg_value = BITFVAL(BATTFET_CTL_EN, flag);
		mask = BITFMASK(BATTFET_CTL_EN);
		break;
	case BATTFET_CTL:
		reg_value = BITFVAL(BATTFET_CTL, flag);
		mask = BITFMASK(BATTFET_CTL);
		break;
	case REV_MOD_EN:
		reg_value = BITFVAL(REV_MOD_EN, flag);
		mask = BITFMASK(REV_MOD_EN);
		break;
	case PLIM_DIS:
		reg_value = BITFVAL(PLIM_DIS, flag);
		mask = BITFMASK(PLIM_DIS);
		break;
	case CHG_LED_EN:
		reg_value = BITFVAL(CHG_LED_EN, flag);
		mask = BITFMASK(CHG_LED_EN);
		break;
    case CHGTMRRST:
		reg_value = BITFVAL(CHGTMRRST, flag);
		mask = BITFMASK(CHGTMRRST);
		break;
	case RESTART_CHG_STAT:
		reg_value = BITFVAL(RESTART_CHG_STAT, flag);
		mask = BITFMASK(RESTART_CHG_STAT);
		break;
	case AUTO_CHG_DIS:
		reg_value = BITFVAL(AUTO_CHG_DIS, flag);
		mask = BITFMASK(AUTO_CHG_DIS);
		break;
	case CYCLING_DIS:
		reg_value = BITFVAL(CYCLING_DIS, flag);
		mask = BITFMASK(CYCLING_DIS);
		break;
	case VI_PROGRAM_EN:
		reg_value = BITFVAL(VI_PROGRAM_EN, flag);
		mask = BITFMASK(VI_PROGRAM_EN);
		break;
	default:
		return PMIC_PARAMETER_ERROR;
	}

	CHECK_ERROR(pmic_write_reg(REG_CHARGE, reg_value, mask));

	return 0;
}

static void init_charger_timer(void)
{
	pmic_set_chg_misc(CHGTMRRST, 1);
	expire = jiffies + ((BAT_CAP_MAH*3600UL*HZ)/CHG_CUR_MA);
	pr_notice("%s\n", __func__);
}

static bool charger_timeout(void)
{
	return time_after(jiffies, expire);
}

static void reset_charger_timer(void)
{
	if(!charger_timeout())
		pmic_set_chg_misc(CHGTMRRST, 1);
}


static int pmic_get_batt_voltage(unsigned short *voltage)
{
	t_channel channel;
	unsigned short result[8];

	channel = BATTERY_VOLTAGE;
	CHECK_ERROR(pmic_adc_convert(channel, result));
	*voltage = result[0];

	return 0;
}

static int pmic_get_batt_current(unsigned short *curr)
{
	t_channel channel;
	unsigned short result[8];

	channel = BATTERY_CURRENT;
	CHECK_ERROR(pmic_adc_convert(channel, result));
	*curr = result[0];

	return 0;
}

static int coulomb_counter_calibration;
static unsigned int coulomb_counter_start_time_msecs;

static int pmic_start_coulomb_counter(void)
{
	/* set scaler */
	CHECK_ERROR(pmic_write_reg(REG_ACC1,
		ACC_COULOMB_PER_LSB * ACC_ONEC_VALUE, BITFMASK(ACC1_ONEC)));

	CHECK_ERROR(pmic_write_reg(
		REG_ACC0, ACC_START_COUNTER, ACC_CONTROL_BIT_MASK));
	coulomb_counter_start_time_msecs = jiffies_to_msecs(jiffies);
	pr_debug("coulomb counter start time %u\n",
		coulomb_counter_start_time_msecs);
	return 0;
}

static int pmic_stop_coulomb_counter(void)
{
	CHECK_ERROR(pmic_write_reg(
		REG_ACC0, ACC_STOP_COUNTER, ACC_CONTROL_BIT_MASK));
	return 0;
}

static int pmic_calibrate_coulomb_counter(void)
{
	int ret;
	unsigned int value;

	/* set scaler */
	CHECK_ERROR(pmic_write_reg(REG_ACC1,
		0x1, BITFMASK(ACC1_ONEC)));

	CHECK_ERROR(pmic_write_reg(
		REG_ACC0, ACC_CALIBRATION, ACC_CONTROL_BIT_MASK));
	msleep(ACC_CALIBRATION_DURATION_MSECS);

	ret = pmic_read_reg(REG_ACC0, &value, BITFMASK(ACC_CCOUT));
	if (ret != 0)
		return -1;
	value = BITFEXT(value, ACC_CCOUT);
	pr_debug("calibrate value = %x\n", value);
	coulomb_counter_calibration = (int)((s16)((u16) value));
	pr_debug("coulomb_counter_calibration = %d\n",
		coulomb_counter_calibration);

	return 0;

}

static int pmic_get_charger_coulomb(int *coulomb)
{
	int ret;
	unsigned int value;
	int calibration;
	unsigned int time_diff_msec;

	ret = pmic_read_reg(REG_ACC0, &value, BITFMASK(ACC_CCOUT));
	if (ret != 0)
		return -1;
	value = BITFEXT(value, ACC_CCOUT);
	pr_debug("counter value = %x\n", value);
	*coulomb = ((s16)((u16)value)) * ACC_COULOMB_PER_LSB;

	if (abs(*coulomb) >= ACC_COULOMB_PER_LSB) {
			/* calibrate */
		time_diff_msec = jiffies_to_msecs(jiffies);
		time_diff_msec =
			(time_diff_msec > coulomb_counter_start_time_msecs) ?
			(time_diff_msec - coulomb_counter_start_time_msecs) :
			(0xffffffff - coulomb_counter_start_time_msecs
			+ time_diff_msec);
		calibration = coulomb_counter_calibration * (int)time_diff_msec
			/ (ACC_ONEC_VALUE * ACC_CALIBRATION_DURATION_MSECS);
		*coulomb -= calibration;
	}

	return 0;
}

static int pmic_restart_charging(void)
{
	pmic_set_chg_misc(BAT_TH_CHECK_DIS, 1);
    #ifndef CONFIG_MACH_MX50_NIMBUS // in u-boot, we set to 1 to disable HW controllng for more current input.
	pmic_set_chg_misc(AUTO_CHG_DIS, 0);
    #endif
	pmic_set_chg_misc(VI_PROGRAM_EN, 1);
	pmic_set_chg_misc(RESTART_CHG_STAT, 1);
	pmic_set_chg_misc(PLIM_DIS, 3);
	return 0;
}

struct mc13892_dev_info {
	struct device *dev;

	unsigned short voltage_raw;
	int voltage_uV;
	unsigned short current_raw;
	int current_uA;
	int battery_status;
	int full_counter;
	int charger_online;
	int charger_voltage_uV;
	int accum_current_uAh;

	struct power_supply bat;
	struct power_supply charger;

	struct workqueue_struct *monitor_wqueue;
	struct delayed_work monitor_work;
};

#define mc13892_SENSER	25
#define to_mc13892_dev_info(x) container_of((x), struct mc13892_dev_info, \
					      bat);

static enum power_supply_property mc13892_battery_props[] = {
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
	POWER_SUPPLY_PROP_CURRENT_NOW,
	POWER_SUPPLY_PROP_CHARGE_NOW,
	POWER_SUPPLY_PROP_STATUS,
	#ifdef CONFIG_MACH_MX50_NIMBUS
    POWER_SUPPLY_PROP_POWER_NOW,
    #endif    
};

static enum power_supply_property mc13892_charger_props[] = {
	POWER_SUPPLY_PROP_ONLINE,
};

static int pmic_get_chg_value(unsigned int *value)
{
	t_channel channel;
	unsigned short result[8], max1 = 0, min1 = 0, max2 = 0, min2 = 0, i;
	unsigned int average = 0, average1 = 0, average2 = 0;

	channel = CHARGE_CURRENT;
	CHECK_ERROR(pmic_adc_convert(channel, result));


	for (i = 0; i < 8; i++) {
		if ((result[i] & 0x200) != 0) {
			result[i] = 0x400 - result[i];
			average2 += result[i];
			if ((max2 == 0) || (max2 < result[i]))
				max2 = result[i];
			if ((min2 == 0) || (min2 > result[i]))
				min2 = result[i];
		} else {
			average1 += result[i];
			if ((max1 == 0) || (max1 < result[i]))
				max1 = result[i];
			if ((min1 == 0) || (min1 > result[i]))
				min1 = result[i];
		}
	}

	if (max1 != 0) {
		average1 -= max1;
		if (max2 != 0)
			average2 -= max2;
		else
			average1 -= min1;
	} else
		average2 -= max2 + min2;

	if (average1 >= average2) {
		average = (average1 - average2) / 6;
		*value = average;
	} else {
		average = (average2 - average1) / 6;
		*value = ((~average) + 1) & 0x3FF;
	}

	return 0;
}

#ifdef CONFIG_MACH_MX50_NIMBUS
int get_charger_mA(void) /* get charging current float into battery */
{
	unsigned int value;
	int bat_curr;

	pmic_get_chg_value(&value);

	bat_curr = ((value&0x200) ? (value|0xfffffc00) : value);
	bat_curr = (bat_curr*3000)/512;
	//pr_notice("%s %d(%d)\n", __func__, bat_curr,value);
	return bat_curr;
}


static int pmic_get_charger_voltage(unsigned short *voltage)
{
	t_channel channel;
	unsigned short result[8];

	channel = CHARGE_VOLTAGE;
	CHECK_ERROR(pmic_adc_convert(channel, result));
	*voltage = result[0];

	return 0;
}

int get_charger_mV(void)
{
	unsigned short value;
	pmic_get_charger_voltage(&value);

	//pr_notice("%s %d(%d)\n", __func__,value ,value*10000/805);
	return(value*10000/805);
}

#endif

int get_battery_mA(void) /* get charging current float into battery */
{
	unsigned short value;
	int bat_curr;

	pmic_get_batt_current(&value);
	bat_curr = ((value&0x200) ? (value|0xfffffc00) : value);
	bat_curr = (bat_curr*3000)/512;
	//pr_notice("%s %d\n", __func__, -bat_curr);
	return -bat_curr;
}

int get_battery_mV(void)
{
	unsigned short value;
	pmic_get_batt_voltage(&value);

#ifdef CONFIG_MACH_MX50_NIMBUS    
	value = (value * BAT_VOLTAGE_UNIT_UV + BAT_VOLTAGE_OFFSET_UV)/1000;
    //pr_notice("%s %d\n", __func__, value);
    return(value);
#else
	//pr_notice("%s %d\n", __func__, value*4800/1023);
	return(value*4800/1023);
#endif
}

static void chg_thread(struct work_struct *work)
{
	unsigned int value = 0;
	int charger;
    #ifdef CONFIG_MACH_MX50_NIMBUS
    int bat_voltage;
    int charger_voltage,charger_current,bat_current;

    pmic_read_reg(REG_INT_SENSE0, &value, BITFMASK(BIT_CHG_DETS));
	charger = BITFEXT(value, BIT_CHG_DETS);

    
    bat_voltage=get_battery_mV();
    if(bat_voltage_avg==0) bat_voltage_avg=bat_voltage;
    bat_voltage_avg+=bat_voltage;
    bat_voltage_avg/=2;

    if(battery_init>0)
    {
        
        battery_init--;
        queue_delayed_work(chg_wq, &chg_work, HZ*3);
        if(battery_init==0)
        {
            if(charger) 
            {
                bat_power_now=BATLEVEL_CHARGING;
                if(bat_voltage_avg>BAT_CHARGE_NORMAL_BTM)
                {
                    bat_capacity_level=BAT_CAP_NORMAL;
                }
                else if(bat_voltage_avg>BAT_CHARGE_LOW_BTM)
                {
                    bat_capacity_level=BAT_CAP_ALARM;
                }
                else
                {
                    bat_capacity_level=BAT_CAP_LOW;
                }
            }
            else
            {
                if(bat_voltage_avg>BAT_DISCHARGE_NORMAL_BTM)
                {
                    bat_capacity_level=BAT_CAP_NORMAL;
                }
                else if(bat_voltage_avg>BAT_DISCHARGE_LOW_BTM)
                {
                    bat_capacity_level=BAT_CAP_ALARM;
                }
                else
                {
                    bat_capacity_level=BAT_CAP_LOW;
                }
                bat_power_now=battery_voltage_to_power_now(bat_voltage_avg);
            }
            printk("<INFO> BAT init - %d (%d %,%d mV)\n",bat_capacity_level,bat_power_now,bat_voltage_avg);
        }
        return;
    }
    #else
	pmic_read_reg(REG_INT_SENSE0, &value, BITFMASK(BIT_CHG_DETS));
	charger = BITFEXT(value, BIT_CHG_DETS);
    #endif

	switch(state)
	{
	case CHG_RESTART:
        #ifndef CONFIG_MACH_MX50_NIMBUS 
		pmic_restart_charging();
		pmic_set_chg_current(0);
        #endif
		if(charger){
            #ifdef CONFIG_MACH_MX50_NIMBUS 
            charging_start_timestamp=jiffies_to_msecs(jiffies)/1000;
            #ifdef NIMBUS_VERY_LOWBAT_WORKAROUND
            if(bat_voltage_avg < 3100)
            {
                init_charger_timer();
                printk("CHG_RESTART: Found in very low battery charging. (%d mV)\n",bat_voltage_avg );
                if(charging_adjust)
                {
                    lowbat_charging=1;
                    #ifdef NIMBUS_VERY_LOWBAT_WORKAROUND
                    bat_voltage_avg_down_check=3;
                    #endif
                }
            }
            else 
            #endif
            {
                pmic_restart_charging();
        		init_charger_timer();

                pmic_set_chg_current(charging_setting);
                printk("CHG_RESTART: ICHRG: %d (720mA:8,480mA:5) (%d mV) STAMP:%d\n",charging_setting,bat_voltage_avg,charging_start_timestamp );
            }
            
            bat_charging_in_small_current=0;
            
            state = CHG_CHARGING;
            #else
    		if(get_battery_mV()>3500){
				init_charger_timer();
				pmic_set_chg_current(0x8);
				state = CHG_CHARGING;
			}
			else{
				pmic_set_chg_current(0x8);
				msleep(50);
				if(get_battery_mA()>240){ /* if PMIC can provide 400mA */
					init_charger_timer();
					state = CHG_CHARGING;
				}
				else
				{
				    state = CHG_POWER_OFF;
                }
 			}
            #endif
		}
		else
			state = CHG_DISCHARGING;
		queue_delayed_work(chg_wq, &chg_work, HZ*1);
		break;

	case CHG_POWER_OFF:
		//pr_notice("Battery level < 3.5V!\n");
		pr_notice("After power off, PMIC will charge up battery.\n");
		pmic_set_chg_current(0x8); /* charge battery during power off */
		orderly_poweroff(1);
		break;
 
	case CHG_CHARGING:
		reset_charger_timer();
		
#ifdef CONFIG_MACH_MX50_NIMBUS 
        charger_current=get_charger_mA();
        bat_current=get_battery_mA();
        //printk("CHARGING> bat_voltage:%d mV,chg:%d mA,bat: %d mA,avg: %d mV\n",bat_voltage,charger_current,bat_current,bat_voltage_avg);
        if(charging_adjust==0 && (charger_timeout() || (bat_voltage_avg>BAT_FULL_LIMITM)))
        {
            if(charger)
            {
                int isFinish=0;
                if(charger_timeout())
                {
                    printk("<INFO> Charging Timeout(voltage: %d mV, current: %d mA)\n",bat_voltage,bat_current);
                    if((bat_voltage_avg>BAT_FULL_LIMITM) && (bat_current<160) && (charger_current<400))
                    {
                        printk("<INFO> Charging: timeout (%d).\n",charger_retry);
                        isFinish=1;
                    }
                    else if(charger_retry-- >0)
                    {
                        state = CHG_RESTART;
                    }
                    else
                    {
                        isFinish=1;
                    }    
                    bat_charging_in_small_current=0;
                }
                else if(bat_voltage_avg>BAT_FULL_LIMITM && bat_current<160 && (charger_current<380))
                {
                    if(++bat_charging_in_small_current > 36 ) // 3 min.
                    {
                        printk("<INFO> Charging: very small current timeout!\n");
                        isFinish=1;
                    }
                }
                else if(bat_voltage_avg>BAT_FULL_LIMITM && bat_current<180 && (charger_current<400))
                {
                    if(++bat_charging_in_small_current > 360 ) // 30 min.
                    {
                        printk("<INFO> Charging: small current timeout!\n");
                        isFinish=1;
                    }
                }
                else
                {
                    // maybe in wi-fi accessing casue in large current on VBUS
                    if(bat_voltage_avg>BAT_FULL_LIMITM && bat_current<180)
                    {
                        if(++bat_charging_in_small_current > 720 ) // 60 min.
                        {
                            printk("<INFO> Charging: small current timeout! (VBUS current changes)\n");
                            isFinish=1;
                        }
                    }
                    else bat_charging_in_small_current=0;
                }

                if(isFinish)
                {
                    unsigned long timestamp;
                    int total_charging_time;

                    state = CHG_DISCHARGING_WITH_CHARGER;
                    timestamp =jiffies_to_msecs(jiffies)/1000;
                    total_charging_time=timestamp-charging_start_timestamp;
                        
                    printk("<INFO> Charging: Full (total: %d sec)\n",total_charging_time);
                    del_timer(&nimbus_led_timer); // no blink 
                    bat_capacity_level=BAT_CAP_NORMAL;
                    bat_power_now=BATLEVEL_CHARGEFULL;
                    battery_update_status();
                }
            }
 		}

        if(charging_adjust)
        {
            charger_voltage=get_charger_mV();
            charger_current=get_charger_mA();

            printk("<INFO> charger: %d mV (%d mA),battery_voltage=%d mV\n",charger_voltage,charger_current,bat_voltage);

            if(charging_checking==0)
            {
                charging_voltage=charger_voltage;
            }
            else
            {
                charging_voltage+=charger_voltage;
                charging_voltage/=2;
            }

            
            charging_current[charging_checking++]=charger_current;
            if(charging_checking==3)
            {
                int cur_base,i,cur_avg,cur_min,cur_max;

                charging_checking=0;

                switch(charging_setting)
                {
                    case MC13892_ICHRG_480mA:
                        cur_base=480;
                        break;
                    case MC13892_ICHRG_560mA:
                        cur_base=560;
                        break;
                    case MC13892_ICHRG_640mA:
                        cur_base=640;
                        break;
                    case MC13892_ICHRG_720mA:
                        cur_base=720;
                        break;
                    default:
                        printk("<WARNING> charging_setting out of range.(%d)\n",charging_setting);
                        charging_setting=MC13892_ICHRG_480mA;
                        cur_base=480;
                        break;
                }

                cur_min=cur_base-40;
                cur_max=cur_base+40;
                
                if(lowbat_charging)
                {
                    //printk("<INFO> During low battery charging.(C: %d mA,V: %d mV)\n",charger_current,bat_voltage);
                    if(bat_voltage_avg>BAT_LOWCHARGING_BTM)
                    {
                        lowbat_charging=0;
                        charging_step_down=0;
                        state = CHG_RESTART;
                        printk("<INFO> leave low battery charing. Re-check charging capability. (CHG_RESTART)\n");
                    }

                    #ifdef NIMBUS_VERY_LOWBAT_WORKAROUND
                    if(bat_voltage_avg<bat_voltage_avg_prev)
                    {
                        if(--bat_voltage_avg_down_check < 0 || bat_voltage_avg<2800 )
                        {
                            printk("<INFO> low battery charing. voltage down in bat(%d mV)\n",bat_voltage_avg);
                        }
                    }
                    bat_voltage_avg_prev=bat_voltage_avg;
                    #endif
                    queue_delayed_work(chg_wq, &chg_work, HZ*5);
                    return;
                }

                
                for(i=0; i<3; i++)
                {
                    if(i==0)
                    {
                        cur_avg=charging_current[i];
                    }
                    else if( (charging_current[i]> cur_min) && (charging_current[i]< cur_max))
                    {
                        cur_avg+=charging_current[i];
                        cur_avg/=2;
                    }
                    else
                    {
                        printk("<INFO> charging current out of range,skipping.(Target:%d,%d-%d)\n",cur_base,i,charging_current[i]);
                        if(charging_current[i] < cur_min)
                        {
                            cur_avg=0;
                        }
                    }
                }

                // sometimes, the current is lower than expection of setting, but the voltage doesn't drop on VBUS.
                if( ((cur_avg>cur_min) && (cur_avg<cur_max)) || charger_voltage>CHGR_VOLTAGE_MIN)
                {

                    if( cur_avg==0 && (bat_voltage<BAT_LOW_LIMIT_ALARM1) && (charger_voltage>CHGR_VOLTAGE_MIN) && (charging_setting<=MC13892_ICHRG_560mA))
                    {
                        lowbat_charging=1;
                        #ifdef NIMBUS_VERY_LOWBAT_WORKAROUND
                        bat_voltage_avg_down_check=3;
                        #endif
                        printk("<INFO> Low battery charging...%d mA(BAT:%d mV,VBUS:%d mV)\n",charger_current,bat_voltage,charger_voltage);
                    }
                    else if( charging_step_down )
                    {
                        if(charger_voltage < 4700)
                        {
                            printk("<WARNING> VBUS: %d mV, very low current.(OFF,output<(5V,560mA)).\n",charger_voltage);
                            del_timer(&nimbus_led_timer); // no blink 
                            pmic_set_chg_current(0);
                            state = CHG_OFF;
                            charging_adjust=0;
                            queue_delayed_work(chg_wq, &chg_work, HZ*2);
                            return;
                        }
                        else
                        {
                            charging_adjust=0;
                            printk("<INFO> ICHRG: %d (VBUS:%d mV) (step-down,finished)\n",charging_setting,charger_voltage);
                        }
                    }
                    else
                    {   
                        // try step up
                        if(charging_setting<MC13892_ICHRG_720mA)
                        {
                            charging_setting++;
                            //printk("<INFO>ICHRG: %d (VBUS:%d mV)(try step up)\n",charging_setting,charging_voltage );
                            pmic_set_chg_current(charging_setting);
                        }
                        else
                        {
                            printk("<INFO> ICHRG: %d (VBUS:%d mV) (finished)\n",charging_setting,charging_voltage);
                            charging_adjust=0;
                        }
                    }
                }
                else
                {
                    // may cause voltage drop  
                    //printk("<INFO> voltage: %d \n",charging_voltage );
                    
                    charging_step_down=1;
                    if(charging_setting>MC13892_ICHRG_480mA)
                    {
                        charging_setting--;
                        pmic_set_chg_current(charging_setting);
                        //printk("<INFO> ICHRG: %d (step down) (VBUS:%d mV)\n",charging_setting,charging_voltage );
                    }
                    else if(cur_avg==0)
                    {
                        if(charger_voltage<CHGR_VOLTAGE_MIN)
                        {
                            printk("<WARNING> VBUS: %d mV, very low current...(OFF,cur_avg=0).\n",charging_voltage);
                            del_timer(&nimbus_led_timer); // no blink 
                            pmic_set_chg_current(0);
                            state = CHG_OFF;
                            charging_adjust=0;
                        }
                        else
                        {
                            printk("<WARNING> VBUS: %d mV, ICHRG is enough. battery_voltage=%d mV\n",charging_voltage,bat_voltage);
                            charging_adjust=0;
                        }
                    }

                    
                }
            }
            queue_delayed_work(chg_wq, &chg_work, HZ*2);
        }
        else
        {
            // check battery level
            #ifdef NIMBUS_LOW_BATTERY_CHECKED
            switch(bat_capacity_level)
            {
                case BAT_CAP_NORMAL:
                    break;
                case BAT_CAP_LOW:
                    if(bat_voltage_avg > BAT_CHARGE_NORMAL_BTM)
                    {
                        if(++bat_capacity_check > BAT_CHARGE_STATE_TIME)
                        {
                            bat_capacity_level=BAT_CAP_NORMAL;
                            printk("<INFO> Battery from LOW to NORMAL.\n");
                        }
                    }
                    else bat_capacity_check=0;
                    break;
                case BAT_CAP_ALARM:
                    if(bat_voltage_avg > BAT_CHARGE_LOW_BTM)
                    {
                        if(++bat_capacity_check > BAT_CHARGE_STATE_TIME)
                        {
                            bat_capacity_level=BAT_CAP_LOW;
                            printk("<INFO> Battery from ALARM to LOW.\n");
                        }
                    }
                    else bat_capacity_check=0;
                    break;
                default:
                    break;
            }
            #endif

            queue_delayed_work(chg_wq, &chg_work, HZ*5);
        }

        if(!charger){
			pmic_set_chg_current(0);
			state = CHG_DISCHARGING;
		}
		break;
#else
     if(charger_timeout() || (get_battery_mA()<50)){
			pmic_set_chg_current(0);
			state = CHG_DISCHARGING_WITH_CHARGER;
      }
    

        //printk("CHARGING> cur:%d,bat:%d\n",get_battery_mA(),get_battery_mV());
		if(!charger){
			pmic_set_chg_current(0);
			state = CHG_DISCHARGING;
		}
		queue_delayed_work(chg_wq, &chg_work, HZ*5);
		break;
#endif              

	case CHG_DISCHARGING:
        #ifdef CONFIG_MACH_MX50_NIMBUS
        if(charger)
			state = CHG_RESTART;
        else
        {
            //printk("DISCHARGE> adc:%d,avg:%d\n",bat_voltage,bat_voltage_avg);
            if(discharge_start>0)
            {
                if (discharge_start == BATLEVEL_SYNC)
                {
                    int i;
                    bat_voltage_avg=bat_voltage;
                    for(i=0; i<3; i++)
                    {
                        bat_voltage=get_battery_mV();
                        bat_voltage_avg+=bat_voltage;
                        bat_voltage_avg/=2;
                    }
                    battery_map_settting(bat_voltage_avg);
                }
                else
                {
                    bat_voltage_avg+=bat_voltage;
                    bat_voltage_avg/=2;
                }

                discharge_start--;
                bat_power_now=battery_voltage_to_power_now(bat_voltage_avg);
                queue_delayed_work(chg_wq, &chg_work, HZ/3);
                return;
            }
            
            #ifdef NIMBUS_LOW_BATTERY_CHECKED
            switch(bat_capacity_level)
            {
                case BAT_CAP_NORMAL:
                    if(bat_voltage_avg < BAT_DISCHARGE_NORMAL_BTM)
                    {
                        if(++bat_capacity_check > BAT_DISCHARGE_STATE_TIME)
                        {
                            bat_capacity_level=BAT_CAP_LOW;
                            printk("<INFO> Battery from NORMAL to LOW. (%d %)\n",bat_power_now);
                        }
                    }
                    else bat_capacity_check=0;
                    break;
                case BAT_CAP_LOW:
                    if(bat_voltage_avg < BAT_DISCHARGE_LOW_BTM)
                    {
                        if(++bat_capacity_check > BAT_DISCHARGE_STATE_TIME)
                        {
                            bat_capacity_level=BAT_CAP_ALARM;
                            printk("<INFO> Battery from LOW to ALARM. (%d %)\n",bat_power_now);
                        }
                    }
                    else bat_capacity_check=0;
                    break;
                case BAT_CAP_ALARM:
                    if(bat_voltage_avg < BAT_LOW_LIMIT_POWEROFF)
                    {
                        if(++bat_capacity_check > 3)
                        {
                            printk("<WARNING> Battery low to power down system.(Limited: %d mV,now:%d mV,avg:%d mV)\n",BAT_LOW_LIMIT_POWEROFF,bat_voltage,bat_voltage_avg);
                            state = CHG_POWER_OFF;
                        }
                    }
                    else bat_capacity_check=0;
                    break;
                default:
                    break;
            }
            battery_update_status();
            #endif

            bat_power_now=battery_voltage_to_power_now(bat_voltage_avg);
        }
        queue_delayed_work(chg_wq, &chg_work, HZ*5);
        #else
        if(charger)
			state = CHG_RESTART;
        queue_delayed_work(chg_wq, &chg_work, HZ*10);
        #endif
		
		break;

	case CHG_DISCHARGING_WITH_CHARGER:
		if(get_battery_mV()<4000)
			state = CHG_RESTART;
        if(!charger)
			state = CHG_DISCHARGING;
		queue_delayed_work(chg_wq, &chg_work, HZ*2);
		break;
    #ifdef CONFIG_MACH_MX50_NIMBUS 
    case CHG_OFF:
        if(!charger){
			pmic_set_chg_current(0);
			state = CHG_DISCHARGING;
		}

        if(charger_usb_testing)
        {
            charger_usb_testing=0;
            pmic_set_chg_current(0);
            printk("<INFO> Turn off charging. (USB test mode)\n");
        }
        queue_delayed_work(chg_wq, &chg_work, HZ*2);
        break;
    #endif
 	}
	//pr_notice("charger state %d %c\n", state, charger?'Y':'N');
 }

static int mc13892_charger_update_status(struct mc13892_dev_info *di)
{
	int ret;
	unsigned int value;
	int online;

	ret = pmic_read_reg(REG_INT_SENSE0, &value, BITFMASK(BIT_CHG_DETS));

	if (ret == 0) {
		online = BITFEXT(value, BIT_CHG_DETS);
		if (online != di->charger_online) {
			di->charger_online = online;
			dev_info(di->charger.dev, "charger status: %s\n",
				online ? "online" : "offline");
			power_supply_changed(&di->charger);

			cancel_delayed_work(&di->monitor_work);
			queue_delayed_work(di->monitor_wqueue,
				&di->monitor_work, HZ / 10);
			if (online) {
				pmic_start_coulomb_counter();
				chg_wa_timer = 1;
                #ifdef CONFIG_MACH_MX50_NIMBUS  
                printk("USB: On-line\n");
                bat_power_now=BATLEVEL_CHARGING;
                #ifdef NIMBUS_LOW_BATTERY_CHECKED
                init_timer(&nimbus_led_timer);
                nimbus_led_timer.function=led_blink_timer;
                nimbus_led_timer.data=(unsigned long)&led_blink_status;
                led_blink_period = LED_BLINK_CHECK_DELAY;
                nimbus_led_timer.expires=jiffies + led_blink_period;
                add_timer(&nimbus_led_timer);
                #endif
                battery_reset_charging_current_detection();
                nimbus_idle_check_stop();
                #endif
			} else {
				chg_wa_timer = 0;
				pmic_stop_coulomb_counter();
                #ifdef CONFIG_MACH_MX50_NIMBUS  
                bat_power_now=battery_voltage_to_power_now(bat_voltage_avg);
                printk("USB: Off-line (power_now:%d)(avg:%d)\n",bat_power_now,bat_voltage_avg); 
                del_timer(&nimbus_led_timer);
                discharge_start=BATLEVEL_SYNC;
                battery_update_status();
                cancel_delayed_work(&chg_work);
                queue_delayed_work(chg_wq, &chg_work, HZ/3);
                nimbus_idle_check_init();
                #endif

		    }
	    }
	}

	return ret;
}

static int mc13892_charger_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct mc13892_dev_info *di =
		container_of((psy), struct mc13892_dev_info, charger);

	switch (psp) {
	case POWER_SUPPLY_PROP_ONLINE:
		val->intval = di->charger_online;
		return 0;
	default:
		break;
	}
	return -EINVAL;
}

static int mc13892_battery_read_status(struct mc13892_dev_info *di)
{
	int retval;
	int coulomb;
	retval = pmic_get_batt_voltage(&(di->voltage_raw));
#ifdef CONFIG_MACH_MX50_NIMBUS    
	if (retval == 0)
		di->voltage_uV = di->voltage_raw * BAT_VOLTAGE_UNIT_UV + BAT_VOLTAGE_OFFSET_UV;
#else
	if (retval == 0)
		di->voltage_uV = di->voltage_raw * BAT_VOLTAGE_UNIT_UV;
#endif    

	retval = pmic_get_batt_current(&(di->current_raw));
	if (retval == 0) {
		if (di->current_raw & 0x200)
			di->current_uA =
				(0x1FF - (di->current_raw & 0x1FF)) *
				BAT_CURRENT_UNIT_UA * (-1);
		else
			di->current_uA =
				(di->current_raw & 0x1FF) * BAT_CURRENT_UNIT_UA;
	}
	retval = pmic_get_charger_coulomb(&coulomb);
	if (retval == 0)
		di->accum_current_uAh = COULOMB_TO_UAH(coulomb);

	return retval;
}

static void mc13892_battery_update_status(struct mc13892_dev_info *di)
{
	unsigned int value;
	int retval;
	int old_battery_status = di->battery_status;

	if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN)
		di->full_counter = 0;

	if (di->charger_online) {
		retval = pmic_read_reg(REG_INT_SENSE0,
					&value, BITFMASK(BIT_CHG_CURRS));

		if (retval == 0) {
			value = BITFEXT(value, BIT_CHG_CURRS);
			if (value)
				di->battery_status =
					POWER_SUPPLY_STATUS_CHARGING;
			else
				di->battery_status =
					POWER_SUPPLY_STATUS_NOT_CHARGING;
		}

		if (di->battery_status == POWER_SUPPLY_STATUS_NOT_CHARGING)
			di->full_counter++;
		else
			di->full_counter = 0;
	} else {
		di->battery_status = POWER_SUPPLY_STATUS_DISCHARGING;
		di->full_counter = 0;
	}

	dev_dbg(di->bat.dev, "bat status: %d\n",
		di->battery_status);

	if (old_battery_status != POWER_SUPPLY_STATUS_UNKNOWN &&
		di->battery_status != old_battery_status)
		power_supply_changed(&di->bat);
}

static void mc13892_battery_work(struct work_struct *work)
{
	struct mc13892_dev_info *di = container_of(work,
						     struct mc13892_dev_info,
						     monitor_work.work);
	const int interval = HZ * 60;

	dev_dbg(di->dev, "%s\n", __func__);

	mc13892_battery_update_status(di);
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, interval);
}

static void charger_online_event_callback(void *para)
{
	struct mc13892_dev_info *di = (struct mc13892_dev_info *) para;
	pr_info("\n\n DETECTED charger plug/unplug event\n");
	mc13892_charger_update_status(di);
}


static int mc13892_battery_get_property(struct power_supply *psy,
				       enum power_supply_property psp,
				       union power_supply_propval *val)
{
	struct mc13892_dev_info *di = to_mc13892_dev_info(psy);
	switch (psp) {
	case POWER_SUPPLY_PROP_STATUS:
		if (di->battery_status == POWER_SUPPLY_STATUS_UNKNOWN) {
			mc13892_charger_update_status(di);
			mc13892_battery_update_status(di);
		}
		val->intval = di->battery_status;
		return 0;
	default:
		break;
	}

	mc13892_battery_read_status(di);

	switch (psp) {
	case POWER_SUPPLY_PROP_VOLTAGE_NOW:
		val->intval = di->voltage_uV;
		break;
	case POWER_SUPPLY_PROP_CURRENT_NOW:
		val->intval = di->current_uA;
		break;
	case POWER_SUPPLY_PROP_CHARGE_NOW:
		val->intval = di->accum_current_uAh;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MAX_DESIGN:
		val->intval = 3800000;
		break;
	case POWER_SUPPLY_PROP_VOLTAGE_MIN_DESIGN:
		val->intval = 3300000;
		break;
    #ifdef CONFIG_MACH_MX50_NIMBUS
    case POWER_SUPPLY_PROP_POWER_NOW:
        val->intval = bat_power_now;
        break;
    #endif
	default:
		return -EINVAL;
	}

	return 0;
}

static ssize_t chg_wa_enable_show(struct device *dev,
				struct device_attribute *attr, char *buf)
{
	if (chg_wa_is_active & chg_wa_timer)
		return sprintf(buf, "Charger LED workaround timer is on\n");
	else
		return sprintf(buf, "Charger LED workaround timer is off\n");
}

static ssize_t chg_wa_enable_store(struct device *dev,
				 struct device_attribute *attr,
				 const char *buf, size_t size)
{
	if (strstr(buf, "1") != NULL) {
		if (chg_wa_is_active) {
			if (chg_wa_timer)
				printk(KERN_INFO "Charger timer is already on\n");
			else {
				chg_wa_timer = 1;
				printk(KERN_INFO "Turned on the timer\n");
			}
		}
	} else if (strstr(buf, "0") != NULL) {
		if (chg_wa_is_active) {
			if (chg_wa_timer) {
				chg_wa_timer = 0;
				printk(KERN_INFO "Turned off charger timer\n");
			 } else {
				printk(KERN_INFO "The Charger workaround timer is off\n");
			}
		}
	}

	return size;
}

static DEVICE_ATTR(enable, 0644, chg_wa_enable_show, chg_wa_enable_store);

static int pmic_battery_remove(struct platform_device *pdev)
{
	pmic_event_callback_t bat_event_callback;
	struct mc13892_dev_info *di = platform_get_drvdata(pdev);

	bat_event_callback.func = charger_online_event_callback;
	bat_event_callback.param = (void *) di;
	pmic_event_unsubscribe(EVENT_CHGDETI, bat_event_callback);

	cancel_rearming_delayed_workqueue(di->monitor_wqueue,
					  &di->monitor_work);
	cancel_rearming_delayed_workqueue(chg_wq,
					  &chg_work);
	destroy_workqueue(di->monitor_wqueue);
	destroy_workqueue(chg_wq);
	chg_wa_timer = 0;
	chg_wa_is_active = 0;
	disable_chg_timer = 0;
	power_supply_unregister(&di->bat);
	power_supply_unregister(&di->charger);

	kfree(di);

	return 0;
}

static int pmic_battery_probe(struct platform_device *pdev)
{
	int retval = 0;
	struct mc13892_dev_info *di;
	pmic_event_callback_t bat_event_callback;
	pmic_version_t pmic_version;

	/* Only apply battery driver for MC13892 V2.0 due to ENGR108085 */
	pmic_version = pmic_get_version();
    printk("pmic_battery_probe: %d\n",pmic_version);
	if (pmic_version.revision < 20) {
		pr_debug("Battery driver is only applied for MC13892 V2.0\n");
		return -1;
	}
	if (machine_is_mx50_arm2()) {
		pr_debug("mc13892 charger is not used for this platform\n");
		return -1;
	}

	di = kzalloc(sizeof(*di), GFP_KERNEL);
	if (!di) {
		retval = -ENOMEM;
		goto di_alloc_failed;
	}

	platform_set_drvdata(pdev, di);

	di->charger.name	= "mc13892_charger";
	di->charger.type = POWER_SUPPLY_TYPE_MAINS;
	di->charger.properties = mc13892_charger_props;
	di->charger.num_properties = ARRAY_SIZE(mc13892_charger_props);
	di->charger.get_property = mc13892_charger_get_property;
	retval = power_supply_register(&pdev->dev, &di->charger);
	if (retval) {
		dev_err(di->dev, "failed to register charger\n");
		goto charger_failed;
	}

	INIT_DELAYED_WORK(&chg_work, chg_thread);
	chg_wq = create_singlethread_workqueue("mxc_chg");
	if (!chg_wq) {
		retval = -ESRCH;
		goto workqueue_failed;
	}
    queue_delayed_work(chg_wq, &chg_work, HZ);
    
	INIT_DELAYED_WORK(&di->monitor_work, mc13892_battery_work);
	di->monitor_wqueue = create_singlethread_workqueue(dev_name(&pdev->dev));
	if (!di->monitor_wqueue) {
		retval = -ESRCH;
		goto workqueue_failed;
	}
	queue_delayed_work(di->monitor_wqueue, &di->monitor_work, HZ * 10);

	di->dev	= &pdev->dev;
	di->bat.name	= "mc13892_bat";
	di->bat.type = POWER_SUPPLY_TYPE_BATTERY;
	di->bat.properties = mc13892_battery_props;
	di->bat.num_properties = ARRAY_SIZE(mc13892_battery_props);
	di->bat.get_property = mc13892_battery_get_property;
	di->bat.use_for_apm = 1;

	di->battery_status = POWER_SUPPLY_STATUS_UNKNOWN;

	retval = power_supply_register(&pdev->dev, &di->bat);
	if (retval) {
		dev_err(di->dev, "failed to register battery\n");
		goto batt_failed;
	}

	bat_event_callback.func = charger_online_event_callback;
	bat_event_callback.param = (void *) di;
	pmic_event_subscribe(EVENT_CHGDETI, bat_event_callback);
	retval = sysfs_create_file(&pdev->dev.kobj, &dev_attr_enable.attr);

	if (retval) {
		printk(KERN_ERR
		       "Battery: Unable to register sysdev entry for Battery");
		goto workqueue_failed;
	}
	chg_wa_is_active = 1;
	chg_wa_timer = 0;
	disable_chg_timer = 0;

    #ifdef CONFIG_MACH_MX50_NIMBUS
    discharge_start=BATLEVEL_SYNC;
    battery_reset_charging_current_detection();
    #endif

	pmic_stop_coulomb_counter();
	pmic_calibrate_coulomb_counter();
	goto success;

workqueue_failed:
	power_supply_unregister(&di->charger);
charger_failed:
	power_supply_unregister(&di->bat);
batt_failed:
	kfree(di);
di_alloc_failed:
success:
	dev_dbg(di->dev, "%s battery probed!\n", __func__);
    printk("pmic_battery_probe: success\n");
	return retval;


	return 0;
}

static struct platform_driver pmic_battery_driver_ldm = {
	.driver = {
#ifdef CONFIG_MACH_MX50_NIMBUS 
		   .name = "mc13892_battery",
#else        
		   .name = "pmic_battery",
#endif		   
		   .bus = &platform_bus_type,
		   },
	.probe = pmic_battery_probe,
	.remove = pmic_battery_remove,
};

static int __init pmic_battery_init(void)
{
	pr_debug("PMIC Battery driver loading...\n");
	return platform_driver_register(&pmic_battery_driver_ldm);
}

static void __exit pmic_battery_exit(void)
{
	platform_driver_unregister(&pmic_battery_driver_ldm);
	pr_debug("PMIC Battery driver successfully unloaded\n");
}

module_init(pmic_battery_init);
module_exit(pmic_battery_exit);

MODULE_DESCRIPTION("pmic_battery driver");
MODULE_AUTHOR("Freescale Semiconductor, Inc.");
MODULE_LICENSE("GPL");
