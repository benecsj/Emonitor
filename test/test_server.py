import requests
import keyboard
from time import sleep
print("Test Start")

counter = 0

while(counter <1000):
    if keyboard.is_pressed('q'):
        break
    #sleep(0.1)
    counter = counter +1
    try:
        payload = {'id': counter}
        response = requests.get("http://192.168.2.108/status.json",params=payload)

        response.elapsed
        timeString = str(response.elapsed).split(":")[2].replace(".","")[:-3]
        timeInt = int(timeString)
        print("Id:"+str(counter)+" Resp:"+str(timeInt))
        if(timeInt > 10000):
            break

    except requests.ConnectionError, e:
        print(e)
        break
        
print("Test 'End")