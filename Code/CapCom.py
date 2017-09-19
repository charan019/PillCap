#!/usr/bin/python
# pylint: disable-msg=C0103
# pylint: disable-msg=C0301

SWVer = '0.30'

import wx
import wx.calendar
import platform
import os.path
import time
import datetime
from time import sleep
import serial
from Modules import ComSubs
from Modules import Reports

MAX_STRING_LENGTH = 20
CAP_BAUD_RATE = 600
EEPROM_WRITE_TIME = .0034*20
DEFAULT_SERIAL_TIMEOUT = 0.3

GlobalSerialPort = ''
GlobalFacilityText = ''
Commands = {'Facility':'0', 'Doctor':'1', 'Treatment':'2', 'Client':'3', 'Patient':'4', 'DosePattern':'5', 'DoseCount':'6'}
DosePatterns = {'24':'1/day', '12':'2/day', '8':'3/day', '6':'4/day', '168':'1/week', '720':'1/month'}
DosePatternsRev = {'1/day':'24', '2/day':'12', '3/day':'8', '4/day':'6', '1/week':'168', '1/month':'720'}
CapStatus = {'0':'Done', '1':'Running', '2':'Programmed','4':'Unprogrammed'}

def CalcFormLength():
	if platform.system() == "Darwin":
		FormLength = 610
	elif platform.system() == "Windows":
		FormLength = 605
	else:
		FormLength = 600
	return FormLength	
	
def CalcFormHeight():
	if platform.system() == "Darwin":
		FormHeight = 444
	elif platform.system() == "Windows":
		FormHeight = 450
	else:
		FormHeight = 424
	return FormHeight
	
def CapDataRead(SPort):
	ReturnStringList = []
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
	COMPort.write('T\r')
	COMPort.readline()
	COMPort.write('D\r')
	ResponseList = ComSubs.RxdStrParse(COMPort.readline())
	for i in range(0, int(ResponseList[1])):
		ReturnStringList.append(COMPort.readline())
	COMPort.close()
	return ReturnStringList
	
def CapTimeRead(SPort, TimeSelect):
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
	COMPort.write('T\r')
	COMPort.readline()
	COMPort.write(TimeSelect)
	try:
		StartDT = datetime.datetime.strptime(COMPort.readline()[4:-2], "%S:%M:%H:%d:0%w:%m:%y")
		ReturnString = StartDT.strftime("%Y-%m-%d %H:%M:%S")
	finally:
		pass
	COMPort.close()
	return ReturnString

def CapBatteryAge(SPort):
	'''Return the battery age in days'''
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
	COMPort.write('T\r')
	COMPort.readline()
	COMPort.write('M\r')
	ResponseString = COMPort.readline()
	RetVal = 0
	try:
		#print ResponseString
		BatteryInstallDate = datetime.datetime.strptime(ResponseString[4:-2], "%S:%M:%H:%d:0%w:%m:%y")
		Now = datetime.datetime.now()
		TimeDiff = Now - BatteryInstallDate
		RetVal = TimeDiff.days +TimeDiff.seconds/86400.0
	except ValueError:
		now = datetime.datetime.now()
		CapWriteString(SPort, 'B:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}\r'.format(now.second, now.minute, now.hour, now.day, now.weekday(), now.month, now.year-2000))
		CapWriteString(SPort, 'N\r')
	finally:
		pass
	COMPort.close()
	return RetVal
	
def CapReadAll(SPort):
	ReturnDict = {}
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
	COMPort.write('T\r')
	COMPort.readline()
	for Description, Command in Commands.iteritems():
		COMPort.write('G')
		COMPort.write(Command)
		COMPort.write('\r')
		ResponseList = ComSubs.RxdStrParse(COMPort.readline())
		#print ResponseList
		ReturnDict[Description] = ResponseList[2]
	COMPort.close()		
	return ReturnDict

def CapWriteString(SPort,OutputString):
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
	OutputChars = list(OutputString)
	for Char in OutputChars:
		COMPort.write(str(Char))
		sleep(0.02)
		#print Char,
	#print
	COMPort.close()
	
def CapPing(SPort):
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
	COMPort.write('T\r')
	# ResponseString = COMPort.readline()
	ResponseList = ComSubs.RxdStrParse(COMPort.readline())
	COMPort.close()
	return ResponseList
	
def CapWriteAll(SPort, WriteDict):
	CapPing(SPort)
	for Description, Command in Commands.iteritems():
		CapWriteString(SPort, 'S' + Commands[Description] + WriteDict[Description] + '\r')
		sleep(EEPROM_WRITE_TIME)
	# # set clock
	now = datetime.datetime.now()
	CapWriteString(SPort, 'B:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}\r'.format(now.second, now.minute, now.hour, now.day, now.weekday(), now.month, now.year-2000))

