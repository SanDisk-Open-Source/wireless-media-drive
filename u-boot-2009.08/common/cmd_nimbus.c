
/*
 *  LED and Buzzer test for Nimbus
 *  created by Brent.Tan on 20120517
 */
#include <common.h>
#include <command.h>

#ifndef CONFIG_MFG
#ifdef CONFIG_IMX_CSPI //johnson 20120527
#include <imx_spi.h>
#include <asm/arch/imx_spi_pmic.h>
#endif


/*************************************************************
* Parameter Definition
**************************************************************/
/* ERROR CODE DEFINITION */
#define NIMBUS_POST_SUCCESS_DEF 0
#define NIMBUS_POST_FAILED_DEF 1
#define NIMBUS_POST_WRONG_PARAMETERS_DEF 2
#define NIMBUS_LED_MAX_NUM_DEF 5
#define NIMBUS_TEST_START_OFFSET_DEF 0x70000000
#define NIMBUS_TEST_END_OFFSET_DEF 0x77FFFFFF /*128M = 8000000, start offset + 128M*/
//#define DEBUG
/**************************************************************
*  parameter definition for I/O Test of POST
**************************************************************/
#define NIMBUS_GPIO_PORT_SHIFT_DEF 16
#define NIMBUS_GPIO_START_PIN_SHIFT_DEF 8
#define NIMBUS_GPIO_MUX_SHIFT_DEF 12
#define NIMBUS_GET_GPIO_PORT_DEF(gpio) ((gpio >> NIMBUS_GPIO_PORT_SHIFT_DEF) & 0xFF)
#define NIMBUS_GET_GPIO_RANGE_START_DEF(gpio) ((gpio >> NIMBUS_GPIO_START_PIN_SHIFT_DEF) & 0xFF)
#define NIMBUS_GET_GPIO_RANGE_END_DEF(gpio) (gpio & 0xFF)	                  
#define NIMBUS_GET_PAD_DEF(pinmax) (pinmax & 0xFFF)
#define NIMBUS_GET_MUX_DEF(pinmax) ((pinmax >> NIMBUS_GPIO_MUX_SHIFT_DEF) & 0xFFF)
typedef struct _nimbus_pinmux_type
{
 unsigned int gpio;
 unsigned int pinmux;
}nimbus_pinmux_type, *pnimbus_pinmux_type;
static nimbus_pinmux_type nimbus_gpio_pinmux_var[] = 
{
/*----------------- Definition of the Matrix -----------------
*  gpio :                                | pinmux :
*  bit 23~16       15~8        7~0       | bit 23 ~12     11~0
*      [gpio port] [pin start] [pin end] |     [mux]      [pad]
*  DO NOT MODIFY THE TABLE.
-------------------------------------------------------------*/
 {0x030007, 0x0202CC},   /*0*/
 {0x051828, 0x0402EC},
 {0x050017, 0x06C318},
 {0x030819, 0x0B4360},
 {0x040017, 0x0E4390},
 {0x010007, 0x12C40C},   /*5*/
 {0x011616, 0x14C42C},
 {0x011919, 0x150430},
 {0x011717, 0x154434},
 {0x012121, 0x158438},
 {0x011818, 0x15C43C},   /*10*/
 {0x012020, 0x160440},
 {0x041828, 0x16444C},
 {0x010815, 0x190470},
 {0x020031, 0x1B054C},
 {0x032030, 0x2305CC},   /*15*/
 {0x000027, 0x25C5F8},
 NULL
};

/**************************************************************
*  parameter definition
**************************************************************/
#define NIMBUS_POST_DELAY_100MS_DEF              0.1 /* 100ms */
#define NIMBUS_POST_KEY_HAS_BEEN_PRESSED_DEF     1
#define NIMBUS_POST_KEY_HAS_BEEN_RELEASED_DEF    0
#define NIMBUS_POST_LED_CONTROL_DEF              1
#define NIMBUS_POST_LED_GREEN_WIFI_MASK_DEF      1
#define NIMBUS_POST_LED_GREEN_POWER_MASK_DEF     2
#define NIMBUS_POST_LED_GREEN_DISCHARGE_MASK_DEF 3
#define NIMBUS_POST_LED_RED_CHARGE_MASK_DEF      4
#define NIMBUS_POST_LED_ALL_MASK_DEF             5
#define NIMBUS_POST_LED_SWITCH_ON_DEF            1
#define NIMBUS_POST_LED_SWITCH_OFF_DEF           0

/*************************************************************
* External Interface Functions
**************************************************************/
extern void nimbus_led_switch_fun(unsigned int LedId, int isOn);
extern void nimbus_gpio_test_fun(unsigned int gpio_port, unsigned int pin_shift, unsigned int mux_index,
                                       unsigned int pad_index, unsigned int io_mode, int onoff);
