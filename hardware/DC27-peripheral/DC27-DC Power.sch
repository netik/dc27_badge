EESchema Schematic File Version 4
LIBS:DC27-badge-cache
EELAYER 26 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 4 5
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text GLabel 10150 3950 2    50   Output ~ 0
VCC3V3
Text GLabel 10200 2200 2    50   Output ~ 0
VCC5V
$Comp
L Regulator_Switching:PAM2305AAB120 U5
U 1 1 5BA47689
P 8250 4050
F 0 "U5" H 8250 4417 50  0000 C CNN
F 1 "PAM2305AAB120" H 8250 4326 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:TSOT-23-5" H 8300 3800 50  0001 L CNN
F 3 "https://www.diodes.com/assets/Datasheets/PAM2305.pdf" H 8000 3700 50  0001 C CNN
	1    8250 4050
	1    0    0    -1  
$EndComp
$Comp
L mic2288:MIC2288 U3
U 1 1 5BA4B4D5
P 8250 2350
F 0 "U3" H 8250 2817 50  0000 C CNN
F 1 "MIC2288" H 8250 2726 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 7650 1600 50  0001 L CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/mic2290.pdf" H 8300 2150 50  0001 C CNN
	1    8250 2350
	1    0    0    -1  
$EndComp
$Comp
L Device:L L1
U 1 1 5BA4B59E
P 8250 1650
F 0 "L1" V 8072 1650 50  0000 C CNN
F 1 "4.7uH" V 8163 1650 50  0000 C CNN
F 2 "Inductor_SMD:L_0603_1608Metric" H 8250 1650 50  0001 C CNN
F 3 "~" H 8250 1650 50  0001 C CNN
	1    8250 1650
	0    1    1    0   
$EndComp
Wire Wire Line
	8650 2200 8850 2200
Wire Wire Line
	8400 1650 8850 1650
Wire Wire Line
	8850 1650 8850 2200
Connection ~ 8850 2200
Wire Wire Line
	8850 2200 9000 2200
$Comp
L Device:C C10
U 1 1 5BA4B8B2
P 7200 2450
F 0 "C10" H 7315 2496 50  0000 L CNN
F 1 "4.7uF" H 7315 2405 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 7238 2300 50  0001 C CNN
F 3 "~" H 7200 2450 50  0001 C CNN
	1    7200 2450
	1    0    0    -1  
$EndComp
Wire Wire Line
	7850 2200 7650 2200
Wire Wire Line
	7650 1650 7650 2200
Connection ~ 7650 2200
Wire Wire Line
	7650 2200 7650 2500
Wire Wire Line
	7650 2500 7850 2500
$Comp
L power:GND #PWR017
U 1 1 5BA4BB8A
P 8250 2900
F 0 "#PWR017" H 8250 2650 50  0001 C CNN
F 1 "GND" H 8255 2727 50  0000 C CNN
F 2 "" H 8250 2900 50  0001 C CNN
F 3 "" H 8250 2900 50  0001 C CNN
	1    8250 2900
	1    0    0    -1  
$EndComp
Wire Wire Line
	8250 2750 8250 2800
$Comp
L Device:R R7
U 1 1 5BA4BCF9
P 9450 2500
F 0 "R7" H 9520 2546 50  0000 L CNN
F 1 "5k62" H 9520 2455 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 9380 2500 50  0001 C CNN
F 3 "~" H 9450 2500 50  0001 C CNN
	1    9450 2500
	1    0    0    -1  
$EndComp
$Comp
L Device:R R8
U 1 1 5BA4BDAC
P 9450 3000
F 0 "R8" H 9520 3046 50  0000 L CNN
F 1 "1k87" H 9520 2955 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 9380 3000 50  0001 C CNN
F 3 "~" H 9450 3000 50  0001 C CNN
	1    9450 3000
	1    0    0    -1  
$EndComp
Wire Wire Line
	9450 2350 9450 2200
Wire Wire Line
	9450 2200 9300 2200
Wire Wire Line
	9450 2650 9450 2750
Wire Wire Line
	8650 2500 9250 2500
Wire Wire Line
	9250 2500 9250 2750
Wire Wire Line
	9250 2750 9450 2750
Connection ~ 9450 2750
Wire Wire Line
	9450 2750 9450 2850
