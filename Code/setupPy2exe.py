from distutils.core import setup
import py2exe
 
setup(name='CapCom',
    version='0.30',
    url='about:none',
    author='J. SooHoo',
    package_dir={'CapCom':'.'},
    packages=['CapCom'],
    windows=['CapCom.py']
    )

setup(windows = 
        [
            {
                "script": 'CapCom.py',
                "icon_resources": [(1,"Icon.ico")]
            }
        ],
)
