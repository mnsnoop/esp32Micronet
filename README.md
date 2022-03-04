# esp32Micronet

This project communicates with Tacktick/Raymarine Micronet devices using an ESP32 coupled with a CC1000 or CC1101 radio module. It is capable of sending and receiving data as well as hosting the network or joining as a node. If you add a CAN bus transceiver module it is capable of bridging the Micronet to a NMEA2000 network. Also included is code to read data from an analog AIRMAR ST850-P17 speed transducer and Raymarine masthead wind transducer.

It is rough and needs a lot of testing but functional. 

## What it does:
• Can use either a CC1000 or CC1101 radio chip.  
• Can connect to a Micronet as either a node or host its own network.  
• Uses proper window timing for all 4 known network round robins.  
• Can send and receive all known data PIDs including node info (it identifies as a type 8 device in settings->health).  
• Handles enough command packets to not get booted off the network.  
• Handles 0x0A/0x0B device status packets.  
• Handles remote shutdown messages.  
• Monitors NMEA2k network for PIDs the network can display and forwards them to the Micronet.  
• Monitors Micronet for PIDs the NMEA2k network can use and forwards them.  
• Can dump packets and monitor other networks.  
• Scrolling waypoint names on Dual Displays.  

## Known issues:
• The CC1101's settings are not completely correct for communicating with the older CC1000 radios. This limits range to about half.  
• Settings are hardcoded and should be moved to flash.  
• A lot of the N2k < Micronet bridge is untested as I don't have the right equipment.  
• There's a few areas in the Micronet class that need to be made thread safe (data structs)  
• Tons of fixme's.  

## Needed libraries:
For CC1101  
	-SPI  
	
