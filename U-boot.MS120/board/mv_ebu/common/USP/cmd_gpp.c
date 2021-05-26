/*
 * Control GPIO pins on the fly
 *
 * Copyright (c) 2008-2011 Analog Devices Inc.
 *
 * Licensed under the GPL-2 or later.
 */

#include <config.h>
#include <common.h>
#include <command.h>
#include <slre.h>
#include "mvCommon.h"
#include "mvOs.h"
#include "gpp/mvGpp.h"
#include "gpp/mvGppRegs.h"
#include <i2c.h>
#include <nand.h>
#include "nand_nfc.h"

#ifndef name_to_gpio
#define name_to_gpio(name) simple_strtoul(name, NULL, 10)
#endif

enum gpio_cmd {
	GPIO_INPUT,
	GPIO_SET,
	GPIO_CLEAR,
	GPIO_TOGGLE,
	GPIO_INT,
	GPIO_DISPLAY,
};

enum gpio_interrupt_sensitive {
	INT_EDGE,
	INT_LEVEL,
};

enum gpio_polarity {
	POL_NORMAL,
	POL_INVERTED,
};

int do_gpp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int gpio;
	enum gpio_cmd sub_cmd;
	ulong value;
	const char *str_cmd, *str_gpio;

	if ((argc > 6) || (argc < 3))
 show_usage:
		return cmd_usage(cmdtp);
	str_cmd = argv[1];
	str_gpio = argv[2];

	/* parse the behavior */
	switch (*str_cmd) {
		case 'i': sub_cmd = GPIO_INPUT;  break;
		case 's': sub_cmd = GPIO_SET;    break;
		case 'c': sub_cmd = GPIO_CLEAR;  break;
		case 't': sub_cmd = GPIO_TOGGLE; break;
		case 'I': sub_cmd = GPIO_INT;    break;
		case 'd': sub_cmd = GPIO_DISPLAY; break;
		default:  goto show_usage;
	}

	/* turn the gpio name into a gpio number */
	gpio = name_to_gpio(str_gpio);
	if ((gpio < 0) || (gpio >= MV_GPP_MAX_PINS))
		goto show_usage;

	/* finally, let's do it: set direction and exec command */
	if (sub_cmd == GPIO_INT)
	{
		if((argc == 6) && (argv[3][0] == 's'))
		{
			enum gpio_interrupt_sensitive	int_sens;
			enum gpio_polarity				pol;

			if(argv[4][0] == 'e')
				int_sens = INT_EDGE;
			else if(argv[4][0] == 'l')
				int_sens = INT_LEVEL;
			else
				goto show_usage;

			if(argv[5][0] == 'r')
				pol = POL_NORMAL;
			else if(argv[5][0] == 'f')
				pol = POL_INVERTED;
			else
				goto show_usage;

			mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN & (1 << (gpio % 32))));
			if(pol == POL_NORMAL)
				mvGppPolaritySet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN_ORIGIN & (1 << (gpio % 32))));
			else
				mvGppPolaritySet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN_INVERT & (1 << (gpio % 32))));

			if(int_sens == INT_EDGE)
				MV_REG_WRITE(GPP_INT_MASK_REG((gpio / 32)), (1 << (gpio % 32)));
			else
				MV_REG_WRITE(GPP_INT_LVL_REG((gpio / 32)), (1 << (gpio % 32)));
		}
		else if((argc == 4) && (argv[3][0] == 'p'))
		{
			if((MV_REG_READ(GPP_INT_CAUSE_REG(gpio / 32))) & (1 << (gpio % 32)))
			{
				printf("interrupt on GPIO %d\n", gpio);
				MV_REG_WRITE(GPP_INT_CAUSE_REG(gpio / 32), (0 << (gpio % 32)));
			}
			else
				printf("no interrupt on GPIO %d\n", gpio);
		}
		else if((argc == 5) && (argv[3][0] == 'p'))
		{
			int count = simple_strtoul(argv[4], NULL, 10), interrupt = 0;
			while(count-- > 0)
			{
				if((MV_REG_READ(GPP_INT_CAUSE_REG(gpio / 32))) & (1 << (gpio % 32)))
				{
					printf("interrupt on GPIO %d, %d\n", gpio, (mvGppValueGet((gpio / 32), (1 << (gpio % 32))) >> (gpio % 32)));
					MV_REG_WRITE(GPP_INT_CAUSE_REG(gpio / 32), (0 << (gpio % 32)));
					interrupt = 1;
				}
			}
			if(interrupt == 0)
				printf("no interrupt on GPIO %d\n", gpio);
		}
		else if((argc == 4) && (argv[3][0] == 'c'))
		{
			mvGppPolaritySet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN_ORIGIN & (1 << (gpio % 32))));
			MV_REG_WRITE(GPP_INT_MASK_REG((gpio / 32)), (0 << (gpio % 32)));
			MV_REG_WRITE(GPP_INT_LVL_REG((gpio / 32)), (0 << (gpio % 32)));
		}
		else
			goto show_usage;

		return 0;
	}
	if (sub_cmd == GPIO_DISPLAY) {
		int i;
		MV_U32 mppVal;

		for (i = 0; i < MV_MPP_MAX_GROUP; i++) {
			mppVal = mvBoardMppGet(i);
			printf("group %d : %08x\n", i, mppVal);
		}
		return 0;
	}
	if (sub_cmd == GPIO_INPUT) {
		mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN & (1 << (gpio % 32))));
		value = (mvGppValueGet((gpio / 32), (1 << (gpio % 32))) >> (gpio % 32));
	} else {
		switch (sub_cmd) {
			case GPIO_SET:    value = 1; break;
			case GPIO_CLEAR:  value = 0; break;
			case GPIO_TOGGLE: value = !mvGppValueGet((gpio / 32), (1 << (gpio % 32)));; break;
			default:          goto show_usage;
		}
		mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_OUT & (1 << (gpio % 32))));
		mvGppValueSet((gpio / 32), (1 << (gpio % 32)), (value << (gpio % 32)));
	}
	printf("gpio: pin %s (gpio %i) value is %lu, INTER_REGS_BASE = %x\n",
		str_gpio, gpio, value, INTER_REGS_BASE);

	return value;
}

