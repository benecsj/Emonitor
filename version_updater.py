
import subprocess
import re
from optparse import OptionParser

def git_version():
    p = subprocess.Popen(["git", "log", '--date=iso'], stdout=subprocess.PIPE)
    out, err = p.communicate()
    m = re.search('\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}', out)
    date = m.group(0).split()[0]
    version = out.count("commit")
    print(str(version)+" "+date)
    return str(version)+" "+date

usage = "usage: %prog -f filepath"
parser = OptionParser(usage=usage)
parser.add_option("-f", default='data/version')
(options, args) = parser.parse_args()
path = options.f
with open(path, 'w+') as f:
    f.write(git_version())