#!/usr/bin/python3

#-*-coding:utf-8-*-


import os
import re


class LogFile(object):
    def __init__(self, path):
        self.name = path
        self.lines = []
        f = open(path)
        for l in f:
            if l.find('deal deck') == -1: 
                continue
            self.lines.append(l)
            #print(l)

    def grepLong(self):
        for l in self.lines:
            ret = re.search('cards=\[.+\]', l)
            if not ret:
                continue
            begin, end = ret.span()
            cardstrs = l[begin + 7: end - 2].split(',')
            #print(cardstrs)
            ranks = [(int(cardstr) - 1) % 13 for cardstr in cardstrs]
            suits = [(int(cardstr) - 1) // 13 for cardstr in cardstrs]

            isStright = ranks == [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12] 
            isFlush   = len(set(suits)) == 1

            if isStright:
                if isFlush:
                    print("发牌, 青龙")
                else:
                    print("发牌, 一条龙")
                print(l)



class Logs(object):
    def __init__(self, directory):
        self.directory = directory


    def grepLong(self):
        for filename in os.listdir(self.directory):
            path = os.path.join(self.directory, filename)
            if os.path.isfile(path):
                if path.find('lobby') == -1: 
                    continue
                print(path)
                f = LogFile(path)
                f.grepLong()




def test():
    LogFile("./script/lobby-1.log")


#test()


logs = Logs("./log")
logs.grepLong()

