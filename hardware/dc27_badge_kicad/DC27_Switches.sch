EESchema Schematic File Version 4
LIBS:DC27-badge-cache
EELAYER 29 0
EELAYER END
$Descr A4 11693 8268
encoding utf-8
Sheet 8 8
Title ""
Date ""
Rev ""
Comp ""
Comment1 ""
Comment2 ""
Comment3 ""
Comment4 ""
$EndDescr
Text GLabel 5000 3050 2    50   Output ~ 0
~A_DOWN
Text GLabel 1700 3050 0    50   Output ~ 0
~A_LEFT
Wire Wire Line
	4000 3200 4000 3400
Wire Wire Line
	3900 3200 4000 3200
$Comp
L power:GND #PWR?
U 1 1 5CC54DE5
P 4000 3400
AR Path="/5CC54DE5" Ref="#PWR?"  Part="1" 
AR Path="/5CBFA8FD/5CC54DE5" Ref="#PWR0801"  Part="1" 
F 0 "#PWR0801" H 4000 3150 50  0001 C CNN
F 1 "GND" H 4005 3227 50  0000 C CNN
F 2 "" H 4000 3400 50  0001 C CNN
F 3 "" H 4000 3400 50  0001 C CNN
	1    4000 3400
	1    0    0    -1  
$EndComp
$Comp
L DC27-badge:SKRHAAE010 SW?
U 1 1 5CC54DF7
P 3450 3250
AR Path="/5CC54DF7" Ref="SW?"  Part="1" 
AR Path="/5CBFA8FD/5CC54DF7" Ref="SW801"  Part="1" 
F 0 "SW801" H 3450 3915 50  0000 C CNN
F 1 "SKRHAAE010" H 3450 3824 50  0000 C CNN
F 2 "lib_fp:SKRHAAE010" H 2850 3650 50  0001 C CNN
F 3 "https://www.mouser.com/datasheet/2/15/SKRH-1370966.pdf" H 2850 3650 50  0001 C CNN
	1    3450 3250
	1    0    0    -1  
$EndComp
Text GLabel 5000 2900 2    50   Output ~ 0
~A_RIGHT
Text GLabel 1700 2900 0    50   Output ~ 0
~A_UP
Text GLabel 1700 3200 0    50   Output ~ 0
~A_SELECT
Text GLabel 9750 3050 2    50   Output ~ 0
~B_DOWN
Text GLabel 6100 3050 0    50   Output ~ 0
~B_LEFT
Wire Wire Line
	8400 3200 8400 3400
Wire Wire Line
	8300 3200 8400 3200
$Comp
L power:GND #PWR?
U 1 1 5CC85D3D
P 8400 3400
AR Path="/5CC85D3D" Ref="#PWR?"  Part="1" 
AR Path="/5CBFA8FD/5CC85D3D" Ref="#PWR0802"  Part="1" 
F 0 "#PWR0802" H 8400 3150 50  0001 C CNN
F 1 "GND" H 8405 3227 50  0000 C CNN
F 2 "" H 8400 3400 50  0001 C CNN
F 3 "" H 8400 3400 50  0001 C CNN
	1    8400 3400
	1    0    0    -1  
$EndComp
$Comp
L DC27-badge:SKRHAAE010 SW?
U 1 1 5CC85D47
P 7850 3250
AR Path="/5CC85D47" Ref="SW?"  Part="1" 
AR Path="/5CBFA8FD/5CC85D47" Ref="SW802"  Part="1" 
F 0 "SW802" H 7850 3915 50  0000 C CNN
F 1 "SKRHAAE010" H 7850 3824 50  0000 C CNN
F 2 "lib_fp:SKRHAAE010" H 7250 3650 50  0001 C CNN
F 3 "https://www.mouser.com/datasheet/2/15/SKRH-1370966.pdf" H 7250 3650 50  0001 C CNN
	1    7850 3250
	1    0    0    -1  
$EndComp
Text GLabel 9750 2900 2    50   Output ~ 0
~B_RIGHT
Text GLabel 6100 2900 0    50   Output ~ 0
~B_UP
Text GLabel 6100 3200 0    50   Output ~ 0
~B_SELECT
Text GLabel 10000 4300 2    50   Output ~ 0
~JOYPAD_INTR
Wire Wire Line
	6100 3200 7350 3200
Wire Wire Line
	1700 2900 1800 2900
Wire Wire Line
	1700 3200 2900 3200
Wire Wire Line
	1700 3050 2350 3050
Wire Wire Line
	6100 3050 6800 3050
Wire Wire Line
	8300 2900 8600 2900
Wire Wire Line
	8300 3050 9300 3050
Wire Wire Line
	10000 4300 9300 4300
Wire Wire Line
	8600 4300 8600 3900
Wire Wire Line
	9300 3900 9300 4300
Connection ~ 9300 4300
Wire Wire Line
	9300 4300 8600 4300
Wire Wire Line
	9300 3600 9300 3050
Connection ~ 9300 3050
Wire Wire Line
	9300 3050 9750 3050
Wire Wire Line
	8600 3600 8600 2900
Connection ~ 8600 2900
Wire Wire Line
	8600 2900 9750 2900
Wire Wire Line
	8600 4300 7350 4300
Wire Wire Line
	6250 4300 6250 3900
Connection ~ 8600 4300
Wire Wire Line
	6250 3600 6250 2900
Wire Wire Line
	6100 2900 6250 2900