U_BOOT_CMD(gpp, 6, 0, do_gpp,
	"input/set/clear/toggle/poll interrupt gpio pins",
	"<input|set|clear|toggle> <pin>\n"
	"    - input/set/clear/toggle the specified pin\n"
	"<Interrupt> <pin> <set> <edge/level> <raise/fall>\n"
	"    - set the specified pin to interrupt\n"
	"<Interrupt> <pin> <poll>\n"
	"    - poll the specified pin\n"
	"<Interrupt> <pin> <clear>\n"
	"    - clear the specified interrupt pin");


#if 0 /* For Tombaugh */

#define BOARD_ID_ENABLE_GPIO	9
int board_id_gpio[4] = {5, 18, 7, 8};
static int get_bid(void)
{
	int i, gpio, bid = 0, value;
	value = 1;
	mvGppTypeSet((BOARD_ID_ENABLE_GPIO / 32), (1 << (BOARD_ID_ENABLE_GPIO % 32)), (MV_GPP_OUT & (1 << (BOARD_ID_ENABLE_GPIO % 32))));
	mvGppValueSet((BOARD_ID_ENABLE_GPIO / 32), (1 << (BOARD_ID_ENABLE_GPIO % 32)), (value << (BOARD_ID_ENABLE_GPIO % 32)));
	for(i = 0; i < 4; i++)
	{
		gpio = board_id_gpio[i];
		mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN & (1 << (gpio % 32))));
		value = mvGppValueGet((gpio / 32), (1 << (gpio % 32)));
		bid |= value ? (1 << i) : 0;
	}
	return bid;
}

#else   /* For Kelpie */

int board_id_gpio[3] = {9, 10, 11};
static int get_bid(void)
{
	int i, gpio, bid = 0, value;
	value = 1;
	for(i = 0; i < 3; i++)
	{
		gpio = board_id_gpio[i];
		mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN & (1 << (gpio % 32))));
		value = mvGppValueGet((gpio / 32), (1 << (gpio % 32)));
		bid |= value ? (1 << i) : 0;
	}
	return bid;
}

#endif

int do_bid(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int bid = get_bid();

	printf("\nBoard ID : 0x%x(%d)\n", bid, bid);
	return 0;
}

U_BOOT_CMD(bid, 1, 1, do_bid,
	"print Board ID",
	"");

int do_uart(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char	c;

	if((argc == 3) && (argv[1][0] == 'p'))
	{
		mvUartPutc(1, argv[2][0]);
	}
	else if((argc == 2) && (argv[1][0] == 'g'))
	{
		c = mvUartGetc(1);
		printf("get char \"%c\"\n", c);
	}
	else
		return cmd_usage(cmdtp);

	return 0;
}

U_BOOT_CMD(uart, 3, 1, do_uart,
	"input/output to UART 1",
	"p <char> - put char\n"
	"uart g - get char\n");

#define RESET_GPIO 7
static int do_reset_test(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	ulong start = get_timer(0);
	ulong delay;
	ulong value;

	if (argc != 2)
		return cmd_usage(cmdtp);

	delay = simple_strtoul(argv[1], NULL, 10) * CONFIG_SYS_HZ;

	mvGppTypeSet((RESET_GPIO / 32), (1 << (RESET_GPIO % 32)), (MV_GPP_IN & (1 << (RESET_GPIO % 32))));
	while (get_timer(start) < delay) {
		value = (mvGppValueGet((RESET_GPIO / 32), (1 << (RESET_GPIO % 32))) >> (RESET_GPIO % 32));
		if(value == 0)
		{
			printf("Reset Button Test is OK!\n");
			return 0;
		}
		if (ctrlc ())
			return (-1);
	}

	printf("Reset Button Test is timeout!\n");
	return 0;
}

