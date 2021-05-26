#include <config.h>
#include <common.h>
#include <command.h>
#include "mvCommon.h"
#include "mvOs.h"
#include "malloc.h"
#include "nand.h"
#include <spi_flash.h>
#include <i2c.h>
#include <usb.h>

#define DIAG_REPORT_OFFSET	0x1e00000
#define DIAG_REPORT_SIZE	0x20000

#define DIAG_NAND_TEST_OFFSET	0xfd00000
#define DIAG_NAND_TEST_SIZE		0x100000
#define DIAG_SPI0_TEST_OFFSET	0x1f00000
#define DIAG_SPI0_TEST_SIZE		0x100000
#define DIAG_RAM_TEST_START		0x2000000
#define DIAG_RAM_TEST_END		0x1f000000

static void parse_cmd(char *cmd, int *nand, int *spi, int *ram, int *i2c, int *usb, int *eth)
{
	if(strstr(cmd, "nand"))
		*nand = 1;
	else
		*nand = 0;
	
	if(strstr(cmd, "spi"))
		*spi = 1;
	else
		*spi = 0;
		
	if(strstr(cmd, "ram"))
		*ram = 1;
	else
		*ram = 0;
	
	if(strstr(cmd, "i2c"))
		*i2c = 1;
	else
		*i2c = 0;
	
	if(strstr(cmd, "usb"))
		*usb = 1;
	else
		*usb = 0;
	
	if(strstr(cmd, "eth"))
		*eth = 1;
	else
		*eth = 0;
}

