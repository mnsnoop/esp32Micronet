#!/usr/bin/perl -w

#nano -0.000000001
#micro-0.000001
#milli-0.001

use strict;
use warnings;

my $printProgramming = 1;
my $printDataRaw = 1;
my $printDataOrderedTX = 1;
my $printDataOrderedRX = 1;
my $printBits = 0;
my $printWidth = 1000000000;

my $FREF = 14745600;
#my $FREF = 11059200;
#my $FREF = 7372800;

$| = 1;

#dothing("digital1.csv", "dp1.txt");
dothing("digital2.csv", "dp2.txt");
#dothing("digital3.csv", "dp3.txt");
#dothing("possible alarm.csv", "possible alarm.txt");
#dothing("depth top volts-date-time-timer-seac-timer-time-date-volts.csv", "depth top volts-date-time-timer-seac-timer-time-date-volts.txt");
#dothing("full cycle 30salarm.csv", "full cycle 30salarm.txt");
#dothing("bat.csv", "bat.txt");
#dothing("wind-bat.csv", "wind-bat.txt");
#dothing("digitaltest.csv", "digitaltest.txt");
#dothing("test.csv", "test.txt");
#dothing2("ttt.csv", "ttt.txt");

#16.340076583333332,0,0b  0010  1010,

sub dothing2
{
	my $file = shift(@_);
	my $ofile = shift(@_);
	
	open(my $fh, '<', $file)
		or die "Could not open datafile $file. $!";
		
	open(my $fh2, '>', $ofile)
		or die "Could not open ofile. $!";
		
	print("Processing: $file\n");

	my $row = <$fh>;
	while ($row = <$fh>)
	{
		$row =~ s/\r|\n//g;
		
		my @field = split(/,/, $row);
		print($field[3] . "\n");
	}


	close($fh);
	close($fh2);
}
 