$Comp
L power:GND #PWR018
U 1 1 5BA4C301
P 9450 3250
F 0 "#PWR018" H 9450 3000 50  0001 C CNN
F 1 "GND" H 9455 3077 50  0000 C CNN
F 2 "" H 9450 3250 50  0001 C CNN
F 3 "" H 9450 3250 50  0001 C CNN
	1    9450 3250
	1    0    0    -1  
$EndComp
Wire Wire Line
	9450 3150 9450 3200
$Comp
L Device:C C11
U 1 1 5BA4C4B0
P 9850 2750
F 0 "C11" H 9965 2796 50  0000 L CNN
F 1 "22uF" H 9965 2705 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 9888 2600 50  0001 C CNN
F 3 "~" H 9850 2750 50  0001 C CNN
	1    9850 2750
	1    0    0    -1  
$EndComp
Wire Wire Line
	9850 2600 9850 2200
Wire Wire Line
	9850 2200 9450 2200
Connection ~ 9450 2200
Wire Wire Line
	9850 2900 9850 3200
Wire Wire Line
	9850 3200 9450 3200
Connection ~ 9450 3200
Wire Wire Line
	9450 3200 9450 3250
$Comp
L Device:D_Schottky D33
U 1 1 5BA4CB93
P 9150 2200
F 0 "D33" H 9150 1984 50  0000 C CNN
F 1 "MBRA140" H 9150 2075 50  0000 C CNN
F 2 "Diode_SMD:D_SMA" H 9150 2200 50  0001 C CNN
F 3 "~" H 9150 2200 50  0001 C CNN
	1    9150 2200
	-1   0    0    1   
$EndComp
Connection ~ 9850 2200
Wire Wire Line
	7200 2800 8250 2800
Connection ~ 8250 2800
Wire Wire Line
	8250 2800 8250 2900
Wire Wire Line
	7200 2200 7200 2300
Wire Wire Line
	7200 2200 7650 2200
Wire Wire Line
	7200 2600 7200 2800
Text GLabel 6900 3350 0    50   Input ~ 0
VIN
Connection ~ 7650 2500
Wire Wire Line
	7650 3950 7850 3950
Wire Wire Line
	7650 3950 7650 4150
Wire Wire Line
	7650 4150 7850 4150
$Comp
L Device:C C14
U 1 1 5BA4F34A
P 7650 4500
F 0 "C14" H 7765 4546 50  0000 L CNN
F 1 "10uF" H 7765 4455 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 7688 4350 50  0001 C CNN
F 3 "~" H 7650 4500 50  0001 C CNN
	1    7650 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	7650 4150 7650 4350
Connection ~ 7650 4150
Wire Wire Line
	8250 4350 8250 4900
Wire Wire Line
	8250 4900 7650 4900
Wire Wire Line
	7650 4900 7650 4650
$Comp
L power:GND #PWR024
U 1 1 5BA50169
P 8250 5100
F 0 "#PWR024" H 8250 4850 50  0001 C CNN
F 1 "GND" H 8255 4927 50  0000 C CNN
F 2 "" H 8250 5100 50  0001 C CNN
F 3 "" H 8250 5100 50  0001 C CNN
	1    8250 5100
	1    0    0    -1  
$EndComp
Wire Wire Line
	8250 4900 8250 5100
Connection ~ 8250 4900
Wire Wire Line
	8100 1650 7650 1650
$Comp
L Device:L L2
U 1 1 5BA519CD
P 9450 3950
F 0 "L2" V 9272 3950 50  0000 C CNN
F 1 "4.7uH" V 9363 3950 50  0000 C CNN
F 2 "Inductor_SMD:L_0603_1608Metric" H 9450 3950 50  0001 C CNN
F 3 "~" H 9450 3950 50  0001 C CNN
	1    9450 3950
	0    1    1    0   
$EndComp
Wire Wire Line
	8650 3950 9300 3950
Wire Wire Line
	9600 3950 9850 3950
Wire Wire Line
	8650 4150 9850 4150
Wire Wire Line
	9850 4150 9850 3950
Connection ~ 9850 3950
Wire Wire Line
	9850 3950 10000 3950
$Comp
L Device:C C15
U 1 1 5BA53CD6
P 10000 4550
F 0 "C15" H 10115 4596 50  0000 L CNN
F 1 "22uF" H 10115 4505 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 10038 4400 50  0001 C CNN
F 3 "~" H 10000 4550 50  0001 C CNN
	1    10000 4550
	1    0    0    -1  