For N2kbridge  
	-NMEA2000 (https://github.com/ttlappalainen/NMEA2000)  
	
For transducers  
	-RunningAngle (https://github.com/RobTillaart/runningAngle)  
	-RunningAverage (https://github.com/RobTillaart/RunningAverage)  
		
## Quick start guide
Download the project and either comment out the parts you'll not be using (transducers and/or bridge) or download the needed libraries for them.  
Open radio.h and select which radio you'll be using, the pins it is connected to, the frequincy your gear uses, desired power level and your network's ID if you know it.  
Compile, upload and hopefully you'll start seeing packets from your network. If so you can get its ID and update radio.h.  

## Wiring
Most pins on the esp32 can be used but I'd stay away from 0, 1, 3, and 6 - 12 because they can interfere with the boot process. This is how I've wired my project.  
![schematic](https://github.com/mnsnoop/esp32Micronet/blob/main/misc/esp32micronetbridge.png)

## NMEA2000 Message information
	esp32Micronet will receive the following messages:
	PGN59392
	PGN59904
	PGN60928
	PGN65240
	PGN126208
	PGN126464
	PGN126993
	PGN126996
	PGN126998
	PGN128259
	PGN128275
	PGN130310
	PGN128267
	PGN130306
	PGN127250
	PGN129026
	PGN129029
	PGN129284
	PGN129285
	PGN129283
	PGN127508
	PGN129033
	PGN126992
		
	esp32Micronet can transmit the following messages:
	PGN59392
	PGN59904
	PGN60928
	PGN65240
	PGN126208
	PGN126464
	PGN126993
	PGN126996
	PGN126998		
	PGN126992
	PGN127250
	PGN127506
	PGN127513
	PGN128259
	PGN128267
	PGN128275
	PGN129025
	PGN129026
	PGN129029
	PGN129033
	PGN129283
	PGN129284
	PGN129285
	PGN130306
	PGN130310
		

## Reverse Engineering Micronet
	The bulk of what I know of the Micronet protocol can be found in Micronet.h. Included in the project are logic analyzer captures of the hull transmitter's mcu and cc1000 radio chip (backwards_engineering_data), timing logs (backwards_engineering_data) and the code to test timing (old_code).
	
	These notes are chaotic. 
 	Some info maybe old or wrong.
  
 	Abandon all hope.
	(if you want to organize this go for it)
	
	Basic time specs.
	  13.02 us per bit
	  104.17 us per byte
	  1771 us for 17 bytes (standard preamble)
	  
	Max packet size: 200
	Max nodes: 32

	NW = Network Id
	DI = Device Id
	UK = Unknown, believed to be a reciever mask or sender status bits.
	NQ = Network quality (RSSI) for responce packets. 0-9 scale, 0 being didn't recieve the packet, 9 being strongest RSSI.
	CS = Checksum. Previous bytes added togeather from the last checksum limited to one byte.
	LN = Total packet length minus the LN fields.
	
	0x01 Network synchronization packet. 
		-First ring.
		NW NW NW NW DI DI DI DI *01* UK NQ CS LN CS *81 00 01 00 00 00 00 12*

	0x02 Data packet.
		-Second ring.
		NW NW NW NW DI DI DI DI *02* UK NQ CS LN CS *04 04 05 13 89 A9 04 1B 05 00 78 9C*

	0x03 Add transmit slot packet.
		-Third ring.
		-Can and does collide with other third ring packets.
		NW NW NW NW DI DI DI DI *03* UK NQ CS LN CS *00 1A 1A*

	0x04 Remove transmit slot packet.
		-Third ring.
		NW NW NW NW DI DI DI DI *04* UK NQ CS LN CS 
	
	0x05 Adjust transmit slot packet.
		-Third ring.
		NW NW NW NW DI DI DI DI *05* UK NQ CS LN CS *1A 1A*

	0x06 Command packet.
		-Third ring.
		-Every node should reply with an 0x07. If not the node keeps sending the 0x06 for 5 times.
		-Only one command packet can be handled a round.
		NW NW NW NW DI DI DI DI *06* UK NQ CS LN CS *E5 00 10 F5*

	0x07 Announcement/Ack.
		-Forth ring.
		-Forth ring runs backwards.
		NW NW NW NW DI DI DI DI *07* UK NQ CS LN CS 

	0x08 Node leaving network.
		-Third ring? (verify me pls)
		NW NW NW NW DI DI DI DI *08* UK NQ CS LN CS 

	0x09 Unknown, unseen, no reply.
		NW NW NW NW DI DI DI DI *09* UK NQ CS LN CS 

	0x0A Network status ping.
		-Third ring, network controller only
		-Used to cull dead nodes from sync list.
		-Also checks to see if there is a device that wants to take over as scheduler.
		NW NW NW NW DI DI DI DI *0A* UK NQ 7E LN CS 

	0x0B Netowrk status pong.
		-Forth ring.
		NW NW NW NW DI DI DI DI *0B* UK NQ 01 LN CS *00 00*
	
	0x18 Passing scheduling duty to another device. The device's id reversed is the data.
		NW NW NW NW DI DI DI DI *18* UK NQ CS LN CS *7C 62 03 81 62*

	0x19 Acknolegement of passing scheduling duty by device taking over.
		NW NW NW NW DI DI DI DI *19* UK NQ CS LN CS 

	0x01 Network synchronization packet.
		-First device in the list is always the scheduler and the it's window size is 3 + (5 * iMNNodes).
		-The network sync packet has a hard limit of ~32ms length. 
	
	0x02 Data packets.
		-Types:
		-0x01 - 06 byte Speed                0x04, 0x01, 0x05, 0x00, 0x64, 0x6E
		-0x02 - 12 byte Trip/Log             0x0A, 0x02, 0x05, 0x00, 0x00, 0x00, 0x64, 0x00, 0x00, 0x00, 0x0A, 0x7F
		-0x03 - 05 byte Temp                 0x03, 0x03, 0x05, 0x02, 0x0D
		-0x04 - 06 byte Depth                0x04, 0x04, 0x05, 0x00, 0x0A, 0x17
		-0x05 - 06 byte App Wind Speed       0x04, 0x05, 0x05, 0x00, 0x0A, 0x18
		-0x06 - 06 byte App Wind Dir         0x04, 0x06, 0x05, 0x00, 0x01, 0x10
		-0x07 - 06 byte HDG                  0x04, 0x07, 0x05, 0x00, 0x01, 0x11
		-0x08 - 08 byte SOG/COG              0x06, 0x08, 0x05, 0x00, 0x0A, 0x00, 0x01, 0x1E
		-0x09 - 11 byte LAT/LONG             0x09, 0x09, 0x05, 0x01, 0x03, 0xE8, 0x01, 0x03, 0xE8, 0x00, 0xEF
		-0x0A - 12 byte BTW                  0x0A, 0x0A, 0x05, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x21
		-0x0B - 06 byte XTE                  0x04, 0x0B, 0x05, 0x00, 0x64, 0x78
		-0x0C - 06 byte Time                 0x04, 0x0C, 0x05, 0x01, 0x00, 0x16
		-0x0D - 07 byte Date                 0x05, 0x0D, 0x05, 0x01, 0x01, 0x01, 0x1A
		- ...
		-0x10 - 08 byte Node Info            0x06, 0x10  0x01, 0x02, 0x20, 0x23, 0x08, 0x64
		- ...
		-0x12 - 06 byte VMG-WP               0x04, 0x12, 0x05, 0x00, 0x64, 0x7F 
		- ...
		-0x1B - 06 byte Battery              0x04, 0x1B, 0x05, 0x00, 0x0A, 0x2E
		- ...
		-0x1F - 08 byte DTW                  0x06, 0x1F, 0x05, 0x00, 0x00, 0x00, 0x01, 0x2B
		- ...
		-0xFD - 08 byte timer stop/start/set	0x06, 0xFD, 0x00, 0x01, 0x00, 0x00, 0x00, 0x04

		-TWDIR needs App Wind Speed, App Wind Dir, HDG
		-BEAUF needs App Wind Speed, App Wind Dir, HDG
		-MAG TWDIR needs App Wind Speed, App Wind Dir, HDG
		-HEAD/LIFT needs App Wind Speed, App Wind Dir, HDG (this one takes a little bit to fill in as its a running average)
		-VMG needs App Wind Speed, App Wind Dir, Speed
		-TRUE WIND SPEED needs App Wind Speed, App Wind Dir, Speed
		-TRUE WIND DIR needs App Wind Speed, App Wind Dir, Speed
		-ETA/TTG needs VMG-WP and DTW.
	
	0x03 Add transmit slot packet.
		-Data is an unknown 0x00 byte followed by the window size you want then a checksum. Repeating this packet with a different window size does not change your window size.
		-First byte might be an 0x0A byte to signal wanting sync duty. Need to check.

	0x04 Remove transmit slot packet.
		-Send this will remove your node's id from the sync list.

	0x05 Adjust transmit slot packet.
		-Data is one byte which is the new window size and a checksum.

	0x06 Command packets.
		0xE5 0x00 Display backlight command.
			Backlight Off: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 00 10 F5 
			Backlight  L1: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 01 0D F3 
			Backlight  L2: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 02 0E F5 
			Backlight  L3: 01 0A 8B 60 81 03 61 7C 06 09 00 66 10 10 E5 03 0F F7 

		0xFF Settings command.
			FF 01 01 00 24 25
			CW RG SS SS CT CS
			CW = Command word.
			RG = Registry.
			SS = Settings.
			CT = Counter. Every time a setting is changed this increments. Probably for synchronization. Doesn't seem to matter as I've sent out 00 multiple times and the display took the setting.
			CS = Checksum.
		
		0xFF Reg 0x00 Speed adj. +/- 200%
			speed +1%:   01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 00 01 33 6D A0 
			speed +2%:   01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 00 01 34 6E A2 
			speed +200%: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 00 01 FA 6F 69 

		0xFF Reg 0x01 General Settings.
			Default settings (all zeros) are: Depth feet, Speed knots, Wind knots, Log units nm. Is bit based. Not fully mapped.
			Default:       01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 00 24 25 0000 0001 0000 00001 0000 0000
			Speed   mph:   01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 01 30 32 0000 0001 0000 00001 0000 0001
			Speed  kmph:   01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 02 2F 32 0000 0001 0000 00001 0000 0010
			Log units sm:  01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 04 26 2B 0000 0001 0000 00001 0000 0100
			Log units  m:  01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 08 25 2E 0000 0001 0000 00001 0000 1000 
			Depth  meters: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 10 13 24 0000 0001 0000 00001 0001 0000
			Depth fathoms: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 20 15 36 0000 0001 0000 00001 0010 0000
			Wind meters:   01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 01 01 40 1F 60 0000 0001 0000 00001 0100 0000
	
		0xFF Reg 0x02 Sea temp adj. +/- 10c
			temp .5c: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 02 01 01 6B 70
			temp +1c: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 02 01 02 6C 70 
			temp +10c: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 02 01 14 6E 84 
	
		0xFF Reg 0x0B More General Settings/Some alarm settings. Bit based. Not fully mapped.
			Temp f:        01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 01 2D 39 0000 1011 0000 00001 0000 0001 (default is c)
			WP Arv Alarm:  01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 10 5B 76 0000 1011 0000 00001 0001 0000
			Alarm XTE:     01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 20 67 92 0000 1011 0000 00001 0010 0000
			Spd frmt 0.01: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 0B 01 40 38 83 0000 1011 0000 00001 0100 0000 (default is 0.1)

		0xFF Reg 0x11 Depth alarm deep.
			depth alarm deep 1ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 11 01 01 57 69 
			depth alarm deep 2ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 11 01 02 5A 6D 

		0xFF Reg 0x12 Depth alarm shallow.
			Depth alarm shallow .5ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 12 01 05 54 6B 
			Depth alarm shallow  1ft: 01 0A 8B 60 81 03 61 7C 06 09 00 66 12 12 FF 12 01 0A 59 75 

		0xFF Reg 0x13 CRSE Alarm.
			CRSE Alarm 1 degree : 01 0A 8B 60 81 03 61 7C 06 09 00 66 13 13 FF 13 02 01 00 5D 72
			CRSE Alarm 2 degrees: 01 0A 8B 60 81 03 61 7C 06 09 00 66 13 13 FF 13 02 02 00 5E 74 

	0x07 Announcement/Ack
		All commands should get a 07 reply from all nodes

	0x08 Node leaving network
		During pairing when a node is leaving a network for a new one it sends 0x08 packets to the old network.

	0x09 Unknown unseen no reply
	
	0x0A Network status ping.
		Part of device culling and master switching. The scheduler will send these out until it gets a responce from all devices in the list.

	0x0B Network status pong.
		Responce is the level of importance of a device. If a device with a higher number then the one running the network replies then sync duties gets passed on to it.
	
		00 - how a hull transmitter responses
		10 - how a slave display reponds
		90 - how a master display responds
	
	0x18 Passing scheduling duty to another device. 
		The device's id reversed is the data. Device needs to respond with a 0x19 packet.

	0x19 Acknolegement of passing scheduling duty by device taking over.
		The current scheduler will send out 4 more sync packets before the switchover happens.

	Notes:
		Pairing
		Status bit change, 5+ second preamble, 60 seconds join peroid with special status bit (0f wait 0d join). Devices joining the new network that are running an old network send out 0x08 packets. After 60 seconds nodes update thier NID to the new network.
		Device Id's are not static. If a device comes online to find it's id in use it'll increment its id and request a new window. 
	
	Scheduling:
		First slot always the node running the network. Not seen it used. Might just be a spacer.
	
	Micronet devices lock avgfilter after ~20 bits of preamble.
	Nodes remember the sync list and on start will use thier memorized list instead of the one we send...
	

## Good luck!  
