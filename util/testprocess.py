import subprocess

pipe = subprocess.Popen("cmd pause", shell=True, bufsize=5, stdout=subprocess.PIPE).stdout
print(pipe.read())
