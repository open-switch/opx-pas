#!/usr/bin/python
#
# Copyright (c) 2018 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.
#

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
