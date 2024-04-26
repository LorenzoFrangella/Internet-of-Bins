from copy import deepcopy
from random import random
from typing import List

from matplotlib import pyplot as plt

listenDistance = 0.11

class Message:
    def __init__(self, x, y, id, type = None, level = None, maxKnowLevel = None, gateway = None, backupGateways = None, content = None):
        self.x = x
        self.y = y
        self.id = id
        self.type = type
        self.level = level
        self.maxKnownLevel = maxKnowLevel
        self.gateway = gateway
        self.backupGateways = backupGateways
        self.content = content
        


class Node:
    def __init__(self, id, x, y, wifi = False):
        self.id = id
        self.wifi = wifi
        self.x = x
        self.y = y
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
        if wifi:
            self.gateway = id
            self.level = 0
            self.maxKnownLevel = 1
        else:
            self.gateway = None
            self.level = None
            self.maxKnownLevel = 100
        self.backupGateways = {}
        self.status = True
        self.void = False
        
    def printStatus(self):
        print(self.id)
        print(self.wifi)
        print(self.status)
        print(self.gateway)
        print(self.level)
        print(self.maxKnownLevel, flush=True)
    
    def hour(self):
        if self.wifi:
            #print("Gateway : " + str(self.id), flush=True)
            #print(self.alerts, flush=True)
            pass
        if random() <= 0.01:
            self.status = False
        if not self.status:
            if random() <= 0.1:
                self.status = True
        
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
        if len(allNodes) == 1 and self.level != None and not self.wifi:
            print('Node ' + str(self.id) + ' returning to void')
            self.void = True
            self.level = None
            self.gateway = None
            self.maxKnownLevel = None
        #define node listening
        if not self.status:
            return (None, None)
        elif self.level == None:
            return (-2, -2)
        elif self.level == 0:
            return (-1, self.maxKnownLevel*2-1)
        else:
            return (self.level-1, ((self.maxKnownLevel-self.level)*2)-1)
        
    def speaks(self):
        self.alerts = list(set(self.alerts))
        content = {
                'status': self.cluster['self']['alert'],
                'alerts' : self.alerts
            }
        return Message(
            self.x, self.y, self.id, level = self.level, gateway=self.gateway, content= content, maxKnowLevel=self.maxKnownLevel
        )
    
    def listen(self, messages: List[Message]):
        '''print('Node :' + str(self.id) + " Messages: ", flush=True)
        print("     " + str(messages), flush=True)'''
        cluster = self.cluster.keys()
        for message in messages:
            if message.id != self.id:
                distance = ((self.x - message.x)**2 + (self.y - message.y)**2)**0.5 + random()/20
                if distance <= listenDistance:
                    if message.id not in cluster:
                        self.cluster[message.id] = {'alert':message.content['status'], 'missing':0}
                    else:
                        try:
                            self.notSeen.remove(message.id)
                        except:
                            pass
                        self.cluster[message.id] = {'alert':message.content['status'], 'missing':0}
                    if self.level == None or message.level < self.level - 1:
                            print(str(self.id)+' moved from GAT ' +str(self.gateway) + ' lv ' + str(self.level) + ' to GAT ' + str(message.gateway) + ' lv ' + str(message.level + 1))
                            self.void = False
                            self.level = message.level + 1
                            self.gateway = message.gateway
                            self.maxKnownLevel = self.level + 1
                    if message.maxKnownLevel > self.maxKnownLevel and message.gateway == self.gateway:
                        self.maxKnownLevel = message.maxKnownLevel
                    self.alerts = message.content['alerts']
        

n   = 200
nodes = []
listenWindows = {}
speakingWindows = {}
gateways = {}
nodes.append(Node(0, wifi=True, x = 0.2, y=0.2))
nodes.append(Node(1, wifi=True, x = 0.8, y=0.8))
nodes.append(Node(2, wifi=True, x = 0.5, y=0.5))
gateways[0] = {'x':[], 'y':[], 'l': [], 'color': 'Reds'}
gateways[1] = {'x':[], 'y':[], 'l': [], 'color': 'Greens'}
gateways[2] = {'x':[], 'y':[], 'l': [], 'color': 'Blues'}

gateways[-1] = {'x':[], 'y':[], 'l': [], 'color': 'pink'}
gateways[-2] = {'x':[], 'y':[], 'l': [], 'color': 'Grays'}

listenWindows[0] = []
speakingWindows[0] = []
listenWindows[1] = []
speakingWindows[1] = []
listenWindows[2] = []
speakingWindows[2] = []

for i in range (3, n):
    wifi = False
    nodes.append(Node(i, random(), random(), wifi))
    listenWindows[i] = []
    speakingWindows[i] = []
    
#print(nodes)
hour = 0
while True:
    #Passing hours and setting speaking and listening
    for node in nodes:
        firstWindows, secondWindows = node.hour()
        if firstWindows == None:
            pass
        elif firstWindows == -2:
            for i in range (0, n):
                listenWindows[i].append(node.id)
        elif firstWindows == -1:
            #When I speak, I might listen
            speakingWindows[0].append(node.id)
            listenWindows[0].append(node.id)
            listenWindows[secondWindows].append(node.id)
        else:
            listenWindows[firstWindows].append(node.id)
            #When I speak, I might listen
            speakingWindows[firstWindows+1].append(node.id)
            listenWindows[firstWindows+1].append(node.id)
            
            listenWindows[secondWindows].append(node.id)
            #When I speak, I might listen
            speakingWindows[secondWindows+1].append(node.id)
            listenWindows[secondWindows+1].append(node.id)

    #print(listenWindows)
    
    #actuall speaking
    for i in speakingWindows.keys():
        window = speakingWindows[i]
        messages = []
        for node in window:
            messages.append(nodes[node].speaks())
        for listener in listenWindows[i]:
            nodes[listener].listen(messages)
            
        speakingWindows[i] = []
        listenWindows[i] = []
    
    #plotting
    for node in nodes:
        if node.gateway != None:
            gateways[node.gateway]['x'].append(node.x)
            gateways[node.gateway]['y'].append(node.y)
            gateways[node.gateway]['l'].append(10 - node.level)
        elif not node.status:
            gateways[-2]['x'].append(node.x)
            gateways[-2]['y'].append(node.y)
            gateways[-2]['l'].append(1)
        else:
            gateways[-1]['x'].append(node.x)
            gateways[-1]['y'].append(node.y)
            gateways[-1]['l'].append(0.5)
            
        plt.text(node.x, node.y, str(node.id), size=5)
        
    if hour % 6 == 0:
    
        for gateway in gateways.keys():
            cmap = plt.cm.get_cmap(gateways[gateway]['color'])
            if gateway >= 0:
                norm = plt.Normalize(vmin=0, vmax=10)
            else:
                norm = plt.Normalize(vmin=0, vmax=1)
            plt.scatter(gateways[gateway]['x'], gateways[gateway]['y'], c=cmap(norm(gateways[gateway]['l'])))
            gateways[gateway]['x'] = []
            gateways[gateway]['y'] = []
            gateways[gateway]['l'] = []
        
        plt.title('Hour ' + str(hour))
        plt.show()
    
    hour += 1
        
            
            