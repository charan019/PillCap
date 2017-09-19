""" Generate reports.  PDF, XL, Text and raw data formats
Version Date        Changes
0.94    12-21-10    Raw data output checks for 'ige', selects correct channel
0.95    02-28-11    In XL output use Plexigen logo if logo not found

"""
import os.path
import stat
from datetime import datetime
from reportlab.lib.pagesizes import letter, inch
from reportlab.platypus import SimpleDocTemplate, Table, TableStyle
from reportlab.lib import colors
from xlwt import Workbook, Borders, Alignment, XFStyle
import ComSubs

# pylint: disable-msg=C0103
# pylint: disable-msg=C0301

def GetReportDestination(ReportRootDirectory):
	""" generate folder based date and sub folder on sequential numbering"""
	# first create folders using today's date
	t = datetime.now()
	# if path doesn't exsist create it
	if not os.path.isdir(ReportRootDirectory + t.strftime("\%Y-%m-%d")):
		os.mkdir(ReportRootDirectory + t.strftime("\%Y-%m-%d"))
	# look in folder for folders with numbers, find highest number, add one,
	# thats our folder for the reports
	i = 0
	while os.path.isdir(ReportRootDirectory + t.strftime("\%Y-%m-%d\\") + str(i)):
		i += 1
	SaveDirectory = ReportRootDirectory + t.strftime("\%Y-%m-%d\\") + str(i)
	os.mkdir(SaveDirectory)
	return SaveDirectory

def GenerateReport(SaveDirectory, ReportType, Info, Data):
	""" send data and patient info to the selected report type generator """
	if ReportType == 'PDF':
		GeneratePDFReport(SaveDirectory, Info, Data)
	elif ReportType == 'Excel':
		GenerateXLReport(SaveDirectory, Info, Data)
	else: # Text
		GenerateTextReport(SaveDirectory, Info, Data)

def GenerateXLReport(Directory, Info, Data):
	"""  create XL report """
	t = datetime.now()
	DateTimeNowStr = t.strftime("%Y-%m-%d %H:%M")

	wb = Workbook()
	ws = []

	N = 0 # number of controls

	ws.append(wb.add_sheet(Info['Patient']))

	borders = Borders()
	borders.left = Borders.THIN
	borders.right = Borders.THIN
	borders.top = Borders.THIN
	borders.bottom = Borders.THIN
	aligncenter = Alignment()
	aligncenter.horz = Alignment.HORZ_CENTER
	BorderStyle = XFStyle()
	BorderStyle.borders = borders
	BorderStyle.alignment = aligncenter

	TableHeaderStyle = XFStyle()
	TableHeaderStyle.alignment = aligncenter

	alignleft = Alignment()
	alignleft.horz = Alignment.HORZ_LEFT
	LeftDateStyle = XFStyle()
	LeftDateStyle.num_format_str = 'M/D/YY'
	LeftDateStyle.alignment = alignleft

	LeftStyle = XFStyle()
	LeftStyle.alignment = alignleft

	alignright = Alignment()
	alignright.horz = Alignment.HORZ_RIGHT
	RightStyle = XFStyle()
	RightStyle.alignment = alignright

	# column 'A'
	ws[N].col(0).width = 4000
	ws[N].write(0, 0, 'cCap Compliance Report')
	ws[N].write(1, 0, 'Report Date:')
	ws[N].write(2, 0, "Facility:")
	ws[N].write(3, 0, "Doctor:")
	ws[N].write(4, 0, 'Client:')
	ws[N].write(5, 0, 'Patient:')
	ws[N].write(7, 0, "Treatment:")
	ws[N].write(8, 0, 'Dose Frequency')
	ws[N].write(9, 0, 'Dose Count:')
	ws[N].write(10, 0, 'Compliance Rate:')
	ws[N].write(12, 0, 'Time Points:')
	ws[N].write(13, 0, 'Month')	
	
	# column 'B'
	ws[N].col(1).width = 4000
	ws[N].write(1, 1, DateTimeNowStr)
	ws[N].write(2, 1, Info['Facility'])
	ws[N].write(3, 1, Info['Doctor'])
	ws[N].write(4, 1, Info['Client'])
	ws[N].write(5, 1, Info['Patient'])
	ws[N].write(7, 1, Info['Treatment'])
	ws[N].write(8, 1, Info['DosePattern'])
	ws[N].write(9, 1, Info['DoseCount'])
	# calculate compliance
	DosesTaken = int(Info['DoseCount'])
	for lines in Data:
		DataSubList = ComSubs.RxdStrParse(lines)
		if int(DataSubList[0]) == 0 and int(DataSubList[1]) == 0 and int(DataSubList[2]) == 0 and int(DataSubList[3]) == 0:
			DosesTaken -= 1
	ws[N].write(10, 1, '{0:.1f} %'.format(100*DosesTaken/int(Info['DoseCount'])))
	ws[N].write(13, 1, 'Day')

	ws[N].col(2).width = 4000
	ws[N].write(13, 2, 'Hour')
	ws[N].col(3).width = 4000
	ws[N].write(13, 3, 'Minute')

	i=0
	for lines in Data:
		DataSubList = ComSubs.RxdStrParse(lines)
		ws[N].write(14+i, 0, str(DataSubList[3]))
		ws[N].write(14+i, 1, str(DataSubList[2]))
		ws[N].write(14+i, 2, str(DataSubList[1]))
		ws[N].write(14+i, 3, str(DataSubList[0]))
		i = i+1
							
	i = 0
	Filename = Directory + "/" + Info['Doctor'] + Info['Client'] + Info['Patient'] + t.strftime("-%Y-%m-%d-") + str(i) + '.xls'
	while os.path.exists(Filename):
		i = i + 1
		Filename = Directory + "/" + Info['Doctor'] + Info['Client'] + Info['Patient'] + t.strftime("-%Y-%m-%d-") + str(i) + '.xls'
	
	wb.save(Filename)
	os.chmod(Filename, stat.S_IREAD)