def CapErase(SPort):
	COMPort = serial.Serial(SPort, CAP_BAUD_RATE, timeout=3)
	COMPort.write('ER\r')
	EraseReturnString = ''
	EraseReturnString =  COMPort.readline()	
	COMPort.close()
	RetVal = 0
	if EraseReturnString is not '':
		RetVal = 1
	return RetVal

def FindBaseStation():
	# scan for open com ports
	#	if no port return Fail
	# starting with port from INI file if present and open
	# 		send 'z\r\n' to port
	#		wait for response
	#		if not found, next port
	#		if found then return OK
	#		if tried all ports return Fail
	# Error messages begin with 'No', for checking later
	ReturnList = []
	ResponseString = ''
	PortList = ComSubs.PortScan()
	if not len(PortList):
		PortName = 'No Com Ports Found'
	else:    # search through open ports, will return last one that responds
		PortName = 'No Base Stations Found.\nCheck cables.\nBe sure FTDI230X driver is installed.\nRemove from bright lights.\nInserting a cap may help.'
		for Port in PortList:
			try:
				COMPort = serial.Serial(Port, CAP_BAUD_RATE, timeout=DEFAULT_SERIAL_TIMEOUT)
				COMPort.write('T')
				sleep(0.05)
				COMPort.write('\r')
				COMPort.readline()
				COMPort.write('Z')
				sleep(0.05)
				COMPort.write('\r')
				ResponseString = COMPort.readline()
				COMPort.close()
				if ResponseString.find('Base') > 0:
					PortName = Port
			finally:
				pass
	ReturnList.append(PortName)
	ReturnList.append(ResponseString)
	return ReturnList

