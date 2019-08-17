#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys

bind = {}
contentl = []

with open(sys.argv[1]) as f:
    content = f.readlines()

    for line in content:
        pline = line
        line = line.strip()
        try:
            if line[0] == '{' and line[1] == '!' and line[-2] == '!' and line[-1] == '}':
                tmp = line.split('<>')
                bind[tmp[0].strip()[2:].strip()] = { 'args':list(map(lambda x: x.strip(), tmp[1:-1])), 'body':tmp[-1][:-2].strip() }

            elif line[0] == '{' and line[1] == '-':
                tmp = line.split('<>')
                args = list(map(lambda x: x.strip(), tmp[1:-1]))
                args.append(tmp[-1][:-2].strip())
                name = tmp[0].strip()[2:].strip()
                tmp1 = bind[name]['body']
                for i in range(0, len(args)):
                    tmp1 = tmp1.replace('#'+bind[name]['args'][i], args[i])
                contentl.append('// ' + line)
                contentl.append(tmp1)
                
            else:
                contentl.append(line)
                    
        except IndexError:
            contentl.append(pline)

    with open(sys.argv[1] + '.cc', 'w') as fw:
        fw.writelines(list(map(lambda x: x + '\n', contentl)))
