ZIPEXE = r'"C:\Program Files\7-Zip\7z.exe"'
ZIPEXEZIP = ZIPEXE + ' a '

import os
import subprocess

def runcmd(cmd):
    try:
        p = subprocess.Popen(cmd)
        p.wait()
    except:
        print('fail to run cmd: ', cmd)    

def zipfiles():
    files = os.listdir('.')
    cmd = ZIPEXEZIP + ' outzip.7z .\*'
    runcmd(cmd)

zipfiles()
input('enter to exit......')
    