class WriteWindow(wx.Frame):
	CapErased = False
	def __init__(self, parent):
		wx.Frame.__init__(self, parent, title='Write Cap', size=(CalcFormLength(), CalcFormHeight()),
			style = wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)

		wx.Frame.CenterOnScreen(self)
		panel = wx.Panel(self, -1)
		BackGroundColour = (233, 228, 214)
		panel.SetBackgroundColour(BackGroundColour)
		font16 = wx.Font(16, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font12 = wx.Font(12, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font10 = wx.Font(10, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
				
		self.StatusBar = wx.StatusBar(self, -1)
		self.StatusBar.SetFieldsCount(4)

		self.WriteFacilityText = wx.StaticText(panel, -1, u'Facility', pos = (15, 25), size = (120, 30))
		self.WriteFacilityText.SetFont(font16)

		self.WriteFacilityTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 25), size = (175, -1))
		self.WriteFacilityTextCtrl.SetFont(font10)
		self.WriteFacilityTextCtrl.SetMaxLength(MAX_STRING_LENGTH)
		
		self.WriteDrText = wx.StaticText(panel, -1, u'Doctor', pos = (15, 50), size = (120, 30))
		self.WriteDrText.SetFont(font16)

		self.WriteDoctorTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 50), size = (175, -1))
		self.WriteDoctorTextCtrl.SetFont(font10)
		self.WriteDoctorTextCtrl.SetMaxLength(MAX_STRING_LENGTH)

		# self.WriteDrChoice = wx.ComboBox(panel, -1, 'Dr. Cook', pos = (150, 50), size = (175, 25),
					# choices = [u'Dr. Bang', u'Dr Camuti', u'C. Bourgelat', u'T. Burgess', u'Dr. Ashford'],	style = wx.CB_DROPDOWN)
		# self.WriteDrChoice.SetFont(font12)

		self.WriteTreatmentText = wx.StaticText(panel, -1, u'Treatment', pos = (15, 75),size = (120, 30))
		self.WriteTreatmentText.SetFont(font16)

		self.WriteTreatmentTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 75), size = (175, -1))
		self.WriteTreatmentTextCtrl.SetFont(font10)
		self.WriteTreatmentTextCtrl.SetMaxLength(MAX_STRING_LENGTH)

		# self.WriteMedChoice = wx.ComboBox(panel, -1, '', pos = (150, 75), size = (175, 25),
			# choices = [u'', u'Lollypop', u'Pat on the Head', u'Thorzine'],	style = wx.CB_DROPDOWN)
		#self.WriteMedChoice.SetFont(font12)

		self.WriteDoseText = wx.StaticText(panel, -1, u'Dose Freq.', pos = (15, 100), size = (120, 30))
		self.WriteDoseText.SetFont(font16)

		self.WriteDoseChoice = wx.ComboBox(panel, -1, '1/day', pos = (150, 100), size = (175, 25),
					choices = [u'1/day', u'2/day', u'3/day', u'4/day', u'1/week', u'1/month'],
					style = wx.CB_DROPDOWN | wx.CB_READONLY)
		self.WriteDoseChoice.SetFont(font12)
		self.Bind(wx.EVT_COMBOBOX, self.on_DoseChange, self.WriteDoseChoice)
		
		self.WriteDoseCountText = wx.StaticText(panel, -1, u'Dose Count', pos = (15, 125), size = (120, 30))
		self.WriteDoseCountText.SetFont(font16)

		self.WriteDoseCount = wx.SpinCtrl(panel, -1, pos = (150, 125), size = (175, 25), initial = 30, min=1, max = 120)
		self.WriteDoseCount.SetFont(font12)        
		self.Bind(wx.EVT_SPINCTRL, self.on_DoseChange, self.WriteDoseCount)

		self.WriteTreatmentLengthText = wx.StaticText(panel, -1, u'Treatment Length', pos = (350, 110), size = (120, 30))
		self.WriteTreatmentLengthOutText = wx.StaticText(panel, -1, u'30 Days', pos = (350, 125), size = (120, 30))
		
		self.WriteClientText = wx.StaticText(panel, -1, u'Client', pos = (15, 150), size = (120, 30))
		self.WriteClientText.SetFont(font16)

		self.WriteClientTextCtrl = wx.TextCtrl(panel, -1, u'', pos = (150, 150), size = (175, -1))
		self.WriteClientTextCtrl.SetFont(font10)
		self.WriteClientTextCtrl.SetMaxLength(MAX_STRING_LENGTH)
		
		self.WritePatientText = wx.StaticText(panel, -1, u'Patient', pos = (15, 175), size = (120, 30))
		self.WritePatientText.SetFont(font16)

		self.WritePatientTextCtrl = wx.TextCtrl(panel, -1, u'', pos = (150, 175), size = (175, -1))
		self.WritePatientTextCtrl.SetFont(font10)
		self.WritePatientTextCtrl.SetMaxLength(MAX_STRING_LENGTH)

		self.WriteClientText = wx.StaticText(panel, -1, u'Start Date', pos = (15, 200), size = (120, 30))
		self.WriteClientText.SetFont(font16)

		self.Calendar = wx.DatePickerCtrl(panel, -1, pos=(150, 200))
		self.Calendar.SetRange(wx.DateTime_Now()-wx.DateSpan(days=1) ,wx.DateTime_Now()+wx.DateSpan(days=90) )
		
		self.WriteStartTimeText = wx.StaticText(panel, -1, u'Start Time', pos = (15, 225), size = (120, 30))
		self.WriteStartTimeText.SetFont(font16)
		self.WriteColonText = wx.StaticText(panel, -1, u':', pos = (200, 225), size = (120, 30))
		self.WriteColonText.SetFont(font16)
		
		now = datetime.datetime.now()
		if now.hour == 23:
			InitHour = 0;
		else:
			InitHour = now.hour+1
		self.WriteStartHours = wx.SpinCtrl(panel, -1, pos = (150, 225), size = (50, 25), initial = InitHour, min=0, max = 23)
		self.WriteStartHours.SetFont(font12)        
		self.WriteStartMins = wx.SpinCtrl(panel, -1, pos = (210, 225), size = (50, 25), initial = 0, min=0, max = 59)
		self.WriteStartMins.SetFont(font12)        
		
		self.WriteWriteButton = wx.Button(panel, -1, label = 'Write', pos = (150, 350), size = (150, 50))
		self.WriteWriteButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_WriteWriteButton, self.WriteWriteButton)
		
		self.WriteEraseButton = wx.Button(panel, -1, label = 'Erase Cap', pos = (300, 350), size = (150, 50))
		self.WriteEraseButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_WriteEraseButton, self.WriteEraseButton)

		self.WriteBackButton = wx.Button(panel, -1, label = 'Back', pos = (450, 350), size = (150, 50))
		self.WriteBackButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_WriteBackButton, self.WriteBackButton)

	def on_DoseChange(self, event):
		self.WriteTreatmentLengthOutText.SetLabel('{:0.0f} Days'.format(self.WriteDoseCount.GetValue()*int(DosePatternsRev[self.WriteDoseChoice.GetValue()])/24))
		
	def on_WriteWriteButton(self, event):
		global GlobalSerialPort
		WriteDict = {}
		dlg = wx.MessageDialog(None, 'Are you sure you want to write cap?',
			'Question', wx.YES_NO | wx.ICON_QUESTION|wx.YES_DEFAULT)
		DialogReturn = dlg.ShowModal()
		if DialogReturn == wx.ID_YES:
			if CapErase(GlobalSerialPort):
				WriteDict['Facility'] = self.WriteFacilityTextCtrl.GetValue()
				WriteDict['Doctor'] = self.WriteDoctorTextCtrl.GetValue()
				WriteDict['Treatment'] = self.WriteTreatmentTextCtrl.GetValue()
				WriteDict['DosePattern'] = DosePatternsRev[self.WriteDoseChoice.GetValue()]
				WriteDict['DoseCount'] = str(self.WriteDoseCount.GetValue())
				WriteDict['Client'] = self.WriteClientTextCtrl.GetValue()
				WriteDict['Patient'] = self.WritePatientTextCtrl.GetValue()
				BatteryAge = CapBatteryAge(GlobalSerialPort)
				BatteryLife = 365-BatteryAge
				TreatmentTime = self.WriteDoseCount.GetValue() * int(DosePatternsRev[self.WriteDoseChoice.GetValue()])/24
				if BatteryLife < TreatmentTime:
					MessageString = 'Insufficient Battery Life.\n Barrery Life:' + ('{0:.0f} Days'.format(BatteryLife)) + 'Days Remaining.\n Treatment Time:' + str(TreatmentTime) + 'Days.'
					wx.MessageBox(MessageString, '')
				else:
					StartTime = self.Calendar.GetValue()
					CapWriteString(GlobalSerialPort, 'F:00:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}\r'.format(self.WriteStartMins.GetValue(), self.WriteStartHours.GetValue(), StartTime.GetDay(), StartTime.GetWeekDay(), StartTime.GetMonth()+1, StartTime.GetYear()-2000))
					CapWriteAll(GlobalSerialPort, WriteDict)
					CapWriteString(GlobalSerialPort, 'I\n')	# start
			else:
				dlg = wx.MessageDialog(None, 'Erase Error:\nCheck cap alignment.\nPress cap.\nCheck battery.', 'Error', wx.OK)
				DialogReturn = dlg.ShowModal()				
			
	def on_WriteEraseButton(self, event):
		global GlobalSerialPort
		dlg = wx.MessageDialog(None, 'Are you sure you want to erase cap?',
			'Question', wx.YES_NO | wx.ICON_QUESTION|wx.YES_DEFAULT)
		DialogReturn = dlg.ShowModal()
		if DialogReturn == wx.ID_YES:
			if CapErase(GlobalSerialPort) == 0:
				dlg = wx.MessageDialog(None, 'Erase Error:\nCheck cap alignment.\nPress cap.\nCheck battery.', 'Error', wx.OK)
				DialogReturn = dlg.ShowModal()				
				
	def on_WriteBackButton(self,event):
		StartTime = self.Calendar.GetValue()
		self.Hide()
		frame.Show()

