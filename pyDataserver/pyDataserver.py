import usb 
import array


MODE_DISABLED=0
MODE_SVMI=1
MODE_SIMV=2

def makePacket(modeA, modeB, aValues, bValues):
	out = chr(modeA) + chr(modeB)
	for a, b in zip(aValues, bValues):
		out += chr(a & 0xff) + chr((b&0x0f) << 4 | a>>8) + chr(b >> 4)
	return out

sys.stdout.write(makePacket(MODE_SVMI, MODE_SIMV, 5*[0]+5*[0xFFF], 10*[2151])*500)
