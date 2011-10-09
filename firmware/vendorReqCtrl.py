import usb.core

MODE_DISABLED=0
MODE_SVMI=1
MODE_SIMV=2

def unpackSign(n):
	return n - (1<<12) if n>2048 else n

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
		
	def readADC(self):
		data = self.dev.ctrl_transfer(0x40|0x80, 0xA0, 0, 0, 6)
		l = self.b12unpack(data[0:3]) + self.b12unpack(data[3:6])
		print l
		vals = map(unpackSign, l)
		return {
			'a_v': vals[0]/2048.0*2.5,
			'a_i': ((vals[1]/2048.0*2.5)-1.25)/45/.07,
			'b_v': vals[2]/2048.0*2.5,
			'b_i': ((vals[3]/2048.0*2.5)-1.25)/45/.07,
		}

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

	def bootload(self):
		self.dev.ctrl_transfer(0x40|0x80, 0xBB, 0, 0, 1)

if __name__ == "__main__":
	cee = CEE()