class ReadWindow(wx.Frame):
	TimePoints = []
	def __init__(self, parent):
		wx.Frame.__init__(self, parent, title='Read Cap', size=(CalcFormLength(), CalcFormHeight()),
			style = wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)

		wx.Frame.CenterOnScreen(self)
		panel = wx.Panel(self, -1)
		BackGroundColour = (233, 228, 214)
		panel.SetBackgroundColour(BackGroundColour)
		font16 = wx.Font(16, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font12 = wx.Font(12, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font10 = wx.Font(10, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)

		# Read Page Specific		
		self.StatusBar = wx.StatusBar(self, -1)
		self.StatusBar.SetFieldsCount(4)

		self.ReadFacilityText = wx.StaticText(panel, -1, u'Facility', pos = (15, 25), size = (120, 30))
		self.ReadFacilityText.SetFont(font16)

		self.ReadFacilityTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 25), size = (175, -1), style=wx.TE_READONLY)
		self.ReadFacilityTextCtrl.SetFont(font12)

		self.ReadDrText = wx.StaticText(panel, -1, u'Doctor', pos = (15, 50), size = (120, 30))
		self.ReadDrText.SetFont(font16)

		self.ReadDrTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 50), size = (175, -1), style=wx.TE_READONLY)
		self.ReadDrTextCtrl.SetFont(font12)

		self.ReadMedText = wx.StaticText(panel, -1, u'Treatment', pos = (15, 75),size = (120, 30))
		self.ReadMedText.SetFont(font16)

		self.ReadMedTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 75), size = (175, -1), style=wx.TE_READONLY)
		self.ReadMedTextCtrl.SetFont(font12)

		self.ReadDoseText = wx.StaticText(panel, -1, u'Dose Freq.', pos = (15, 100), size = (120, 30))
		self.ReadDoseText.SetFont(font16)

		self.ReadDoseTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 100), size = (175, -1), style=wx.TE_READONLY)
		self.ReadDoseTextCtrl.SetFont(font12)
		
		self.ReadDoseCountText = wx.StaticText(panel, -1, u'Dose Count', pos = (15, 125), size = (120, 30))
		self.ReadDoseCountText.SetFont(font16)

		self.ReadDoseCount = wx.TextCtrl(panel, -1, pos = (150, 125), size = (175, -1), style=wx.TE_READONLY)
		self.ReadDoseCount.SetFont(font12)        
		
		self.ReadClientText = wx.StaticText(panel, -1, u'Client', pos = (15, 150), size = (120, 30))
		self.ReadClientText.SetFont(font16)

		self.ReadClientTextCtrl = wx.TextCtrl(panel, -1, u'', pos = (150, 150), size = (175, -1), style=wx.TE_READONLY)
		self.ReadClientTextCtrl.SetFont(font10)
		
		self.ReadPatientText = wx.StaticText(panel, -1, u'Patient', pos = (15, 175), size = (120, 30))
		self.ReadPatientText.SetFont(font16)

		self.ReadPatientTextCtrl = wx.TextCtrl(panel, -1, u'', pos = (150, 175), size = (175, -1), style=wx.TE_READONLY)
		self.ReadPatientTextCtrl.SetFont(font10)
		
		self.ReadStartTimeText = wx.StaticText(panel, -1, u'Start Time', pos = (15, 200), size = (120, 30))
		self.ReadStartTimeText.SetFont(font16)

		self.ReadStartTimeTextCtrl = wx.TextCtrl(panel, -1, u'', pos = (150, 200), size = (175, -1), style=wx.TE_READONLY)
		self.ReadStartTimeTextCtrl.SetFont(font10)
		
		self.ReadPatientText = wx.StaticText(panel, -1, u'Time Points', pos = (400, 0), size = (120, 30))
		self.ReadPatientText.SetFont(font16)
		
		self.ReadPatientText = wx.StaticText(panel, -1, u'Mon	Day	Hr	Min', pos = (400, 25), size = (120, 30))
		self.ReadPatientText.SetFont(font12)
		
		self.TimePointsText = wx.TextCtrl(panel, -1, '', pos = (400, 50), size = (175, 225), style=wx.TE_READONLY | wx.TE_MULTILINE)
		self.TimePointsText.SetFont(font12)

		self.ReportFormatText = wx.StaticText(panel, -1, u'Report', pos = (15, 275), size = (120, 30))
		self.ReportFormatText.SetFont(font16)

		self.ReportDestinationText = wx.StaticText(panel, -1, u'Destination', pos = (15, 300), size = (120, 30))
		self.ReportDestinationText.SetFont(font16)

		self.ReportDestination = wx.TextCtrl(panel, -1, u'', pos = (150, 300), size = (450, 25), style = wx.TE_READONLY)
		self.ReportDestination.SetFont(font12)
		self.ReportDestination.Bind(wx.EVT_LEFT_DOWN, self.on_ReportDestination_mouseDown)

		self.ReportFormatText = wx.StaticText(panel, -1, u'Format', pos = (15, 325), size = (120, 30))
		self.ReportFormatText.SetFont(font16)

		self.ReportFormatChoice = wx.ComboBox(panel, -1, 'Text', pos = (150, 325), size = (100, 25), 
					choices = [u'PDF', u'Excel', u'Text'], style = wx.CB_DROPDOWN | wx.TE_READONLY)
		self.ReportFormatChoice.SetFont(font12)

		self.ReadReadButton = wx.Button(panel, -1, label = 'Read', pos = (150, 350), size = (150, 50))
		self.ReadReadButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_ReadReadButton, self.ReadReadButton)
		self.ReadReadButton.SetToolTip(wx.ToolTip("Press to retrieve data from Cap"))
		
		self.ReadSaveButton = wx.Button(panel, -1, label = 'Save', pos = (300, 350), size = (150, 50))
		self.ReadSaveButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_ReadSaveButton, self.ReadSaveButton)
		self.ReadSaveButton.Disable()
		
		self.ReadBackButton = wx.Button(panel, -1, label = 'Back', pos = (450, 350), size = (150, 50))
		self.ReadBackButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_ReadBackButton, self.ReadBackButton)

	def on_ReportDestination_mouseDown(self, event):
		""" get directory for report saving"""
		DirDialog = wx.DirDialog(None, message = 'Choose a directory to save Reports')
		if DirDialog.ShowModal() == wx.ID_OK:
			if os.access(DirDialog.GetPath(), os.W_OK):
				self.ReportDestination.SetValue(DirDialog.GetPath())
			else:
				wx.MessageBox('Cannot write to this directory. Reselect.', '')
		DirDialog.Destroy()
		
	def on_ReadReadButton(self,event):
		global GlobalSerialPort
		NotRead = True
		for i in range(0,2):
			if NotRead:
				try:
					InfoDict = CapReadAll(GlobalSerialPort)
					self.ReadFacilityTextCtrl.SetValue(InfoDict['Facility'].strip())
					self.ReadDrTextCtrl.SetValue(InfoDict['Doctor'].strip())
					self.ReadMedTextCtrl.SetValue(InfoDict['Treatment'].strip())
					self.ReadDoseTextCtrl.SetValue(DosePatterns.get(InfoDict['DosePattern'].strip(), 'Unrecognized'))
					self.ReadDoseCount.SetValue(InfoDict['DoseCount'].strip())
					self.ReadClientTextCtrl.SetValue(InfoDict['Client'].strip())
					self.ReadPatientTextCtrl.SetValue(InfoDict['Patient'].strip())
					self.TimePointsText.Clear()
					self.TimePoints = CapDataRead(GlobalSerialPort)
					for Points in self.TimePoints:
						DataSubList = ComSubs.RxdStrParse(Points)
						if int(DataSubList[0]) == 0 and int(DataSubList[1]) == 0 and int(DataSubList[2]) == 0 and int(DataSubList[3]) == 0:
							self.TimePointsText.AppendText('Missed Dose\r')
						else:
							self.TimePointsText.AppendText('{}	{}	{}	{}\r'.format(DataSubList[3], DataSubList[2], DataSubList[1], DataSubList[0]))
					self.ReadStartTimeTextCtrl.SetValue(CapTimeRead(GlobalSerialPort, 'H\r'))
					self.ReadSaveButton.Enable()
					NotRead = False # success don't try again
				except:
					if(i==1):
						dlg = wx.MessageDialog(None, 'Read Error:\nCheck cap alignment.\nPress cap.\nCheck battery.\nCap may be blank.', 'Error', wx.OK)
						DialogReturn = dlg.ShowModal()

	def on_ReadSaveButton(self,event):
		InfoDict = {}
		InfoDict['Facility'] = self.ReadFacilityTextCtrl.GetValue()
		InfoDict['Doctor'] =  self.ReadDrTextCtrl.GetValue()
		InfoDict['Treatment'] =  self.ReadMedTextCtrl.GetValue()
		InfoDict['DosePattern'] =  self.ReadDoseTextCtrl.GetValue()
		InfoDict['DoseCount'] =  self.ReadDoseCount.GetValue()
		InfoDict['Client'] =  self.ReadClientTextCtrl.GetValue()
		InfoDict['Patient'] =  self.ReadPatientTextCtrl.GetValue()
		Reports.GenerateReport(self.ReportDestination.GetValue(), self.ReportFormatChoice.GetValue(), InfoDict, self.TimePoints)
		
	def on_ReadBackButton(self,event):
		self.ReadSaveButton.Disable()
		self.Hide()
		frame.Show()
		