$EndComp
Wire Wire Line
	10000 4400 10000 3950
Connection ~ 10000 3950
Wire Wire Line
	10000 3950 10150 3950
Wire Wire Line
	8300 4900 10000 4900
Wire Wire Line
	10000 4900 10000 4700
Wire Wire Line
	7650 2500 7650 3350
Connection ~ 7650 3950
Wire Wire Line
	6900 3350 7650 3350
Connection ~ 7650 3350
Wire Wire Line
	7650 3350 7650 3950
Wire Wire Line
	9850 2200 10050 2200
Wire Wire Line
	10050 2200 10050 1900
Connection ~ 10050 2200
Wire Wire Line
	10050 2200 10200 2200
$Comp
L Battery_Management:MCP73811T-435I-OT U4
U 1 1 5BA59662
P 3700 3050
F 0 "U4" H 3200 3600 50  0000 L CNN
F 1 "MCP73811T-435I-OT" H 3200 3500 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-23-5" H 3750 2800 50  0001 L CNN
F 3 "http://ww1.microchip.com/downloads/en/DeviceDoc/22036b.pdf" H 3450 3300 50  0001 C CNN
	1    3700 3050
	1    0    0    -1  
$EndComp
Text Notes 6300 700  0    79   ~ 16
DC/DC Supply +3v, +5v
Wire Notes Line
	6250 500  6250 5500
Wire Notes Line
	6250 5500 11150 5500
Text Notes 650  750  0    79   ~ 16
LiPo Charging and USB Power\n
$Comp
L power:GND #PWR021
U 1 1 5BA5B6B7
P 1250 3550
F 0 "#PWR021" H 1250 3300 50  0001 C CNN
F 1 "GND" H 1255 3377 50  0000 C CNN
F 2 "" H 1250 3550 50  0001 C CNN
F 3 "" H 1250 3550 50  0001 C CNN
	1    1250 3550
	1    0    0    -1  
$EndComp
NoConn ~ 1150 3250
$Comp
L Device:Fuse_Small F1
U 1 1 5BA5DFA5
P 1350 2850
F 0 "F1" H 1350 3035 50  0000 C CNN
F 1 "1.1A" H 1350 2944 50  0000 C CNN
F 2 "Fuse:Fuse_0603_1608Metric" H 1350 2850 50  0001 C CNN
F 3 "~" H 1350 2850 50  0001 C CNN
	1    1350 2850
	1    0    0    -1  
$EndComp
Text GLabel 2100 3150 2    50   Output ~ 0
USBDATA+
Text GLabel 2100 3050 2    50   Output ~ 0
USBDATA-
$Comp
L Device:C C12
U 1 1 5BA6D774
P 2850 3100
F 0 "C12" H 2600 3050 50  0000 L CNN
F 1 "4.7uF" H 2600 2950 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 2888 2950 50  0001 C CNN
F 3 "~" H 2850 3100 50  0001 C CNN
	1    2850 3100
	1    0    0    -1  
$EndComp
Wire Wire Line
	3200 2850 3200 2650
Wire Wire Line
	3200 2650 3700 2650
Wire Wire Line
	3700 2650 3700 2750
$Comp
L power:GND #PWR019
U 1 1 5BA70BE9
P 3700 3550
F 0 "#PWR019" H 3700 3300 50  0001 C CNN
F 1 "GND" H 3705 3377 50  0000 C CNN
F 2 "" H 3700 3550 50  0001 C CNN
F 3 "" H 3700 3550 50  0001 C CNN
	1    3700 3550
	1    0    0    -1  
$EndComp
Wire Wire Line
	3700 3550 3700 3450
Wire Wire Line
	2850 2950 2850 2850
Connection ~ 2850 2850
Wire Wire Line
	2850 2850 3000 2850
Wire Wire Line
	2850 3250 2850 3450
Wire Wire Line
	2850 3450 3700 3450
Connection ~ 3700 3450
Wire Wire Line
	3700 3450 3700 3350
