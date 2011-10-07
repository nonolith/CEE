import usb.core

MODE_DISABLED=0
MODE_SVMI=1
MODE_SIMV=2

class CEE(object):
	def __init__(self):
		self.init()

	def init(self):
		self.dev = usb.core.find(idVendor=0x9999, idProduct=0xffff)
		if not self.dev:
			raise IOError("device not found")
			
		self.dev.set_configuration()

	def b12unpack(self, s):
		"Turn a 3-byte string containing 2 12-bit values into two ints"
		return s[0]|((s[1]&0x0f)<<8), ((s[1]&0xf0) >> 4)|(s[2]<<4)
		
	def readADC(self,wValue=0, wIndex=0):
		data = self.dev.ctrl_transfer(0x40|0x80, 0xA0, wValue, wIndex, 6)
		return self.b12unpack(data[0:3]) + self.b12unpack(data[3:6])

	def set(self, chan, v=None, i=None):
		cmd = 0xAA+chan
		if v is not None:
			dacval = int(round(v/5.0*4095))
			print dacval
			self.dev.ctrl_transfer(0x40|0x80, cmd, dacval, MODE_SVMI, 6)
		elif i is not None:
			dacval = int((2**12*(1.25+(45*.07*i)))/2.5)
			print dacval
			self.dev.ctrl_transfer(0x40|0x80, cmd, dacval, MODE_SIMV, 6)
		else:
			self.dev.ctrl_transfer(0x40|0x80, cmd, 0, MODE_DISABLED, 6)	

	def setA(self, v=None, i=None):
		self.set(0, v, i)
	
	def setB(self, v=None, i=None):
		self.set(1, v, i)

if __name__ == "__main__":
	cee = CEE()