extern int nimbus_is_power_key_pressed(void);
/*************************************************************
* purpose: delay (in second)
* log    : Brent.Tan create on 20120524
**************************************************************/
static void nimbus_sleep_fun (int delay_sec)
{
	ulong start = get_timer(0);
    ulong delay;
    delay = delay_sec * CONFIG_SYS_HZ;
	while (get_timer(start) < delay) {
		//udelay (100);
        udelay (1);
	}
}

/*************************************************************
* purpose: select gpio port, pin to test the i/o
* log    : Brent.Tan create on 20120525
**************************************************************/
static void nimbus_gpiotest_fun (unsigned int gpio_port_id, unsigned int pin_id, unsigned int io_mode, unsigned int onoff)
{
   unsigned int gpio_port, pin_start, pin_end, pad_start, mux_start;
   unsigned int i = 0, pin_offset, imx_gpio_port_id, gpio_pad, gpio_mux, mxc_pin;
   imx_gpio_port_id = gpio_port_id - 1;
   pnimbus_pinmux_type pinmux_ptr = nimbus_gpio_pinmux_var;
   while(pinmux_ptr != NULL) {
     gpio_port = NIMBUS_GET_GPIO_PORT_DEF(nimbus_gpio_pinmux_var[i].gpio);
     pin_start = NIMBUS_GET_GPIO_RANGE_START_DEF(nimbus_gpio_pinmux_var[i].gpio);
     pin_end = NIMBUS_GET_GPIO_RANGE_END_DEF(nimbus_gpio_pinmux_var[i].gpio);
     if ((imx_gpio_port_id == gpio_port) && ((pin_id >= pin_start) && (pin_start <= pin_end))) {
       pad_start = NIMBUS_GET_PAD_DEF(nimbus_gpio_pinmux_var[i].pinmux);
       mux_start = NIMBUS_GET_MUX_DEF(nimbus_gpio_pinmux_var[i].pinmux);
       pin_offset = pin_id - pin_start;
       gpio_mux = mux_start + (pin_offset * 4);       
       gpio_pad = pad_start + (pin_offset * 4);
       break;
     }
     i += 1;
     pinmux_ptr++;
   }
   nimbus_gpio_test_fun(gpio_port_id, pin_id, gpio_mux, gpio_pad, io_mode, onoff);
}

/*************************************************************
* purpose: POST command for power ker detection test.
* log    : Brent.Tan create on 20120621
**************************************************************/
int do_nimbus_post_power_key_detection(cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    unsigned int times_of_keypressed = 0, key_pressed = 0, key_has_been_pressed = NIMBUS_POST_KEY_HAS_BEEN_RELEASED_DEF;
    ulong tmp,testing_timer;
    
    nimbus_led_switch_fun(NIMBUS_POST_LED_ALL_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);

    tmp=get_timer (0);
    testing_timer=tmp+5*CONFIG_SYS_HZ;
    while(1)
    {
       key_pressed = nimbus_is_power_key_pressed();
       if (key_pressed == 1) /* key is pressed now, 1:pressed*/
       {  
         key_has_been_pressed = NIMBUS_POST_KEY_HAS_BEEN_PRESSED_DEF;
         nimbus_led_switch_fun(NIMBUS_POST_LED_ALL_MASK_DEF, NIMBUS_POST_LED_SWITCH_OFF_DEF);
         printf("PWRKEY_TEST_OK\n");
         return NIMBUS_POST_SUCCESS_DEF;
       }
       else
       {
            tmp=get_timer (0);
            if(tmp>testing_timer)
            {
                printf("PWRKEY_TEST_NG\n");
                break;
            }
       }
       //nimbus_sleep_fun(1);
    }
    return NIMBUS_POST_FAILED_DEF;
}

/*******************************************************************************
*
* Name        : mem_data_bus_test
* Description : The function tests data bus of the DRAM. It writes walking one
*               pattern to a fixed memory location.
* Input arg   : None 
* Output arg  : None
* Return      : Returns 1 on failure, else 0
*******************************************************************************/
int nimbus_mem_data_bus_test_fun(void)
{
	unsigned int pattern  = 1;
	unsigned int *address = NIMBUS_TEST_START_OFFSET_DEF; /* 0x70000000 */

	printf("\tLPDDR data bus test                              ");

	for(; pattern != 0; pattern <<= 1)
	{
		*address = pattern;
		if(pattern != *address)
		{
			printf("FAILED\n");
			printf("\t\tTest failed at offset = 0x%X\n"
			       "\t\tData written          = 0x%X\n"
			       "\t\tData read             = 0x%X\n", address, 
			       pattern, *address);
			return 1;
		}
	}
    
    nimbus_sleep_fun(1);
	printf("PASSED\n");
	return 0;
}