$Comp
L Connector:Conn_01x02_Male J1
U 1 1 5BA77BCE
P 5350 2950
F 0 "J1" H 5323 2923 50  0000 R CNN
F 1 "Conn_01x02_Male" H 5323 2832 50  0000 R CNN
F 2 "Connector:JWT_A3963_1x02_P3.96mm_Vertical" H 5350 2950 50  0001 C CNN
F 3 "~" H 5350 2950 50  0001 C CNN
	1    5350 2950
	-1   0    0    -1  
$EndComp
Text Notes 5500 3000 0    50   ~ 0
JST for Battery\n Pin 1 hot\n
Wire Wire Line
	5150 3050 5000 3050
Wire Wire Line
	5000 3050 5000 3450
Wire Wire Line
	5000 3450 4550 3450
$Comp
L Device:C C13
U 1 1 5BA86FCC
P 4550 3200
F 0 "C13" H 4665 3246 50  0000 L CNN
F 1 "4.7uF" H 4665 3155 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 4588 3050 50  0001 C CNN
F 3 "~" H 4550 3200 50  0001 C CNN
	1    4550 3200
	1    0    0    -1  
$EndComp
Wire Wire Line
	4550 3050 4550 2950
Connection ~ 4550 2950
Wire Wire Line
	4550 2950 4900 2950
Wire Wire Line
	4550 3350 4550 3450
Connection ~ 4550 3450
Wire Wire Line
	4550 3450 3700 3450
Text GLabel 5050 2550 2    50   Output ~ 0
VBATT
Wire Wire Line
	5050 2550 4900 2550
Wire Wire Line
	4900 2550 4900 2950
Connection ~ 4900 2950
Wire Wire Line
	4900 2950 5150 2950
Text Notes 6400 3650 0    50   Italic 0
Expecting either +5 (VUSB) or \n3.7V (VBATT) here
$Comp
L Device:LED D36
U 1 1 5BA8E741
P 3150 3800
F 0 "D36" V 3095 3878 50  0000 L CNN
F 1 "LED" V 3186 3878 50  0000 L CNN
F 2 "LED_SMD:LED_0603_1608Metric" H 3150 3800 50  0001 C CNN
F 3 "~" H 3150 3800 50  0001 C CNN
	1    3150 3800
	0    1    1    0   
$EndComp
Wire Wire Line
	3300 2950 3150 2950
Wire Wire Line
	3150 2950 3150 3650
$Comp
L Device:R R9
U 1 1 5BA91391
P 3150 4200
F 0 "R9" H 3220 4246 50  0000 L CNN
F 1 "330" H 3220 4155 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 3080 4200 50  0001 C CNN
F 3 "~" H 3150 4200 50  0001 C CNN
	1    3150 4200
	1    0    0    -1  
$EndComp
Wire Wire Line
	3150 3950 3150 4050
Wire Wire Line
	3000 2850 3000 4350
Wire Wire Line
	3000 4350 3150 4350
Connection ~ 3000 2850
Wire Wire Line
	3000 2850 3200 2850
$Comp
L Device:R R10
U 1 1 5BA968D8
P 3500 4200
F 0 "R10" H 3570 4246 50  0000 L CNN
F 1 "2.0k" H 3570 4155 50  0000 L CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 3430 4200 50  0001 C CNN
F 3 "~" H 3500 4200 50  0001 C CNN
	1    3500 4200
	1    0    0    -1  
$EndComp
Wire Wire Line
	3300 3150 3200 3150
Wire Wire Line
	3200 3150 3200 3350
Wire Wire Line
	3200 3350 3500 3350
Wire Wire Line
	3500 3350 3500 4050
$Comp
L power:GND #PWR022
U 1 1 5BA998E5
P 3500 4500
F 0 "#PWR022" H 3500 4250 50  0001 C CNN
F 1 "GND" H 3505 4327 50  0000 C CNN
F 2 "" H 3500 4500 50  0001 C CNN
F 3 "" H 3500 4500 50  0001 C CNN
	1    3500 4500
	1    0    0    -1  
$EndComp
Wire Wire Line
	3500 4350 3500 4500
$Comp
L max17043:MAX17043 U6
U 1 1 5BAA0329
P 2750 5600
F 0 "U6" H 2850 6150 50  0000 C CNN
F 1 "MAX17043" H 3000 6050 50  0000 C CNN
F 2 "Package_DFN_QFN:DFN-8-1EP_2x3mm_P0.5mm_EP0.61x2.2mm" H 2750 5550 50  0001 C CNN
F 3 "" H 2750 5550 50  0001 C CNN
	1    2750 5600
	1    0    0    -1  
