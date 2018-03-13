import binascii
import os
import re
import sys
path = './data'
fs_file = './test.c'
mod = 32

with open(fs_file,'w') as fs:
    for filename in os.listdir(path):
        with open(path+'/'+filename, 'rb') as f:
            content = f.read()
            hexString = binascii.hexlify(content)
            fs.write("uint8_t "+filename.replace(".","_")+"["+str(len(hexString)/2)+"] = {\n")
            counter = -1
            for hexPart in re.findall('..',hexString):
                if counter != -1:
                    fs.write(",")
                    if(counter %mod == (mod-1)):
                        fs.write("\n")
                        
                counter = counter +1 
                fs.write("0x"+hexPart)
            fs.write("}\n")
        