#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import re

FUNC_START_REGEXP = r'void\s?\w'
FUNC_END_REGEXP = r'}'

content = ""
with open(sys.argv[1], 'r') as f:
    content = f.read()

continues = True
while continues == True:
    nest = 0
    func = ""
    result = ""
    result_header = ""
    recording = False
    continues = False
    for l in content.split("\n"):
        if re.search(FUNC_START_REGEXP, l):
            if nest == 1:
                recording = True

            if nest == 2: continues = True
            nest += 1
        elif re.search(FUNC_END_REGEXP, l):
            nest -= 1
            if nest == 1:
                recording = False
                result_header += l + "\n"
                continue
        
        if recording == True:
            result_header += l + "\n"
        else:
            result +=  l + "\n"
    content = result_header + result
    

#print(content)

with open(sys.argv[1]+'.new', 'w') as f:
    f.write(content)
    
