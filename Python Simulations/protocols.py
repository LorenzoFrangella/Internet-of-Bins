from copy import deepcopy
from random import random
from typing import List

class Message:
    def __init__(self, id = None, type = None, level = None, maxKnowLevel = None, gateway = None, backupGateways = None, content = None):
        self.id = id
        self.type = type
        self.level = level
        self.maxKnownLevel = maxKnowLevel
        self.gateway = gateway
        self.backupGateways = backupGateways
        self.content = content
        


class Node:
    def __init__(self, id):
        self.id = id
        self.x = random()
        self.y = random()
        self.full = 0
        self.usage = random()/24
        self.cluster = {
            'self' : {
                'alert' : False,
                'missing' : 0
            }
        }
        self.alerts = []
        self.notSeen = []
        self.gateway = None
        self.backupGateways = {}
        self.level = None
        self.maxKnownLevel = 100
    
    def hour(self):
        self.full += self.usage
        if self.full >= 0.8:
            self.cluster['self']['alert'] = True
        else:
            self.cluster['self']['alert'] = False
        #adjust cluster
        for node in self.notSeen:
            self.cluster[node]['missing'] += 1
            if self.cluster[node]['missing'] == 12:
                self.cluster.pop(node, None)
        #checkCluster
        allNodes = self.cluster.keys()
        alerts = 0
        for node in allNodes:
            if self.cluster[node]['alert']:
                alerts += 1
        self.alerts = list(set(self.alerts))
        if alerts > len(allNodes)/2:
            self.alerts +=  allNodes
        else:
            for node in allNodes:
                try:
                    self.alerts.remove(node)
                except:
                    pass
        self.notSeen = deepcopy(list(allNodes))
        self.notSeen.remove('self')
        if len(allNodes) == 1:
            self.level = None
            self.gateway = None
            self.maxKnownLevel = None
        #define node listening and speaking windows
        if self.level == None:
            return (-1, -1)
        else:
            return (self.level-1, self.maxKnownLevel-self.level)
        
    def speaks(self):
        self.alerts = list(set(self.alerts))
        content = {
                'status': self.cluster['self']['alert'],
                'alerts' : self.alerts
            }
        return Message(
            id = self.id, level = self.level, gateway=self.gateway, backupGateways=self.backupGateways[0], content= content
        )
    
    def listen(self, messages: List[Message]):
        cluster = self.cluster.keys()
        for message in messages:
            if message.id not in cluster:
                self.cluster[message.id] = {'alert':message.content['status'], 'missing':0}
            else:
                self.notSeen.remove(message.id)
                self.cluster[message.id] = {'alert':message.content['status'], 'missing':0}
            if message.level - 1 < self.level:
                    self.level = message.level + 1
                    self.gateway = message.gateway
                    self.maxKnownLevel = self.level + 1
            if message.maxKnownLevel > self.maxKnownLevel and message.gateway == self.gateway:
                self.maxKnownLevel = message.maxKnownLevel
        self.alerts = message.content['alerts']
        

n= 100
nodes = []

for i in range (0, n):
    nodes.append(Node(str(i)))

while True:
    for node in nodes:
        print(node.hour())
        
            
            