sub dothing
{
	my $file = shift(@_);
	my $ofile = shift(@_);
	
	open(my $fh, '<', $file)
		or die "Could not open datafile $file. $!";
		
	open(my $fh2, '>', $ofile)
		or die "Could not open ofile. $!";
		
	print("Processing: $file\n");
	
	my $row = <$fh>;

	my $pStatus = 0; #0=idle, 1=getting address, 2=getting data, 3=waiting reset
	my $pAddr = "";
	my $pData = "";
	my $pLastClk = 0;
	my $pLastData = 0;
	my $pLastAle = 0;
	my $pBitPos = 0;
	my $pAddRec = 0;
	my $pTime = "";

	my $mRegRXTX = "RX";
	my $mRegFreq = "A";
	my $mRegRXPD = 0;
	my $mRegTXPD = 0;
	my $mRegFSPD = 0;
	my $mRegCorePD = 0;
	my $mRegBiasPD = 0;
	my $mRegReset = 0;
	my $mRegFreqA = 0;
	my $mRegFreqB = 0;
	my $Freq = 0;
	my $mRegRefDiv = 0;
	my $PaPow = 0;
	my $FSep = 0;
	my $VCOCurrent = "unk";
	my $LODrive = "unk";
	my $PADrive = "unk";
	
	my $xmitStatus = ""; # RX, TX, or NIL
	my $lxmitStatus = "";  
	
	my $lFreq = 0;

	my $dLastClk = 0;
	my $dLastData = 0;
	my $dData = "";
	my $dTime = "";

	while ($row = <$fh>)
	{
		$row =~ s/\r|\n//g;
		
		my @field = split(/,/, $row);
		if ($field[0] > 14.23169512)
		{
			#exit(0);
		}
		
		my $time = $field[0];
		my $pale = $field[1];
		my $pdata = $field[2];
		my $pclk = $field[3];
		my $dclk = $field[4];
		my $ddata = $field[5];
		
#		print("$row $pStatus - $pBitPos\n");
		
		if ($pclk ne $pLastClk || $pdata ne $pLastData || $pale ne $pLastAle)
		{#change in any of the programming lines
			if ($pStatus == 0) #unknown
			{
				if ($pale == 1 && $pclk == 1 && $pdata == 1)
				{
					$pStatus = 1; #priming
				}
			}
			elsif ($pStatus == 1) #receiving address
			{
				if ($pclk == 0 && $pLastClk == 1) #clock change
				{
					if ($pale == 1)
					{
						print("PALE sync error address side! pStatus: $pStatus pBitPos:$pBitPos\n");
						print("$row\n");
						exit(0);
					}
					
					$pAddr .= $pdata;

					$pBitPos++;
					if ($pBitPos > 7)
					{
						$pBitPos = 0;
						$pStatus = 2;
					}
					
					if ($pTime eq "")
					{
						$pTime = $time;
					}
				}
			}
			elsif ($pStatus == 2) #receiving data
			{
				if ($pclk == 0 && $pLastClk == 1) #clock change / negitive edge
				{
					if ($pale == 0)
					{
						print("PALE sync error data side!\n");
						print("$row\n");
						exit(0);
					}
					
					$pData .= $pdata;

					$pBitPos++;
					if ($pBitPos > 7)
					{
						my $out = sprintf("%011.8f %s $pAddr:$pData %s%s%s", $pTime, substr($pAddr, 7, 1) == "1" ? "Prog Write" : "Prog Read ", ($mRegRXPD == 0) ? "RX " : "", ($mRegTXPD == 0) ? "TX " : "", ($mRegRXPD == 0 || $mRegTXPD == 0) ? "Freq: $Freq " : "");
					
						if ($printProgramming == 1)
						{
							print($fh2 $out);
							print($out);
							
							#print("cc1000SendProgramming(0b$pAddr, 0b$pData);\n");
							
							#$pBitPos = 0;
							#$pStatus = 3;
							#$pAddr = "";
							#$pData = "";
							#$pTime = "";
							#next;
						}
						
						my $oldxmit = $xmitStatus;
						
						if ($pAddr eq "00000001")
						{
							if (substr($pData, 0, 1) == '0')
							{$mRegRXTX = "RX";}
							else
							{$mRegRXTX = "TX";}
							
							if (substr($pData, 1, 1) == '0')
							{$mRegFreq = "A";}
							else
							{$mRegFreq = "B";}

							if (substr($pData, 2, 1) == '0')
							{$mRegRXPD = 0;}
							else
							{$mRegRXPD = 1;}		
							
							if (substr($pData, 3, 1) == '0')
							{$mRegTXPD = 0;}
							else
							{$mRegTXPD = 1;}

							if (substr($pData, 4, 1) == '0')
							{$mRegFSPD = 0;}
							else
							{$mRegFSPD = 1;}
							
							if (substr($pData, 5, 1) == '0')
							{$mRegCorePD = 0;}
							else
							{$mRegCorePD = 1;}
							
							if (substr($pData, 6, 1) == '0')
							{$mRegBiasPD = 0;}
							else
							{$mRegBiasPD = 1;}
							
							if (substr($pData, 7, 1) == '0')
							{$mRegReset = 1;}
							else
							{$mRegReset = 0;}
							
							if ($mRegRXTX eq "RX" && $mRegRXPD == 0)
							{
								$xmitStatus = "RX";
							}
							elsif ($mRegRXTX eq "TX" && $mRegTXPD == 0)
							{
								$xmitStatus = "TX";
							}
							else
							{
								$xmitStatus = "";
							}
							
							my $startstop = "";
							if ($oldxmit ne $xmitStatus)
							{
								if ($xmitStatus ne "")
								{
									$startstop = "!!START $xmitStatus!! ";
								}
								else
								{
									$startstop = "!!STOP $oldxmit!! ";
								}
							}
							
							my $out = sprintf("- Freq:$mRegFreq Power State:(RX_PD:$mRegRXPD TX_PD:$mRegTXPD FS_PD:$mRegFSPD Core_PD:$mRegCorePD Bias_PD:$mRegBiasPD) $startstop%s ", ($mRegReset == 1) ? "Main Reset Triggered" : "");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00000011")
						{
							#FREQ_2A Register (01h)
							$mRegFreqA = BitOffsetCopy($mRegFreqA, 16, $pData);
							
							my $out = sprintf("- FreqWordA2:$mRegFreqA ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00000101")
						{
							#FREQ_1A Register (02h)
							$mRegFreqA = BitOffsetCopy($mRegFreqA, 8, $pData);
							
							my $out = sprintf("- FreqWordA1:$mRegFreqA ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00000111")
						{
							#FREQ_0A Register (03h)
							$mRegFreqA = BitOffsetCopy($mRegFreqA, 0, $pData);
							
							my $out = sprintf("- FreqWordA0:$mRegFreqA ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00001001")
						{
							#FREQ_2B Register (04h)
							$mRegFreqB = BitOffsetCopy($mRegFreqB, 16, $pData);
							
							my $out = sprintf("- FreqWordB2:$mRegFreqB ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00001011")
						{
							#FREQ_1B Register (05h)
							$mRegFreqB = BitOffsetCopy($mRegFreqB, 8, $pData);
							
							my $out = sprintf("- FreqWordB1:$mRegFreqB ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00001101")
						{
							#FREQ_0B Register (06h)
							$mRegFreqB = BitOffsetCopy($mRegFreqB, 0, $pData);
							
							my $out = sprintf("- FreqWordB0:$mRegFreqB ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00001111")
						{
							#FSEP1 Register (07h)						
							$FSep = BitOffsetCopy($FSep, 8, substr($pData, 5, 3));
							
							my $fsep6 = ($FREF / 6) * ($FSep / 16384);
							my $fsep8 = ($FREF / 8) * ($FSep / 16384);
							
							my $out = sprintf("- FSep1 Word: $FSep Value: $fsep6/$fsep8 Hz (refdiv 6/8)");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00010001")
						{
							#FSEP0 Register (08h)
							$FSep = BitOffsetCopy($FSep, 0, $pData);
							
							my $fsep6 = ($FREF / 6) * ($FSep / 16384);
							my $fsep8 = ($FREF / 8) * ($FSep / 16384);
							
							my $out = sprintf("- FSep0 Word: $FSep Value: $fsep6/$fsep8 Hz (refdiv 6/8)");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00010011")
						{
							#CURRENT Register (09h)
							if (substr($pData, 0, 4) eq "1000")
							{
								$VCOCurrent = "1450µA";
							}
							elsif (substr($pData, 0, 4) eq "1111")
							{
								$VCOCurrent = "1450µA";
							}
							else
							{
								$VCOCurrent = "unk";
							}

							if (substr($pData, 4, 2) eq "00")
							{
								$LODrive = "0.5mA";
							}
							elsif (substr($pData, 4, 2) eq "11")
							{
								$LODrive = "2mA";
							}
							else
							{
								$LODrive = "unk";
							}							
							
							if (substr($pData, 6, 2) eq "00")
							{
								$PADrive = "1mA";
							}
							elsif (substr($pData, 6, 2) eq "11")
							{
								$PADrive = "4mA";
							}
							else
							{
								$PADrive = "unk";
							}
							
							my $out = sprintf("- Current Register VCOCurrent: $VCOCurrent LODrive:$LODrive PADrive:$PADrive ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00010101")
						{
							#FRONT_END Register (0Ah)
							
							my $out = sprintf("- Front_end Register ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00010111")
						{
							#PA_POW Register (0Bh)
							#$PaPow = oct("0b$pData");
							$PaPow = $pData;
							
							my $out = sprintf("- PAPow $PaPow ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00011001")
						{
							#PLL Register (0Ch)
							$mRegRefDiv = oct("0b" . substr($pData, 1, 4));
							
							my $out = sprintf("- Referance Divider: $mRegRefDiv ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00011011")
						{
							#LOCK Register (0Dh)
							
							my $out = sprintf("- Lock Register ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00011101")
						{
							#CAL Register (0Eh)
							
							my $out = sprintf("- Calibration Register Cal_Start: %s ", substr($pData, 0, 1));
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00011111")
						{
							#MODEM2 Register (0Fh)
							
							my $out = sprintf("- Modem2 Register PEAK_LEVEL_OFFSET:%d(%s) ", oct("0b" . substr($pData, 1, 7)), substr($pData, 1, 7));
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00100001")
						{
							#MODEM1 Register (10h)
							
							my $out = sprintf("- Modem1 Register LOCK_AVG_IN:%s LOCK_AVG_MODE:%s SETTLING %s MODEM_RESET_N %s ", substr($pData, 3, 1), substr($pData, 4, 1), substr($pData, 5, 2), substr($pData, 7, 1));
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00100011")
						{
							#MODEM0 Register (11h)
							
							my $out = sprintf("- Modem0 Register BAUDRATE:%s DATA_FORMAT:%s XOSC_FREQ %s ", substr($pData, 1, 3), substr($pData, 4, 2), substr($pData, 6, 2));
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00100101")
						{
							#MATCH Register (12h)
							
							my $out = sprintf("- Match Register ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00100111")
						{
							#FSCTRL Register (13h)
							
							my $out = sprintf("- FSCTRL Register ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "00111001")
						{
							#PRESCALER Register (1Ch)
							
							my $out = sprintf("- Prescaler Register ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						elsif ($pAddr eq "10000101")
						{
							#TEST4 Register (for test only, 42h)
							
							my $out = sprintf("- TEST4 Register ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						else
						{
							my $out = sprintf("- Unhandled %s", substr($pAddr, 7, 1) == "1" ? "" : "(Program read) ");
						
							if ($printProgramming == 1)
							{
								print($fh2 $out);
								print($out);
							}
						}
						
						if ($mRegRefDiv ne 0)
						{
							$Freq = ($FREF / $mRegRefDiv) * (((($mRegFreq eq 'A') ? $mRegFreqA : $mRegFreqB) + 8192) / 16384);
							$Freq /= 1000000;
						}
						else
						{
							$Freq = 0;
						}
						
						$out = sprintf("\n");
						
						if ($printProgramming == 1)
						{
							print($fh2 $out);
							print($out);
						}
						
						#HEREHERHEHERE
						#$out = sprintf("ProgramWrite(0b$pAddr, 0b$pData, false);\n");
						#print($fh2 $out);
						
						$pBitPos = 0;
						$pStatus = 3;
						$pAddr = "";
						$pData = "";
						$pTime = "";
					}
				}
			}
			elsif ($pStatus == 3) #waiting for a reset
			{
				if ($pclk == 0 && $pdata == 0)
				{
					$pStatus = 0; #reset
				}
			}
			
			$pLastClk = $pclk;
			$pLastData = $pdata;
			$pLastAle = $pale;
		}

		if ($xmitStatus ne "")
		{#we're xmitting data
			if ($lxmitStatus eq "")
			{#first cycle setup
				$lxmitStatus = $xmitStatus;
				$dLastClk = 0;
				$dData = "";
				$dTime = $time;
				$lFreq = $Freq;
			}
		
			if ($dclk eq "1" && $dLastClk eq "0")
			{#data is clocked in on the rising edge.
				$dData .= "$ddata";					
			}
			
			$dLastClk = $dclk;
		}
		elsif ($lxmitStatus ne "")
		{#we were xmitting but stopped
		
			if ($lxmitStatus eq "RX")
			{#inverse data
				for (my $i = 0; $i < length($dData); $i++)
				{
					if (substr($dData, $i, 1) eq "0")
					{
						substr($dData, $i, 1, "1")
					}
					else
					{
						substr($dData, $i, 1, "0")
					}
				}
			}
		
		
		
			my $pData = $dData;
			
			if (length($pData) > 0)
			{#print out raw data
				my $dHex = "";
				my $pLimit = 0;
				my $out = sprintf("%011.8f - %011.8f %s %d bytes(%d bits) Freq: $lFreq RAW.\n", $dTime, $time, ($lxmitStatus eq "RX") ? "Received" : "Transmitted", length($pData) / 8, length($pData));
				if ($printDataRaw == 1)
				{
					print($fh2 $out);
					print($out);
				}
				
				while (length($pData) > 8)
				{
					my $byte = substr($pData, 0, 8, "");
					
					$dHex .= sprintf("%.2X ", oct("0b" . $byte));
					$out = "$byte ";
					if ($printDataRaw == 1 && $printBits == 1)
					{
						print($fh2 $out);
						print($out);
					}
					
					$pLimit += 9;
					
					if ($pLimit > $printWidth)
					{
						$out = " | $dHex\n";
						if ($printDataRaw == 1)
						{
							print($fh2 $out);
							print($out);
						}
						
						$dHex = "";
						$pLimit = 0;
					}
				}
				
				if (length($dHex) > 0)
				{
					if ($printBits == 1)
					{
						$out = sprintf("$pData %*s | $dHex\n", $printWidth - $pLimit - length($pData), "");
					}
					else
					{
						$out = sprintf("$dHex\n");
					}
					
					if ($printDataRaw == 1)
					{
						print($fh2 $out);
						print($out);
					}
					
					$pData = "";
					$pLimit = 0;
				}
			}
			
			my $oData = "";
			
			for (my $i = 0; $i < length($dData) - 16; $i++)
			{#search for 00110011
				if (substr($dData, $i, 24) eq "010101010101010110011001") #"0101010100110011"
				{#found it!
					$oData = substr($dData, $i + 24, length($dData) - $i - 24);
					$i = length($dData);
				}
			}
			
			if (length($oData) > 0)
			{#print out ordered data
				my $dHex = "";
				my $pLimit = 0;
				my $out = sprintf("%011.8f - %011.8f %s packet %d bytes(0x%x) Freq $lFreq.\n", $dTime, $time, ($lxmitStatus eq "RX") ? "Received" : "Transmitted", length($oData) / 8, length($oData) / 8);
				if (($printDataOrderedTX == 1 && $lxmitStatus eq "TX") ||
					($printDataOrderedRX == 1 && $lxmitStatus eq "RX"))
				{
					print($fh2 $out);
					print($out);
				}
				
				while (length($oData) > 8)
				{
					my $byte = substr($oData, 0, 8, "");
					
					$dHex .= sprintf("%.2X ", oct("0b" . $byte));
					$out = "$byte ";
					if ((($printDataOrderedTX == 1 && $lxmitStatus eq "TX") ||
						($printDataOrderedRX == 1 && $lxmitStatus eq "RX")) &&
						$printBits == 1)
					{
						print($fh2 $out);
						print($out);
					}
					
					$pLimit += 9;
					
					if ($pLimit > $printWidth)
					{
						$out = " | $dHex\n";
						if (($printDataOrderedTX == 1 && $lxmitStatus eq "TX") ||
						($printDataOrderedRX == 1 && $lxmitStatus eq "RX"))
						{
							print($fh2 $out);
							print($out);
						}
						
						$dHex = "";
						$pLimit = 0;
					}
					
					#print("$oData\n");
				}
				
				if (length($dHex) > 0)
				{
					if ($printBits == 1)
					{
						$out = sprintf("$oData %*s | $dHex\n", $printWidth - $pLimit - length($oData), "");
					}
					else
					{
						$out = sprintf("$dHex\n");
					}

					if (($printDataOrderedTX == 1 && $lxmitStatus eq "TX") ||
						($printDataOrderedRX == 1 && $lxmitStatus eq "RX"))
					{
						print($fh2 $out);
						print($out);
					}
					
					$oData = "";
					$pLimit = 0;
				}
				
				if (($printDataOrderedTX == 1 && $lxmitStatus eq "TX") ||
					($printDataOrderedRX == 1 && $lxmitStatus eq "RX"))
				{
#					$out = "\n";
#					print($fh2 $out);
#					print($out);
				}
			}
			
			$dData = "";
			$lxmitStatus = "";
		}
	}

	close($fh);
	close($fh2);
}

sub BitOffsetCopy
{
	my $ret = $_[0]; #number we're working on
	my $offset = $_[1]; #offset we start at
	my $value = $_[2]; #value to transfer as a string of 0's and 1's
	
	my $len = length($value);
	
#	print(substr(unpack("B32", pack("N", $ret)), 8) . "\n");
#	for (my $i = 0; $i < 16 - $offset; $i++)
#	{
#		print("-");
#	}
#	print("$value\n");
	
	for (my $i = 0; $i < $len; $i++)
	{
		if (substr($value, $i, 1) == "0")
		{
			$ret &= ~(1 << (($len - 1) - $i + $offset));
		}
		else
		{
			$ret |= (1 << (($len - 1) - $i + $offset));
		}
	}

#	print(substr(unpack("B32", pack("N", $ret)), 8) . "\n\n");
	
	return $ret;
}
