import time
import usb.core
import usb.util


dev = usb.core.find(idVendor=0x9999, idProduct=0xa023)

# was it found?
if dev is None:
    raise ValueError('Device not found')

# set the active configuration. With no arguments, the first
# configuration will be the active one
dev.set_configuration(0)

cfg = dev[0]
cfg.set()
intf = cfg[(0,0)]
endpoint = intf[0]
print endpoint

c = 0

def readADC():
	d = dev.read(0x81, 64)
	return d[0]|d[1]<<8, d[2]|d[3]<<8 
	
def xmit_i2c(buf):
	dev.write(0x02, buf)
	return dev.read(0x82, 64)
	
tstart = time.time()
while 1:
	c+= 1
	print 'ADC', c, time.time()-tstart, readADC()
	print 'i2c', xmit_i2c('abcdefg')
	