$EndComp
$Comp
L Device:R R11
U 1 1 5BAA05A1
P 3600 5700
F 0 "R11" V 3807 5700 50  0000 C CNN
F 1 "1k" V 3716 5700 50  0000 C CNN
F 2 "Resistor_SMD:R_0603_1608Metric" V 3530 5700 50  0001 C CNN
F 3 "~" H 3600 5700 50  0001 C CNN
	1    3600 5700
	0    -1   -1   0   
$EndComp
Text GLabel 4000 5700 2    50   Input ~ 0
VBATT
Wire Wire Line
	3750 5700 4000 5700
Wire Wire Line
	3150 5700 3450 5700
$Comp
L Device:C C16
U 1 1 5BAA6C60
P 1700 4950
F 0 "C16" H 1815 4996 50  0000 L CNN
F 1 "1.0uF" H 1815 4905 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1738 4800 50  0001 C CNN
F 3 "~" H 1700 4950 50  0001 C CNN
	1    1700 4950
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR023
U 1 1 5BAA710D
P 1700 4650
F 0 "#PWR023" H 1700 4500 50  0001 C CNN
F 1 "+3.3V" H 1715 4823 50  0000 C CNN
F 2 "" H 1700 4650 50  0001 C CNN
F 3 "" H 1700 4650 50  0001 C CNN
	1    1700 4650
	1    0    0    -1  
$EndComp
$Comp
L power:GND #PWR027
U 1 1 5BAADAD5
P 2400 6300
F 0 "#PWR027" H 2400 6050 50  0001 C CNN
F 1 "GND" H 2405 6127 50  0000 C CNN
F 2 "" H 2400 6300 50  0001 C CNN
F 3 "" H 2400 6300 50  0001 C CNN
	1    2400 6300
	1    0    0    -1  
$EndComp
Wire Wire Line
	2400 6200 2400 6300
Wire Wire Line
	2400 6200 2750 6200
Wire Wire Line
	2750 6200 2750 6100
Connection ~ 2400 6200
Text Notes 600  4350 0    50   ~ 10
OPTIONAL Fuel Gauge Circuit\n
Wire Wire Line
	1700 4650 1700 4750
$Comp
L power:GND #PWR025
U 1 1 5BAB8F83
P 1700 5200
F 0 "#PWR025" H 1700 4950 50  0001 C CNN
F 1 "GND" H 1705 5027 50  0000 C CNN
F 2 "" H 1700 5200 50  0001 C CNN
F 3 "" H 1700 5200 50  0001 C CNN
	1    1700 5200
	1    0    0    -1  
$EndComp
Wire Wire Line
	1700 5100 1700 5200
Wire Wire Line
	2750 4750 1700 4750
Wire Wire Line
	2750 4750 2750 5150
Connection ~ 1700 4750
Wire Wire Line
	1700 4750 1700 4800
Text GLabel 2100 5400 0    50   BiDi ~ 0
SDA1
Wire Wire Line
	2100 5400 2350 5400
Text GLabel 2100 5500 0    50   Input ~ 0
SCL1
Wire Wire Line
	2100 5500 2350 5500
Wire Wire Line
	2350 5600 1800 5600
Wire Wire Line
	1800 6200 2400 6200
NoConn ~ 2350 5700
Text Notes 2800 6250 0    50   ~ 0
MAX17043 i2c address 0x36\nCould send ALT to a interrupt pin for power down alerting
$Comp
L power:GND #PWR026
U 1 1 5BADA991
P 3300 5800
F 0 "#PWR026" H 3300 5550 50  0001 C CNN
F 1 "GND" H 3305 5627 50  0000 C CNN
F 2 "" H 3300 5800 50  0001 C CNN
F 3 "" H 3300 5800 50  0001 C CNN
	1    3300 5800
	1    0    0    -1  
$EndComp
Wire Wire Line
	3150 5600 3300 5600
Wire Wire Line
	3300 5600 3300 5800
$Comp
L Device:D_Schottky D35
U 1 1 5BADFB0B
P 4300 2500
F 0 "D35" V 4254 2579 50  0000 L CNN
F 1 "MBRA140" V 4345 2579 50  0000 L CNN
F 2 "Diode_SMD:D_SMA" H 4300 2500 50  0001 C CNN
F 3 "~" H 4300 2500 50  0001 C CNN
	1    4300 2500
	0    1    1    0   