def GeneratePDFReport(Directory, Info, Data):
	""" create PDF report """
	MARGIN_SIZE = 0.5 * inch
	t = datetime.now()
	DateTimeNowStr = t.strftime("%Y-%m-%d %H:%M")

	# calculate compliance
	DosesTaken = int(Info['DoseCount'])
	for lines in Data:
		DataSubList = ComSubs.RxdStrParse(lines)
		if int(DataSubList[0]) == 0 and int(DataSubList[1]) == 0 and int(DataSubList[2]) == 0 and int(DataSubList[3]) == 0:
			DosesTaken -= 1
	
	if os.path.exists('Logo.jpg'):
		LogoFile = 'Logo.jpg'
		
	t = datetime.now()
	
	i = 0
	Filename = Directory + "/" + Info['Doctor'] + Info['Client'] + Info['Patient'] + t.strftime("-%Y-%m-%d-") + str(i) + '.pdf'
	while os.path.exists(Filename):
		i = i + 1
		Filename = Directory + "/" + Info['Doctor'] + Info['Client'] + Info['Patient'] + t.strftime("-%Y-%m-%d-") + str(i) + '.pdf'
		
	doc = SimpleDocTemplate(Filename, topMargin = MARGIN_SIZE, bottomMargin = MARGIN_SIZE, pagesize=letter)
	elements = []
	
	Info = [["cCap Compliance Report", 	''],
			['', 							''],
			['Report Date:',     	DateTimeNowStr],
			["Facility:", 				Info['Facility']],
			['Doctor:',         		Info['Doctor']],
			['Client:',       			Info['Client']],
			['Patient:',     			Info['Patient']],
			['', 							''],
			['Treatment:',        	Info['Treatment']],
			['Dose Frequency:', Info['DosePattern']],
			['Dose Count:',         Info['DoseCount']],
			['Compliance Rate:',str('{0:.1f} %'.format(100*DosesTaken/int(Info['DoseCount'])))]]
		  
	t2 = Table(Info, 4*[1.75*inch], 12*[0.2*inch])
	t2.setStyle(TableStyle([('FONT', (0, 0), (-1, -1), 'Helvetica', 10), ('ALIGN', (0, 0), (-1, -1), 'LEFT')]))
	elements.append(t2)
			
	TableHeader = [['', 'Month', 'Day', 'Hour', 'Minute']]
	i=0
	for lines in Data:
		DataSubList = ComSubs.RxdStrParse(lines)
		TableHeader.append(['', str(DataSubList[3]), str(DataSubList[2]), str(DataSubList[1]), str(DataSubList[0])])
		i = i+1
	t = Table(TableHeader, 6*[0.75*inch], (i+1)*[0.2*inch])
	
	t.setStyle(TableStyle([('INNERGRID', (1, 1), (-1, -1), 0.35, colors.black),
					  ('FONT', (1, 1), (-1, -1), 'Helvetica', 10),
					  ('FONT', (0, 0), (-1, 0), 'Helvetica-Bold', 10),
					  ('FONT', (0, 0), (0, -1), 'Helvetica-Bold', 10),
					  ('ALIGN', (1, 0), (-1, -1), 'CENTER'),
					  ('ALIGN', (0, 0), (0, -1), 'RIGHT'),
					  ('BOX', (1, 1), (-1, -1), 0.35, colors.black)]))

	elements.append(t)
	doc.build(elements)    # write the document to disk

