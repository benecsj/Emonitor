#import urllib2
import requests
from time import sleep
print("Test Start")

counter = 0

while(counter <100):
    #sleep(0)
    counter = counter +1
    try:
        payload = {'id': counter}
        response = requests.get("http://192.168.2.109/status.json",params=payload)

        response.elapsed
        timeString = str(response.elapsed).split(":")[2].replace(".","")[:-3]
        timeInt = int(timeString)
        print("Id:"+str(counter)+" Resp:"+str(timeInt))
        if(timeInt > 1000):
            break

    except requests.ConnectionError, e:
        print(e)
        break
        
print("Test 'End")