U_BOOT_CMD(reset_test, 2, 1, do_reset_test,
	"test reset pin (GPIO10)",
	"N\n"
	"    - check if reset button is pressed in N seconds (N is _decimal_ !!!)\n"
	"      return value \"1\" is pressed, \"0\" is not pressed.");


static void display_pd_info(void)
{
	unsigned char	data;

	if (i2c_read(0x70, 0x06, 1, &data, sizeof(data)) != 0)
	{
		printf("Unable to read from I2C 0x70\n");
		return;
	}

	if((data & 0x40) == 0)
		printf("    AC present\n");

	if((data & 0x20) == 0)
		printf("    port 10 802.3af present\n");

	if((data & 0x10) == 0)
		printf("    port 9 802.3af present\n");

	if((data & 0x8) == 0)
		printf("    port 10 60W POE present\n");

	if((data & 0x4) == 0)
		printf("    port 9 60W POE present\n");

	if((data & 0x2) == 0)
		printf("    port 10 802.3at present\n");

	if((data & 0x1) == 0)
		printf("    port 9 802.3at present\n");
}

static unsigned char get_cpld_ver(void)
{
	unsigned char	data;

	if (i2c_read(0x70, 0x0, 1, &data, sizeof(data)) != 0)
	{
		printf("Unable to read from I2C 0x70\n");
		return 0;
	}

	return data;
}

static void display_nand_info(void)
{
	struct mtd_info *mtd = &nand_info[0];
	MV_U16 read_id = 0;

	mvNfcReadIdNative(0, &read_id);
	printf("NAND        : id %x", read_id);
	printf(", %lu MiB\n", (mtd->size / 1024 / 1024));
}

#include "version.h"

static void gpio_settings_Kelpie(void)
{
	/* For Kelpie GPIO degault settings */
    int output_gpio[5] = {0, 1, 2, 3, 8},input_gpio[7] = {5, 7, 9, 10, 11, 13, 18};
	int i, gpio, value;

	for(i = 0; i < 5; i++){
	   gpio = output_gpio[i];
	   mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_OUT & (1 << (gpio % 32))));
       /*
	   value = mvGppValueGet((gpio / 32), (1 << (gpio % 32)));
	   bid |= value ? (1 << i) : 0;
       */
	}

    for(i = 0; i < 7; i++){
	   gpio = input_gpio[i];
	   mvGppTypeSet((gpio / 32), (1 << (gpio % 32)), (MV_GPP_IN & (1 << (gpio % 32))));
       /*
	   value = mvGppValueGet((gpio / 32), (1 << (gpio % 32)));
	   bid |= value ? (1 << i) : 0;
       */
	}
}

static void LedSettings88E1514(void)
{
    run_command("phyWrite 1 0x16 0x0", 0);

    run_command("phyWrite 1 0x16 0x3", 0);
    run_command("phyRead 1 0x10", 0);
    run_command("phyWrite 1 0x10 0x101E", 0);
    run_command("phyRead 1 0x10", 0);

    run_command("phyRead 1 0x11", 0);
    run_command("phyWrite 1 0x11 0x8000", 0);
    run_command("phyRead 1 0x11", 0);

    run_command("phyWrite 1 0x16 0x0", 0);

    printf("\nFinished: 88E1514 Led settings \n");
}


static int regex_match(char *string, const char* re, int len, struct caps *captured)
{
	struct slre slre;
	return (slre_compile(&slre, re) && slre_match(&slre, string, len, captured));
}