static void parse_report(char *report, int *t, int *n, int *s, int *r, int *i, int *u, int *e)
{
	char *tmp = report, *tmp0;
	
	if(tmp = strstr(report, "t="))
	{
		tmp += strlen("t=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*t = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*t = 0;

	if(tmp = strstr(report, "n="))
	{
		tmp += strlen("n=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*n = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*n = 0;
	
	if(tmp = strstr(report, "s="))
	{
		tmp += strlen("s=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*s = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*s = 0;
	
	if(tmp = strstr(report, "r="))
	{
		tmp += strlen("r=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*r = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*r = 0;
	
	if(tmp = strstr(report, "i="))
	{
		tmp += strlen("i=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*i = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*i = 0;
	
	if(tmp = strstr(report, "u="))
	{
		tmp += strlen("u=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*u = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*u = 0;
	
	if(tmp = strstr(report, "e="))
	{
		tmp += strlen("e=");
		if((tmp0 = strstr(tmp, " ")) == NULL)
			tmp0 = strchr(tmp, '\0');
		tmp0 = '\0';
		*e = simple_strtoul(tmp, NULL, 10);
		tmp0 = ' ';
		tmp = tmp0 + 1;
	}
	else
		*e = 0;
}

int do_i2c_test(void)
{
	unsigned char wdata, rdata;
	
	if(i2c_set_bus_num(0))
	{
		printf("Failure changing to bus number 0\n");
		return 1;
	}
	
	if (i2c_read(0x70, 0x03, 1, &wdata, 1) != 0)
	{
		printf("Error reading from the chip 0x2e.\n");
		return 1;
	}
	
	wdata = ((wdata & 0x03) == 0x03) ? (wdata & ~0x03) : (wdata | 0x03);
	if (i2c_write(0x70, 0x03, 1, &wdata, 1) != 0)
	{
		printf("Error writing to the chip 0x2e.\n");
		return 1;
	}
	
	if (i2c_read(0x70, 0x03, 1, &rdata, 1) != 0)
	{
		printf("Error reading from the chip 0x2e.\n");
		return 1;
	}
	
	if(rdata != wdata)
	{
		printf("Failure writing to chip 0x2e (0x%2x, 0x%2x)\n", wdata, rdata);
		return 1;
	}
	
	printf("I2C test OK!\n");
	return 0;
}

int do_usb_test(void)
{
	block_dev_desc_t *stor_dev;
	int usb_stor_dev;
	
	if((usb_init() < 0) || (usb_stor_dev = usb_stor_scan(1)) < 0)
	{
		printf("There is no USB storage device.\n");
		return 1;
	}

	stor_dev = usb_stor_get_dev(0);
	if(stor_dev->block_read(usb_stor_dev, 0, 1, (ulong *)load_addr) != 1)
	{
		printf("Read from USB storage fail\n");
		return 1;
	}
	
	memset((load_addr + 0x100), 0x55, stor_dev->blksz);
	if(stor_dev->block_write(usb_stor_dev, 0, 1, (ulong *)(load_addr + stor_dev->blksz)) != 1)
	{
		printf("Write to USB storage fail\n");
		return 1;
	}
	
	if(stor_dev->block_read(usb_stor_dev, 0, 1, (ulong *)(load_addr + stor_dev->blksz * 2)) != 1)
	{
		printf("Read from USB storage fail\n");
		return 1;
	}
	
	if(stor_dev->block_write(usb_stor_dev, 0, 1, (ulong *)load_addr) != 1)
	{
		printf("Write back to USB storage fail\n");
		return 1;
	}
	
	if(memcmp((load_addr + stor_dev->blksz), (load_addr + stor_dev->blksz * 2), stor_dev->blksz))
	{
		printf("Read back data is not equal to writen data\n");
		return 1;
	}
	
	printf("USB test OK!\n");
	return 0;
}

void diag_create_report(int nand, int spi, int ram, int i2c, int usb, int eth)
{
	int dev = nand_curr_device;
	nand_info_t *nand_i = &nand_info[dev];
	nand_erase_options_t opts;
	size_t rwsize;
	char *report = malloc(DIAG_REPORT_SIZE + 1);
	
	memset(report, 0, (DIAG_REPORT_SIZE + 1));
	sprintf(report, "cmd=%s %s %s %s %s %s\nt=0 %s %s %s %s %s %s", nand ? "nand" : "", spi ? "spi" : "", ram ? "ram" : "", i2c ? "i2c" : "", usb ? "usb" : "", eth ? "eth" : "", nand ? "n=0" : "", spi ? "s=0" : "", ram ? "r=0" : "", i2c ? "i=0" : "", usb ? "u=0" : "", eth ? "e=0" : "");
	
	memset(&opts, 0, sizeof(opts));
	opts.offset = DIAG_REPORT_OFFSET;
	opts.length = DIAG_REPORT_SIZE;
	if(nand_erase_opts(nand_i, &opts))
	{
		printf("%s(%d) : erase fail\n", __FUNCTION__, __LINE__);
	}
	rwsize = DIAG_REPORT_SIZE;
	if(nand_write_skip_bad(nand_i, DIAG_REPORT_OFFSET, &rwsize, (u_char *)report, 0))
	{
		printf("%s(%d) : write fail\n", __FUNCTION__, __LINE__);
	}
	
	free(report);
}

void diag_remove_report(void)
{
	int dev = nand_curr_device;
	nand_info_t *nand_i = &nand_info[dev];
	nand_erase_options_t opts;
	
	memset(&opts, 0, sizeof(opts));
	opts.offset = DIAG_REPORT_OFFSET;
	opts.length = DIAG_REPORT_SIZE;
	if(nand_erase_opts(nand_i, &opts))
	{
		printf("%s(%d) : erase fail\n", __FUNCTION__, __LINE__);
	}
}

void diag_check_cmd(void)
{
	int dev = nand_curr_device, nand, spi, ram, i2c, usb, eth, t, n, s, r, i, u, e;
	nand_info_t *nand_i = &nand_info[dev];
	size_t rwsize;
	nand_erase_options_t opts;
	char *report = malloc(DIAG_REPORT_SIZE + 1), *cmd, *tmp;

	if(!report)
	{
		printf("%s(%d) : malloc 0x%x fail\n", __FUNCTION__, __LINE__, DIAG_REPORT_SIZE);
		return;
	}

	memset(report, 0, (DIAG_REPORT_SIZE + 1));
	rwsize = DIAG_REPORT_SIZE;
	if(nand_read_skip_bad(nand_i, DIAG_REPORT_OFFSET, &rwsize, (u_char *)report))
	{
		printf("%s(%d) : read fail\n", __FUNCTION__, __LINE__);
		goto diag_exit;
	}
	
	if(cmd = strstr(report, "cmd="))
	{
		cmd += strlen("cmd=");
		if((tmp = strstr(cmd, "\n")) == NULL)
		{
			if(tmp = strstr(cmd, "\0") == NULL)
			{
				printf("%s(%d) : incorrect format \n\n%s\n\n\n", __FUNCTION__, __LINE__, report);
				goto diag_exit;
			}
		}
		*tmp = '\0';
		tmp++;

		parse_cmd(cmd, &nand, &spi, &ram, &i2c, &usb, &eth);
		parse_report(tmp, &t, &n, &s, &r, &i, &u, &e);
		t++;

		if(nand)
		{
			if(do_nand_test((loff_t)DIAG_NAND_TEST_OFFSET, (loff_t)DIAG_NAND_TEST_SIZE))
				n++;
		}
		
		if(spi)
		{
			if(do_spi_flash_ptest(0, DIAG_SPI0_TEST_OFFSET, DIAG_SPI0_TEST_SIZE))
				s++;
		}
		
		if(ram)
		{
			if(do_mem_test(DIAG_RAM_TEST_START, DIAG_RAM_TEST_END))
				r++;
		}
		
		if(i2c)
		{
			if(do_i2c_test())
				i++;
		}
		
		if(usb)
		{
			if(do_usb_test())
				u++;
		}
		
		printf("Total : %d\n", t);
		if(nand)
			printf("Nand test fail : %d\n", n);
		if(spi)
			printf("SPI test fail : %d\n", s);
		if(ram)
			printf("RAM test fail : %d\n", r);
		if(i2c)
			printf("I2C test fail : %d\n", i);
		if(usb)
			printf("USB test fail : %d\n", u);
		if(eth)
			printf("ETHERNET test fail : %d\n", e);
	}
	else
		goto diag_exit;
	memset(report, 0, (DIAG_REPORT_SIZE + 1));
	sprintf(report, "cmd=%s %s %s %s %s %s\nt=%d n=%d s=%d r=%d i=%d u=%d e=%d", nand ? "nand" : "", spi ? "spi" : "", ram ? "ram" : "", i2c ? "i2c" : "", usb ? "usb" : "", eth ? "eth" : "", t, n, s, r, i, u, e);

	memset(&opts, 0, sizeof(opts));
	opts.offset = DIAG_REPORT_OFFSET;
	opts.length = DIAG_REPORT_SIZE;
	if(nand_erase_opts(nand_i, &opts))
	{
		printf("%s(%d) : erase fail\n", __FUNCTION__, __LINE__);
		goto diag_exit;
	}
	rwsize = DIAG_REPORT_SIZE;
	if(nand_write_skip_bad(nand_i, DIAG_REPORT_OFFSET, &rwsize, (u_char *)report, 0))
	{
		printf("%s(%d) : write fail\n", __FUNCTION__, __LINE__);
		goto diag_exit;
	}

diag_exit:
	free(report);
}

int do_diag(cmd_tbl_t *cmdtp, int flag, int argc, char * const argv[])
{
	int j, repeat = 0, nand = 0, spi = 0, ram = 0, i2c = 0, usb = 0, eth = 0, stop = 0, t = 0, n = 0, s = 0, r = 0, i = 0, u = 0, e = 0;
	
	for(j = 0; j < argc; j++)
	{
		if(strcmp(argv[j], "-r") == 0)
		{
			repeat = 1;
			continue;
		}

		if(strcmp(argv[j], "nand") == 0)
		{
			nand = 1;
			continue;
		}

		if(strcmp(argv[j], "spi") == 0)
		{
			spi = 1;
			continue;
		}

		if(strcmp(argv[j], "ram") == 0)
		{
			ram = 1;
			continue;
		}

		if(strcmp(argv[j], "i2c") == 0)
		{
			i2c = 1;
			continue;
		}

		if(strcmp(argv[j], "usb") == 0)
		{
			usb = 1;
			continue;
		}

		if(strcmp(argv[j], "eth") == 0)
		{
			eth = 1;
			continue;
		}
		
		if(strcmp(argv[j], "stop") == 0)
		{
			stop = 1;
			continue;
		}
	}
	
	if(stop)
	{
		diag_remove_report();
	}
	else if(!repeat)
	{
		if(nand)
		{
			if(do_nand_test((loff_t)DIAG_NAND_TEST_OFFSET, (loff_t)DIAG_NAND_TEST_SIZE))
				;
		}
		
		if(spi)
		{
			if(do_spi_flash_ptest(0, DIAG_SPI0_TEST_OFFSET, DIAG_SPI0_TEST_SIZE))
				;
		}
		
		if(ram)
		{
			if(do_mem_test(DIAG_RAM_TEST_START, DIAG_RAM_TEST_END))
				;
		}
		
		if(i2c)
		{
			if(do_i2c_test())
				;
		}
		
		if(usb)
		{
			if(do_usb_test())
				;
		}
	}
	else if(eth == 0)
	{
		while(1)
		{
			if (ctrlc()) {
				putc ('\n');
				break;
			}
			
			if(nand)
			{
				if(do_nand_test((loff_t)DIAG_NAND_TEST_OFFSET, (loff_t)DIAG_NAND_TEST_SIZE))
					n++;
			}
			
			if (ctrlc()) {
				putc ('\n');
				break;
			}
			
			if(spi)
			{
				if(do_spi_flash_ptest(0, DIAG_SPI0_TEST_OFFSET, DIAG_SPI0_TEST_SIZE))
					s++;
			}
			
			if (ctrlc()) {
				putc ('\n');
				break;
			}
			
			if(ram)
			{
				if(do_mem_test(DIAG_RAM_TEST_START, DIAG_RAM_TEST_END))
					r++;
			}
			
			if (ctrlc()) {
				putc ('\n');
				break;
			}
			
			if(i2c)
			{
				if(do_i2c_test())
					i++;
			}
			
			if (ctrlc()) {
				putc ('\n');
				break;
			}
			
			if(usb)
			{
				if(do_usb_test())
					u++;
			}
			t++;
		}
		
		printf("Total : %d\n", t);
		if(nand)
			printf("Nand test fail : %d\n", n);
		if(spi)
			printf("SPI test fail : %d\n", s);
		if(ram)
			printf("RAM test fail : %d\n", r);
		if(i2c)
			printf("I2C test fail : %d\n", i);
		if(usb)
			printf("USB test fail : %d\n", u);
	}
	else	/* repeate */
	{
		char *s = getenv ("bootcmd");
		diag_create_report(nand, spi, ram, i2c, usb, eth);
		diag_check_cmd();
		run_command(s, 0);
	}

	return 0;
}

U_BOOT_CMD(diag, 8, 1, do_diag,
	"diagnostic test",
	"[-r] [opt]    - [repeatedly]\n"
	" opt :\n"
	"   nand\n"
	"   spi\n"
	"   ram\n"
	"   i2c\n"
	"   usb\n"
	"   eth\n"
	"diag stop    - stop\n");