/*******************************************************************************
*
* Name        : mem_address_bus_test
* Description : The function tests address bus of the DRAM. It writes walking 
*               one pattern while changes the addresses in the same way. i.e. 
*		write 1 to offset 1, 8 to offset 8, etc. Verify the data after
*		writting to the buffer with length MEM_ADD_TEST_SIZE.
* Input arg   : None 
* Output arg  : None
* Return      : Returns 1 on failure, else 0
*******************************************************************************/

int nimbus_mem_address_bus_test_fun(void)
{
	unsigned int offset = 1, size;
	unsigned int *base_address = NIMBUS_TEST_START_OFFSET_DEF;
       
	size = NIMBUS_TEST_END_OFFSET_DEF - NIMBUS_TEST_START_OFFSET_DEF;

	printf("\tLPDDR address bus test                           ");

	/* We should start with pattern = 1. As we want to use offset and 
	 * pattern the same variable, we will decrement base address by 1.
	 * So, even if we start with offset = pattern = 1, we have actually
	 * started from the given base address. */
	base_address--;

	//for(; offset < size; offset <<= 1)
	for(; (base_address + offset) < NIMBUS_TEST_END_OFFSET_DEF; offset <<= 1)
	{
		*(base_address + offset) = offset;
	}

    //for(offset = 1; offset < size; offset <<= 1)
	for(offset = 1; (base_address + offset) < NIMBUS_TEST_END_OFFSET_DEF; offset <<= 1)
	{
		if(base_address[offset] != offset)
		{
			printf("FAILED\n");
			printf("\t\tTest failed at offset = 0x%X\n"
			       "\t\tData written          = 0x%X\n"
			       "\t\tData read             = 0x%X\n", base_address 
			       + offset, offset, base_address[offset]);
			return 1;
		}
	}
    
    nimbus_sleep_fun(1);
	printf("PASSED\n");
	return 0;
}

/*************************************************************
* purpose: POST command for Memory test
* log    : Brent.Tan create on 20120622
**************************************************************/
int nimbus_mem_device_test_fun(void)
{
	unsigned int offset = 1, size;
	unsigned int *base_address = NIMBUS_TEST_START_OFFSET_DEF;
       
	size = NIMBUS_TEST_END_OFFSET_DEF - NIMBUS_TEST_START_OFFSET_DEF + 1;

	printf("\tLPDDR device test                                ");

	/* Debug print */
#ifdef DEBUG
	printf("Start address = %#X\n", base_address);
	printf("size = %#X\n", size);
#endif

	/* We should start with pattern = 1. As we want to use offset and 
	 * pattern the same variable, we will decrement base address by 1.
	 * So, even if we start with offset = pattern = 1, we have actually
	 * started from the given base address. */
	base_address--;

	//for(; offset < size; offset++)
	for(; (base_address + offset) < NIMBUS_TEST_END_OFFSET_DEF; offset <<= 1)
	{
		base_address[offset] = offset;
	}

	/* Check previously written pattern and write anti-pattern */
	//for(offset = 1; offset < size; offset++)
    for(offset = 1; (base_address + offset) < NIMBUS_TEST_END_OFFSET_DEF; offset <<= 1)
	{
		if(base_address[offset] != offset)
		{
			printf("FAILED\n");
			printf("\t\tTest failed at offset = 0x%X\n"
			       "\t\tData written          = 0x%X\n"
			       "\t\tData read             = 0x%X\n", base_address 
			       + offset, offset, base_address[offset]);
			return 1;
		}

		/* Write anti-pattern */
		base_address[offset] = ~offset;
	}

	/* Debug print */
#ifdef DEBUG
	printf("Checking anti-pattern\n");
#endif

	/* Check anti-pattern */
	//for(offset = 1; offset < size; offset++)
    for(offset = 1; (base_address + offset) < NIMBUS_TEST_END_OFFSET_DEF; offset <<= 1)
	{
		if(base_address[offset] != ~offset)
		{
			printf("FAILED\n");
			printf("\t\tTest failed at offset = 0x%X\n"
			       "\t\tData written          = 0x%X\n"
			       "\t\tData read             = 0x%X\n", base_address 
			       + offset, ~offset, base_address[offset]);
			return 1;
		}
	}

    nimbus_sleep_fun(1);
	printf("PASSED\n");
	return 0;
}


