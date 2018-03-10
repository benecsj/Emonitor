#import urllib2
import requests

print("Test Start")

retry = 0

try:
    response = requests.get("http://192.168.2.109/status.json")

    response.elapsed
    timeString = str(response.elapsed).split(":")[2].replace(".","")[:-3]
    timeInt = int(timeString)
    print(timeInt)


except requests.ConnectionError, e:
    print(e)
    retry = retry + 1

        
print("Test 'End")