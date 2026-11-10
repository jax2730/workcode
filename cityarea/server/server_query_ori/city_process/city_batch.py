city_list= [
    "Beijing",
    "Shanghai",
    "Guangzhou",
    "Shenzhen",
]

import subprocess

for city in city_list:
    subprocess.call(["python", "city_process.py", city])