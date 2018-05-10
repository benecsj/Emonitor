import sys
import subprocess
import re
from random import randint

fname = "ids.txt"
fid = "data/id"
fpass = "data/pass"
flang = "data/lang"
parameterCount = len(sys.argv)

if parameterCount > 1 :
    print("ID update start")
    idreq = str(sys.argv[1])
    #Read older ids
    with open(fname) as f:
        content = f.readlines()
        
    if "rand" in idreq:
        #Generate new number not present in list
        number = str(randint(1000, 9999))
        while number+"\n" in content:
            number = str(randint(1000, 9999))
        #Store new number into list
        open(fname,"a+b").write(number+"\n")
        #Update id parameter
        idreq = number
        print("                          <<<<<<Auto ID: "+idreq+" >>>>>>")
    elif len(idreq) == 4: 
        #Check if id need to be stored
        if idreq+"\n" not in content:
            open(fname,"a+b").write(idreq+"\n")
        print("                          <<<<<<Manual ID: "+idreq+" >>>>>>")
    else:
        print("Invalid ID given")
        idreq = None
    if idreq != None:
        #Write new id into id file
        open(fid,"wb").write(idreq)
        #Write new password into pass file
        open(fpass,"wb").write("00"+idreq+"00")
if parameterCount > 2 :
    print("Language update start")
    lang = str(sys.argv[2])
    if len(lang) ==2:
        print("                          <<<<<<Language changed to "+lang+" >>>>>>")
        #Write new lang into lang file
        open(flang,"wb").write(lang)
    else:
        print("Invalid language given")
    