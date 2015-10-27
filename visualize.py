import serial
import time
from threading import Thread

import numpy as np
from matplotlib.lines import Line2D
import matplotlib.pyplot as plt
import matplotlib.animation as animation

PORT = '/dev/cu.usbmodemfa1311'
BAUD = 115200
BINS = 128
SMOOTH = .5
RUNNING_AVG_SMOOTH = .99

class SecondLogger(object):
    def __init__(self, tracking, period_seconds):
        self.tracking = tracking
        self.period_seconds = period_seconds
        self.last_output = time.time()
        self.count = 1

    def ping(self):
        self.count += 1
        duration = time.time() - self.last_output
        if duration > self.period_seconds:
            print('{}: {}/s'.format(self.tracking, self.count / duration))
            self.last_output = time.time()
            self.count = 0

class Sampler(object):

    def __init__(self, serial_port, baud):
        print 'Opening port'
        self.serial = serial.Serial(serial_port, baud)
        print 'Port opened'

        self.lines_read = SecondLogger('Lines Read', 5)

    def read_data(self):
        while True:
            serial_data = self.serial.readline()
            # Didn't get a whole line
            if not serial_data.startswith('start'):
                continue
            try:
                parsed = [int(v) for v in serial_data.split(',')[1:]]
            except Exception as e:
                print 'Bad line: {}'.format(e)
                continue
            if len(parsed) != BINS:
                print 'Bad read. {} entries'.format(len(parsed))
                continue
            self.lines_read.ping()
            return np.array(parsed, dtype=np.int)

    def new_data(self):
        return self.read_data()

    def calibrate(self, seconds):
        print 'Calibrating for {} seconds'.format(seconds)
        end = time.time() + seconds
        running_sum, i = self.read_data(), 1
        while time.time() < end:
            self.data = self.read_data()
            running_sum += self.data
            i += 1
        print 'Calibrated over {} samples'.format(i)
        self.average = running_sum / i
        print 'Average:\n{}'.format(self.average)


class Chart(object):
    def __init__(self, ax, sampler):
        self.ax = ax
        self.sampler = sampler

        self.xdata = np.linspace(0, 20000, BINS)
        d = self.sampler.read_data()
        self.raw_data = d
        self.running_avg = d

        self.line = Line2D(self.xdata, self.raw_data)
        self.max_line = Line2D(np.array([0,0]),
                               np.array([0,244]), color='red')

        self.ax.add_line(self.line)
        self.ax.add_line(self.max_line)

        self.ax.set_ylim(-.1, 256)
        self.ax.set_xlim(0, 20000)

    def update(self, unused_param):
        new_data = self.sampler.read_data()
        self.running_avg = (self.running_avg * RUNNING_AVG_SMOOTH
                            + (1 - RUNNING_AVG_SMOOTH) * new_data)
        self.raw_data = self.raw_data * SMOOTH +  (1 - SMOOTH) * new_data
        transformed = np.clip(self.raw_data - self.running_avg, 0, 256)
        self.line.set_data(self.xdata, transformed)
        max_val = self.xdata[np.argmax(transformed)]
        self.max_line.set_data(np.array([max_val, max_val]),
                               np.array([0,244]))
        return self.line, self.max_line



def main():
    sampler = Sampler(PORT, BAUD)

    print 'Example data:\n{}'.format(sampler.read_data())

    fig, ax = plt.subplots()
    chart = Chart(ax, sampler)

    anim = animation.FuncAnimation(fig, chart.update, interval=10,
                                   blit=False)
    plt.show()

if __name__ == '__main__':
    main()