class SettingsWindow(wx.Frame):
	def __init__(self, parent):
		wx.Frame.__init__(self, parent, title='Settings', size=(CalcFormLength(), CalcFormHeight()),
			style = wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER)
		wx.Frame.CenterOnScreen(self)
		panel = wx.Panel(self, -1)
		BackGroundColour = (233, 228, 214)
		panel.SetBackgroundColour(BackGroundColour)
		font16 = wx.Font(16, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font12 = wx.Font(12, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font10 = wx.Font(10, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		
		self.StatusBar = wx.StatusBar(self, -1)
		self.StatusBar.SetFieldsCount(4)
		
		self.SettingsFacilityText = wx.StaticText(panel, -1, u'Facility', pos = (15, 25), size = (120, 30))
		self.SettingsFacilityText.SetFont(font16)

		self.SettingsFacilityTextCtrl = wx.TextCtrl(panel, -1, '', pos = (150, 25), size = (175, -1))
		self.SettingsFacilityTextCtrl .SetFont(font12)
		self.SettingsFacilityTextCtrl.SetMaxLength(MAX_STRING_LENGTH)
		self.SettingsFacilityTextCtrl.Value=GlobalFacilityText

		self.SettingsCommandText = wx.StaticText(panel, -1, u'Configure Commands', pos = (15, 300), size = (120, 30))
		self.SettingsCommandText.SetFont(font16)

		self.SettingsCommandTextCtrl = wx.TextCtrl(panel, -1, '', pos = (15, 325), size = (175, -1), style = wx.TE_PROCESS_ENTER)
		self.SettingsCommandTextCtrl .SetFont(font12)
		self.SettingsCommandTextCtrl.SetMaxLength(MAX_STRING_LENGTH)
		self.Bind(wx.EVT_TEXT_ENTER, self.on_SettingsCommandTextCtrl_Enter, self.SettingsCommandTextCtrl)
		
		# self.SettingsDrText = wx.StaticText(panel, -1, u'Doctors', pos = (15, 75), size = (120, 30))
		# self.SettingsDrText.SetFont(font16)

		# self.SettingsDrList = wx.TextCtrl(parent = panel, id = -1, pos = (150, 75), size = (175, 100), style = wx.TE_MULTILINE)
		# self.SettingsDrList .SetFont(font12)
		# self.SettingsDrList.AppendText(u'Dr. Bang\n')
		# self.SettingsDrList.AppendText(u'Dr. Camuti\n')
		# self.SettingsDrList.AppendText(u'Dr. Bourgelat\n')
		# self.SettingsDrList.AppendText(u'Dr. Burgess\n')
		# self.SettingsDrList.AppendText(u'Dr. Ashford\n')
		
		# self.SettingsMedText = wx.StaticText(panel, -1, u'Treatments', pos = (15, 225),size = (120, 30))
		# self.SettingsMedText.SetFont(font16)

		# self.SettingsMedList  = wx.TextCtrl(panel, -1, pos = (150, 225), size = (175, 100), style = wx.TE_MULTILINE)
		# self.SettingsMedList.SetFont(font12)
		# self.SettingsMedList.AppendText(u'Interceptor\n')
		# self.SettingsMedList.AppendText(u'Lollypop\n')
		# self.SettingsMedList.AppendText(u'Pat on the Head\n')
		# self.SettingsMedList.AppendText(u'Thorzine\n')
		
		self.SettingsSaveButton = wx.Button(panel, -1, label = 'Save', pos = (300, 350), size = (150, 50))
		self.SettingsSaveButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_SettingsSaveButton, self.SettingsSaveButton)
				
		self.SettingsBackButton = wx.Button(panel, -1, label = 'Cancel', pos = (450, 350), size = (150, 50))
		self.SettingsBackButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_SettingsBackButton, self.SettingsBackButton)
		
	def on_SettingsCommandTextCtrl_Enter(self,event):
		global GlobalSerialPort
		if str(self.SettingsCommandTextCtrl.GetValue())[:3] == 'new':
			CapWriteString(GlobalSerialPort, 'T\r')
			now = datetime.datetime.now()
			CapWriteString(GlobalSerialPort, 'B:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}:{:0>2}\r'.format(now.second, now.minute, now.hour, now.day, now.weekday(), now.month, now.year-2000))
			CapWriteString(GlobalSerialPort, 'N\r')	
			self.SettingsCommandTextCtrl.SetValue('')
		elif str(self.SettingsCommandTextCtrl.GetValue())[:4] == 'demo':
			CapWriteString(GlobalSerialPort, 'AR\r')
			CapWriteString(GlobalSerialPort, 'AR\r')
			self.SettingsCommandTextCtrl.SetValue('')
		elif str(self.SettingsCommandTextCtrl.GetValue())[:4] == 'age':
			
			self.SettingsCommandTextCtrl.SetValue('{0:.2f} Days'.format(CapBatteryAge(GlobalSerialPort)))
		
	def on_SettingsSaveButton(self,event):
		global GlobalFacilityText
		GlobalFacilityText = self.SettingsFacilityTextCtrl.Value
		self.Hide()
		frame.Show()	
		
	def on_SettingsBackButton(self,event):
		self.SettingsCommandTextCtrl.SetValue('')
		self.Hide()
		frame.Show()			
	
class MainWindow(wx.Frame):
	""" real main"""
	PreviousComPort = ''
	def __init__(self):
		global GlobalSerialPort
		global GlobalFacilityText
		wx.Frame.__init__(self, None, wx.ID_ANY, 'CapCom ', style = wx.DEFAULT_FRAME_STYLE ^ wx.RESIZE_BORDER,
			size=(CalcFormLength(), CalcFormHeight()))# + SWVer, )
		panel = wx.Panel(self, -1)
		wx.Frame.CenterOnScreen(self)		
		BackGroundColour = (233, 228, 214)
		panel.SetBackgroundColour(BackGroundColour)
		self.Bind (wx.EVT_IDLE, self.OnIdle)
		self.WritePanel = WriteWindow(self)
		self.ReadPanel = ReadWindow(self)
		self.SettingsPanel = SettingsWindow(self)		
		
		font16 = wx.Font(16, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font12 = wx.Font(12, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)
		font10 = wx.Font(10, wx.FONTFAMILY_SWISS, wx.NORMAL, wx.NORMAL)

		# Main Page		
		self.timer = wx.Timer(self)
		self.Bind(wx.EVT_TIMER, self.OnTimer, self.timer)
		self.timer.Start(5000)
		
		self.StatusBar = wx.StatusBar(self, -1)
		self.StatusBar.SetFieldsCount(4)
		
		self.MainLogoText = wx.StaticText(panel, -1, u'cCap', pos = (180, 100), size = (120, 30))
		self.MainLogoText.SetFont(wx.Font(72, wx.FONTFAMILY_SWISS, wx.ITALIC, wx.NORMAL))
		
		self.ReadPageGotoButton = wx.Button(panel, -1, label = 'Read Cap', pos = (0, 350), size = (150, 50))
		self.ReadPageGotoButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_ReadPageGotoButton, self.ReadPageGotoButton)

		self.WritePageGotoButton = wx.Button(panel, -1, label = 'Write Cap', pos = (150, 350), size = (150, 50))
		self.WritePageGotoButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_WritePageGotoButton, self.WritePageGotoButton)

		self.SettingsPageGotoButton = wx.Button(panel, -1, label = 'Settings', pos = (300, 350), size = (150, 50))
		self.SettingsPageGotoButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_SettingsPageGotoButton, self.SettingsPageGotoButton)

		self.ExitButton = wx.Button(panel, -1, label = 'Exit', pos = (450, 350), size = (150, 50))
		self.ExitButton.SetFont(font16)
		self.Bind(wx.EVT_BUTTON, self.on_ExitButton, self.ExitButton)
		# end of GUI initialization

		# load settings
		if os.path.exists('CapCom.INI'):
			f = open('CapCom.INI', 'r')
			line = f.readline()
			self.PreviousComPort = line.rstrip()
			line = f.readline()
			GlobalFacilityText = line.rstrip()
			# ReadWindow.ReportDestination.SetValue(line.rstrip())
			# line = f.readline()
			# line = line.rstrip()
			# if line == 'PDF' or line == 'Excel' or line == 'RawData':
				# self.ReportFormatChoice.SetStringSelection(line)
			# else:
				# self.ReportFormatChoice.SetStringSelection('Text')
			# line = f.readline()
			# line = line.rstrip()
			f.close()
		else:
			self.ReadPanel.ReportDestination.SetValue(os.getcwd())
			self.ReadPanel.ReportFormatChoice.SetStringSelection('PDF')

		BaseStationList = FindBaseStation()
		GlobalSerialPort = BaseStationList[0]
		SerialPortName = str(BaseStationList[0])
		if SerialPortName.find('No') == 0:
			dlg = wx.MessageDialog(None, GlobalSerialPort, 'Error', wx.OK)
			DialogReturn = dlg.ShowModal()
			self.Close(True)
		else:
			if platform.system() == "Windows":
				SerialPortName = 'COM' + SerialPortName
			self.StatusBar.SetStatusText(SerialPortName,0)
			self.ReadPanel.StatusBar.SetStatusText(SerialPortName,0)
			self.WritePanel.StatusBar.SetStatusText(SerialPortName,0)
			self.SettingsPanel.StatusBar.SetStatusText(SerialPortName,0)
			BaseStationTitle = BaseStationList[1][4:].strip()
			BaseStationTitle = BaseStationTitle.replace(':', ' ' )
			self.StatusBar.SetStatusText(BaseStationTitle,1)
			self.ReadPanel.StatusBar.SetStatusText(BaseStationTitle,1)
			self.WritePanel.StatusBar.SetStatusText(BaseStationTitle,1)
			self.SettingsPanel.StatusBar.SetStatusText(BaseStationTitle,1)
			self.OnTimer(wx.EVT_TIMER)
			
	def on_WritePageGotoButton(self, event):
		self.WritePanel.WriteFacilityTextCtrl.Value = GlobalFacilityText
		self.WritePanel.Show()
		frame.Hide()

	def on_SettingsPageGotoButton(self, event):
		self.SettingsPanel.SettingsFacilityTextCtrl.Value = GlobalFacilityText
		self.SettingsPanel.Show()
		frame.Hide()

	def on_ReadPageGotoButton(self, event):
		self.ReadPanel.Show()
		frame.Hide()
  
	def OnTimer(self, event):
		global GlobalSerialPort
		try:
			PillCapTitleList = CapPing(GlobalSerialPort)
			if len(PillCapTitleList) < 3:
				CapTitle = 'No Cap'
				CapStatusStr = ''
			else:
				CapTitle = PillCapTitleList[1] + ' '+ PillCapTitleList[2].strip()
				CapStatusStr = CapStatus[PillCapTitleList[3].strip()]
		except:
				CapTitle = 'Read Failure'
				CapStatusStr = ''
			
		self.StatusBar.SetStatusText(CapTitle,2)
		self.ReadPanel.StatusBar.SetStatusText(CapTitle, 2)
		self.WritePanel.StatusBar.SetStatusText(CapTitle, 2)
		self.SettingsPanel.StatusBar.SetStatusText(CapTitle, 2)
		self.StatusBar.SetStatusText(CapStatusStr,3)
		self.ReadPanel.StatusBar.SetStatusText(CapStatusStr, 3)
		self.WritePanel.StatusBar.SetStatusText(CapStatusStr, 3)
		self.SettingsPanel.StatusBar.SetStatusText(CapStatusStr, 3)		
		
	def OnIdle(self, event):
		pass

	def on_ExitButton(self, event):
		""" exit, stop threads, save report save location and type"""
		global GlobalFacilityText
		# save settings
		f = open('CapCom.INI', 'w')
		f.write(self.PreviousComPort + '\n')
		f.write(GlobalFacilityText + '\n')
		# f.write(self.ReportDestination.GetValue() + '\n')
		# f.write(self.ReportFormatChoice.GetValue() + '\n')
		f.close()
		self.Close(True)
		
if __name__ == '__main__':
	app = wx.App(False)
	frame = MainWindow()
	frame.Show()
	app.MainLoop()