static int do_show(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if (argc != 2)
		return cmd_usage(cmdtp);

	if(argv[1][0] == 'i')
	{
		char name[50];
		char eeprom_rd_sku_cmd[50];
		char kelpie_sku[21] = {0};
		struct slre slre;
		struct cap captured_strings[2];
		sprintf(eeprom_rd_sku_cmd, "i2c read 0x57 0xf.2 0x14 0x%x", kelpie_sku);
		run_command(eeprom_rd_sku_cmd, 0);

		/* CPU */
		mvCtrlModelRevNameGet(name);
		printf("SoC         : %s\n", name);
		mvCpuNameGet(name);
		printf("CPU         : %s",  name);
#ifdef MV_CPU_LE
		printf(" LE\n");
#else
		printf(" BE\n");
#endif

		/* DDR */
		printf("DDR         : %dBit Width, %s Memory Access, DLB %s ",
			mvCtrlDDRBudWidth(), mvCtrlDDRThruXbar()?"XBAR":"FastPath",
			mvCtrlIsDLBEnabled() ? "Enabled" : "Disabled");
#if defined(CONFIG_ECC_SUPPORT)
		printf(", ECC %s", mvCtrlDDRECC()?"Enabled":"Disabled");
#endif
		printf("\n");

		/* NAND */
		display_nand_info();

		/* U-boot version, CPLD FW version */
		printf("U-Boot VER  : %s\n", U_BOOT_VERSION);

#if 0
		printf("CPLD FW VER : %d\n", get_cpld_ver());
#endif

		/* ROS checksum todo */

		if(regex_match(kelpie_sku, "(6[15]0[123]0)", sizeof(kelpie_sku), captured_strings)) {
			printf("\n Found SKU in EEPROM as: %.*s", captured_strings[0].len, captured_strings[0].ptr);
			run_command("setenv fit_config kelpie-8", 0);
		} else if(regex_match(kelpie_sku, "(6[15]040)", sizeof(kelpie_sku), captured_strings)) {
			printf("\n Found SKU in EEPROM as: %.*s", captured_strings[0].len, captured_strings[0].ptr);
			run_command("setenv fit_config kelpie-24", 0);
		} else if(regex_match(kelpie_sku, "(6[15]050)", sizeof(kelpie_sku), captured_strings)) {
			printf("\n Found SKU in EEPROM as: %.*s", captured_strings[0].len, captured_strings[0].ptr);
			run_command("setenv fit_config kelpie-24", 0);
			tesla_i2c_initKelpie24P48P();
			run_command("fan set speed 100",0);
		} else if(regex_match(kelpie_sku, "(6[15]060)", sizeof(kelpie_sku), captured_strings)) {
			printf("\n Found SKU in EEPROM as: %.*s", captured_strings[0].len, captured_strings[0].ptr);
			run_command("setenv fit_config kelpie-48", 0);
		} else if(regex_match(kelpie_sku, "(6[15]0[78]0)", sizeof(kelpie_sku), captured_strings)) {
			printf("\n Found SKU in EEPROM as: %.*s", captured_strings[0].len, captured_strings[0].ptr);
			run_command("setenv fit_config kelpie-48", 0);
			tesla_i2c_initKelpie24P48P();
			run_command("fan set speed 100",0);
		} else {
			printf("Kelpie-Unknown. Unknown board specified\n");
			run_command("setenv fit_config kelpie-unknown", 0);
		}

		#if 0
        if((bid>3)&&(bid<7)) {
            printf("SF550X-48x PCIe Serdes settings for Slave to Master\n");

            run_command("md 0xe0040090 1",0);
            run_command("mw 0xe0040090 0x102 1",0);
            run_command("md 0xe0040090 1",0);

            run_command("mw 0xe0041a00 0x40140a2c 1",0);
            run_command("md 0xe0041a00 1",0);
            run_command("md 0xe0040090 1",0);
        }
        #endif



	    #if 0
		/* PD information */
		printf("PD info     : \n");
		display_pd_info();
	    #endif
	}
	else if(argv[1][0] == 'p')
	{
		display_pd_info();
	}
	else
		return cmd_usage(cmdtp);

	return 0;
}

U_BOOT_CMD(show, 2, 1, do_show,
	"display information",
	" info - display system information\n"
	"show pd   - display PD information\n");


static void cpld_reg_set(unsigned char reg, unsigned char data)
{
	if (i2c_write(0x70, reg, 1, &data, 1) != 0)
	{
		printf("Unable to write to I2C 0x70 reg %d\n", reg);
	}
}

static int do_led(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	if((argc == 3) && (argv[1][0] == 'a') && (argv[2][0] == '0'))
	{
		cpld_reg_set(0x2, 0xff);
		cpld_reg_set(0x3, 0x0);
		cpld_reg_set(0x5, 0x10);
	}
	else if((argc == 3) && (argv[1][0] == 's') && (argv[2][0] == '0'))
	{
		cpld_reg_set(0x3, 0x0);
	}
	else if((argc == 4) && (argv[1][0] == 'a') && (argv[2][0] == 'g') && (argv[3][0] == '1'))
	{
		cpld_reg_set(0x3, 0x2);
		cpld_reg_set(0x5, 0x0);
	}
	else if((argc == 4) && (argv[1][0] == 'a') && (argv[2][0] == 'a') && (argv[3][0] == '1'))
	{
		cpld_reg_set(0x2, 0x0);
		cpld_reg_set(0x3, 0x1);
	}
	else if((argc == 4) && (argv[1][0] == 's') && (argv[2][0] == 'g') && (argv[3][0] == '1'))
	{
		cpld_reg_set(0x3, 0x2);
	}
	else if((argc == 4) && (argv[1][0] == 's') && (argv[2][0] == 'g') && (argv[3][0] == 'b'))
	{
		cpld_reg_set(0x3, 0x6);
	}
	else if((argc == 4) && (argv[1][0] == 's') && (argv[2][0] == 'a') && (argv[3][0] == '1'))
	{
		cpld_reg_set(0x3, 0x1);
	}
	else if((argc == 4) && (argv[1][0] == 's') && (argv[2][0] == 'a') && (argv[3][0] == 'b'))
	{
		cpld_reg_set(0x3, 0x9);
	}
	else if((argc == 4) && (argv[1][0] == 'p') && (argv[2][0] == 'g') && (argv[3][0] == '1'))
	{
		cpld_reg_set(0x5, 0x0);
	}
	else if((argc == 4) && (argv[1][0] == 'p') && (argv[2][0] == 'g') && (argv[3][0] == '0'))
	{
		cpld_reg_set(0x5, 0x10);
	}
	else if((argc == 4) && (argv[1][0] == 'p') && (argv[2][0] == 'a') && (argv[3][0] == '1'))
	{
		cpld_reg_set(0x2, 0x0);
	}
	else if((argc == 4) && (argv[1][0] == 'p') && (argv[2][0] == 'a') && (argv[3][0] == '0'))
	{
		cpld_reg_set(0x2, 0xff);
	}
	else
		return cmd_usage(cmdtp);

	return 0;
}

