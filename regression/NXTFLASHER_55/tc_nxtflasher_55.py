import unittest
import os, sys

file_dir = os.path.dirname(os.path.realpath(__file__))  # xxx/src/
base_root = os.path.dirname(file_dir)  # location where all projects reside

sys.path.append(base_root)

from ptb_api.SW_Test_flasher.src.class_dyntest import *


class NxtFlasher_55(Flashertest):
    """
    verify, that the flasher can be connected to the netX 90 rev0, if a SW is running
		NXHX90-JTAG board 7833 000 rev 3 20160
		JTAG port of netX via on board FTDI
		SDRAM required 7703 080 rev 1 20009
    """

    test_command_list = None

    binary_file_read_from_netx = None
    binary_file_write_to_netx = None

    def __init__(self):
        Flashertest.__init__(self)

    def pre_test_step(self):
        pass

    def init_command_array(self):
        iflash0_cal_info_page = os.path.join(file_dir, "test_files", "iflash0_info.bin_readback")
        iflash1_secure_com_info_page = os.path.join(file_dir, "test_files", "iflash1_info.bin_readback")
        iflash2_secure_app_info_page = os.path.join(file_dir, "test_files", "iflash2_info.bin_readback")
        iflash_com = os.path.join(file_dir, "test_files", "COM_iflash.bin")
        iflash_app = os.path.join(file_dir, "test_files", "APP_iflash.bin")
        openocd_bin_path = os.path.join(file_dir, "openocd-0.10.0", "bin", "openocd.exe")
        openocd_reset_netx = os.path.join(file_dir, "board", "reset_netx90_rev1.cfg")

        enable_flasher = {"flasher": True}
        enable_openocd = {"openocd": True, "bin_path": openocd_bin_path}
        self.command_structure = [
            # verify secure info pages have correct content
            [enable_flasher, "cli_flash.lua", "verify", self.plugin_name, "-b 2 -u 0 -cs 1 %s" % (iflash0_cal_info_page)],
            [enable_flasher, "cli_flash.lua", "verify", self.plugin_name, "-b 2 -u 0 -cs 1 %s" % (iflash1_secure_com_info_page)],
            [enable_flasher, "cli_flash.lua", "verify", self.plugin_name, "-b 2 -u 0 -cs 1 %s" % (iflash2_secure_app_info_page)],
            # erase whole int flash
            [enable_flasher, "cli_flash.lua", "erase", self.plugin_name, "-b 2 -u 3 -cs 0 -l 0xFFFFffff"],
            [enable_flasher, "cli_flash.lua", "erase", self.plugin_name, "-b 2 -u 2 -cs 0 -l 0xFFFFffff"],
            # write new data
            [enable_flasher, "cli_flash.lua", "flash", self.plugin_name, "-b 2 -u 3 -cs 0 %s" % (iflash_com)],
            [enable_flasher, "cli_flash.lua", "flash", self.plugin_name, "-b 2 -u 2 -cs 0 %s" % (iflash_app)],
            # do a power cycle / reset netX
            [enable_openocd, "--search %s --file %s --command shutdown" % (file_dir, openocd_reset_netx)],
            # This should work, if the fix is applied
            [enable_flasher, "cli_flash.lua", "flash", self.plugin_name, "-b 2 -u 2 -cs 0 %s" % (iflash_app)],

        ]