/*************************************************************
* purpose: POST command for Memory test
* log    : Brent.Tan create on 20120622
**************************************************************/
int nimbus_post_mem_fun (void)
{

  nimbus_led_switch_fun(NIMBUS_POST_LED_ALL_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
  if (nimbus_mem_data_bus_test_fun() != NIMBUS_POST_SUCCESS_DEF)
  {
    printf ("[memtest] Data Bus test failed\n");
    return NIMBUS_POST_FAILED_DEF;
  }

  nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_DISCHARGE_MASK_DEF, NIMBUS_POST_LED_SWITCH_OFF_DEF);  
  nimbus_led_switch_fun(NIMBUS_POST_LED_RED_CHARGE_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);  
  if (nimbus_mem_address_bus_test_fun() != NIMBUS_POST_SUCCESS_DEF)
  {
    printf ("[memtest] Address Bus test failed\n");
    return NIMBUS_POST_FAILED_DEF;
  }

  nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_DISCHARGE_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);  
  nimbus_led_switch_fun(NIMBUS_POST_LED_RED_CHARGE_MASK_DEF, NIMBUS_POST_LED_SWITCH_OFF_DEF);  
  if (nimbus_mem_device_test_fun() != NIMBUS_POST_SUCCESS_DEF)
  {
    printf ("[memtest] Device Test failed\n");
    return NIMBUS_POST_FAILED_DEF;
  }

  printf("\tMemory Test Passed\n");
  return NIMBUS_POST_SUCCESS_DEF;
}

