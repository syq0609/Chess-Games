#-*-coding:utf-8-*-

import os
import re
import sys 


class LogFile(object):
    def __init__(self, path):
        self.name = path
        self.lines = []
        self.money = 0 
        f = open(path)
        for l in f:
            if l.find('moneyChange') == -1: 
                continue
            #print(l)
            self.lines.append(l)

    def grepMoney(self):
        for l in self.lines:
            ret = None
            if len(sys.argv) > 2:
                cuid = sys.argv[2]
                keyword = 'cuid={}'.format(cuid)
                ret = re.search(keyword, l)
                if not ret:
                    continue

            ret = re.search('moneyChange=-\d+', l)
            if not ret:
                continue
            print(l)
            begin, end = ret.span()
            moneystr = l[begin + 12: end]
            money = int(moneystr)
            self.money += money



class Logs(object):
    def __init__(self, directory):
        self.directory = directory
        self.money = 0 


    def grepMoney(self):
        for filename in os.listdir(self.directory):
            path = os.path.join(self.directory, filename)
            if os.path.isfile(path):
                if path.find('lobby') == -1 or path.find(sys.argv[1]) == -1: 
                    continue
#                print(path)
                f = LogFile(path)
                f.grepMoney()
                self.money += f.money
        print("date: {}, total: {}".format(sys.argv[1], self.money))





def test():
    LogFile("./script/lobby-1.log")


#test()


def main():
    if len(sys.argv) < 2:
        print "missing date parameter"
        return
    logs = Logs("./log")
    logs.grepMoney()


main()

