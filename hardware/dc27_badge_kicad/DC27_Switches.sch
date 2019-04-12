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
Text GLabel 4950 3550 2    50   Output ~ 0
~A_DOWN
Text GLabel 2000 3550 0    50   Output ~ 0
~A_LEFT
Wire Wire Line
	4300 3700 4300 3900
Wire Wire Line
	4200 3700 4300 3700
$Comp
L power:GND #PWR?
U 1 1 5CC54DE5
P 4300 3900
AR Path="/5CC54DE5" Ref="#PWR?"  Part="1" 
AR Path="/5CBFA8FD/5CC54DE5" Ref="#PWR0801"  Part="1" 
F 0 "#PWR0801" H 4300 3650 50  0001 C CNN
F 1 "GND" H 4305 3727 50  0000 C CNN
F 2 "" H 4300 3900 50  0001 C CNN
F 3 "" H 4300 3900 50  0001 C CNN
	1    4300 3900
	1    0    0    -1  
$EndComp
$Comp
L DC27-badge:SKRHAAE010 SW?
U 1 1 5CC54DF7
P 3750 3750
AR Path="/5CC54DF7" Ref="SW?"  Part="1" 
AR Path="/5CBFA8FD/5CC54DF7" Ref="SW801"  Part="1" 
F 0 "SW801" H 3750 4415 50  0000 C CNN
F 1 "SKRHAAE010" H 3750 4324 50  0000 C CNN
F 2 "lib_fp:SKRHAAE010" H 3150 4150 50  0001 C CNN
F 3 "https://www.mouser.com/datasheet/2/15/SKRH-1370966.pdf" H 3150 4150 50  0001 C CNN
	1    3750 3750
	1    0    0    -1  
$EndComp
Text GLabel 4950 3400 2    50   Output ~ 0
~A_RIGHT
Text GLabel 2000 3400 0    50   Output ~ 0
~A_UP
Text GLabel 2000 3700 0    50   Output ~ 0
~A_SELECT
Text GLabel 9550 5450 2    50   Output ~ 0
~JOYPAD_INTR
$Comp
L Diode:BAT54AW D801
U 1 1 5CC6D920
P 2700 5200
F 0 "D801" H 2700 5425 50  0000 C CNN
F 1 "BAT54AW" H 2700 5334 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-323_SC-70" H 2775 5325 50  0001 L CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/BAT54W_SER.pdf" H 2580 5200 50  0001 C CNN
	1    2700 5200
	1    0    0    -1  
$EndComp
$Comp
L Diode:BAT54AW D802
U 1 1 5CC71DBD
P 3500 5200
F 0 "D802" H 3500 5425 50  0000 C CNN
F 1 "BAT54AW" H 3500 5334 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-323_SC-70" H 3575 5325 50  0001 L CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/BAT54W_SER.pdf" H 3380 5200 50  0001 C CNN
	1    3500 5200
	1    0    0    -1  
$EndComp
$Comp
L Diode:BAT54AW D803
U 1 1 5CC72EBD
P 4300 5200
F 0 "D803" H 4300 5425 50  0000 C CNN
F 1 "BAT54AW" H 4300 5334 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-323_SC-70" H 4375 5325 50  0001 L CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/BAT54W_SER.pdf" H 4180 5200 50  0001 C CNN
	1    4300 5200
	1    0    0    -1  
$EndComp
$Comp
L Diode:BAT54AW D804
U 1 1 5CC74091
P 5100 5200
F 0 "D804" H 5100 5425 50  0000 C CNN
F 1 "BAT54AW" H 5100 5334 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-323_SC-70" H 5175 5325 50  0001 L CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/BAT54W_SER.pdf" H 4980 5200 50  0001 C CNN
	1    5100 5200
	1    0    0    -1  
$EndComp
$Comp
L Diode:BAT54AW D805
U 1 1 5CC757FC
P 9150 5200
F 0 "D805" H 9150 5425 50  0000 C CNN
F 1 "BAT54AW" H 9150 5334 50  0000 C CNN
F 2 "Package_TO_SOT_SMD:SOT-323_SC-70" H 9225 5325 50  0001 L CNN
F 3 "https://assets.nexperia.com/documents/data-sheet/BAT54W_SER.pdf" H 9030 5200 50  0001 C CNN
	1    9150 5200
	1    0    0    -1  
$EndComp
Wire Wire Line
	9550 5450 9150 5450
Wire Wire Line
	2700 5450 2700 5400
Wire Wire Line
	3500 5400 3500 5450
Connection ~ 3500 5450
Wire Wire Line
	3500 5450 2700 5450
Wire Wire Line
	4300 5400 4300 5450
Connection ~ 4300 5450
Wire Wire Line
	4300 5450 3500 5450
Wire Wire Line
	5100 5400 5100 5450
Wire Wire Line
	5100 5450 4300 5450
Wire Wire Line
	9150 5400 9150 5450
Connection ~ 9150 5450
Wire Wire Line
	3000 5200 3050 5200
Wire Wire Line
	3050 5200 3050 3550