/*************************************************************
* purpose: POST command for LED test
* log    : Brent.Tan create on 20120622
**************************************************************/
int nimbus_post_led_fun (void)
{
  unsigned int counter = 0;

  while (counter < 2)
  {
    nimbus_led_switch_fun(NIMBUS_POST_LED_ALL_MASK_DEF, NIMBUS_POST_LED_SWITCH_OFF_DEF);
    nimbus_sleep_fun(1);
    
    if (counter % 2 == 0)
    {
      nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_WIFI_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
      nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_POWER_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
      nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_DISCHARGE_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
    }
    else
    {
      nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_WIFI_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
      nimbus_led_switch_fun(NIMBUS_POST_LED_GREEN_POWER_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
      nimbus_led_switch_fun(NIMBUS_POST_LED_RED_CHARGE_MASK_DEF, NIMBUS_POST_LED_SWITCH_ON_DEF);
    }
    nimbus_sleep_fun(1);
    counter++;
  }

  nimbus_sleep_fun(1);
  nimbus_led_switch_fun(NIMBUS_POST_LED_ALL_MASK_DEF, NIMBUS_POST_LED_SWITCH_OFF_DEF);
  
  return NIMBUS_POST_SUCCESS_DEF;
}


/*************************************************************
* purpose: POST command for LED test
* log    : Brent.Tan create on 20120622
**************************************************************/
int do_nimbus_post (cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
  unsigned int counter = 0;

  
  if (nimbus_post_mem_fun() != NIMBUS_POST_SUCCESS_DEF)
  {
    printf("\tMemory Test Failed\n");
    return NIMBUS_POST_FAILED_DEF;
  }
  nimbus_led_switch_fun(NIMBUS_POST_LED_ALL_MASK_DEF, NIMBUS_POST_LED_SWITCH_OFF_DEF);

  //nimbus_post_led_fun();
  
  return NIMBUS_POST_SUCCESS_DEF;
}


/*************************************************************
* purpose: POST command for LED Test
* log    : Brent.Tan create on 20120517
**************************************************************/
int do_nimbus_post_led(cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    unsigned long whichLed;
    unsigned long onoff;
    char *endp;
    
	switch (argc) {
	 case 0:
	 case 1:
     case 2:
        printf("Help:\n%s\n", cmd->help);
        return NIMBUS_POST_WRONG_PARAMETERS_DEF;
     case 3:
     	whichLed = simple_strtoul(argv[1], &endp, 10);
        if (whichLed == 0 || whichLed > NIMBUS_LED_MAX_NUM_DEF) {
          printf("Help:\n%s\n", cmd->help);
          return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
        onoff = simple_strtoul(argv[2], &endp, 10);
        if (onoff < 0 || onoff > 1) {
          printf("Help:\n%s\n", cmd->help);
          return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }        
        printf("argv[1] = %d, %d\n", (int)whichLed, (int)onoff);
        nimbus_led_switch_fun((int)whichLed, (int)onoff);
        break;
     default:
        break;   
    }   
    
	return NIMBUS_POST_SUCCESS_DEF;
}

/*************************************************************
* purpose: POST command for i/o test.
* log    : Brent.Tan Create on 20120517
**************************************************************/
int do_nimbus_post_iotest(cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    unsigned long gpio_id, pin_id, mode, onoff, reg;
    char *endp;
    
	switch (argc) {
	 case 0:
	 case 1:
     case 2:
     case 3:
     case 4:        
        printf("Help:\n%s\n", cmd->help); 
        return NIMBUS_POST_WRONG_PARAMETERS_DEF;
     case 5:
        gpio_id = simple_strtoul(argv[1], &endp, 10);
        if (gpio_id < 1 || gpio_id > 6) {
          printf("Help:\n%s\n", cmd->help); 
          return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
        pin_id = simple_strtoul(argv[2], &endp, 10);
        if (((gpio_id == 1) && (pin_id < 0 || pin_id > 27)) || 
            ((gpio_id == 2) && (pin_id < 0 || pin_id > 21)) ||
            ((gpio_id == 3) && (pin_id < 0 || pin_id > 31)) ||
            ((gpio_id == 4) && (pin_id < 0 || pin_id > 30)) ||
            ((gpio_id == 5) && (pin_id < 0 || pin_id > 28)) ||
            ((gpio_id == 6) && (pin_id < 0 || pin_id > 17)))
        {
          printf("Help:\n%s\n", cmd->help); 
          return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }       
        mode = simple_strtoul(argv[3], &endp, 10);
        if (mode < 0 || mode > 1) {
          printf("Help:\n%s\n", cmd->help); 
          return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }   
        onoff = simple_strtoul(argv[4], &endp, 10);
        if (onoff < 0 || onoff > 1) {
          printf("Help:\n%s\n", cmd->help);
          return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
        /* i/o request */
        nimbus_gpiotest_fun(gpio_id, pin_id, mode, onoff);

        break;
     default:
        break;
    } 

	return NIMBUS_POST_SUCCESS_DEF;
}


#ifdef CONFIG_IMX_CSPI
/*************************************************************
* purpose: command handler for pmic
* Description:
*   Command format: pmic <action> [parameters] ...
*       action - show/read/write
*       parameters-
*           read  : pmic read <register>
*           write : pmic write <register> <setting>
* log    : Johnson Create on 20120526
**************************************************************/
#define PMIC_REGS_MAX 64
#define PMIC_ACT_SHOWALL    0
#define PMIC_ACT_READ       1
#define PMIC_ACT_WRITE      2
int do_nimbus_post_pmic (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    struct spi_slave *slave;
    int action=-1;
    u32 val,reg;

    if(argc<2)
    {
        printf("Help:\n%s\n", cmd->help);
        return NIMBUS_POST_FAILED_DEF;
    }
    else if(strcmp(argv[1],"show")==0)
    {
        action=PMIC_ACT_SHOWALL;
    }
    else if(strcmp(argv[1],"read")==0)
    {
        action=PMIC_ACT_READ;

        if(argc < 3)
        {
            printf("<cmd error> pmic read <register> (%d)\n",argc);
            return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
        reg = simple_strtoul(argv[2], NULL, 16);
        if( reg >PMIC_REGS_MAX || reg <0)
        {
            printf("<cmd error> register out of range(%d)!\n\t=>(%d)\n",PMIC_REGS_MAX,reg);
            return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
    }
    else if(strcmp(argv[1],"write")==0)
    {
        action=PMIC_ACT_WRITE;

        if(argc < 4)
        {
            printf("<cmd error(%d)> pmic write <register> <setting>\n",argc);
            return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
        
        reg = simple_strtoul(argv[2], NULL, 16);
        if( reg >PMIC_REGS_MAX || reg <0)
        {
            printf("<cmd error> register out of range(%d)!\n\t=>(%d)\n",PMIC_REGS_MAX,reg);
            return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }

        val = simple_strtoul(argv[3], NULL, 16);
        if( val>0xffffffff || val<0 )
        {
            printf("<cmd error> setting out of range(0x%x)!\n\t=>(0x%x)\n",0xFFFFFFFF,val);
            return NIMBUS_POST_WRONG_PARAMETERS_DEF;
        }
    }
    else
    {
        for(val=0; val<argc; val++)
        {
            printf("argv[%d]=%s\n",val,argv[val]);
        }
        printf("Help:\n%s\n", cmd->help);
        return NIMBUS_POST_FAILED_DEF;
    }
   
    
    slave = spi_pmic_probe();
    switch(action)
    {
        case PMIC_ACT_SHOWALL:
            printf("Show PMIC registers:");
            for(reg=0; reg<PMIC_REGS_MAX; reg++)
            {
                if(reg%8 ==0)
                {
                    printf("\n %08x: ",reg);
                }
                val = pmic_reg(slave, reg, 0, 0); 
                printf(" %08x ",val);
            }
            break;
        case PMIC_ACT_READ:
            val = pmic_reg(slave, reg, 0, 0); 
            printf("\tReading- Reg:0x%x , Val: 0x%08x",reg,val);
            break;
        case PMIC_ACT_WRITE:
            val = pmic_reg(slave, reg, val, 1); 
            printf("\tWriting- Reg:0x%x , Val: 0x%08x",reg,val);
            break;
        default:
            break;
    }
    spi_pmic_free(slave);
    printf("\n",val);

	return NIMBUS_POST_SUCCESS_DEF;
}

/*************************************************************
* purpose: power off Nimbus
* log    : Johnson created
**************************************************************/
int do_nimbus_post_poweroff (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    struct spi_slave *slave;
    u32 val,reg;

    slave = spi_pmic_probe();
    printf("Power down Nimbus\n");
    val = pmic_reg(slave, 13, 0, 0); //Power Control 1
    val |= 0x8; //USEROFFSPI 
    pmic_reg(slave, 13, val, 1);
    spi_pmic_free(slave);
    return NIMBUS_POST_SUCCESS_DEF;
}


#ifdef PATCHED_MC13892_BATTERY_20120706
/*************************************************************
* purpose: Show battery voltage value
* log    : 20120706-Johnson created
**************************************************************/
extern int nimbus_get_battery_voltage(void);
extern int nimbus_get_vbus_voltage(void);
extern int nimbus_get_current(int isChanrge,int limited);

int do_nimbus_post_battery_voltage (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    int cnt=1,voltage;
    
    if(argc==2)
    {
        
        cnt = simple_strtoul(argv[1], NULL, 10);
        if(cnt <= 0) cnt=1;
    }

    do
    {
        voltage=nimbus_get_battery_voltage();
        printf("Battery voltage: %d mV\n",voltage);
    }while(--cnt>0);

    
    return NIMBUS_POST_SUCCESS_DEF;
}

int do_nimbus_post_vbus_voltage (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    int cnt=1,voltage;
    
    if(argc==2)
    {
        
        cnt = simple_strtoul(argv[1], NULL, 10);
        if(cnt <= 0) cnt=1;
    }

    do
    {
        voltage=nimbus_get_vbus_voltage();
        printf("VBUS voltage: %d mV\n",voltage);
    }while(--cnt>0);

    
    return NIMBUS_POST_SUCCESS_DEF;
}

int do_nimbus_post_battery_current (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    int cnt=1,current1,current4,limited=0;

    limited=simple_strtoul(argv[1], NULL, 10);
    if (limited>15) limited=13; //1200mA
    
    if(argc==3)
    {
        
        cnt = simple_strtoul(argv[2], NULL, 10);
        if(cnt <= 0) cnt=1;
    }

    do
    {
        current1=nimbus_get_current(0,limited);
        current4=nimbus_get_current(1,limited);
        printf("Battery current: (ch1) %d , (ch4) %d mA\n",current1,current4);
    }while(--cnt>0);

    
    return NIMBUS_POST_SUCCESS_DEF;
}

#endif

#endif

#ifdef QSI_NIMBUS_FIRMWARE_RESCUE
#include <mmc.h>
/* This API will take in charge of firmware checked for resuce related images.
 * Then, enter rescue system to upgrade firmware.
 * Condition:
 * 1. SD card has to format as FAT file system.
 * 2. There are two images required in SD.
 *      - rescue: rescue image which will launch ramdisk linux
 *      - nimbus.img: firmware will be upgrading.
 * 3. Error code will depend on showerr parameter setting.
 */
#define RESCUE_LINUX    "rescue"
#define RESCUE_FIRMWARE "nimbus.img"
#define RESCUE_UBOOT    "u-boot.bin"

#define LOADFILE_RESCUE "fatload mmc 0 70800000 " RESCUE_LINUX
#define LOADFILE_UBOOT  "fatload mmc 0 70800000 " RESCUE_UBOOT

#define ERRCODE_NO_SDCARD   1
#define ERRCODE_NO_UBOOT    2
#define ERRCODE_NO_LINUX    3
#define ERRCODE_NO_FIRMWARE 4
#define ERRCODE_INVALID_RESCUE 5
#define ERRCODE_LOAD_LINUX  6
#define ERRCODE_LOAD_UBOOT  7
#define ERRCODE_INIT_INAND  8
#define ERRCODE_UPDATE_LINUX  9
#define ERRCODE_UPDATE_UBOOT  10
#define ERRCODE_BOOT_LINUX 11


extern int nimbus_check_filename(char *filename);
int do_nimbus_rescue_by_sd (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    int showerr=0,errcode=0;
    char *tag_p;
    struct mmc *mmc;
    
    if(argc==2)
    {
        if(strcmp(argv[1],"showerr")==0)
        {
            showerr=1;
        }
    }

    // Step 1. check mmc 0 for sd card if present.
    mmc = find_mmc_device(0);  /* Dev_Num 0: SD card */
	if (mmc) 
    {
	    if (mmc_init(mmc)) 
        {
		    
            errcode=ERRCODE_NO_SDCARD;
        }
        else
        {
            // check related files
            if(nimbus_check_filename(RESCUE_LINUX)==0)
            {
                errcode=ERRCODE_NO_LINUX;
            }
            else if(nimbus_check_filename(RESCUE_FIRMWARE)==0)
            {
                errcode=ERRCODE_NO_FIRMWARE;
            }
            else
            {
                // check
                if( run_command(LOADFILE_RESCUE, 0)== -1)
                {
                    errcode=ERRCODE_LOAD_LINUX;
                }

                // We need to check if rescue file validated.
                tag_p=(char *)0x70800000+0x20;
                if(strncmp(tag_p,QSI_NIMBUS_RESCUE_TAG,strlen(QSI_NIMBUS_RESCUE_TAG))!=0)
                {
                    errcode=ERRCODE_INVALID_RESCUE;
                    //printf("<ERROR> invalid rescue (%s)\n",tag_p);
                }
                
                if(errcode==0)
                {
                    if( run_command("mmcinfo 2", 0)== -1)
                    {
                        errcode=ERRCODE_INIT_INAND;
                    }
                    else if( run_command("mmc write 2 70800000 0x2800 0x5000", 0)== -1)
                    {
                        errcode=ERRCODE_UPDATE_LINUX;
                    }
                    else if( run_command("setenv bootargs ${bootargs} root=/dev/ram0 rw console=ttymxc0,115200;bootm", 0)== -1)
                    {
                        errcode=ERRCODE_BOOT_LINUX;
                    }
                }
                    
            }
        }
    }

    if(errcode)
    {
        printf("<ERROR> code=%d\n",errcode);
        if(showerr)
        {
            // for error code show on LED.
            int i,loop;

            nimbus_led_switch_fun(5,0);
            loop=20;
            while(loop-- >0)
            {
                nimbus_led_switch_fun(4,1);
                udelay(500000); // 500ms
                nimbus_led_switch_fun(4,0);
                for(i=ERRCODE_NO_SDCARD; i<=errcode; i++)
                {
                    nimbus_led_switch_fun(3,1);
                    udelay(250000);
                    nimbus_led_switch_fun(3,0);
                    udelay(250000);
                }
                if(nimbus_is_power_key_pressed())
                {
                    run_command("poweroff",0);
                }
            }
        }
        else return NIMBUS_POST_FAILED_DEF;
    }
    else
    {
        return NIMBUS_POST_SUCCESS_DEF;
    }
}


int do_nimbus_updateuboot_by_sd (cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    int showerr=0,errcode=0;
    struct mmc *mmc;

    // Step 1. check mmc 0 for sd card if present.
    mmc = find_mmc_device(0);  /* Dev_Num 0: SD card */
	if (mmc) 
    {
	    if (mmc_init(mmc)) 
        {
		    
            errcode=ERRCODE_NO_SDCARD;
        }
        else
        {
            // check related files
            if(nimbus_check_filename(RESCUE_UBOOT)==0)
            {
                errcode=ERRCODE_NO_UBOOT;
            }
            else
            {
                // check
                if( run_command(LOADFILE_UBOOT, 0)== -1)
                {
                    errcode=ERRCODE_LOAD_UBOOT;
                }
                else if( run_command("mmcinfo 2", 0)== -1)
                {
                    errcode=ERRCODE_INIT_INAND;
                }
                else if( run_command("mmc write 2 70800400 2 400", 0)== -1)
                {
                    errcode=ERRCODE_UPDATE_UBOOT;
                }
            }
        }
    }

    if(errcode)
    {
        printf("<ERROR> code=%d\n",errcode);
        return NIMBUS_POST_FAILED_DEF;
    }
    else
    {
        return NIMBUS_POST_SUCCESS_DEF;
    }
}


#endif

#ifdef QSI_NIMBUS_IOPULSE_CMD

int do_nimbus_iopulse(cmd_tbl_t * cmd, int flag, int argc, char *argv[])
{
    int port=0,pulse=0,loop=0,i=0;

    if(argc < 4) 
    {
        printf("Help:\n%s\n", cmd->help);
        return NIMBUS_POST_SUCCESS_DEF;
    }

    port = simple_strtoul(argv[1], NULL, 10);
    pulse = simple_strtoul(argv[2], NULL, 10);
    loop = simple_strtoul(argv[3], NULL, 10);

    printf("TEST: port=%d,width:%d ms,loop: %d\n",port,pulse,loop);

    port%=5;
    if(port>4 || port==0) port=1;
    pulse*=1000;
    for(i=0; i<loop; i++)
    {
        nimbus_led_switch_fun(port, 1);
        udelay(pulse);
        nimbus_led_switch_fun(port, 0);
        udelay(pulse);
    }

    return NIMBUS_POST_SUCCESS_DEF;
}


U_BOOT_CMD(
	iopulse,	4,	0,	do_nimbus_iopulse,
	"IO port pulse.",
	"iopulse <port> <width> <count>\n"
	"     port   : IO PORT ID\n"
	"     width  : io pulse width, unit:ms\n"
	"     count  : loop time\n"
	""
);
#endif


/*************************************************************
* U_BOOT_CMD List
* U_BOOT_CMD(name, maxargs, repeatable, cmd, *usage, *help)
**************************************************************/

U_BOOT_CMD(
	led,	3,	0,	do_nimbus_post_led,
	"Nimbus LED Test..",
	"led [led_num] [action]\n"
	"     Name   : 1: WIFI, 2: Power, 3: Discharge, 4: Charge, 5: ALL\n"
	"     action : 1: on, 0: off\n"
	""
);

U_BOOT_CMD(
	gpio,	  5,	0,	do_nimbus_post_iotest,
	"Nimbus I/O Test..",
	"io [gpio_id] [pin_id] [io_mode] [onoff]\n"
	"    gpio_id : gpio port\n"
	"    pin_id : gpio pin\n"
	"    io_mode: 1: gpo, 0: gpi \n"
	"    onoff  : 1: on, 0; off\n"
	""
);


U_BOOT_CMD(
	pwrkey,	1,	0,	do_nimbus_post_power_key_detection,
	"Nimbus LED Test..",
	"Power key Detection"
	""
);

U_BOOT_CMD(
	post,	1,	0,	do_nimbus_post,
	"Nimbus POST..",
	"Power-on Self Test \n"
	""
);

#ifdef QSI_NIMBUS_FIRMWARE_RESCUE
U_BOOT_CMD(
	sdrescue,	2,	0,	do_nimbus_rescue_by_sd,
	"Rescure system from SD",
	"Need files in SD (FAT):"
	"   rescue : rescue linux\n"
	"   nimbus.img : system firmware\n"
);

U_BOOT_CMD(
	sduboot,	1,	0,	do_nimbus_updateuboot_by_sd,
	"Updated u-boot from SD",
	"Need files in SD (FAT):"
	"   u-boot.bin : u-boot image\n"
	""
);

#endif

#ifdef CONFIG_IMX_CSPI
U_BOOT_CMD(
	pmic,	4,	0,	do_nimbus_post_pmic,
	"Nimbus PMIC (MC34708/MC13892) operations",
	"pmic <cmd> [parameters] ...\n"
	"     cmd: show,read,write\n"
	"         show - show all register\n"
	"         read - read specific register\n"
	"         write - write specific register\n"
	"Example: (base: hex)\n"
	"pmic  show\n"
	"pmic  read <register>\n"
	"pmic  write <register> <setting>\n"	
);

U_BOOT_CMD(
	poweroff,	1,	0,	do_nimbus_post_poweroff,
	"Nimbus Power off",
	"poweroff"
);


#ifdef PATCHED_MC13892_BATTERY_20120706
U_BOOT_CMD(
	batvoltage,	2,	0,	do_nimbus_post_battery_voltage,
	"Nimbus battery voltage",
	"batvoltage [cnt]\n"
	"     cnt: testing loop\n"
	"Example: (base: hex)\n"
	"batvoltage - show battery voltage\n"
);

U_BOOT_CMD(
	vbusvoltage,	2,	0,	do_nimbus_post_vbus_voltage,
	"Nimbus vbus voltage",
	"vbusvoltage [cnt]\n"
	"     cnt: testing loop\n"
	"Example: (base: hex)\n"
	"vbusvoltage - show vbus voltage\n"
);

#endif

U_BOOT_CMD(
	batcurrent,	3,	0,	do_nimbus_post_battery_current,
	"Nimbus battery charge current",
	"batcurrent <level> [cnt]\n"
	"     level: from 0~15 ,13=1200mA\n"
	"     cnt: testing loop\n"
	"Example: (base: hex)\n"
	"batcurrent - show battery charge/discharge path current\n"
);


#endif

#endif
