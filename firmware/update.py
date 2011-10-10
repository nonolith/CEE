#!/usr/bin/python
import sys, os

pathToBootloader = os.path.join(os.path.dirname(__file__), 'usb', 'bootloader')

sys.path.append(pathToBootloader)

from flash import *
	
try:
	b = Bootloader()
	b.handle_args(sys.argv)
except AttributeError:
	usb.core.find(idVendor=0x9999, idProduct=0xffff).ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)
	time.sleep(1)
	b = Bootloader()
	b.handle_args(sys.argv)
else:
	print "can't find CEE or xmega in bootloader mode"
	exit()
