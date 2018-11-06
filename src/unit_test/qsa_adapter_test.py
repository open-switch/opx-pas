#!/usr/bin/python
import subprocess
import sys



p = subprocess.Popen(["opx-show-transceivers","summary"], stdout=subprocess.PIPE, stderr=subprocess.PIPE)
out, err = p.communicate()
out_list = str(out).splitlines()

# Remove output title
out_list.pop(0)

for x in out_list:
    cols = x.split()
    presence = cols[1]
    port = cols[0]
    cps_str = "cps_get_oid.py observed/base-pas/media port="+port+" | grep -e qsa-adapter"
    p = subprocess.Popen(cps_str, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = p.communicate()
    qsa_type = str(out).split()[-1]
    if presence != "Not" and qsa_type == "0":
            print "Error on port "+ port +". QSA type " + qsa_type + " found on empty port"
            sys.exit(1)
print "All ports checked successfully"
