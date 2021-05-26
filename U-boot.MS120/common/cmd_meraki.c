/*
 * (C) Copyright 2017 Cisco Systems, Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <common.h>
DECLARE_GLOBAL_DATA_PTR;

static int do_meraki_check_upgrade(cmd_tbl_t* cmdtp, int flag, int argc, char* const argv[])
{
    char cmd[128];
    uint8_t count;

    if (run_command("ubi check upgrade_retries", 0) != CMD_RET_SUCCESS)
        return CMD_RET_FAILURE;

    printf("Upgrade in progress\n");

    /* Read the retry counter */
    sprintf(cmd, "ubi read 0x%x upgrade_retries 1", (uintptr_t)&count);
    if (run_command(cmd, 0) != CMD_RET_SUCCESS) {
        return CMD_RET_FAILURE;
    }

    /* sanity */
    if (count > 5) {
        count = 5;
    }

    printf("Upgrade retry counter is: %d\n", count);
    if (!count) {
        printf("\nUpgrade retries 0!! Not booting part.new\n");
        return CMD_RET_FAILURE;
    }

    /* Update the retry counter */
    count--;
    sprintf(cmd, "ubi write 0x%x upgrade_retries 1", (uintptr_t)&count);
    if (run_command(cmd, 0) != CMD_RET_SUCCESS) {
        return CMD_RET_FAILURE;
    }

    return CMD_RET_SUCCESS;
}

U_BOOT_CMD(
    meraki_check_upgrade,	1,	0,	do_meraki_check_upgrade,
    "Test and decrement the upgrade counter",
    NULL
);
