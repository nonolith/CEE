import usb.core
import time

setupTime = time.time()
dev = usb.core.find(idVendor=0x9999, idProduct=0xffff)
if not dev:
	raise IOError("device not found")
	
dev.set_configuration()

def b12unpack(s):
	"Turn a 3-byte string containing 2 12-bit values into two ints"
	return s[0]|((s[1]&0x0f)<<8), ((s[1]&0xf0) >> 4)|(s[2]<<4)
	
def readADC(wValue=0, wIndex=0):
	data = dev.ctrl_transfer(0x40|0x80, 0xA0, wValue, wIndex, 6)
	return b12unpack(data[0:3]) + b12unpack(data[3:6])

def writeDAC(wValue=0, wIndex=0):
	data = dev.ctrl_transfer(0x40|0x80, 0xB0, wValue, wIndex, 6)