U_BOOT_CMD(led, 4, 1, do_led,
	"control LEDs",
	" - turn on/off all green/amber LEDs\n"
	"led a 0   - All LEDs off\n"
	"led a g 1 - All Green LEDs on\n"
	"led a a 1 - All Amber LEDs on\n"
	"led s 0   - System LED off\n"
	"led s g 1 - System Green LED on\n"
	"led s g b - System Green LED blinking\n"
	"led s a 1 - System Amber LED on\n"
	"led s a b - System Amber LED blinking\n"
	"led p g 1 - Port Green LEDs on\n"
	"led p g 0 - Port Green LEDs off\n"
	"led p a 1 - Port Amber LEDs on\n"
	"led p a 0 - Port Amber LEDs off\n");

static int do_upgrade(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	char cmd[512];
	char *tftp_argv[] = {"tftp", NULL, NULL};

        if((argc == 2) && (argv[1][0] == 'C')){
        }
        else{
	    sprintf(cmd, "setenv usbethaddr; usb start; setenv ethact asx0; setenv serverip %s", argv[2]);
	    run_command(cmd, 0);
        }

	if((argc == 4) && (argv[1][0] == 'u'))	/* u-boot only */
	{

		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		#if 0
		 run_command("nand erase 0 0x700000; nand write 0x2000000 0 $filesize;nand read 2000000 0 $filesize;crc32 2000000 $filesize", 0);
	        #else
	         run_command("crc32 0x2000000 $filesize;nand erase.chip; nand write 0x2000000 0 $filesize;nand read 0x2000000 0 $filesize;crc32 0x2000000 $filesize", 0);
	        #endif
	}
	else if((argc == 4) && (argv[1][0] == 'k')) /* kernel only */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	       #if 0
		run_command("nand erase 0x700000 0x400000; nand write 0x2000000 0x700000 $filesize", 0);
	       #else
	        run_command("crc32 0x2000000 $filesize;nand erase 0x700000 0xA00000; nand write 0x2000000 0x700000 $filesize;crc32 0x2000000 $filesize;", 0);
	       #endif

	}
	else if((argc == 4) && (argv[1][0] == 'f'))	/* file system only */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize;nand erase.part rootfs; ubi part rootfs; ubi create vol1; ubi write 0x2000000 vol1 $filesize", 0);
	}
	else if((argc == 4) && (argv[1][0] == 'r'))	/* Backup root file system image only */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize;nand erase 0x1100000 0x2800000; nand write 0x2000000 0x1100000 $filesize;nand read 0x2000000 0x1100000 $filesize ;crc32 0x2000000 $filesize", 0);
	}
	else if((argc == 5) && (argv[1][0] == 'd'))	/* kernel + file system */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	       #if 0
		run_command("nand erase 0x700000 0x400000; nand write 0x2000000 0x700000 $filesize;", 0);
	       #else
	        run_command("crc32 0x2000000 $filesize;nand erase 0x700000 0xA00000; nand write 0x2000000 0x700000 $filesize;crc32 0x2000000 $filesize;", 0);
	       #endif

		tftp_argv[1] = argv[4];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize;nand erase.part rootfs; ubi part rootfs; ubi create vol1; ubi write 0x2000000 vol1 $filesize", 0);
	}
	else if((argc == 6) && (argv[1][0] == 'a'))	/* all */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}


		#if 0
		 run_command("nand erase 0 0x500000; nand write 0x2000000 0 $filesize", 0);
		#else
		 run_command("crc32 2000000 $filesize;nand erase 0 0x700000; nand write 0x2000000 0 $filesize;nand read 0x2000000 0 $filesize;crc32 0x2000000 $filesize", 0);
		#endif

		tftp_argv[1] = argv[4];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	       #if 0
		run_command("nand erase 0x700000 0x400000; nand write 0x2000000 0x700000 $filesize;", 0);
	       #else
	        run_command("crc32 2000000 $filesize;nand erase 0x700000 0xA00000; nand write 0x2000000 0x700000 $filesize;nand read 2000000 700000 $filesize;crc32 2000000 $filesize", 0);
	       #endif

		tftp_argv[1] = argv[5];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

		#if 0
		 run_command("nand erase.part rootfs; ubi part rootfs; ubi create vol1; ubi write 0x2000000 vol1 $filesize", 0);
		#else
		 run_command("crc32 2000000 $filesize;nand erase.part rootfs; ubi part rootfs; ubi create vol1; ubi write 0x2000000 vol1 $filesize", 0);
		#endif

		printf("\n==> Please reset you DUT for mfg codes upgrade. <== \n\n");
	}
	else if((argc == 6) && (argv[1][0] == 'A'))	/* all */
	{

		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}


		run_command("crc32 0x2000000 $filesize;nand erase 0x8000000 0x700000; nand write 0x2000000 0x8000000 $filesize;nand read 0x2000000 0x8000000 $filesize;crc32 0x2000000 $filesize", 0);


		tftp_argv[1] = argv[4];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	        run_command("crc32 0x2000000 $filesize;nand erase 0x8700000 0xA00000; nand write 0x2000000 0x8700000 $filesize;nand read 0x2000000 0x8700000 $filesize;crc32 0x2000000 $filesize", 0);


		tftp_argv[1] = argv[5];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

		run_command("crc32 0x2000000 $filesize;nand erase 0x9100000 0x2800000; nand write 0x2000000 0x9100000 $filesize;nand read 0x2000000 0x9100000 $filesize ;crc32 0x2000000 $filesize", 0);

		printf("\n==> Backup u-boot/kernel/roofs to nand flash [ offset 0x8000000/0x8700000/0x9100000 ]  <== \n\n");
	}
	else if((argc == 6) && (argv[1][0] == 'b'))	/*Official Booton + ros_U-boot + ros_image */
	{

		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	        run_command("crc32 0x2000000 $filesize;nand erase 0x0 0x20000; nand write.e 0x2000000 0x0 0x1f800; nand erase 0x20000 0x20000; nand write.e 0x2000000 0x20000 0x1f800;nand read 0x2000000 0 $filesize;crc32 0x2000000 $filesize;nand read 0x2800000 0x20000 $filesize;crc32 0x2800000 $filesize", 0);


		tftp_argv[1] = argv[4];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize;nand erase 0x60000 0x140000; nand write.e 0x2000000 0x60000 0x140000; nand erase 0x1a0000 0x140000; nand write.e 0x2000000 0x1a0000 0x140000;nand read 0x2000000 0x60000 $filesize ;crc32 0x2000000 $filesize;nand read 0x2800000 0x1a0000 $filesize ;crc32 0x2800000 $filesize", 0);
		tftp_argv[1] = argv[5];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize ;nand erase 0x400000 0xFC00000; nand write 0x2000000 0x400000 $filesize;nand read 0x2000000 0x400000 $filesize ;crc32 2000000 $filesize", 0);
		printf("\n==> Please reset you DUT for official codes[Booton,ros_U-boot,ROS_image] upgrade. <== \n\n");

	}
	else if((argc == 6) && (argv[1][0] == 'o'))	/* Back official codes to nand flash [ 0x4000000 / 0x4700000 / 0x5100000 ] */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

		run_command("crc32 0x2000000 $filesize;nand erase 0x4000000 0x700000; nand write 0x2000000 0x4000000 $filesize;nand read 0x2000000 0x4000000 $filesize;crc32 0x2000000 $filesize", 0);

		run_command("setenv booton_filesize $filesize;saveenv", 0);

		tftp_argv[1] = argv[4];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	        run_command("crc32 0x2000000 $filesize;nand erase 0x4700000 0xA00000; nand write 0x2000000 0x4700000 $filesize;nand read 0x2000000 0x4700000 $filesize;crc32 2000000 $filesize", 0);

	        run_command("setenv ros_uboot_filesize $filesize;saveenv", 0);

		tftp_argv[1] = argv[5];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize;nand erase 0x5100000 0x2800000; nand write 0x2000000 0x5100000 $filesize;nand read 0x2000000 0x5100000 $filesize;crc32 2000000 $filesize", 0);

		run_command("setenv ros_filesize $filesize;saveenv", 0);

	        printf("\n==> Backup booton/ros_u-boot/ros_image to nand flash [ offset 0x4000000/0x4700000/0x5100000 ]  <== \n\n");
	}
	else if((argc == 6) && (argv[1][0] == 'O'))	/* Back official codes to nand flash [ 0xC000000 / 0xC700000 / 0xD100000 ] */
	{
		tftp_argv[1] = argv[3];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

		run_command("crc32 0x2000000 $filesize;nand erase 0xC000000 0x700000; nand write 0x2000000 0xC000000 $filesize;nand read 0x2000000 0xC000000 $filesize;crc32 0x2000000 $filesize", 0);

		tftp_argv[1] = argv[4];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}

	        run_command("crc32 0x2000000 $filesize;nand erase 0xC700000 0xA00000; nand write 0x2000000 0xC700000 $filesize;nand read 0x2000000 0xC700000 $filesize;crc32 2000000 $filesize", 0);

		tftp_argv[1] = argv[5];
		if (do_tftpb(NULL, 0, 2, tftp_argv))
		{
			printf("tftp fail\n");
			return 1;
		}
		run_command("crc32 0x2000000 $filesize;nand erase 0xD100000 0x2800000; nand write 0x2000000 0xD100000 $filesize;nand read 0x2000000 0xD100000 $filesize;crc32 2000000 $filesize", 0);
	         printf("\n==> Backup booton/ros_u-boot/ros_image to nand flash [ offset 0xC000000/0xC700000/0xD100000 ]  <== \n\n");
	}
	else if((argc == 2) && (argv[1][0] == 'C'))	/* Change mfg code to official Booton and ros_U-boot codes */
	{
		printf("\n\n==> Write Booton code. <== \n");

	       #if 0
		run_command("nand read 0x2000000 0x4000000 $booton_filesize ;crc32 0x2000000 $booton_filesize", 0);
	        run_command("nand erase 0x0 0x20000; nand write.e 0x2000000 0x0 0x1f800; nand erase 0x20000 0x20000; nand write.e 0x2000000 0x20000 0x1f800;nand read 0x2000000 0 $booton_filesize ;crc32 0x2000000 $booton_filesize ;nand read 0x2800000 0x20000 $booton_filesize ;crc32 0x2800000 $booton_filesize;printenv booton_filesize", 0);
	       #else
	        run_command("nand read 0x2000000 0x4000000 0x170e4 ;crc32 0x2000000 0x170e4", 0);
	        run_command("nand erase 0x0 0x20000; nand write.e 0x2000000 0x0 0x1f800; nand erase 0x20000 0x20000; nand write.e 0x2000000 0x20000 0x1f800;nand read 0x2000000 0 0x170e4 ;crc32 0x2000000 0x170e4 ;nand read 0x2800000 0x20000 0x170e4 ;crc32 0x2800000 0x170e4 ", 0);
	       #endif

	       	printf("\n\n==> Write ros_u-boot code. <== \n");
	       #if 0
	       	run_command("nand read 0x2000000 0x4700000 $ros_uboot_filesize ;crc32 0x2000000 $ros_uboot_filesize ", 0);
		run_command("nand erase 0x60000 0x140000; nand write.e 0x2000000 0x60000 0x140000; nand erase 0x1a0000 0x140000; nand write.e 0x2000000 0x1a0000 0x140000;nand read 0x2000000 0x60000 $ros_uboot_filesize ;crc32 0x2000000 $ros_uboot_filesize ;nand read 0x2800000 0x1a0000 $ros_uboot_filesize ;crc32 0x2800000 $ros_uboot_filesize;printenv ros_uboot_filesize", 0);
	       #else
	        run_command("nand read 0x2000000 0x4700000 0xd531c ;crc32 0x2000000 0xd531c ", 0);
		run_command("nand erase 0x60000 0x140000; nand write.e 0x2000000 0x60000 0x140000; nand erase 0x1a0000 0x140000; nand write.e 0x2000000 0x1a0000 0x140000;nand read 0x2000000 0x60000 0xd531c ;crc32 0x2000000 0xd531c ;nand read 0x2800000 0x1a0000 0xd531c ;crc32 0x2800000 0xd531c ", 0);
	       #endif

		printf("\n\n==> Write ros_image code. <== \n");
	       #if 0
		run_command("nand erase 0x400000 0x3C00000; nand read 0x2000000 0x5100000 $ros_filesize ; crc32 0x2000000 $ros_filesize ; nand write 0x2000000 0x400000 $ros_filesize ; nand read 0x2000000 0x400000 $ros_filesize ; crc32 0x2000000 $ros_filesize;printenv ros_filesize", 0);
	       #else
	        run_command("nand erase 0x400000 0x3C00000; nand read 0x2000000 0x5100000 0x2000000 ; crc32 0x2000000 0x185bb06 ; nand write 0x2000000 0x400000 0x2000000 ; nand read 0x2000000 0x400000 0x185bb06 ; crc32 0x2000000 0x185bb06 ", 0);
	       #endif

		printf("\n==> Please reset you DUT for official codes[ Booton,ros_u-boot and ros_image ] upgrade. <== \n\n");

	}
	else
		return cmd_usage(cmdtp);

	return 0;
}