def GenerateTextReport(Directory, Info, Data):
	""" create TXT report """
	t = datetime.now()
	DateTimeNowStr = t.strftime("%Y-%m-%d %H:%M")

	i = 0
	Filename = Directory + "/" + Info['Doctor'] + Info['Client'] + Info['Patient'] + t.strftime("-%Y-%m-%d-") + str(i) + '.txt'
	while os.path.exists(Filename):
		i = i + 1
		Filename = Directory + "/" + Info['Doctor'] + Info['Client'] + Info['Patient'] + t.strftime("-%Y-%m-%d-") + str(i) + '.txt'

	f = open(Filename, 'w')
	f.write('cCap COMPLIANCE REPORT')
	f.write('\n--------------------------------------------------------')
	f.write('\nReport Date:		' + DateTimeNowStr)
	f.write('\nFacility:			' + Info['Facility'])
	f.write('\nDoctor:				' + Info['Doctor'])
	f.write('\nClient:				' + Info['Client'])
	f.write('\nPatient:			' + Info['Patient'])

	f.write('\n--------------------------------------------------------')
	f.write('\nTreatment:       	' + Info['Treatment'])
	f.write('\nDose Frequency:		' + Info['DosePattern'])
	f.write('\nDose Count:			' + str(Info['DoseCount']))

	# calculate compliance
	DosesTaken = int(Info['DoseCount'])
	for lines in Data:
		DataSubList = ComSubs.RxdStrParse(lines)
		if int(DataSubList[0]) == 0 and int(DataSubList[1]) == 0 and int(DataSubList[2]) == 0 and int(DataSubList[3]) == 0:
			DosesTaken -= 1
	f.write('\nCompliance Rate:		{0:.2f} %' .format(DosesTaken/int(Info['DoseCount'])))
		
	f.write('\n')
	f.write('\nTime Points:')
	f.write('\nMon Day Hr  Min\n')

	for lines in Data:
		DataSubList = ComSubs.RxdStrParse(lines)
		if int(DataSubList[0]) == 0 and int(DataSubList[1]) == 0 and int(DataSubList[2]) == 0 and int(DataSubList[3]) == 0:
			f.write('Missed Dose\n')
		else:
			f.write(str(DataSubList[3]) + '	' +str(DataSubList[2]) + '	' +str(DataSubList[1]) + '	'+ str(DataSubList[0]) + '\n')

	f.close()
	os.chmod(Filename, stat.S_IREAD)