$EndComp
Wire Wire Line
	4300 2650 4300 2950
Connection ~ 4300 2950
Wire Wire Line
	4300 2950 4550 2950
Wire Wire Line
	2650 2850 2650 2150
Wire Wire Line
	2650 2150 4300 2150
Wire Wire Line
	4300 2150 4300 2350
Connection ~ 2650 2850
Wire Wire Line
	2650 2850 2850 2850
Text GLabel 5050 2150 2    50   Output ~ 0
VIN
Wire Wire Line
	4300 2150 4900 2150
Connection ~ 4300 2150
Text Notes 3550 3950 0    50   Italic 0
TODO:Maybe a bicolor LED here for charging/charged\n
Wire Wire Line
	1800 5600 1800 6200
$Comp
L Connector:TestPoint_2Pole TP2
U 1 1 5BB79543
P 10650 1900
F 0 "TP2" H 10650 2095 50  0000 C CNN
F 1 "5v TEST" H 10650 2004 50  0000 C CNN
F 2 "TestPoint:TestPoint_2Pads_Pitch2.54mm_Drill0.8mm" H 10650 1900 50  0001 C CNN
F 3 "~" H 10650 1900 50  0001 C CNN
	1    10650 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	10050 1900 10450 1900
Connection ~ 10050 1900
$Comp
L power:GND #PWR0110
U 1 1 5BB7E435
P 11000 2000
F 0 "#PWR0110" H 11000 1750 50  0001 C CNN
F 1 "GND" H 11005 1827 50  0000 C CNN
F 2 "" H 11000 2000 50  0001 C CNN
F 3 "" H 11000 2000 50  0001 C CNN
	1    11000 2000
	1    0    0    -1  
$EndComp
Wire Wire Line
	10850 1900 11000 1900
Wire Wire Line
	11000 1900 11000 2000
$Comp
L Connector:TestPoint_2Pole TP4
U 1 1 5BB83579
P 10650 3700
F 0 "TP4" H 10650 3895 50  0000 C CNN
F 1 "3.3V TEST" H 10650 3804 50  0000 C CNN
F 2 "TestPoint:TestPoint_2Pads_Pitch2.54mm_Drill0.8mm" H 10650 3700 50  0001 C CNN
F 3 "~" H 10650 3700 50  0001 C CNN
	1    10650 3700
	1    0    0    -1  
$EndComp
$Comp
L power:+5V #PWR020
U 1 1 5BA571D7
P 10050 1700
F 0 "#PWR020" H 10050 1550 50  0001 C CNN
F 1 "+5V" H 10065 1873 50  0000 C CNN
F 2 "" H 10050 1700 50  0001 C CNN
F 3 "" H 10050 1700 50  0001 C CNN
	1    10050 1700
	1    0    0    -1  
$EndComp
$Comp
L power:+3.3V #PWR016
U 1 1 5BA588DB
P 10000 3500
F 0 "#PWR016" H 10000 3350 50  0001 C CNN
F 1 "+3.3V" H 10015 3673 50  0000 C CNN
F 2 "" H 10000 3500 50  0001 C CNN
F 3 "" H 10000 3500 50  0001 C CNN
	1    10000 3500
	1    0    0    -1  
$EndComp
Wire Wire Line
	10050 1700 10050 1900
Wire Wire Line
	10000 3500 10000 3700
Wire Wire Line
	10000 3700 10450 3700
Connection ~ 10000 3700
Wire Wire Line
	10000 3700 10000 3950
$Comp
L power:GND #PWR0111
U 1 1 5BB92E27
P 10950 3900
F 0 "#PWR0111" H 10950 3650 50  0001 C CNN
F 1 "GND" H 10955 3727 50  0000 C CNN
F 2 "" H 10950 3900 50  0001 C CNN
F 3 "" H 10950 3900 50  0001 C CNN
	1    10950 3900
	1    0    0    -1  
$EndComp
Wire Wire Line
	10850 3700 10950 3700
Wire Wire Line
	10950 3700 10950 3900
