# thinkbat

Monitor battery health for lenovo thinkpad. 

Compile C program and move program to local executables. Then run using command `thinkbat`:
```
$ gcc thinkbat.c -o thinkbat
$ mv thinkbat /usr/local/bin/thinkbat
$ thinkbat
BAT Type:
        Manufacturer: Celxpert
        Model: XXXXXXXXX
        Serial:  0000
BAT Status:
        Percentage: 77.54%
        State: Not charging
        Capacity: 86.41%
        Cycle count: #227
        Voltage: 12.331 V
        Start thresh: 75%
        Stop thresh: 80%
```

To change start and stop charging thresholds run:
```
$ thinkbat [start charging percentage] [stop charging percentage]
$ thinkbat 75 80
```