Connection ~ 6250 2900
Wire Wire Line
	6250 2900 7400 2900
Wire Wire Line
	6800 3600 6800 3050
Connection ~ 6800 3050
Wire Wire Line
	6800 3050 7400 3050
Wire Wire Line
	7350 3600 7350 3200
Connection ~ 7350 3200
Wire Wire Line
	7350 3200 7400 3200
Wire Wire Line
	7350 3900 7350 4300
Connection ~ 7350 4300
Wire Wire Line
	7350 4300 6800 4300
Wire Wire Line
	6800 3900 6800 4300
Connection ~ 6800 4300
Wire Wire Line
	6800 4300 6250 4300
Wire Wire Line
	4150 4300 4150 3900
Wire Wire Line
	4850 3900 4850 4300
Connection ~ 4850 4300
Wire Wire Line
	4850 4300 4150 4300
Wire Wire Line
	4850 3600 4850 3050
Wire Wire Line
	4150 3600 4150 2900
Wire Wire Line
	4150 4300 2900 4300
Wire Wire Line
	1800 4300 1800 3900
Connection ~ 4150 4300
Wire Wire Line
	1800 3600 1800 2900
Wire Wire Line
	2350 3600 2350 3050
Wire Wire Line
	2900 3600 2900 3200
Wire Wire Line
	2900 3900 2900 4300
Connection ~ 2900 4300
Wire Wire Line
	2900 4300 2350 4300
Wire Wire Line
	2350 3900 2350 4300
Connection ~ 2350 4300
Wire Wire Line
	2350 4300 1800 4300
Wire Wire Line
	3900 2900 4150 2900
Wire Wire Line
	3900 3050 4850 3050
Connection ~ 4850 3050
Wire Wire Line
	4850 3050 5000 3050
Wire Wire Line
	4850 4300 6250 4300
Connection ~ 6250 4300
Connection ~ 1800 2900
Wire Wire Line
	1800 2900 3000 2900
Connection ~ 2350 3050
Wire Wire Line
	2350 3050 3000 3050
Connection ~ 2900 3200
Wire Wire Line
	2900 3200 3000 3200
Connection ~ 4150 2900
Wire Wire Line
	4150 2900 5000 2900
Text Notes 1300 2150 0    79   ~ 16
Joypad Control\n
Text Notes 7350 7500 0    50   ~ 0
DC27 Badge - Joypad and Joypad Interrupt
Text Notes 8150 7650 0    50   ~ 0
4/12/2019\n
Text Notes 10850 7650 0    50   ~ 0
1\n
$Comp
L Diode:BAT42W-V D801
U 1 1 5CD4BB66
P 1800 3750
F 0 "D801" V 1754 3829 50  0000 L CNN
F 1 "BAT42W-V" V 1845 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 1800 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 1800 3750 50  0001 C CNN
	1    1800 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D802
U 1 1 5CD4EF76
P 2350 3750
F 0 "D802" V 2304 3829 50  0000 L CNN
F 1 "BAT42W-V" V 2395 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 2350 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 2350 3750 50  0001 C CNN
	1    2350 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D803
U 1 1 5CD4FC56
P 2900 3750
F 0 "D803" V 2854 3829 50  0000 L CNN
F 1 "BAT42W-V" V 2945 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 2900 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 2900 3750 50  0001 C CNN
	1    2900 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D804
U 1 1 5CD50617
P 4150 3750
F 0 "D804" V 4104 3829 50  0000 L CNN
F 1 "BAT42W-V" V 4195 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 4150 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 4150 3750 50  0001 C CNN
	1    4150 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D805
U 1 1 5CD510F2
P 4850 3750
F 0 "D805" V 4804 3829 50  0000 L CNN
F 1 "BAT42W-V" V 4895 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 4850 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 4850 3750 50  0001 C CNN
	1    4850 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D806
U 1 1 5CD5193B
P 6250 3750
F 0 "D806" V 6204 3829 50  0000 L CNN
F 1 "BAT42W-V" V 6295 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 6250 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 6250 3750 50  0001 C CNN
	1    6250 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D807
U 1 1 5CD52918
P 6800 3750
F 0 "D807" V 6754 3829 50  0000 L CNN
F 1 "BAT42W-V" V 6845 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 6800 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 6800 3750 50  0001 C CNN
	1    6800 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D808
U 1 1 5CD531EE
P 7350 3750
F 0 "D808" V 7304 3829 50  0000 L CNN
F 1 "BAT42W-V" V 7395 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 7350 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 7350 3750 50  0001 C CNN
	1    7350 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D809
U 1 1 5CD53BDE
P 8600 3750
F 0 "D809" V 8554 3829 50  0000 L CNN
F 1 "BAT42W-V" V 8645 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 8600 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 8600 3750 50  0001 C CNN
	1    8600 3750
	0    1    1    0   
$EndComp
$Comp
L Diode:BAT42W-V D810
U 1 1 5CD5488F
P 9300 3750
F 0 "D810" V 9254 3829 50  0000 L CNN
F 1 "BAT42W-V" V 9345 3829 50  0000 L CNN
F 2 "Diode_SMD:D_SOD-123" H 9300 3575 50  0001 C CNN
F 3 "http://www.vishay.com/docs/85660/bat42.pdf" H 9300 3750 50  0001 C CNN
	1    9300 3750
	0    1    1    0   
$EndComp
$EndSCHEMATC