$Comp
L Connector:TestPoint TP3
U 1 1 5BB988A5
P 4900 2400
F 0 "TP3" H 4958 2520 50  0000 L CNN
F 1 "VBATT" H 4958 2429 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 5100 2400 50  0001 C CNN
F 3 "~" H 5100 2400 50  0001 C CNN
	1    4900 2400
	1    0    0    -1  
$EndComp
Connection ~ 4900 2550
$Comp
L Connector:TestPoint TP1
U 1 1 5BB9DD2E
P 4900 1900
F 0 "TP1" H 4958 2020 50  0000 L CNN
F 1 "VIN" H 4958 1929 50  0000 L CNN
F 2 "TestPoint:TestPoint_Pad_D1.0mm" H 5100 1900 50  0001 C CNN
F 3 "~" H 5100 1900 50  0001 C CNN
	1    4900 1900
	1    0    0    -1  
$EndComp
Wire Wire Line
	4900 2400 4900 2550
Wire Wire Line
	4900 1900 4900 2150
Connection ~ 4900 2150
Wire Wire Line
	4900 2150 5050 2150
Text Notes 6400 5200 0    50   ~ 10
TODO: Do we even need the 5v supply?\n
Wire Wire Line
	4150 2950 4300 2950
Wire Wire Line
	4100 2950 4300 2950
$Comp
L Power_Protection:TPD3E001DRLR U8
U 1 1 5BABCB94
P 1950 2150
F 0 "U8" V 1550 2400 50  0000 L CNN
F 1 "TPD3E001DRLR" V 1650 2400 50  0000 L CNN
F 2 "Package_TO_SOT_SMD:SOT-543" H 1250 1850 50  0001 C CNN
F 3 "http://www.ti.com/lit/ds/symlink/tpd3e001.pdf" H 1750 2400 50  0001 C CNN
	1    1950 2150
	0    1    1    0   
$EndComp
Wire Wire Line
	1150 2850 1250 2850
Wire Wire Line
	1150 3050 2050 3050
Wire Wire Line
	1150 3150 1950 3150
Wire Wire Line
	1450 2850 1500 2850
Wire Wire Line
	1500 1550 1650 1550
Wire Wire Line
	1950 1550 1950 1750
$Comp
L Device:C C24
U 1 1 5BADF3B4
P 1650 1750
F 0 "C24" H 1700 2100 50  0000 L CNN
F 1 "0.1uF" H 1700 2000 50  0000 L CNN
F 2 "Capacitor_SMD:C_0603_1608Metric" H 1688 1600 50  0001 C CNN
F 3 "~" H 1650 1750 50  0001 C CNN
	1    1650 1750
	1    0    0    -1  
$EndComp
Wire Wire Line
	1650 1600 1650 1550
Connection ~ 1650 1550
Wire Wire Line
	1650 1550 1950 1550
Wire Wire Line
	1650 1900 1650 2150
Wire Wire Line
	1950 2550 1950 3150
Connection ~ 1950 3150
NoConn ~ 1850 2550
Text Notes 2200 1650 0    50   ~ 0
ESD Protection\n
Wire Wire Line
	1950 3150 2100 3150
Wire Wire Line
	2050 2550 2050 3050
Connection ~ 2050 3050
Wire Wire Line
	2050 3050 2100 3050
Wire Wire Line
	1500 2850 2650 2850
Connection ~ 1500 2850
$Comp
L MF_Connectors:USB_MICRO_RIGHT J2
U 1 1 5BB6E9A0
P 850 3150
F 0 "J2" H 906 3604 45  0000 C CNN
F 1 "USB_MICRO_RIGHT" H 906 3520 45  0000 C CNN
F 2 "MF_Connectors:MF_Connectors-MICROUSB-RIGHT" H 880 3300 20  0001 C CNN
F 3 "" H 850 3150 60  0000 C CNN
	1    850  3150
	1    0    0    -1  
$EndComp
Wire Wire Line
	1500 1550 1500 2850
Wire Wire Line
	1150 2850 1150 2950
Wire Wire Line
	1250 3550 1250 3450
Wire Wire Line
	1250 3350 1150 3350
Wire Wire Line
	1150 3450 1250 3450
Connection ~ 1250 3450
Wire Wire Line
	1250 3450 1250 3350
Text Notes 7350 7500 0    50   ~ 0
DC 27 Badge - Team Ides - John Adams / Bill Paul
$EndSCHEMATC