Wire Wire Line
	3050 3550 3300 3550
Wire Wire Line
	3150 3700 3150 5200
Wire Wire Line
	3150 5200 3200 5200
Wire Wire Line
	3150 3700 3300 3700
Wire Wire Line
	2000 3700 3150 3700
Connection ~ 3150 3700
Wire Wire Line
	2000 3550 3050 3550
Connection ~ 3050 3550
Wire Wire Line
	2000 3400 2200 3400
Wire Wire Line
	2400 5200 2200 5200
Wire Wire Line
	2200 5200 2200 3400
Connection ~ 2200 3400
Wire Wire Line
	2200 3400 3300 3400
Wire Wire Line
	4200 3400 4450 3400
Wire Wire Line
	4450 3400 4450 4850
Wire Wire Line
	4450 4850 3850 4850
Wire Wire Line
	3850 4850 3850 5200
Wire Wire Line
	3850 5200 3800 5200
Connection ~ 4450 3400
Wire Wire Line
	4450 3400 4950 3400
Wire Wire Line
	4000 5200 3950 5200
Wire Wire Line
	3950 5200 3950 4900
Wire Wire Line
	3950 4900 4600 4900
Wire Wire Line
	4600 4900 4600 3550
Wire Wire Line
	4200 3550 4600 3550
Connection ~ 4600 3550
Wire Wire Line
	4600 3550 4950 3550
Text GLabel 9350 3550 2    50   Output ~ 0
~B_DOWN
Text GLabel 6400 3550 0    50   Output ~ 0
~B_LEFT
Wire Wire Line
	8700 3700 8700 3900
Wire Wire Line
	8600 3700 8700 3700
$Comp
L power:GND #PWR?
U 1 1 5CC85D3D
P 8700 3900
AR Path="/5CC85D3D" Ref="#PWR?"  Part="1" 
AR Path="/5CBFA8FD/5CC85D3D" Ref="#PWR0802"  Part="1" 
F 0 "#PWR0802" H 8700 3650 50  0001 C CNN
F 1 "GND" H 8705 3727 50  0000 C CNN
F 2 "" H 8700 3900 50  0001 C CNN
F 3 "" H 8700 3900 50  0001 C CNN
	1    8700 3900
	1    0    0    -1  
$EndComp
$Comp
L DC27-badge:SKRHAAE010 SW?
U 1 1 5CC85D47
P 8150 3750
AR Path="/5CC85D47" Ref="SW?"  Part="1" 
AR Path="/5CBFA8FD/5CC85D47" Ref="SW802"  Part="1" 
F 0 "SW802" H 8150 4415 50  0000 C CNN
F 1 "SKRHAAE010" H 8150 4324 50  0000 C CNN
F 2 "lib_fp:SKRHAAE010" H 7550 4150 50  0001 C CNN
F 3 "https://www.mouser.com/datasheet/2/15/SKRH-1370966.pdf" H 7550 4150 50  0001 C CNN
	1    8150 3750
	1    0    0    -1  
$EndComp
Text GLabel 9350 3400 2    50   Output ~ 0
~B_RIGHT
Text GLabel 6400 3400 0    50   Output ~ 0
~B_UP
Text GLabel 6400 3700 0    50   Output ~ 0
~B_SELECT
Wire Wire Line
	6400 3400 6600 3400
Wire Wire Line
	4600 5200 4700 5200
Wire Wire Line
	4700 5200 4700 4050
Wire Wire Line
	4700 4050 6600 4050
Wire Wire Line
	6600 4050 6600 3400
Connection ~ 6600 3400
Wire Wire Line
	6600 3400 7700 3400
Wire Wire Line
	6400 3550 6750 3550
Wire Wire Line
	4800 5200 4750 5200
Wire Wire Line
	4750 5200 4750 4100
Wire Wire Line
	4750 4100 6750 4100
Wire Wire Line
	6750 4100 6750 3550
Connection ~ 6750 3550
Wire Wire Line
	8600 3400 8900 3400
Wire Wire Line
	6400 3700 6900 3700
Wire Wire Line
	6750 3550 7700 3550
Wire Wire Line
	5100 5450 9150 5450
Connection ~ 5100 5450
Wire Wire Line
	5400 5200 6900 5200
Wire Wire Line
	6900 5200 6900 3700
Connection ~ 6900 3700
Wire Wire Line
	6900 3700 7700 3700
Wire Wire Line
	8850 5200 8650 5200
Wire Wire Line
	8650 5200 8650 4200
Wire Wire Line
	8650 4200 8900 4200
Wire Wire Line
	8900 4200 8900 3400
Connection ~ 8900 3400
Wire Wire Line
	8900 3400 9350 3400
Wire Wire Line
	8600 3550 9100 3550
Wire Wire Line
	9450 5200 9650 5200
Wire Wire Line
	9650 5200 9650 4200
Wire Wire Line
	9650 4200 9100 4200
Wire Wire Line
	9100 4200 9100 3550
Connection ~ 9100 3550
Wire Wire Line
	9100 3550 9350 3550
$EndSCHEMATC
