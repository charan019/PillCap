To build run Py2build.bat which runs the command "python setupPy2exe.py py2exe"
Requires:
Python 2.7.1
xlrd 0.7.1
xlwt 0.7.2
Reportlab 2.5
PIL 1.1.7
wxPython 2.8.11.0
Pyserial 2.5
crcmod 1.7
py2exe 0.6.9
InnoIDE 1.0.0.0070

Using PyLint 0.21.4 (0.22.0 doesn't work)

After build, copy contents of "to dist" folder to "dist" folder.
Open Installer Script.iss in InnoIDE (double click)
Compile
Close
Close InnoIDE
Installer file "PSCom Installer.exe" (~5.4 MB) found in "Output" folder