U_BOOT_CMD(upgrade, 6, 1, do_upgrade,
	"upgrade command",
	"a <serverip> <u-boot> <kernel> <file system>\n"
	"upgrade u <serverip> <u-boot>\n"
	"upgrade k <serverip> <kernel>\n"
	"upgrade f <serverip> <file system>\n"
	"upgrade r <serverip> <rootfs image>\n"
	"upgrade d <serverip> <kernel> <file system>\n"
	"upgrade a <serverip> <u-boot> <kernel> <file system>\n"
	"upgrade A <serverip> <u-boot> <kernel> <file system> [ Backup 0x8000000/0x8700000/0x9100000 ]\n"
	"upgrade b <serverip> <Booton> <ros_U-boot> <ROS_Image>\n"
	"upgrade o <serverip> <Booton> <ros_U-boot> <ROS_Image>\n"
	"upgrade O <serverip> <Booton> <ros_U-boot> <ROS_Image> Backup 0xC000000/0xC700000/0xD100000 ]\n"
	"upgrade C [ Change mfg codes to official Booton,ros_u-boot and ros_image codes ]\n");

int do_sfp(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	unsigned char	data;

   #if 1
	if((get_bid()>0)&&(get_bid()<4)){
	    run_command("i2c mw 70 6 1 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG1 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG1 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("i2c mw 70 6 2 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG2 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG2 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("i2c mw 70 6 3 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG3 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG3 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("i2c mw 70 6 4 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG4 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG4 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	   if( i2c_read(0x70, 0x4, 1, &data, sizeof(data)) != 0)
	       printf("Unable to read SFP+ Status Register \n");
	   else{
	       printf("SFP+ Present_on and LOS_on : 0x00 ]\n");
	       printf("SFP+ Present_on and LOS_off : 0x55 ]\n");
	       printf("SFP+ Present_off and LOS_off : 0xff ]\n");
	       printf("Bit_0/2/4/6 for SFP+ LOS_XG1/XG2/XG3/XG4 \n");
	       printf("Bit_1/3/5/7 for SFP+ Present_XG1/XG2/XG3/XG4 \n");
	       printf("Read SFP+ Status Register=0x%02x \n",data);
	   }

	}
	else if((get_bid()>3)&&(get_bid()<7)){
	    run_command("i2c mw 70 a 40 1",0);

	    run_command("mw.l e0018100 4408000A 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG1 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG1 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("mw.l e0018100 4408000B 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG2 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG2 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("mw.l e0018100 4408000E 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG3 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG3 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("mw.l e0018100 4408000F 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG4 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG4 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    printf("Read SFP+ Status Register \n");
	    printf("SFP+ Present_on and LOS_on : 0x44080005 ]\n");
	    printf("SFP+ Present_on and LOS_off : 0x4408003f ]\n");
	    printf("SFP+ Present_off and LOS_off : 0x440807bf ]\n");
	    printf("Bit_1/3/4/5 for SFP+ LOS_XG1/XG2/XG3/XG4 \n");
	    printf("Bit_7/8/9/10 for SFP+ Present_XG1/XG2/XG3/XG4 \n");
	    run_command("md.l e0018110 1",0);

	}
	else if(get_bid()==0xE){
		run_command("i2c mw 70 a 40 1",0);

	    run_command("mw.l e0018100 4408000A 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG1 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG1 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("mw.l e0018100 4408000B 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG2 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG2 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("mw.l e0018100 4408000E 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG3 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG3 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    run_command("mw.l e0018100 4408000F 1",0);
	    if( i2c_read(0x50, 0x0, 1, &data, sizeof(data)) != 0)
	    	printf("Unable to read XG4 EEPROM from I2C 0x50\n");
	    else{
	        printf("Success. Read XG4 EEPROM offset 0x0 data=0x%x from I2C 0x50\n",data);
	        run_command("i2c md 50 0 100",0);
	    }

	    printf("Read SFP+ Status Register \n");
	    printf("SFP+ Present_on and LOS_on : 0x4408e805 ]\n");
	    printf("SFP+ Present_on and LOS_off : 0x4408e83f ]\n");
	    printf("SFP+ Present_off and LOS_off : 0x4408efbf ]\n");
	    printf("Bit_1/3/4/5 for SFP+ LOS_XG1/XG2/XG3/XG4 \n");
	    printf("Bit_7/8/9/10 for SFP+ Present_XG1/XG2/XG3/XG4 \n");
	    run_command("md.l e0018110 1",0);
	}
   #else
	if (i2c_read(0x70, 0x04, 1, &data, sizeof(data)) != 0){
	    printf("Unable to read from I2C 0x70\n");
	    return;
	}

	printf("Port  9 module is %s.\n", ((data & 0x4) ? "absent" : "insent"));
	printf("Port 10 module is %s.\n", ((data & 0x8) ? "absent" : "insent"));
   #endif

	return 0;
}

U_BOOT_CMD(sfp, 1, 1, do_sfp,
	"display SFP+ Module Present",
	"");

extern int do_ping(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[]);
int ping_net_console_client(void)
{
	char *nc_ip = getenv("serverip");
	char *ping_argv[] = {"ping", NULL, NULL};

	if(nc_ip)
	{
		run_command("setenv usbethaddr; usb start; setenv ethact asx0", 0);
		ping_argv[1] = nc_ip;
		if(do_ping(NULL, 0, 2, ping_argv))
			return 0;

		setenv("ncip", nc_ip);
		setenv("stdin", "nc");
		setenv("stdout", "nc");
		setenv("stderr", "nc");
		return 1;
	}

	return 0;
}
